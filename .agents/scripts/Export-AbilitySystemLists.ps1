# Copyright Woogle. All Rights Reserved.
# Rebuilds the ability and effect list wiki articles from Content packages and C++ sources without launching the editor.
# Runs on Windows PowerShell 5.1 and PowerShell 7; the output must not depend on which one ran it.
[CmdletBinding()]
param([string]$RepoRoot)
$ErrorActionPreference = 'Stop'

$Latin1 = [Text.Encoding]::GetEncoding(28591)
$Invariant = [Globalization.CultureInfo]::InvariantCulture
# Both articles live in .wiki/wiki/references, so links climb three folders to the repository root.
$ArticleFolder = '.wiki/wiki/references'
$ArticleToRepo = '../../../'
# Assets are found by name prefix; no other package is read.
$PackagePattern = '^(GA|ABS|GE|AM|BT|ST|DA|DT|BP)_'
# Engine bases that decide what a project class is.
$AbilityRoots = @('UGameplayAbility')
$EffectRoots = @('UGameplayEffect')
$EffectPartRoots = @('UGameplayEffectComponent', 'UGameplayEffectUIData', 'UGameplayModMagnitudeCalculation', 'UGameplayEffectExecutionCalculation')
# Fields deprecated in 5.3 for GEComponents; UGameplayEffect::PreSave copies the component values back into them, so they only repeat the components.
$DeprecatedEffectProps = @('InheritableGameplayEffectTags', 'InheritableOwnedTagsContainer', 'InheritableBlockedAbilityTagsContainer', 'OngoingTagRequirements', 'ApplicationTagRequirements', 'RemovalTagRequirements', 'RemoveGameplayEffectsWithTags', 'GrantedApplicationImmunityTags', 'GrantedApplicationImmunityQuery', 'RemoveGameplayEffectQuery', 'GrantedAbilities', 'ConditionalGameplayEffects', 'PrematureExpirationEffectClasses', 'RoutineExpirationEffectClasses')

# ---------- Package parsing ----------
# Field layout follows the UE 5.8 editor package format: Core/Public/UObject/ObjectVersion.h, CoreUObject/Private/UObject/ObjectResource.cpp and PropertyTag.cpp.

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

# A name reference is a name table index plus a number: "Foo_1" is stored as the entry "Foo" with number 2.
function Read-Name($Package, [int]$Offset) {
    $index = [BitConverter]::ToInt32($Package.Bytes, $Offset)
    $number = [BitConverter]::ToInt32($Package.Bytes, $Offset + 4)
    # Range checks make a wrong structural guess fail at once instead of reading garbage.
    if ($index -lt 0 -or $index -ge $Package.Names.Length -or $number -lt 0) { throw "bad name reference at $Offset" }
    $name = $Package.Names[$index]
    if ($number -eq 0) { return $name }
    return $name + '_' + ($number - 1)
}

function Read-Package([byte[]]$Bytes) {
    $b = $Bytes
    # PowerShell reads 0x9E2A83C1 as a negative Int32, so the tag is compared as Int32 too.
    if ($b.Length -lt 64 -or [BitConverter]::ToInt32($b, 0) -ne 0x9E2A83C1) { throw 'not an Unreal package' }
    $p = 4
    $legacy = [BitConverter]::ToInt32($b, $p); $p += 4
    if ($legacy -ne -4) { $p += 4 }
    $ue4 = [BitConverter]::ToInt32($b, $p); $p += 4
    $ue5 = 0
    if ($legacy -le -8) { $ue5 = [BitConverter]::ToInt32($b, $p); $p += 4 }
    $p += 4
    # PACKAGE_SAVED_HASH moved TotalHeaderSize in front of the custom versions.
    if ($ue5 -ge 1016) { $p += 24 }
    $p += 4 + 20 * [BitConverter]::ToInt32($b, $p)
    if ($ue5 -lt 1016) { $p += 4 }
    [void](Read-FString $b ([ref]$p))
    $flags = [BitConverter]::ToInt32($b, $p); $p += 4
    $nameCount = [BitConverter]::ToInt32($b, $p); $p += 4
    $nameOffset = [BitConverter]::ToInt32($b, $p); $p += 4
    $softCount = 0; $softOffset = 0
    if ($ue5 -ge 1008) {
        $softCount = [BitConverter]::ToInt32($b, $p); $p += 4
        $softOffset = [BitConverter]::ToInt32($b, $p); $p += 4
    }
    if (($flags -band 0x80000000) -ne 0) { throw 'cooked packages are not supported' }
    [void](Read-FString $b ([ref]$p))
    $p += 8
    $exportCount = [BitConverter]::ToInt32($b, $p); $p += 4
    $exportOffset = [BitConverter]::ToInt32($b, $p); $p += 4
    $importCount = [BitConverter]::ToInt32($b, $p); $p += 4
    $importOffset = [BitConverter]::ToInt32($b, $p); $p += 4
    # VERSE_CELLS adds cell export/import tables, METADATA_SERIALIZATION_OFFSET a metadata offset; then depends and soft package references.
    if ($ue5 -ge 1015) { $p += 16 }
    if ($ue5 -ge 1014) { $p += 4 }
    $p += 12
    $searchableOffset = [BitConverter]::ToInt32($b, $p)

    $package = @{ Bytes = $b; Names = (New-Object string[] $nameCount); Imports = @(); Exports = $null; RowReferences = @() }
    # Inlined FString reads; each entry is followed by a 4 byte hash. Big packages have thousands of names.
    $p = $nameOffset
    for ($i = 0; $i -lt $nameCount; $i++) {
        $n = [BitConverter]::ToInt32($b, $p)
        if ($n -ge 0) {
            $package.Names[$i] = $Latin1.GetString($b, $p + 4, [Math]::Max($n - 1, 0))
            $p += 8 + $n
        } else {
            $package.Names[$i] = [Text.Encoding]::Unicode.GetString($b, $p + 4, -2 * $n - 2)
            $p += 8 - 2 * $n
        }
    }

    # Editor imports carry PackageName and, from OPTIONAL_RESOURCES, bImportOptional after the 28 byte core.
    if ($ue5 -ge 1003) {
        $package.Imports = @(for ($i = 0; $i -lt $importCount; $i++) {
            $o = $importOffset + $i * 40
            @{ Outer = [BitConverter]::ToInt32($b, $o + 16); Name = (Read-Name $package ($o + 20)) }
        })
    }
    # Searchable names are the engine's "Find Row References" record: each DataTable this package uses, with the row names it points at.
    if ($searchableOffset -gt 0) {
        $p = $searchableOffset
        $count = [BitConverter]::ToInt32($b, $p); $p += 4
        $package.RowReferences = @(for ($i = 0; $i -lt $count; $i++) {
            $index = [BitConverter]::ToInt32($b, $p)
            $n = [BitConverter]::ToInt32($b, $p + 4)
            $p += 8
            $rows = @(for ($j = 0; $j -lt $n; $j++) { Read-Name $package ($p + 8 * $j) })
            $p += 8 * $n
            @{ Index = $index; Rows = $rows }
        })
    }
    # Older packages keep only names and imports readable; PROPERTY_TAG_COMPLETE_TYPE_NAME changed the property tags below.
    if ($ue5 -lt 1012) { return $package }

    # From SCRIPT_SERIALIZATION_OFFSET an export entry is 112 bytes and ends with the tagged property range.
    $package.Exports = @(for ($i = 0; $i -lt $exportCount; $i++) {
        $o = $exportOffset + $i * 112
        @{
            Class = [BitConverter]::ToInt32($b, $o); Super = [BitConverter]::ToInt32($b, $o + 4); Name = (Read-Name $package ($o + 16))
            ScriptStart = [BitConverter]::ToInt64($b, $o + 36) + [BitConverter]::ToInt64($b, $o + 96)
            ScriptEnd = [BitConverter]::ToInt64($b, $o + 36) + [BitConverter]::ToInt64($b, $o + 104)
            SerialEnd = [BitConverter]::ToInt64($b, $o + 36) + [BitConverter]::ToInt64($b, $o + 28)
        }
    })
    $package.Soft = @()
    $p = $softOffset
    for ($i = 0; $i -lt $softCount; $i++) {
        $path = (Read-Name $package $p) + '.' + (Read-Name $package ($p + 8))
        $p += 16
        $subPath = Read-FString $b ([ref]$p)
        if ($subPath) { $path += ':' + $subPath }
        $package.Soft += $path
    }
    return $package
}

