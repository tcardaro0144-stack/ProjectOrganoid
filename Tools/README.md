# ProjectOrganoid Packaging & Diagnostics Pipeline

## Shipping package
```powershell
# From repo root — requires UE_ROOT or auto-detected Epic install
.\Tools\Package-Shipping.ps1
.\Tools\Package-Shipping.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8" -ArchiveDirectory "D:\Builds\Organoid"
.\Tools\Package-Shipping.ps1 -DryRun
```

## Strip developer cook paths
```powershell
.\Tools\Strip-DeveloperContent.ps1
.\Tools\Validate-Pipeline.ps1
```

Excludes Variant_* demo maps, Developers/Collections, and editor-only content via `Config/DefaultGame.ini` `DirectoriesToNeverCook`.

## Platform profiles (runtime)
`UProjectOrganoidPlatformProfileSubsystem` auto-applies PC High or Console Performance.
- Force console presets: launch with `-consoleprofile` or set `bForceConsoleProfile`
- Device CVars: `Config/DefaultDeviceProfiles.ini`

## Crash / telemetry upload
`UProjectOrganoidDiagnosticsUploader` listens to `UProjectOrganoidTelemetrySubsystem::OnCrashReportWritten`,
packages `Saved/ProjectOrganoid/Telemetry` + engine logs + `Saved/Crashes` under
`Saved/ProjectOrganoid/Diagnostics/`, optionally zips, and POSTs when `bConsentToUpload=true`.
