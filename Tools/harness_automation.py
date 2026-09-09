from __future__ import annotations

import json
import os
import socket
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
UNREAL_MCP_DIR = SCRIPT_DIR / "unreal_mcp"

# The production file belongs at Tools/harness_automation.py, beside
# Tools/unreal_mcp/server.py.
sys.path.insert(0, str(UNREAL_MCP_DIR))
try:
    from server import READ_TOOLS, WRITE_TOOLS, call_unreal
except (ImportError, SyntaxError) as exc:
    print(f"[ERROR] Could not import Tools/unreal_mcp/server.py: {exc}")
    raise SystemExit(1) from exc


OLLAMA_URL = os.environ.get(
    "ORGANOID_OLLAMA_URL",
    "http://127.0.0.1:11434/v1/chat/completions",
)
MODEL_NAME = os.environ.get("ORGANOID_OLLAMA_MODEL", "qwen-en")
OLLAMA_TIMEOUT_SECONDS = int(os.environ.get("ORGANOID_OLLAMA_TIMEOUT", "600"))
CONTEXT_MODE = os.environ.get("ORGANOID_CONTEXT_MODE", "compact").strip().lower()
if CONTEXT_MODE not in {"compact", "full"}:
    CONTEXT_MODE = "compact"
MAX_TOOL_ROUNDS = 8
MAX_IDENTICAL_CONSECUTIVE_CALLS = 2

# Repeating a status query is legitimate while polling an asynchronous test.
REPETITION_GUARD_EXEMPT_TOOLS = {
    "unreal_get_playtest_status",
}

PROJECT_DOCUMENTS = (
    (
        "CURRENT PROJECT STATE",
        SCRIPT_DIR.parent / "PROJECT_STATE.md",
        False,
    ),
    (
        "DESIGN AUTHORITY — PROJECT_ORGANOID_CANON.md",
        UNREAL_MCP_DIR / "PROJECT_ORGANOID_CANON.md",
        True,
    ),
    (
        "IMPLEMENTATION HANDOFF — PROJECT_ORGANOID_MASTER_AI_HANDOFF.md",
        UNREAL_MCP_DIR / "PROJECT_ORGANOID_MASTER_AI_HANDOFF.md",
        True,
    ),
)

BASE_SYSTEM_PROMPT = """You are the local planning and read-only inspection assistant for Project Organoid, an Unreal Engine 5.8 third-person survival-horror RPG.

AUTHORITY AND SAFETY:
- Tom is the final creative and canon authority.
- Restored Canon v1.0 and PROJECT_ORGANOID_CANON.md control game design.
- Implementation evidence is not automatically canon.
- Do not invent missing canon or treat a proposal as approval.
- This harness exposes read and non-asset-mutating playtest tools only. Do not request or claim to perform durable writes.
- Never claim Unreal state that was not returned by a tool.

TOOL-CALLING RULES:
1. For actors near the player, call unreal_get_player_pawn first, use its returned location, and then call unreal_list_actors_near.
2. Do not call unreal_get_editor_state repeatedly for actor, pawn, collision, or gameplay questions.
3. Read and use each tool result before selecting the next tool.
4. Do not repeat an identical tool call when it already returned a usable result.
5. Use the most specific available tool.
"""

FINAL_OPERATIONAL_REMINDER = """
ACTIVE SESSION BOUNDARY:
- Read-only inspection and registered non-asset-mutating playtests are allowed.
- Durable write tools are intentionally unavailable in this harness build.
- If a task requires mutation, stop and tell Tom that a separately approved write workflow is required.
- For the diagnostic question about nearby actors: get pawn, then list actors near that location, then answer.
"""


