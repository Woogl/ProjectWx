# Copyright Woogle. All Rights Reserved.
# Checks the mechanical coding rules in AGENTS.md: 1 (Wx prefix), 2 (copyright line), 3 (no inline function definitions), 5 (native gameplay tags only).
# -Changed limits the scan to files changed against HEAD, untracked files and commits not yet on the upstream.
# Exit codes: 0 = no violations, 1 = violations found, 2 = processing error.
[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [switch]$Changed
)

$CopyrightLine = '// Copyright Woogle. All Rights Reserved.'
$Utf8 = New-Object Text.UTF8Encoding($false)

# Returns $null on failure and an array (possibly empty) on success; the leading comma keeps an empty result from unrolling to $null.
function Invoke-Git([string[]]$Arguments) {
    $output = & git -C $ProjectRoot -c core.quotepath=false @Arguments 2>$null
    if ($LASTEXITCODE -ne 0) { return $null }
    return ,@($output | Where-Object { $_ })
}

# Blanks out comments and string literals so that code patterns are not matched inside them; line numbers are preserved.
function Remove-CommentsAndStrings([string]$Text) {
    $Text = [regex]::Replace($Text, '(?s)/\*.*?\*/', { param($m) "`n" * ([regex]::Matches($m.Value, "`n").Count) })
    $Text = [regex]::Replace($Text, '//[^\n]*', '')
    $Text = [regex]::Replace($Text, '"(?:\\.|[^"\\\n])*"', '""')
    return $Text
}

function Test-TemplateContext([string[]]$Lines, [int]$Index) {
    $seen = 0
    for ($j = $Index; $j -ge 0 -and $seen -lt 3; $j--) {
        $s = $Lines[$j].Trim()
        if (-not $s) { continue }
        if ($s -cmatch '^template\b') { return $true }
        $seen++
    }
    return $false
}

$violations = New-Object System.Collections.Generic.List[string]
function Add-Violation([string]$Path, [int]$Line, [string]$Rule, [string]$Message) {
    $violations.Add(('{0}:{1}: [{2}] {3}' -f $Path, $Line, $Rule, $Message))
}

