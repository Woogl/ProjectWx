# Copyright Woogle. All Rights Reserved.
#requires -Version 7.1
$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$identity = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($repo.ToLowerInvariant()))).ToLowerInvariant()
$node = (Get-Command node -ErrorAction Stop).Source
$script = Join-Path $PSScriptRoot 'Workflow-Server.cjs'
$appBin = Join-Path $env:LOCALAPPDATA 'OpenAI/Codex/bin'
$executable = Get-Process codex -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($appBin, [StringComparison]::OrdinalIgnoreCase) } | Select-Object -First 1 -ExpandProperty Path
if (!$executable -and (Test-Path -LiteralPath $appBin)) {
    $executable = Get-ChildItem -LiteralPath $appBin -Recurse -Filter codex.exe -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
}
if (!$executable) {
    $codex = Get-Command codex -ErrorAction SilentlyContinue
    if ($codex -and $codex.Source.EndsWith('.exe')) { $executable = $codex.Source }
    elseif ($codex) {
        $package = Join-Path (Split-Path $codex.Source -Parent) 'node_modules/@openai/codex'
        $executable = Get-ChildItem -LiteralPath $package -Recurse -Filter codex.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
    }
}


$providers = [ordered]@{}
if ($executable) { $providers.codex = [ordered]@{ file = $executable; args = @() } }
foreach ($entry in @(@('claude', '@anthropic-ai/claude-code'), @('gemini', '@google/gemini-cli'))) {
    $command = Get-Command $entry[0] -ErrorAction SilentlyContinue
    if (!$command) { continue }
    if ($command.Source.EndsWith('.exe')) { $providers[$entry[0]] = [ordered]@{ file = $command.Source; args = @() }; continue }
    # npm wrappers are resolved to JavaScript instead of invoking a shell with AI input.
    $package = Join-Path (Split-Path $command.Source -Parent) ('node_modules/' + $entry[1])
    $manifest = Join-Path $package 'package.json'
    if (Test-Path -LiteralPath $manifest) {
        $metadata = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        $bin = if ($metadata.bin -is [string]) { $metadata.bin } else { $metadata.bin.($entry[0]) }
        if ($bin -and (Test-Path -LiteralPath (Join-Path $package $bin))) {
            $providers[$entry[0]] = [ordered]@{ file = $node; args = @((Join-Path $package $bin)) }
        }
    }
}
$output = Join-Path $repo 'Saved/Workflow'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$configPath = Join-Path $output 'ai-providers.json'
$connection = Join-Path $output 'ai-connection.json'
function Get-WorkflowServer([int]$TimeoutSec = 1) {
    try { Invoke-RestMethod 'http://127.0.0.1:18743/health' -TimeoutSec $TimeoutSec } catch { $null }
}
# An idle server is always restarted so new code and CLI discovery take effect; a busy one is kept so a running AI is not cut off.
$health = Get-WorkflowServer
# A slow server is not a missing one: while the port is in use, wait longer instead of starting a second server or dropping the connection file.
if (!$health -and (Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 18743 -State Listen -ErrorAction SilentlyContinue)) {
    $health = Get-WorkflowServer 15
    if (!$health) { throw 'Port 18743 is in use but did not answer. Wait for the running AI to finish or close the program using the port, then rerun OpenWorkflow.bat.' }
}
if ($health) {
    if ($health.identity -ne $identity) { throw "Port 18743 belongs to another repository's Workflow server." }
    if ($health.busy) { Write-Warning 'AI is processing; keeping the running server. Rerun OpenWorkflow.bat after it finishes to load new code.'; exit 0 }
    # The matching identity proves the listener is this repository's server under any script name, so stop the process that owns the port.
    $owner = @(Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 18743 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess -Unique)
    if ($owner.Count -ne 1) { throw 'Could not identify the Workflow server process on port 18743.' }
    Stop-Process -Id $owner[0] -ErrorAction Stop
    Wait-Process -Id $owner[0] -Timeout 10 -ErrorAction SilentlyContinue
}
# Remove the old connection first so a failed start leaves the page without a stale token; the server writes a new one when it listens.
Remove-Item -LiteralPath $connection -ErrorAction SilentlyContinue
[IO.File]::WriteAllText($configPath, (ConvertTo-Json -InputObject $providers -Depth 5 -Compress), [Text.UTF8Encoding]::new($false))
$arguments = '"{0}" "{1}"' -f $script, $configPath
Start-Process -FilePath $node -ArgumentList $arguments -WindowStyle Hidden -WorkingDirectory $repo -RedirectStandardOutput (Join-Path $output 'ai.log') -RedirectStandardError (Join-Path $output 'ai-error.log') | Out-Null
for ($attempt = 0; $attempt -lt 20; $attempt++) {
    Start-Sleep -Milliseconds 250
    $health = Get-WorkflowServer
    if ($health -and $health.identity -eq $identity -and (Test-Path -LiteralPath $connection)) { exit 0 }
}
throw 'The Workflow server did not start. Check Saved/Workflow/ai-error.log.'
