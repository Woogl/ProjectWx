# WxDialogue — 코드 리뷰

> 846줄 11파일의 작은 모듈이고, 설계 근거가 헤더 주석에 촘촘히 적혀 있어 전반적으로 건강하다. CLAUDE.md 코딩·모듈 규칙 위반은 한 건도 없다(Copyright 첫 줄 12파일 전수 확인, `BlueprintCallable`·`FORCEINLINE`·인라인 정의 0건, Build.cs 의존은 `WxCore`뿐, 유일한 람다에 예외 사유 주석 있음). 남은 지적은 세션 수명 관리와 죽은 API 쪽이다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 통독했고, 세션 컴포넌트와 StateTree 태스크는 UE 5.8 엔진 소스(`FinishTask`·`AutoManageActiveCameraTarget`)까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 세션을 밖에서 끝낼 수단이 없어 폰 소실 시 세션이 굳는다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:129`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:203`
- **범주**: 설계/구조
- **문제**: `EndDialogue()`는 private이고 호출처가 `Advance()`(cpp:65·72·81)와 `ClientStartDialogue_Implementation()`의 겹침 처리(cpp:126) 넷뿐이다. 즉 세션은 **끝까지 넘기거나, 다른 대화가 새로 열려야만** 닫힌다. 컴포넌트에 `EndPlay`/`UninitializeComponent`/`OnUnregister` 정리도 없다.
  구체적 실패: 대화 도중 폰이 죽거나 교체되면 `WxUIManagerSubsystem::WatchPawnTags`가 무조건 `CloseDialogueScreen()`을 부른다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:226`). 대화 창이 사라지면 `Advance()`를 부를 주체가 없고, 세션은 `CurrentRowName`을 그대로 든 채 `HasActiveDialogue()==true`로 남는다. 그 결과 (a) `OnDialogueEnded`가 영영 발화하지 않아 `FWxStateTreeTask_PlayDialogue`가 Running으로 고착되고(`Private/WxStateTreeTask_PlayDialogue.cpp:50-53`) 그 퀘스트 스텝이 진행을 멈춘다, (b) `EndDialogueCamera()`가 돌지 않아 스폰한 `ACameraActor`가 남는다 — Owner가 PlayerController라(cpp:251) 폰과 함께 정리되지 않는다. 뷰 타겟 자체는 재빙의 때 엔진의 `AutoManageActiveCameraTarget`이 되돌려 주지만, 액터와 세션 상태는 남는다.
  또 하나: 같은 폰을 unpossess 후 다시 possess 하는 경로에서는 ASC가 살아 있어 `State.Dialogue` 카운트가 1로 남는데, `WatchPawnTags`가 창을 이미 닫은 뒤라 `NewOrRemoved` 전이가 다시 오지 않는다 — 창 없이 태그만 남아 상호작용 어빌리티가 계속 차단된다.
- **제안**: 공개 취소 진입점(`CancelDialogue()` 또는 `EndDialogue` 공개)을 두고, `UninitializeComponent`/`OnUnregister`에서 세션을 접는다. 추가로 컨트롤러의 `OnPossessedPawnChanged`를 관찰해 폰이 바뀌면 세션을 닫으면, 태그 발행처(`TaggedAbilitySystem`)와 세션의 수명이 같은 축에서 끝난다.
- **확신도**: 중간

### 2. 🟡 `StartDialogueRow` 실패 갈래가 비대칭이라 `HasActiveDialogue()`를 성공 신호로 쓰는 계약이 성립하지 않을 수 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:41-52`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:25-46`
- **범주**: 버그/정확성
- **문제**: 시작 행 검증(cpp:43)은 RPC **바깥**에, 이전 세션 정리(`EndDialogue()`, cpp:124-127)는 RPC **안쪽**에 있다. 그래서 `StartDialogueRow`가 자기 검증에서 걸러지면 이전 세션이 그대로 살아남고, 검증을 통과한 뒤 실패(ASC 없음·`EnterRow` 실패)하면 이전 세션이 접힌다.
  `FWxStateTreeTask_PlayDialogue`는 호출 직후 `HasActiveDialogue()`로 성공을 판정하는데(cpp:42), 앞의 갈래에서는 **직전 대화의 잔존 상태**를 자기 성공으로 오독해 열지도 않은 대화를 기다리며 Running으로 남는다. 지금은 태스크가 같은 `StartRow` 검사를 앞단에 중복해 둔 덕분에(cpp:25-29) 이 경로가 우연히 막혀 있을 뿐이라, 새 호출자가 생기거나 검증 조건이 늘면 바로 드러난다.
- **제안**: `StartDialogueRow`를 `bool` 반환으로 바꿔 호출자가 반환값으로 판정하게 하거나, 검증을 `ClientStartDialogue` 안쪽(이전 세션 `EndDialogue` 이후)으로 옮겨 모든 실패가 같은 결과 상태를 남기게 만든다. 그러면 태스크의 중복 검사도 지울 수 있다.
- **확신도**: 중간

### 3. 🟡 호출자 없는 public API 두 개 (`GetCurrentRowHandle`, `GetCurrentDialogueTarget`)
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:63`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:69` (정의는 `Private/WxDialogueSessionComponent.cpp:94-106`)
- **범주**: 중복/복잡도
- **문제**: 저장소 전체(`Source`·`Plugins`)에서 두 함수의 호출자가 하나도 없다. `BlueprintCallable`도 아니라 BP에서도 부를 수 없으므로 현재로선 완전한 데드 코드다. 그런데 헤더 주석과 README는 이 둘을 "대화의 의미를 관찰로 판정하는 소비자 계약"으로 설명하고 있어, 문서가 존재하지 않는 소비자를 전제한다. 특히 `GetCurrentRowHandle`은 발견 1의 굳은 세션 상태에서 이미 끝난 대화의 행을 계속 답하게 되므로, 지금 상태로 소비자를 붙이면 그 결함을 그대로 물려받는다.
- **제안**: 소비자를 붙일 계획이 확정적이면 그 시점까지 미루고 지금은 지운다(프로젝트 규칙: 호출자 없는 선언 금지). 남긴다면 헤더 주석에 "예정 계약, 아직 소비자 없음"을 명시해 문서와 실제를 맞춘다.
- **확신도**: 중간(관찰 자체는 확실, 제거 여부는 퀘스트 연동 계획에 달림)

### 4. 🟢 `StartDialogueWith`의 실패 갈래만 조용하다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:24-27`
- **범주**: 버그/정확성
- **문제**: 상호작용 주체가 폰이 아니거나 컨트롤러에 세션 컴포넌트가 없으면 로그 없이 return 한다. 같은 모듈의 `StartDialogueRow`는 정확히 같은 상황을 두고 "이 갈래가 조용하면 'F 를 눌러도 아무 일이 없다'만 남는다"고 적으며 경고를 찍는다(`Private/WxDialogueSessionComponent.cpp:45-48`). Experience 주입이 빠져 세션 컴포넌트가 안 붙은 조립 실수가 바로 이 갈래로 떨어지는데, 유일하게 단서가 없다.
- **제안**: 같은 형식의 `LogWxDialogue` Warning 한 줄을 추가한다(주체·컨트롤러 이름 포함).
- **확신도**: 중간(진단 가드를 의도적으로 미루는 프로젝트 방침이 있으므로 판단은 사람 몫)

