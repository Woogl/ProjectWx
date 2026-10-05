# Copyright Woogle. All Rights Reserved.
# Runs a short Play In Editor session through unreal-mcp and reports error log lines that appeared during it.
# It catches errors, ensures, Blueprint script errors (Accessed None) and spawn failures; it cannot judge gameplay, which still needs a person.
# -Map opens that level first; it refuses when the current level has unsaved changes, because the save prompt would block MCP.
# Exit codes: 0 = no error lines, 1 = error lines found, 2 = the check could not run.
[CmdletBinding()]
param(
    [int]$Port = 8000,
    [string]$Map,
    [int]$Seconds = 20
)

$Invoke = Join-Path $PSScriptRoot 'Invoke-UnrealMcp.ps1'
$ErrorPattern = ': Error:|Ensure condition failed|Accessed None|Script Msg|LogSpawn: Warning|Fatal error'
$TimestampPattern = '^\[(\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3})\]'

function Invoke-Tool([string]$Toolset, [string]$Tool, [string]$Arguments = '{}', [int]$TimeoutSeconds = 300) {
    $text = (& $Invoke -Port $Port -Toolset $Toolset -Tool $Tool -Arguments $Arguments -TimeoutSeconds $TimeoutSeconds) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw "$Toolset.$Tool failed: $text" }
    return ($text | ConvertFrom-Json).returnValue
}

function Get-LogLines([string]$Pattern, [int]$MaxEntries) {
    return @(Invoke-Tool 'EditorToolset.LogsToolset' 'GetLogEntries' ('{"category":"","pattern":' + (ConvertTo-Json $Pattern) + ',"maxEntries":' + $MaxEntries + '}'))
}

$pieStarted = $false
try {
    if (Invoke-Tool 'EditorToolset.EditorAppToolset' 'IsPIERunning') { throw 'A play session is already running.' }

    if ($Map) {
        $current = Invoke-Tool 'editor_toolset.toolsets.scene.SceneTools' 'get_current_level'
        if ($current -ne $Map) {
            if (Invoke-Tool 'editor_toolset.toolsets.asset.AssetTools' 'is_dirty' ('{"asset_path":' + (ConvertTo-Json $current) + '}')) {
                throw "$current has unsaved changes; save or discard them before switching levels."
            }
            Invoke-Tool 'editor_toolset.toolsets.scene.SceneTools' 'load_level' ('{"level_path":' + (ConvertTo-Json $Map) + '}') | Out-Null
        }
    }
    $level = Invoke-Tool 'editor_toolset.toolsets.scene.SceneTools' 'get_current_level'
    Write-Output "WX_PIE_MAP=$level"

    # The log tool cannot filter by time, so the newest timestamp before PIE marks where this session's lines begin.
    $marker = Get-LogLines '' 50 | ForEach-Object { if ($_ -match $TimestampPattern) { $Matches[1] } } | Select-Object -Last 1
    if (-not $marker) { throw 'Could not read a timestamped log line before starting PIE.' }

    $pieStarted = $true
    Invoke-Tool 'EditorToolset.EditorAppToolset' 'StartPIE' ('{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":' + $Seconds + '}}') ($Seconds + 600) | Out-Null

    # The editor's git provider logs errors for PIE's transient /Memory packages; they are not game errors.
    $lines = @(Get-LogLines $ErrorPattern 0 | Where-Object { $_ -match $TimestampPattern -and [string]::CompareOrdinal($Matches[1], $marker) -gt 0 -and $_ -notmatch 'SourceControl: Error: .*/Memory' })
    $groups = @($lines | Group-Object { $_ -replace '^(\[[^\]]*\])+', '' })
    foreach ($group in $groups) {
        Write-Output ("WX_PIE_LOG=" + $(if ($group.Count -gt 1) { "($($group.Count)x) " } else { '' }) + $group.Name)
    }
    Write-Output "WX_PIE_ERROR_LINES=$($lines.Count)"
    if ($lines.Count -gt 0) {
        Write-Output 'WX_PIE_RESULT=errors'
        $exitCode = 1
    } else {
        Write-Output 'WX_PIE_RESULT=clean'
        $exitCode = 0
    }
} catch {
    Write-Output "WX_PIE_ERROR=$($_.Exception.Message)"
    Write-Output 'WX_PIE_RESULT=failed'
    $exitCode = 2
} finally {
    if ($pieStarted) {
        try { if (Invoke-Tool 'EditorToolset.EditorAppToolset' 'IsPIERunning') { Invoke-Tool 'EditorToolset.EditorAppToolset' 'StopPIE' | Out-Null } }
        catch { Write-Output "WX_PIE_ERROR=Could not stop PIE: $($_.Exception.Message)" }
    }
}
exit $exitCode
