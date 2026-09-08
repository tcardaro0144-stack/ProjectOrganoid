# PROJECT ORGANOID — MASTER AI HANDOFF

**Living transfer record for a successor local AI stack (Qwen3.8 / Qwen Coder / Ollama).**

This file exists so a fresh instance can inherit Project Organoid **without** historical ChatGPT or Grok conversations.

Read this first. Then read `PROJECT_ORGANOID_CANON.md`. Then inspect the repository and Unreal project. Do **not** invent missing canon. Do **not** treat this file as design authority.

| Field | Value |
|---|---|
| Document identity | `PROJECT_ORGANOID_MASTER_AI_HANDOFF.md` |
| Created | 2026-09-01 |
| Last verified milestone | **Checkpoint 25% health floor** (campaign Blocks 1–3 remain verified; Block 4 not implemented) |
| Last handoff update | 2026-09-01 (checkpoint floor verified + Block 4 plan awaiting Tom) |
| Design authority | Restored Canon v1.0 (owner-approved) via `PROJECT_ORGANOID_CANON.md` index |
| Implementation evidence | Repository + Unreal project + Playtest Bot results |
| This file is | Process, architecture, tooling, verified state, contradictions, next boundary |
| This file is not | A replacement for Restored Canon v1.0; a license to mutate; a ChatGPT summary |

---

## How to use this document

1. Treat labels as legally distinct. A thing that is **implemented** is not automatically **canon**. A thing that is **canon** is not automatically **in the maps**. A thing that is **proposed** is not approved.
2. When this file and live Unreal state disagree, **live Unreal + tests win for implementation facts**. Report the discrepancy to Tom. Do not silently rewrite this file to match a guess.
3. When this file and Restored Canon disagree on design, **canon wins**. Implementation that contradicts canon is **STALE IMPLEMENTATION**, not a canon change.
4. After every **VERIFIED** implementation task, update the relevant sections **and** append a short changelog entry. Do not rewrite the whole history.
5. After a **FAILED** or **STOPPED** task, do not advance the verified milestone. You may record a durable technical discovery labeled **DISCOVERED DURING FAILED/STOPPED TASK — TASK NOT VERIFIED**.

---

## State labels (mandatory vocabulary)

Use these labels. Do not flatten them.

| Label | Meaning |
|---|---|
| **APPROVED / LOCKED** | Tom-approved design or process. Do not reopen without Tom. |
| **VERIFIED IMPLEMENTATION** | Built, tested with named playtests, targeted save completed where required. |
| **IMPLEMENTED — NOT YET VERIFIED** | Code/assets exist; Playtest Bot / inspection has not confirmed. |
| **PROVISIONAL** | Working compatible system. Keep unless Tom says replace. Not locked canon. |
| **PROPOSED — NOT APPROVED** | AI or collaborator idea. Not evidence of user approval. |
| **UNRESOLVED** | Authority cannot be determined, or Tom must decide before implementation. |
| **HISTORICAL / SUPERSEDED** | Once used; do not restore as current canon. |
| **STALE IMPLEMENTATION** | Code/comments/maps that contradict current approved canon. |
| **KNOWN TECHNICAL LIMITATION** | Real constraint. Do not “fix” by weakening tests or inventing UX. |
| **KNOWN SAFE ENGINE BEHAVIOR** | Looks like a bug; is Unreal/Recast/editor-normal. Do not treat as authored work. |
| **DO NOT REINTRODUCE** | Failed or prohibited approach. |

---

## Role separation (APPROVED / LOCKED process)

Even if one Qwen model later does planning **and** coding, keep the roles separate in **context**.

```
Qwen (planning) proposes
  → Tom reviews / approves a specific implementation scope
  → that approved specification becomes the only implementation scope
  → Qwen or Qwen Coder implements
  → Playtest Bot / targeted inspection verifies
  → targeted save_maps / package save
  → this handoff is updated
```

Rules:

- Planning does **not** authorize implementation.
- Tom is final canon and creative authority.
- An AI-generated proposal is **never** canon merely because an AI wrote it.
- The same model must **never** treat its own earlier proposal as proof of Tom’s approval.
- ChatGPT (historical planning collaborator) and Grok/Cursor (historical implementation agent) are **not** available to the successor unless Tom reconnects them. Their conversation history is **not** in the repo.
- Dual-approval identities used in the current Unreal write gate are **Tom** (`role=user`) and a distinct second reviewer (historically **Grok**, `role=second_review`). A Qwen successor must **not** approve its own writes as both roles.

---

## Authority hierarchy (APPROVED / LOCKED)

From `PROJECT_ORGANOID_CANON.md`:

1. **Restored Canon v1.0 (owner-approved)** — game design. Full text currently lives in owner-approved session material and is **not** duplicated in the repo. This is a real gap: Qwen will **not** have the complete v1.0 prose unless Tom later places it in the repository or pastes it. Until then, use this file + `PROJECT_ORGANOID_CANON.md` + Tom. Do **not** invent the missing prose.
2. **`PROJECT_ORGANOID_CANON.md`** — authority index, superseded list, locked identity, TBD list.
3. **Existing Unreal implementation** — evidence of current project state, not design authority.
4. **Legacy `.cursorrules`, `.cursor/rules/*.mdc`, C++ comments, Python, mission assets, `Tools/unreal_mcp/README.md`** — may contain superseded concepts.

Collaborator rule:

- Implementation vs Restored Canon → **canon wins**.
- Two design sources disagree → **owner-approved Restored Canon v1.0 wins**.
- Owner approval cannot be determined → **stop and ask Tom**.

**UNKNOWN does not mean fall back to legacy.**  
**Working compatible systems should be preserved rather than rewritten for taste.**

---

# FIRST SESSION — QWEN BOOTSTRAP PROCEDURE

You are a successor AI. You initially know nothing except this document, the repository, the Unreal project, and whatever harness Tom attached.

**STOP before any durable mutation unless Tom has approved a specific implementation task.** Dual-approved Unreal writes are not a substitute for a Tom-approved *design* scope.

### 1. Read the master handoff

Read this entire file. Note the **latest verified milestone**, **next development boundary**, and **UNRESOLVED** items.

### 2. Locate authoritative canon/rules

Read, in order:

1. `PROJECT_ORGANOID_CANON.md`
2. `.cursor/rules/organoid-editor-automation.mdc` (write/save/Live Coding process — still valid even if you are not Cursor)
3. `.cursor/rules/ue5-cpp-gameplay.mdc` (C++ conventions; tactical constants are **code defaults**, not locked canon)
4. `.cursorrules` and `.cursor/rules/project-organoid-core.mdc` — **HISTORICAL / SUPERSEDED for design**. Useful only for class-name / module path hints. Avery Vance, four-weapon roster, Cellular Denature / Bio-Stabilize, Sterling-as-shopkeeper, 800uu as final tactical UI are **not** design authority.

If Tom later adds a full Restored Canon v1.0 markdown to the repo, that file outranks the index for design prose. Until then, **do not reconstruct** missing biography, weapon names, or mystery answers.

### 3. Inspect repository state

From the project root (typical path):

`C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid`

Confirm:

- `ProjectOrganoid.uproject` exists and `"EngineAssociation": "5.8"`
- Modules: `ProjectOrganoid` (runtime), `ProjectOrganoidPlaytest` (editor)
- Plugin enabled in `.uproject`: `OrganoidAIBridge`
- Canon + this handoff exist at repo root

Useful layout:

```
ProjectOrganoid.uproject
PROJECT_ORGANOID_CANON.md
PROJECT_ORGANOID_MASTER_AI_HANDOFF.md
Source/ProjectOrganoid/          runtime game code
Source/ProjectOrganoidPlaytest/  editor Playtest Bot
Plugins/OrganoidAIBridge/        localhost HTTP bridge
Tools/unreal_mcp/                Python MCP/HTTP client (Cursor-optional)
Content/Maps/                    Lvl_MainMenu, Lvl_Epitope, Epitope sublevels
Content/Data/                    items, missions, dialogue
Content/Python/                  one-off editor scripts (not the mutation path)
Config/                          DefaultEngine.ini, DefaultGame.ini
```

### 4. Identify Unreal version / project

- Engine: **Unreal Engine 5.8**
- Typical engine path on Tom’s machine: `C:\Users\tomca\Desktop\UE_5.8`
- EditorStartupMap / GameDefaultMap: `/Game/Maps/Lvl_MainMenu`
- Default game mode: `/Script/ProjectOrganoid.ProjectOrganoidGameMode`
- Main menu map prefix uses `ProjectOrganoidMainMenuGameMode`

Closed-editor UBT (campaign C++ — **No Live Coding** for Block-style work):

```
Engine\Build\BatchFiles\Build.bat ProjectOrganoidEditor Win64 Development -Project="C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid\ProjectOrganoid.uproject" -WaitMutex -NoHotReloadFromIDE
```

Run from the engine `Engine\Build\BatchFiles` directory, or pass the full path to `Build.bat`. One editor instance only after compile.

### 5. Verify tool / plugin connectivity

You do **not** need Cursor. You need:

- Filesystem read (and later write) to the repo
- Ability to run commands (UBT, `python Tools/unreal_mcp/client.py …`)
- HTTP to `127.0.0.1:8732` while the editor is open

Optional env (document names only; **never store secrets in this file**):

- `ORGANOID_BRIDGE_HOST` (default `127.0.0.1`)
- `ORGANOID_BRIDGE_PORT` (default `8732`)
- `ORGANOID_BRIDGE_TOKEN` (optional header `X-Organoid-Bridge-Token`; if set in Tom’s environment, use it; do not print it)