# Imports resolve to "/Game/Dir/Asset.Object" or "/Script/Module.Class"; exports to their local name.
function Resolve-Index($Package, [int]$Index) {
    if ($Index -eq 0) { return $null }
    if ($Index -gt 0) { return $Package.Exports[$Index - 1].Name }
    $import = $Package.Imports[-$Index - 1]
    $path = $import.Name
    while ($import.Outer -lt 0) {
        $import = $Package.Imports[-$import.Outer - 1]
        $path = $import.Name + '.' + $path
    }
    return $path
}

function Get-Exports($Package) {
    if ($null -eq $Package.Exports) { throw 'saved before UE 5.4 property tags; resave the asset' }
    return $Package.Exports
}

function Find-Export($Package, [string]$Name) {
    foreach ($export in (Get-Exports $Package)) { if ($export.Name -eq $Name) { return $export } }
    return $null
}

# FPropertyTypeName is a pre-order list of (name, parameter count) nodes, e.g. ArrayProperty(StructProperty(GameplayTag(/Script/GameplayTags))).
function Read-TypeName($Package, [ref]$Pos) {
    # Most tags are a single node such as FloatProperty.
    if ([BitConverter]::ToInt32($Package.Bytes, $Pos.Value + 8) -eq 0) {
        $name = Read-Name $Package $Pos.Value
        $Pos.Value += 12
        return @{ Name = $name; Params = @() }
    }
    $nodes = New-Object System.Collections.ArrayList
    $remaining = 1
    while ($remaining -gt 0) {
        $count = [BitConverter]::ToInt32($Package.Bytes, $Pos.Value + 8)
        if ($count -lt 0 -or $count -gt 8) { throw "bad property type at $($Pos.Value)" }
        [void]$nodes.Add(@{ Name = (Read-Name $Package $Pos.Value); Count = $count })
        $Pos.Value += 12
        $remaining += $count - 1
    }
    $next = 0
    return (New-TypeNode $nodes ([ref]$next))
}

function New-TypeNode($Nodes, [ref]$Next) {
    $node = $Nodes[$Next.Value]
    $Next.Value++
    $params = @(for ($i = 0; $i -lt $node.Count; $i++) { New-TypeNode $Nodes $Next })
    return @{ Name = $node.Name; Params = $params }
}

# Returns the property values in serialized order; the stream ends with a "None" name.
# $Only, a set of property names, limits which values are read at every nesting level; the rest are skipped by size.
function Read-Properties($Package, [ref]$Pos, [bool]$IsObject, $Only = $null) {
    $b = $Package.Bytes
    if ($IsObject) {
        # EClassSerializationControlExtension; OverridableSerializationInformation adds an operation byte.
        $control = $b[$Pos.Value]
        $Pos.Value += 1
        if ($control -band 0x02) { $Pos.Value += 1 }
    }
    $values = [ordered]@{}
    $names = $Package.Names
    while ($true) {
        # Read-Name and Skip-TypeName inlined: this loop runs for every field of every notify event and dominates the run time.
        $nameIndex = [BitConverter]::ToInt32($b, $Pos.Value)
        $number = [BitConverter]::ToInt32($b, $Pos.Value + 4)
        if ($nameIndex -lt 0 -or $nameIndex -ge $names.Length -or $number -lt 0) { throw "bad name reference at $($Pos.Value)" }
        $name = $names[$nameIndex]
        if ($number -ne 0) { $name += '_' + ($number - 1) }
        $Pos.Value += 8
        if ($name -eq 'None') { return $values }
        $wanted = $null -eq $Only -or $Only.ContainsKey($name)
        if ($wanted) {
            $type = Read-TypeName $Package $Pos
        } else {
            $remaining = 1
            while ($remaining -gt 0) {
                $count = [BitConverter]::ToInt32($b, $Pos.Value + 8)
                if ($count -lt 0 -or $count -gt 8) { throw "bad property type at $($Pos.Value)" }
                $Pos.Value += 12
                $remaining += $count - 1
            }
        }
        $size = [BitConverter]::ToInt32($b, $Pos.Value)
        if ($size -lt 0 -or $Pos.Value + $size -gt $b.Length) { throw "bad property size at $($Pos.Value)" }
        $flags = $b[$Pos.Value + 4]
        $Pos.Value += 5
        $index = 0
        if ($flags -band 0x01) { $index = [BitConverter]::ToInt32($b, $Pos.Value); $Pos.Value += 4 }
        if ($flags -band 0x02) { $Pos.Value += 16 }
        if ($flags -band 0x04) {
            # EPropertyTagExtension: overridable information is a uint8 and a 4 byte bool, external objects a 4 byte bool.
            $extension = $b[$Pos.Value]
            $Pos.Value += 1
            if ($extension -band 0x02) { $Pos.Value += 5 }
            if ($extension -band 0x04) { $Pos.Value += 4 }
        }
        $key = $name
        if ($index -ne 0) { $key = "$name[$index]" }
        $start = $Pos.Value
        if (-not $wanted) {
        } elseif ($flags -band 0x20) {
            $values[$key] = $null
        } elseif ($type.Name -eq 'BoolProperty') {
            $values[$key] = ($flags -band 0x10) -ne 0
        } else {
            $values[$key] = Read-Value $Package $type $start $size (($flags -band 0x08) -ne 0) $Only
        }
        $Pos.Value = $start + $size
    }
}

# Values with a fixed width inside arrays; top-level tags carry their own size.
$ScalarWidths = @{
    FloatProperty = 4; DoubleProperty = 8; IntProperty = 4; UInt32Property = 4; Int64Property = 8; UInt64Property = 8
    Int16Property = 2; UInt16Property = 2; Int8Property = 1; NameProperty = 8; EnumProperty = 8
    ObjectProperty = 4; ClassProperty = 4; WeakObjectProperty = 4; InterfaceProperty = 4; SoftObjectProperty = 4; SoftClassProperty = 4
}

# Core math and id structs are always binary; trying them as tags only wastes time.
$BinaryStructs = @('Guid', 'Vector', 'Vector2D', 'Vector4', 'Rotator', 'Quat', 'Transform', 'LinearColor', 'Color', 'IntPoint', 'IntVector', 'Box', 'Box2D', 'Plane', 'Matrix', 'DateTime', 'Timespan', 'FrameNumber', 'FrameRate')

function Read-Value($Package, $Type, [int]$Pos, [int]$Size, [bool]$Native, $Only = $null) {
    $b = $Package.Bytes
    switch ($Type.Name) {
        'FloatProperty' { return [BitConverter]::ToSingle($b, $Pos) }
        'DoubleProperty' { return [BitConverter]::ToDouble($b, $Pos) }
        'IntProperty' { return [BitConverter]::ToInt32($b, $Pos) }
        'UInt32Property' { return [BitConverter]::ToUInt32($b, $Pos) }
        'Int64Property' { return [BitConverter]::ToInt64($b, $Pos) }
        'UInt64Property' { return [BitConverter]::ToUInt64($b, $Pos) }
        'Int16Property' { return [BitConverter]::ToInt16($b, $Pos) }
        'UInt16Property' { return [BitConverter]::ToUInt16($b, $Pos) }
        'Int8Property' { return [sbyte]$b[$Pos] }
        # An enum-backed byte stores the enumerator name.
        'ByteProperty' { if ($Size -eq 8) { return Read-Name $Package $Pos } else { return $b[$Pos] } }
        'BoolProperty' { return $b[$Pos] -ne 0 }
        'EnumProperty' { return Read-Name $Package $Pos }
        'NameProperty' { return Read-Name $Package $Pos }
        'StrProperty' { $p = $Pos; return Read-FString $b ([ref]$p) }
        'TextProperty' { return Read-Text $b $Pos }
        { $_ -in @('ObjectProperty', 'ClassProperty', 'WeakObjectProperty', 'InterfaceProperty') } { return Resolve-Index $Package ([BitConverter]::ToInt32($b, $Pos)) }
        # Packages with a soft object path list store the list index.
        { $_ -in @('SoftObjectProperty', 'SoftClassProperty') } { return $Package.Soft[[BitConverter]::ToInt32($b, $Pos)] }
        'StructProperty' {
            if (-not $Native) { $p = $Pos; return Read-Properties $Package ([ref]$p) $false $Only }
            if ($Type.Params[0].Name -eq 'GameplayTagContainer') { return ,(Read-TagContainer $Package $Pos) }
            if ($BinaryStructs -contains $Type.Params[0].Name) { return '(' + $Type.Params[0].Name + ')' }
            # A native serializer may decline and fall back to tags (FGameplayEffectModifierMagnitude); accept that only when the stream fills the value exactly.
            try {
                $p = $Pos
                $values = Read-Properties $Package ([ref]$p) $false $Only
                if ($p -eq $Pos + $Size) { return $values }
            } catch {
            }
            return '(' + $Type.Params[0].Name + ')'
        }
        'ArrayProperty' { return ,(Read-Array $Package $Type.Params[0] $Pos $Size $Native $Only) }
    }
    return '(' + $Type.Name + ')'
}

