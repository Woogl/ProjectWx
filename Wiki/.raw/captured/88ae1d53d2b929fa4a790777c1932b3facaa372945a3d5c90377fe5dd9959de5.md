# 상호작용 목록 VM 단순화

상태: 완료 · 체크리스트 6/6 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 목록 표시 | 게임: 겹친 대상들의 목록이 보이고, 선택지가 여러 개인 장치는 행이 여러 개 뜬다 | 사람 | 통과 | 이우성 2026-09-25 |
| 휠 선택 | 게임: 휠로 선택 표시와 외곽선이 함께 움직이고 선택한 선택지가 실행된다 | 사람 | 통과 | 이우성 2026-09-25 |
| 범위 이탈·리스폰 | 게임: 범위를 벗어나면 목록이 사라지고 리스폰 뒤에도 동작한다 | 사람 | 통과 | 이우성 2026-09-25 |
| 행 표시 | 게임: 엘리베이터 탑승칸 버튼이 `Floor N`으로 보이고 픽업 행의 키 아이콘과 이름이 한 번씩만 보인다 | 사람 | 통과 | 이우성 2026-09-25 |
| 탑승칸 버튼 잠금 | 게임: 비활성 상태에서 탑승칸 버튼이 잠기고, 대기 상태에서는 다른 층만 `Floor N`으로 뜨며, 엘리베이터가 와 있는 층의 호출 버튼은 잠긴다 | 사람 | 통과 | 이우성 2026-09-25 |
| 코드 리뷰 | 상호작용 목록 VM·엘리베이터 버튼 잠금 변경 | 사람 | 통과 | 이우성 2026-09-25 |

## 확정 설계 · 2026-09-23 사용자 확정

- 요청: InteractionList VM이 행 VM과 역할이 겹친다고 보고 제거하거나, 선택 처리를 더 단순하게 하기를 원했다.
- 판단: 목록 VM 클래스는 유지한다. 목록 VM은 행 VM(`UWxViewModel_Interaction`)을 담는 쪽이라 역할이 겹치지 않는다. 클래스를 없애려면 로직을 위젯이나 스캐너로 옮기거나 UX를 줄여야 한다.
- 판단: 겹친 대상을 목록으로 보여 주고 휠로 선택하는 동작은 유지한다(사용자).
- 판단: Resolver는 유지한다. 엔진 CreateInstance는 `NewObject`만 호출하고, `ReleaseInstance`는 Resolver에만 `DestroyInstance`를 부른다(UE 5.8 `MVVMViewClass.cpp`). 따라서 Resolver 없이는 스캐너를 넘길 수도, VM을 정리할 수도 없다.
- 설계: 스캐너 신호를 `OnRowsChanged` 하나로 합친다. VM은 신호마다 스캐너에서 목록과 선택을 읽어 행 전체를 다시 만든다. 이로써 선택 상태를 목록 VM `SelectedIndex`와 행 `bSelected`에 동기화하던 코드가 사라진다.
- 설계: 스캐너 늦은 도착을 관찰하던 코드(`OnAnyScannerReady`)를 제거한다. 이 코드는 Experience 주입(`7e709a764`) 때문에 추가됐다(`6d3cc56de`). `97eb35f97`에서 스캐너가 `AWxPlayerController` 생성자 컴포넌트로 돌아왔으므로 Resolver가 직접 찾아 넘긴다.
- 대가: 선택을 바꿀 때마다 ListView 엔트리가 새로 붙는다. `WBP_Interaction`은 `bSelected`로 이미지 가시성만 바꾸므로 화면상 차이는 없다. 선택 전환 애니메이션을 넣으려면 선택 신호를 다시 나눠야 한다.
- 범위 밖: 엔진 MVVM ListView 확장으로 `WBP_Interaction`의 BP 연결 코드를 제거하는 작업(에디터 작업), 공용 Resolver 통합(미검증).

## 구현 · 2026-09-23

- 커밋되지 않은 변경이며, 기준은 HEAD `445b4b66c`다.
- 스캐너(`WxInteractionScannerComponent.h/.cpp`): `OnListChanged`와 `OnSelectionChanged`를 `OnRowsChanged` 하나로 합쳤다. `OnAnyScannerReady`와 `FWxOnScannerReady`를 제거했다.
- 목록 VM(`WxViewModel_InteractionList.h/.cpp`):
  - 남긴 것: `Initialize`, `Deinitialize`, `HandleRowsChanged`, `RequestInteract`, `RequestCycle`, `Entries`, `CachedScanner`
  - 제거한 것: 관찰 코드, `SelectedIndex`, `ApplySelection`, `RebuildEntries`, 두 신호 핸들러