### 6. Verify bridge connectivity

With Unreal Editor open and OrganoidAIBridge enabled:

```
python Tools/unreal_mcp/client.py ping
python Tools/unreal_mcp/client.py get_editor_state
```

Expect `ok: true`. If `bridge_unreachable`, the editor is closed or the plugin is not listening. Do not invent editor state.

At baseline creation (2026-09-01), a ping returned `bridge_unreachable` (connection refused). **Live dirty-package state at handoff-write time is therefore unknown.** Use the last verified save record below until you obtain a live `get_editor_state`.

### 7. Verify Playtest Bot connectivity

```
python Tools/unreal_mcp/client.py list_playtests
```

Expect registered `*_Functional` tests. This does not start PIE.

Smoke (non-destructive, `playtest_mutates_assets=false`):

```
python Tools/unreal_mcp/client.py run_playtest "{\"test_id\":\"OpeningFoundation_Functional\"}"
```

Then poll:

```
python Tools/unreal_mcp/client.py get_playtest_status "{\"run_id\":\"<run_id>\"}"
python Tools/unreal_mcp/client.py get_playtest_result "{\"run_id\":\"<run_id>\"}"
```

`run_playtest` returns a `run_id` immediately. Tests are asynchronous. Do not assume completion from the first response.

### 8. READ-ONLY project checks

Use only read commands until Tom approves a write:

- `get_editor_state` — persistent map, streaming levels, dirty packages
- `get_pie_state` — whether PIE is running
- `list_actors_near` / `get_actor` / `get_actor_property` — world evidence
- `get_output_log` — recent log
- `list_playtests` — catalog

Do **not** call `execute_write` with `session.read_only=false` in bootstrap.

### 9. Establish current dirty-package state

From `get_editor_state`, record dirty world packages.

Last **verified** save after Opening Block 3:

| Package | Status after Block 3 |
|---|---|
| `/Game/Maps/Epitope/SL_Epitope_Admin` | **Saved** (targeted `save_maps`) |
| `/Game/Data/Items/DA_Item_TraumaStabilizer` | **Saved** |
| `/Game/Maps/Epitope/SL_Epitope_NeuroGenetics` | **Do not save** unless Tom authorizes. Recast/NavMesh dirty is **KNOWN SAFE ENGINE BEHAVIOR**, not authored work. |
| `/Game/Maps/Lvl_Epitope` | Not saved for Block 3 |
| `/Game/Maps/Lvl_MainMenu` | Not saved for Block 3 |
| `/Game/Data/Items/DA_Item_PistolAmmo` | Existing asset; not retuned; not saved for Block 3 |
| `DA_Mission_TheAudit` | On disk; **not** loaded at New Game |

### 10. Establish latest verified milestone

**VERIFIED IMPLEMENTATION: Checkpoint 25% minimum-health stabilization floor** (2026-09-01), on top of Opening Blocks 1–3.

Campaign Blocks 1–3 remain verified. Block 4 (first Host) is **surveyed only** — **PROPOSED / AWAITING TOM APPROVAL**. Do not implement Block 4 without Tom’s approval of the plan in this file.

### 11. Run an appropriate non-destructive smoke test

Preferred bootstrap smoke: `OpeningFoundation_Functional` or `CheckpointHealth_Functional`.

If Admin is loaded and you need Block 3 confidence: `OpeningResources_Functional`.

Do not start a write-gated mutation to “make the smoke test pass.”

### 12. Compare live state against this handoff

Check at least:

- Persistent map (usually `Lvl_MainMenu` with Epitope sublevels streamed)
- Unique Admin actors: Reception + Security terminals, Block 3 pickups, RW keycard, vestibule DoorLock
- No Host in Admin opening
- Opening objectives: `Mission_OpeningFoundation` / `Obj_ReceptionCheckIn` / `Obj_SecurityStatus`
- Checkpoint `HealthStabilizationFloorPercent == 0.25` on all five campaign instances (full-heal boolean is gone)

### 13. Report discrepancies to Tom

If live state differs from this file, report:

- what this file claimed
- what you observed
- which playtest/command produced the observation
- whether you stopped (you should)

### 14. STOP

Do not begin durable changes. Do not start Opening Block 4 unless Tom has approved the Block 4 implementation plan in this file.

Greet Tom, state the verified milestone, state the next boundary, list UNRESOLVED blockers, and wait.

---

# Product / game identity

**APPROVED / LOCKED (from canon index)**

- Working title: **Project Organoid**
- Genre: third-person **survival-horror RPG**
- Intended experience: investigation and containment under biological lockdown; scarce resources; tactical decisions in real time; horror from **science and personnel**, not jump-scare carnival
- Major influences: **modern Resident Evil structure** + **Parasite Eve tactical/RPG spirit**. Influences are not templates to copy. Do not build a generic RE clone inventory UI or a PE clone skill list without Tom.
- Current scope: playable **Epitope Subterranean Complex** campaign spine, opening Admin tutorial integrated into the campaign, systems scaffolding for later sectors
- Campaign structure: **Admin → NeuroGenetics → Cryo → Compute → Reactor / Incubator**
- Facility/world: OSHA/BSL-4-style subterranean complex (Upstate NY in legacy pitch; treat location color as **PROVISIONAL** unless Tom restates it in canon text you actually have)

**STALE IMPLEMENTATION in rules/comments:** “Avery Vance, ex-JSOC combat medic / bio-hazard auditor” as current protagonist.

---

# Canon / creative

## Protagonist

| Topic | Status |
|---|---|
| **Nathan Grant**, male, he/him, visible in third person | **APPROVED / LOCKED** |
| Outside investigator/respondent; experienced with unusual biological incidents, investigation, containment, emergency response, firearms | **APPROVED / LOCKED** |
| Capable of real-time movement during tactical decision mode | **APPROVED / LOCKED** |
| **Not** a research scientist | **APPROVED / LOCKED** |
| Knows how to deal with biological hazards; does **not** necessarily know what they are | **APPROVED / LOCKED** |
| Full biography (employer, military, age, appearance, family, tragic backstory, previous Epitope relationship, arrival reason) | **UNRESOLVED / CANON CONFIRMATION REQUIRED** — do not invent; do not copy Avery’s JSOC medic bio |
| Avery Vance as current protagonist | **HISTORICAL / SUPERSEDED** |
| Intermediate label “Nathan Vance” | **HISTORICAL / SUPERSEDED** (Vance belongs to Tom’s separate novel *Glitched Reality*) |
| C++ class `AProjectOrganoidCharacter` comments still say Avery | **STALE IMPLEMENTATION** (name the player Nathan in new player-facing text) |

## Characters

| Character | Status |
|---|---|
| Nathan Grant | Locked identity; biography incomplete |
| Dr. Sterling | **Sterling-as-shopkeeper / escalating SOT shop is SUPERSEDED.** Final Sterling role **UNRESOLVED** |
| Transformed Epitope personnel (scientists, researchers, technicians, security, staff) | **APPROVED / LOCKED** as the intended enemy *reading*; complete transformation mechanism **UNRESOLVED** |
| HostBase | Technical chassis, **not** final enemy taxonomy. Later archetypes (e.g. The Integrated) are named in canon index as future taxonomy, not current Admin opening content |
| Incinerator survivor dialogue asset | `Content/Data/Dialogue/DA_Dialogue_IncineratorSurvivor.uasset` — **IMPLEMENTED — NOT YET VERIFIED** as campaign-facing; do not treat as locked story |

## Story / world / mystery

- Setting: Epitope facility under lockdown. Science-horror: organoids, bio-silicon, Neural Stems, Auditory Nodes are **relevant territory**, not a locked complete weak-point roster or a complete in-game mechanism.
- Mystery: who/what directs transformation, why, relationship to Epitope’s larger program, final vaccine mechanism — **UNRESOLVED**. Do **not** write lore that answers these.
- Gameplay/narrative relationship: opening tutorial is **in the campaign**, not a detached training room. Two layers: (1) player learns controls for things Nathan already knows; (2) Project Organoid-specific systems arrive gradually. **Nathan does not explain to himself how to reload.**
- Approved reveals vs premature: owner Restored Canon v1.0 lists these; full prose is **not in repo**. Practical rule from the index: do not implement Neuro Candidate B / first meaningful Research Station intro / first Host / biological targeting tutorial / pursuer until Tom + design pass lock them. **Neuro Candidate B is preferred design and must not be implemented until that campaign design is finished.**

## Science-horror principles (APPROVED / LOCKED direction)

- Transformed people should read as former staff, not generic zombies.
- Winning an encounter does not always mean killing (fight / disable / stagger / bypass / retreat / escape / conserve ammo).
- Unlimited ammunition is not intended campaign behavior.

## Superseded canon (DO NOT REINTRODUCE as current design)

From `PROJECT_ORGANOID_CANON.md` (index; not exhaustive of the full v1.0 text):

- Avery Vance protagonist
- Four-weapon roster (P226 / M4 Shotgun / Cryo Lancer / Denaturing Launcher as *the* locked set)
- Cellular Denature / Bio-Stabilize as the public PE skill names
- Sterling-as-shopkeeper
- 800uu debug sphere as **final** tactical UI
- Arc Gun that duplicates Lytic Cannon’s job

## Unresolved creative decisions (do not decide)

See canon TBD list. Especially blocking for *current* campaign work:

