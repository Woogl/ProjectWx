---
name: dump-assets
description: 프로젝트 에셋(.uasset)을 JSON 텍스트로 덤프해 .claude/asset_dump/에 기록한다. 에디터 없이 헤드리스 커맨드릿으로 전체 재덤프하므로 언제든 다시 돌릴 수 있다.
argument-hint: "[all|에셋명...]"
user-invocable: true
allowed-tools: PowerShell, Bash, Read, Glob, Grep, Write
---

# 에셋 텍스트 덤프

프로젝트 자체 에셋(마켓플레이스 제외, ~315개)을 JSON으로 덤프해 `.claude/asset_dump/`에 기록한다. 이후 세션은 에디터·MCP 없이 grep/Read로 DataTable 행, DataAsset 값, StateTree 구조, BP/WBP 구조를 파악한다. 덤프 본체는 이 폴더의 `dump_assets.py`이고, 이 스킬은 실행·검증·보고를 오케스트레이션한다.

인자: 없거나 `all`이면 전체 재생성(증분 없음). 에셋명(또는 `/Game/...` 경로, 쉼표 구분 가능)을 주면 **그 에셋의 JSON 파일만 교체**한다 — `README.md`(신선도 기록)는 전체 실행에서만 재작성되므로, stale 판정의 기준은 언제나 마지막 전체 실행이다.

---

## 1. stale 확인 (실행 전)

`.claude/asset_dump/README.md` 끝의 provenance 라인에서 첫 백틱 스팬(`[0-9a-f]{7,40}`)을 `<sha>`로 읽는다(readme-writer와 동일 규칙). 아래 두 명령의 출력 중 `.uasset`/`.umap`만 남긴다:

- `git diff --name-only <sha> HEAD -- Content Plugins/WxUI/Content Plugins/WxWorld/Content`
- `git status --porcelain -- Content Plugins/WxUI/Content Plugins/WxWorld/Content`

변경이 없고 사용자가 강제하지 않았다면 "덤프가 최신"이라 보고하고 끝낸다. README.md가 없으면 첫 실행이다. SHA를 못 찾으면(`fatal: bad object`) stale로 간주하고 진행한다.

## 2. 실행

아래 PowerShell로 엔진을 해석하고 커맨드릿을 실행한다. `<인자>`에는 `--sha=`(현재 `git rev-parse --short HEAD`)와 `--date=`(오늘)를 항상 넣고, 에셋 지정 시 `--asset=<에셋명,...>`을 덧붙인다.

```powershell
# 엔진 해석 (run-editor와 동일한 3단 폴백)
$projPath = "C:\Wx\Wx.uproject"
$assoc = (Get-Content $projPath -Raw | ConvertFrom-Json).EngineAssociation
$engine = $null
try { $engine = (Get-ItemProperty "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc" -ErrorAction Stop).InstalledDirectory } catch {}
if (-not $engine) { try { $engine = (Get-ItemProperty "HKCU:\Software\Epic Games\Unreal Engine\Builds" -ErrorAction Stop).$assoc } catch {} }
if (-not $engine -and $assoc -match '^[\d.]+$') { $engine = "C:\Program Files\Epic Games\UE_$assoc" }
if (-not $engine -or -not (Test-Path $engine)) { throw "엔진 경로를 찾을 수 없습니다 (EngineAssociation=$assoc)." }

$cmd = Join-Path $engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
& $cmd $projPath -run=pythonscript `
  -script="C:/Wx/.claude/skills/dump-assets/dump_assets.py <인자>" `
  -EnablePlugins=PythonScriptPlugin -DisablePlugins=ModelContextProtocol `
  -unattended -nosplash -nullrhi -nosound -stdout -FullStdOutLogOutput
```

- `-EnablePlugins=PythonScriptPlugin`: 프로젝트 baseline은 이 플러그인 OFF다 — uproject를 바꾸지 말고 플래그로 켠다.
- `-DisablePlugins=ModelContextProtocol`: 커맨드릿이 MCP 포트(8000)를 물지 않게 한다. 이게 없으면 바인드 실패가 엔진 오류로 찍혀 exit code가 1이 된다.
- 에디터가 떠 있어도 실행 가능하지만, 에디터에서 저장 안 한 변경은 덤프에 반영되지 않는다고 사용자에게 알린다.
- 출력이 길므로 백그라운드 실행 후 완료를 기다린다(전체 덤프 기준 엔진 부팅 포함 1~2분).

**성공 판정은 exit code가 아니라 로그다**: `DUMP: DONE files=<N> errors=0`과 `Python script executed successfully`를 확인한다. 무관한 엔진 오류 한 줄만 있어도 exit code는 1이 된다.

**실패 시**: 로그에 게임 모듈 로드 실패(`GetLastError=126` 등)가 보이면 스테일 DLL이다 — WxEditor(Development)를 빌드해 최신화 후 1회 재시도한다(빌드 커맨드는 `build-doctor` 참조). 그래도 실패하면 `UnrealEditor.exe <uproject> -EnablePlugins=PythonScriptPlugin -ExecutePythonScript="<스크립트> <인자>"`(에디터 기동 실행)로 폴백하고, 최후에는 에디터 Output Log에서 `py "<스크립트 경로>"` 수동 실행을 안내한다.

## 3. 검증·보고

1. 로그의 카테고리별 개수와 실제 생성 파일 수(`.claude/asset_dump/<카테고리>/*.json`)를 대조한다.
2. `errors=N`이 0이 아니면 해당 `DUMP:` 오류 라인을 그대로 보고한다.
3. 마무리 보고는 짧게: 카테고리별 개수, 오류 유무, provenance 갱신 여부.

산출물 규약(스크립트가 보장, 참고용): 에셋당 JSON 1파일(`{"asset","class","data"}` 봉투), 키 정렬·LF 고정으로 재실행 diff 0, DataAsset·CDO 프로퍼티 키는 camelCase, DataTable만 저작 이름 그대로. protected 프로퍼티(WidgetTree, ParentClass)는 스크립트가 ObjectIterator·AssetRegistry 태그로 우회한다.

## 참고

- **이 폴더를 gitignore에 넣으면 안 된다.** Grep 도구는 ripgrep 기반이라 무시된 경로를 경로 지정으로도 못 읽는다 — 넣는 순간 덤프 전체가 검색에서 조용히 사라진다. 재생성 가능한 산출물이지만 추적 상태로 커밋해 둔다.
- 몽타주(AM_)·BehaviorTree·레벨·아트 에셋은 본문 덤프가 없다 — 에셋의 존재·경로는 `Content/`의 `.uasset`이 원본(SSOT)이므로 Glob으로 직접 찾는다. 그래프 노드 수준(핀 연결)·MVVM 바인딩도 아직 없다 — 필요하면 라이브 에디터 + unreal-mcp로 개별 조회한다.
- 새 마켓플레이스 팩을 들이면 `dump_assets.py`의 `EXCLUDED_TOP`에 폴더명을 추가한다.
- 플러그인 Content 루트를 새로 만들면 `ROOTS`에 마운트 경로(`/<플러그인명>`)를 추가한다.