- Resolver: `FindComponentByClass`로 찾은 스캐너를 `Initialize`에 넘긴다.
- 행 VM(`WxViewModel_Interaction.h`): 주석만 "대상 하나당"에서 "행(선택지) 하나당"으로 정정했다.
- WBP 에셋은 변경하지 않았다. 바인딩하는 `Entries`, `RequestInteract`, `RequestCycle`과 Resolver 클래스 이름은 그대로다.

## 검증 · 2026-09-23

- build-doctor(WxEditor Win64 Development): 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_002738_023_5548.log`이다.
- `CompileAllBlueprints` 커맨드렛: 허용 목록 경로 해석에 실패해 전체 BP를 대상으로 실행됐다. 커맨드렛은 이 경로를 프로젝트 기준 상대 경로로 붙인다.
  - `WBP_InteractionList`, `WBP_Interaction`, `WBP_GameLayout`은 모두 컴파일에 성공했다.
  - 569개가 성공했고, `EUB_SnapToActor` 1개만 실패했다(오류 8, 경고 2). 이 BP는 NiagaraExamples 소속이며 Dataprep 플러그인이 없어 실패한 것으로, 이번 변경과 무관하다.
  - MVVM·스캐너 관련 로그는 없었다.
- 제거한 심볼을 소스 전체에서 검색한 결과 0건이다.
- 인게임은 미실행이다(unreal-mcp 미연결). 다음 항목은 인간 확인이 필요하다.
  - 겹친 대상들의 목록이 표시되는지
  - 휠로 선택 표시와 외곽선이 함께 움직이는지
  - 선택한 선택지가 실행되는지
  - 선택지가 여러 개인 장치에 행이 여러 개 뜨는지
  - 범위를 벗어나면 목록이 사라지는지
  - 리스폰한 뒤에도 동작하는지
- 완료 단계에서 할 일: Wiki `world.md`의 상호작용 계약 절에 목록 VM이 스캐너에 연결되는 방식과, 주입 방식으로 바꿀 때의 주의점을 반영한다.

## 추가 정리 · 2026-09-23 사용자 확정

- 판단: 두 클래스 구조(목록 VM + 행 VM)를 유지한다. `UWxViewModel_Interaction` 하나로 합치는 안은 기각했다. 한 클래스가 두 역할을 맡게 되고, 모듈 이동과 CoreRedirects 3개가 필요하다.
- 행 VM: `SetPrompt`/`SetSelected`와 `WxViewModel_Interaction.cpp`를 삭제했다. 행은 만들어진 뒤 바뀌지 않으므로 목록 VM이 필드에 직접 값을 넣는다.
  - 근거 1: 엔진은 VM을 붙이는 순간 해당 소스의 바인딩을 모두 실행한다(UE 5.8 `MVVMView.cpp:1074-1076`).
  - 근거 2: 문구 변경도 새 행으로 반영된다. Device StateTree 대기 태스크 등이 문구를 바꾸면 스캐너가 0.1초마다 읽어 비교하고, `OnRowsChanged`를 발행해 행을 새로 만든다.
- 목록 VM: `Initialize` 첫 줄의 `Deinitialize()` 호출과 `Deinitialize`의 표시 비우기(`Entries` 초기화와 알림)를 제거했다. `Initialize`는 새 VM에 한 번만 불리고, 정리된 VM은 다시 쓰이지 않는다.
- `FieldNotify` 지정자는 유지했다. WBP의 OneWay 바인딩이 필요로 한다.
- build-doctor 결과: 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_005331_888_43468.log`이다.
- `CompileAllBlueprints` 재실행: 허용 목록을 프로젝트 기준 상대 경로로 넘겨 대상 3개만 컴파일했다. `WBP_InteractionList`, `WBP_Interaction`, `WBP_GameLayout` 모두 성공했고 오류 0, 경고 0이다.
- 인게임 확인은 앞의 인간 확인 항목과 같다(미실행).

## 상호작용 문구 출처 정리 · 2026-09-23 사용자 확정

- 판단: 모든 문구를 StateTree로 옮기는 안은 기각했다. 대신 "문구는 상호작용했을 때 실제로 일어날 행동의 주인이 갖는다"를 기준으로 삼는다.
  - Device: StateTree
  - 대화 액터: 대화 컴포넌트
  - 픽업: 아이템 데이터
  - 피니시: 피니시 어빌리티(`WxEnemyCharacter.cpp:124` 주석)
  - 픽업과 피니시는 행동이 StateTree에 없고, 픽업은 실행 중에 스폰되는 드랍이라 StateTree가 맞지 않는다.
