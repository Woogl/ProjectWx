@rem Copyright Woogle. All Rights Reserved.
@echo off
setlocal
set "WxRedirScript=%~f0"
set "WxRedirRoot=%~dp0.."
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$script = [IO.File]::ReadAllText($env:WxRedirScript); & ([scriptblock]::Create($script.Substring($script.LastIndexOf('# POWERSHELL PAYLOAD') + 20)))"
set "WxRedirExit=%ERRORLEVEL%"
pause
exit /b %WxRedirExit%
# POWERSHELL PAYLOAD
# Reports which redirects in the project ini files are still referenced, then offers to resave the referencing packages with commandlets and to remove the unreferenced redirects.
# Exit codes: 0 = every redirect is removable (or none exist), 1 = some still need a resave or a manual review, 2 = processing error.
$ErrorActionPreference = 'Stop'

$Latin1 = [Text.Encoding]::GetEncoding(28591)
$CoreKinds = @('Class', 'Struct', 'Enum', 'Object', 'Function', 'Property', 'Package')

# ---------- Package header parsing ----------

function Read-FString([byte[]]$Bytes, [ref]$Pos) {
    $n = [BitConverter]::ToInt32($Bytes, $Pos.Value)
    $Pos.Value += 4
    if ($n -eq 0) { return '' }
    if ($n -gt 0) {
        $s = $Latin1.GetString($Bytes, $Pos.Value, $n - 1)
        $Pos.Value += $n
        return $s
    }
    $s = [Text.Encoding]::Unicode.GetString($Bytes, $Pos.Value, -2 * $n - 2)
    $Pos.Value += -2 * $n
    return $s
}

# Reads only the package header; names, imports, exports and soft object paths all live before TotalHeaderSize.
function Read-HeaderBytes([string]$Path) {
    $stream = [IO.File]::Open($Path, 'Open', 'Read', 'ReadWrite')
    try {
        # The first read covers the summary of any package and the whole header of most; the buffer is then resized to TotalHeaderSize.
        $size = [int][Math]::Min(65536, $stream.Length)
        $bytes = New-Object byte[] $size
        $summary = $null
        $read = 0
        while ($read -lt $size) {
            $chunk = $stream.Read($bytes, $read, $size - $read)
            if ($chunk -le 0) { break }
            $read += $chunk
            if ($read -eq $size -and $null -eq $summary) {
                $summary = Read-SummaryPrefix $bytes
                if ($null -eq $summary) { return $null }
                $size = [int][Math]::Min([long]$summary.TotalHeaderSize, $stream.Length)
                [Array]::Resize([ref]$bytes, $size)
            }
        }
        if ($null -eq $summary) { return $null }
        # The leading comma keeps PowerShell from unrolling the byte array into the pipeline.
        return ,$bytes
    } finally {
        $stream.Dispose()
    }
}

# Field presence follows EUnrealEngineObjectUE4Version / EUnrealEngineObjectUE5Version in Core/Public/UObject/ObjectVersion.h.
function Read-SummaryPrefix([byte[]]$Bytes) {
    # PowerShell reads 0x9E2A83C1 as a negative Int32, so the tag is compared as Int32 too.
    if ($Bytes.Length -lt 64 -or [BitConverter]::ToInt32($Bytes, 0) -ne 0x9E2A83C1) { return $null }
    $p = 4
    $legacy = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    if ($legacy -ne -4) { $p += 4 }
    $ue4 = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $ue5 = 0
    if ($legacy -le -8) { $ue5 = [BitConverter]::ToInt32($Bytes, $p); $p += 4 }
    $p += 4
    $totalHeaderSize = 0
    # PACKAGE_SAVED_HASH moved TotalHeaderSize in front of the custom versions.
    if ($ue5 -ge 1016) { $p += 20; $totalHeaderSize = [BitConverter]::ToInt32($Bytes, $p); $p += 4 }
    $customCount = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    if ($customCount -lt 0 -or $p + $customCount * 20 + 4 -gt $Bytes.Length) { return $null }
    $p += $customCount * 20
    if ($ue5 -lt 1016) { $totalHeaderSize = [BitConverter]::ToInt32($Bytes, $p); $p += 4 }
    if ($totalHeaderSize -le 0) { return $null }
    return @{ UE4 = $ue4; UE5 = $ue5; TotalHeaderSize = $totalHeaderSize; AfterCustomVersions = $p }
}

# A name reference is a name table index plus a number: "Foo_1" is stored as the entry "Foo" with number 2.
function Read-Name([string[]]$Names, [byte[]]$Bytes, [int]$Offset) {
    $name = $Names[[BitConverter]::ToInt32($Bytes, $Offset)]
    $number = [BitConverter]::ToInt32($Bytes, $Offset + 4)
    if ($number -eq 0) { return $name }
    return $name + '_' + ($number - 1)
}

