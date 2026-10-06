# Copyright Woogle. All Rights Reserved.
# LLM Wiki(Wiki/)를 기계적으로 점검해 항목별 목록을 낸다. 아무 파일도 고치지 않는다.
# Windows PowerShell 5.1이 한글 리터럴을 읽도록 UTF-8 BOM으로 저장한다.
[CmdletBinding()]
param([string]$RepoRoot)

$ErrorActionPreference = 'Stop'
# 5.1은 param 기본값을 계산할 때 $PSScriptRoot가 비어 있어 본문에서 정한다.
if (-not $RepoRoot) { $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../../..')).Path }
$utf8 = New-Object System.Text.UTF8Encoding $false
$wiki = Join-Path $RepoRoot 'Wiki'

# git이 내는 한글 경로를 콘솔 코드 페이지로 깨뜨리지 않게 한다.
$prevEncoding = [Console]::OutputEncoding
try { [Console]::OutputEncoding = $utf8 } catch { }

function Get-Rel($File) { "$($File.Directory.Name)/$($File.Name)" }
function Write-Section([string]$Title, $Items) {
    $list = @($Items | Where-Object { $_ })
    "== $Title ($($list.Count))"
    $list
    ''
}
# 파일·폴더·확장자 없는 에셋 경로를 모두 실행 시작 시점의 HEAD에서 찾는다.
function Test-InHead([string]$Path) {
    $pathInTree = $Path.Replace('\', '/').TrimEnd('/')
    foreach ($file in $headPaths) {
        if ([string]::Equals($file, $pathInTree, [StringComparison]::Ordinal) -or
            $file.StartsWith($pathInTree + '/', [StringComparison]::Ordinal) -or
            $file.StartsWith($pathInTree + '.', [StringComparison]::Ordinal)) { return $true }
    }
    return $false
}
# 커밋을 찾지 못하면 -1을 돌려준다. 0으로 넘기면 낡은 자료가 없는 것처럼 보인다.
function Get-CommitsSince([string]$Hash, [string]$Path) {
    $ErrorActionPreference = 'Continue'
    $lines = @(git log --oneline "$Hash..$headCommit" -- $Path 2>$null)
    if ($LASTEXITCODE -ne 0) { return -1 }
    $lines.Count
}

Push-Location $RepoRoot
try {
    if ((git rev-parse --is-shallow-repository) -eq 'true') { throw '얕은 클론이라 커밋을 비교할 수 없다. git fetch --unshallow 뒤 다시 돌려라.' }

    $headCommit = git rev-parse --verify HEAD
    if ($LASTEXITCODE -ne 0) { throw 'HEAD를 읽을 수 없다.' }
    $headPaths = @(git -c core.quotepath=false ls-tree -r --name-only $headCommit)
    if ($LASTEXITCODE -ne 0) { throw 'HEAD의 파일 목록을 읽을 수 없다.' }

    $docs = @(foreach ($kind in 'summaries', 'entities', 'topics') { Get-ChildItem (Join-Path $wiki $kind) -Filter *.md -ErrorAction SilentlyContinue })
    $summaries = @($docs | Where-Object { $_.Directory.Name -eq 'summaries' })
    $topics = @($docs | Where-Object { $_.Directory.Name -ne 'summaries' })
    $text = @{}
    foreach ($d in $docs) { $text[$d.FullName] = [IO.File]::ReadAllText($d.FullName, $utf8) }

    $staleSources = @(); $goneSources = @()
    foreach ($s in $summaries) {
        $m = [regex]::Match($text[$s.FullName], '(?m)^- 자료: `([^`]+)` \(([0-9a-f]+)\)')
        if (-not $m.Success) { $staleSources += "$(Get-Rel $s) — 자료 줄이 없다"; continue }
        $path = $m.Groups[1].Value
        if (-not (Test-InHead $path)) { $goneSources += "$(Get-Rel $s) — $path"; continue }
        $n = Get-CommitsSince $m.Groups[2].Value $path
        if ($n -lt 0) { $staleSources += "$(Get-Rel $s) — $path ($($m.Groups[2].Value) 커밋을 찾을 수 없다)" }
        elseif ($n -gt 0) { $staleSources += "$(Get-Rel $s) — $path ($($m.Groups[2].Value) 뒤 커밋 $($n)개)" }
    }

    $staleCode = @(); $missingNames = @()
    foreach ($t in $topics) {
        foreach ($m in [regex]::Matches($text[$t.FullName], '(?m)^- `([^`]+)` \(([0-9a-f]{7,})\)')) {
            $path = $m.Groups[1].Value.TrimEnd('/')
            if (-not (Test-InHead $path)) { $missingNames += "$(Get-Rel $t) — 출처 $path"; continue }
            $n = Get-CommitsSince $m.Groups[2].Value $path
            if ($n -lt 0) { $staleCode += "$(Get-Rel $t) — $path ($($m.Groups[2].Value) 커밋을 찾을 수 없다)" }
            elseif ($n -gt 0) { $staleCode += "$(Get-Rel $t) — $path ($($m.Groups[2].Value) 뒤 커밋 $($n)개)" }
        }
        # 구현 절이 인용한 클래스·파일 경로가 HEAD에 아직 있는지 본다.
        $impl = [regex]::Match($text[$t.FullName], '(?ms)^## 구현\s*$(.*?)(?=^## |\z)').Groups[1].Value
        $names = [regex]::Matches($impl, '`([UAFE]?Wx[A-Za-z0-9_]+|(?:Source|Content|Plugins|Config)/[^`]+)`') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique
        foreach ($name in $names) {
            if ($name -match '/') { if (-not (Test-InHead $name.TrimEnd('/'))) { $missingNames += "$(Get-Rel $t) — $name" } }
            else { git grep -q -F $name $headCommit -- Source Plugins; if ($LASTEXITCODE -ne 0) { $missingNames += "$(Get-Rel $t) — $name" } }
        }
    }

    $broken = @(); $inbound = @{}
    foreach ($d in $docs) { $inbound[$d.FullName] = 0 }
    foreach ($d in $docs) {
        foreach ($m in [regex]::Matches($text[$d.FullName], '\]\(([^)\s]+\.md)\)')) {
            $target = [IO.Path]::GetFullPath((Join-Path $d.DirectoryName $m.Groups[1].Value))
            if (-not (Test-Path -LiteralPath $target)) { $broken += "$(Get-Rel $d) -> $($m.Groups[1].Value)" }
            elseif ($inbound.ContainsKey($target) -and $target -ne $d.FullName) { $inbound[$target]++ }
        }
    }
    $orphans = $docs | Where-Object { $inbound[$_.FullName] -eq 0 } | ForEach-Object { Get-Rel $_ }

    $pairs = @()
    foreach ($s in $summaries) {
        $reflected = [regex]::Match($text[$s.FullName], '(?ms)^## 반영한 문서\s*$(.*?)(?=^## |\z)')
        if (-not $reflected.Success) { $pairs += "$(Get-Rel $s) — 반영한 문서 절이 없다"; continue }
        foreach ($m in [regex]::Matches($reflected.Groups[1].Value, '\]\(\.\./([^)]+\.md)\)')) {
            $p = Join-Path $wiki $m.Groups[1].Value
            if ((Test-Path -LiteralPath $p) -and -not $text[(Get-Item -LiteralPath $p).FullName].Contains("](../summaries/$($s.Name))")) {
                $pairs += "$(Get-Rel $s) -> $($m.Groups[1].Value) — 상대 문서의 출처에 이 요약이 없다"
            }
        }
    }
    foreach ($t in $topics) {
        foreach ($m in [regex]::Matches($text[$t.FullName], '\]\(\.\./summaries/([^)]+\.md)\)')) {
            $p = Join-Path $wiki "summaries/$($m.Groups[1].Value)"
            if ((Test-Path -LiteralPath $p) -and -not $text[(Get-Item -LiteralPath $p).FullName].Contains("](../$(Get-Rel $t))")) {
                $pairs += "$(Get-Rel $t) -> summaries/$($m.Groups[1].Value) — 요약의 반영한 문서에 이 문서가 없다"
            }
        }
    }

    # 다른 엔티티·주제 문서의 제목을 본문에 쓰면서 링크하지 않은 곳. 단순 문자열 일치라 후보일 뿐이다.
    $candidates = @()
    foreach ($target in $topics) {
        $title = [regex]::Match($text[$target.FullName], '(?m)^# (.+)$').Groups[1].Value.Trim()
        foreach ($t in $topics | Where-Object { $_.FullName -ne $target.FullName }) {
            $body = $text[$t.FullName]
            if ($body.Contains($title) -and -not $body.Contains("$($target.Name))")) { $candidates += "$(Get-Rel $t) — '$title' 언급, 링크 없음" }
        }
    }

    $index = [IO.File]::ReadAllText((Join-Path $wiki 'index.md'), $utf8)
    $notIndexed = $docs | Where-Object { -not $index.Contains("($(Get-Rel $_))") } | ForEach-Object { Get-Rel $_ }

    $everything = (@($text.Values) + [IO.File]::ReadAllText((Join-Path $wiki 'log.md'), $utf8)) -join "`n"
    $notIngested = $headPaths | Where-Object { $_.StartsWith('Docs/', [StringComparison]::Ordinal) } | Where-Object { $_ -notmatch '\.(png|jpe?g)$' -and -not $everything.Contains($_) }

    Write-Section '낡은 자료' $staleSources
    Write-Section '사라진 자료' $goneSources
    Write-Section '낡은 코드 출처' $staleCode
    Write-Section '없는 코드 이름' $missingNames
    Write-Section '깨진 링크' $broken
    Write-Section '고아 문서' $orphans
    Write-Section '출처·반영 짝' $pairs
    Write-Section '연결 후보' $candidates
    Write-Section '색인 누락' $notIndexed
    Write-Section '미적재 자료' $notIngested
}
finally {
    Pop-Location
    try { [Console]::OutputEncoding = $prevEncoding } catch { }
}