# FGameplayTagContainer serializes natively as a counted FName array.
function Read-TagContainer($Package, [int]$Pos) {
    $count = [BitConverter]::ToInt32($Package.Bytes, $Pos)
    return @(for ($i = 0; $i -lt $count; $i++) { Read-Name $Package ($Pos + 4 + 8 * $i) })
}

# FText: flags, history type, then Base = namespace, key, source string; None = optional culture invariant string.
function Read-Text([byte[]]$Bytes, [int]$Pos) {
    $p = $Pos + 4
    $history = [sbyte]$Bytes[$p]
    $p += 1
    if ($history -eq 0) {
        [void](Read-FString $Bytes ([ref]$p))
        [void](Read-FString $Bytes ([ref]$p))
        return Read-FString $Bytes ([ref]$p)
    }
    if ($history -eq -1) {
        if ([BitConverter]::ToInt32($Bytes, $p) -eq 0) { return '' }
        $p += 4
        return Read-FString $Bytes ([ref]$p)
    }
    return '(FText)'
}

# Elements have no tags; anything whose width cannot be known collapses to an item count instead of guessing.
function Read-Array($Package, $Inner, [int]$Pos, [int]$Size, [bool]$Native, $Only = $null) {
    $b = $Package.Bytes
    $count = [BitConverter]::ToInt32($b, $Pos)
    $p = $Pos + 4
    $items = New-Object System.Collections.ArrayList
    for ($i = 0; $i -lt $count; $i++) {
        $width = $ScalarWidths[$Inner.Name]
        if ($Inner.Name -eq 'ByteProperty') { $width = 1; if ($Inner.Params.Count -gt 0) { $width = 8 } }
        if ($Inner.Name -eq 'BoolProperty') { $width = 1 }
        if ($width) {
            [void]$items.Add((Read-Value $Package $Inner $p $width $false))
            $p += $width
        } elseif ($Inner.Name -eq 'StructProperty' -and $Inner.Params[0].Name -eq 'GameplayTagContainer') {
            [void]$items.Add((Read-TagContainer $Package $p))
            $p += 4 + 8 * [BitConverter]::ToInt32($b, $p)
        } elseif ($Inner.Name -eq 'StructProperty') {
            # Native element structs may still be tagged (see Read-Value); the size check below rejects a wrong guess.
            try { [void]$items.Add((Read-Properties $Package ([ref]$p) $false $Only)) } catch { return "($count items)" }
        } elseif ($Inner.Name -eq 'StrProperty') {
            [void]$items.Add((Read-FString $b ([ref]$p)))
        } else {
            return "($count items)"
        }
    }
    if ($p -ne $Pos + $Size) { return "($count items)" }
    return $items.ToArray()
}

# The tagged range must end exactly where the export table says; anything else means the layout above is wrong.
function Read-ExportProperties($Package, $Export, [string]$Path, $Only = $null) {
    $p = [int]$Export.ScriptStart
    $values = Read-Properties $Package ([ref]$p) $true $Only
    if ($p -ne $Export.ScriptEnd) { throw "$Path`: property stream of $($Export.Name) ended at $p, expected $($Export.ScriptEnd)" }
    return $values
}

# UDataTable saves its rows after the tagged properties and the object's GUID flag: a count, then a name and a tagged row struct per row.
# Rows are saved without defaults, so every field is present.
function Read-DataTableRows($Package, $Export, [string]$Path) {
    $b = $Package.Bytes
    $p = [int]$Export.ScriptEnd
    if ([BitConverter]::ToInt32($b, $p) -ne 0) { $p += 16 }
    $p += 4
    $count = [BitConverter]::ToInt32($b, $p)
    $p += 4
    $rows = [ordered]@{}
    for ($i = 0; $i -lt $count; $i++) {
        $name = Read-Name $Package $p
        $p += 8
        $rows[$name] = Read-Properties $Package ([ref]$p) $false
    }
    if ($p -ne $Export.SerialEnd) { throw "$Path`: rows of $($Export.Name) ended at $p, expected $($Export.SerialEnd)" }
    return $rows
}

# ---------- Formatting ----------

function Get-ShortName([string]$Path) {
    $name = $Path.Substring($Path.LastIndexOfAny([char[]]'.:') + 1)
    # Blueprint generated classes read as their asset name.
    if ($Path.StartsWith('/Game/') -and $name.EndsWith('_C')) { $name = $name.Substring(0, $name.Length - 2) }
    return $name
}

# Missing keys read as $null, so nested struct lookups need no guards.
function Get-Field($Value, [string[]]$Path) {
    foreach ($key in $Path) {
        if ($Value -isnot [System.Collections.IDictionary] -or -not $Value.Contains($key)) { return $null }
        $Value = $Value[$key]
    }
    if ($Value -is [array]) { return ,$Value }
    return $Value
}

function Format-Value($Value) {
    if ($null -eq $Value) { return '' }
    if ($Value -is [string]) {
        # The empty FName.
        if ($Value -eq 'None') { return '' }
        if ($Value -match '^/\w+/\S*\.\S+$') { return Get-ShortName $Value }
        if ($Value -match '^\w+::(\w+)$') { return $Matches[1] }
        return ($Value -replace '\s*\r?\n\s*', ' ')
    }
    if ($Value -is [bool]) { if ($Value) { return 'true' } else { return 'false' } }
    # Decimal conversion keeps the float digits identical between .NET Framework and .NET.
    if ($Value -is [single] -or $Value -is [double]) { return ([decimal]$Value).ToString($Invariant) }
    if ($Value -is [System.Collections.IDictionary]) { return Format-Struct $Value }
    if ($Value -is [array]) { return (@(foreach ($item in $Value) { Format-Value $item }) -join ', ') }
    return [string]$Value
}

# Engine structs read by their shape; any other struct lists its non-empty fields.
function Format-Struct($Value) {
    if ($Value.Contains('AttributeName') -and $Value.Contains('AttributeOwner')) { return Format-Value $Value['AttributeName'] }
    if ($Value.Contains('CombinedTags')) { return Format-Value $Value['CombinedTags'] }
    if ($Value.Contains('QueryTokenStream')) {
        if (@($Value['QueryTokenStream']).Count -eq 0) { return '' }
        foreach ($key in @('UserDescription', 'AutoDescription')) { $text = Format-Value $Value[$key]; if ($text) { return $text } }
        return '(GameplayTagQuery)'
    }
    if ($Value.Contains('DataTable') -and $Value.Contains('RowName')) {
        $row = Format-Value $Value['RowName']
        if (-not $row) { return '' }
        return (Format-Value $Value['DataTable']) + '.' + $row
    }
    if ($Value.Contains('MagnitudeCalculationType')) { return Format-Magnitude $Value }
    if ($Value.Contains('ModifierOp') -and $Value.Contains('Attribute')) { return Format-Modifier $Value }
    if ($Value.Contains('Value') -and $Value.Contains('Curve')) { return Format-ScalableFloat $Value }
    # Single-field structs such as FGameplayTag read as their field.
    if ($Value.Count -eq 1) { foreach ($only in $Value.Values) { return Format-Value $only } }
    $parts = @(foreach ($key in $Value.Keys) {
        $text = Format-Value $Value[$key]
        if ($text) { $key + '=' + $text }
    })
    if ($parts.Count -eq 0) { return '' }
    return '{' + ($parts -join ', ') + '}'
}

# FScalableFloat: the value, times a curve row when one is set.
function Format-ScalableFloat($Float) {
    $text = Format-Value (Get-Field $Float 'Value')
    $table = Format-Value (Get-Field $Float 'Curve', 'CurveTable')
    if ($table) { $text += ' × ' + $table + '.' + (Format-Value (Get-Field $Float 'Curve', 'RowName')) }
    return $text
}

