# UE5 Build Failure Reference

Use this file only when classifying Unreal Engine 5 C++ build failures.

## 1) UHT / reflection / generated code

High-signal signatures:
- `UnrealHeaderTool failed`
- `Expected an include at the top of the header: '#include "X.generated.h"'`
- `#include found after .generated.h file`
- reflected type macros missing or malformed

Likely cause:
- Header reflection layout is invalid.
- `.generated.h` is missing or not last.
- `UCLASS`, `USTRUCT`, `UENUM`, `UPROPERTY`, `UFUNCTION`, `GENERATED_BODY()` usage is invalid.

Good immediate fixes:
- Make `.generated.h` the last include in the header.
- Verify matching reflection macros and class/struct declarations.
- Regenerate project files if generated code is stale.

## 2) C/C++ compiler errors

High-signal signatures:
- `error C2065`
- `error C2143`
- `error C2664`
- `error C2672`
- `fatal error C1083`

Likely cause:
- Missing include.
- Wrong type or template usage.
- API changed in engine/plugin version.
- Namespace or forward declaration misuse.

Good immediate fixes:
- Fix the first compiler error first.
- Check recent API changes for the engine/plugin version.
- Replace vague type assumptions with explicit includes or correct namespaces.

## 3) Missing module or include dependency

High-signal signatures:
- `Cannot open include file`
- unresolved types/functions clearly owned by another UE module
- compile succeeds for some files but symbols from one module consistently fail

Likely cause:
- `*.Build.cs` is missing a dependency.
- Header visibility mismatch: private/public include path issue.
- Plugin module is disabled or not loaded for the target.

Good immediate fixes:
- Add the module to `PublicDependencyModuleNames` or `PrivateDependencyModuleNames`.
- Rebuild after regenerating project files.
- Confirm the plugin/module is enabled for the target platform/configuration.

## 4) Linker errors

High-signal signatures:
- `LNK2001`
- `LNK2019`
- `LNK1120`

Likely cause:
- Declaration exists but definition is missing.
- Function signature does not match exactly.
- Symbol is conditionally excluded by macros.
- Required module/library is not linked.

Good immediate fixes:
- Match declaration and definition exactly.
- Confirm the `.cpp` implementing the symbol is compiled into the target.
- Check missing module/library dependencies in `Build.cs`.

## 5) Target / plugin / configuration mismatch

High-signal signatures:
- module/plugin only failing on `Editor` or only on `Game`
- target not found
- plugin unsupported for the chosen platform/config

Likely cause:
- Wrong target name.
- Plugin/module rules exclude the requested target or platform.
- Build is using the wrong configuration for the intended check.

Good immediate fixes:
- Use the correct target, usually `<ProjectName>Editor` for C++ iteration.
- Inspect `.Target.cs`, `.uplugin`, and module rules.
- Confirm the plugin supports Win64 and the chosen target.

## 6) Stale intermediates / hot reload / generated artifacts

High-signal signatures:
- build errors reference old generated paths or removed symbols
- errors disappear after cleaning
- repeated failures after hot reload/live coding changes

Likely cause:
- `Binaries` / `Intermediate` / generated files are stale.

Good immediate fixes:
- Close the editor.
- Delete `Binaries` and `Intermediate`.
- Regenerate project files.
- Rebuild from a clean state.
