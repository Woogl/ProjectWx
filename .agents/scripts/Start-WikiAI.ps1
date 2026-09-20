# Copyright Woogle. All Rights Reserved.
#requires -Version 7.0
$ErrorActionPreference = 'Stop'
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$identity = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes($repo.ToLowerInvariant()))).ToLowerInvariant()
$node = (Get-Command node -ErrorAction Stop).Source
$script = Join-Path $PSScriptRoot 'Wiki-AI.cjs'
$revision = (@($script, (Join-Path $PSScriptRoot 'wiki-checklist.schema.json'), (Join-Path $PSScriptRoot 'wiki-viewer/workflow-model.js'), (Join-Path $PSScriptRoot 'Wiki-AI-Providers.cjs'), (Join-Path $PSScriptRoot 'wiki-gemini-policy.toml'), (Join-Path $PSScriptRoot 'wiki-gemini-settings.json'), $PSCommandPath, (Join-Path $PSScriptRoot 'Wiki-Import.cjs'), (Join-Path $PSScriptRoot 'Wiki-Import.py'), (Join-Path $PSScriptRoot 'Wiki-Tasks.cjs'), (Join-Path $PSScriptRoot 'Workflow-Execution.cjs')) | ForEach-Object { (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant() }) -join ':'
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
$output = Join-Path $repo 'Saved/Wiki'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$configPath = Join-Path $output 'ai-providers.json'
$configJson = ConvertTo-Json -InputObject $providers -Depth 5 -Compress
# Restart when CLI discovery changes, even if the server code has not changed.
$configUnchanged = (Test-Path -LiteralPath $configPath) -and ([IO.File]::ReadAllText($configPath) -eq $configJson)
function Get-WikiAI {
    try { Invoke-RestMethod 'http://127.0.0.1:18743/health' -TimeoutSec 1 } catch { $null }
}
$health = Get-WikiAI
if ($health) {
    if ($health.identity -ne $identity) { throw 'Port 18743 belongs to another Wiki.' }
    $connection = Join-Path $repo 'Saved/Wiki/ai-connection.json'
    if ($configUnchanged -and $health.revision -eq $revision -and (Test-Path -LiteralPath $connection)) { exit 0 }
    $processes = @(Get-CimInstance Win32_Process)
    $owned = @($processes | Where-Object { $_.Name -eq 'node.exe' -and $_.ExecutablePath -eq $node -and $_.CommandLine -and $_.CommandLine.Contains('"' + $script + '"') })
    if ($owned.Count -ne 1 -or $health.busy -or @($processes | Where-Object { $_.ParentProcessId -in $owned.ProcessId -and $_.Name -ne 'conhost.exe' }).Count -gt 0) {
        throw 'Wiki AI update requires an idle, identifiable server. Finish the running analysis and retry.'
    }
    Stop-Process -Id $owned[0].ProcessId -ErrorAction Stop
    Wait-Process -Id $owned[0].ProcessId -Timeout 10 -ErrorAction SilentlyContinue
}
$output = Join-Path $repo 'Saved/Wiki'
New-Item -ItemType Directory -Path $output -Force | Out-Null
$script = Join-Path $PSScriptRoot 'Wiki-AI.cjs'
[IO.File]::WriteAllText($configPath, $configJson, [Text.UTF8Encoding]::new($false))
$arguments = '"{0}" "{1}"' -f $script, $configPath
Start-Process -FilePath $node -ArgumentList $arguments -WindowStyle Hidden -WorkingDirectory $repo -RedirectStandardOutput (Join-Path $output 'ai.log') -RedirectStandardError (Join-Path $output 'ai-error.log') | Out-Null
for ($attempt = 0; $attempt -lt 20; $attempt++) {
    Start-Sleep -Milliseconds 250
    $health = Get-WikiAI
    if ($health -and $health.identity -eq $identity) { exit 0 }
}
throw 'Wiki AI did not start. Check Saved/Wiki/ai-error.log.'