- First transformed-human encounter / first meaningful combat (Block 4 plan written; **not approved to implement**)
- Exact opening tutorial sequence and guidance UI beyond what Blocks 1–3 already shipped
- First Research Station **campaign** placement (Neuro actor exists as implementation; Candidate B not locked)
- Public name of the tactical resource currently called PE
- Overcharged PE Pulse disposition
- Remaining five principal weapon names/specs beyond Lytic Cannon
- Inventory grid UI vs current H-key consumable path

---

# Game design vs what the code actually does

This section is the most important anti-confusion table in the project.

## Combat / targeting

| Topic | Canon | Code / maps | Label |
|---|---|---|---|
| Tactical decision mode, real-time movement | Locked | RMB / trigger; `TacticalTimeDilation = 0.2f`; debug sphere `TacticalSphereRadius = 800.0f` | Sphere + dilation = **PROVISIONAL** code defaults, **not** locked final UI |
| Weak points | Locomotor Nerves / Optical Nodes / Organoid Core appear in legacy rules; canon says not a complete locked roster | `EProjectOrganoidWeakPointType`; tactical multiplier `2.5f` on LocomotorNerves / OrganoidCore | **PROVISIONAL** |
| Biological targeting tutorial | Not for opening Blocks 1–3 | Not in Admin opening | **PROPOSED — NOT APPROVED** for current boundary |
| Hitscan default pistol | Six principal weapons locked as *count/roles*; names incomplete | `AProjectOrganoidDefaultWeapon`: Damage 28, mag 12, reload 1.6s, FireRate 4.5, pistol ammo, hitscan 10000uu | **VERIFIED IMPLEMENTATION** for opening pistol loop; **STALE** Avery/P226 naming in comments |

## Weapons

| Topic | Status |
|---|---|
| Six principal weapons, distinct roles; Lytic Cannon = scarce emergency biological payload | **APPROVED / LOCKED** (names/specs of the other five **UNRESOLVED**) |
| Four-weapon roster | **HISTORICAL / SUPERSEDED** |
| Default pistol mag 12, reload 1.6s, New Game 12/12 loaded + 0 reserve | **VERIFIED IMPLEMENTATION** (Block 3 explicitly did not retune this) |
| Overcharged pulse on default weapon (radius 750, PE 28, cooldown 3.5, damage 18) | **PROVISIONAL** / disposition **UNRESOLVED** in canon |
| Weapon mods (e.g. Stabilized Barrel) | Code exists under `Source/ProjectOrganoid/Weapons/` — **IMPLEMENTED — NOT YET VERIFIED** as opening-campaign content |
| Unlimited ammo | Not intended | Do not add infinite reserve |

## Health / resources / inventory

| Topic | Status |
|---|---|
| Finite healing economy | **APPROVED / LOCKED** direction. Trauma Stabilizer = finite +35. Checkpoint = 25% floor only. |
| Checkpoints restoring full health on save | **HISTORICAL / SUPERSEDED** (removed `bRestoreHealthOnSave` full-heal) |
| Checkpoint 25% floor | **VERIFIED IMPLEMENTATION** — `HealthStabilizationFloorPercent = 0.25` on class CDO; all five campaign instances live-read 0.25 |
| Grid inventory component (8×6, weight cap 40) | **IMPLEMENTED**; **no player-facing grid UI** in opening |
| Block 3 use path: pick up with **E**; heal with **H** (`TryUseFirstHealingConsumable`) | **VERIFIED IMPLEMENTATION** |
| Trauma Stabilizer: HealAmount 35, stack 3, 1×1, weight 1.0, refuse at full health | **VERIFIED IMPLEMENTATION** |
| Pistol ammo pickup qty 8 at Block 3 | **VERIFIED IMPLEMENTATION** |
| Inventing a Resident Evil-style inventory screen | **DO NOT REINTRODUCE** without Tom + ChatGPT/design approval |

## Progression / PE / adaptations / Research Stations

| Topic | Status |
|---|---|
| Research Stations = development infrastructure; free respec; not in combat | **APPROVED / LOCKED** |
| `AProjectOrganoidResearchStation`: 0 SOT cost, no heal, no checkpoint; locked by encounter presence (Pursue/Attack) | **VERIFIED IMPLEMENTATION** of station *class behavior* (see `ResearchStation_Functional`) |
| Neuro placed station `ResearchStation_NeuroGenetics` at `(800, -1600, -1100)` yaw 180 | **VERIFIED IMPLEMENTATION** of placement (`NeuroResearchStationPlacement_Functional`); **not** locked as first *meaningful campaign* intro (Neuro Candidate B **do not implement** until Tom finishes that design) |
| Sterling shop / SOT shopkeeper | **HISTORICAL / SUPERSEDED**; leftover SOT item + upgrade terminal code may still exist — **STALE IMPLEMENTATION** if presented as the progression fantasy |
| PE Energy max 100, drain for abilities | **PROVISIONAL** code; public name TBD |
| Cellular Denature / Bio-Stabilize | **HISTORICAL / SUPERSEDED** names |
| Biological adaptations (e.g. Neural Slow) | Code + `BiologicalAdaptation_Functional` — **VERIFIED** as system tests; not an opening-campaign beat |
| SOT harvest item `DA_Item_SOT` | Asset exists; shop loop superseded |

## Enemies / Hosts / pursuer / bosses

| Topic | Status |
|---|---|
| HostBase chassis: health 100, melee 15, combat states Idle/Investigate/Pursue/Attack/Search/Return/Dead | **VERIFIED IMPLEMENTATION** of *system* (`HostCombatLoop_Functional`) |
| Host AI controller `AProjectOrganoidHostAIController` | Implemented |
| Admin opening Host | **None.** Staging reserved near `~(2680, -470, 100)` — keep clear | **VERIFIED** absence in opening tests |
| First Host / transformed personnel encounter (opening Block 4) | **PROPOSED — NOT APPROVED** as implementation |
| Pursuer | Canon TBD; **do not invent** biology/HP/attacks |
| Bosses | TBD; do not schedule |

## Checkpoints / save / restart / difficulty / tutorial

| Topic | Status |
|---|---|
| Checkpoints restoring full health on save | **HISTORICAL / SUPERSEDED** |
| Five facility checkpoints + 25% floor | **VERIFIED IMPLEMENTATION** (`CheckpointHealth_Functional` 70/70) |
| Death/restart | Independent: successful checkpoint save load restores **saved** (already-stabilized) vitals; PlayerStart / missing-save fallback still `Health = MaxHealth`. Death C++ path was **not** rewritten. |
| Save subsystem `UProjectOrganoidSaveSubsystem` slots `OrganoidSave0` / `OrganoidAutosave` | **IMPLEMENTED** |
| Difficulty philosophy | Not a locked numeric GDD in repo; conserve ammo / not always kill is canon direction |
| Tutorial | Integrated; Blocks 1–3 shipped specific teaching; remaining sequence **UNRESOLVED** |
| Cinematics | Opening Block 3 explicitly **no cinematic**. Do not add one. |
| Marketing opening | Title menu map `Lvl_MainMenu`; `Content/UI/Menus/T_TitleVista.png`; pause/title widgets have been rebuilt in this era — inspect live assets; do not restore deleted green main menu |

## Audio / music

Admin audio zones (S22), ambience subsystem, alarm/hazard/tension beds exist as assets under `Content/Audio/Ambient/`. Facility state can drive lighting and audio listeners (S21–S22). Treat shipped Admin audio as **VERIFIED IMPLEMENTATION** of those section tests; soundtrack/music direction beyond beds is **UNRESOLVED**.

---

# Current implementation inventory

## Opening campaign (the live player path)

New Game does **not** load `DA_Mission_TheAudit`. It seeds `UProjectOrganoidObjectiveSubsystem::SeedOpeningFoundationMission`.

| Block | Content | Status |
|---|---|---|
| **1 Opening Foundation** | Vestibule start; `Mission_OpeningFoundation` “Epitope”; `Obj_ReceptionCheckIn` → `Obj_SecurityStatus`; no TheAudit | **VERIFIED IMPLEMENTATION** — `OpeningFoundation_Functional` |
| **2 Investigation** | Reception + Security terminals/evidence; dressing/branding; `DoorLock_VestibuleToAtrium` authored **non-interactable** at `(1000, 0, 100)`; Research Wing keycard **held** at `(2580, -560, 80)` | **VERIFIED IMPLEMENTATION** — `OpeningInvestigation_Functional` |
| **3 Survival resources** | After Security, before Host. Ammo + Trauma Stabilizer. No new objective. No Host. No biological targeting. No cinematic. | **VERIFIED IMPLEMENTATION** — `OpeningResources_Functional` PASS 103/103 |
| **Checkpoint health floor** | 25% min floor on save; not Block 3/4 content | **VERIFIED IMPLEMENTATION** — `CheckpointHealth_Functional` 70/70 |
| **4 First Host** | Reserved staging `~(2680, -470, 100)` | **PROPOSED / AWAITING TOM APPROVAL** — plan below; **not implemented** |

### Block 3 placements (VERIFIED)

| Actor | Location | Data | Notes |
|---|---|---|---|
| `Pickup_Block3_PistolAmmo` | `(2560, -340, 80)` | `/Game/Data/Items/DA_Item_PistolAmmo` qty **8** | Fresh New Game: 12/12 loaded, 0 reserve; after pickup no shots: 12 loaded, **8 reserve** |
| `Pickup_Block3_TraumaStabilizer` | `(2760, -300, 80)` | `/Game/Data/Items/DA_Item_TraumaStabilizer` qty **1** | Full health: refuse, do not consume. Injured: consume 1, `ApplyHealthDelta(+35)`, clamp MaxHealth |
| Research Wing keycard | `(2580, -560, 80)` | `DA_Item_ResearchWingKeycard` | Still held from Block 2 |
| Host staging | `~(2680, -470, 100)` | — | Keep clear |