# FGameplayEffectModifierMagnitude: only the member its calculation type uses; ScalableFloat is the default type.
function Format-Magnitude($Magnitude) {
    switch (Format-Value (Get-Field $Magnitude 'MagnitudeCalculationType')) {
        'AttributeBased' { return 'AttributeBased(' + (Format-Value (Get-Field $Magnitude 'AttributeBasedMagnitude', 'BackingAttribute')) + ' × ' + (Format-Value (Get-Field $Magnitude 'AttributeBasedMagnitude', 'Coefficient')) + ')' }
        'CustomCalculationClass' { return (Format-Value (Get-Field $Magnitude 'CustomMagnitude', 'CalculationClassMagnitude')) + ' × ' + (Format-Value (Get-Field $Magnitude 'CustomMagnitude', 'Coefficient')) }
        'SetByCaller' {
            $key = Format-Value (Get-Field $Magnitude 'SetByCallerMagnitude', 'DataTag')
            if (-not $key) { $key = Format-Value (Get-Field $Magnitude 'SetByCallerMagnitude', 'DataName') }
            return 'SetByCaller(' + $key + ')'
        }
    }
    return Format-Value (Get-Field $Magnitude 'ScalableFloatMagnitude')
}

# FGameplayModifierInfo reads as "attribute op magnitude", with its tag requirements when set.
function Format-Modifier($Modifier) {
    $text = (Format-Value (Get-Field $Modifier 'Attribute')) + ' ' + (Format-Value (Get-Field $Modifier 'ModifierOp')) + ' ' + (Format-Value (Get-Field $Modifier 'ModifierMagnitude'))
    foreach ($key in @('SourceTags', 'TargetTags')) {
        $tags = Format-Value (Get-Field $Modifier $key)
        if ($tags) { $text += ' ' + $key + '=' + $tags }
    }
    return $text
}

# Saved values outside the dedicated columns; an explicitly saved empty value still overrides the parent.
function Format-Others($Props, [string[]]$Skip) {
    $parts = @(foreach ($key in $Props.Keys) {
        if ($Skip -contains $key) { continue }
        $text = Format-Value $Props[$key]
        if (-not $text) { $text = '없음' }
        $key + ': ' + $text
    })
    return $parts -join '; '
}

function Format-Cell([string]$Text) {
    if (-not $Text) { return '' }
    return $Text.Replace('|', '\|')
}

function Format-Row([string[]]$Cells) {
    return '| ' + (@(foreach ($cell in $Cells) { Format-Cell $cell }) -join ' | ') + ' |'
}

function Format-Link([string]$Text, [string]$RepoPath) {
    return '[' + $Text + '](' + $ArticleToRepo + $RepoPath + ')'
}

function Sort-Ordinal($Items, [scriptblock]$Key) {
    $list = New-Object 'System.Collections.Generic.List[object]'
    foreach ($item in $Items) { $list.Add($item) }
    $list.Sort([Comparison[object]] { param($a, $b) [string]::CompareOrdinal((& $Key $a), (& $Key $b)) })
    return ,$list.ToArray()
}

# ---------- Sources ----------

# EnumerateFiles walks the Content tree about twice as fast as Get-ChildItem on Windows PowerShell.
function Get-FilesUnder([string[]]$Roots, [string]$Pattern) {
    foreach ($root in $Roots) {
        if (Test-Path -LiteralPath $root) { [IO.Directory]::EnumerateFiles($root, $Pattern, [IO.SearchOption]::AllDirectories) }
    }
}

function Get-PluginFolders([string]$Repo, [string]$Name) {
    return @(Get-ChildItem -LiteralPath (Join-Path $Repo 'Plugins') -Directory | ForEach-Object { Join-Path $_.FullName $Name })
}