# Mirrors ParseNumberFromName in Core/Public/UObject/NameTypes.h; the result is what the name table holds for the name.
function Get-NameBase([string]$Name) {
    if ($Name -match '^(?<base>.+)_(?<digits>0|[1-9][0-9]{0,9})$' -and [long]$Matches.digits -lt [int]::MaxValue) { return $Matches.base }
    return $Name
}

function Read-Package([string]$Path, [byte[]]$Bytes) {
    $s = Read-SummaryPrefix $Bytes
    $ue4 = $s.UE4; $ue5 = $s.UE5
    $p = $s.AfterCustomVersions
    [void](Read-FString $Bytes ([ref]$p))
    $flags = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $nameCount = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $nameOffset = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $softCount = 0; $softOffset = 0
    if ($ue5 -ge 1008) {
        $softCount = [BitConverter]::ToInt32($Bytes, $p); $p += 4
        $softOffset = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    }
    $editorData = ($flags -band 0x80000000) -eq 0
    if ($editorData -and $ue4 -ge 516) { [void](Read-FString $Bytes ([ref]$p)) }
    if ($ue4 -ge 459) { $p += 8 }
    $exportCount = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $exportOffset = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $importCount = [BitConverter]::ToInt32($Bytes, $p); $p += 4
    $importOffset = [BitConverter]::ToInt32($Bytes, $p); $p += 4

    $names = New-Object string[] $nameCount
    $p = $nameOffset
    for ($i = 0; $i -lt $nameCount; $i++) {
        $names[$i] = Read-FString $Bytes ([ref]$p)
        if ($ue4 -ge 504) { $p += 4 }
    }

    $importSize = 28
    $hasImportPackageName = $editorData -and $ue4 -ge 520
    if ($hasImportPackageName) { $importSize += 8 }
    if ($ue5 -ge 1003) { $importSize += 4 }
    $imports = New-Object object[] $importCount
    for ($i = 0; $i -lt $importCount; $i++) {
        $o = $importOffset + $i * $importSize
        $imports[$i] = @{
            ClassPackage = Read-Name $names $Bytes $o
            ClassName = Read-Name $names $Bytes ($o + 8)
            Outer = [BitConverter]::ToInt32($Bytes, $o + 16)
            Name = Read-Name $names $Bytes ($o + 20)
            PackageName = $null
        }
        if ($hasImportPackageName) { $imports[$i].PackageName = Read-Name $names $Bytes ($o + 28) }
    }

    $soft = New-Object System.Collections.ArrayList
    $p = $softOffset
    for ($i = 0; $i -lt $softCount; $i++) {
        $package = Read-Name $names $Bytes $p
        $asset = Read-Name $names $Bytes ($p + 8)
        $p += 16
        $subPath = Read-FString $Bytes ([ref]$p)
        [void]$soft.Add(@{ Package = $package; Asset = $asset; SubPath = $subPath })
    }

    $nameSet = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($n in $names) { [void]$nameSet.Add($n) }

    return @{
        Path = $Path; UE4 = $ue4; UE5 = $ue5; Bytes = $Bytes
        Names = $nameSet; NameList = $names; Imports = $imports; Soft = $soft; HasSoftList = $ue5 -ge 1008
        ExportCount = $exportCount; ExportOffset = $exportOffset
    }
}

# Returns @{ Package = '/Script/X'; Segments = @('Outer', 'Name') } for an import, walking outers up to the package.
function Resolve-Import($Package, [int]$Index) {
    $segments = New-Object System.Collections.ArrayList
    $current = $Package.Imports[$Index]
    while ($current.Outer -lt 0) {
        [void]$segments.Insert(0, $current.Name)
        $current = $Package.Imports[-$current.Outer - 1]
    }
    return @{ Package = $current.Name; Segments = @($segments) }
}

# The external actor's main object is the export flagged bIsAsset; its class import is what the world partition builder filters by.
function Get-ExternalActorInfo($Package) {
    # FObjectExport layout from TRACK_OBJECT_EXPORT_IS_INHERITED on: 96 bytes, plus the script serialization offsets from SCRIPT_SERIALIZATION_OFFSET.
    if ($Package.UE5 -lt 1006) { return $null }
    $isAssetOffset = 68
    $size = 96
    if ($Package.UE5 -ge 1010) { $size += 16 }
    $map = $null
    for ($i = 0; $i -lt $Package.Imports.Count; $i++) {
        if ($Package.Imports[$i].Name -eq 'PersistentLevel') { $map = (Resolve-Import $Package $i).Package }
    }
    for ($i = 0; $i -lt $Package.ExportCount; $i++) {
        $o = $Package.ExportOffset + $i * $size
        if ([BitConverter]::ToInt32($Package.Bytes, $o + $isAssetOffset) -ne 1) { continue }
        $classIndex = [BitConverter]::ToInt32($Package.Bytes, $o)
        if ($classIndex -ge 0 -or $null -eq $map) { return $null }
        $class = Resolve-Import $Package (-$classIndex - 1)
        return @{ Map = $map; ActorClass = $class.Package + '.' + ($class.Segments -join ':') }
    }
    return $null
}