Teaching copy (shipped):

- First pickup: “Supplies are stored in your inventory.”
- Ammo HUD: `12 \| Reserve N` (C++ `UProjectOrganoidHUDWidget`; `PROJECTORGANOID_API`)
- Trauma toast: “Trauma Stabilizer acquired. Press H to use.”

Player controls relevant to opening (inspect Enhanced Input assets if you change bindings):

- WASD move, Space jump, E interact, LMB fire, RMB tactical, R reload, Q ability, **H** use first healing consumable

### Opening Block 3 architecture added (VERIFIED)

- `UProjectOrganoidItemData`: `FText Description`, `float HealAmount`
- `UProjectOrganoidInventoryComponent`: `CountItem` / `ConsumeItem` (exact asset)
- `AProjectOrganoidCharacter`: `TryUseConsumable`, `TryUseFirstHealingConsumable`, `UseConsumableAction` → H
- HUD C++ ammo readout + acquisition toast
- Bridge action `spawn_admin_block3_resources` in `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeOpeningBlock3.inl`

## Admin facility sections (world-building, not opening Blocks)

Admin was built in numbered sections. Geometry S1–S12 is treated as **protected authored space**. S13 Blueprint architecture was **frozen pending authorization**. Later sections added controllers, triggers, doors, terminals, hologram, lighting, facility state, audio.

| Area | Approx. meaning | Tests | Label |
|---|---|---|---|
| S14 Sector controller | `AProjectOrganoidAdminSectorController` | (covered indirectly by S18/S21) | **IMPLEMENTED** |
| S15 Room triggers | `AProjectOrganoidAdminRoomTrigger` | `RoomEntryBeep_Functional` | **VERIFIED** for beep hook |
| S16–S17 Access door | `BP_AdminAccessDoor` + C++ listeners | `S17_AccessDoor_Functional` | **VERIFIED** |
| S18 Terminals | Reception, Security, Records, Operations, Executive, Transit | `S18_*_Functional` | **VERIFIED** per terminal test |
| S19 Facility hologram | `BP_AdminFacilityHologram` | `S19_FacilityHologram_Functional` | **VERIFIED** |
| S20 Lighting | `BP_AdminLightController`, zone lights | `S20_AdminLighting_Functional` | **VERIFIED** |
| S21 Facility state | `UProjectOrganoidAdminFacilityStateSubsystem` + door/light listeners | `S21_*_Functional` | **VERIFIED** |
| S22 Audio zones | `BP_AdminAudioZone` | `S22_AdminAudioZones_Functional` | **VERIFIED** |
| Traversal Admin → Neuro | Spine / gate / airlock | `AdminToNeuroTraversal_Functional`, `NeuroAccess_Functional` | **VERIFIED** as traversal tests |

Blueprints live under `Content/ProjectOrganoid/Environment/Admin/Blueprints/`.

**DO NOT** mass-rebuild Admin with `Content/Python/build_admin_section_*.py` unless Tom explicitly authorizes a native-bridge equivalent. Those scripts are historical builders.

## Other sectors

| Map | Role | Label |
|---|---|---|
| `SL_Epitope_NeuroGenetics` | BSL-4 Neuro-Genetics; Research Station actor; `NavMeshBounds_NeuroGenetics`; checkpoint Neuro Airlock | Geometry + station placement **implemented**; Candidate B campaign design **not locked**; Recast dirty **KNOWN SAFE ENGINE BEHAVIOR** |
| `SL_Epitope_Cryo` | Cryogenic; `Checkpoint_FreightAirlock` | Blockout / scaffolding; campaign beats **UNRESOLVED** |
| `SL_Epitope_Compute` | Compute vaults; `Checkpoint_InterfaceChamber` | Same |
| `SL_Epitope_Reactor` | Reactor / incubator; `Checkpoint_BasinRim` | Same |

## Gameplay systems (code modules)

All under `Source/ProjectOrganoid/` unless noted.

| System | Primary types | Status |
|---|---|---|
| Character / vitals | `AProjectOrganoidCharacter` — Health 100, PE 100, BPM, Toxicity, tactical sphere | **PROVISIONAL** vitals; Avery comments **STALE** |
| GameMode / PC | `AProjectOrganoidGameMode`, `AProjectOrganoidPlayerController`, `AProjectOrganoidMainMenuGameMode` | **IMPLEMENTED** |
| Inventory | `UProjectOrganoidInventoryComponent`, `UProjectOrganoidItemData`, pickups | Grid logic **implemented**; UI **KNOWN TECHNICAL LIMITATION** |
| Weapons | `AProjectOrganoidWeapon`, `AProjectOrganoidDefaultWeapon`, `UProjectOrganoidWeaponComponent`, mods | Opening pistol **verified**; roster incomplete vs canon |
| Interaction | `UProjectOrganoidInteractionComponent`, `AProjectOrganoidInteractable`, terminals, doors, power, hazards | **IMPLEMENTED** (many Admin tests) |
| DoorLock | `AProjectOrganoidDoorLock` — power must **not** grant interaction if authored `bIsInteractable=false` | **VERIFIED** — `DoorLockPowerInteractable_Functional` |
| Power | `UProjectOrganoidPowerSubsystem`, `UProjectOrganoidPowerAwareComponent`, `AProjectOrganoidPowerPanel` | **IMPLEMENTED** |
| Hosts | `AProjectOrganoidHostBase`, `AProjectOrganoidHostAIController`, combat types | System **verified**; not in opening |
| Objectives | `UProjectOrganoidObjectiveSubsystem`, mission DAs | Opening seed **verified**; TheAudit/Handover/Production/Conclusion assets **on disk, not New Game** |
| Save / checkpoints | `UProjectOrganoidSaveSubsystem`, `AProjectOrganoidCheckpoint` | **IMPLEMENTED**; heal-on-save **STALE vs canon** |
| Research stations | `AProjectOrganoidResearchStation` | Class + Neuro placement **verified**; campaign intro **not locked** |
| Adaptations | `UProjectOrganoidBiologicalAdaptationComponent` + NeuralSlow | **VERIFIED** as tests |
| Level streaming | `UProjectOrganoidLevelManagerSubsystem` | **IMPLEMENTED** |
| Feedback / audio | Ambience zone/subsystem, telemetry, QA automation | **IMPLEMENTED** (audio tests exist) |
| Encounter presence | `UProjectOrganoidEncounterPresenceSubsystem` | Used to lock Research Station UI in combat |
| Template leftovers | `Variant_Combat`, `Variant_SideScrolling`, `Variant_Platforming` | **Keep** unless replacing for Organoid systems (core directive) |

## Important data assets

`Content/Data/Items/`

- `DA_Item_AdminKeycard`
- `DA_Item_ResearchWingKeycard`
- `DA_Item_PistolAmmo` (existing; Block 3 did not retune)
- `DA_Item_TraumaStabilizer` (**saved** with Block 3)
- `DA_Item_SOT`

`Content/Data/Missions/`

- `DA_Mission_TheAudit` — **do not auto-load on New Game**
- `DA_Mission_TheHandover`
- `DA_Mission_TheProduction`
- `DA_Mission_TheConclusion`

`Content/Data/Dialogue/`

- `DA_Dialogue_IncineratorSurvivor`

---

# Levels / world

## Maps

| Package | Role |
|---|---|
| `/Game/Maps/Lvl_MainMenu` | Editor startup + game default map. Persistent world **often remains this** while Epitope sublevels are streamed for Admin work. |
| `/Game/Maps/Lvl_Epitope` | Campaign persistent world when playing the facility spine. Contains `Spine_Landing_Admin`, `Spine_Bridge_Admin`, `Gate_ResearchWing` (unique on this package for Lvl_Epitope-only saves). |
| `/Game/Maps/Epitope/SL_Epitope_Admin` | Admin & decontamination. Opening Blocks 1–3 live here. |
| `/Game/Maps/Epitope/SL_Epitope_NeuroGenetics` | Neuro |
| `/Game/Maps/Epitope/SL_Epitope_Cryo` | Cryo |
| `/Game/Maps/Epitope/SL_Epitope_Compute` | Compute |
| `/Game/Maps/Epitope/SL_Epitope_Reactor` | Reactor |

**KNOWN TECHNICAL LIMITATION:** Opening another persistent map to “satisfy” an Admin-only save can **unload unsaved streaming-level actors**. Admin-only `save_maps` does **not** require `Lvl_Epitope` as persistent.

## Checkpoints (placed; 25% floor)

Class default `AProjectOrganoidCheckpoint::HealthStabilizationFloorPercent = 0.25f`.  
**DO NOT REINTRODUCE** `bRestoreHealthOnSave` full-heal.

On `TriggerCheckpointSave`, **before** serialize: if `Health < 0.25 * MaxHealth`, `ApplyHealthDelta` up to exactly the floor; otherwise no health change. Repeated activation cannot farm (25 stays 25).

Death/restart does **not** read this property. Successful load uses saved vitals (already floored if they saved while critical). `TryRestartFromPlayerStart` and the no-save checkpoint fallback still set `Health = MaxHealth`.

| Id | Approx. location | Level |
|---|---|---|
| `Checkpoint_ReceptionAtrium` | `(-1535, 0, 60)` | Admin |
| `Checkpoint_NeuroAirlock` | `(1950, 0, -1140)` | Neuro |
| `Checkpoint_FreightAirlock` | `(1950, 0, -2340)` | Cryo |
| `Checkpoint_InterfaceChamber` | `(-2425, -1650, -3540)` | Compute |
| `Checkpoint_BasinRim` | `(25, 0, -4740)` | Reactor |

