# PROJECT ORGANOID — RESTORED CANON v1.0

**DESIGN AUTHORITY**

This file is the project’s game-design authority index. It does **not** replace Restored Canon v1.0 as delivered by the owner. The full v1.0 text currently lives in owner-approved conversation/session material and is **not** duplicated here, to avoid inventing or compressing locked design.

If a complete Restored Canon v1.0 document is later placed in the repository, this file remains the pointer and hierarchy. Until then, this file plus the owner’s Restored Canon v1.0 messages are the design source. Repository code, comments, missions, Python, and Cursor rules are **not**.

---

## Hierarchy

1. **Restored Canon v1.0 (owner-approved)** — game design.
2. **This file** — authority index and superseded-list for collaborators.
3. **Existing Unreal implementation** — evidence of current project state, not design authority.
4. **Legacy `.cursorrules`, `.mdc`, C++ comments, Python, mission text** — may contain superseded concepts. Implementation notes may still be useful.

**Canon changes require explicit owner approval.**  
**AI collaborators must not silently reinterpret canon.**  
**UNKNOWN does not mean fall back to legacy.**  
**Working compatible systems should be preserved rather than rewritten for taste.**

---

## Locked identity (index only)

- Third-person survival-horror RPG.
- Inspirations: modern Resident Evil structure + Parasite Eve tactical/RPG spirit. Not templates to copy.
- **Protagonist: Nathan Grant.** Male. Pronouns: he/him. Visible in third person. Outside investigator/respondent experienced with unusual biological incidents, investigation, containment procedure, emergency response, and firearms. Capable of real-time movement during tactical decision mode. **Not a research scientist.** **Nathan knows how to deal with biological hazards; he does not necessarily know what they are.**
- **Avery Vance is superseded / historical only.** Do not restore Avery Vance as current canon. Nathan Grant is a later explicit Tom-approved revision and therefore survives restoration. An intermediate 2026-08-26 label “Nathan Vance” is also superseded (Vance belongs to Tom’s separate novel *Glitched Reality*).
- Full Nathan Grant biography (employer, military history, age, appearance, family, tragic backstory, previous Epitope relationship, reason for arriving at Epitope) is **CANON CONFIRMATION REQUIRED** — do not invent those fields and do not copy the Avery-era JSOC combat-medic biography onto Nathan automatically.
- Epitope progression: Admin → NeuroGenetics → Cryo → Compute → Reactor / Incubator.
- Opening tutorial is **integrated into the campaign**, not a detached training room. Two layers: (1) the player learns to control Nathan’s already-known fundamentals; (2) Project Organoid-specific systems are introduced gradually. Nathan does not explain to himself how to reload.
- Transformed personnel exist and should read as former Epitope people (scientists, researchers, technicians, security, other staff), not generic zombies. Complete transformation mechanism is **not** finalized. HostBase remains a technical chassis, not the final enemy taxonomy (The Integrated and later archetypes).
- Winning an encounter does not always mean killing the enemy (fight / disable / stagger / bypass / retreat / escape / conserve ammo).
- Six principal weapons with distinct roles. Lytic Cannon is the scarce emergency biological payload. Four-weapon roster is superseded. Do not restore an Arc Gun that duplicates Lytic’s job.
- Unlimited ammunition is not intended campaign behavior.
- PhotoScan is a foundation for Bio-Scan, not a competing design.
- HostBase is a technical chassis, not the final enemy taxonomy (The Integrated and later archetypes).
- Sterling-as-shopkeeper / escalating SOT shop is superseded. Research Stations are the development infrastructure. Free respec at stations (not in combat).
- Global time dilation tactical implementation is **provisional**, not canonical behavior.
- Tactical debug sphere is not the final targeting UI. Tactical **radius is not locked** (neither ~800uu nor 8000uu).
- Admin Sections 1–12 geometry is protected. Section 13 is frozen until explicitly authorized.
- No conventional magical safe rooms. Safety is a player-created state.

