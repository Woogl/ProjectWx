# UE5 Build Failure Reference

Use this file only when classifying Unreal Engine 5 C++ build failures.

## 0) Runner / UnrealBuildTool startup

High-signal signatures:
- `BUILD_DOCTOR_RESULT=preflight-failure`
- `BUILD_DOCTOR_EXIT_CODE=2`
- `BUILD_DOCTOR_ERROR=`
- `Access to the path ... is denied`
- `BUILD_DOCTOR_EXIT_CODE=-532462766`
- output stops immediately after `Running UnrealBuildTool`

Likely cause:
- The project root is wrong or contains zero/multiple `.uproject` files.
- `LauncherInstalled.dat` has no `UE_5.8` installation entry.
- `Build.bat` or the build log directory cannot be accessed.
- UnrealBuildTool cannot create `%LOCALAPPDATA%/UnrealBuildTool/Log.txt`, `Trace.uba`, or its mutex. The CLR exception exit code `-532462766` (`0xE0434352`) can be the only visible symptom.

Good immediate fixes:
- Fix the failing preflight path instead of editing project source.
- Keep build-doctor logs under `<project>/Saved/Logs/BuildDoctor`.
- In Codex desktop, run the build with approved sandbox escalation so UnrealBuildTool can write its user-local files.
- Retry once only when the first UnrealBuildTool process ended before producing a log; repeated failure needs permission or process-lock diagnosis.

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
- `UnrealEditor.exe` is holding the output DLL open when `LNK1104` names an `UnrealEditor-*.dll` or UBA reports that the file is used by another process.

Good immediate fixes:
- Match declaration and definition exactly.
- Confirm the `.cpp` implementing the symbol is compiled into the target.
- Check missing module/library dependencies in `Build.cs`.
- For an editor-owned DLL, close the editor and rebuild or use the `run-editor` skill.

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