# ---------- Redirect collection ----------

function Split-ObjectPath([string]$Path) {
    $package = $null
    $rest = $Path
    if ($Path.StartsWith('/')) {
        $dot = $Path.IndexOf('.')
        if ($dot -lt 0) { return @{ Package = $Path; Segments = @() } }
        $package = $Path.Substring(0, $dot)
        $rest = $Path.Substring($dot + 1)
    }
    return @{ Package = $package; Segments = @($rest -split '[.:]') }
}

function Get-IniFiles([string]$Root) {
    $files = @(Get-ChildItem -LiteralPath (Join-Path $Root 'Config') -Filter '*.ini' -File -ErrorAction SilentlyContinue)
    $plugins = Join-Path $Root 'Plugins'
    if (Test-Path -LiteralPath $plugins) {
        $files += @(Get-ChildItem -LiteralPath $plugins -Filter '*.ini' -File -Recurse |
            Where-Object { $_.FullName -match '\\Config\\' -and $_.FullName -notmatch '\\(Binaries|Intermediate|Saved)\\' })
    }
    return $files
}

$RedirectLine = '^\s*\+?(?<key>\w+Redirects)\s*=\s*\((?<body>.*)\)\s*$'

function Get-Redirects([string]$Root) {
    $result = New-Object System.Collections.ArrayList
    foreach ($file in Get-IniFiles $Root) {
        $lines = [IO.File]::ReadAllLines($file.FullName)
        $section = ''
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i]
            if ($line -match '^\s*\[(.+)\]\s*$') { $section = $Matches[1]; continue }
            if ($line -notmatch $RedirectLine) { continue }
            $key = $Matches.key
            $body = $Matches.body
            $fields = @{}
            foreach ($m in [regex]::Matches($body, '(\w+)\s*=\s*"([^"]*)"')) { $fields[$m.Groups[1].Value] = $m.Groups[2].Value }
            $r = @{
                File = $file.FullName; Line = $i; Text = $line.Trim(); Kind = $null; Old = $null; Needles = @()
                Unsupported = $null; Evidence = New-Object System.Collections.ArrayList; TextHits = New-Object System.Collections.ArrayList
            }
            $kind = $key.Substring(0, $key.Length - 'Redirects'.Length)
            if ($kind -in $CoreKinds -and $section -eq 'CoreRedirects') {
                $r.Kind = $kind; $r.Old = $fields['OldName']
                if ($body -match '(MatchSubstring|MatchWildcard|MatchPrefix)\s*=\s*true') { $r.Unsupported = 'uses ' + $Matches[1] }
                elseif ($body -match 'ValueChanges\s*=') { $r.Unsupported = 'renames enum values (ValueChanges)' }
            } elseif ($key -eq 'ActiveGameNameRedirects') {
                $r.Kind = 'GameName'; $r.Old = $fields['OldGameName']
            } elseif ($key -eq 'GameplayTagRedirects') {
                $r.Kind = 'GameplayTag'; $r.Old = $fields['OldTagName']
            } else {
                continue
            }
            if ([string]::IsNullOrEmpty($r.Old)) { $r.Unsupported = 'no old name found' }
            [void]$result.Add($r)
        }
    }
    return $result
}

# Strings whose presence in a header makes the package worth parsing; they are name table spellings, so without the number suffix.
function Get-Needles($Redirect) {
    if ($Redirect.Kind -eq 'GameName') {
        $old = Get-NameBase $Redirect.Old
        if ($old.StartsWith('/Script/')) { return @($old, $old.Substring(8)) }
        return @($old, ('/Script/' + $old))
    }
    if ($Redirect.Kind -eq 'GameplayTag') { return @(Get-NameBase $Redirect.Old) }
    $split = Split-ObjectPath $Redirect.Old
    if ($split.Segments.Count -eq 0) { return @(Get-NameBase $split.Package) }
    return @(Get-NameBase $split.Segments[-1])
}

# ---------- Reference detection ----------

function Test-SegmentsEqual($A, $B) {
    if ($A.Count -ne $B.Count) { return $false }
    for ($i = 0; $i -lt $A.Count; $i++) { if ($A[$i] -ne $B[$i]) { return $false } }
    return $true
}

# Packages saved before the soft object path list keep each soft path in the export data as a single "/Package.Asset" name.
function Test-LegacySoftName($Package, [string]$OldPackage, [string]$OldAsset) {
    foreach ($n in $Package.NameList) {
        if (-not $n.StartsWith('/')) { continue }
        $dot = $n.IndexOf('.')
        if ($dot -lt 0) { continue }
        if ($OldPackage -and $n.Substring(0, $dot) -ne $OldPackage) { continue }
        if ($OldAsset -and $n.Substring($dot + 1) -ne (Get-NameBase $OldAsset)) { continue }
        return $true
    }
    return $false
}

