# ProjectOrganoid — Shipping build + package automation
# Compiles Shipping, cooks/stages with pak, excludes developer content.
#
# Usage (from repo root or anywhere):
#   .\Tools\Package-Shipping.ps1
#   .\Tools\Package-Shipping.ps1 -Platform Win64 -Configuration Shipping -ArchiveDirectory "D:\Builds\Organoid"
#   .\Tools\Package-Shipping.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
#
# Requires: Unreal Engine 5.x with RunUAT.bat

[CmdletBinding()]
param(
    [ValidateSet("Win64", "Linux", "Mac")]
    [string]$Platform = "Win64",

    [ValidateSet("Shipping", "Development", "DebugGame")]
    [string]$Configuration = "Shipping",

    [string]$EngineRoot = $env:UE_ROOT,

    [string]$ArchiveDirectory = "",

    [switch]$SkipBuild,
    [switch]$SkipCook,
    [switch]$NoPak,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$UProject = Join-Path $RepoRoot "ProjectOrganoid.uproject"
if (-not (Test-Path $UProject)) {
    throw "UProject not found: $UProject"
}

function Resolve-EngineRoot {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint "Engine\Build\BatchFiles\RunUAT.bat"))) {
        return (Resolve-Path $Hint).Path
    }

    $candidates = @(
        $env:UE_ROOT,
        $env:UE5_ROOT,
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Program Files\Epic Games\UE_5.7",
        "C:\Program Files\Epic Games\UE_5.6",
        "C:\UE\UE_5.8"
    ) | Where-Object { $_ }

    foreach ($c in $candidates) {
        $uat = Join-Path $c "Engine\Build\BatchFiles\RunUAT.bat"
        if (Test-Path $uat) { return (Resolve-Path $c).Path }
    }

    # Association from .uproject
    $assoc = Get-ItemProperty -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine" -ErrorAction SilentlyContinue
    if ($assoc -and $assoc.InstalledDirectory) {
        $uat = Join-Path $assoc.InstalledDirectory "Engine\Build\BatchFiles\RunUAT.bat"
        if (Test-Path $uat) { return $assoc.InstalledDirectory }
    }

    throw "Could not locate Unreal Engine. Pass -EngineRoot or set UE_ROOT."
}

$EngineRoot = Resolve-EngineRoot -Hint $EngineRoot
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
Write-Host "Engine: $EngineRoot"
Write-Host "Project: $UProject"

# Strip / exclude developer content before cook (non-destructive copy of exclude list applied via ini)
$StripScript = Join-Path $PSScriptRoot "Strip-DeveloperContent.ps1"
if (Test-Path $StripScript) {
    Write-Host "Refreshing cook-exclude list..."
    & $StripScript -RepoRoot $RepoRoot
}

if (-not $ArchiveDirectory) {
    $ArchiveDirectory = Join-Path $RepoRoot "Saved\StagedBuilds\$Platform-$Configuration"
}

$uatArgs = @(
    "BuildCookRun",
    "-project=`"$UProject`"",
    "-noP4",
    "-platform=$Platform",
    "-clientconfig=$Configuration",
    "-serverconfig=$Configuration",
    "-utf8output"
)

if (-not $SkipBuild) { $uatArgs += "-build" }
if (-not $SkipCook) {
    $uatArgs += @(
        "-cook",
        "-unversionedcookedcontent",
        "-allmaps"
    )
}

$uatArgs += @(
    "-stage",
    "-package",
    "-archive",
    "-archivedirectory=`"$ArchiveDirectory`""
)

if (-not $NoPak) {
    $uatArgs += "-pak"
}

# Shipping: omit editor/dev extras
if ($Configuration -eq "Shipping") {
    $uatArgs += @(
        "-nodebuginfo",
        "-prereqs",
        "-iostore"
    )
}

$cmdLine = "`"$RunUAT`" $($uatArgs -join ' ')"
Write-Host "UAT command:"
Write-Host $cmdLine

if ($DryRun) {
    Write-Host "DryRun — exiting without invoking UAT."
    exit 0
}

$proc = Start-Process -FilePath $RunUAT -ArgumentList $uatArgs -NoNewWindow -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    throw "RunUAT failed with exit code $($proc.ExitCode)"
}

Write-Host "Packaging complete."
Write-Host "Archive: $ArchiveDirectory"
exit 0
