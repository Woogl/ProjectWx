# Copyright Woogle. All Rights Reserved.

function Assert-WxDirectoryWritable {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string]$DirectoryPath)

    $probePath = $null
    try {
        New-Item -ItemType Directory -Path $DirectoryPath -Force -ErrorAction Stop | Out-Null
        $probePath = Join-Path $DirectoryPath ('.wx-write-probe-' + [Guid]::NewGuid().ToString('N') + '.tmp')
        [IO.File]::WriteAllText($probePath, 'probe')
    } catch {
        throw "Directory is not writable: $DirectoryPath ($($_.Exception.Message))"
    } finally {
        if ($probePath -and (Test-Path -LiteralPath $probePath -PathType Leaf)) {
            Remove-Item -LiteralPath $probePath -Force -ErrorAction Stop
        }
    }
}

function Get-WxEditorBuildContext {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [switch]$RequireEditor,
        [switch]$CheckWriteAccess
    )

    $root = (Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop).Path
    $projects = @(Get-ChildItem -LiteralPath $root -Filter '*.uproject' -File -ErrorAction Stop)
    if ($projects.Count -ne 1) { throw "Expected exactly one .uproject in $root (found $($projects.Count))." }
    $engineRoot = & (Join-Path $root 'BatchFiles\Get-WxEngineRoot.ps1') -ProjectFile $projects[0].FullName
    $buildBatch = Join-Path $engineRoot 'Engine\Build\BatchFiles\Build.bat'
    $editorExe = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
    if (-not (Test-Path -LiteralPath $buildBatch -PathType Leaf)) { throw "Build.bat not found: $buildBatch" }
    if ($RequireEditor -and -not (Test-Path -LiteralPath $editorExe -PathType Leaf)) { throw "Editor not found: $editorExe" }
    if (-not $env:LOCALAPPDATA) { throw 'LOCALAPPDATA is not set.' }
    $logDirectory = Join-Path $root 'Saved\Logs\BuildDoctor'
    $ubtDirectory = Join-Path $env:LOCALAPPDATA 'UnrealBuildTool'
    if ($CheckWriteAccess) {
        Assert-WxDirectoryWritable -DirectoryPath $logDirectory
        Assert-WxDirectoryWritable -DirectoryPath $ubtDirectory
    }

    [PSCustomObject]@{
        ProjectRoot = $root
        ProjectFile = $projects[0]
        EditorTarget = "$($projects[0].BaseName)Editor"
        EngineRoot = $engineRoot
        BuildBatchFile = $buildBatch
        EditorExe = $editorExe
        LogDirectory = $logDirectory
        UnrealBuildToolDataDirectory = $ubtDirectory
    }
}