# Returns a short reason when the package references the redirect's old name, $null otherwise.
# Imports and soft paths are compared exactly; the name table alone decides only for references that leave nothing but a name.
function Find-Reference($Redirect, $Package) {
    $names = $Package.Names
    switch ($Redirect.Kind) {
        'GameName' {
            foreach ($n in $Redirect.Needles) { if ($names.Contains($n)) { return "name '$n'" } }
            return $null
        }
        'GameplayTag' {
            if ($names.Contains((Get-NameBase $Redirect.Old))) { return 'tag name' }
            return $null
        }
        'Package' {
            $old = $Redirect.Old
            foreach ($import in $Package.Imports) {
                if ($import.Name -eq $old -or $import.PackageName -eq $old) { return 'import' }
                if ($import.ClassPackage -eq $old) { return 'import class' }
            }
            foreach ($soft in $Package.Soft) { if ($soft.Package -eq $old) { return 'soft path' } }
            # Property tags spell out the script package of their struct/enum type without an import.
            if ($old.StartsWith('/Script/') -and $names.Contains((Get-NameBase $old))) { return 'package name (conservative)' }
            if (-not $Package.HasSoftList -and (Test-LegacySoftName $Package $old $null)) { return 'legacy soft path name (conservative)' }
            return $null
        }
    }
    $old = Split-ObjectPath $Redirect.Old
    $leaf = $old.Segments[-1]
    if ($Redirect.Kind -in @('Property', 'Function')) {
        # Property tags and member references keep only the bare name, so the owner cannot be checked from the header.
        if ($names.Contains((Get-NameBase $leaf))) { return 'name (conservative)' }
        return $null
    }
    for ($i = 0; $i -lt $Package.Imports.Count; $i++) {
        $import = $Package.Imports[$i]
        # An import of an instance names its class without importing the class itself.
        if ($Redirect.Kind -eq 'Class' -and $old.Segments.Count -eq 1 -and $import.ClassName -eq $leaf -and ($null -eq $old.Package -or $import.ClassPackage -eq $old.Package)) { return 'import class' }
        if ($import.Name -ne $leaf) { continue }
        $resolved = Resolve-Import $Package $i
        if (($null -eq $old.Package -or $resolved.Package -eq $old.Package) -and (Test-SegmentsEqual $resolved.Segments $old.Segments)) { return 'import' }
    }
    foreach ($soft in $Package.Soft) {
        $segments = @($soft.Asset)
        if ($soft.SubPath) { $segments += @($soft.SubPath -split '[.:]') }
        if (($null -eq $old.Package -or $soft.Package -eq $old.Package) -and (Test-SegmentsEqual $segments $old.Segments)) { return 'soft path' }
    }
    if (-not $Package.HasSoftList -and (Test-LegacySoftName $Package $old.Package $old.Segments[0])) { return 'legacy soft path name (conservative)' }
    if ($Redirect.Kind -in @('Struct', 'Enum') -and $names.Contains((Get-NameBase $leaf)) -and ($null -eq $old.Package -or $names.Contains($old.Package))) {
        # Property tags name their struct/enum type without an import.
        return 'type name (conservative)'
    }
    return $null
}