## Intentional geometry / seams

- Vestibule DoorLock is **authored non-interactable**. That is not a bug.
- Host staging region in Admin must stay clear until Block 4 is approved.
- Spine landing/bridge Y scale was intentionally trimmed via `trim_spine_landing_admin` (Lvl_Epitope actors). Do not “restore” scale without Tom.
- Neuro navmesh bounds actor label `NavMeshBounds_NeuroGenetics` must be **unique on Neuro** for Neuro-only saves.

---

# Unreal environment

| Item | Fact |
|---|---|
| Version | **UE 5.8** (`EngineAssociation` in `.uproject`) |
| Project path (Tom’s machine) | `C:\Users\tomca\Documents\Unreal Projects\ProjectOrganoid` |
| Engine path (Tom’s machine) | `C:\Users\tomca\Desktop\UE_5.8` |
| One editor instance | **Required.** Do not attach a second editor to the same project. |
| Startup map | `Lvl_MainMenu` |
| Live Coding vs campaign C++ | **Conflict — see below.** Do not pick casually. |
| Plugin compile | OrganoidAIBridge is an Editor module; many C++ game changes need **closed-editor UBT**, then **one** reopen |
| Dirty Neuro Recast | **KNOWN SAFE ENGINE BEHAVIOR.** Do not save Neuro to “clean” it. |
| Save All | **Forbidden** |
| `save_asset` on map packages | **Forbidden** |
| Python eval as mutation | **Forbidden** |
| Shell from Unreal plugin | **Forbidden** |

## Live Coding policy (read both; they conflict on purpose)

1. **Campaign Block-style gameplay C++ (Opening Blocks, Hosts, vitals, inventory use):** **No Live Coding.** Close the editor, run UBT, reopen **one** instance. Live Coding has produced incomplete types / missed HUD API exports in this workflow.
2. **Unsaved Admin world work exists:** Do **not** close/restart Unreal (that discards unsaved streaming actors). Prefer keeping the editor open. The automation rule says prefer Live Coding so worlds stay loaded — that applies when the alternative is **losing unsaved Admin actors**, not as a default for Block 3-style C++.

If you need a plugin/game C++ rebuild **and** Admin is dirty: **save Admin first** (dual-approved `save_maps`), confirm no unsaved authored Admin work remains, **then** close editor and UBT.

## `save_maps` persistent-world policy (APPROVED / LOCKED)

`UEditorLoadingAndSavingUtils::SavePackages` saves the explicit `UPackage` list. It does **not** require a particular persistent world except as gated below.

- **Admin-only** `[/Game/Maps/Epitope/SL_Epitope_Admin]`: Admin loaded; PIE stopped; unique Security + Reception on Admin; do **not** include `Lvl_MainMenu` or `Lvl_Epitope`. Persistent **may** be `Lvl_MainMenu`.
- **Neuro-only** `[/Game/Maps/Epitope/SL_Epitope_NeuroGenetics]`: Neuro loaded; PIE stopped; unique `NavMeshBounds_NeuroGenetics` on Neuro; do not include MainMenu, Admin, or Lvl_Epitope. Persistent may be `Lvl_MainMenu`.
- **Two-package** `[Admin, /Game/Maps/Lvl_Epitope]`: persistent **must** be `Lvl_Epitope`.
- **Lvl_Epitope-only** `[/Game/Maps/Lvl_Epitope]`: persistent **must** be `Lvl_Epitope`; unique `Spine_Landing_Admin` and `Gate_ResearchWing` on `Lvl_Epitope`; do not include Admin, Neuro, or MainMenu.

`trim_spine_landing_admin` may open `Lvl_Epitope` as persistent only when Admin is already saved and no world packages are dirty. That action shrinks spine Y scale; it does not save. Persist with Lvl_Epitope-only `save_maps`.

## Failure recovery

- PIE stuck: `stop_playtest` if the bot started PIE; do not Save All to “unstick.”
- Bridge down: editor closed or plugin disabled. Reopen editor with OrganoidAIBridge enabled in `.uproject`.
- Preflight fail: **stop**. Do not retry a different action to force success. Do not weaken tests.
- Dual-approval missing: `execute_write` must fail closed.
- Crash / force-kill editor: may be blocked by local safety tooling; prefer graceful close. Unsaved Admin work is lost if you kill the editor.
- `BiologicalAdaptation_Functional` has shown a **first-run flake** on `dead_target_rejected` reason mismatch; isolated rerun has passed. Treat a single fail as **investigate**, not “rewrite the test.”

## Known editor quirks (KNOWN SAFE ENGINE BEHAVIOR)

- Recast regenerates and dirties Neuro (and sometimes other nav-bearing maps) without authored changes.
- HTTP server binds **localhost only** (UE 5.8 `HTTPServer` default).
- Plugin `EnabledByDefault: false` in `.uplugin` but **enabled in `.uproject`**. That is intentional.
- `DefaultOrganoidAIBridge.ini` has `ReadOnly=True` — this is the **HTTP default**. Mutations still go through gated `execute_write` with `session.read_only=false`, not by flipping that ini casually.
- `Tools/unreal_mcp/README.md` still says Phase 1 `writes_disabled` and “plugin not in uproject.” That README is **STALE**. Live system: dual-approve writes work; plugin **is** in `.uproject`.

---

# Source architecture

## Modules

| Module | Type | Path |
|---|---|---|
| `ProjectOrganoid` | Runtime | `Source/ProjectOrganoid/` |
| `ProjectOrganoidPlaytest` | Editor, `PostEngineInit` | `Source/ProjectOrganoidPlaytest/` |
| `OrganoidAIBridge` | Editor plugin, `PostEngineInit` | `Plugins/OrganoidAIBridge/` |

`ProjectOrganoid.Build.cs` public deps include Core, Engine, EnhancedInput, AIModule, GameplayTasks, NavigationSystem, StateTree, UMG, HTTP, Json. Host combat loop is **not** a StateTree (comment on `EProjectOrganoidHostCombatState`).

## Naming

- `AProjectOrganoid*`, `UProjectOrganoid*`, `FProjectOrganoid*`
- Headers: `CoreMinimal.h` … `*.generated.h` last
- Designer categories: `Vitals`, `Tactical`, `Inventory`, `Weapons`, `PE` (PE category name is **STALE vs canon public-name TBD**, but still the code category)

## Architectural boundaries

- Prefer extending existing `AProjectOrganoid*` types over parallel frameworks.
- Keep `Variant_*` template code unless replacing it for Organoid systems.
- Playtest module must not mutate assets (`playtest_mutates_assets=false`).
- Bridge mutations: allowlisted `action` strings only; native Unreal API on game thread; no Python eval.
- Map packages save only via `save_maps`, never `save_asset`.
- Opening mission seed is **code**, not `DA_Mission_TheAudit`.

## Save architecture

`UProjectOrganoidSaveSubsystem` serializes vitals, inventory, weapon mods, objectives, stats. Checkpoints and objective autosaves default to `OrganoidAutosave`. Checkpoint activation applies the 25% health floor **before** `SaveAtCheckpoint` captures vitals.

## Inventory architecture

Grid packing + weight on `UProjectOrganoidInventoryComponent`. Pickups add items. Opening player **does not** open a grid. Healing is `TryUseFirstHealingConsumable` (first item with `HealAmount > 0`).

## Combat architecture

`AProjectOrganoidWeapon` ballistics (hitscan/projectile), magazine, reload, tactical weak-point multiplier. Character owns `UProjectOrganoidWeaponComponent`. Hosts implement damageable/weak-point interfaces.

## Campaign / objective architecture

`UProjectOrganoidObjectiveSubsystem` holds runtime objectives and event triggers (`Event_ReceptionTerminalUsed`, `Event_SecurityTerminalUsed`). Mission data assets exist for later acts but are not the New Game path.

## AI architecture

`AProjectOrganoidHostAIController` + HostBase state enum. EQS pieces exist under leftover `Variant_Combat/AI` — do not assume they drive Hosts.

## Editor-only

- OrganoidAIBridge HTTP server, command dispatch, write gate, playtest dispatch
- ProjectOrganoidPlaytest registry, editor subsystem (tickable), PIE host

---

# Automation / tooling

## What the current workflow actually is

Cursor/Grok is **one client**. The Unreal side is a **localhost HTTP command server**. Qwen does not need MCP. Qwen needs HTTP (or the Python client).

```
AI client (Cursor MCP stdio OR python Tools/unreal_mcp/client.py OR any HTTP POST)
    POST http://127.0.0.1:8732/v1/command
OrganoidAIBridge (Unreal Editor, game thread)
    read commands | playtest commands | gated writes
```

Implementation:

- Server: `Plugins/OrganoidAIBridge/Source/OrganoidAIBridge/Private/OrganoidAIBridgeServer.cpp`
- Commands: `OrganoidAIBridgeCommands.cpp`
- Writes: `OrganoidAIBridgeWrites.cpp` + `*.inl` (OpeningBlock2/3, ResearchStation, NavMesh, Audio, Lighting, Hologram, AccessDoor, Traversal, …)
- Playtest bridge: `OrganoidAIBridgePlaytest.cpp` → `IOrganoidPlaytestHost` implemented by `UProjectOrganoidPlaytestEditorSubsystem`
- Python: `Tools/unreal_mcp/server.py` (MCP Content-Length stdio), `client.py` (CLI), `run_named_playtest.py` (poll helper), `call_unreal()`

