# Copyright Woogle. All Rights Reserved.
function Get-WxProjectProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$ProjectFile,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Processes
    )
    $project = [IO.Path]::GetFullPath($ProjectFile).Replace('/', '\')
    $root = Split-Path $project -Parent
    $name = [IO.Path]::GetFileNameWithoutExtension($project)
    $binaryDirectory = [IO.Path]::GetFullPath((Join-Path $root 'Binaries\Win64'))
    foreach ($process in $Processes) {
        if ($process.Name -match '^UnrealEditor(?:-Win64-(?:Debug|DebugGame|Development|Shipping|Test))?\.exe$') {
            $projects = @()
            foreach ($token in [regex]::Matches([string]$process.CommandLine, '(?:[^\s"]+|"[^"]*")+')) {
                $argument = $token.Value.Replace('"', '')
                if ($argument -match '^-project=(.+)$') {
                    $argument = $Matches[1]
                } elseif ($argument.StartsWith('-')) {
                    continue
                }
                if ($argument -notmatch '(?i)\.uproject$') { continue }
                # Relative paths have no reliable working directory in Win32_Process.
                if ($argument -notmatch '^(?:[A-Za-z]:[\\/]|\\\\[^\\]+\\[^\\]+\\)') {
                    $projects += $null
                    continue
                }
                try { $projects += [IO.Path]::GetFullPath($argument).Replace('/', '\') }
                catch { $projects += $null }
            }
            if ($projects.Count -eq 1 -and [string]::Equals($projects[0], $project, [StringComparison]::OrdinalIgnoreCase)) {
                $process
            }
        } elseif ($process.Name -match ('^' + [regex]::Escape($name) + '(?:-Win64-(?:Debug|DebugGame|Development|Shipping|Test))?\.exe$')) {
            if (!$process.ExecutablePath) { continue }
            try {
                $executable = [IO.Path]::GetFullPath($process.ExecutablePath)
                if ([string]::Equals((Split-Path $executable -Parent), $binaryDirectory, [StringComparison]::OrdinalIgnoreCase)) {
                    $process
                }
            } catch { continue }
        }
    }
}