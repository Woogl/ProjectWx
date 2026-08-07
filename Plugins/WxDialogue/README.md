# WxDialogue — 대화 시스템

> NPC·기믹과의 대사 진행을 담당하는 도메인 플러그인. 대화 정의는 대상 액터에, 세션 진행은 플레이어 컨트롤러에 나누어 두고, DataTable 행을 노드 삼아 대사·포즈·카메라를 이어간다. StateTree 노드를 함께 제공해 퀘스트 등 소비 도메인이 대화 모듈을 참조하지 않고도 대화를 열고 관찰한다.

## 책임
**담당**
- 대화 데이터 모델(`FWxDialogueTableRow` 기반 테이블 = 대화 1편, 행 = 대사 1줄, `NextRow`로 선형 진행, 종료는 `NextRow=None`)
- 대화 정의 보유(`UWxDialogueComponent`, 대상 액터에 부착, 시작 행만 보유) 및 세션 진행(`UWxDialogueSessionComponent`, PlayerController에 주입)
- 대화 연출: 전용 카메라 구도·전환, 대사별 대상 포즈(AnimMontage) 비동기 스트리밍·재생, 세션 중 폰 ASC의 `State.Dialogue` 태그 부여
- 대화 NPC 액터(`AWxNpc`, `IWxInteractable` 구현) 및 상호작용 영역 콜리전 토글
- 대화용 StateTree 태스크(관찰/출력/NPC 토글) 제공

**경계 (비담당)**
- 대화창 UI·위젯·뷰모델 — 대사 변경 델리게이트와 `State.Dialogue` 태그만 발행, 렌더링은 [[WxUI]] 몫
- 대화의 의미 해석·기록(수주/납품 판정)과 대화를 여는 퀘스트 진행 — 세션은 현재 행 신원만 노출, 위임 [[WxQuest]]
- 상호작용 스캔·발동·프롬프트 계약(`IWxInteractable`)·`State.Dialogue` 태그 정의 — 위임 [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]](`IWxInteractable` 상호작용 계약·`State.Dialogue` 태그 정의), StateTreeModule(대화 태스크), UniversalObjectLocator(NPC 배치 액터 지목), GameplayAbilities(폰 ASC 태그 발행), ModularGameplay(컨트롤러 컴포넌트 주입)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 `WxCore`만 참조. WxUI/WxQuest는 역방향 소비자로 이 모듈이 참조하지 않는다.)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxDialogueTableRow` | 대화 노드 = 대사 한 줄(화자·본문·`TargetPose`·`NextRow`). 대화 데이터의 기본 단위 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` |
| `UWxDialogueSessionComponent` | PC에 주입되는 세션. 진입·진행·카메라·포즈·태그를 소유하는 모듈 심장부 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` |
| `UWxDialogueComponent` | 대상 액터가 보유하는 대화 정의. 시작 행만 들고 세션에 넘긴다 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h` |
| `AWxNpc` | 대화 NPC 베이스(Abstract). 메시가 상호작용 영역, 상호작용을 세션에 위임 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxNpc.h` |
| `FWxStateTreeTask_WaitDialogueCompleted` | 지정 대사를 거친 대화 완주를 관찰(퀘스트 게이트) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |
| `FWxStateTreeTask_PlayDialogue` | 트리가 대사를 열어 연출(독백·무전 등) | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |
| `FWxStateTreeTask_EnableNpcInteraction` | 지정 NPC들의 상호작용을 (Targets, bEnable)로 토글 | `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` |

## 확장 포인트 / 규약
- **새 대화 추가**: `FWxDialogueTableRow` 타입 DataTable 1개 = 대화 1편. 행 이름이 노드 이름, `NextRow`로 선형 진행(분기/선택지 없음), `None`이면 종료. 모든 행의 `Line`은 채워야 하며 빈 대사는 잘못된 행으로 접힌다. `TargetPose`는 소프트 참조라 대사를 넘길 때 비동기 스트리밍되므로 포즈를 많이 걸어도 레벨 로드 비용이 늘지 않는다.
- **새 NPC 추가**: `AWxNpc`(Abstract) 상속 후 인스턴스별 `StartRow`·`NpcName` 지정. 상호작용 계약은 `IWxInteractable`(WxCore)로 이미 구현. 잠긴 채 시작할 NPC는 별도 플래그 없이 영역 메시 콜리전을 배치 인스턴스에서 미리 꺼 둔다.
- **대화 진입 두 경로**: 액터 기반은 상호작용 응답이 `StartDialogue(UWxDialogueComponent*)`, 비-액터(퀘스트 ST)는 `StartDialogueRow(RowHandle, Target)`(Target 비우면 카메라가 플레이어에 머무는 나레이션). 서버 권위 진입 → 소유 클라 `ClientStartDialogue`(Client RPC)로 세션 오픈. 세션은 표시 전용 로컬 상태(v1 싱글/리슨 호스트 전제, 서버 검증 없음).
- **퀘스트 연동(데이터 주도)**: 소비 도메인은 위 StateTree 태스크를 에셋에서 골라 쓴다 — 대화 모듈을 코드로 참조하지 않는다. 완주 게이트는 `Wait Dialogue Completed`(시작 행=대화 전체, 중간·끝 행=그 대사까지), 트리가 대사를 소유하면 `Play Dialogue`, NPC 잠금·해제는 `Enable Npc Interaction`(되돌리지 않는 월드 변경, 완료 판정 제외). 세 노드 모두 0번 컨트롤러 세션을 폴링·관찰.
- **카메라 조정**: 주입 컴포넌트라 세션 헤더의 `Camera*` 기본값(FOV·오프축각·거리·높이·블렌드)이 곧 실제 값. 에셋으로 바꾸려면 이 클래스의 BP 서브클래스를 만들어 주입 액션에 등록.

## 여기서부터 읽어라
1. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueSessionComponent.h` — 시스템의 중심. 소유·RPC·카메라·포즈·태그의 설계 근거가 헤더 주석에 집약
2. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueTableRow.h` — 대화 표현(노드=대사 한 줄=행)을 먼저 잡아야 세션 순회가 읽힌다
3. `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueSessionComponent.cpp` — `EnterRow`/`Advance` 순회, `State.Dialogue` 태그 부착/해제, 카메라 구도 산출의 실제 흐름
4. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueStateTreeNodes.h` — 관찰(Wait)/출력(Play)/토글(Enable) 세 노드의 역할 분리와 완주 판정 규약

## 관련
- 소비자: [[WxUI]](태그·델리게이트 구독으로 위젯 연결), [[WxQuest]](ST 노드로 대화 관찰·출력)
- 상위 계약: [[WxCore]](`IWxInteractable`·`State.Dialogue` 태그)

---
*문서 기준 커밋 `bb8ee6b` · 생성일 2026-08-07 · 소스 11파일 — `/readme-writer`로 갱신*
