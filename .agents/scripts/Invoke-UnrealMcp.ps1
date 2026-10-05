# Copyright Woogle. All Rights Reserved.
# Calls a tool on a running Unreal Editor's MCP server (unreal-mcp) over JSON-RPC.
# Use it when the session's own MCP tools are not attached: the editor started mid-session or listens on another port.
#   -Tool list_toolsets
#   -Tool describe_toolset -Arguments '{"toolset_name":"WxToolset.WxPackageToolset"}'
#   -Toolset WxToolset.WxPackageToolset -Tool SavePackages -Arguments '{"objects":[{"refPath":"/Game/X.X"}]}'
# Pass -ArgumentsFile instead of -Arguments when calling through another shell, which mangles the quotes.
# Exit codes: 0 = success, 1 = the tool or JSON-RPC call reported an error, 2 = connection or protocol error.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Tool,
    [string]$Toolset,
    [string]$Arguments = '{}',
    [string]$ArgumentsFile,
    [int]$Port = 8000,
    [string]$OutFile,
    [int]$TimeoutSeconds = 300
)

# Callers' settings leak into this scope: 'Stop' turns curl's stderr into a terminating error and strict mode rejects absent JSON fields.
$ErrorActionPreference = 'Continue'
Set-StrictMode -Off
$Utf8 = New-Object Text.UTF8Encoding($false)
try { [Console]::OutputEncoding = $Utf8 } catch { }

$Url = "http://127.0.0.1:$Port/mcp"
$SessionFile = Join-Path $env:TEMP "wx-unreal-mcp-$Port.session"
$WorkDirectory = Join-Path $env:TEMP "wx-unreal-mcp-$PID"
New-Item -ItemType Directory -Force -Path $WorkDirectory | Out-Null

function Send-McpRequest([string]$Body, [string]$SessionId) {
    $requestFile = Join-Path $WorkDirectory 'request.json'
    $headerFile = Join-Path $WorkDirectory 'headers.txt'
    $responseFile = Join-Path $WorkDirectory 'response.txt'
    [IO.File]::WriteAllText($requestFile, $Body, $Utf8)
    Remove-Item -LiteralPath $headerFile, $responseFile -ErrorAction SilentlyContinue

    $curlArguments = @('-s', '-S', '--max-time', $TimeoutSeconds, '-X', 'POST', $Url,
        '-H', 'Content-Type: application/json', '-H', 'Accept: application/json, text/event-stream',
        '-D', $headerFile, '-o', $responseFile, '-w', '%{http_code}', '--data-binary', "@$requestFile")
    if ($SessionId) { $curlArguments += @('-H', "Mcp-Session-Id: $SessionId") }

    # PowerShell's own web cmdlets never return on large event-stream responses, so curl writes to files.
    $status = & curl.exe @curlArguments 2>$null
    if ($LASTEXITCODE -ne 0) { return $null }

    $headers = if (Test-Path -LiteralPath $headerFile) { [IO.File]::ReadAllLines($headerFile) } else { @() }
    $sessionLine = $headers | Where-Object { $_ -match '^Mcp-Session-Id:\s*(\S+)' } | Select-Object -First 1
    [PSCustomObject]@{
        Status = [int]$status
        Body = if (Test-Path -LiteralPath $responseFile) { [IO.File]::ReadAllText($responseFile, $Utf8) } else { '' }
        SessionId = if ($sessionLine -match '^Mcp-Session-Id:\s*(\S+)') { $Matches[1] } else { $null }
    }
}

function New-McpSession {
    $init = '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"wx-invoke-unreal-mcp","version":"1"}}}'
    $response = Send-McpRequest $init $null
    if (-not $response -or $response.Status -ne 200 -or -not $response.SessionId) { return $null }
    Send-McpRequest '{"jsonrpc":"2.0","method":"notifications/initialized"}' $response.SessionId | Out-Null
    Set-Content -LiteralPath $SessionFile -Value $response.SessionId
    return $response.SessionId
}

function ConvertTo-JsonString([string]$Value) {
    return '"' + $Value.Replace('\', '\\').Replace('"', '\"') + '"'
}

try {
    if ($ArgumentsFile) { $Arguments = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $ArgumentsFile).Path, $Utf8) }
    try { $null = $Arguments | ConvertFrom-Json -ErrorAction Stop }
    catch { Write-Output "WX_MCP_ERROR=Arguments is not valid JSON: $Arguments"; exit 2 }

    # The arguments are embedded verbatim because a ConvertFrom-Json/ConvertTo-Json round trip reshapes arrays and numbers.
    if ($Toolset) {
        $callArguments = '{"toolset_name":' + (ConvertTo-JsonString $Toolset) + ',"tool_name":' + (ConvertTo-JsonString $Tool) + ',"arguments":' + $Arguments + '}'
        $call = '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"call_tool","arguments":' + $callArguments + '}}'
    } else {
        $call = '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":' + (ConvertTo-JsonString $Tool) + ',"arguments":' + $Arguments + '}}'
    }

    $sessionId = if (Test-Path -LiteralPath $SessionFile) { (Get-Content -LiteralPath $SessionFile -Raw).Trim() } else { $null }
    if (-not $sessionId) { $sessionId = New-McpSession }
    $response = if ($sessionId) { Send-McpRequest $call $sessionId } else { $null }

    # A restarted editor answers 404 for the cached session id; start a new session once.
    if ($response -and ($response.Status -eq 404 -or $response.Status -eq 400)) {
        $sessionId = New-McpSession
        $response = if ($sessionId) { Send-McpRequest $call $sessionId } else { $null }
    }
    if (-not $response) {
        Write-Output "WX_MCP_ERROR=No MCP server answered at $Url. Start one with .agents/scripts/Start-WxEditorMcp.ps1."
        exit 2
    }

    $payload = $response.Body
    $dataLines = @($payload -split "`n" | Where-Object { $_.StartsWith('data:') } | ForEach-Object { $_.Substring(5).Trim() })
    if ($dataLines.Count -gt 0) { $payload = $dataLines[-1] }
    try { $message = $payload | ConvertFrom-Json -ErrorAction Stop }
    catch { Write-Output "WX_MCP_ERROR=HTTP $($response.Status), unreadable response: $payload"; exit 2 }

    if ($message.error) {
        Write-Output "WX_MCP_ERROR=$($message.error.message)"
        exit 1
    }

    $text = (@($message.result.content) | Where-Object { $_.type -eq 'text' } | ForEach-Object { $_.text }) -join "`n"
    if ($OutFile) {
        $outPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutFile)
        [IO.File]::WriteAllText($outPath, $text, $Utf8)
        Write-Output "WX_MCP_OUTFILE=$outPath ($($text.Length) chars)"
    } else {
        Write-Output $text
    }
    if ($message.result.isError) { exit 1 }
    exit 0
} finally {
    Remove-Item -LiteralPath $WorkDirectory -Recurse -Force -ErrorAction SilentlyContinue
}
