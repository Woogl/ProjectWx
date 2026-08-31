# Copyright Woogle. All Rights Reserved.

[CmdletBinding()]
param(
	[Parameter(Mandatory = $true)]
	[string]$ProjectRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false

function Write-BuildDoctorError
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$Message
	)

	[Console]::Error.WriteLine("BUILD_DOCTOR_ERROR=$Message")
}

function Assert-DirectoryWritable
{
	param(
		[Parameter(Mandatory = $true)]
		[string]$DirectoryPath
	)

	$ProbePath = $null
	try
	{
		New-Item -ItemType Directory -Path $DirectoryPath -Force | Out-Null
		$ProbeName = '.build-doctor-write-probe-{0}-{1}.tmp' -f $PID, [Guid]::NewGuid().ToString('N')
		$ProbePath = Join-Path $DirectoryPath $ProbeName
		[System.IO.File]::WriteAllText($ProbePath, 'probe')
	}
	catch
	{
		throw "디렉터리에 쓸 수 없습니다: $DirectoryPath ($($_.Exception.Message))"
	}
	finally
	{
		if ($ProbePath -and (Test-Path -LiteralPath $ProbePath -PathType Leaf -ErrorAction SilentlyContinue))
		{
			Remove-Item -LiteralPath $ProbePath -Force -ErrorAction SilentlyContinue
		}
	}
}

$LogPath = $null

try
{
	$ResolvedProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop).Path
	$ProjectFiles = @(Get-ChildItem -LiteralPath $ResolvedProjectRoot -Filter '*.uproject' -File)
	if ($ProjectFiles.Count -ne 1)
	{
		throw "프로젝트 루트에는 .uproject가 정확히 하나 있어야 합니다: $ResolvedProjectRoot (발견: $($ProjectFiles.Count))"
	}

	$ProjectFile = $ProjectFiles[0]
	$EditorTarget = "$($ProjectFile.BaseName)Editor"

	$LauncherDataPath = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
	if (-not (Test-Path -LiteralPath $LauncherDataPath -PathType Leaf))
	{
		throw "Epic Games Launcher 설치 정보를 찾을 수 없습니다: $LauncherDataPath"
	}

	$LauncherData = Get-Content -LiteralPath $LauncherDataPath -Raw | ConvertFrom-Json
	$EngineEntry = $LauncherData.InstallationList |
		Where-Object { $_.AppName -eq 'UE_5.8' } |
		Select-Object -First 1
	if (-not $EngineEntry)
	{
		throw "LauncherInstalled.dat에 UE_5.8 설치 정보가 없습니다: $LauncherDataPath"
	}

	$EngineRoot = $EngineEntry.InstallLocation
	$BuildBatchFile = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
	if (-not (Test-Path -LiteralPath $BuildBatchFile -PathType Leaf))
	{
		throw "UE 5.8 Build.bat을 찾을 수 없습니다: $BuildBatchFile"
	}

	$LogDirectory = Join-Path $ResolvedProjectRoot 'Saved\Logs\BuildDoctor'
	Assert-DirectoryWritable -DirectoryPath $LogDirectory
	$LogName = 'build_{0}_{1}.log' -f (Get-Date -Format 'yyyy-MM-dd_HHmmss_fff'), $PID
	$LogPath = Join-Path $LogDirectory $LogName

	if (-not $env:LOCALAPPDATA)
	{
		throw 'LOCALAPPDATA 환경 변수가 없어 UnrealBuildTool 데이터 경로를 확인할 수 없습니다.'
	}
	$UnrealBuildToolDataDirectory = Join-Path $env:LOCALAPPDATA 'UnrealBuildTool'

	$EditorProcesses = @(Get-Process -Name 'UnrealEditor' -ErrorAction SilentlyContinue)
	$EditorProcessSummary = if ($EditorProcesses.Count -gt 0)
	{
		($EditorProcesses.Id -join ',')
	}
	else
	{
		'none'
	}

	$BuildCommand = '& "{0}" {1} Win64 Development "-Project={2}" -WaitMutex -NoHotReloadFromIDE' -f `
		$BuildBatchFile, $EditorTarget, $ProjectFile.FullName

	$HeaderLines = @(
		"BUILD_DOCTOR_TARGET=$EditorTarget Win64 Development",
		"BUILD_DOCTOR_PROJECT=$($ProjectFile.FullName)",
		"BUILD_DOCTOR_ENGINE=$EngineRoot",
		"BUILD_DOCTOR_UBT_DATA=$UnrealBuildToolDataDirectory",
		"BUILD_DOCTOR_EDITOR_PIDS=$EditorProcessSummary",
		"BUILD_DOCTOR_COMMAND=$BuildCommand"
	)
	$HeaderLines | Set-Content -LiteralPath $LogPath -Encoding utf8NoBOM

	$HeaderLines | ForEach-Object { Write-Output $_ }
	Write-Output "BUILD_DOCTOR_LOG=$LogPath"
	Assert-DirectoryWritable -DirectoryPath $UnrealBuildToolDataDirectory

	& $BuildBatchFile $EditorTarget Win64 Development "-Project=$($ProjectFile.FullName)" -WaitMutex -NoHotReloadFromIDE 2>&1 |
		Tee-Object -FilePath $LogPath -Append
	$BuildExitCode = $LASTEXITCODE

	$ResultLine = if ($BuildExitCode -eq 0)
	{
		'BUILD_DOCTOR_RESULT=success'
	}
	else
	{
		'BUILD_DOCTOR_RESULT=build-failure'
	}
	$ExitLine = "BUILD_DOCTOR_EXIT_CODE=$BuildExitCode"
	@($ResultLine, $ExitLine) | Tee-Object -FilePath $LogPath -Append
	exit $BuildExitCode
}
catch
{
	$ErrorMessage = $_.Exception.Message
	$ErrorLine = "BUILD_DOCTOR_ERROR=$ErrorMessage"
	$ResultLine = 'BUILD_DOCTOR_RESULT=preflight-failure'
	$ExitLine = 'BUILD_DOCTOR_EXIT_CODE=2'
	Write-BuildDoctorError -Message $ErrorMessage
	if ($LogPath -and (Test-Path -LiteralPath $LogPath -PathType Leaf -ErrorAction SilentlyContinue))
	{
		@($ErrorLine, $ResultLine, $ExitLine) | Add-Content -LiteralPath $LogPath -Encoding utf8NoBOM -ErrorAction SilentlyContinue
	}
	Write-Output $ResultLine
	Write-Output $ExitLine
	exit 2
}
