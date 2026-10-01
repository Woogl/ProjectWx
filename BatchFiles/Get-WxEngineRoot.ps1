# Copyright Woogle. All Rights Reserved.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectFile,
    [string]$LauncherDataPath = (Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat')
)

$ErrorActionPreference = 'Stop'
$ProjectData = Get-Content -LiteralPath $ProjectFile -Raw | ConvertFrom-Json
$Association = [string]$ProjectData.EngineAssociation
if ([string]::IsNullOrWhiteSpace($Association)) { throw "Missing EngineAssociation: $ProjectFile" }

if (Test-Path -LiteralPath $LauncherDataPath -PathType Leaf) {
    $LauncherData = Get-Content -LiteralPath $LauncherDataPath -Raw | ConvertFrom-Json
    $Entry = $LauncherData.InstallationList | Where-Object { $_.AppName -eq "UE_$Association" } | Select-Object -First 1
    if ($Entry -and (Test-Path -LiteralPath $Entry.InstallLocation -PathType Container)) {
        return $Entry.InstallLocation
    }
}

throw "Engine '$Association' from '$ProjectFile' was not found in '$LauncherDataPath'."
