# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$ProjectRoot)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$buildScripts = Join-Path $PSScriptRoot '../../build-doctor/scripts'

try {
    . (Join-Path $buildScripts 'Get-WxEditorBuildContext.ps1')
    . (Join-Path $PSScriptRoot 'Get-WxProjectProcess.ps1')
    . (Join-Path $PSScriptRoot 'Stop-WxProjectProcesses.ps1')

    # Check engine paths and UBT write access before closing a working editor.
    $context = Get-WxEditorBuildContext -ProjectRoot $ProjectRoot -RequireEditor -CheckWriteAccess
    $stopped = @(Stop-WxProjectProcesses -ProjectFile $context.ProjectFile.FullName)
    Write-Output "RUN_EDITOR_STOPPED_PIDS=$($stopped -join ',')"

    & (Join-Path $buildScripts 'Invoke-WxEditorBuild.ps1') -ProjectRoot $context.ProjectRoot
    $buildExit = $LASTEXITCODE
    if ($buildExit -ne 0) {
        Write-Output 'RUN_EDITOR_RESULT=build-failure'
        exit $buildExit
    }

    $editor = Start-Process -FilePath $context.EditorExe -ArgumentList ('"' + $context.ProjectFile.FullName + '"') -PassThru -ErrorAction Stop
    Write-Output "RUN_EDITOR_STARTED_PID=$($editor.Id)"
    Write-Output 'RUN_EDITOR_RESULT=started'
    exit 0
} catch {
    [Console]::Error.WriteLine("RUN_EDITOR_ERROR=$($_.Exception.Message)")
    Write-Output 'RUN_EDITOR_RESULT=failed'
    exit 2
}