try {
    if (-not $ProjectRoot) {
        $ProjectRoot = (& git -C $PSScriptRoot rev-parse --show-toplevel)
        if ($LASTEXITCODE -ne 0 -or -not $ProjectRoot) { throw 'Cannot resolve the repository root.' }
    }
    $ProjectRoot = (Resolve-Path $ProjectRoot).Path

    $scopes = @('Source', 'Plugins', 'Config')
    $tracked = Invoke-Git (@('ls-files', '--') + $scopes)
    $untracked = Invoke-Git (@('ls-files', '--others', '--exclude-standard', '--') + $scopes)
    if ($null -eq $tracked) { throw 'git ls-files failed.' }
    $files = @($tracked) + @($untracked) | Sort-Object -Unique

    if ($Changed) {
        $changedSet = @{}
        $modified = Invoke-Git @('diff', '--name-only', 'HEAD', '--')
        $unpushed = $null
        $upstream = Invoke-Git @('rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}')
        if ($upstream) { $unpushed = Invoke-Git @('diff', '--name-only', '@{upstream}', 'HEAD', '--') }
        foreach ($n in @($modified) + @($untracked) + @($unpushed)) { if ($n) { $changedSet[$n] = $true } }
        $files = @($files | Where-Object { $changedSet.ContainsKey($_) })
    }

    $checked = 0
    foreach ($rel in $files) {
        if ($rel -notmatch '^(Source|Plugins)/.+\.(h|cpp|cs)$') { continue }
        $full = Join-Path $ProjectRoot $rel
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { continue }
        $checked++

        $text = [IO.File]::ReadAllText($full, $Utf8).TrimStart([char]0xFEFF)
        $rawLines = $text -split "`r?`n"

        # Rule 2: copyright line, every source file including general-purpose plugins.
        if ($rawLines[0] -ne $CopyrightLine) {
            Add-Violation $rel 1 'R2' 'first line must be the copyright line'
        }

        # Rule 5: gameplay tags are declared and defined only in WxGameplayTags.h/.cpp.
        if ($rel -notmatch '/WxGameplayTags\.(h|cpp)$') {
            for ($i = 0; $i -lt $rawLines.Count; $i++) {
                if ($rawLines[$i] -cmatch '\bUE_(DECLARE|DEFINE)_GAMEPLAY_TAG') {
                    Add-Violation $rel ($i + 1) 'R5' 'gameplay tag declared outside WxGameplayTags.h/.cpp'
                }
            }
        }

        # Rules 1 and 3 cover project code only; general-purpose plugins (not named Wx*) are excluded.
        $isOwnedHeader = $rel -match '^(Source/|Plugins/Wx[^/]*/).+\.h$'
        if (-not $isOwnedHeader) { continue }
        $lines = (Remove-CommentsAndStrings $text) -split "`r?`n"

        for ($i = 0; $i -lt $lines.Count; $i++) {
            $s = $lines[$i].Trim()
            if (-not $s -or $s.StartsWith('#')) { continue }

            # Rule 1: reflected types carry the Wx prefix.
            if ($s -cmatch '^(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(') {
                for ($j = $i + 1; $j -lt [Math]::Min($i + 4, $lines.Count); $j++) {
                    if ($lines[$j] -cmatch '^\s*(class|struct|enum\s+class|enum)\s+(?:\w+_API\s+)?(\w+)') {
                        if ($Matches[2] -cnotmatch '^[AUFEIST]Wx') {
                            Add-Violation $rel ($j + 1) 'R1' ("type '{0}' lacks the Wx prefix" -f $Matches[2])
                        }
                        break
                    }
                }
                continue
            }

            # Rule 3: no inline function definitions in headers, except templates and StateTree GetInstanceDataType().
            if ($s -cmatch 'GetInstanceDataType' -or (Test-TemplateContext $lines $i)) { continue }
            if ($s -cmatch '^(if|for|while|switch|else|return|do|case)\b' -or $s -cmatch '^(UE_|DECLARE_|GENERATED_|ATTRIBUTE_ACCESSORS|UFUNCTION|UPROPERTY|UCLASS|USTRUCT|UENUM|UINTERFACE)') { continue }
            if ($s -cmatch '\]\s*\(') { continue }

            $isInline = $false
            if ($s -cmatch '\bFORCEINLINE\b') {
                $isInline = $true
            }
            elseif ($s -cmatch '\binline\b' -and $s.Contains('(') -and -not $s.Split('(')[0].Contains('=')) {
                $isInline = $true
            }
            elseif ($s -cmatch '\)\s*(const\s*)?((override|final|noexcept)\s*)*(\{|:\s*\w+\s*[({])') {
                $isInline = $true
            }
            elseif ($s -cmatch '\)\s*(const\s*)?((override|final|noexcept)\s*)*$') {
                for ($j = $i + 1; $j -lt $lines.Count; $j++) {
                    $next = $lines[$j].Trim()
                    if (-not $next) { continue }
                    if ($next.StartsWith('{')) { $isInline = $true }
                    break
                }
            }
            if ($isInline) {
                $snippet = $s
                if ($snippet.Length -gt 80) { $snippet = $snippet.Substring(0, 80) }
                Add-Violation $rel ($i + 1) 'R3' ('inline function definition: ' + $snippet)
            }
        }
    }

    # Rule 5: no gameplay tag ini files.
    foreach ($ini in @('Config/DefaultGameplayTags.ini')) {
        if (Test-Path -LiteralPath (Join-Path $ProjectRoot $ini)) { Add-Violation $ini 1 'R5' 'gameplay tag ini must not exist' }
    }
    $tagDir = Join-Path $ProjectRoot 'Config/Tags'
    if (Test-Path -LiteralPath $tagDir) {
        foreach ($f in Get-ChildItem -LiteralPath $tagDir -Filter '*.ini') { Add-Violation ('Config/Tags/' + $f.Name) 1 'R5' 'gameplay tag ini must not exist' }
    }

    foreach ($v in $violations) { Write-Output $v }
    if ($violations.Count -gt 0) {
        Write-Output ('{0} violation(s) in {1} checked file(s).' -f $violations.Count, $checked)
        exit 1
    }
    Write-Output ('OK: {0} file(s) checked.' -f $checked)
    exit 0
}
catch {
    Write-Error $_
    exit 2
}