---

## Approved canon — NeuroGenetics revelation (2026-08-31)

Owner-approved clarification. Design record only. Does **not** authorize map, mission, objective, or actor implementation.

### Science / horror principle

Project Organoid’s horror is rooted in modern and emerging biological science. Real or emerging concepts are the foundation; fictional extrapolation pushes them into survival-horror territory.

**Start with real or emerging science → extrapolate it into plausible-enough science fiction → explore the horrifying consequence.**

Relevant territory may include neural organoids, neural cultures, nervous-system manipulation, neural plasticity, bio-silicon interfaces, biological computing, adaptive tissue, neural signaling, tissue regeneration, experimental biotechnology, and related modern biotechnology. Naming that territory here does **not** canonize a specific catastrophe mechanism. Any major scientific explanation still requires Tom’s approval.

Desired player reaction, occasionally: the science feels disturbingly adjacent to the real world. The science should make the horror worse, not turn the game into a lecture.

Do not claim real vaccines cause neurological transformation. Do not present speculative game science as claims about real-world medicine.

### Nathan and the science

Nathan knows how to deal with biological hazards; he does not necessarily know what they are. He can recognize operationally significant abnormalities (abnormal tissue behavior, contamination, nervous-system damage or alteration, containment failure, a biological process behaving unexpectedly) without immediately knowing the scientific cause.

The player and Nathan should often learn the deeper science together. He must not spontaneously possess expert knowledge of advanced neural organoids, bio-silicon systems, experimental neural engineering, or other specialized Epitope research unless the game has first given him a believable source.

His reaction may include surprise, disgust, confusion, skepticism, alarm, dry humor, or understated confidence while keeping calm-under-pressure competence. He is not an emotionless superhero. Training helps him survive stranger situations; it does not magically supply scientific answers.

