import os
import sys
import json
import requests

OLLAMA_ENDPOINT = "http://localhost:11434/v1/chat/completions"
MODEL_NAME = "qwen-en"

# --- Bridge integration -----------------------------------------------
# Pull the tool list and the actual Unreal-calling function straight from
# server.py so there's exactly one definition of what each tool does and
# what it expects. Importing this module does NOT start the MCP stdio
# loop (that only runs under `if __name__ == "__main__"` in server.py),
# so it's safe to import here as a library.

BRIDGE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "unreal_mcp")
if BRIDGE_DIR not in sys.path:
    sys.path.insert(0, BRIDGE_DIR)

from server import READ_TOOLS, call_unreal  # noqa: E402

# Deliberately NOT importing WRITE_TOOLS yet. The approval chain
# (prepare_write -> manual y/n -> approve_write -> execute_write) isn't
# wired into this loop yet, so only read tools are exposed to the model
# for now. Extend this once that flow is built and tested.


def build_tool_definitions():
    """Convert server.py's READ_TOOLS into OpenAI-style tool schemas."""
    tools = []
    for name, description in READ_TOOLS:
        tools.append({
            "type": "function",
            "function": {
                "name": name,
                "description": description,
                "parameters": {
                    "type": "object",
                    "properties": {
                        "args": {
                            "type": "object",
                            "description": "Command arguments forwarded to Unreal. Empty object if the tool takes none.",
                        }
                    },
                },
            },
        })
    return tools


TOOLS = build_tool_definitions()


def load_document(file_path):
    if os.path.exists(file_path):
        with open(file_path, "r", encoding="utf-8") as f:
            return f.read()
    print(f"Warning: Document {file_path} not found.")
    return ""


def initialize_harness_agent(doc1_path, doc2_path):
    doc1_content = load_document(doc1_path)
    doc2_content = load_document(doc2_path)
    return (
        "You are an automated development assistant for Project Organoid.\n"
        "You have read-only tools available to inspect the live Unreal Editor "
        "state (actors, blueprints, PIE state, output log, etc). Use them "
        "whenever a task requires current, factual information about the "
        "project rather than guessing from the documents below.\n\n"
        f"--- DOCUMENT 1 ---\n{doc1_content}\n\n"
        f"--- DOCUMENT 2 ---\n{doc2_content}"
    )


def _execute_tool_call(tool_call) -> str:
    """Run one model-requested tool call against the real bridge and
    return a JSON string suitable for a tool-role message."""
    name = tool_call["function"]["name"]
    raw_args = tool_call["function"].get("arguments") or "{}"

    try:
        parsed = json.loads(raw_args) if isinstance(raw_args, str) else raw_args
    except json.JSONDecodeError:
        return json.dumps({"ok": False, "error_code": "bad_arguments", "error": raw_args})

    args = parsed.get("args") if isinstance(parsed.get("args"), dict) else {}

    valid_names = {n for n, _ in READ_TOOLS}
    if name not in valid_names:
        return json.dumps({
            "ok": False,
            "error_code": "tool_not_available",
            "error": f"'{name}' is not an enabled read tool in this loop.",
        })

    command = name[len("unreal_"):] if name.startswith("unreal_") else name
    result = call_unreal(command, args, session={"read_only": True})
    return json.dumps(result)


def run_task(system_prompt, task_prompt, history=None):
    """Runs one user task through Ollama, handling any tool calls the
    model makes before returning the final natural-language answer."""
    messages = [{"role": "system", "content": system_prompt}]
    if history:
        messages.extend(history)
    messages.append({"role": "user", "content": task_prompt})

    max_tool_rounds = 6  # guard against infinite tool-call loops

    for _ in range(max_tool_rounds):
        payload = {
            "model": MODEL_NAME,
            "messages": messages,
            "tools": TOOLS,
            "temperature": 0.2,
        }
        try:
            response = requests.post(OLLAMA_ENDPOINT, json=payload, timeout=120)
            response.raise_for_status()
        except Exception as e:
            return f"Error communicating with local Harness endpoint: {e}", messages

        message = response.json()["choices"][0]["message"]
        messages.append(message)

        tool_calls = message.get("tool_calls")

        # Workaround for a known Ollama/Qwen bug (see e.g. ollama/ollama
        # issues on qwen2.5-coder tool calling): the model correctly emits
        # a tool call, but Ollama's parser sometimes fails to wrap it into
        # the structured `tool_calls` field and instead returns it as a
        # plain JSON string in `content`. Detect and recover that case
        # ourselves rather than waiting on an upstream fix.
        if not tool_calls:
            content = (message.get("content") or "").strip()
            valid_names = {n for n, _ in READ_TOOLS}
            if content.startswith("{") and content.endswith("}"):
                try:
                    parsed = json.loads(content)
                    if isinstance(parsed, dict) and parsed.get("name") in valid_names:
                        tool_calls = [{
                            "id": f"recovered-{len(messages)}",
                            "function": {
                                "name": parsed["name"],
                                "arguments": json.dumps(parsed.get("arguments", {})),
                            },
                        }]
                        print("[Recovered a tool call Ollama returned as plain text]")
                except json.JSONDecodeError:
                    pass

        if not tool_calls:
            return message.get("content", ""), messages

        print(f"\n[Model requested {len(tool_calls)} tool call(s):]")
        for tc in tool_calls:
            tool_name = tc["function"]["name"]
            print(f"  -> {tool_name}({tc['function'].get('arguments', '{}')})")
            result_json = _execute_tool_call(tc)
            messages.append({
                "role": "tool",
                "tool_call_id": tc.get("id", tool_name),
                "content": result_json,
            })

    return "Stopped after too many tool-call rounds without a final answer.", messages


if __name__ == "__main__":
    DOC_1 = "PROJECT_ORGANOID_CANON.md"
    DOC_2 = "design_notes.md"
    print("Initializing Harness agent and ingesting documents...")
    context = initialize_harness_agent(DOC_1, DOC_2)
    print(f"Loaded {len(TOOLS)} read tools from the Unreal bridge.")
    print("Agent ready. Entering automation loop. Type 'exit' to quit.\n")

    conversation_history = []
    while True:
        task = input("Task/Prompt for Qwen: ")
        if task.lower() in ["exit", "quit"]:
            break
        print("\n[Executing via Harness...]")
        output, conversation_history = run_task(context, task, conversation_history)
        print(f"\n{output}\n" + "-" * 40)
