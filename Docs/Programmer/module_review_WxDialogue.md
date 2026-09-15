# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈로 호스트 액터 / 대화 정의 / PC 세션의 책임 분리가 선명하고 `CLAUDE.md` 위반은 0건이다(저작권 첫 줄·`Wx` prefix 전부 통과, 헤더 본문 정의는 사유 주석이 달린 `GetInstanceDataType()` 하나 — `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:13`·`:41`, 유일한 람다에도 사유 주석 — `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:48`, Wx 플러그인 의존은 `WxCore` 뿐). 직전 리뷰 이후 커밋으로 빙의 전이 시 세션 고착·카메라 매니저 무검사·미사용 접근자·UOL 의존이 해소됐고, 새로 들어간 빙의 전이 종료 경로는 엔진 `AController::Possess`/`UnPossess` 발화 순서와 UI 매니저의 같은 델리게이트 처리까지 대조해 정확하다고 판단했다. 남은 문제는 종료 신호가 사유를 싣지 않는 설계 하나가 중심이며, 이번 리뷰는 모듈 11파일을 전부 읽고 세션 컴포넌트·Play Dialogue 태스크를 정독한 뒤 소비자(WxGame·WxUI)와 엔진 소스를 계약 검증용으로 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 종료 신호에 사유가 없어, 사망·겹침·데이터 오류로 끊긴 대화도 `Play Dialogue` 가 `Succeeded` 로 끝난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:230`(사유 없는 단일 발화) ← 호출부 `:84`·`:91`·`:100`·`:131`·`:169` → `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:50-53`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 는 `OnDialogueEnded.Broadcast()` 만 하므로 태스크 리스너는 무조건 `FinishTask(EStateTreeFinishTaskType::Succeeded)` 를 낸다. 그러나 호출부 다섯 곳 중 정상 완주는 `NextRow = None` 인 `:91` 하나뿐이다.
  - `:169` 빙의 전이 — 이번 수정으로 생긴 경로다. 대화 중 사망 후 리스폰하면 `Source/WxGame/Framework/WxRespawnLibrary.cpp:47` 의 `UnPossess` 가 세션을 접고, 그 대화를 기다리던 퀘스트 단계는 플레이어가 읽지 않은 대사를 완료로 넘긴다. 고착(퀘스트 정지)은 풀렸지만 "조용한 건너뛰기"로 바뀌었고, 수정 워크로그(`.claude/worklog/2026-09-14-대화-세션-빙의-전이-종료.md:63`)도 이를 미결 결정으로 남겼다.
  - `:131` 겹침 — 다른 트리의 `Play Dialogue` 가 앞 대화를 접으면 앞 태스크도 성공으로 끝난다. 이 경로가 실재한다는 것은 `:127` 주석이 스스로 적었다.
  - `:100` 다음 행 해석 실패 — 바로 위 `:97` 주석이 "정상 종료가 아니다"라고 하면서 성공으로 보고한다. 같은 데이터 오류가 첫 행이면 `Failed`(`Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:28`), 두 번째 행 이후면 `Succeeded` 로 갈려 `NextRow` 오타 하나가 퀘스트를 그대로 통과시킨다.
  - `:84` 세션 도중 테이블 재임포트(에디터 한정).
- **제안**: `OnDialogueEnded` 에 완주 여부(bool 또는 종료 사유 enum)를 실어 태스크가 `:91` 외의 종료를 `Failed` 로 내게 하고, 퀘스트 트리에는 실패 시 같은 상태를 재진입하는 전이를 둔다. 저작 영향 때문에 현행을 유지하기로 결정한다면, "종료되면 Succeeded"라고만 적은 `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:25` 에 사망·겹침·진행 중 해석 실패도 성공으로 보고된다고 명시해 저작 쪽이 알고 쓰게 한다.
- **확신도**: 높음 (경로는 코드로 확정. 일괄 `Succeeded` 자체는 의도일 수 있으나 `:97` 문구·태스크 헤더 `:28` 의 실패 규정과 어긋나고, 결정이 미결로 남아 있다)

### 2. 🟢 포즈를 얹을 수 없는 요청도 앞 스트리밍을 취소하고 몽타주를 끝까지 로드한 뒤에야 버린다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:313-330`(취소·요청), `:350-360`(대상 판정)
- **범주**: 성능/안전
- **문제**: 포즈를 얹을 수 있는지는 대상이 `AWxDialogueActor` 이고 `GetPoseMesh()` 에 애님 인스턴스가 있느냐로 갈리는데, 이 판정을 로드가 끝난 뒤의 `PlayPendingPose()` 에서야 한다. 판정 재료인 `CurrentTarget` 은 요청 시점(`:320`)에 이미 손에 있다.
  - `Play Dialogue` 는 항상 대상 없이 연다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:39`). 트리가 고른 행에 `TargetPose` 가 있으면 대사마다 몽타주를 스트리밍했다가 "애님 BP 없음"을 가정한 경고(`:356-358`)를 대상 `None` 으로 찍고 버린다. `GetPoseMesh()` 기본값이 `nullptr` 인 파생(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:22-25`)도 같은 길이다.
  - 취소(`:313-317`)가 대상 판정보다 앞이라, 이 헛요청이 앞 대화가 다른 대상에 걸어 둔 스트리밍까지 끊는다. `EndDialogue()` 는 끝난 대화의 포즈가 늦게라도 얹히도록 스트리밍을 일부러 남기는데(`:227`), NPC 대화의 포즈 로드 중 포즈 있는 행으로 `Play Dialogue` 가 겹쳐 열리면 그 NPC 포즈는 끝내 재생되지 않는다.
  - 현재 콘텐츠에서는 잠복 상태다. `Content/DesignerTables/DT_Dialogue.uasset` 을 바이너리로 확인한 결과 `TargetPose` 가 채워진 행은 NPC 체인의 `BP_Npc_5`(`AM_Death`) 하나이고, 퀘스트 트리가 쓰는 `ST_Quest_Main1_Success` 행은 비어 있으며, 유일한 파생 `AWxNpc` 는 메시를 돌려준다(`Source/WxGame/Character/WxNpc.cpp:46-49`). 같은 테이블을 NPC 와 퀘스트 트리가 함께 쓰므로 퀘스트 행에 포즈를 넣는 순간 드러난다.