SCHEMA_OVERRIDES: dict[str, dict[str, Any]] = {
    "unreal_get_actor": {
        "type": "object",
        "properties": {
            "name": {"type": "string"},
            "label": {"type": "string"},
            "prefer_pie": {"type": "boolean"},
        },
        "additionalProperties": True,
    },
    "unreal_get_actor_property": {
        "type": "object",
        "properties": {
            "actor": {"type": "string"},
            "property": {"type": "string"},
            "prefer_pie": {"type": "boolean"},
        },
        "required": ["actor", "property"],
        "additionalProperties": True,
    },
    "unreal_get_component": {
        "type": "object",
        "properties": {
            "actor": {"type": "string"},
            "component": {"type": "string"},
        },
        "required": ["actor", "component"],
        "additionalProperties": True,
    },
    "unreal_get_collision": {
        "type": "object",
        "properties": {
            "actor": {"type": "string"},
            "component": {"type": "string"},
        },
        "required": ["actor", "component"],
        "additionalProperties": True,
    },
    "unreal_list_actors_near": {
        "type": "object",
        "properties": {
            "origin": {
                "type": "array",
                "items": {"type": "number"},
                "minItems": 3,
                "maxItems": 3,
            },
            "radius": {"type": "number", "exclusiveMinimum": 0},
            "max": {"type": "integer", "minimum": 1},
        },
        "additionalProperties": True,
    },
    "unreal_get_output_log": {
        "type": "object",
        "properties": {
            "filter": {"type": "string"},
            "max": {"type": "integer", "minimum": 1},
        },
        "additionalProperties": True,
    },
    "unreal_find_blueprint": {
        "type": "object",
        "properties": {
            "path": {"type": "string"},
            "name": {"type": "string"},
        },
        "additionalProperties": True,
    },
    "unreal_get_blueprint_components": {
        "type": "object",
        "properties": {
            "path": {"type": "string"},
            "name": {"type": "string"},
        },
        "additionalProperties": True,
    },
    "unreal_get_blueprint_members": {
        "type": "object",
        "properties": {
            "path": {"type": "string"},
            "name": {"type": "string"},
        },
        "additionalProperties": True,
    },
    "unreal_run_playtest": {
        "type": "object",
        "properties": {"test_id": {"type": "string"}},
        "required": ["test_id"],
        "additionalProperties": True,
    },
    "unreal_get_playtest_status": {
        "type": "object",
        "properties": {"run_id": {"type": "string"}},
        "required": ["run_id"],
        "additionalProperties": True,
    },
    "unreal_get_playtest_result": {
        "type": "object",
        "properties": {"run_id": {"type": "string"}},
        "required": ["run_id"],
        "additionalProperties": True,
    },
    "unreal_stop_playtest": {
        "type": "object",
        "properties": {"run_id": {"type": "string"}},
        "required": ["run_id"],
        "additionalProperties": True,
    },
}

DEFAULT_TOOL_PARAMETERS: dict[str, Any] = {
    "type": "object",
    "properties": {},
    "additionalProperties": True,
}


def normalize_tool_entry(tool: Any) -> tuple[str, str, dict[str, Any]]:
    """Normalize tuple-, dict-, or object-style bridge tool definitions."""
    if isinstance(tool, (tuple, list)):
        if not tool:
            return "", "", DEFAULT_TOOL_PARAMETERS
        name = str(tool[0])
        description = ""
        parameters: dict[str, Any] = DEFAULT_TOOL_PARAMETERS
        if len(tool) >= 2:
            second = tool[1]
            if isinstance(second, dict):
                description = str(second.get("description", ""))
                candidate = second.get("inputSchema", second.get("parameters"))
                if isinstance(candidate, dict):
                    parameters = candidate
            else:
                description = str(second)
        if len(tool) >= 3 and isinstance(tool[2], dict):
            parameters = tool[2]
        return name, description, parameters

    if isinstance(tool, dict):
        name = str(tool.get("name", ""))
        description = str(tool.get("description", ""))
        parameters = tool.get("inputSchema", tool.get("parameters"))
        if not isinstance(parameters, dict):
            parameters = DEFAULT_TOOL_PARAMETERS
        return name, description, parameters

    if hasattr(tool, "name"):
        name = str(getattr(tool, "name"))
        description = str(getattr(tool, "description", ""))
        parameters = getattr(tool, "inputSchema", getattr(tool, "parameters", None))
        if hasattr(parameters, "model_dump"):
            parameters = parameters.model_dump()
        if not isinstance(parameters, dict):
            parameters = DEFAULT_TOOL_PARAMETERS
        return name, description, parameters

    return str(tool), "", DEFAULT_TOOL_PARAMETERS


