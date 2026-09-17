# WxDialogue — 코드 리뷰

> 11파일짜리 작은 모듈로 호스트 액터 / 대화 정의 / PC 세션의 책임 분리가 선명하고, 이전 리뷰(`7d1d0374`) 이후 소스 변경이 없어 이전 발견 3건이 그대로 유효하다(라인도 동일). 새로 찾은 것은 오늘 `CLAUDE.md` 규칙 번호가 바뀌면서 남은 낡은 사유 주석 하나다. 이번 리뷰는 모듈 11파일을 전부 읽고 세션 컴포넌트·`Play Dialogue` 태스크를 정독했으며, 엔진 5.8 StateTree 약한 컨텍스트·`AController` 빙의 발화 경로와 소비자(WxGame)·에셋 참조까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 종료 신호에 사유가 없어, 사망·겹침·데이터 오류로 끊긴 대화도 `Play Dialogue` 가 `Succeeded` 로 끝난다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:230`(사유 없는 단일 발화) ← 호출부 `:84`·`:91`·`:100`·`:131`·`:169` → `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:50-53`
- **범주**: 설계/구조
- **문제**: `EndDialogue()` 는 `OnDialogueEnded.Broadcast()` 만 하므로 태스크 리스너는 어떤 경우든 `FinishTask(EStateTreeFinishTaskType::Succeeded)` 를 낸다. 그러나 호출부 다섯 곳 중 정상 완주는 `NextRow = None` 인 `:91` 하나뿐이다.
  - `:169` 빙의 전이: 대화 중 사망 후 리스폰하면 `Source/WxGame/Framework/WxRespawnLibrary.cpp:47` 의 `UnPossess` 가 세션을 접고, 그 대화를 기다리던 퀘스트 단계는 플레이어가 읽지 않은 대사를 완료로 넘긴다. 수정 워크로그(`.claude/worklog/2026-09-14-대화-세션-빙의-전이-종료.md:63`)가 이 문제를 미결 후속 과제로 남겼고, 이후 결정 기록은 없다.
  - `:131` 겹침: 다른 `Play Dialogue` 가 앞 대화를 접으면 앞 태스크도 성공으로 끝난다. 이 경로가 실제로 있다는 것은 `:127` 주석이 직접 밝히고 있다.
  - `:100` 다음 행 해석 실패: 바로 위 `:97` 주석은 이를 "정상 종료가 아니다"라고 적었지만 결과는 성공으로 보고된다. 같은 데이터 오류라도 첫 행에서 나면 `Failed`(`Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:28`), 두 번째 행 이후에서 나면 `Succeeded` 가 되므로 `NextRow` 오타 하나로 퀘스트가 그대로 통과된다.
  - `:84` 세션 도중 테이블 재임포트(에디터 한정).
- **제안**: `OnDialogueEnded` 에 완주 여부(bool 또는 종료 사유 enum)를 실어 `:91` 외의 종료는 태스크가 `Failed` 로 내게 하고, 퀘스트 트리에는 실패하면 같은 상태로 다시 들어가는 전이를 둔다. 저작 영향 때문에 현행을 유지하기로 한다면, "종료되면 Succeeded"라고만 적힌 `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:25` 에 사망·겹침·진행 중 해석 실패도 성공으로 보고된다는 점을 명시한다.
- **확신도**: 높음 (경로는 코드로 확정했다. 일괄 `Succeeded` 자체는 의도일 수 있으나 `:97` 주석과 태스크 헤더 `:28` 의 실패 규정에 어긋나고, 결정도 아직 미결이다)

### 2. 🟢 포즈 스트리밍 슬롯 하나를 모든 대상이 공유해, 포즈를 얹을 수 없는 요청도 다른 대상의 대기 중 포즈를 취소한다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:313-330`(취소·요청), `:350-360`(대상 판정), `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:152-159`(단일 슬롯)
- **범주**: 성능/안전
- **문제**: 포즈를 얹을 수 있는지는 대상이 `AWxDialogueActor` 이고 `GetPoseMesh()` 에 애님 인스턴스가 있는지로 정해진다. 그런데 이 판정은 로드가 끝난 뒤 `PlayPendingPose()` 에서야 이루어진다. 판정에 필요한 `CurrentTarget` 은 요청 시점(`:320`)에 이미 알고 있다.
  - `Play Dialogue` 는 항상 대상 없이 대화를 연다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:39`). 그래서 트리가 고른 행에 `TargetPose` 가 있으면 대사마다 몽타주를 끝까지 스트리밍한 뒤, 애님 BP가 없는 상황을 가정한 경고(`:356-358`)를 대상 `None` 으로 찍고 버린다. `GetPoseMesh()` 기본값이 `nullptr` 인 파생(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:22-25`)도 같은 경로를 탄다.
  - 취소(`:313-317`)가 대상 판정보다 먼저 실행되고 슬롯(`PendingPose`/`PendingPoseTarget`/`PoseLoadHandle`)도 하나뿐이다. 따라서 새 요청은 대상이 달라도 앞 요청을 끊는다. `EndDialogue()` 는 끝난 대화의 포즈가 늦게라도 얹히도록 스트리밍을 일부러 남기는데(`:227`), NPC 포즈를 로드하는 도중에 포즈가 있는 행으로 `Play Dialogue` 가 겹쳐 열리면(또는 곧바로 다른 NPC 에게 말을 걸면) 앞 NPC 의 포즈는 끝내 재생되지 않는다.
  - 현재 콘텐츠에서는 드러나지 않은 상태다. `Content/DesignerTables/DT_Dialogue.uasset` 이 참조하는 몽타주는 `AM_Death` 하나뿐이고, 유일한 파생 `AWxNpc` 는 메시를 돌려준다(`Source/WxGame/Character/WxNpc.cpp:46-49`). 다만 NPC 와 퀘스트 트리가 같은 테이블(`BP_Npc_*`·`ST_Quest_Main1_Success` 행)을 함께 쓰므로, 퀘스트 행에 포즈를 넣는 순간 드러난다.