Cursor MCP example config: `Tools/unreal_mcp/cursor-mcp.example.json`. Optional. Not required for Qwen.

## Read vs mutation

**Read (session.read_only true is fine):**  
`ping`, `get_editor_state`, `get_pie_state`, `get_player_pawn`, `get_actor`, `get_actor_property`, `get_light_component`, `get_component`, `list_actors_near`, `get_collision`, `capsule_sweep`, `overlap_query`, `get_output_log`, `find_blueprint`, `get_blueprint_components`, `get_blueprint_members`, `get_user_defined_enum`, `get_blueprint_graph`, `inspect_playing_audio`, `list_playtests`, `get_playtest_status`, `get_playtest_result`, `list_changes`, `get_change`

**Playtest (does not mutate assets):**  
`run_playtest`, `get_playtest_status`, `get_playtest_result`, `stop_playtest`

**Gated writes:**  
`prepare_write` (no mutation) → `approve_write` ×2 distinct identities → `execute_write` with `session.read_only=false` and exact `change_id`.  
Also: `reject_write`.

Direct mutation command names from the old README (`set_visibility` as a top-level MCP tool, etc.) are **not** how writes work. Writes are **prepare/approve/execute** with an `action` field.

## Allowlisted write `action` values (inspect `OrganoidAIBridgeWrites.cpp` before using)

Durable examples actually present in code (not a promise they are all safe to run):

`set_visibility`, `set_collision`, `set_transform`, `set_component_property`, `set_actor_property`, `delete_actor`, `rerun_construction`, `add_blueprint_variable`, `add_scs_component`, `connect_blueprint_pins`, `compile_blueprint`, `save_asset` (**not for map packages**), `save_maps`, `move_actor_to_level`, `spawn_blueprint_actor`, hologram/light/room-trigger/door listener authors, S20 light actions, S22 audio spawn/delete, `spawn_neuro_navmesh_bounds`, `spawn_neuro_research_station`, `spawn_admin_research_wing_connector`, `spawn_admin_research_wing_keycard`, `trim_spine_landing_admin`, `set_admin_doorlock_interactable`, `spawn_admin_block2_dressing`, `spawn_admin_block3_resources`

Preflight is **fail-closed**. Many actions require `required_world_package` matching Admin vs Neuro vs Lvl_Epitope.

**Adding a new action means C++ in the plugin, closed-editor compile, dual approval to use it.** Do not eval Python as a shortcut.

## Approval architecture

1. `prepare_write` — snapshot, preflight, `change_id`, **zero mutation**
2. `approve_write` `role=user` identity **Tom** (non-empty)
3. `approve_write` `role=second_review` **different** identity (historically Grok)
4. `execute_write` `session.read_only=false`

The agent must **never** independently decide to mutate. Tom should not routinely click Levels/Save All for allowlisted tasks; after dual approval the agent executes.

Qwen replacement: Tom still approves as user. Second review must be a **different** human or a **different** designated reviewer identity — **not** the same Qwen session approving itself twice.

## Playtest Bot

- Registry: `FOrganoidPlaytestRegistry`
- Cases: `Source/ProjectOrganoidPlaytest/Private/Tests/*_Functional.cpp`
- Host: `UProjectOrganoidPlaytestEditorSubsystem` (Tickable editor subsystem)
- Async: `run_playtest` queues and returns `run_id`
- `playtest_mutates_assets=false`
- Bot may start/stop PIE for the test; `stop_playtest` ends PIE only if the bot started it
- Does not save, does not compile, does not discard editor work on stop (by design)

## Python scripts under `Content/Python/`

Historical/one-off builders, diagnostics, recoveries (Admin sections, epitope data, pause menu, etc.). **Not** the approved mutation path when a native bridge action exists. Unreal Python remote eval is **DO NOT REINTRODUCE**.

`__pycache__` files are noise.

## Launcher / watchdog

No first-class Organoid launcher/watchdog product is in-repo as a required runtime. Pinokio/Gepeto skills in the agent environment are **unrelated** unless Tom later asks. **OPTIONAL / FUTURE.**

---

# Security / safety (APPROVED / LOCKED)

- Durable mutations: prepared payload + Tom approval + distinct second review + exact `change_id` + fail-closed preflight
- No Python eval
- No shell from the Unreal plugin
- No Save All
- No `save_asset` on map packages
- No blind continuation after failed validation
- Idempotent spawns where preflight expects unique labels (do not duplicate `Pickup_Block3_*`)
- Create/validate-before-delete where applicable
- Map/package gating (`required_world_package`, unique actor checks)
- Do not weaken tests merely to pass
- Do not close Unreal with unsaved Admin actors
- Do not save Neuro Recast dirt without authorization
- Do not invent Block 4 / Host / cinematic / inventory UI / Candidate B Neuro station campaign beat
- Do not store secrets, tokens, or passwords in this file or in commits
- Bot permissions: playtest cannot mutate assets
- HTTP bind localhost only

**Idempotency:** re-running an approved spawn that already exists should preflight-fail or no-op uniquely — treat duplicates as bugs.

---

# Testing

## How to run

Editor open, bridge up:

```
python Tools/unreal_mcp/client.py list_playtests
python Tools/unreal_mcp/client.py run_playtest "{\"test_id\":\"<TestId>\"}"
# poll status until complete
python Tools/unreal_mcp/client.py get_playtest_result "{\"run_id\":\"<run_id>\"}"
```

Prerequisites: usually Admin (and other streamed Epitope levels) loaded; PIE stopped before some writes; enough time for async run.

Expected result: structured JSON with assertions, expected vs actual, `ok` / pass counts. **VERIFIED** means the named test passed after the change, plus any required regression suite, plus targeted save if assets changed.

**Do not** treat a compile as verification. **Do not** treat a screenshot as verification for gameplay.

## Inventory of registered functional tests

Paths: `Source/ProjectOrganoidPlaytest/Private/Tests/`

| TestId | Protects |
|---|---|
| `OpeningFoundation_Functional` | New Game opening seed, vestibule, two objectives, no TheAudit load |
| `OpeningInvestigation_Functional` | Reception/Security investigation, DoorLock, dressing, RW keycard; **updated** to expect authorized Block 3 pickups (world-state update, not a weaken) |
| `OpeningResources_Functional` | Block 3 ammo + Trauma Stabilizer loop; five checkpoints present with **0.25 floor** (not full-heal) |
| `CheckpointHealth_Functional` | 25% floor cases, no farm, saved health, death restores saved (not MaxHealth), Trauma +35 / refuse-at-full, respawn anchor, editor-world five-instance floor |
| `AdminToNeuroTraversal_Functional` | Admin → Neuro traversal |
| `NeuroAccess_Functional` | Research wing access / Neuro entry |
| `S17_AccessDoor_Functional` | Admin access door |
| `S18_ReceptionTerminal_Functional` | Reception terminal |
| `S18_SecurityTerminal_Functional` | Security terminal |
| `S18_RecordsTerminal_Functional` | Records terminal |
| `S18_OperationsTerminal_Functional` | Operations terminal |
| `S18_ExecutiveTerminal_Functional` | Executive terminal |
| `S18_TransitTerminal_Functional` | Transit terminal |
| `S19_FacilityHologram_Functional` | Facility hologram |
| `S20_AdminLighting_Functional` | Admin lighting zones |
| `S21_AdminFacilityState_Functional` | Facility state subsystem |
| `S21_AccessDoorListener_Functional` | Door listens to facility state |
| `S21_AdminLightingStateListener_Functional` | Lights listen to facility state |
| `S22_AdminAudioZones_Functional` | Admin audio zones |
| `RoomEntryBeep_Functional` | Room-entry beep |
| `BeepClickInjection_Functional` | Beep click injection |
| `BeepSourceIsolation_Functional` | Beep source isolation |
| `BeepMixCapture_Functional` | Beep mix capture |
| `BeepMixCapture_CombatMute_Functional` | Combat mute mix (second id in same file) |
| `DeconAudioIsolation_Functional` | Decon audio isolation |
| `AmbienceLayerPlayback_Functional` | Ambience layers |
| `DoorLockPowerInteractable_Functional` | Power must not override authored non-interactable |
| `AmmoReload_Functional` | Magazine / reserve / reload |
| `PETactical_Functional` | Tactical mode / PE (provisional systems) |
| `HostCombatLoop_Functional` | Host combat states (not Admin opening placement) |
| `ResearchStation_Functional` | Station rules: 0 SOT, no heal, encounter lock |
| `NeuroResearchStationPlacement_Functional` | Neuro station unique transform |
| `BiologicalAdaptation_Functional` | Adaptation system; **known first-run flake** on `dead_target_rejected` |

## Block 3 regression suite (what “verified” meant)

After Block 3: `OpeningResources_Functional` 103/103 plus the opening/Admin regression set used in that session (OpeningFoundation, OpeningInvestigation, AmmoReload, and related Admin tests as run). Isolated `BiologicalAdaptation_Functional` rerun **PASS** after one flake.

Future Qwen: a change is **VERIFIED** only if:

1. The tests that protect the changed behavior pass
2. You did not edit assertions to match a worse product
3. Required packages were saved with `save_maps` / explicit DA save
4. This handoff’s milestone/changelog was updated

## Harness limitations

- Async; must poll
- Needs editor + plugin
- May start PIE; don’t concurrently `save_maps`
- Flakes exist (BiologicalAdaptation)
- Does not replace reading `get_actor` for world coordinates
- Does not cook a shipping build

---

# Failed experiments / technical history (DO NOT REINTRODUCE)

