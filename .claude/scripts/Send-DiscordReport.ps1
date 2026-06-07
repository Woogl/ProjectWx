# Copyright Woogle. All Rights Reserved.
# Sends a Claude work report to a Discord channel via incoming webhook.
# Webhook URL is read from .claude/discord-webhook.local.json (git-ignored).
# Pass the message inline with -Message, or from a UTF-8 file with -Path
# (prefer -Path for Korean text to avoid console-encoding issues).
[CmdletBinding()]
param(
    [string]$Message,
    [string]$Path,
    [string]$Username
)

$ErrorActionPreference = 'Stop'

if ($PSBoundParameters.ContainsKey('Path') -and $Path) {
    if (-not (Test-Path $Path)) {
        Write-Error "Message file not found: $Path"
        exit 1
    }
    $Message = Get-Content -Path $Path -Raw -Encoding UTF8
}

if ([string]::IsNullOrWhiteSpace($Message)) {
    Write-Error "Message is empty. Provide -Message or -Path."
    exit 1
}

$secretPath = Join-Path $PSScriptRoot '..\discord-webhook.local.json'
if (-not (Test-Path $secretPath)) {
    Write-Error "Webhook config not found: $secretPath"
    exit 1
}

$config = Get-Content -Path $secretPath -Raw -Encoding UTF8 | ConvertFrom-Json
$webhookUrl = $config.url
if ([string]::IsNullOrWhiteSpace($webhookUrl)) {
    Write-Error "Webhook url is empty: $secretPath"
    exit 1
}
if ([string]::IsNullOrWhiteSpace($Username)) {
    if ([string]::IsNullOrWhiteSpace($config.username)) {
        $Username = 'Claude Code'
    }
    else {
        $Username = $config.username
    }
}

# Discord caps each message at 2000 chars -> split into 1900-char chunks on line breaks.
$limit = 1900
$chunks = New-Object System.Collections.Generic.List[string]
$current = ''
foreach ($rawLine in ($Message -split "`n")) {
    $line = $rawLine.TrimEnd("`r")
    if (($current.Length + $line.Length + 1) -gt $limit) {
        if ($current.Length -gt 0) {
            $chunks.Add($current)
            $current = ''
        }
        while ($line.Length -gt $limit) {
            $chunks.Add($line.Substring(0, $limit))
            $line = $line.Substring($limit)
        }
    }
    if ($current.Length -gt 0) {
        $current = $current + "`n" + $line
    }
    else {
        $current = $line
    }
}
if ($current.Length -gt 0) {
    $chunks.Add($current)
}

foreach ($chunk in $chunks) {
    $payload = @{ content = $chunk; username = $Username } | ConvertTo-Json -Compress
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
    Invoke-RestMethod -Uri $webhookUrl -Method Post -ContentType 'application/json; charset=utf-8' -Body $bytes | Out-Null
}

Write-Host ("Sent to Discord ({0} message(s))." -f $chunks.Count)