def tool_names(collection: Any) -> set[str]:
    entries = collection.items() if isinstance(collection, dict) else collection
    names: set[str] = set()
    for entry in entries:
        name, _, _ = normalize_tool_entry(entry)
        if name:
            names.add(name)
    return names


READ_TOOL_NAMES = tool_names(READ_TOOLS)
WRITE_TOOL_NAMES = tool_names(WRITE_TOOLS)


def get_tool_schemas() -> list[dict[str, Any]]:
    """Expose only read/playtest tools during harness acceptance testing."""
    entries = READ_TOOLS.items() if isinstance(READ_TOOLS, dict) else READ_TOOLS
    schemas: list[dict[str, Any]] = []
    for entry in entries:
        name, description, parameters = normalize_tool_entry(entry)
        if not name:
            continue
        parameters = SCHEMA_OVERRIDES.get(name, parameters or DEFAULT_TOOL_PARAMETERS)
        schemas.append(
            {
                "type": "function",
                "function": {
                    "name": name,
                    "description": description,
                    "parameters": parameters,
                },
            }
        )
    return schemas


MASTER_HANDOFF_COMPACT_HEADINGS = (
    "## Role separation (APPROVED / LOCKED process)",
    "## Authority hierarchy (APPROVED / LOCKED)",
    "# Security / safety (APPROVED / LOCKED)",
    "# Current development state (exceptionally precise)",
    "# Opening Block 4 — recommended implementation plan",
    "# FIRST ACTIONS YOU MUST NOT TAKE",
)


def markdown_heading_level(line: str) -> int | None:
    stripped = line.lstrip()
    if not stripped.startswith("#"):
        return None
    marker = stripped.split(maxsplit=1)[0]
    if marker and set(marker) == {"#"}:
        return len(marker)
    return None


def extract_markdown_section(lines: list[str], heading: str) -> list[str]:
    try:
        start = lines.index(heading)
    except ValueError:
        return []
    level = markdown_heading_level(heading)
    if level is None:
        return []
    end = len(lines)
    for index in range(start + 1, len(lines)):
        candidate_level = markdown_heading_level(lines[index])
        if candidate_level is not None and candidate_level <= level:
            end = index
            break
    return lines[start:end]


def compact_master_handoff(text: str) -> str:
    """Keep operationally critical master sections within a practical prompt size.

    The complete handoff remains on disk and is still the durable implementation
    record. PROJECT_STATE.md and the canon file are always injected in full.
    Set ORGANOID_CONTEXT_MODE=full only for a task that genuinely needs the
    complete historical handoff and can tolerate the larger prefill cost.
    """
    lines = text.splitlines()
    compact_parts: list[str] = []

    # Preserve the document identity and metadata table before the first section.
    first_section = next(
        (index for index, line in enumerate(lines) if line.startswith("## ")),
        min(len(lines), 24),
    )
    compact_parts.append("\n".join(lines[:first_section]))

    for heading in MASTER_HANDOFF_COMPACT_HEADINGS:
        section = extract_markdown_section(lines, heading)
        if section:
            compact_parts.append("\n".join(section))

    compact_parts.append(
        "[Compact-context note: the complete master handoff remains available "
        "at this file path but is not injected into every Ollama request.]"
    )
    return "\n\n".join(part for part in compact_parts if part)


