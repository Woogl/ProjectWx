# 에디터 UI 조작·PIE

MCP 툴로 안 되는 일은 SlateInspector로 에디터 UI를 직접 누른다. 클릭이 `true`를 돌려줘도 일어났다는 증거가 아니니 결과를 따로 확인한다.

## SlateInspector

- `SlateInspectorToolset.Observe(ref:"", maxDepth:40)`로 한 번 등록한 뒤 `Snapshot(ref:"", maxDepth:60)`을 뜬다. 반환은 JSON 문자열 안에 이스케이프된 텍스트라 JSON으로 디코드해 `returnValue`를 꺼내야 줄 단위로 읽힌다. 전체 스냅샷은 매우 크니 창 ref(`w<N>`)로 좁힌다.
- UMG·CommonUI 버튼은 `button`이 아니라 `generic "라벨" [ref=gNN]`으로 잡힌다. 라벨은 위젯 텍스트 그대로다. `Click(ref)`로 누른다.
- 대상 탭이나 창이 앞에 있어야 한다. 다른 탭 뒤에 있으면 `WaitFor`가 false이고 클릭은 먹은 척만 한다. 탭 노드를 먼저 클릭하거나 `EditorAppToolset.OpenEditorForAsset`으로 앞으로 가져온다.
- 트리 항목 ref는 갱신될 때마다 바뀐다. 누르기 직전에 다시 스냅샷한다. 컨텍스트 메뉴는 새 최상위 창으로 뜨고 ref는 직전 창 번호 근처다.
- `Windows`가 빈 배열이면 에디터 창이 가려져 UI 자동화를 할 수 없다.
- 이름 입력처럼 OS 키 입력이 필요한 곳은 SendKeys로 넣는다. 모달이 뜨면 MCP 전체가 막히니 user32 `WM_CLOSE`로 닫는다.
- 콘솔 명령·cvar: 상태 표시줄 "Cmd" 콤보 옆 `textbox` ref를 스냅샷으로 찾아 `Type(ref, text, submit:true)`. PIE 중에도 된다.

## PIE

- 오류만 볼 때는 `.agents/scripts/Check-PieErrors.ps1`을 쓴다. 에디터 git 소스 컨트롤이 PIE 임시 패키지(`/Memory`)에 내는 오류는 게임과 무관해 빼고 보고한다.
- `EditorAppToolset.StartPIE(options{bSimulate, playMode, warmupSeconds, startTransform})`·`StopPIE`·`IsPIERunning`. `StartPIE`는 BeginPlay 뒤 `warmupSeconds`가 지나야 돌아온다. 스폰 지점이 막혀 있으면 폰이 아예 생기지 않는다. `LogSpawn`을 확인하고 여유 높이에서 떨어뜨린다.
- 게임 UI 흐름(메뉴 → 선택 → 맵 이동)은 위 SlateInspector 클릭으로 끝까지 몰 수 있다.
- `PressKey`는 PIE 게임 뷰포트까지 닿지 않는다. 캐릭터 조작이 필요한 검증은 사람이 한다.
- PIE 오브젝트는 `UEDPIE_0_` 경로로 조회·편집한다. 폰을 텔레포트해 오버랩을 일으키는 식으로 상황을 만든다. UPROPERTY가 아닌 값(`IgnoreMoveInput` 등)은 읽을 수 없다.
- 자동화 테스트는 `AutomationTestToolset`(DiscoverTests → ListTests → RunTests → GetTestResults)으로 돌린다.

## 로그·캡처

- 로그: `LogsToolset.GetLogEntries(category:"", maxEntries, pattern)`. 컴파일·저장 같은 결과는 반환값보다 새로 찍힌 로그 줄로 판정한다.
- 같은 머신의 다른 에디터도 `Saved/Logs/Wx.log`에 쓴다. 파일로 볼 때는 명령줄 줄로 내 프로세스 것인지 가른다.
- 뷰포트: `EditorAppToolset.CaptureViewport`는 `captureTransform`이 필수이고 에디터 월드 기준이다. 결과는 `returnValue.image.data`의 base64 PNG다. PIE 게임 화면은 `CaptureEditorImage`로 찍는다.
- UI: `SlateInspectorToolset.Screenshot(ref:"")`은 `returnValue.data`에 base64 PNG를 준다.
- base64는 파일로 저장해 디코드한 뒤 이미지로 본다.