- **제안**: `ApplyCurrentPose()` 에서 행을 확인한 직후 `CurrentTarget` 을 `AWxDialogueActor` 로 캐스팅하고, `GetPoseMesh()` 가 없으면 취소·로드 전에 돌아간다(대상이 없을 때는 경고도 남기지 않는다). 애님 인스턴스 경고는 메시가 있는데 인스턴스가 없을 때만 남긴다. 대상 간 취소까지 없애려면 `FStreamableDelegate::CreateUObject` 에 포즈·대상을 페이로드로 묶어 요청마다 따로 들게 하고, 취소는 같은 대상의 요청끼리만 한다. 헤더 `:153` 은 "완료 콜백이 인자를 받지 않으므로" 슬롯에 남긴다고 적었지만, 페이로드 바인딩을 쓰면 그 제약이 없다.
- **확신도**: 높음 (코드 순서는 확정했고, 영향은 아직 드러나지 않았다)

### 3. 🟢 `UWxDialogueComponent` 에 `BlueprintSpawnableComponent` 가 남아 있다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:16`
- **범주**: 설계/구조
- **문제**: 상호작용 계약이 `AWxDialogueActor` 로 옮겨진 뒤로 이 컴포넌트는 붙이기만 해서는 아무 일도 하지 않는다. 헤더도 그렇게 적었고(`:13-14`), 호스트는 자기 네이티브 서브오브젝트만 본다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:9`·`:14`). 그런데 컴포넌트 추가 메뉴에는 여전히 노출되므로, 일반 액터에 붙이거나 파생 BP 에 하나 더 붙여 `StartRow` 를 채우는 실수가 경고 없이 무시된다. 계약 이관 워크로그(`.claude/worklog/2026-08-21-상호작용-계약-액터-이관.md:85`)는 BP 리페어런트가 끝난 뒤 이 메타를 떼기로 미뤘고, 같은 처지였던 WxWorld 쪽은 이미 뗐다(`Plugins/WxWorld/Source` 에 이 메타 0건). 대화 쪽 리페어런트도 끝난 상태다. 이 컴포넌트를 참조하는 BP 는 `Content/WorldObject/Npc/BP_Npc.uasset`·`BP_Npc1.uasset` 둘뿐이고, 둘 다 부모가 `WxNpc` 이며 SCS 변수는 `DefaultSceneRoot`·`TextRender` 뿐이라 따로 추가한 사본이 없다.
- **제안**: `meta = (BlueprintSpawnableComponent)` 를 제거한다. 이 메타는 추가 메뉴 노출에만 쓰이므로 기존 부착분 로드에는 영향이 없다.
- **확신도**: 중간

### 4. 🟢 규칙 개정(`5fe1ceb6`) 뒤 예외 사유 주석이 존재하지 않는 "코딩 규칙 4"를 가리킨다
- **위치**: `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h:13`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:48`
- **범주**: 규칙 위반
- **문제**: 규칙 3은 헤더 내 `GetInstanceDataType()` 정의에 예외 사유 주석을 요구한다. `:13` 의 사유 자체는 타당하지만, 오늘 커밋 `5fe1ceb6` 에서 람다 규칙이 삭제되면서 인라인 금지 규칙이 4번에서 3번으로 바뀌었다. 그 결과 이 주석은 이제 없는 번호를 인용한다. 같은 커밋으로 람다 제한도 사라졌으므로, `.cpp:48` 의 "여기선 람다를 쓴다" 사유 문장은 더 이상 어떤 규칙에도 대응하지 않는 잔여 설명이다. 바로 아래 `:49` 의 등록 수명 설명은 여전히 유효하다(엔진 5.8 `FActiveStateID` 는 상태 진입마다 새로 발급되므로 먼저 떠난 노드의 컨텍스트는 `GetActivePathInfo` 에서 무효 처리된다).
- **제안**: `:13` 을 "코딩 규칙 3"으로 고치고 `.cpp:48` 한 줄을 지운다. 같은 문구가 Source/Plugins 전체 27개 파일에 있으므로 모듈별로 고치기보다 한 번에 일괄 치환한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxStateTreeTask_PlayDialogue.h`
- **훑은 파일**: `Plugins/WxDialogue/README.md`, `Plugins/WxDialogue/WxDialogue.uplugin`, `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueActor.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h`, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueModule.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueModule.cpp`. 계약 확인용으로 `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`(퀘스트 ST 오너가 액터인지), 엔진 5.8 `StateTreeAsyncExecutionContext.cpp`·`StateTreeInstanceData.h`(약한 컨텍스트 무효화·재귀 쓰기 접근 허용), `Controller.cpp`(`OnPossessedPawnChanged` 가 `Possess`·`UnPossess`·`OnRep_Pawn` 세 곳에서 발화하는 것)를 교차 확인했다.
- **미검토 / 한계**:
  - 규칙 준수: 11파일 모두 저작권 첫 줄과 `Wx` prefix를 지키고, Wx 플러그인 의존은 `WxCore` 뿐이다(Build.cs·uplugin·include). 헤더 본문 정의는 사유 주석이 달린 `GetInstanceDataType()` 하나다(번호 문제는 4번).
  - 빙의 전이 종료(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp:165-171`)는 원격 클라에서도 `OnRep_Pawn` 발화로 동작하는 것을 코드로 확인했다. PIE 실측은 없다(워크로그도 미실측).
  - 복제 경로는 모듈이 문서화한 v1 싱글/리슨 호스트 전제(`Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h:29`) 안에서만 검증했다. 전제 밖의 RPC 인자 해소, 클라 로컬 루즈 태그와 서버측 `ActivationBlockedTags`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxStateTreeTask_PlayDialogue.cpp:42` 의 동기 `HasActiveDialogue()` 판정은 명시된 한계라 발견으로 세지 않았다.
  - 현재 실현 경로가 없어 세지 않은 잠복 항목이 세 가지다. 첫째, `OnDialogueEnded` 는 `Broadcast()` 직후 `Clear()`(`:230-231`)하므로 발화 중에 새로 붙은 구독까지 지워진다. 다만 유일한 구독자의 `FinishTask` 는 `bHasPendingCompletedState` 표시와 다음 틱 예약만 하고 동기로 재진입하지 않는다(엔진 확인). 둘째, 겹쳐 열릴 때 앞 세션을 새 세션 검증(`:136-150`)보다 먼저 접는다(`:129-132`). 새 행이 데이터 오류면 앞 대화만 사라지지만, 태스크가 행 지정을 먼저 검사하고 폰 부재는 빙의 전이가 이미 처리하므로 남는 경우는 행 누락·빈 대사뿐이다. 셋째, `EndPlay`(`:39-47`)는 진행 중 세션을 접지 않는다. 그래서 대화 카메라 수명 부여나 종료 발화가 생략될 수 있지만, 세션을 가진 쪽(로컬 PC)이 월드보다 먼저 파괴되는 경로를 찾지 못했다.
  - NPC 상호작용은 `AWxDialogueActor::OnInteracted` 로 NPC 의 `StartRow` 대화를 열고, 같은 호출에서 퀘스트 대기 태스크를 진행시킨다. 퀘스트 트리가 그 직후 `Play Dialogue` 를 두면 방금 연 NPC 대화가 `:131` 에서 접힌다. 트리의 상태 순서는 바이너리라 읽지 못해 발견으로 세지 않았다.
  - 카메라 구도 수식(`BeginDialogueCamera`)의 실제 화면 결과, 대화 테이블 데이터 정합(순환 `NextRow` 등, 행별 `TargetPose` 배치는 문자열 추출로만 확인), BP/WBP/StateTree 에셋 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 11파일 — `/module-review`로 갱신*
