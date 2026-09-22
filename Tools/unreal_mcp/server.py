"""Stdio MCP server for the Organoid Unreal AI Bridge.

Uses MCP Content-Length framing on stdin/stdout.
Talks to Unreal on 127.0.0.1 only. No cloud. No arbitrary Python eval.
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request

HOST = os.environ.get("ORGANOID_BRIDGE_HOST", "127.0.0.1")
PORT = int(os.environ.get("ORGANOID_BRIDGE_PORT", "8732"))
TOKEN = os.environ.get("ORGANOID_BRIDGE_TOKEN", "")
BASE = f"http://{HOST}:{PORT}"

READ_TOOLS = [
    ("unreal_get_editor_state", "Current editor world, loaded map, streaming levels, and selected actors."),
    ("unreal_get_pie_state", "Whether PIE is running, PIE world, PlayerController 0, and possessed pawn names."),
    ("unreal_get_player_pawn", "Possessed pawn class/name, location, capsule, movement mode, IsMovingOnGround, and a Pawn-profile capsule sweep. Use this to identify the Section 17 blocker without the Outliner."),
    ("unreal_get_actor", "Actor label/class/transform/bounds/component hierarchy. Args: name or label. Optional prefer_pie."),
    ("unreal_get_actor_property", "Read one instance UPROPERTY from a unique actor via native FProperty. Returns the live instance value, not the CDO. Args: actor (label or unique path), property. Optional prefer_pie (default false)."),
    ("unreal_get_component", "One primitive component collision profile, Pawn response, and world bounds. Args: actor, component."),
    ("unreal_list_actors_near", "Actors near an origin or the possessed pawn. Args: origin [x,y,z], radius, max."),
    ("unreal_get_collision", "Collision dump for an actor component. Same args as unreal_get_component."),
    ("unreal_capsule_sweep", "Pawn-profile capsule sweep from start along direction. Defaults to possessed pawn capsule and forward."),
    ("unreal_overlap_query", "Pawn-profile capsule overlap at origin. Defaults to possessed pawn location."),
    ("unreal_get_output_log", "Recent Unreal Output Log lines. Args: filter, max."),
    ("unreal_find_blueprint", "Blueprint existence, parent class, compile status, path. Args: path or name."),
    ("unreal_get_blueprint_components", "Blueprint SCS component list and collision defaults. Args: path or name."),
    ("unreal_get_blueprint_members", "Blueprint variable names and Timeline nodes. Args: path or name."),
    ("unreal_get_user_defined_enum", "Read-only UserDefinedEnum dump: index, internal_name, display_name, value from native UEnum APIs. Optional blueprint_path dumps matching UK2Node_SwitchEnum pin_name/pin_friendly_name. Args: path, optional blueprint_path."),
    ("unreal_inspect_playing_audio", "Read-only dump of live UAudioComponents in PIE/editor: owner, sound path, IsPlaying, volume, pitch, spatialization, looping, duration, world location. Args: prefer_pie (default true), playing_only (default true), max."),
    ("unreal_list_playtests", "List registered Organoid Playtest Bot tests. Does not mutate. Does not start PIE."),
    ("unreal_run_playtest", "Queue a registered playtest asynchronously and return run_id immediately. Args: test_id. Does not mutate assets; does not save/compile. Poll with get_playtest_status / get_playtest_result."),
    ("unreal_get_playtest_status", "Poll playtest run state, current stage, elapsed_ms, needs_approval. Args: run_id."),
    ("unreal_get_playtest_result", "Structured playtest record: assertions, expected vs actual, actors, errors, headline. Args: run_id."),
    ("unreal_stop_playtest", "Abort the active playtest and end PIE only if the bot started it. Args: run_id. Does not save or discard editor work."),
]

WRITE_TOOLS = [
    ("unreal_prepare_write", "Prepare an allowlisted mutation. Snapshots before-state, runs preflight, returns a change_id. Does not mutate. Read-only safe."),
    ("unreal_approve_write", "Record user or second_review approval on a change_id. Requires identity. Both roles required before execute."),
    ("unreal_reject_write", "Reject a prepared change_id. Zero world mutation."),
    ("unreal_get_change", "Inspect a prepared/approved/executed change_id and its audit fields."),
    ("unreal_list_changes", "List in-memory gated changes for this editor session."),
    ("unreal_execute_write", "Execute a dual-approved change_id. Requires session.read_only=false. Re-runs preflight and package guards. Does not eval Python or run shell."),
]


def _command_name(tool_name: str) -> str:
    return tool_name[len("unreal_") :] if tool_name.startswith("unreal_") else tool_name


def call_unreal(command: str, args=None, session=None) -> dict:
    payload = {
        "command": command,
        "args": args or {},
        "session": session or {"read_only": True},
    }
    data = json.dumps(payload).encode("utf-8")
    headers = {"Content-Type": "application/json"}
    if TOKEN:
        headers["X-Organoid-Bridge-Token"] = TOKEN
    req = urllib.request.Request(
        f"{BASE}/v1/command",
        data=data,
        headers=headers,
        method="POST",
    )
    timeout = 120 if command in ("execute_write", "prepare_write", "compile_blueprint", "save_maps", "move_actor_to_level", "spawn_blueprint_actor", "add_scs_component", "author_hologram_apply_state", "author_light_controller_set_zone", "author_admin_room_trigger_lighting_hook", "author_access_door_facility_state_listener", "author_light_controller_facility_state_listener", "spawn_s20_zone_light", "invoke_s20_set_lighting_zone", "delete_s22_legacy_admin_ambience", "spawn_s22_admin_audio_zones", "spawn_neuro_navmesh_bounds", "spawn_neuro_research_station", "spawn_admin_research_wing_connector", "spawn_admin_research_wing_keycard", "trim_spine_landing_admin", "spawn_neuro_arrival_lab_dressing", "spawn_neuro_ch3_containment_evidence", "spawn_neuro_ch4_transformed_personnel", "configure_neuro_power_failure_discovery", "spawn_neuro_power_diagnostic", "create_neurogenetics_mission", "expand_neurogenetics_mission_beat3", "expand_neurogenetics_mission_beat4", "expand_neurogenetics_mission_beat5", "expand_neurogenetics_mission_beat6", "spawn_neuro_neural_mapping_array", "spawn_neuro_research_load_cutoff", "spawn_neuro_neural_mapping_terminal", "spawn_neuro_neural_signature_observation_node", "spawn_neuro_neural_change_evidence_instrument") else 8
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        try:
            return json.loads(body)
        except json.JSONDecodeError:
            return {"ok": False, "error_code": "http_error", "error": body or str(exc)}
    except Exception as exc:
        return {
            "ok": False,
            "error_code": "bridge_unreachable",
            "error": (
                f"Cannot reach Unreal bridge at {BASE}. "
                f"Enable OrganoidAIBridge in the editor and keep Unreal open. ({exc})"
            ),
        }


def tool_schema(name: str, description: str) -> dict:
    return {
        "name": name,
        "description": description,
        "inputSchema": {
            "type": "object",
            "properties": {
                "args": {
                    "type": "object",
                    "description": "Command arguments forwarded to Unreal.",
                },
                "session": {
                    "type": "object",
                    "description": "Optional guards: read_only (default true), required_world_package, change_id. execute_write requires read_only=false.",
                },
            },
        },
    }


def handle(message: dict):
    method = message.get("method")
    msg_id = message.get("id")
    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": msg_id,
            "result": {
                "protocolVersion": "2024-11-05",
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "organoid-unreal-bridge", "version": "0.3.1"},
            },
        }
    if method == "notifications/initialized":
        return None
    if method == "tools/list":
        tools = [tool_schema(n, d) for n, d in READ_TOOLS + WRITE_TOOLS]
        return {"jsonrpc": "2.0", "id": msg_id, "result": {"tools": tools}}
    if method == "tools/call":
        params = message.get("params") or {}
        name = params.get("name", "")
        arguments = params.get("arguments") or {}
        args = arguments.get("args") if isinstance(arguments.get("args"), dict) else arguments
        session = arguments.get("session") if isinstance(arguments.get("session"), dict) else {"read_only": True}
        result = call_unreal(_command_name(name), args, session)
        return {
            "jsonrpc": "2.0",
            "id": msg_id,
            "result": {
                "content": [{"type": "text", "text": json.dumps(result, indent=2)}],
                "isError": not result.get("ok", False),
            },
        }
    if method == "ping":
        return {"jsonrpc": "2.0", "id": msg_id, "result": {}}
    if msg_id is not None:
        return {
            "jsonrpc": "2.0",
            "id": msg_id,
            "error": {"code": -32601, "message": f"Unknown method {method}"},
        }
    return None


def read_message():
    headers = {}
    while True:
        line = sys.stdin.buffer.readline()
        if not line:
            return None
        if line in (b"\r\n", b"\n"):
            break
        decoded = line.decode("utf-8", errors="replace")
        if ":" in decoded:
            key, value = decoded.split(":", 1)
            headers[key.strip().lower()] = value.strip()
    length = int(headers.get("content-length", "0") or 0)
    if length <= 0:
        return None
    body = sys.stdin.buffer.read(length)
    return json.loads(body.decode("utf-8"))


def write_message(message: dict) -> None:
    encoded = json.dumps(message, ensure_ascii=False).encode("utf-8")
    sys.stdout.buffer.write(f"Content-Length: {len(encoded)}\r\n\r\n".encode("ascii"))
    sys.stdout.buffer.write(encoded)
    sys.stdout.buffer.flush()


def main() -> None:
    while True:
        message = read_message()
        if message is None:
            return
        reply = handle(message)
        if reply is not None:
            write_message(reply)


if __name__ == "__main__":
    main()
