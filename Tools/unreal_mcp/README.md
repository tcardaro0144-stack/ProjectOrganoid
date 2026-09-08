# Organoid Unreal ↔ Cursor Bridge

Local-only read-only bridge so Cursor/Grok can inspect Unreal Editor and PIE without copy-paste, Outliner hunting, or Output Log `py` shuttling.

- Bind: `127.0.0.1` only (UE 5.8 `HTTPServer` default `localhost`)
- Allowlisted commands only
- No arbitrary Python eval
- No shell execution from Unreal
- Writes exist as stubs and return `writes_disabled` in Phase 1
- Game content is not modified by this tool

## Architecture

```
Grok plans
  → OrganoidAIBridge prepare_write (fail-closed preflight, change_id, no mutation)
  → Tom approve_write role=user
  → distinct second_review approve_write
  → agent execute_write (native Unreal API on the game thread)
  → bridge/playtest bot verifies
  → Grok reports
```

Tom approves payloads. Tom does **not** routinely click editor UI (Levels save, Save All, compile, spawn) for allowlisted deterministic operations. After dual approval the Cursor/Grok agent calls `execute_write`. Direct mutation commands are rejected.

Playtest bot (`run_playtest`) is PIE-only and `playtest_mutates_assets=false`. Map/package/actor writes stay on the approval-gated bridge.

```
Cursor MCP (Tools/unreal_mcp/server.py, stdio)
    POST http://127.0.0.1:8732/v1/command
Unreal Editor plugin OrganoidAIBridge (Editor module)
    GEditor PIE world / editor world
    structured JSON
```

Unreal Python remote execution was **not** used for mutations. That protocol accepts arbitrary Python, which violates the allowlist rule. An Editor plugin with named commands is the safe UE 5.8 path.

Ollama/Qwen later: call the same HTTP API (`python Tools/unreal_mcp/client.py ...`) or keep this MCP server. No Cursor-specific protocol is required on the Unreal side.

## What was created (source only — not compiled, not enabled)

### Unreal plugin

- `Plugins/OrganoidAIBridge/OrganoidAIBridge.uplugin` (`EnabledByDefault: false`)
- `Plugins/OrganoidAIBridge/Config/DefaultOrganoidAIBridge.ini`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/OrganoidAIBridge.Build.cs`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Public/OrganoidAIBridgeModule.h`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeModule.cpp`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeServer.h`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeServer.cpp`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeCommands.h`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeCommands.cpp`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeJson.h`
- `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeLogSink.h`

### MCP / tools

- `Tools/unreal_mcp/server.py`
- `Tools/unreal_mcp/client.py`
- `Tools/unreal_mcp/schemas/commands.json`
- `Tools/unreal_mcp/cursor-mcp.example.json`
- `Tools/unreal_mcp/README.md` (this file)

**Not changed:** `ProjectOrganoid.uproject`, maps, Blueprints, S1–S16, `BP_AdminAccessDoor`, player movement, Cursor `mcp.json`.

## Read-only tools

| MCP tool | Unreal command | Use |
|---|---|---|
| `unreal_get_editor_state` | `get_editor_state` | map, streaming, selection |
| `unreal_get_pie_state` | `get_pie_state` | PIE running, PC, pawn names |
| `unreal_get_player_pawn` | `get_player_pawn` | location, capsule, movement, sweep |
| `unreal_get_actor` | `get_actor` | transform, bounds, components |
| `unreal_get_component` | `get_component` | collision + bounds |
| `unreal_list_actors_near` | `list_actors_near` | nearby actors |
| `unreal_get_collision` | `get_collision` | alias of get_component |
| `unreal_capsule_sweep` | `capsule_sweep` | Pawn-profile sweep |
| `unreal_overlap_query` | `overlap_query` | overlaps at origin |
| `unreal_get_output_log` | `get_output_log` | filtered log ring |
| `unreal_find_blueprint` | `find_blueprint` | parent, status, path |
| `unreal_get_blueprint_components` | `get_blueprint_components` | SCS + CDO collision |
| `unreal_get_blueprint_members` | `get_blueprint_members` | variables + Timeline nodes |
| `unreal_get_user_defined_enum` | `get_user_defined_enum` | UserDefinedEnum index/internal_name/display_name/value; optional SwitchEnum pins |

Write tool names exist. Direct mutations are rejected (`needs_prepare`). The only mutation path is:

1. `prepare_write` — snapshot + preflight, returns `change_id`. **No world change.**
2. `approve_write` `role=user` with a non-empty identity
3. `approve_write` `role=second_review` with a **different** identity
4. `execute_write` with `session.read_only=false` — re-runs preflight and package guards

Allowlisted actions only: `delete_actor`, `set_component_property`, `set_collision`, `set_visibility`, `set_transform`, `compile_blueprint`, `save_asset` (non-map), `save_maps` (game-thread `UEditorLoadingAndSavingUtils::SavePackages`; Admin-only does not require persistent `Lvl_Epitope`; two-package Admin+`Lvl_Epitope` still requires persistent `Lvl_Epitope`), `move_actor_to_level`, `set_actor_property`, `spawn_blueprint_actor`, `rerun_construction`, `add_blueprint_variable`, `connect_blueprint_pins`.

