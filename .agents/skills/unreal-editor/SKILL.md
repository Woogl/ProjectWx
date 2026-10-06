---
name: unreal-editor
description: 실행 중인 언리얼 에디터를 unreal-mcp로 다룰 때(에셋 조회·편집·저장, StateTree·블루프린트·위젯 편집, 레벨 배치, PIE·에디터 UI 조작)와 에셋 재저장·리다이렉트·개명 같은 에셋 이전을 할 때 쓴다. 연결 방법, 저장 확인, 도구별 함정을 담는다.
---

# 언리얼 에디터 작업

에셋은 텍스트가 아니라서 실행 중인 에디터의 MCP(unreal-mcp)로 하나씩 조회·편집한다.
도구는 대부분 실패해도 `true`를 돌려주거나 값을 조용히 바꾼다. 쓰고 나면 다시 읽어 확인하는 것이 이 문서 전체의 기본 규칙이다.

## 연결

- 에디터가 필요하면 `.agents/scripts/Start-WxEditorMcp.ps1`로 띄운다. 빈 포트를 골라 이 프로젝트 에디터를 띄우고 `WxToolset` 툴이 등록될 때까지 기다린 뒤 `WX_MCP_PORT`·`WX_MCP_PID`를 출력한다. 다른 에디터는 건드리지 않는다. `-List`는 이미 떠 있는 MCP 서버와 그 주인 프로세스만 보여 준다. 코드를 고쳤다면 먼저 `build-doctor`로 빌드한다.
- 세션 도구(`mcp__unreal-mcp__*`)는 세션 시작 때 떠 있던 에디터의 `127.0.0.1:8000/mcp`에만 붙는다. 그 밖에는 `.agents/scripts/Invoke-UnrealMcp.ps1 -Port N`으로 부른다.
  - `-Tool list_toolsets`, `-Tool describe_toolset -Arguments '{"toolset_name":"..."}'`, `-Toolset <툴셋> -Tool <툴> -Arguments '<JSON>'` 세 형태다. `-Tool`은 툴셋 접두사를 뺀 이름이다.
  - 다른 셸(Bash 등)을 거쳐 부르면 따옴표가 깨지니 인자 JSON을 파일로 써서 `-ArgumentsFile`로 넘긴다. 캡처처럼 큰 응답은 `-OutFile`로 받는다.
  - 종료 코드는 0 성공, 1 툴 오류, 2 연결 실패다. 세션은 포트별로 캐시하고, 에디터가 재시작돼 만료되면 스스로 다시 맺는다.
- 인자 이름은 `describe_toolset`의 스키마를 따른다(C++ 툴셋은 camelCase). 틀리면 오류 메시지에 스키마가 실려 온다.
- 오브젝트 인자는 `{"refPath":"/Game/.../X.X"}`다. C++ 클래스는 `/Script/모듈.클래스`, BP 클래스는 `/Game/.../BP_X.BP_X_C`다.
- 호출은 게임 스레드에서 초당 1건꼴로 처리되고 다른 세션과 큐를 공유한다. 수십 건이면 `ProgrammaticToolset.execute_tool_script`로 묶는다.
  - 샌드박스라 `execute_tool`과 안전 모듈(json·math·re·copy·time·datetime)만 쓸 수 있고 `unreal` 모듈은 없다. dict의 `.get(key, default)`도 안 된다.
  - 스크립트 안의 도구 호출이 하나라도 실패하면 그 스크립트의 편집이 전부 롤백된다. 저장처럼 실패할 수 있는 호출은 따로 부른다. 파이썬 예외는 롤백하지 않는다.
- Python 툴셋(`editor_toolset.*`, `state_tree_toolset.*` 등)은 PythonScriptPlugin이 켜져 있어야 등록된다. 지금은 다른 플러그인 의존으로 켜진다. `ObjectTools`가 목록에 없으면 로그의 `LogPython`부터 본다.

## 기본 규칙