def load_project_documents() -> tuple[str, list[str], list[str]]:
    sections: list[str] = []
    loaded: list[str] = []
    warnings: list[str] = []

    for label, path, required in PROJECT_DOCUMENTS:
        if not path.is_file():
            level = "required" if required else "optional"
            warnings.append(f"{level} document not found: {path}")
            continue
        try:
            text = path.read_text(encoding="utf-8-sig", errors="replace")
        except OSError as exc:
            warnings.append(f"could not read {path}: {exc}")
            continue

        loaded_description = str(path)
        if path.name == "PROJECT_ORGANOID_MASTER_AI_HANDOFF.md" and CONTEXT_MODE == "compact":
            text = compact_master_handoff(text)
            loaded_description += " (compact operational sections)"

        sections.append(f"\n\n===== {label} =====\n{text}\n===== END {label} =====")
        loaded.append(loaded_description)

    return "".join(sections), loaded, warnings


def build_system_prompt() -> tuple[str, list[str], list[str]]:
    document_context, loaded, warnings = load_project_documents()
    prompt = BASE_SYSTEM_PROMPT + document_context + "\n" + FINAL_OPERATIONAL_REMINDER
    return prompt, loaded, warnings


def bridge_command_name(tool_name: str) -> str:
    return tool_name[len("unreal_") :] if tool_name.startswith("unreal_") else tool_name


def unwrap_tool_arguments(arguments: Any) -> dict[str, Any]:
    if not isinstance(arguments, dict):
        return {}
    # Accept the MCP-style {"args": {...}} envelope if the model happens to use it.
    if isinstance(arguments.get("args"), dict):
        return arguments["args"]
    return arguments


def execute_tool(tool_name: str, arguments: Any) -> str:
    if tool_name in WRITE_TOOL_NAMES:
        result = {
            "ok": False,
            "error_code": "write_tools_disabled",
            "error": (
                "Durable write tools are intentionally disabled in the automation "
                "harness until the separate dual-approval workflow is verified."
            ),
        }
        return json.dumps(result, ensure_ascii=False)

    if tool_name not in READ_TOOL_NAMES:
        result = {
            "ok": False,
            "error_code": "unknown_or_unavailable_tool",
            "error": f"Tool is not available in this harness: {tool_name}",
        }
        return json.dumps(result, ensure_ascii=False)

    args = unwrap_tool_arguments(arguments)
    command = bridge_command_name(tool_name)
    print(f"[Bridge] Dispatching {command} to Unreal Engine (127.0.0.1:8732)...")
    # Enforce read-only at the transport layer even if the model supplied a
    # misleading session object inside its arguments.
    args.pop("session", None)
    result = call_unreal(command, args, {"read_only": True})
    return json.dumps(result, ensure_ascii=False, default=str)


def json_objects_in_text(text: str) -> list[Any]:
    """Return JSON objects/lists embedded in prose, code fences, or XML tags."""
    decoder = json.JSONDecoder()
    found: list[Any] = []
    for index, character in enumerate(text):
        if character not in "[{":
            continue
        try:
            value, _ = decoder.raw_decode(text[index:])
        except json.JSONDecodeError:
            continue
        found.append(value)
    return found


