---
name: build-doctor
description: 프로젝트 빌드 에러를 진단하고 해결법을 제시합니다. 사용자가 원한다면 로컬에서 임시 수정합니다.
---

Build UE5 C++ projects and explain failures in Korean.

## Workflow

1. Build with `BuildProjectFiles.bat`.
2. Save the full build output to a log file when possible.
3. If the build fails, inspect the log directly and identify the earliest high-signal failure first.
4. Produce a short Korean report with:
   - 1-3 line cause summary
   - quoted log lines
   - immediate fixes to try now

## Build command rules

Use the build command defined in CLAUDE.md. Preserve the raw command in the response so the user can rerun it.

## Diagnosis rules

Always distinguish between the **first causal error** and downstream noise.

Prioritize errors in this order:
1. `UnrealHeaderTool` / generated code failures
2. C/C++ compiler errors (`error Cxxxx`, `fatal error Cxxxx`, syntax/type issues)
3. include/path/module definition errors (`cannot open include file`, missing module dependency)
4. linker errors (`LNK2001`, `LNK2019`, `LNK1120`)
5. target/plugin/configuration mismatches
6. stale intermediates/hot reload artifacts

Use `references/common-ue5-build-failures.md` for mapping signatures to likely causes.

Important:
- Do not summarize every error line.
- Find the **earliest high-signal failure** and treat later errors as consequences unless the log clearly shows multiple unrelated failures.
- Prefer concrete causes like "`Build.cs` missing dependency on `GameplayTags`" over vague causes like "module problem".
- If the evidence is insufficient, say it is a best-effort diagnosis and list 2-3 plausible causes in priority order.

## Required output format

Always answer in Korean using this structure:

```markdown
## 빌드 결과
- 상태: 성공 | 실패

## 원인 요약
- <1-3줄 요약>

## 근거 로그
> <most relevant log line 1>
> <most relevant log line 2>
> <optional line 3>

## 수정 방법
1. <highest-confidence action>
2. <next action>
3. <optional validation step>
```

## Code modification rules

- After diagnosis, **do not modify code automatically**. Present the suggested fix and wait for user confirmation before applying any changes.

## What not to do

- Do not bury the answer in generic Unreal advice.
- Do not quote dozens of lines when 2-3 lines are enough.
- Do not ignore the first causal error in favor of the final summary line.
