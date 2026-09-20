# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param([string]$RepoRoot)
$ErrorActionPreference = 'Stop'
if (!$RepoRoot) { $RepoRoot = Split-Path $PSScriptRoot -Parent }
$repo = [IO.Path]::GetFullPath($RepoRoot)
. (Join-Path $repo '.agents/skills/run-editor/scripts/Get-WxProjectProcess.ps1')

function Assert-WxEqual($Actual, $Expected, [string]$Label) {
    if ([string]$Actual -cne [string]$Expected) { throw "$Label : expected=$Expected actual=$Actual" }
    Write-Output "PASS $Label"
}

$project = 'C:\Project With Spaces\Wx.uproject'
$cases = @(
    @{ Name='UnrealEditor.exe'; CommandLine='UnrealEditor.exe "C:\Project With Spaces\Wx.uproject"'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=1 },
    @{ Name='UnrealEditor.exe'; CommandLine='UnrealEditor.exe "D:\Other\Wx.uproject"'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=2 },
    @{ Name='UnrealEditor.exe'; CommandLine='UnrealEditor.exe Wx.uproject'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=3 },
    @{ Name='UnrealEditor-Win64-DebugGame.exe'; CommandLine='UnrealEditor.exe -project="c:/Project With Spaces/../Project With Spaces/Wx.uproject"'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=4 },
    @{ Name='Wx.exe'; CommandLine='Wx.exe'; ExecutablePath='C:\Project With Spaces\Binaries\Win64\Wx.exe'; ProcessId=5 },
    @{ Name='Wx.exe'; CommandLine='Wx.exe'; ExecutablePath='D:\Other\Binaries\Win64\Wx.exe'; ProcessId=6 },
    @{ Name='Wx.exe'; CommandLine='Wx.exe'; ExecutablePath=$null; ProcessId=7 },
    @{ Name='UnrealEditor.exe'; CommandLine='UnrealEditor.exe -log="C:\Project With Spaces\Wx.uproject"'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=8 },
    @{ Name='UnrealEditor.exe'; CommandLine='UnrealEditor.exe "C:\Project With Spaces\Wx.uproject" "D:\Other\Wx.uproject"'; ExecutablePath='C:\UE\UnrealEditor.exe'; ProcessId=9 }
)
$selected = @(Get-WxProjectProcess -ProjectFile $project -Processes @($cases | ForEach-Object { [pscustomobject]$_ }))
Assert-WxEqual (($selected.ProcessId | Sort-Object) -join ',') '1,4,5' 'process ownership, normalization, ambiguous and missing data'
Assert-WxEqual @(Get-WxProjectProcess -ProjectFile $project -Processes @()).Count 0 'empty process list'

# Only the launcher metadata and a fake Build.bat are used; no engine is started.
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('wx-harness-' + [guid]::NewGuid().ToString('N'))
$originalProgramData = $env:ProgramData
$originalLocalAppData = $env:LOCALAPPDATA
$originalModulePath = $env:PSModulePath
$utf8 = New-Object System.Text.UTF8Encoding($false)
try {
    $fixtureProject = Join-Path $fixture 'Project With Spaces'
    $engine = Join-Path $fixture 'Engine Root'
    $build = Join-Path $engine 'Engine/Build/BatchFiles/Build.bat'
    $env:ProgramData = Join-Path $fixture 'ProgramData'
    $env:LOCALAPPDATA = Join-Path $fixture 'LocalAppData'
    $launcher = Join-Path $env:ProgramData 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
    foreach ($dir in @($fixtureProject, (Split-Path $build -Parent), (Split-Path $launcher -Parent))) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
    [IO.File]::WriteAllText((Join-Path $fixtureProject 'Wx.uproject'), '{}', $utf8)
    [IO.File]::WriteAllText($launcher, (@{InstallationList=@(@{AppName='UE_5.8';InstallLocation=$engine})} | ConvertTo-Json -Depth 4), $utf8)
    $doctor = Join-Path $repo '.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1'
    $hostExecutable = (Get-Process -Id $PID).Path
    if ($PSVersionTable.PSVersion.Major -le 5) {
        $env:PSModulePath = Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/Modules'
    }
    foreach ($stderr in @($false, $true)) {
        foreach ($result in @(0, 7)) {
            $stderrCommand = if ($stderr) { "echo MOCK_BUILD_STDERR 1>&2`r`n" } else { '' }
            [IO.File]::WriteAllText($build, "@echo off`r`necho MOCK_BUILD_OUTPUT`r`n${stderrCommand}echo MOCK_BUILD_AFTER_STDERR`r`nexit /b $result`r`n", $utf8)
            $output = @(& $hostExecutable -NoProfile -ExecutionPolicy Bypass -File $doctor -ProjectRoot $fixtureProject)
            Assert-WxEqual $LASTEXITCODE $result "build exit code $result"
            $expected = if ($result -eq 0) { 'success' } else { 'build-failure' }
            Assert-WxEqual ($output -contains "BUILD_DOCTOR_RESULT=$expected") $true "build classification $expected"
            $log = ($output | Where-Object { $_ -like 'BUILD_DOCTOR_LOG=*' } | Select-Object -First 1).Substring(17)
            $bytes = [IO.File]::ReadAllBytes($log)
            Assert-WxEqual ($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) $false 'no UTF-8 BOM'
            $decoded = (New-Object System.Text.UTF8Encoding($false, $true)).GetString($bytes)
            Assert-WxEqual ($decoded.Contains('MOCK_BUILD_OUTPUT') -and $decoded.Contains("BUILD_DOCTOR_EXIT_CODE=$result") -and !$decoded.Contains([char]0)) $true 'consistent UTF-8 header, output and footer'
            Assert-WxEqual ($output -contains 'MOCK_BUILD_AFTER_STDERR') $true 'output collection reaches end'
            if ($stderr) {
                Assert-WxEqual (($output -join "`n").Contains('MOCK_BUILD_STDERR')) $true 'stderr forwarded as text'
                Assert-WxEqual $decoded.Contains('MOCK_BUILD_STDERR') $true 'stderr preserved in log'
            }
        }
    }
    $output = @(& $hostExecutable -NoProfile -ExecutionPolicy Bypass -File $doctor -ProjectRoot $fixture)
    Assert-WxEqual $LASTEXITCODE 2 'preflight exit code'
    Assert-WxEqual ($output -contains 'BUILD_DOCTOR_RESULT=preflight-failure') $true 'preflight classification'
} finally {
    $env:ProgramData = $originalProgramData
    $env:LOCALAPPDATA = $originalLocalAppData
    $env:PSModulePath = $originalModulePath
    $resolved = [IO.Path]::GetFullPath($fixture)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')
    if ((Split-Path $resolved -Parent) -ne $tempRoot -or (Split-Path $resolved -Leaf) -notlike 'wx-harness-*') {
        throw "Unexpected fixture cleanup path: $resolved"
    }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
