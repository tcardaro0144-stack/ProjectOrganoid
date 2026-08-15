# Quick validation that packaging scripts and cook-exclude config exist.

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

$required = @(
    "Tools\Package-Shipping.ps1",
    "Tools\Strip-DeveloperContent.ps1",
    "Config\DefaultGame.ini"
)

foreach ($rel in $required) {
    $path = Join-Path $RepoRoot $rel
    if (-not (Test-Path $path)) { throw "Missing: $rel" }
    Write-Host "OK $rel"
}

& (Join-Path $PSScriptRoot "Strip-DeveloperContent.ps1") -RepoRoot $RepoRoot
Write-Host "Pipeline self-check passed."