# UCLASS declarations of every project class: enough to tell effects apart, to give /Script classes their U or A prefix and to link the file holding a class's defaults.
function Read-NativeTypes([string]$Repo) {
    $types = @{}
    $roots = @(Join-Path $Repo 'Source') + (Get-PluginFolders $Repo 'Source')
    # UCLASS specifiers may nest one level of parentheses, as in meta=(...).
    $classPattern = 'UCLASS\((?:[^()]|\((?:[^()]|\([^()]*\))*\))*\)\s*class\s+(?:\w+_API\s+)?(?<name>[UA]\w+)\s*(?:final\s*)?:\s*public\s+(?<parent>[UA]\w+)'
    foreach ($path in (Get-FilesUnder $roots '*.h')) {
        if ($path -match '\\(Intermediate|ThirdParty)\\') { continue }
        foreach ($m in [regex]::Matches([IO.File]::ReadAllText($path), $classPattern)) {
            $types[$m.Groups['name'].Value] = @{ Name = $m.Groups['name'].Value; Parent = $m.Groups['parent'].Value; Source = $path.Substring($Repo.Length + 1).Replace('\', '/') }
        }
    }
    # A class without a constructor keeps its defaults in the header's member initializers.
    foreach ($path in (Get-FilesUnder $roots '*.cpp')) {
        if ($path -match '\\(Intermediate|ThirdParty)\\') { continue }
        foreach ($m in [regex]::Matches([IO.File]::ReadAllText($path), '\b(?<name>[UA]\w+)::\k<name>\s*\(')) {
            $type = $types[$m.Groups['name'].Value]
            if ($type) { $type.Source = $path.Substring($Repo.Length + 1).Replace('\', '/') }
        }
    }
    return $types
}

function Get-DerivedTypes($Types, [string[]]$Roots) {
    return @($Types.Values | Where-Object {
        $name = $_.Name
        while ($name -and $Roots -notcontains $name) { $name = $Types[$name].Parent }
        $null -ne $name
    })
}

# "/Script/WxCombat.WxAbility_Skill" reads as the C++ name UWxAbility_Skill; Blueprint parents read as their asset name.
function Get-ClassDisplay([string]$Path, $Types) {
    $name = Get-ShortName $Path
    if ($Path.StartsWith('/Script/')) {
        $known = $AbilityRoots + $EffectRoots + $EffectPartRoots
        foreach ($prefix in @('U', 'A')) { if ($Types.ContainsKey($prefix + $name) -or $known -contains ($prefix + $name)) { return $prefix + $name } }
    }
    return $name
}

# Project classes link to their constructor file; engine classes and Blueprint parents stay plain names.
function Format-Class([string]$Name, $Types) {
    $type = $Types[$Name]
    if ($type) { return Format-Link $Name $type.Source }
    return $Name
}

function Get-PackagePath([string]$File, [string]$Repo) {
    $relative = $File.Substring($Repo.Length + 1).Replace('\', '/')
    $relative = $relative.Substring(0, $relative.Length - '.uasset'.Length)
    if ($relative.StartsWith('Content/')) { return '/Game/' + $relative.Substring(8) }
    # Plugins/<Name>/Content/... mounts as /<Name>/...
    $parts = $relative.Split('/')
    return '/' + $parts[1] + '/' + (($parts[3..($parts.Length - 1)]) -join '/')
}

# ---------- Assets ----------

function New-AssetRecord([string]$File, [string]$Repo, $Package) {
    $name = [IO.Path]::GetFileNameWithoutExtension($File)
    $record = @{ Name = $name; File = $File.Substring($Repo.Length + 1).Replace('\', '/'); Package = (Get-PackagePath $File $Repo); ParentPath = $null }
    $record.Imports = @(for ($i = 0; $i -lt $Package.Imports.Count; $i++) { Resolve-Index $Package (-$i - 1) })
    $record.RowReferences = @(foreach ($reference in $Package.RowReferences) {
        if ($reference.Index -gt 0 -and $null -eq $Package.Exports) { continue }
        @{ Table = (Resolve-Index $Package $reference.Index); Rows = $reference.Rows }
    })
    if ($null -ne $Package.Exports) {
        $class = Find-Export $Package ($name + '_C')
        if ($class) { $record.ParentPath = Resolve-Index $Package $class.Super }
    }
    return $record
}

function Read-Ability($Record, $Package, $Types) {
    $cdo = Find-Export $Package ('Default__' + $Record.Name + '_C')
    if ($null -eq $Record.ParentPath -or $null -eq $cdo) { Write-Warning "$($Record.File): not a Blueprint, skipped"; return $false }
    $Record.Type = Get-ClassDisplay $Record.ParentPath $Types
    $Record.Props = Read-ExportProperties $Package $cdo $Record.File
    $Record.Sets = New-Object System.Collections.ArrayList
    return $true
}

function Read-AbilitySet($Record, $Package) {
    $asset = Find-Export $Package $Record.Name
    if ($null -eq $asset -or (Resolve-Index $Package $asset.Class) -notmatch '\.WxAbilitySet$') { return $false }
    $Record.Props = Read-ExportProperties $Package $asset $Record.File
    $Record.Users = New-Object System.Collections.ArrayList
    return $true
}

# A GE_ must derive from UGameplayEffect directly, through a project class or through another GE_; other GE_-named Blueprints are skipped.
function Read-Effect($Record, $Package, $Types, $EffectTypes) {
    $cdo = Find-Export $Package ('Default__' + $Record.Name + '_C')
    if ($null -eq $Record.ParentPath -or $null -eq $cdo) { return $false }
    $Record.Type = Get-ClassDisplay $Record.ParentPath $Types
    if ($Record.ParentPath.StartsWith('/Script/') -and $EffectRoots -notcontains $Record.Type -and -not (@($EffectTypes | Where-Object { $_.Name -eq $Record.Type }).Count)) { return $false }
    $Record.Props = Read-ExportProperties $Package $cdo $Record.File
    # GEComponents lists instanced subobjects saved as exports of the same package.
    $Record.Components = @(foreach ($name in @($Record.Props['GEComponents'])) {
        $export = if ($name) { Find-Export $Package $name }
        if ($null -eq $export) { continue }
        @{ Class = (Get-ShortName (Resolve-Index $Package $export.Class)); Props = (Read-ExportProperties $Package $export $Record.File) }
    })
    return $true
}

# The ability system component is a default subobject, so its saved values live in its own export.
function Read-Character($Record, $Package, $Types) {
    foreach ($export in (Get-Exports $Package)) {
        if ((Resolve-Index $Package $export.Class) -notmatch 'AbilitySystemComponent$') { continue }
        $props = Read-ExportProperties $Package $export $Record.File
        if ($props.Contains('AbilitySets')) { $Record.Sets = @($props['AbilitySets']) }
    }
    if ($null -eq $Record.Sets) { return $false }
    $Record.Parent = ''
    if ($Record.ParentPath) { $Record.Parent = Get-ClassDisplay $Record.ParentPath $Types }
    return $true
}

# Montage sections and notifies sort by their start time; ties keep the saved order.
function Sort-ByTime($Items) {
    $index = 0
    $keyed = @(foreach ($item in $Items) {
        @{ Key = ([double](Get-Field $item 'LinkValue')).ToString('000000.000000', $Invariant) + $index.ToString('D5'); Item = $item }
        $index++
    })
    return ,@(foreach ($entry in (Sort-Ordinal $keyed { param($e) $e.Key })) { $entry.Item })
}

# The montage fields the list shows; everything else in a notify event (links, filters, colors) is skipped unread.
$MontageFields = @{ CompositeSections = 1; SectionName = 1; Notifies = 1; Notify = 1; NotifyStateClass = 1; NotifyName = 1; LinkValue = 1 }

# Notifies read as their class in time order, counted, with the distinct saved values of their instances (damage rows, effects, spawned classes).
function Read-Montage($Record) {
    $package = $Record.Data
    $export = Find-Export $package $Record.Name
    if ($null -eq $export) { return $false }
    $props = Read-ExportProperties $package $export $Record.File $MontageFields
    $Record.Sections = @(foreach ($section in (Sort-ByTime @($props['CompositeSections']))) { Format-Value (Get-Field $section 'SectionName') })
    $groups = [ordered]@{}
    foreach ($notify in (Sort-ByTime @($props['Notifies']))) {
        if ($notify -isnot [System.Collections.IDictionary]) { continue }
        $instance = Get-Field $notify 'Notify'
        if (-not $instance) { $instance = Get-Field $notify 'NotifyStateClass' }
        $object = if ($instance) { Find-Export $package $instance }
        # Skeleton notifies have no instance and read as their name.
        $class = Format-Value (Get-Field $notify 'NotifyName')
        $values = $null
        if ($object) {
            $class = Get-ShortName (Resolve-Index $package $object.Class)
            $values = Read-ExportProperties $package $object $Record.File
        }
        if (-not $groups.Contains($class)) { $groups[$class] = @{ Hits = 0; Fields = [ordered]@{} } }
        $group = $groups[$class]
        $group.Hits++
        if ($null -eq $values) { continue }
        foreach ($key in $values.Keys) {
            $text = Format-Value $values[$key]
            # Binary structs such as a spawn offset read only as their type name.
            if (-not $text -or $text -match '^\(\w+\)$') { continue }
            if (-not $group.Fields.Contains($key)) { $group.Fields[$key] = New-Object System.Collections.ArrayList }
            if (-not $group.Fields[$key].Contains($text)) { [void]$group.Fields[$key].Add($text) }
        }
    }
    $Record.Notifies = @(foreach ($class in $groups.Keys) {
        $group = $groups[$class]
        $text = $class
        if ($group.Hits -gt 1) { $text += '×' + $group.Hits }
        $fields = @(foreach ($key in $group.Fields.Keys) { $key + '=' + ($group.Fields[$key] -join ', ') })
        if ($fields.Count) { $text += '{' + ($fields -join '; ') + '}' }
        $text
    })
    return $true
}

# Same-named assets in different folders (ABS_Template, BP_Template) get their folder as a prefix.
function Set-DisplayNames($Records) {
    $counts = @{}
    foreach ($r in $Records) { $counts[$r.Name] = 1 + [int]$counts[$r.Name] }
    foreach ($r in $Records) {
        $r.Display = $r.Name
        if ($counts[$r.Name] -gt 1) { $r.Display = (Split-Path (Split-Path $r.File -Parent) -Leaf) + '/' + $r.Name }
    }
}

# Assets that import each tracked class; "(파생)" marks a GE_ whose parent it is.
function Get-AssetUsage($Records, [string[]]$Keys) {
    $usage = @{}
    foreach ($key in $Keys) { $usage[$key] = New-Object System.Collections.ArrayList }
    foreach ($record in $Records) {
        foreach ($path in $record.Imports) {
            $key = $null
            if ($path -match '^/Script/[^.]+\.(\w+)$') { $key = 'U' + $Matches[1] }
            elseif ($path -match '^(/[^.]+)\.\w+_C$') { $key = $Matches[1] }
            if (-not $key -or -not $usage.ContainsKey($key)) { continue }
            $label = $record.Display
            if ($path -eq $record.ParentPath) { $label += ' (파생)' }
            if (-not $usage[$key].Contains($label)) { [void]$usage[$key].Add($label) }
        }
    }
    foreach ($key in $Keys) { $usage[$key] = (Sort-Ordinal $usage[$key] { param($n) $n }) -join ', ' }
    return $usage
}

# Rows each asset points at, from the engine's row reference record; rows missing from the table are kept so broken references show.
function Get-RowUsage($Records, $Tables) {
    $usage = @{}
    foreach ($table in $Tables) { $usage[$table.Package] = [ordered]@{} }
    foreach ($record in $Records) {
        foreach ($reference in $record.RowReferences) {
            if (-not $reference.Table) { continue }
            $rows = $usage[$reference.Table.Split('.')[0]]
            if ($null -eq $rows) { continue }
            foreach ($row in $reference.Rows) {
                if (-not $rows.Contains($row)) { $rows[$row] = New-Object System.Collections.ArrayList }
                if (-not $rows[$row].Contains($record.Display)) { [void]$rows[$row].Add($record.Display) }
            }
        }
    }
    return $usage
}

# ---------- Articles ----------

# Dates move only when the body changes, so regenerating an unchanged list leaves the file and Git untouched.
function Write-Article([string]$Repo, [string]$Slug, [string]$Title, [string]$Aliases, [string]$Summary, [string]$Body) {
    $path = Join-Path $Repo "$ArticleFolder/$Slug.md"
    $today = Get-Date -Format 'yyyy-MM-dd'
    $old = $null
    $created = $today; $updated = $today; $verified = $today
    if (Test-Path -LiteralPath $path) {
        $old = [IO.File]::ReadAllText($path)
        if ($old -match '(?m)^created: (\S+)') { $created = $Matches[1] }
        if ($old -match '(?m)^updated: (\S+)') { $updated = $Matches[1] }
        if ($old -match '(?m)^verified: (\S+)') { $verified = $Matches[1] }
    }
    $make = {
        param($Updated, $Verified)
        (@(
            '---'
            'title: "' + $Title + '"'
            'category: reference'
            'sources:'
            '  - "raw/notes/2026-09-25-ability-data-on-ga.md"'
            'created: ' + $created
            'updated: ' + $Updated
            'tags: [wx, combat]'
            'aliases: ' + $Aliases
            'confidence: high'
            'volatility: warm'
            'verified: ' + $Verified
            'summary: "' + $Summary + '"'
            '---'
            ''
            ''
        ) -join "`n") + $Body
    }
    if ($old -eq (& $make $updated $verified)) { return "unchanged: $ArticleFolder/$Slug.md" }
    [IO.File]::WriteAllText($path, (& $make $today $today), (New-Object Text.UTF8Encoding($false)))
    return "updated: $ArticleFolder/$Slug.md"
}

function Get-IntroLines([string]$Subject) {
    return @(
        $Subject + ' ' + (Format-Link 'ExportAbilitySystemLists.bat' 'BatchFiles/ExportAbilitySystemLists.bat') + '이 어빌리티·이펙트·캐릭터 목록을 함께 다시 만들고 ' + (Format-Link 'OpenWiki.bat' 'BatchFiles/OpenWiki.bat') + '도 위키를 열 때 다시 만든다. 손으로 고치지 않는다.'
        ''
        '- 에셋은 이름으로 적고 파일은 `Content/**/<이름>.uasset`로 찾는다. 같은 이름이 여러 폴더에 있으면 폴더를 앞에 붙인다.'
    )
}

# Every saved field is a column; rows are saved whole, so an empty cell is an empty value.
function Get-DataTableLines($Table, $RowUsage, [string]$UsageTitle) {
    $fields = New-Object System.Collections.ArrayList
    foreach ($row in $Table.Rows.Values) { foreach ($key in $row.Keys) { if (-not $fields.Contains($key)) { [void]$fields.Add($key) } } }
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add((Format-Row (@('행') + @($fields) + @($UsageTitle))))
    $lines.Add('|' + ('---|' * ($fields.Count + 2)))
    $usage = $RowUsage[$Table.Package]
    foreach ($name in $Table.Rows.Keys) {
        $users = ''
        if ($usage.Contains($name)) { $users = (Sort-Ordinal $usage[$name] { param($n) $n }) -join ', ' }
        $lines.Add((Format-Row (@($name) + @(foreach ($key in $fields) { Format-Value (Get-Field $Table.Rows[$name] $key) }) + @($users))))
    }
    $missing = @(foreach ($name in $usage.Keys) { if (-not $Table.Rows.Contains($name)) { '`' + $name + '` ← ' + ((Sort-Ordinal $usage[$name] { param($n) $n }) -join ', ') } })
    if ($missing.Count) {
        $lines.Add('')
        $lines.Add('표에 없는 행을 가리키는 참조: ' + ($missing -join '; '))
    }
    return $lines
}

function Get-CharacterBody($Characters, $Sets, $Abilities, $AttributeTables, $RowUsage) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('# 캐릭터 목록')
    $lines.Add('')
    foreach ($line in (Get-IntroLines '에디터 없이 캐릭터 BP·ABS_·DT_ 에셋을 읽어 만든 표다.')) { $lines.Add($line) }
    $lines.Add('- 캐릭터는 ASC의 `AbilitySets`를 저장한 BP이고, 이 문서는 그 GAS 구성(WxAbilitySet이 주는 어빌리티·이펙트와 속성 초기값)만 다룬다.')
    $lines.Add('- 어빌리티 상세는 [어빌리티 목록](../references/ability-list.md), 이펙트 상세는 [이펙트 목록](../references/effect-list.md)에 있다.')
    $lines.Add('')

    $lines.Add('## 캐릭터')
    $lines.Add('')
    $lines.Add('| 캐릭터 | 부모 | AbilitySets (부여 순서) |')
    $lines.Add('|---|---|---|')
    foreach ($c in $Characters) {
        $setNames = @(foreach ($path in $c.Sets) { $s = $Sets[$path.Split('.')[0]]; if ($s) { $s.Display } else { Get-ShortName $path } })
        $lines.Add((Format-Row @($c.Display, $c.Parent, ($setNames -join ', '))))
    }
    $lines.Add('')

    $lines.Add('## WxAbilitySet')
    $lines.Add('')
    $lines.Add('같은 입력의 어빌리티가 여럿이면 캐릭터의 AbilitySets 순서, WxAbilitySet 안의 부여 순서대로 시도한다.')
    $lines.Add('')
    $lines.Add('| WxAbilitySet | 캐릭터 | AttributeInitRow | GrantedAbilities (부여 순서) | GrantedEffects |')
    $lines.Add('|---|---|---|---|---|')
    foreach ($s in (Sort-Ordinal $Sets.Values { param($r) $r.File })) {
        $granted = @(foreach ($path in @($s.Props['GrantedAbilities'])) { if ($path) { $a = $Abilities[$path.Split('.')[0]]; if ($a) { $a.Display } else { Get-ShortName $path } } })
        $lines.Add((Format-Row @($s.Display, (@($s.Users) -join ', '), (Format-Value $s.Props['AttributeInitRow']), ($granted -join ', '), (Format-Value $s.Props['GrantedEffects']))))
    }
    $lines.Add('')

    foreach ($table in $AttributeTables) {
        $lines.Add('## 속성 초기값 · ' + $table.Display)
        $lines.Add('')
        foreach ($line in (Get-DataTableLines $table $RowUsage 'WxAbilitySet')) { $lines.Add($line) }
        $lines.Add('')
    }

    $lines.Add('## 관련 문서')
    $lines.Add('')
    $lines.Add('- [[ability-list|어빌리티 목록]] ([어빌리티 목록](../references/ability-list.md))')
    $lines.Add('- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))')
    $lines.Add('- [[effect-list|이펙트 목록]] ([이펙트 목록](../references/effect-list.md))')
    $lines.Add('')
    $lines.Add('## Sources')
    $lines.Add('')
    $lines.Add('- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — WxAbilitySet의 부여 규칙과 데이터 배치')
    return ($lines -join "`n") + "`n"
}

function Get-AbilityBody($Abilities, $Montages, $Types) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('# 어빌리티 목록')
    $lines.Add('')
    foreach ($line in (Get-IntroLines '에디터 없이 GA_·AM_ 에셋을 읽어 만든 표다.')) { $lines.Add($line) }
    $lines.Add('- 값은 에셋에 저장된 값이다. 빈 칸은 "없음"이 아니라 저장된 값이 없어 부모 기본값을 따른다는 뜻이다. 기본값은 타입 칸의 C++ 클래스와 그 상위 클래스의 생성자·헤더 초기값에 있고, 타입 칸 링크가 그 클래스의 생성자 파일이다.')
    $lines.Add('- 발동 조건의 Required·Blocked는 `ActivationRequiredTags`·`ActivationBlockedTags`다. `ActivationOwnedTags`는 AbilityTags와 같으면 적지 않는다.')
    $lines.Add('- WxAbilitySet 칸의 에셋을 받는 캐릭터와 그 구성은 [캐릭터 목록](../references/character-list.md)에 있다.')
    $lines.Add('')

    # Properties that have their own column; everything else goes to 기타 in serialized order.
    $columnProps = @('ActivationInputAction', 'AbilityMontage', 'AbilityTags', 'ActivationRequiredTags', 'ActivationBlockedTags', 'CooldownTags', 'CooldownTime', 'MaxRecharges', 'CooldownGameplayEffectClass', 'CostResource', 'CostAmount', 'Title', 'Description')
    $lines.Add('## 어빌리티')
    $group = $null
    foreach ($a in (Sort-Ordinal $Abilities.Values { param($r) $r.File })) {
        # Abilities are grouped by the folder that owns their Abilities folder, e.g. Content/Character/HGTest.
        $folder = $a.File.Substring(0, $a.File.LastIndexOf('/'))
        $at = $folder.IndexOf('/Abilities')
        if ($at -ge 0) { $folder = $folder.Substring(0, $at) }
        if ($folder -ne $group) {
            $group = $folder
            $lines.Add('')
            $lines.Add('### ' + $folder)
            $lines.Add('')
            $lines.Add('| 어빌리티 | 타입 | WxAbilitySet | 입력 | 몽타주 | AbilityTags | 발동 조건 | 쿨다운 | 비용 | 설명 | 기타 |')
            $lines.Add('|---|---|---|---|---|---|---|---|---|---|---|')
        }
        $p = $a.Props
        $conditions = @()
        if ($p.Contains('ActivationRequiredTags')) { $conditions += 'Required: ' + (Format-Value $p['ActivationRequiredTags']) }
        if ($p.Contains('ActivationBlockedTags')) { $conditions += 'Blocked: ' + (Format-Value $p['ActivationBlockedTags']) }
        $cooldown = @()
        if ($p.Contains('CooldownTags')) { $cooldown += Format-Value $p['CooldownTags'] }
        if ($p.Contains('CooldownTime')) { $cooldown += (Format-Value $p['CooldownTime']) + '초' }
        if ($p.Contains('MaxRecharges')) { $cooldown += '충전 ' + (Format-Value $p['MaxRecharges']) }
        if ($p.Contains('CooldownGameplayEffectClass')) { $cooldown += Format-Value $p['CooldownGameplayEffectClass'] }
        $cost = @()
        if ($p.Contains('CostResource')) { $cost += Format-Value $p['CostResource'] }
        if ($p.Contains('CostAmount')) { $cost += Format-Value $p['CostAmount'] }
        $text = @()
        if ($p.Contains('Title')) { $text += Format-Value $p['Title'] }
        if ($p.Contains('Description')) { $text += Format-Value $p['Description'] }
        $skip = $columnProps
        if ($p.Contains('ActivationOwnedTags') -and $p.Contains('AbilityTags') -and (Format-Value $p['ActivationOwnedTags']) -eq (Format-Value $p['AbilityTags'])) { $skip += 'ActivationOwnedTags' }
        $setCell = '—'
        if ($a.Sets.Count) { $setCell = @($a.Sets) -join ', ' }
        $lines.Add((Format-Row @(
            $a.Display, (Format-Class $a.Type $Types), $setCell, (Format-Value $p['ActivationInputAction']), (Format-Value $p['AbilityMontage']),
            (Format-Value $p['AbilityTags']), ($conditions -join '; '), ($cooldown -join ', '), ($cost -join ' '), ($text -join ' — '), (Format-Others $p $skip))))
    }
    $lines.Add('')

    $lines.Add('## 몽타주')
    $lines.Add('')
    $lines.Add('GA_가 쓰는 몽타주와 그 몽타주가 참조하는 몽타주다. 섹션은 시작 시각 순이다. 노티파이는 클래스별로 묶어 처음 나오는 시각 순으로 적고 `×N`은 개수다. `{}` 안은 그 클래스 인스턴스들에 저장된 값을 필드별로 중복 없이 모은 것이라, 어느 인스턴스·섹션의 값인지와 모든 인스턴스가 그 값을 갖는지는 나타내지 않는다. 벡터·회전·트랜스폼 같은 수학 구조체 값(예: `LocalSpawnOffset`)은 적지 않는다. 피해 행 값은 [이펙트 목록](../references/effect-list.md)에 있다.')
    $lines.Add('')
    $lines.Add('| 몽타주 | 쓰는 곳 | 섹션 | 노티파이 |')
    $lines.Add('|---|---|---|---|')
    foreach ($m in (Sort-Ordinal $Montages { param($r) $r.File })) {
        $lines.Add((Format-Row @($m.Display, (@($m.Users) -join ', '), ($m.Sections -join ', '), ($m.Notifies -join '; '))))
    }
    $lines.Add('')

    $lines.Add('## 관련 문서')
    $lines.Add('')
    $lines.Add('- [[character-list|캐릭터 목록]] ([캐릭터 목록](../references/character-list.md))')
    $lines.Add('- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))')
    $lines.Add('- [[effect-list|이펙트 목록]] ([이펙트 목록](../references/effect-list.md))')
    $lines.Add('')
    $lines.Add('## Sources')
    $lines.Add('')
    $lines.Add('- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GA_가 가진 데이터와 배치 규칙')
    return ($lines -join "`n") + "`n"
}

function Get-EffectBody($Effects, $NativeClasses, $DamageTables, $RowUsage, $AssetUsage, $Types) {
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('# 이펙트 목록')
    $lines.Add('')
    foreach ($line in (Get-IntroLines '에디터 없이 GE_·DT_ 에셋과 그 참조를 읽어 만든 표다.')) { $lines.Add($line) }
    $lines.Add('- C++ 이펙트·컴포넌트·계산 클래스의 정의는 코드에서 본다. 이 문서는 에셋에 저장된 값과 에셋의 참조만 적는다.')
    $lines.Add('- 에셋 사용처는 이름이 `GA_`·`ABS_`·`GE_`·`AM_`·`BT_`·`ST_`·`DA_`·`DT_`·`BP_`로 시작하는 에셋의 참조에서 찾는다. `(파생)`은 그 클래스를 부모로 둔 GE_다.')
    $lines.Add('')

    $lines.Add('## GE_ 에셋')
    $lines.Add('')
    $lines.Add('값은 에셋에 저장된 값이다. 빈 칸은 "없음"이 아니라 저장된 값이 없어 부모 기본값을 따른다는 뜻이고, `없음`은 부모 값을 빈 값으로 덮어쓴 것이다. 값 없이 이름만 적힌 컴포넌트도 부모 기본값을 쓴다. 부모가 C++ 클래스면 기본값은 그 클래스와 상위 클래스의 생성자·헤더 초기값에 있고, 부모 칸 링크가 그 클래스의 생성자 파일이다. 모디파이어는 `어트리뷰트 연산 크기`로 적는다.')
    $lines.Add('')
    $lines.Add('| 이펙트 | 부모 | Modifiers | GEComponents | 기타 | 에셋 사용처 |')
    $lines.Add('|---|---|---|---|---|---|')
    foreach ($e in (Sort-Ordinal $Effects.Values { param($r) $r.File })) {
        $components = @(foreach ($component in $e.Components) {
            $fields = @(foreach ($key in $component.Props.Keys) {
                $text = Format-Value $component.Props[$key]
                if (-not $text) { $text = '없음' }
                $key + '=' + $text
            })
            if ($fields.Count) { $component.Class + '{' + ($fields -join ', ') + '}' } else { $component.Class }
        })
        $lines.Add((Format-Row @($e.Display, (Format-Class $e.Type $Types), (Format-Value $e.Props['Modifiers']), ($components -join '; '), (Format-Others $e.Props (@('Modifiers', 'GEComponents', 'DataVersion') + $DeprecatedEffectProps)), $AssetUsage[$e.Package])))
    }
    $lines.Add('')

    $lines.Add('## C++ 클래스의 에셋 사용처')
    $lines.Add('')
    $lines.Add('`UGameplayEffect`·`UGameplayEffectComponent`·`UGameplayEffectUIData`·`UGameplayModMagnitudeCalculation`·`UGameplayEffectExecutionCalculation`을 상속한 프로젝트 C++ 클래스 중 에셋이 참조하는 것만 적는다.')
    $lines.Add('')
    $lines.Add('| 클래스 | 에셋 사용처 |')
    $lines.Add('|---|---|')
    foreach ($t in (Sort-Ordinal $NativeClasses { param($r) $r.Name })) {
        if ($AssetUsage[$t.Name]) { $lines.Add((Format-Row @($t.Name, $AssetUsage[$t.Name]))) }
    }
    $lines.Add('')

    foreach ($table in $DamageTables) {
        $lines.Add('## 피해 행 · ' + $table.Display)
        $lines.Add('')
        $lines.Add('`UWxEffect_Damage`를 만들 때 쓰는 행이다. 사용처는 그 행을 가리키는 에셋(엔진의 행 참조 기록)이다.')
        $lines.Add('')
        foreach ($line in (Get-DataTableLines $table $RowUsage '사용처')) { $lines.Add($line) }
        $lines.Add('')
    }

    $lines.Add('## 관련 문서')
    $lines.Add('')
    $lines.Add('- [[ability-list|어빌리티 목록]] ([어빌리티 목록](../references/ability-list.md))')
    $lines.Add('- [[character-list|캐릭터 목록]] ([캐릭터 목록](../references/character-list.md))')
    $lines.Add('- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))')
    $lines.Add('')
    $lines.Add('## Sources')
    $lines.Add('')
    $lines.Add('- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — GE_가 가진 데이터와 배치 규칙')
    return ($lines -join "`n") + "`n"
}

# ---------- Main ----------

try {
    if (-not $RepoRoot) { $RepoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent }
    $repo = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    if (-not (Test-Path -LiteralPath (Join-Path $repo 'Wx.uproject'))) { throw "Wx.uproject not found in $repo" }

    $types = Read-NativeTypes $repo
    $effectTypes = @(Get-DerivedTypes $types $EffectRoots)
    $partTypes = @(Get-DerivedTypes $types $EffectPartRoots)

    $files = @(Get-FilesUnder (@(Join-Path $repo 'Content') + (Get-PluginFolders $repo 'Content')) '*.uasset' | Where-Object { [IO.Path]::GetFileName($_) -match $PackagePattern })
    $abilities = @{}
    $sets = @{}
    $effects = @{}
    $montages = @{}
    $tables = @{}
    $characters = New-Object System.Collections.ArrayList
    $records = New-Object System.Collections.ArrayList

    # Small asset kinds are always read; what they reference decides which other packages are worth parsing.
    $alwaysRead = '^(GA|ABS|GE|AM|DT)_'
    foreach ($file in $files) {
        $name = [IO.Path]::GetFileName($file)
        if ($name -notmatch $alwaysRead) { continue }
        try {
            $package = Read-Package ([IO.File]::ReadAllBytes($file))
            $record = New-AssetRecord $file $repo $package
            if ($name.StartsWith('GA_')) {
                if (-not (Read-Ability $record $package $types)) { continue }
                $abilities[$record.Package] = $record
            } elseif ($name.StartsWith('ABS_')) {
                if (-not (Read-AbilitySet $record $package)) { continue }
                $sets[$record.Package] = $record
            } elseif ($name.StartsWith('GE_')) {
                if (-not (Read-Effect $record $package $types $effectTypes)) { continue }
                $effects[$record.Package] = $record
            } elseif ($name.StartsWith('AM_')) {
                $record.Data = $package
                $montages[$record.Package] = $record
            } else {
                $record.Data = $package
                $tables[$record.Package] = $record
            }
            [void]$records.Add($record)
        } catch {
            throw "$($file.Substring($repo.Length + 1).Replace('\', '/')): $($_.Exception.Message)"
        }
    }
    foreach ($effect in @($effects.Values)) {
        if ($effect.ParentPath.StartsWith('/Game/') -and -not $effects.ContainsKey($effect.ParentPath.Split('.')[0])) { $effects.Remove($effect.Package); [void]$records.Remove($effect) }
    }

    # Damage tables are recognised by their row struct, attribute tables by the sets that point at them.
    $damageTables = @(foreach ($table in $tables.Values) { if (@($table.Imports | Where-Object { $_ -match '\.WxDamageTableRow$' }).Count) { $table } })
    $attributeByPackage = @{}
    foreach ($set in $sets.Values) {
        $path = Get-Field $set.Props 'AttributeInitRow', 'DataTable'
        if ($path -and $tables.ContainsKey($path.Split('.')[0])) { $attributeByPackage[$path.Split('.')[0]] = $tables[$path.Split('.')[0]] }
    }
    $attributeTables = @($attributeByPackage.Values)
    foreach ($table in ($damageTables + $attributeTables)) {
        if ($table.Rows) { continue }
        try { $table.Rows = Read-DataTableRows $table.Data (Find-Export $table.Data $table.Name) $table.File } catch { throw "$($table.File): $($_.Exception.Message)" }
    }

    # Other packages are parsed only when their bytes mention a tracked class, a damage table or AbilitySets; most BP_ packages are unrelated and large.
    $mentioned = @(foreach ($type in ($effectTypes + $partTypes)) { $type.Name.Substring(1) }) + @(foreach ($effect in $effects.Values) { $effect.Name }) + @(foreach ($table in $damageTables) { $table.Name }) + 'AbilitySets'
    $mentions = New-Object regex (@(foreach ($word in $mentioned) { [regex]::Escape($word) }) -join '|')
    foreach ($file in $files) {
        if ([IO.Path]::GetFileName($file) -match $alwaysRead) { continue }
        try {
            $bytes = [IO.File]::ReadAllBytes($file)
            if (-not $mentions.IsMatch($Latin1.GetString($bytes))) { continue }
            $package = Read-Package $bytes
            $record = New-AssetRecord $file $repo $package
            if ($record.Name.StartsWith('BP_') -and $package.Names -contains 'AbilitySets' -and (Read-Character $record $package $types)) { [void]$characters.Add($record) }
            [void]$records.Add($record)
        } catch {
            throw "$($file.Substring($repo.Length + 1).Replace('\', '/')): $($_.Exception.Message)"
        }
    }

    Set-DisplayNames $records
    $characterList = Sort-Ordinal $characters { param($r) $r.File }
    foreach ($character in $characterList) {
        foreach ($path in $character.Sets) {
            $set = $sets[$path.Split('.')[0]]
            if ($set) { [void]$set.Users.Add($character.Display) }
        }
    }
    foreach ($set in (Sort-Ordinal $sets.Values { param($r) $r.File })) {
        foreach ($path in @($set.Props['GrantedAbilities'])) {
            if (-not $path) { continue }
            $ability = $abilities[$path.Split('.')[0]]
            if ($ability) { [void]$ability.Sets.Add($set.Display) }
        }
    }

    # Montages used by abilities, plus montages those montages reference (a finisher's victim montage).
    $pending = New-Object System.Collections.Queue
    foreach ($ability in (Sort-Ordinal $abilities.Values { param($r) $r.File })) {
        $path = Get-Field $ability.Props 'AbilityMontage'
        if (-not $path) { continue }
        $montage = $montages[$path.Split('.')[0]]
        if ($null -eq $montage) { continue }
        if ($null -eq $montage.Users) { $montage.Users = New-Object System.Collections.ArrayList; $pending.Enqueue($montage) }
        [void]$montage.Users.Add($ability.Display)
    }
    $usedMontages = New-Object System.Collections.ArrayList
    while ($pending.Count) {
        $montage = $pending.Dequeue()
        if (-not (Read-Montage $montage)) { continue }
        [void]$usedMontages.Add($montage)
        foreach ($path in $montage.Imports) {
            if ($path -notmatch '^(/[^.]+)\.AM_\w+$' -or $path.Split('.')[0] -eq $montage.Package) { continue }
            $linked = $montages[$Matches[1]]
            if ($null -eq $linked) { continue }
            if ($null -eq $linked.Users) { $linked.Users = New-Object System.Collections.ArrayList; $pending.Enqueue($linked) }
            if (-not $linked.Users.Contains($montage.Display)) { [void]$linked.Users.Add($montage.Display) }
        }
    }

    $assetUsage = Get-AssetUsage $records (@(foreach ($type in ($effectTypes + $partTypes)) { $type.Name }) + @($effects.Keys))
    $rowUsage = Get-RowUsage $records ($damageTables + $attributeTables)

    $abilityBody = Get-AbilityBody $abilities $usedMontages $types
    $characterBody = Get-CharacterBody $characterList $sets $abilities (Sort-Ordinal $attributeTables { param($r) $r.File }) $rowUsage
    $effectBody = Get-EffectBody $effects ($effectTypes + $partTypes) (Sort-Ordinal $damageTables { param($r) $r.File }) $rowUsage $assetUsage $types
    Write-Output (Write-Article $repo 'ability-list' '어빌리티 목록' '["GA_ 목록", "몽타주 노티파이 목록"]' 'GA_·AM_ 에셋에서 생성한 표로, 어빌리티마다 타입·WxAbilitySet·입력·몽타주·태그·쿨다운·비용과 몽타주 섹션·노티파이를 보인다.' $abilityBody)
    Write-Output (Write-Article $repo 'character-list' '캐릭터 목록' '["WxAbilitySet 목록", "캐릭터 속성 초기값"]' 'AbilitySets를 가진 캐릭터 BP의 GAS 구성으로, 캐릭터별 WxAbilitySet과 각 WxAbilitySet이 주는 어빌리티·이펙트, 속성 초기값을 보인다.' $characterBody)
    Write-Output (Write-Article $repo 'effect-list' '이펙트 목록' '["GE_ 목록", "GameplayEffect 목록", "DT_Damage 행 목록"]' 'GE_ 에셋의 모디파이어·컴포넌트 저장값, 에셋이 참조하는 C++ 이펙트·컴포넌트·계산 클래스, 피해 행(DT_Damage) 값을 에셋 사용처와 함께 보인다.' $effectBody)
    Write-Output ("$($abilities.Count) abilities, $($sets.Count) sets, $($characters.Count) characters, $($usedMontages.Count) montages, $($effectTypes.Count) C++ effects, $($effects.Count) GE_ assets, $($damageTables.Count) damage tables")
    exit 0
} catch {
    Write-Output "ERROR: $($_.Exception.Message)"
    exit 1
}