No Python eval. No shell. High-risk actions (`delete_actor`, `compile_blueprint`, `save_asset`) require that exact action on the approved `change_id`. Admin actor writes must match `/Game/Maps/Epitope/SL_Epitope_Admin`.

## Safety / approval design

Phase 1:

- HTTP bind is localhost
- Non-loopback peers are rejected
- Optional `ORGANOID_BRIDGE_TOKEN` header `X-Organoid-Bridge-Token`
- Command allowlist in C++ (`FOrganoidAIBridgeCommands::Dispatch`)
- `session.read_only` defaults true
- Write commands return `writes_disabled`
- Admin write guard (wired, not yet executable): `required_world_package` must match `/Game/Maps/Epitope/SL_Epitope_Admin` or the write aborts with zero changes

Phase 2 (approval-gated):

- `prepare_write` / `approve_write` / `execute_write`
- Low-risk: visibility
- Medium: collision, transform, allowlisted component properties
- High-risk: delete actor, compile Blueprint, save specific package
- High-risk requires **user + second-review** identities on the same `change_id`
- Every write response logs change_id, action, target, before, after, package, timestamp, approvals, save_performed

## Startup / shutdown (after you approve the build)

### Unreal side

1. **Close Unreal Editor.** Compiling an Editor plugin with the editor open is unsafe in this project.
2. Enable the plugin (see Build requirement below).
3. Generate Visual Studio project files.
4. Build `ProjectOrganoidEditor` Development Win64.
5. Launch the editor. Output Log should contain `[OrganoidAIBridge] Listening on 127.0.0.1:8732`.
6. Probe: `python Tools/unreal_mcp/client.py ping`

Shutdown: close the editor. The listener dies with the process.

### Cursor side (after Unreal is listening)

Merge `Tools/unreal_mcp/cursor-mcp.example.json` into `.cursor/mcp.json` **or** user-level `C:\Users\tomca\.cursor\mcp.json`, using an **absolute** path to `server.py` if Cursor’s cwd is not the project. Reload MCP.

Do not add this until the plugin is actually running, or Cursor will show a dead server.

## Build requirement — wait for approval

| Item | Requirement |
|---|---|
| Unreal closed? | **Yes** before compile |
| Visual Studio / build tools? | **Yes** — this is a new Editor C++ module |
| `.uproject` change? | **Yes, after approval** — add the plugin entry |
| Game content compile? | **No** — do not Live Coding the game module for this |
| Maps / BPs / S1–S16? | **Do not touch** |

Proposed `.uproject` plugin block (not applied yet):

```json
{
  "Name": "OrganoidAIBridge",
  "Enabled": true
}
```

Then:

```
"%UE58%\Engine\Build\BatchFiles\Build.bat" ProjectOrganoidEditor Win64 Development -Project="C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\ProjectOrganoid.uproject" -WaitMutex
```

Or Generate Project Files and build the `OrganoidAIBridge` / `ProjectOrganoidEditor` target in Visual Studio.

## Section 17 runtime test (after the plugin is live)

1. Open `Lvl_Epitope` / Admin as usual.
2. PIE. Walk to the stop (~X 738, Y 96, Z 118).
3. Leave PIE running.
4. From the project root:

```
python Tools/unreal_mcp/client.py get_player_pawn "{\"include_sweep\":true,\"direction\":[1,0,0],\"distance\":150}"
```

Success looks like:

```json
{
  "ok": true,
  "data": {
    "pie_running": true,
    "pawn": "ProjectOrganoidCharacter_0",
    "location": [737.97, 96.29, 118.15],
    "movement_mode": "Walking",
    "is_moving_on_ground": true,
    "blocking_hit": {
      "actor": "...",
      "component": "...",
      "impact_point": [...],
      "impact_normal": [...],
      "belongs_to_admin_access_door": true
    }
  }
}
```

No Outliner. No Unreal Python paste.

Optional follow-up:

```
python Tools/unreal_mcp/client.py get_actor "{\"name\":\"AdminAccessDoor\"}"
python Tools/unreal_mcp/client.py get_component "{\"actor\":\"AdminAccessDoor\",\"component\":\"Door_Left\"}"
```

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| `bridge_unreachable` | Plugin not enabled, editor not running, or port 8732 taken |
| `no_pie` | PIE is not playing |
| `no_pawn` | Controller has no pawn |
| `forbidden` | Request did not come from loopback |
| `writes_disabled` | Phase 1 — expected for write tools |
| Plugin missing after launch | `.uproject` entry not added, or EnabledByDefault still false and not enabled in Plugins |
| Compile errors in `OrganoidAIBridge` | Editor was open, or VS project files not regenerated |
| Cursor MCP dead | Absolute path to `server.py` missing, or Unreal not listening |

## Ollama / Qwen

Unreal exposes HTTP, not Cursor. A local model can:

1. Call `python Tools/unreal_mcp/client.py <command>` as a tool.
2. Or POST JSON to `http://127.0.0.1:8732/v1/command`.

Keep the same allowlist. Do not point a LAN bind at Ollama. Stay on `127.0.0.1`.
