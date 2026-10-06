# Copyright Woogle. All Rights Reserved.
# Starts an Unreal Editor for this project on a free MCP port and waits until its MCP server serves the WxToolset tools.
# It never stops other editors, so several sessions can each keep their own editor. Build first (build-doctor) if code changed.
# -List only prints the MCP servers already listening and which process owns each.
# A startup dialog (Restore Packages) blocks the editor's game thread and MCP with it, so the script stops and reports it instead of answering it.
# Exit codes: 0 = ready (or listed), 1 = the editor exited, is blocked by a startup dialog, or did not answer in time, 2 = setup error.
[CmdletBinding()]
param(
    [string]$ProjectRoot = (Join-Path $PSScriptRoot '..\..'),
    [ValidateSet('Development', 'DebugGame')][string]$Configuration = 'Development',
    [int]$Port,
    [int]$TimeoutSeconds = 600,
    [switch]$List
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PortRange = 8000..8099
# Restoring overwrites the saved assets with autosaves and closing discards them, so a person decides.
$BlockingDialogTitles = @('Restore Packages')

try {
    if (-not ('WxWindowList' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class WxWindowList
{
    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowText(IntPtr hWnd, StringBuilder text, int maxCount);

    public static string[] VisibleTitles(int processId)
    {
        var titles = new List<string>();
        EnumWindows((hWnd, lParam) =>
        {
            uint owner;
            GetWindowThreadProcessId(hWnd, out owner);
            var text = new StringBuilder(256);
            if (owner == (uint)processId && IsWindowVisible(hWnd) && GetWindowText(hWnd, text, text.Capacity) > 0)
            {
                titles.Add(text.ToString());
            }
            return true;
        }, IntPtr.Zero);
        return titles.ToArray();
    }
}
'@
    }

    . (Join-Path $PSScriptRoot '..\skills\build-doctor\scripts\Get-WxEditorBuildContext.ps1')
    . (Join-Path $PSScriptRoot '..\skills\run-editor\scripts\Get-WxProjectProcess.ps1')
    $context = Get-WxEditorBuildContext -ProjectRoot $ProjectRoot -RequireEditor

    $processes = @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'")
    $projectPids = @(Get-WxProjectProcess -ProjectFile $context.ProjectFile.FullName -Processes $processes | ForEach-Object { $_.ProcessId })
    $listeners = @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $PortRange -contains $_.LocalPort })
    foreach ($listener in $listeners | Sort-Object LocalPort -Unique) {
        $owner = $processes | Where-Object { $_.ProcessId -eq $listener.OwningProcess } | Select-Object -First 1
        $ownerName = if ($owner) { $owner.Name } else { (Get-Process -Id $listener.OwningProcess -ErrorAction SilentlyContinue).ProcessName }
        $scope = if ($projectPids -contains $listener.OwningProcess) { 'this-project' } else { 'other' }
        Write-Output "WX_MCP_LISTENING=port $($listener.LocalPort) pid $($listener.OwningProcess) $ownerName $scope"
    }
    if ($List) { exit 0 }

    $usedPorts = @($listeners | ForEach-Object { $_.LocalPort })
    if ($Port) {
        if ($usedPorts -contains $Port) { throw "Port $Port is already in use." }
    } else {
        $Port = $PortRange | Where-Object { $usedPorts -notcontains $_ } | Select-Object -First 1
        if (-not $Port) { throw "No free port in $($PortRange[0])-$($PortRange[-1])." }
    }

    $editorExe = $context.EditorExe
    $gameModule = Join-Path $context.ProjectRoot 'Binaries\Win64\UnrealEditor-WxGame.dll'
    if ($Configuration -eq 'DebugGame') {
        $editorExe = Join-Path (Split-Path $editorExe -Parent) 'UnrealEditor-Win64-DebugGame.exe'
        $gameModule = Join-Path $context.ProjectRoot 'Binaries\Win64\UnrealEditor-WxGame-Win64-DebugGame.dll'
    }
    # Missing project modules make the editor open a rebuild prompt that blocks MCP.
    if (-not (Test-Path -LiteralPath $gameModule -PathType Leaf)) { throw "Project modules for $Configuration are not built: $gameModule" }

    $editor = Start-Process -FilePath $editorExe -ArgumentList @(('"' + $context.ProjectFile.FullName + '"'), "-ModelContextProtocolPort=$Port") -PassThru
    Write-Output "WX_MCP_PID=$($editor.Id)"
    Write-Output "WX_MCP_PORT=$Port"
    Write-Output "WX_MCP_URL=http://127.0.0.1:$Port/mcp"

    $invoke = Join-Path $PSScriptRoot 'Invoke-UnrealMcp.ps1'
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds 5
        if ($editor.HasExited) {
            Write-Output "WX_MCP_RESULT=exited (code $($editor.ExitCode)); see Saved\Logs"
            exit 1
        }
        $dialog = @([WxWindowList]::VisibleTitles($editor.Id) | Where-Object { $BlockingDialogTitles -contains $_ })
        if ($dialog) {
            Write-Output "WX_MCP_RESULT=blocked by the '$($dialog[0])' dialog; the editor (pid $($editor.Id)) waits for a person to answer it"
            exit 1
        }
        # The server listens before plugins finish registering, so wait for the project toolsets.
        $toolsets = (& $invoke -Tool list_toolsets -Port $Port -TimeoutSeconds 10) -join "`n"
        if ($LASTEXITCODE -eq 0 -and $toolsets -match 'WxToolset\.') {
            $python = if ($toolsets -match 'editor_toolset\.') { 'yes' } else { 'no' }
            Write-Output "WX_MCP_PYTHON_TOOLSETS=$python"
            Write-Output 'WX_MCP_RESULT=ready'
            exit 0
        }
    }
    $titles = [WxWindowList]::VisibleTitles($editor.Id) -join "', '"
    Write-Output "WX_MCP_RESULT=timeout after $TimeoutSeconds s; the editor (pid $($editor.Id)) is still running with windows '$titles'"
    exit 1
} catch {
    Write-Output "WX_MCP_ERROR=$($_.Exception.Message)"
    Write-Output 'WX_MCP_RESULT=failed'
    exit 2
}