- 기준에서 벗어난 곳 1 — 엘리베이터 층 문구: C++ `"{0}층"`이던 것을 `FWxDeviceTriggerRule_SplineStops::StopPrompt`로 옮겼다. 이 규칙은 WaitForTrigger 태스크 안의 구조체라 StateTree에서 편집한다.
  - C++ 초기값은 `"Floor {0}"`이다. 기존 `ST_Elevator`가 에셋을 고치지 않고도 값을 받게 하려는 것이다.
  - 인게임 문구를 영어로 한다는 규칙 위반도 함께 해결됐다.
- 기준에서 벗어난 곳 2 — 픽업 문구의 `[F]`: 제거했다. 키 표시는 `WBP_Interaction`의 `CommonActionWidget`이 맡는다(`DT_InputActions`의 `Interact` 행, 키보드 아이콘 `T_Keyboard_F`).
- build-doctor(Development) 결과: 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_010554_487_43268.log`이다.
- 사용자가 실행 중인 DebugGame 에디터(01:00 빌드)에는 목록 VM 단순화까지만 들어 있고, 이 문구 수정은 없다. 재빌드와 에디터 재시작이 필요하다.
- 인게임 확인 필요:
  - 엘리베이터 탑승칸 버튼 목록이 `Floor N`으로 표시되는지
  - 픽업 행에 키 아이콘과 이름이 한 번씩만 표시되는지

## 엘리베이터 탑승칸 버튼 잠금 · 2026-09-23 사용자 확정(B안)

- 증상: 비활성 상태에서 탑승칸 버튼 목록에 `Floor 1`과 `Floor 2`가 모두 떴다. 원인은 비활성 상태 작동 대기 노드에 켜진 `bAcceptCurrentStop=true`가 탑승칸 목록에도 지금 층을 넣은 것이다. 원래 있던 동작이며 문구 수정과는 무관하다.
- 조사(MCP):
  - `ST_Elevator`는 Inactive(`bAcceptCurrentStop=true`), Moving(Close→Move→Open), Idle(`false`)로 구성된다. Inactive에 진입 조건이 없어 모든 엘리베이터가 비활성으로 시작한다.
  - `BP_Elevator`에 내장된 장치는 호출 버튼 `ButtonDevice_1F`/`ButtonDevice_2F`와 탑승칸 버튼 `ButtonDevice_Lift`뿐이다.
  - 배치된 엘리베이터는 LV_DevCombat, LV_Dungeon1, SiegeCannonEmplacement01의 3대이고, 연결된 외부 장치(활성화 레버)는 없다.
- 판단: 기획서(`Object_Design.md` §4.4)의 "활성화 레버로만 영구 활성화"(118행)를 그대로 구현하면 레버가 없는 기존 3대가 영구히 잠기므로 A안은 보류한다. B안은 비활성 상태에서 탑승칸 버튼을 잠그고, 탑승칸 목록에서 항상 지금 층을 뺀다(122행).
  - 활성화 레버와 「초기 활성화 여부」는 레벨에서 필요해질 때 기획 확인을 거쳐 별도 작업으로 한다.
- 구현: `FWxDeviceTriggerRule_SplineStops::bAcceptCurrentStop`의 이름을 `bWakeOnCall`로 바꿨다(깨우는 상태: 밖 호출만 받고 지금 층 호출도 받으며, 탑승칸 버튼은 잠근다). 탑승칸 목록은 플래그와 관계없이 지금 층을 뺀다.
  - `Config/DefaultEngine.ini`에 임시 PropertyRedirect를 두었다. 프로젝트 관례에 따라 `ST_Elevator`를 재저장한 뒤 제거한다.
- build-doctor(Development) 결과: 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_013040_807_45120.log`이다.
- `ST_Elevator` 저장 실패: `LogSavePackage: Error: Unexpected custom version "FortniteMain" found when saving`가 났다. 원인은 `StopPrompt`의 C++ 기본값(`NSLOCTEXT "Floor {0}"`)이다.
  - 인스턴스 구조체는 저장할 때 기본값과 같은 필드를 생략한다(`InstancedStruct.cpp:144-155`). 반면 번역 대상 문구 수집은 그 FText를 잡아 `FortniteMain` 버전을 쓴다(`GatherableTextData.cpp:28`). 두 경로가 어긋나 저장이 중단된다.
  - MCP로 검증했다. 기본값과 같은 값은 저장 실패, 다른 값(`Stop {0}`)은 저장 성공(01:39).
- 조치: `StopPrompt`의 C++ 기본값을 제거했다. 문구 출처가 StateTree로 통일되고, WaitForTrigger `Prompt`와 같은 방식이다.
  - 01:39 저장으로 `bWakeOnCall`이 에셋에 기록되었으므로 임시 PropertyRedirect도 제거했다.