Recorded because they explain architecture, not because the chat was interesting.

| Failure | Why it matters |
|---|---|
| Unreal Python eval / remote Python as the AI mutation channel | Arbitrary code; violates allowlist. Native named commands only. |
| File → Save All | Writes unrelated dirty packages (Neuro Recast, etc.) |
| `save_asset` on map packages | Wrong API; use `save_maps` |
| Opening `Lvl_Epitope` as persistent to save Admin | Can unload unsaved Admin streaming actors |
| Closing/restarting Unreal with unsaved Admin | Data loss |
| Live Coding for Opening Block C++ / HUD API export | Incomplete types; Block 3 required closed-editor UBT |
| Treating Neuro Recast dirty as authored | Do not save to “clean” |
| Expecting Neuro/Cryo/Compute/Reactor checkpoints to exist in **early Lvl_Epitope PIE** | They live on streamed sublevels. Assert those instances in the **editor world** after PIE (see `CheckpointHealth_Functional` durable + `OpeningResources`). |
| Checkpoint full-heal (`bRestoreHealthOnSave`) | **SUPERSEDED.** Floor only. |
| Inventing RE-style inventory UI for Block 3 | Explicitly out of scope; H-key is the shipped path |
| Weakening playtest assertions to pass | Forbidden. OpeningInvestigation **was** updated to expect Block 3 pickups because the world **authorized** those pickups — that is a world-state update, not a weaken. |
| DoorLock: power grants interactable when authored false | Real bug class; test exists; do not regress |
| HUD as BlueprintImplementableEvent with no C++ visual | Ammo/toast had to be C++ `PROJECTORGANOID_API` |
| README Phase-1 `writes_disabled` | Stale docs; do not disable writes based on that paragraph |
| Restoring Avery / four weapons / Sterling shop / Denature+Stabilize as current design | Superseded |
| Implementing Neuro Candidate B because it is “preferred” | Preferred ≠ approved to build |
| Force-killing UnrealEditor | May be blocked; can lose unsaved work |
| `Tools/unreal_mcp/_tmp_*.py` as the normal client | Prefer `client.py` / `python -c` + `call_unreal` |
| Treating ChatGPT/Grok conversation as repo source of truth | Conversations die; this file + git + Unreal remain |

---

# Source-of-truth contradictions (do not silently resolve)

| Topic | Canon / Tom | Implementation / docs | Resolution |
|---|---|---|---|
| Protagonist | Nathan Grant | Avery in `.cursorrules`, character comments, inventory comment | **STALE IMPLEMENTATION** |
| Weapons | Six principal; Lytic scarce; four-weapon list superseded | Default pistol + P226-style mag 12 comments | Keep pistol as opening tool; do not restore four-weapon *canon* |
| PE skill names | Cellular Denature / Bio-Stabilize superseded; public PE name TBD | PE energy + Q ability still in code | **PROVISIONAL** |
| Tactical UI | Debug sphere not final; radius not locked | 800uu sphere still runs | **PROVISIONAL** |
| Progression | Research Stations, free respec; Sterling shop superseded | Upgrade/SOT/crafting leftovers possible | Stations are the intended infrastructure; shop fantasy **STALE** |
| Hosts in Admin opening | None yet | Host combat tests exist; Admin has zero Hosts | Tests are **system** tests, not campaign placement |
| Checkpoint heal | Finite healing economy | 25% floor on all five campaign checkpoints | **VERIFIED IMPLEMENTATION** (2026-09-01) |
| Inventory UI | Player must use healing | No grid UI; H-key | **KNOWN TECHNICAL LIMITATION** for now |
| Neuro Research Station | Candidate B preferred; do not implement campaign intro yet | Actor exists at Neuro coords | Placement **verified**; campaign meaning **not locked** |
| Live Coding | Automation rule: prefer if unsaved Admin would be discarded | Block work: No Live Coding | **Both true** in different situations; see Unreal environment |
| MCP README | Says writes disabled, plugin not enabled | Plugin enabled; writes gated and working | **STALE DOCUMENTATION** |
| Full Restored Canon v1.0 prose | Design authority | **Not in the repository** | **UNRESOLVED gap for Qwen** until Tom files or pastes it |
| `.cursorrules` / core.mdc | Marked superseded for design | Still always-applied in Cursor | Follow canon index, not Avery pitch |

---

# Current development state (exceptionally precise)

**As of 2026-09-01 (checkpoint floor verified).**

## Latest verified milestone

**Checkpoint 25% minimum-health stabilization floor** (2026-09-01), with Opening Blocks 1–3 still verified.

Rule: on successful checkpoint save, if health < 25% of MaxHealth, raise to exactly 25%; if already ≥ 25%, do not change health. Not +25%, not +25 HP, not full heal, not repeatable farming.

## Exact campaign progression implemented

1. New Game → vestibule → check in at Reception → check the security office  
2. Investigation dressing, evidence, locked vestibule door, held RW keycard  
3. Resource loop (ammo + Trauma Stabilizer)  
4. Checkpoints stabilize critically wounded Nathan to 25% then save  
5. **Stop.** First Host not placed.

## Exact technical state

- Engine 5.8 C++ project with OrganoidAIBridge (`VersionName` 0.5.9 on ping)
- `AProjectOrganoidCheckpoint::HealthStabilizationFloorPercent = 0.25f` (CDO). Live instance read 2026-09-01: all five campaign checkpoints **0.25**. No map save required.
- Death restart C++ unchanged: load save if present; else teleport + MaxHealth; PlayerStart + MaxHealth
- Editor dirty after this task: Neuro Recast only (**KNOWN SAFE ENGINE BEHAVIOR**). Do not save Neuro.

## Last targeted saves (this task)

**None.** C++/playtest only. Maps not modified.

Prior Block 3 saves still stand: Admin + `DA_Item_TraumaStabilizer`.

## Work approved but not implemented as a write

None for checkpoint health (done).

## Work proposed but not approved

- Opening Block 4: first transformed security employee / first true survival-horror encounter — **plan in this file, awaiting Tom/ChatGPT**
- Biological targeting tutorial
- Opening cinematic
- RE-style inventory grid UI
- Neuro Candidate B as first meaningful Research Station intro

## Immediate next development task

**Await Tom approval of Opening Block 4 plan.** Do not implement Hosts in Admin until that approval exists.

## Dependencies / blockers

1. Tom (+ ChatGPT if he wants) locks Block 4 encounter location/offset, trigger, Host tunables, whether a new objective is added, and death-restart teaching
2. Dual-approve any Admin spawn write (`spawn_blueprint_actor` or a new allowlisted spawn)
3. Closed-editor UBT if Host C++ tunables change
4. New `OpeningBlock4` / Host-in-Admin playtest + regressions
5. Targeted Admin `save_maps` only after verification

## What Qwen should inspect first when taking over

1. This file + `PROJECT_ORGANOID_CANON.md`  
2. `get_editor_state` dirty packages  
3. `CheckpointHealth_Functional` or `OpeningFoundation_Functional` smoke  
4. Actors: Block 3 pickups, DoorLock, Reception/Security, five checkpoints’ `HealthStabilizationFloorPercent`  
5. Confirm **zero** Hosts in Admin  
6. If continuing campaign: the Block 4 proposal below — **do not implement until Tom approves**

Opening Blocks 1–3 player-facing loop is unchanged: Security investigation, pistol ammo (8 reserve after pickup), one Trauma Stabilizer, **H** to heal when damaged, refuse at full health, HUD ammo `12 | Reserve N`. Mag 12 / reload 1.6s / starting 12+0 unchanged. No Host in Admin.

---

# Opening Block 4 — recommended implementation plan

**Label: PROPOSED / AWAITING TOM APPROVAL. Not implemented. Not verified.**

Do **not** treat this section as permission to spawn a Host.

## Intent

First transformed-human encounter and first true survival-horror combat beat after Blocks 1–3. A **single** former Epitope **security employee** in the Security office, using existing `AProjectOrganoidHostBase` + `AProjectOrganoidHostAIController`. Not a boss, not the pursuer, not a horde, not Node Zero, not a full biological-targeting tutorial, not a science lecture.

## Live geometry (read 2026-09-01, `list_actors_near` origin `(2680, -470, 100)` r=450)

Reserved Block 3 staging `~(2680, -470, 100)` sits **inside the Security office**, 70uu from `Admin_Terminal_Security` `(2680, -400, 110)` and **64uu from `Admin_Block2_Security_Chair` `(2680, -510, 50)`**.

Nearby (do not delete; Block 3 keep-clear was loot vs staging, not vs furniture):

| Actor | Location | Dist to staging |
|---|---|---|
| `Admin_Terminal_Security` | `(2680, -400, 110)` | 71 |
| `Admin_RoomTrigger_Security` | `(2680, -400, 150)` | 86 |
| `Admin_Block2_Security_Chair` | `(2680, -510, 50)` | 64 |
| `Pickup_ResearchWingKeycard` | `(2580, -560, 80)` | 136 |
| `Pickup_Block3_PistolAmmo` | `(2560, -340, 80)` | 178 |
| `Pickup_Block3_TraumaStabilizer` | `(2760, -300, 80)` | 189 |
| East/south walls | ~`(2890, -365–390, 200)` | 250+ |

**Recommended spawn (needs Tom lock):** do **not** place a full Host capsule exactly on `(2680, -470, 100)` — it will intersect the chair and crowd the terminal. Prefer a floor position **south of the chair / toward the east wall**, still obviously “this is the security officer’s body in this room,” e.g. candidate **`(2780, -520, 100)`** or **`(2680, -620, 100)`** after a capsule sweep. Exact coordinates require Tom + a PIE capsule overlap check before write.