def extract_tool_calls_from_content(
    content: str,
    valid_tool_names: set[str] | None = None,
) -> list[dict[str, Any]]:
    """Recover Ollama/Qwen tool calls emitted as raw multiline JSON content."""
    valid_names = valid_tool_names or (READ_TOOL_NAMES | WRITE_TOOL_NAMES)
    recovered: list[dict[str, Any]] = []

    def add_candidate(candidate: Any) -> None:
        if not isinstance(candidate, dict):
            return
        if isinstance(candidate.get("function"), dict):
            function = candidate["function"]
            name = function.get("name")
            arguments = function.get("arguments", {})
        else:
            name = candidate.get("name")
            arguments = candidate.get("arguments", {})
        if not isinstance(name, str) or name not in valid_names:
            return
        if isinstance(arguments, str):
            try:
                parsed_arguments = json.loads(arguments)
            except json.JSONDecodeError:
                parsed_arguments = {}
        else:
            parsed_arguments = arguments
        if not isinstance(parsed_arguments, dict):
            parsed_arguments = {}
        recovered.append(
            {
                "type": "function",
                "function": {
                    "name": name,
                    "arguments": json.dumps(parsed_arguments),
                },
            }
        )

    for value in json_objects_in_text(content):
        if isinstance(value, list):
            for item in value:
                add_candidate(item)
        elif isinstance(value, dict) and isinstance(value.get("tool_calls"), list):
            for item in value["tool_calls"]:
                add_candidate(item)
        else:
            add_candidate(value)
        if recovered:
            break

    return recovered


def parse_function_arguments(raw_arguments: Any) -> dict[str, Any]:
    if isinstance(raw_arguments, dict):
        return raw_arguments
    if isinstance(raw_arguments, str):
        try:
            parsed = json.loads(raw_arguments)
        except json.JSONDecodeError:
            return {}
        return parsed if isinstance(parsed, dict) else {}
    return {}


def call_signature(tool_name: str, arguments: dict[str, Any]) -> str:
    canonical_arguments = json.dumps(arguments, sort_keys=True, separators=(",", ":"))
    return f"{tool_name}:{canonical_arguments}"


def request_ollama(payload: dict[str, Any]) -> dict[str, Any]:
    request = urllib.request.Request(
        OLLAMA_URL,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=OLLAMA_TIMEOUT_SECONDS) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"Ollama HTTP {exc.code}: {body or exc.reason}") from exc
    except (TimeoutError, socket.timeout) as exc:
        raise RuntimeError(
            f"Ollama did not answer within {OLLAMA_TIMEOUT_SECONDS} seconds. "
            "The model may still be loading or processing context."
        ) from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"Connection to Ollama failed: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"Ollama returned invalid JSON: {exc}") from exc


