# Project Organoid

Survival horror / tactical action RPG (Resident Evil × Parasite Eve) in Unreal Engine **5.8.3**.
Protagonist Avery Vance / Nathan Grant in the Epitope Subterranean Complex.

Design authority: `Tools/unreal_mcp/PROJECT_ORGANOID_CANON.md` (Restored Canon v1.0).
Session / beat log: `PROJECT_STATE.md` (implementation history; not design authority).

## Final build (Beat 33 published)

| Item | Value |
|---|---|
| Engine | 5.8.3 (`EngineAssociation` 5.8) |
| Target | Win64 Development `ProjectOrganoidEditor` |
| Catalog | **63/63** `COMPLETE_PASS` · **5423** assertions |
| Beep | **12/12** one-shot · ~1.2s · 878.57 Hz (`SW_AlarmPulse` `bLooping` false) |
| NavMesh | Recast `needs_rebuild` deferred after paint; catalog `NAVMESH NEEDS TO BE REBUILT` **0**; `gray_remaining` **0** |
| Nathan | Final Drake mesh `SKM_NathanGrant_Final` · **13** slots |
| Camera (OTS) | arm **180** · socket `(0,45,22)` · FOV **92** · lag **10** · collision true · `yawFollowsLook` true |
| World color | Admin · NeuroGenetics · Cryo · Compute · Reactor done |
| Node Zero | Destroy vs Extract · NG+ **only on Extract** · Sterling lines + escape cinematic |
| Conclusion | Destroy vs Extract · credits roll · save cleanup · NG+ flag **only on Extract** |
| Commit chain | `522776a` (Research Station) → … → `378da42` (Conclusion polish) |

Sector map prefixes (inside the tracked **34**): Admin `2a9e21bb…` · Neuro `73e5da44…` · Cryo `46e05eb9…` · Compute `69043b7a…` · Reactor `cfb8b9cb…` · Conclusion mission `7f952f18…`.

## Build

```powershell
& "C:\Users\tomca\Desktop\UE_5.8\Engine\Build\BatchFiles\Build.bat" ProjectOrganoidEditor Win64 Development -Project="$PWD\ProjectOrganoid.uproject" -WaitMutex
```

## Playtests

Editor automation uses OrganoidAIBridge (`http://127.0.0.1:8732/v1/command`) with dual-approve writes.
Catalog ends at `NodeZero_Functional` (63 tests).

## Packaging

See `Tools/README.md` for shipping package, cook strip, and diagnostics upload.

## Credits (in-game)

`BP_CreditsRoll` / `AProjectOrganoidCreditsRoll`: Tom Cardaro · Project Organoid · Engine 5.8.3 · Beats 19–33 · 63/63 COMPLETE_PASS 5423 · 34 hashes · thank you.