- **저장은 `WxToolset.WxPackageToolset.SavePackages`로 한다.** `AssetTools.save_assets`는 수정 표시가 없는 패키지(컴파일만 한 BP 등)를 `true`만 돌려주고 건너뛴다. `SavePackages`는 실제로 쓴 파일만 돌려주고 못 쓰면 실패한다. 외부 액터는 맵이 아니라 액터를 넘긴다.
- **쓰고 나면 다시 읽는다.** `set_properties`·`write_graph_dsl` 등은 일부를 무시하거나 엉뚱한 원소를 바꾸고도 성공을 답한다.
- **최종 판정은 PIE 동작이다.** 에셋 조회 결과가 맞아도 저장 경합이나 미컴파일로 실제 동작이 다를 수 있다. `.agents/scripts/Check-PieErrors.ps1 -Port N [-Map /Game/Maps/LV_X]`가 PIE를 잠깐 돌려 그동안 새로 찍힌 오류·ensure·스크립트 오류(Accessed None 등)·스폰 실패를 보고한다(0 깨끗함, 1 오류 있음, 2 실행 못 함). 조작이 필요한 동작 확인은 여전히 사람이 한다.
- **모달 창은 MCP를 통째로 막는다.** 저장 확인·스키마 선택 같은 창이 뜨면 응답이 멈춘다. user32 `WM_CLOSE`로 닫는다. 에디터를 닫기 전에 저장할 것이 남았는지 먼저 확인한다.
- **큰 응답은 파일로 받는다.** 캡처·스크린샷은 base64 PNG가 수십만 자라 잘라 읽지 말고 디코드해 파일로 본다.
- 에디터는 새로 저장한 파일을 git 인덱스에 자동으로 올린다(`A`, 삭제는 `D`). 커밋에 섞이지 않게 `git restore --staged`로 내린다.
- 프로젝트 툴셋 `WxToolset.*`은 기존 표면이 못 닿는 곳(StateTree 바인딩·파라미터·컴파일, 몽타주 섹션·노티파이, MVVM 경로, BP 변수 메타·enum 변수, 랜드스케이프 생성, 물 바디 스플라인, 저장)을 맡는다. 쓰기 전에 `describe_toolset`으로 설명을 읽는다. 소스는 `Plugins/WxToolset/`이고, 툴을 추가하거나 시그니처를 바꾸면 Live Coding으로는 등록되지 않으니 빌드 후 에디터를 재시작한다.

## 다른 세션과 함께 쓸 때

- 다른 세션이 띄운 에디터는 닫지 않는다. 에디터 프로세스는 `UnrealEditor*` 패턴으로 찾는다(`-Win64-DebugGame`·`-Cmd` 변형이 있다).
- `run-editor`는 이 프로젝트의 에디터를 모두 종료한다. 다른 세션 에디터가 떠 있으면 쓰지 말고, 빌드 후 `Start-WxEditorMcp.ps1`로 띄운다. 끝나면 저장할 것을 다 저장했는지 확인하고 내가 띄운 PID만 `Stop-Process -Id`로 닫는다(창을 닫으면 저장 확인 창이 MCP를 막는다).
  - 자동 저장이 한 번이라도 돈 세션을 이렇게 닫으면 다음 시작에 `Restore Packages` 창이 떠 `Start-WxEditorMcp.ps1`이 시간 초과한다. 그 세션에서 저장하지 않은 작업이 없으면 그 창을 user32 `WM_CLOSE`로 닫는다(복구하지 않는다).
- 포트 8000은 먼저 뜬 에디터가 차지한다. `Start-WxEditorMcp.ps1`가 빈 포트(`-ModelContextProtocolPort=N`)를 고르니 출력된 포트로만 호출한다. 엉뚱한 에디터에 보내면 옛 바이너리가 에셋을 만든다.
- 다른 에디터가 로드한 에셋은 파일이 잠겨 내 저장이 "Failed to move ... to temp directory"로 실패한다. 그 에디터가 닫힐 때까지 저장을 미룬다.
- 병행 세션이 도는 동안에는 `git add`·`git mv`로 인덱스를 채워 두지 않는다. 그 세션의 커밋에 섞인다.
- 커맨드릿(`UnrealEditor-Cmd`)도 MCP 포트를 열었다 닫아 에디터 MCP 세션이 흔들릴 수 있다.

## 영역별 참고

필요한 영역의 문서만 읽는다.

| 작업 | 문서 |
|---|---|
| 에셋 프로퍼티 편집(`ObjectTools`), BP CDO, 데이터 에셋·테이블, 머티리얼·Niagara | [references/assets.md](references/assets.md) |
| StateTree 상태·태스크·전이·바인딩·링크 | [references/statetree.md](references/statetree.md) |
| 블루프린트 그래프, 위젯(UMG), MVVM | [references/blueprint-ui.md](references/blueprint-ui.md) |
| 레벨 배치, 외부 액터, PCG, 물, 랜드스케이프, 미니맵 | [references/world.md](references/world.md) |
| 에디터 UI 조작(SlateInspector), PIE, 로그·콘솔·캡처 | [references/slate-pie.md](references/slate-pie.md) |
| 클래스·구조체·태그 개명, 리다이렉트 제거, 재저장 커맨드릿 | [references/migration.md](references/migration.md) |