def chat_round(
    messages: list[dict[str, Any]],
    tools: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    previous_signature: str | None = None
    consecutive_identical_calls = 0

    for round_number in range(1, MAX_TOOL_ROUNDS + 1):
        payload = {
            "model": MODEL_NAME,
            "messages": messages,
            "tools": tools,
            "stream": False,
            "temperature": 0.1,
        }

        print(f"\n[Thinking] Querying Ollama ({MODEL_NAME}) — round {round_number}...")
        started = time.time()
        try:
            response_data = request_ollama(payload)
        except RuntimeError as exc:
            print(f"[ERROR] {exc}")
            return messages
        print(f"[Thinking] Ollama finished in {time.time() - started:.1f}s.")

        try:
            message = response_data["choices"][0]["message"]
        except (KeyError, IndexError, TypeError) as exc:
            print(f"[ERROR] Unexpected Ollama response shape: {response_data}")
            return messages

        content = message.get("content") or ""
        model_tool_calls = message.get("tool_calls") or []
        recovered_from_content = False

        if not model_tool_calls and content:
            model_tool_calls = extract_tool_calls_from_content(content, READ_TOOL_NAMES)
            if model_tool_calls:
                recovered_from_content = True
                print(
                    f"[Parser] Recovered {len(model_tool_calls)} tool call(s) "
                    "from raw JSON content."
                )

        if not model_tool_calls:
            print(f"\nAssistant:\n{content}")
            messages.append({"role": "assistant", "content": content})
            return messages

        normalized_calls: list[dict[str, Any]] = []
        for call_index, tool_call in enumerate(model_tool_calls, start=1):
            if not isinstance(tool_call, dict) or not isinstance(tool_call.get("function"), dict):
                continue
            function = tool_call["function"]
            name = function.get("name", "")
            arguments = parse_function_arguments(function.get("arguments", {}))
            call_id = tool_call.get("id") or f"call_{round_number}_{call_index}"
            normalized_calls.append(
                {
                    "id": call_id,
                    "type": "function",
                    "function": {
                        "name": name,
                        "arguments": json.dumps(arguments),
                    },
                }
            )

        if not normalized_calls:
            print("[ERROR] Ollama returned tool calls in an unusable shape.")
            return messages

        messages.append(
            {
                "role": "assistant",
                "content": None if recovered_from_content else (content or None),
                "tool_calls": normalized_calls,
            }
        )

        steering_needed = False
        for tool_call in normalized_calls:
            call_id = tool_call["id"]
            function = tool_call["function"]
            tool_name = function["name"]
            arguments = parse_function_arguments(function["arguments"])
            signature = call_signature(tool_name, arguments)

            if signature == previous_signature:
                consecutive_identical_calls += 1
            else:
                previous_signature = signature
                consecutive_identical_calls = 1

            print(f"\n[Tool Execution] -> {tool_name}({json.dumps(arguments)})")

            if (
                tool_name not in REPETITION_GUARD_EXEMPT_TOOLS
                and consecutive_identical_calls > MAX_IDENTICAL_CONSECUTIVE_CALLS
            ):
                tool_output = json.dumps(
                    {
                        "ok": False,
                        "error_code": "duplicate_tool_call_blocked",
                        "error": (
                            "The harness blocked this identical tool call because it "
                            "was already executed twice consecutively. Use the prior "
                            "result and choose a more specific next tool."
                        ),
                    }
                )
                steering_needed = True
                print("[Loop Guard] Blocked a third identical consecutive tool call.")
            else:
                tool_output = execute_tool(tool_name, arguments)
                print(f"[Bridge Response] Received {len(tool_output)} characters.")

            messages.append(
                {
                    "role": "tool",
                    "tool_call_id": call_id,
                    "name": tool_name,
                    "content": tool_output,
                }
            )

        if steering_needed:
            messages.append(
                {
                    "role": "system",
                    "content": (
                        "An identical tool call was blocked by the repetition guard. "
                        "Do not request it again. Use the existing result and select "
                        "the task-specific tool. For nearby actors, call "
                        "unreal_get_player_pawn and then unreal_list_actors_near."
                    ),
                }
            )

    print("\n[WARN] Stopped after reaching the maximum number of tool rounds.")
    return messages


def initial_messages(system_prompt: str) -> list[dict[str, Any]]:
    return [{"role": "system", "content": system_prompt}]


def main() -> None:
    print("=" * 64)
    print("Project Organoid Automation Harness — read-only acceptance build")
    print("=" * 64)

    system_prompt, loaded_documents, document_warnings = build_system_prompt()
    for path in loaded_documents:
        print(f"[Context] Loaded {path}")
    for warning in document_warnings:
        print(f"[WARN] {warning}")
    print(f"[Context] Mode: {CONTEXT_MODE}; system prompt: {len(system_prompt):,} characters")

    tools = get_tool_schemas()
    print(f"[Tools] Loaded {len(tools)} read/playtest tools.")
    print(f"[Safety] {len(WRITE_TOOL_NAMES)} durable write tools are disabled.")
    print(f"[Model] {MODEL_NAME}")
    print("Ready. Type 'reset' to clear chat history or 'exit' to quit.\n")

    messages = initial_messages(system_prompt)
    while True:
        try:
            user_input = input("User > ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting...")
            return

        if not user_input:
            continue
        if user_input.lower() in {"exit", "quit"}:
            return
        if user_input.lower() == "reset":
            messages = initial_messages(system_prompt)
            print("[Context] Chat history reset.")
            continue

        messages.append({"role": "user", "content": user_input})
        messages = chat_round(messages, tools)


if __name__ == "__main__":
    main()