## How the player first perceives them

Recommended (needs approval): after Security terminal use (Blocks 2–3 already complete that beat), Nathan turns and sees a **still** transformed officer — slumped or standing idle at the desk/chair, **not** immediately sprinting from off-screen. First lesson is recognition (“this was a person / this is wrong”), then the Host **Investigate → Pursue** when Nathan aims, shoots, or gets close.

Starting state: `Idle` (or Investigate if Tom wants them already aware). No rage, no bio-shield, no second Host.

## Encounter trigger / progression

- **No new main objective required** unless Tom wants one. Existing `Obj_SecurityStatus` already completes on terminal use. Adding `Obj_SurviveSecurityHost` would be a **creative approval** item.
- Trigger: Host is **already in the room** (authored actor), not a cinematic spawn. Player can pick ammo/trauma **before** engaging if they entered for Block 3 first (current pickup layout supports that).
- After victory: Host `Dead`; room remains traversable; RW keycard still held if not taken; **no** Neuro unlock change; **no** Research Station intro; no Node Zero text.

## Combat lesson

- Guns work; melee hurts (Host melee **15**, player **100** HP → ~7 hits to down if they stand still — threatening, not a sponge).
- Ammo: New Game 12 loaded + Block 3 **+8 reserve** = 20 pistol rounds. Default pistol **28** damage, Host **100** HP → ~4 torso hits if no weak-point bonus. Teach **aim + back up**, not dump the mag. Do **not** require Optical/Locomotor tutorial UI; default weapon 2.5× tactical weak-point can remain **PROVISIONAL** if they enter tactical, but Block 4 must be completable **without** teaching the full weak-point roster.
- Trauma Stabilizer remains the finite +35, not a combat heal fountain.
- Death: restart from last **activated checkpoint** (Reception atrium if they saved there; otherwise PlayerStart / opening). Death health = **saved** health (25% floor if they saved critical). Do not special-case full heal for this fight.

## Reuse existing Host architecture

Use `AProjectOrganoidHostBase` with `AProjectOrganoidHostAIController`. Neuro already has `Host_Neuro_1` for **system** tests — **do not** move those into Admin. Admin opening must stay **one** Host.

Suggested Block 4 tunables (approval items, not code yet):

- Health 80–100 (class default 100 is acceptable)
- Melee 12–15
- No bio-shield, no rage
- Idle until proximity or gunfire (existing Investigate-on-noise exists in HostCombatLoop)

## Assets / classes likely requiring modification (if approved later)

- Admin map spawn: unique label e.g. `Host_Admin_SecurityOfficer` on `SL_Epitope_Admin`
- Possibly a new allowlisted bridge action `spawn_admin_block4_host` (fail-closed unique label, no overlap with pickups/chair/terminal, Admin package only)
- Optional: dressing (blood/uniform cue) — **creative approval**; do not invent lore
- Tests: new `OpeningBlock4_Functional`; update `OpeningResources`/`OpeningInvestigation` **only** if they currently assert zero Admin Hosts (they do — that becomes “exactly one named Host after Block 4,” not a weaken)
- **No** TheAudit load; **no** cinematic sequence actor

## Automated test plan (if later approved)

- Unique Host on Admin at approved transform
- Zero Hosts in vestibule/reception
- Block 3 pickups still present
- Host starts Idle; gunfire or proximity → Investigate/Pursue
- Host can damage player; player can kill Host
- Death restart still uses checkpoint save (not MaxHealth unless that is what was saved)
- `OpeningFoundation` / `OpeningInvestigation` / `OpeningResources` / `CheckpointHealth` / `AmmoReload` regressions
- Durable: only Admin dirty if spawn wrote the map; no Neuro save

## Requires Tom’s creative approval before any implementation

1. Exact spawn transform (staging coordinate is **too tight** as-is)
2. Idle tableau vs immediately aggressive
3. Whether a new objective appears
4. Host display name / log line (security employee, not “zombie,” not Avery)
5. Whether first sight happens **before** or **after** picking Block 3 ammo (layout currently allows both)
6. Any unique visual beyond HostBase blockout mesh

---

# Qwen successor architecture (non-Cursor)

**Intended stack:** Qwen3.8 (planning / architecture / review) + Qwen Coder (implementation worker when useful) + Ollama (local execution) + a harness that can see this repo and talk to Unreal the same way `Tools/unreal_mcp/client.py` does.

Do not assume Cursor, Grok, MCP, or ChatGPT exist.

Where current functionality depends on Cursor: it is **optional sugar**. The Unreal protocol is HTTP JSON POST to `127.0.0.1:8732/v1/command`.

Replacement integration must expose at least the **REQUIRED** capabilities below and must **not** grant silent `execute_write`.

---

# Integration requirements for a future Qwen harness

Derived from the **actual** current workflow (not a wish list).

## REQUIRED

| Capability | Why | Permission boundary |
|---|---|---|
| Repository read | Canon, this handoff, source, tests, plugin | Read-only until Tom authorizes writes |
| Search / grep | Find classes, test ids, allowlisted actions | Read |
| File inspect (text) | `.h/.cpp/.ini/.md/.py/.uproject` | Read |
| HTTP to OrganoidAIBridge | Same as `call_unreal` | Read commands default; writes only after dual approval |
| `prepare_write` / `approve_write` / `execute_write` / `reject_write` / `get_change` / `list_changes` | Durable Unreal mutations | execute requires `session.read_only=false` **and** dual approval **and** Tom-approved scope |
| Playtest `list/run/status/result/stop` | Verification | No asset mutation |
| Poll async playtest | `run_playtest` returns immediately | Read |
| Command execution for UBT | Closed-editor compile | Local process; not Unreal shell |
| Log inspect | `get_output_log` + UBT stdout | Read |
| Targeted package save | `save_maps` allowlisted packages only | Dual-approved write |
| Project-state inspect | `get_editor_state`, actors, properties | Read |
| Diff inspect | Know what you changed before save | Read git / editor dirty list |

## HIGHLY DESIRABLE

| Capability | Why | Permission boundary |
|---|---|---|
| Repository write for `Source/` and this handoff | C++ / tests / docs | Tom-approved implementation scope only |
| Process inspection | Is UnrealEditor running? One instance? | Read; do not force-kill by default |
| Unreal launch | Reopen after UBT | User-visible; don’t launch a second copy if one exists |
| MCP stdio (`Tools/unreal_mcp/server.py`) | Drop-in if the host is Cursor-like | Same HTTP backend |
| Blueprint inspect commands already on the bridge | Admin BP work | Read; writes gated |
| Token header support | If Tom set `ORGANOID_BRIDGE_TOKEN` | Do not log the token |

## OPTIONAL / FUTURE

| Capability | Why |
|---|---|
| Cook / package shipping build | Not used in current Block workflow |
| Cloud agents | Current work is local editor |
| Unreal Python | Explicitly rejected as mutation channel |
| Automatic second-review identity | Must remain a **distinct** reviewer, not self-approve |
| Watchdog / 1-click launcher | Not required today |
| Full Restored Canon v1.0 ingest | Needed for complete design; file missing from repo |
| Cursor-specific MCP, Composer, Bugbot | Convenience only |

## How to verify a replacement harness

1. `ping` → ok  
2. `get_editor_state` → map + dirty list  
3. `list_playtests` → catalog includes `OpeningFoundation_Functional`  
4. `run_playtest` + poll → structured pass/fail  
5. `prepare_write` with a **deliberately wrong** payload → preflight **fails**, world unchanged  
6. Confirm `execute_write` without approvals **fails**  
7. Confirm no path exists to eval Python inside Unreal  
8. Confirm the harness cannot Save All  

If any of 5–8 fail, the harness is **not** a replacement.

---

# Living handoff protocol

After every **VERIFIED** implementation:

1. Update **Current development state** (milestone, campaign, saves, next boundary)  
2. Update the relevant system/level/test sections if facts changed  
3. Append **one** changelog row below  
4. Do **not** rewrite the entire history  

If a task **fails** or Tom **stops** it: do not move the verified milestone. Optional discovery row with **DISCOVERED DURING FAILED/STOPPED TASK — TASK NOT VERIFIED**.

Future Grok/Qwen: this file is mandatory output of verified work, same as tests.

---

# Changelog

| Date | Milestone | Result | Packages saved | Notes |
|---|---|---|---|---|
| 2026-09-01 | Handoff baseline | Document created | none (markdown only) | Editor bridge unreachable during write; Block 3 remains last **VERIFIED** game milestone |
| 2026-09-01 | Checkpoint 25% floor | **VERIFIED** | none (C++/tests only) | Replaced full-heal bool with `HealthStabilizationFloorPercent=0.25`. `CheckpointHealth_Functional` 70/70. OpeningResources 103/103, OpeningFoundation 41/41, OpeningInvestigation 69/69, AmmoReload 57/57, HostCombatLoop 30/30. Death load still restores **saved** health. Block 4 plan recorded as **PROPOSED / AWAITING TOM APPROVAL**. |

---

# FIRST ACTIONS YOU MUST NOT TAKE

- Do not place a Host  
- Do not implement Block 4 until Tom approves the plan in this file  
- Do not restore checkpoint full-heal (`bRestoreHealthOnSave`)  
- Do not save Neuro  
- Do not Save All  
- Do not restore Avery, four-weapon canon, Sterling shop, or Denature/Stabilize as current design  
- Do not treat this document as permission to mutate  

Tom remains the owner. Ask him.
