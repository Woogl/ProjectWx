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

. (Join-Path $PSScriptRoot 'Get-WxEditorBuildContext.ps1')

$LogEncoding = New-Object System.Text.UTF8Encoding($false)
$LogPath = $null
$BuildInvoked = $false

try
{
	$Context = Get-WxEditorBuildContext -ProjectRoot $ProjectRoot
	$ProjectFile = $Context.ProjectFile
	$EditorTarget = $Context.EditorTarget
	$EngineRoot = $Context.EngineRoot
	$BuildBatchFile = $Context.BuildBatchFile
	$LogDirectory = $Context.LogDirectory
	$UnrealBuildToolDataDirectory = $Context.UnrealBuildToolDataDirectory
	Assert-WxDirectoryWritable -DirectoryPath $LogDirectory
	$LogName = 'build_{0}_{1}.log' -f (Get-Date -Format 'yyyy-MM-dd_HHmmss_fff'), $PID
	$LogPath = Join-Path $LogDirectory $LogName

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
	[IO.File]::WriteAllLines($LogPath, [string[]]$HeaderLines, $LogEncoding)

	$HeaderLines | ForEach-Object { Write-Output $_ }
	Write-Output "BUILD_DOCTOR_LOG=$LogPath"
	Assert-WxDirectoryWritable -DirectoryPath $UnrealBuildToolDataDirectory

	# Windows PowerShell 5.1 emits redirected native stderr as ErrorRecord objects.
	# Capture those as text; the native exit code determines build success.
	$BuildInvoked = $true
	$PreviousErrorActionPreference = $ErrorActionPreference
	try
	{
		$ErrorActionPreference = 'Continue'
		& $BuildBatchFile $EditorTarget Win64 Development "-Project=$($ProjectFile.FullName)" -WaitMutex -NoHotReloadFromIDE 2>&1 |
			ForEach-Object {
				$Line = [string]$_
				try { [IO.File]::AppendAllText($LogPath, ($Line + [Environment]::NewLine), $LogEncoding) }
				catch { throw }
				Write-Output $Line
			}
		$BuildExitCode = $LASTEXITCODE
	}
	finally
	{
		$ErrorActionPreference = $PreviousErrorActionPreference
	}

	$ResultLine = if ($BuildExitCode -eq 0)
	{
		'BUILD_DOCTOR_RESULT=success'
	}
	else
	{
		'BUILD_DOCTOR_RESULT=build-failure'
	}
	$ExitLine = "BUILD_DOCTOR_EXIT_CODE=$BuildExitCode"
	@($ResultLine, $ExitLine) | ForEach-Object { [IO.File]::AppendAllText($LogPath, ([string]$_ + [Environment]::NewLine), $LogEncoding); Write-Output $_ }
	exit $BuildExitCode
}
catch
{
	$ErrorMessage = $_.Exception.Message
	$ErrorLine = "BUILD_DOCTOR_ERROR=$ErrorMessage"
	$ResultLine = if ($BuildInvoked) { 'BUILD_DOCTOR_RESULT=build-failure' } else { 'BUILD_DOCTOR_RESULT=preflight-failure' }
	$ExitLine = 'BUILD_DOCTOR_EXIT_CODE=2'
	Write-BuildDoctorError -Message $ErrorMessage
	if ($LogPath -and (Test-Path -LiteralPath $LogPath -PathType Leaf -ErrorAction SilentlyContinue))
	{
		try { [IO.File]::AppendAllText($LogPath, ((@($ErrorLine, $ResultLine, $ExitLine) -join [Environment]::NewLine) + [Environment]::NewLine), $LogEncoding) } catch {}
	}
	Write-Output $ResultLine
	Write-Output $ExitLine
	exit 2
}