### 5. 🟢 `FWxDialogueTableRow`와 `FWxSubtitleTableRow`가 거의 같은 모델이다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h:16-38`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h:14-33`
- **범주**: 중복/복잡도
- **문제**: 두 행 타입 모두 `Speaker`·`Line`(MultiLine)·`NextRow`(None이면 종료)로 이루어진 같은 연결 리스트 모델이고, 주석 문구까지 거의 같다. 차이는 `TargetPose`(대화) 대 `Duration`(자막) 한 필드뿐이다. 진행 규약(빈 `Line`은 잘못된 행, `NextRow=None`이 종료)이 두 곳에 복제되어 있어 한쪽 규약이 바뀌면 다른 쪽이 조용히 어긋난다.
- **제안**: 즉시 통합할 필요는 없다 — 공용 타입을 `WxCore`로 올리는 것은 "WxCore엔 도메인 타입 금지" 방침과 충돌하므로 현 상태가 의도된 결과일 수 있다. 다만 어느 한쪽의 진행 규약을 손댈 때 다른 쪽을 반드시 함께 확인하도록 두 헤더에 상호 참조 한 줄을 남겨 두는 편이 안전하다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`
- **훑은 파일**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/WxDialogue.uplugin`
- **경계 밖에서 대조한 파일**(발견 근거용): `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxNpc.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`
- **미검토 / 한계**:
  - 포즈 스트리밍의 동기 완료 경로(`RequestAsyncLoad`가 즉시 콜백하는 경우 `PoseLoadHandle` 대입이 `HandlePoseLoaded`의 `Reset()`보다 늦는다)는 코드가 앞서 `PendingPose.Get()`으로 이미 로드된 경우를 걸러 내므로 실제 도달 가능성을 판정하지 못했다. 잔존 핸들이 남더라도 다음 `CancelHandle()`이 무동작이라 피해는 없어 보여 발견으로 올리지 않았다.
  - 멀티플레이(전용 서버·리슨 서버의 원격 클라)에서 `HasActiveDialogue()` 즉시 조회가 실패하는 것은 v1 싱글/리슨 호스트 전제로 헤더·README·태스크 주석 세 곳에 명시되어 있어 발견에서 제외했다. 실제 멀티 검증은 하지 않았다.
  - BP/WBP 에셋 내부(대화 창 위젯의 Advance 바인딩, DT 대화 테이블의 행 정합)는 범위 밖이다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 11파일 — `/module-review`로 갱신*