Show biological evidence before fully explaining it. Early language should stay cautious (“Something's changing the way their nervous systems work”) rather than claiming an exact mechanism. The critical Neuro revelation must be understandable on the main path; optional exploration may deepen science, personnel stories, foreshadowing, and resources. Do not hide the central plot only inside optional datapads.

### NeuroGenetics narrative function

During Nathan’s investigation of NeuroGenetics, he discovers evidence that **the victims’ nervous systems are being reorganized/adapted by something connected to Epitope’s neural research.**

Neuro should move his understanding approximately from:

- “Something has gone catastrophically wrong inside Epitope.”

to:

- “These people are not simply infected or injured. Their nervous systems are being deliberately or systematically reorganized by a process connected to the neural research conducted here.”

That is the revelation. Neuro must **not** yet answer who/what ultimately controls the process, why it is occurring, the complete mechanism, its full relationship to Epitope’s larger program, the final role of Node Zero, the final vaccine mechanism, or the final role of Dr. Sterling.

Existing Avery/Sterling-era Neuro material (missions, datapads, survivor dialogue, Python builders) remains historical/stale implementation evidence. It is **not** canonized by this clarification.

### Progression design context (not an implementation order)

The 2026-08-31 read-only NeuroGenetics progression survey remains authoritative for **what currently exists** in the level. Its recommended structural direction was Candidate B: Neuro power restoration as the progression spine, with Cryo held so the floor cannot be sequence-broken immediately. That remains the **preferred design direction**. **Do not implement Candidate B until Tom and ChatGPT finish detailed campaign design** (opening tutorial sequence, first transformed encounter, exact Neuro room order, required vs optional discoveries, targeting tutorial, scientist encounters, whether the first pursuer appears in Neuro, first Research Station placement, Neuro climax, Cryo unlock).

---

## Approved design — opening tutorial + Neuro campaign direction (2026-08-31)

Owner-approved clarification. Design record only. Does **not** authorize implementation, maps, missions, UI, controls, enemies, the pursuer, or Candidate B.

Maintain a strict distinction:

1. **Approved canon/design** — this file and owner-approved Restored Canon v1.0 messages.
2. **Current implementation** — what Unreal actually does (survey-authoritative for Neuro).
3. **Stale/historical implementation** — Avery/Sterling-era missions, pads, dialogue, shop terminal. Evidence only.
4. **Proposed/unresolved design** — not canon until Tom approves.

Existing assets and prior AI text do not become canon merely by existing.

### Core identity

Modern survival horror + RPG mechanics + modern/emerging biological science. Inspirations include modern Resident Evil structure/tension and Parasite Eve biological/RPG identity; Project Organoid must develop its own identity. Same science→fiction→consequence principle as the Neuro revelation section.

Early campaign should escalate conceptually from investigation → signs something is wrong → containment irregularities → biological evidence → resource discovery → first genuine danger → first transformed personnel → increasingly abnormal biology → deeper Epitope science → NeuroGenetics revelation. This is **not** a locked room-by-room sequence.

### Opening tutorial (integrated)

The campaign has already begun. Tutorialization happens through Nathan’s investigation.

- **Layer 1 — Controlling Nathan:** movement, camera, interaction, investigation, navigation/objectives, sprint, inventory, ammo, reload, healing, firearm combat, creating distance, retreat/evasion. Nathan already knows these. The player is learning controls. No self-narrating reload lectures.
- **Layer 2 — Learning Project Organoid:** introduce gradually — tactical targeting; Targeting Sphere as the current *mechanic to teach*, not a lock of the debug-sphere presentation; anatomical/biological weak points; different biological targets producing different effects; resource conservation; enemies as biological problems rather than simple HP bars; eventual adaptations, Epitope Syringe abilities, RPG progression, Research Stations; deciding whether to kill, disable, bypass, stagger, escape, or conserve ammo. Do not dump all of this at once.

Tutorial guidance may later support Full / Minimal / Reduced-or-Off. Unique Organoid systems must remain understandable unless a future setting explicitly suppresses essentially all assistance. Exact settings/UI are **not** finalized.

### Transformed personnel

Nathan should encounter zombie-esque transformed humans. Some should clearly have been Epitope employees. Transformations may produce different behaviors and combat characteristics. **Do not finalize the complete transformation mechanism. Do not automatically create new enemy classes.** Design exact enemies separately. Prefer fewer, more threatening, meaningfully placed threats over horde density.

### Biological targeting (design philosophy, not a locked roster)

Neuro is a strong place to teach **why** biological targets matter. Understanding the biology should give tactical options. Weak points should not feel like arbitrary glowing shooter dots.

Examples of the *intended effect language* (not a complete locked taxonomy, and not an order to add missing hitboxes this pass): Locomotor Nerve Cluster → impair movement; Optical Nodes → impair vision; Exposed Core → high damage; Neural Stems → disrupt certain advanced movement/reflex behavior; Auditory Nodes → impair tracking.

Current HostBase implementation exposes three hitboxes (Locomotor Nerves, Optical Nodes, Organoid Core). HostBase remains a chassis. Tactical **radius is not locked**. The debug sphere is **not** the final targeting UI; “Targeting Sphere” here means the tactical targeting system the player must learn.

### First major pursuer (intended direction, placement not locked)

A recurring Mr. X / Nemesis-style pursuer is intended. NeuroGenetics is a **candidate** for the first meaningful introduction, not final placement canon.

The first encounter should teach through gameplay that this is a different category of threat and that standing still to kill it with current resources is the wrong assumption. Do not simply display “THIS ENEMY CANNOT BE KILLED.” Possible communication: extreme resilience, recovering from apparent damage, Nathan reacting to abnormal durability, environmental destruction, an obvious escape opportunity, temporary stagger rather than conventional defeat.

**Do not finalize** biology, appearance, identity, origin, HP, attacks, or narrative connection.

Difficulty philosophy: major **authored** appearances should stay broadly consistent across difficulties so narrative pacing holds. Difficulty may change how dangerous those encounters are (damage, durability, aggression, stagger recovery, pursuit persistence, detection, reaction speed, pressure, resource generosity). Do not canonize random pursuer spam. Optional dynamic extra appearances may later vary by difficulty if testing proves they help. Fear from uncertainty and pressure, not merely frequency.

### Intended Neuro chapter shape (design target, not an implementation order)

Arrival → establish scientific environment → evidence of containment/research failure → transformed personnel → discover systems/power compromised → exploration branches (resources, science, danger, optional discoveries) → deeper nervous-system evidence → power-restoration as structural spine → targeting knowledge becomes more meaningful → significant transformed-scientist encounter(s) → possible first pursuer escalation → power restored / state changes → Nathan reaches the Neuro revelation → Cryo route becomes legitimately available → chapter transition.

Exact ordering may change during detailed campaign design.

Neuro should answer **one** major question (what is happening to these people → nervous systems systematically reorganized/adapted in connection with Epitope neural research) and open larger ones (who directs it, why, how it spreads, what Epitope was developing, how it connects to the larger catastrophe). Increase curiosity; do not collapse the mystery.

### Research Stations

Remain the deeper RPG/build management location. Free reconfiguration of already-unlocked elements, no currency/respec penalty, not during combat/boss fights. Introduce when the player has enough context to understand why customization matters. Do not revive the Sterling shop-terminal structure. **Exact first Research Station placement remains to be designed** (a Neuro station actor already exists in implementation; that does not lock it as the first *meaningful* introduction).

---

## Explicitly superseded (do not restore)

| Legacy claim | Status |
|---|---|
| Complete roster: P226 / M4 / Cryo Lancer / Denaturing Launcher | SUPERSEDED |
| Avery Vance as current protagonist | SUPERSEDED / HISTORICAL ONLY |
| Nathan Vance as current protagonist | SUPERSEDED (intermediate 2026-08-26 name; current name is Nathan Grant) |
| Avery as ex-JSOC combat medic (as design) | SUPERSEDED |
| Cellular Denature / Bio-Stabilize as the ability kit | SUPERSEDED |
| Sterling upgrade shop with escalating SOT cost as the progression model | SUPERSEDED |
| Debug sphere / global 0.2 dilation as final tactical design | SUPERSEDED as canon (code may still run) |
| First-person conversion | NOT CANON |

---

## TBD in Restored Canon v1.0 (do not invent)

Exact remaining five weapon names/specs beyond Lytic; full syringe kit; Node Zero implementation details beyond the locked concept; Overcharged PE Pulse disposition; public name of the tactical resource currently called PE; storage-box specifics; enemy/boss implementation schedules; NG+ route details; the complete Neuro transformation mechanism; who/what directs that process; why it is occurring; its full relationship to Epitope’s larger program; final vaccine mechanism; final Sterling role; exact opening tutorial sequence and guidance UI; first transformed-human encounter; first meaningful combat encounter; exact Neuro room order; required vs optional Neuro discoveries; biological-targeting tutorial beats; transformed-scientist encounters; whether the first pursuer introduction occurs in Neuro; pursuer biology/appearance/identity/origin/HP/attacks; first Research Station placement; Neuro climax; Cryo unlock/transition. These stay unknown until the owner supplies them. They do **not** restore the superseded rows above. Mentioning neural organoids / bio-silicon / Neural Stems / Auditory Nodes as relevant territory or targeting examples does not lock a specific in-game mechanism or a complete weak-point roster.

---

## Collaborator rule

When implementation and Restored Canon v1.0 disagree: **canon wins**.  
When two design sources disagree: **owner-approved Restored Canon v1.0 wins**.  
When owner approval cannot be determined: **stop and ask**.

## AI transfer record

`PROJECT_ORGANOID_MASTER_AI_HANDOFF.md` is the living orientation document for repository layout, Unreal tooling, Playtest Bot, verified implementation, and the current development boundary. It is **not** design authority. Update it after every VERIFIED implementation task.
