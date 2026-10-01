# Copyright Woogle. All Rights Reserved.

function Stop-WxProjectProcesses {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$ProjectFile)

    $processes = @(Get-CimInstance Win32_Process -ErrorAction Stop)
    $targets = @(Get-WxProjectProcess -ProjectFile $ProjectFile -Processes $processes)
    foreach ($target in $targets) {
        $process = $null
        try {
            $process = Get-Process -Id $target.ProcessId -ErrorAction Stop
        } catch {
            # A process can exit between enumeration and opening its handle.
            if ($_.CategoryInfo.Category -eq [Management.Automation.ErrorCategory]::ObjectNotFound) { continue }
            throw
        }
        try {
            $null = $process.Handle
            if (-not $process.HasExited) {
                Stop-Process -InputObject $process -Force -ErrorAction Stop
                if (-not $process.HasExited) {
                    Wait-Process -InputObject $process -Timeout 30 -ErrorAction Stop
                }
            }
            $process.Refresh()
            if (-not $process.HasExited) { throw 'Process is still running after the exit wait.' }
        } catch {
            $reason = $_.Exception.Message
            $process.Refresh()
            if (-not $process.HasExited) {
                throw "Could not stop project process $($target.ProcessId): $reason"
            }
        }
        $target.ProcessId
    }
    # Also catch project instances started while the original processes were stopping.
    $remaining = @(Get-WxProjectProcess -ProjectFile $ProjectFile -Processes @(Get-CimInstance Win32_Process -ErrorAction Stop))
    if ($remaining.Count -gt 0) {
        throw "Project processes are still running: $($remaining.ProcessId -join ', '). Build cancelled."
    }
}