- **제안**: `ApplyCurrentPose()` 에서 행 확인 직후 `CurrentTarget` 을 `AWxDialogueActor` 로 캐스팅해 `GetPoseMesh()` 가 없으면 취소·로드 전에 돌아간다(대상 없음은 경고 없이). 애님 인스턴스 경고는 메시가 있는데 인스턴스가 없을 때만 남긴다.
- **확신도**: 높음 (코드 순서는 확정, 영향은 현재 잠복)

### 3. 🟢 `UWxDialogueComponent` 에 `BlueprintSpawnableComponent` 가 남아 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:16`
- **범주**: 설계/구조
- **문제**: 상호작용 계약이 `AWxDialogueActor` 로 옮겨진 뒤 이 컴포넌트는 붙이기만 해서는 아무 일도 하지 않는다 — 헤더 스스로 그렇게 적었고(`:13-14`), 호스트는 자기 네이티브 서브오브젝트만 본다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:9`·`:14`). 그런데 컴포넌트 추가 메뉴에는 여전히 노출돼, 일반 액터에 붙이거나 파생 BP 에 하나 더 붙여 `StartRow` 를 채우는 실수가 조용히 무시된다. 계약 이관 워크로그(`.claude/worklog/2026-08-21-상호작용-계약-액터-이관.md:85`)는 BP 리페어런트가 끝난 뒤 떼기로 미뤘고, 같은 처지였던 WxWorld 장치 컴포넌트는 이미 뗐다(`Plugins/WxWorld/Source` 에 이 메타 0건). 대화 쪽 호스트 파생은 이미 `AWxNpc` 로 옮겨져 있어 미룬 이유가 남아 있지 않다.
- **제안**: `meta = (BlueprintSpawnableComponent)` 를 제거한다. 메타는 추가 메뉴 노출에만 쓰여 기존 부착분 로드에는 영향이 없다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp` — 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, 최근 커밋 워크로그(`.claude/worklog/2026-09-14-대화-세션-빙의-전이-종료.md`, `.claude/worklog/2026-09-14-대화카메라-카메라매니저-가드.md`, `.claude/worklog/2026-09-14-대화세션-미사용-접근자-제거.md`)
- **미검토 / 한계**:
  - 빙의 전이 종료(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:165-171`)는 엔진 5.8 `AController::UnPossess` 가 폰을 비운 뒤 발화하고 `Possess` 가 기존 폰을 `UnPossess` 로 먼저 넘기며, UI 매니저가 같은 전이에서 창을 닫는다는 것까지 코드로 확인했다. PIE 실측은 없다(워크로그도 미실측).
  - 복제 경로는 모듈이 문서화한 v1 싱글/리슨 호스트 전제(`Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:29`) 안에서만 검증했다. 전제 밖 RPC 인자 해소, 클라 로컬 루즈 태그와 서버측 `ActivationBlockedTags`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:42` 의 동기 `HasActiveDialogue()` 판정은 명시된 한계라 세지 않았다.
  - 현재 실현 경로가 없어 세지 않은 잠복 항목이 두 가지 있다. 첫째, `OnDialogueEnded` 가 `Broadcast()` 직후 `Clear()`(`:230-231`)라 발화 중 새로 붙은 구독까지 지워진다. 유일한 구독자의 `FinishTask` 는 동기 재진입하지 않는다. 둘째, 겹침 시 앞 세션을 새 세션 검증(`:136-150`)보다 먼저 접는다(`:129-132`). 새 행이 데이터 오류면 앞 대화만 사라지지만, 태스크가 행 지정을 선검사하고 폰 부재는 빙의 전이가 이미 접으므로 남는 경우는 행 누락·빈 대사뿐이다.
  - NPC 상호작용은 `AWxDialogueActor::OnInteracted` 로 NPC 의 `StartRow` 대화를 열고(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:12-15`), 같은 호출에서 `FWxStateTreeTask_WaitForInteraction::NotifyInteracted` 로 퀘스트를 진행시킨다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:84-87`). 퀘스트 트리가 그 직후 `Play Dialogue` 를 두면 방금 연 NPC 대화가 `:131` 에서 접힌다. 트리의 상태 순서는 바이너리라 읽지 못해 발견으로 세지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera`)의 실제 화면 결과, 대화 테이블의 데이터 정합(순환 `NextRow` 등), BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 11파일 — `/module-review`로 갱신*