function Get-PackageName([string]$Root, [string]$FilePath) {
    $relative = $FilePath.Substring($Root.Length).TrimStart('\')
    $noExt = [IO.Path]::ChangeExtension($relative, $null).TrimEnd('.')
    if ($noExt -match '^Content\\(.+)$') { return '/Game/' + ($Matches[1] -replace '\\', '/') }
    if ($noExt -match '^(?<plugin>Plugins\\.+?)\\Content\\(?<rest>.+)$') {
        $pluginDir = Join-Path $Root $Matches.plugin
        $rest = $Matches.rest
        $uplugin = Get-ChildItem -LiteralPath $pluginDir -Filter '*.uplugin' -File | Select-Object -First 1
        $mount = Split-Path $pluginDir -Leaf
        if ($uplugin) { $mount = $uplugin.BaseName }
        return '/' + $mount + '/' + ($rest -replace '\\', '/')
    }
    return $noExt
}

function Get-ContentRoots([string]$Root) {
    $roots = @()
    $content = Join-Path $Root 'Content'
    if (Test-Path -LiteralPath $content) { $roots += $content }
    $plugins = Join-Path $Root 'Plugins'
    if (Test-Path -LiteralPath $plugins) {
        $roots += @(Get-ChildItem -LiteralPath $plugins -Directory -Recurse -Filter 'Content' |
            Where-Object { $_.FullName -notmatch '\\(Binaries|Intermediate|Saved)\\' -and (Get-ChildItem -LiteralPath $_.Parent.FullName -Filter '*.uplugin' -File) } |
            ForEach-Object { $_.FullName })
    }
    return $roots
}

function Invoke-Audit([string]$Root, $Redirects) {
    $active = @($Redirects | Where-Object { -not $_.Unsupported })
    foreach ($r in $active) {
        $r.Needles = @(Get-Needles $r)
        # A name with non-ASCII characters is stored as UTF-16, which the Latin-1 view of the header shows byte by byte.
        $r.SearchTerms = @($r.Needles) + @($r.Needles | Where-Object { $_ -match '[^\x00-\x7F]' } | ForEach-Object { $Latin1.GetString([Text.Encoding]::Unicode.GetBytes($_)) })
        # Names ignore case; lowering both sides once is much faster than a case-insensitive search per term.
        $r.SearchTerms = @($r.SearchTerms | ForEach-Object { $_.ToLowerInvariant() })
    }
    $allTerms = @($active | ForEach-Object { $_.SearchTerms } | Select-Object -Unique)
    $scanned = 0
    $parsed = 0
    $unreadable = 0
    if ($active.Count -gt 0) {
        foreach ($contentRoot in Get-ContentRoots $Root) {
            foreach ($file in Get-ChildItem -LiteralPath $contentRoot -Recurse -File | Where-Object { $_.Extension -in '.uasset', '.umap' }) {
                $scanned++
                $bytes = Read-HeaderBytes $file.FullName
                if ($null -eq $bytes) { $unreadable++; continue }
                $text = $Latin1.GetString($bytes).ToLowerInvariant()
                $any = $false
                foreach ($n in $allTerms) { if ($text.IndexOf($n, [StringComparison]::Ordinal) -ge 0) { $any = $true; break } }
                if (-not $any) { continue }
                $candidates = @($active | Where-Object {
                    $found = $false
                    foreach ($n in $_.SearchTerms) { if ($text.IndexOf($n, [StringComparison]::Ordinal) -ge 0) { $found = $true; break } }
                    $found
                })
                if ($candidates.Count -eq 0) { continue }
                $parsed++
                try {
                    $package = Read-Package $file.FullName $bytes
                } catch {
                    $unreadable++
                    foreach ($r in $candidates) { [void]$r.Evidence.Add(@{ File = $file.FullName; Reason = 'header parse failed: ' + $_.Exception.Message; Package = $null }) }
                    continue
                }
                foreach ($r in $candidates) {
                    $reason = Find-Reference $r $package
                    if ($reason) { [void]$r.Evidence.Add(@{ File = $file.FullName; Reason = $reason; Package = $package }) }
                }
            }
        }
    }

    # Missing evidence from an unreadable package must never authorize removal.
    if ($unreadable -gt 0) {
        throw ('Audit incomplete: {0} asset file(s) could not be read. Redirect removal is blocked.' -f $unreadable)
    }

    # Text references (string paths in code or config) are never fixed by a resave, so they always need a person.
    $textFiles = @(Get-IniFiles $Root)
    foreach ($dir in @((Join-Path $Root 'Source'), (Join-Path $Root 'Plugins'))) {
        if (Test-Path -LiteralPath $dir) {
            $textFiles += @(Get-ChildItem -LiteralPath $dir -Recurse -File |
                Where-Object { $_.Extension -in '.h', '.cpp', '.cs' -and $_.FullName -notmatch '\\(Binaries|Intermediate|Saved|ThirdParty)\\' })
        }
    }
    $textPaths = @($textFiles | ForEach-Object { $_.FullName })
    foreach ($r in $active) {
        if ($textPaths.Count -eq 0) { break }
        $pattern = $null
        if ($r.Kind -eq 'GameplayTag') { $pattern = '"' + $r.Old + '"' }
        elseif ($r.Kind -eq 'GameName') { $pattern = $r.Needles | Where-Object { $_.StartsWith('/Script/') } | Select-Object -First 1 }
        elseif ($r.Old.StartsWith('/')) { $pattern = $r.Old }
        if (-not $pattern) { continue }
        foreach ($m in (Select-String -LiteralPath $textPaths -Pattern $pattern -SimpleMatch -CaseSensitive -ErrorAction SilentlyContinue)) {
            # The redirect entries themselves and ini comments are not references.
            if ($m.Line -match $RedirectLine -or ($m.Path.EndsWith('.ini') -and $m.Line -match '^\s*;')) { continue }
            [void]$r.TextHits.Add(('{0}:{1}' -f $m.Path.Substring($Root.Length).TrimStart('\'), $m.LineNumber))
        }
    }
    return @{ Scanned = $scanned; Parsed = $parsed; Unreadable = $unreadable; TextFiles = $textFiles.Count }
}

function Get-Status($Redirect, $Resaved) {
    if ($Redirect.Unsupported) { return 'REVIEW' }
    if ($Redirect.TextHits.Count -gt 0) { return 'REVIEW' }
    if ($Redirect.Evidence.Count -eq 0) { return 'SAFE' }
    if ($Resaved) {
        # Evidence that survives a resave is nothing another resave would fix.
        foreach ($e in $Redirect.Evidence) { if ($Resaved.Contains($e.File)) { return 'REVIEW' } }
    }
    return 'RESAVE'
}

function Write-Report([string]$Root, $Redirects, $Stats, $Resaved) {
    Write-Host ('Scanned {0} assets ({1} parsed) and {2} text files.' -f $Stats.Scanned, $Stats.Parsed, $Stats.TextFiles)
    if ($Stats.Unreadable -gt 0) { Write-Warning ('{0} asset file(s) are not readable packages and were skipped.' -f $Stats.Unreadable) }
    $counts = @{ SAFE = 0; RESAVE = 0; REVIEW = 0 }
    foreach ($r in $Redirects) {
        $status = Get-Status $r $Resaved
        $counts[$status]++
        Write-Host ''
        Write-Host ('[{0}] {1}:{2}' -f $status, $r.File.Substring($Root.Length).TrimStart('\'), ($r.Line + 1))
        Write-Host ('    ' + $r.Text)
        if ($r.Unsupported) { Write-Host ('    cannot be checked automatically: ' + $r.Unsupported) }
        foreach ($t in $r.TextHits) { Write-Host ('    text reference: ' + $t) }
        foreach ($e in $r.Evidence) {
            $line = '    {0} ({1})' -f (Get-PackageName $Root $e.File), $e.Reason
            if ($e.Package -and $e.File -match '\\__ExternalActors__\\') {
                $info = Get-ExternalActorInfo $e.Package
                if ($info) { $line += ' - external actor {0} in {1}' -f $info.ActorClass, $info.Map }
            }
            if ($Resaved -and $Resaved.Contains($e.File)) {
                if ($e.Reason -in 'import', 'import class', 'soft path') { $line += ' - still present after resave; the resave did not rewrite this reference' }
                else { $line += ' - still present after resave; likely an unrelated use of the same name' }
            }
            Write-Host $line
        }
    }
    Write-Host ''
    Write-Host ('Summary: {0} removable, {1} need resave, {2} need review.' -f $counts.SAFE, $counts.RESAVE, $counts.REVIEW)
    return $counts
}

# ---------- Resave ----------

function Get-EngineDir {
    $launcherData = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
    if (-not (Test-Path -LiteralPath $launcherData)) { throw "Epic Games Launcher installation data not found: $launcherData" }
    foreach ($entry in (ConvertFrom-Json ([IO.File]::ReadAllText($launcherData))).InstallationList) {
        if ($entry.AppName -eq 'UE_5.8') { return $entry.InstallLocation }
    }
    throw "UE 5.8 installation not found in: $launcherData"
}

function Invoke-Commandlet([string]$Exe, [string[]]$Arguments, [string]$LogPath) {
    # Under 'Stop', Windows PowerShell turns the first redirected stderr line of a native program into a terminating error.
    $ErrorActionPreference = 'Continue'
    & $Exe @Arguments *> $LogPath
    return $LASTEXITCODE
}

function Invoke-Resave([string]$Root, $Redirects) {
    $uproject = Join-Path $Root 'Wx.uproject'
    $running = @(Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" |
        Where-Object { $_.CommandLine -and $_.CommandLine.IndexOf('Wx.uproject', [StringComparison]::OrdinalIgnoreCase) -ge 0 })
    if ($running.Count -gt 0) {
        # A running editor keeps loaded packages open, so the commandlet cannot replace them.
        throw ('Close every editor or commandlet that has Wx.uproject open before resaving (PID: {0}).' -f (($running | ForEach-Object { $_.ProcessId }) -join ', '))
    }
    $exe = Join-Path (Get-EngineDir) 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $logDir = Join-Path $Root 'Saved\Logs\CheckRedirects'
    [void](New-Item -ItemType Directory -Force -Path $logDir)

    $packages = @{}
    $worlds = @{}
    $failed = 0
    foreach ($r in $Redirects) {
        if ((Get-Status $r $null) -ne 'RESAVE') { continue }
        foreach ($e in $r.Evidence) {
            if ($e.File -notmatch '\\__ExternalActors__\\') { $packages[$e.File] = $true; continue }
            $info = $null
            if ($e.Package) { $info = Get-ExternalActorInfo $e.Package }
            if ($null -eq $info) {
                $failed++
                Write-Warning ('Skipped {0}: could not read its map and actor class. Resave it from the editor.' -f $e.File)
                continue
            }
            if (-not $worlds.ContainsKey($info.Map)) { $worlds[$info.Map] = @{} }
            $worlds[$info.Map][$info.ActorClass] = $true
        }
    }

    $resaved = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $index = 0
    $files = @($packages.Keys)
    $start = 0
    while ($start -lt $files.Count) {
        # One commandlet run takes every package that fits in the command line, so the editor boots once instead of once per package.
        $switches = @()
        $length = 0
        $end = $start
        while ($end -lt $files.Count) {
            $argument = '-PACKAGE=' + (Get-PackageName $Root $files[$end])
            if ($switches.Count -gt 0 -and $length + $argument.Length + 1 -gt 24000) { break }
            $switches += $argument
            $length += $argument.Length + 1
            $end++
        }
        $index++
        $batch = @($files[$start..($end - 1)])
        $start = $end
        Write-Host ('Resaving {0} package(s)' -f $batch.Count)
        $before = @{}
        foreach ($file in $batch) { $before[$file] = (Get-Item -LiteralPath $file).LastWriteTimeUtc }
        $log = Join-Path $logDir ('resave_{0}.log' -f $index)
        $code = Invoke-Commandlet $exe (@($uproject, '-run=ResavePackages') + $switches + @('-unattended', '-nopause', '-nosplash', '-SCCProvider=None')) $log
        # Rewritten timestamps do not prove a failed commandlet completed safely.
        if ($code -ne 0) {
            $failed++
            Write-Warning ('ResavePackages reported errors (exit code {0}). See {1}' -f $code, $log)
        }
        foreach ($file in $batch) {
            if ((Get-Item -LiteralPath $file).LastWriteTimeUtc -eq $before[$file]) {
                $failed++
                Write-Warning ('Not resaved: {0}. See {1}' -f (Get-PackageName $Root $file), $log)
            } else {
                Write-Host ('    ' + (Get-PackageName $Root $file))
                [void]$resaved.Add($file)
            }
        }
    }

    foreach ($map in $worlds.Keys) {
        $index++
        $classes = @($worlds[$map].Keys)
        $mapFolder = Get-ExternalActorFolder $Root $map
        if (-not $mapFolder) {
            # Without the folder the actor files the builder deletes would go unnoticed.
            $failed++
            Write-Warning ('Skipped {0}: its external actor folder was not found. Resave its actors from the editor.' -f $map)
            continue
        }
        Write-Host ('Resaving actors in {0}: {1}' -f $map, ($classes -join ', '))
        $classFile = Join-Path $logDir ('actor_classes_{0}.txt' -f $index)
        [IO.File]::WriteAllLines($classFile, [string[]]$classes)
        $before = @{}
        Get-ChildItem -LiteralPath $mapFolder -Recurse -File | ForEach-Object { $before[$_.FullName] = $_.LastWriteTimeUtc }
        $log = Join-Path $logDir ('resave_{0}.log' -f $index)
        $code = Invoke-Commandlet $exe @($uproject, $map, '-run=WorldPartitionBuilderCommandlet', '-Builder=WorldPartitionResaveActorsBuilder', ('-ActorClassesFromFile=' + $classFile), '-SCCProvider=None', '-unattended', '-nopause', '-nosplash') $log
        $changed = 0
        $missing = @()
        foreach ($path in $before.Keys) {
            if (-not (Test-Path -LiteralPath $path)) { $missing += $path; continue }
            if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -ne $before[$path]) { $changed++; [void]$resaved.Add($path) }
        }
        if ($missing.Count -gt 0) {
            # The builder deletes actor packages it fails to load; restoring the whole folder would also undo the actors it did resave.
            $failed++
            Write-Warning ('The world partition builder deleted {0} actor file(s) in {1}. Restore them from the project folder with:' -f $missing.Count, $map)
            foreach ($path in $missing) { Write-Warning ('    git restore -- "{0}"' -f ($path.Substring($Root.Length).TrimStart('\') -replace '\\', '/')) }
        }
        if ($code -ne 0) {
            $failed++
            Write-Warning ('World partition resave failed for {0} (exit code {1}). See {2}' -f $map, $code, $log)
        }
        Write-Host ('    {0} actor file(s) rewritten' -f $changed)
    }
    return @{ Resaved = $resaved; Failed = $failed }
}

function Get-ExternalActorFolder([string]$Root, [string]$Map) {
    if ($Map -notmatch '^/(?<mount>[^/]+)/(?<rest>.+)$') { return $null }
    $rest = $Matches.rest -replace '/', '\'
    if ($Matches.mount -eq 'Game') { $folder = Join-Path $Root ('Content\__ExternalActors__\' + $rest) }
    else {
        $uplugin = Get-ChildItem -LiteralPath (Join-Path $Root 'Plugins') -Recurse -Filter ($Matches.mount + '.uplugin') -File | Select-Object -First 1
        if (-not $uplugin) { return $null }
        $folder = Join-Path $uplugin.DirectoryName ('Content\__ExternalActors__\' + $rest)
    }
    if (Test-Path -LiteralPath $folder) { return $folder }
    return $null
}

# ---------- Remove ----------

function Remove-SafeRedirects([string]$Root, $Redirects) {
    $removed = 0
    foreach ($group in ($Redirects | Where-Object { (Get-Status $_ $null) -eq 'SAFE' } | Group-Object { $_.File })) {
        $path = $group.Name
        $raw = [IO.File]::ReadAllBytes($path)
        $hasBom = $raw.Length -ge 3 -and $raw[0] -eq 0xEF -and $raw[1] -eq 0xBB -and $raw[2] -eq 0xBF
        $text = (New-Object Text.UTF8Encoding($false)).GetString($raw)
        if ($hasBom) { $text = $text.Substring(1) }
        # Each element keeps its own line terminator so untouched lines are written back byte for byte.
        $lines = [regex]::Split($text, '(?<=\n)')

        $drop = New-Object 'System.Collections.Generic.HashSet[int]'
        foreach ($r in $group.Group) {
            if ($r.Line -ge $lines.Count -or $lines[$r.Line].Trim() -cne $r.Text) { throw ('{0} changed since it was checked; nothing was removed from it.' -f $path) }
            [void]$drop.Add($r.Line)
            $removed++
        }
        foreach ($r in $group.Group) {
            # A comment block on top of a run of redirects may document the whole run, so it stays while a redirect of the run survives.
            $next = $r.Line + 1
            while ($next -lt $lines.Count -and $drop.Contains($next)) { $next++ }
            if ($next -lt $lines.Count -and $lines[$next] -match $RedirectLine) { continue }
            $i = $r.Line - 1
            while ($i -ge 0 -and $lines[$i] -match '^\s*;') { [void]$drop.Add($i); $i-- }
        }
        # Drop [CoreRedirects] sections left with nothing but blank lines.
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -notmatch '^\s*\[CoreRedirects\]\s*$') { continue }
            $j = $i + 1
            $empty = $true
            while ($j -lt $lines.Count -and $lines[$j] -notmatch '^\s*\[') {
                if (-not $drop.Contains($j) -and $lines[$j].Trim() -ne '') { $empty = $false }
                $j++
            }
            if (-not $empty) { continue }
            for ($k = $i; $k -lt $j; $k++) { [void]$drop.Add($k) }
            # The blank line above the header still separates the neighbours unless the section was the last one.
            if ($j -ge $lines.Count -and $i -gt 0 -and $lines[$i - 1].Trim() -eq '') { [void]$drop.Add($i - 1) }
        }

        $output = New-Object Text.StringBuilder
        for ($i = 0; $i -lt $lines.Count; $i++) { if (-not $drop.Contains($i)) { [void]$output.Append($lines[$i]) } }
        [IO.File]::WriteAllText($path, $output.ToString(), (New-Object Text.UTF8Encoding($hasBom)))
        Write-Host ('Removed {0} redirect(s) from {1}' -f $group.Count, $path.Substring($Root.Length).TrimStart('\'))
    }
    return $removed
}

# ---------- Main ----------

try {
    $root = [IO.Path]::GetFullPath($env:WxRedirRoot).TrimEnd('\')
    if (-not (Test-Path -LiteralPath (Join-Path $root 'Wx.uproject') -PathType Leaf)) {
        throw 'Wx.uproject not found next to BatchFiles.'
    }
    $redirects = @(Get-Redirects $root)
    if ($redirects.Count -eq 0) {
        Write-Host 'No redirects found.'
        exit 0
    }

    $stats = Invoke-Audit $root $redirects
    $counts = Write-Report $root $redirects $stats $null

    # Both steps change files, so each one waits for an explicit yes; an empty answer means no.
    if ($counts.RESAVE -gt 0) {
        Write-Host ''
        $answer = Read-Host ('Resave the assets that still reference {0} redirect(s)? Close the editor first; this runs it in the background for a few minutes. [y/N]' -f $counts.RESAVE)
        if ($answer -match '^\s*y(es)?\s*$') {
            $result = Invoke-Resave $root $redirects
            if ($result.Failed -gt 0) {
                throw ('{0} resave step(s) failed. Redirect removal is blocked; inspect the logs before retrying.' -f $result.Failed)
            }
            Write-Host ''
            Write-Host 'Re-checking after resave...'
            $redirects = @(Get-Redirects $root)
            $stats = Invoke-Audit $root $redirects
            $counts = Write-Report $root $redirects $stats $result.Resaved
        }
    }

    if ($counts.SAFE -gt 0) {
        Write-Host ''
        $answer = Read-Host ('Remove the {0} removable redirect(s) from the ini files? [y/N]' -f $counts.SAFE)
        if ($answer -match '^\s*y(es)?\s*$') { [void](Remove-SafeRedirects $root $redirects) }
    }

    if ($counts.RESAVE -gt 0 -or $counts.REVIEW -gt 0) { exit 1 }
    exit 0
} catch {
    [Console]::Error.WriteLine('ERROR: ' + $_.Exception.Message)
    exit 2
}