- 남은 일: 새 코드로 에디터를 재시작한 뒤 두 노드의 `StopPrompt`를 `Floor {0}`로 입력하고 저장한다. 지금은 확인용 값 `Stop {0}`이 들어 있다.
- 01:42 사용자가 새 코드로 재빌드·재시작했다. 01:43 사용자 저장과 01:44 종료 시 저장이 모두 성공했다.
  - 최종 `ST_Elevator` 값(MCP와 파일로 확인): Inactive는 `bWakeOnCall=true`, `StopPrompt="Floor {0}"`. Idle은 `false`, `"Floor {0}"`.
  - 확인용 값 `Stop {0}`과 옛 속성 이름 `bAcceptCurrentStop`은 에셋에 남아 있지 않다.
- 인게임 확인 필요:
  - 비활성 상태에서 탑승칸 버튼이 잠기는지
  - 대기 상태에서 탑승칸 목록에 다른 층만 `Floor N`으로 뜨는지
  - 엘리베이터가 이미 와 있는 층의 호출 버튼이 잠기는지
- 완료 단계에서 Wiki에 반영할 것: "인스턴스 구조체(StateTree 노드 안의 규칙 등)의 FText에 C++ 기본값을 두면, 값이 기본값과 같을 때 에셋 저장이 FortniteMain 커스텀 버전 불일치로 실패한다."
- Wiki 반영(2026-09-23, 사용자 요청): 원자료 `.wiki/raw/notes/2026-09-23-instanced-struct-ftext-default-save-fail.md`를 수집하고, `.wiki/wiki/topics/world.md`에 「엘리베이터 정차 지점 규칙」 절과 FText 기본값 저장 함정을 편찬했다. 순정 lint PASS(0건), CheckWikiLinks 38개 문서 오류 0.

## Wiki 반영 · 2026-09-23

- 사용자 요청(위키 최신화)으로 인게임 확인 전에 반영했다. 목록 동작에는 "인게임 미검증"을 표기했다.
- 원자료 `.wiki/raw/notes/2026-09-23-interaction-list-vm.md`를 수집했다. `wiki/topics/world.md`에 행 교체·선택 복원 규칙, 「HUD 목록 연결」(리졸버의 스캐너 연결과 주입 전환 시 주의점), 「상호작용 문구의 출처」를 편찬했다. `ui.md`·`inventory.md`에는 요약과 링크만 두었다.
- 순정 lint PASS(0건), `CheckWikiLinks.ps1` 오류 0(46개 문서). PowerShell 7이 없어 뷰어(`Export-Wiki.ps1`)는 갱신하지 못했다.


## 사용자 테스트 결과 · 2026-09-25T17:21:48.320Z

<!-- test-feedback:request-9e631ae3-bbf4-403f-a945-1a8f5a80fe55:submitted -->
- 전달한 사람: 이우성

> 통과 · 목록 표시
> 통과 · 휠 선택
> 통과 · 범위 이탈·리스폰
> 통과 · 행 표시
> 통과 · 탑승칸 버튼 잠금
> 통과 · 코드 리뷰


## AI 완료 정리 · 2026-09-25T17:21:48.324Z

<!-- test-feedback:request-9e631ae3-bbf4-403f-a945-1a8f5a80fe55-cleanup:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 정리 완료

AI 요약:

> 사람 테스트 6개 통과 범위를 Wiki에 반영했습니다. 게임 코드·에셋·작업 기록·접수 JSON과 작업 상태는 변경하지 않았습니다.
> 작업과 무관한 기존 변경과 Git 줄바꿈 경고는 그대로 보존했습니다.

> 변경: .wiki/raw/notes/2026-09-26-interaction-list-play-acceptance.md에 사람 테스트 결과와 검증 경계를 수집했습니다.

> 변경: .wiki/wiki/topics/world.md의 인게임 미검증 설명을 갱신하고, 대기 상태의 호출 잠금과 비활성 상태의 깨우기 예외를 구분했습니다.

> 변경: 관련 색인·원자료 통계·log.md를 갱신했습니다. 기존 원자료는 보존했습니다.

> 근거: AGENTS.md, 작업 절차, 지정 작업 기록, wiki 스킬, .wiki/config.md·schema.md와 기존 관련 원자료를 읽었습니다.

> 근거: Get-FileHash 결과 작업 기록 SHA-256이 접수 해시와 일치했습니다.

> 근거: 순정 llm-wiki lint --local --json: 모든 검사 항목 0건, pass.

> 근거: CheckWikiLinks.ps1: 66개 문서 오류 0. 변경 문서의 로컬 링크 36개와 UTF-8 검사 통과.

> 근거: git diff --check -- .wiki: 공백 오류 없음. 게임 테스트는 재실행하지 않았습니다.

> 근거: Export-Wiki.ps1: Wiki·Workflow 뷰어 각각 64개 문서 갱신.
