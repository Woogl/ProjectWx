# 기믹 StateTree 실행을 AWxDevice 로 흡수

## 계획

### 목표
`UWxGimmickStateTreeComponent` 를 없애고 `AWxDevice` 가 StateTree 를 직접 실행한다. 컴포넌트에 남아 있던 것들(트리 접근자 4개 + 상호작용 배선)은 엔진 `UStateTreeComponent` 의 protected 멤버를 요구해서 밖으로 못 뺐던 것인데, 그 멤버들은 상속으로만 얻는 것이 아니라 **직접 들면 되는 것들**이다. 엔진이 `StateTreeExecutionContext.h` 클래스 주석에 커스텀 호스트 규약을 명시해 두었고, 그것을 따르면 액터 하나가 상태·트리·상호작용을 전부 든다.

부수 효과로 「컴포넌트→액터 한 방향 틱 결합」·「호스트가 아닌 액터에 붙었을 때의 무동작 분기」·`FindComponentByClass` 조회 한 단·`UBrainComponent`(AI) 상속 계보가 함께 사라진다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxDevice.h` · `.cpp` | ST 실행 일체 흡수(트리 참조·인스턴스 데이터·틱·시작/정지·접근자·상호작용 배선·실행 확장) | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeComponent.h` · `.cpp` | 파일 삭제 | 삭제 |
| `Plugins/WxWorld/.../Interaction/WxStateTreeTask_EnableInteraction.cpp` | 컴포넌트 조회 → `Cast<AWxDevice>(Context.GetOwner())` | 수정 |
| `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs` | `AIModule` 제거, `GameplayStateTreeModule` 을 Private 으로 강등 | 수정 |
| `Plugins/WxWorld/README.md` | 컴포넌트 행·문장 삭제, 의존성 설명 정정 | 수정 |

ST 에셋 4종은 손대지 않는다 — 스키마를 `UStateTreeComponentSchema` 그대로 두는 것이 이 설계의 전제다.

### 접근 방식
- **컨텍스트 세팅은 한 줄, 전용 함수는 두지 않는다**: `UStateTreeComponentSchema::SetContextRequirements` 는 `UBrainComponent&` 를 받아 못 쓴다. 대신 `Context.SetContextDataByName(TEXT("Actor"), FStateTreeDataView(this))` 한 줄을 시작·틱·정지 세 자리에 적는다. Actor 컨텍스트 세팅은 필수다 — ST 에셋이 `TargetComponent`·스플라인을 Context 액터의 컴포넌트로 바인딩한다. `SetCollectExternalDataCallback` 은 넣지 않는다(외부 데이터를 선언한 태스크 0건). `AreContextDataViewsValid()` 검사는 시작 한 곳에서만 — 어긋난 에셋은 거기서 걸려 아예 안 열린다.
- **틱은 액터 자기 틱**: 이동·대기 태스크가 틱을 요구하고 대행 서브시스템이 없다. `bCanEverTick=true`·`bStartWithTickEnabled=false`·`TickGroup=TG_DuringPhysics`(컴포넌트 시절 그룹 유지). `GetNextScheduledTick()` 을 `PrimaryActorTick` 에 반영해 잠자기 최적화를 유지한다.
- **발행·추종은 여전히 트리 틱 직후**: 실행 컨텍스트를 블록으로 감싸 파괴한 뒤 `SyncStateWithTree()`. 한 방향 호출이 함수 호출 순서로 바뀔 뿐 판정 전제는 같다. 정지 시 마지막 발행은 `EndPlay` 가 맡는다.
- **상호작용 배선도 액터로**: `FWxDeviceInteractionBinding` 으로 이관, `SetInteractionBinding` 만 public.

### 함께 덜어내는 것
`NotifyHostStateSync`·`ResetInteractions`·`SetInteractionEnabled` 중복·위임 홉(`IsInteractionEnabled`/`GetInteractionPrompt`)·`HasState`·`ConditionalEnableTick`/`DisableTick` 3단·`StartAtState` 폴백 분기·`UBrainComponent` 계보 전부(`StartLogic`~`Cleanup`·`bIsPaused`·`IGameplayTaskOwnerInterface`)·`bStartLogicAutomatically`·`OnStateTreeRunStatusChanged`·`BlueprintSpawnableComponent`·doc-comment 이중화.
새로 만들지 않는 것: `SetContextRequirements`·`CollectExternalData` 오버라이드·`ValidateStateTreeReference`·`TickTree` 보조 함수·재진입 가드.

### 호환성
- **에디터 작업 필요**: ST 에셋 지정이 컴포넌트 서브오브젝트에 실려 있어 이관되지 않는다. BP 4종에 같은 이름 에셋을 다시 지정해야 한다(`BP_Door`→`ST_Door` 등). 배치 인스턴스 11건은 컴포넌트 이름만 들고 있어 맵 재저장으로 정리된다.
- 세이브 영향 없음 — `StateTag` 는 이미 액터 직렬화다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Device/WxDevice.h` · `.cpp` | `FStateTreeReference`·`FStateTreeInstanceData`·`bTreeRunning` 을 들고 `Tick`·`StartTree`·`StopTree`·`GetActiveStateTag`·`RequestState`·`ResolveState`·`ScheduleTickFrame` 신설, 상호작용 배선(`FWxDeviceInteractionBinding`·`SetInteractionBinding`·`BroadcastInteractionDelegate`)과 실행 확장 이관 | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeComponent.h` · `.cpp` | 삭제 | 삭제 |
| `Plugins/WxWorld/.../Gimmick/` → `.../Device/` (Public·Private 각 13파일) | 폴더명 변경, 전 `#include "Gimmick/…"` → `"Device/…"` 및 정렬 | 이동 |
| `Plugins/WxWorld/.../Interaction/WxStateTreeTask_EnableInteraction.h` · `.cpp` | 컴포넌트 조회 → `Cast<AWxDevice>(Context.GetOwner())` | 수정 |
| `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs` | `AIModule` 제거, `GameplayStateTreeModule` Private 강등, 나머지 의존 사유 주석 정정 | 수정 |
| WxWorld·WxCore·WxUI·WxGame 의 주석 21파일 | 「기믹」 → 「장치」 (조사까지 교정) | 수정 |
| `Plugins/WxWorld/README.md` · `Plugins/WxCore/README.md` · `.claude/skills/design-checklist/SKILL.md` | 컴포넌트 행·경로·확장 절차·의존성 설명 정정 | 수정 |

### 구현·결정과 그 이유
- **컨텍스트는 한 줄로 채운다**: `UStateTreeComponentSchema::SetContextRequirements` 가 `UBrainComponent&` 를 받아 쓸 수 없어, 엔진이 `StateTreeExecutionContext.h` 주석에 적어 둔 커스텀 호스트 규약 중 실제로 필요한 것만 남겼다 — `SetContextDataByName(TEXT("Actor"), this)` 뿐이다. Actor 컨텍스트는 필수인데, ST 에셋이 `ComponentMove`·`ComponentSplineMove` 의 대상 컴포넌트를 Context 액터로 바인딩하기 때문이다. 반대로 `SetCollectExternalDataCallback` 은 넣지 않았다 — 외부 데이터를 선언한 노드가 WxWorld·WxQuest 통틀어 0건이라 불릴 자리가 없다.
- **검증 게이트는 시작 한 곳**: 에셋 유효성(`IsReadyToRun`)·스키마 파생·`AreContextDataViewsValid` 를 `StartTree` 에서만 본다. 어긋난 에셋은 여기서 Error 로그와 함께 아예 열리지 않으므로 틱·정지에서 다시 볼 이유가 없다.
- **틱은 액터가 직접, 그러나 대부분 꺼져 있다**: 이동·대기 태스크가 틱을 요구하고 대행 서브시스템이 없어 틱 자체는 뺄 수 없다. `GetNextScheduledTick()` 을 `PrimaryActorTick` 에 반영해 잠자기 최적화를 그대로 살렸고, 틱 그룹은 컴포넌트 시절의 `TG_DuringPhysics` 를 명시해 이동 태스크가 프레임 안에서 도는 자리를 바꾸지 않았다(액터 기본값은 `TG_PrePhysics`).
- **틱 컨텍스트를 블록으로 감쌌다**: 발행·추종은 트리 틱 직후여야 하는데, 같은 인스턴스 데이터에 쓰기 컨텍스트가 겹치면 안 된다. 보조 함수를 만드는 대신 블록 스코프로 컨텍스트를 죽이고 `SyncStateWithTree()` 를 부른다.
- **시작 경로가 하나로 줄었다**: 컴포넌트판은 「태그를 못 찾으면 `Super::StartLogic()` 으로 다시 시작」하는 폴백 갈래가 따로 있었다. 이제 상태 override 만 조건부로 채우므로 갈래가 사라지고 시작 후 동기화도 한 번뿐이다.
- **함께 사라진 것들**: `NotifyHostStateSync`(액터를 찾아 부르던 다리)·`ResetInteractions`·`SetInteractionEnabled` 중복 선언·`IsInteractionEnabled`/`GetInteractionPrompt` 위임 홉·`HasState`·`ConditionalEnableTick`/`DisableTick` 3단·`UBrainComponent` 계보 전부(`StartLogic`~`Cleanup`·`bIsPaused`·`IGameplayTaskOwnerInterface`)·`bStartLogicAutomatically`·`OnStateTreeRunStatusChanged`·`BlueprintSpawnableComponent`. 서버 권위 패턴을 두 헤더가 중복 설명하던 doc-comment 도 액터 한 곳으로 모았다.
- **`ModularGameplay` 는 남겼다**: README 가 이것을 `UStateTreeComponent`=BrainComponent 탓으로 적어 두었지만 오기였다 — 스캐너의 `UControllerComponent` 가 그 모듈이다. 실제로 뺄 수 있었던 것은 `AIModule` 뿐이다.

### 계획 대비 달라진 점
- 작업 중 지시로 **낡은 `Gimmick` 어휘 정리**가 추가됐다. 소스 폴더 `Gimmick/` → `Device/`(Public·Private), 코드 주석의 「기믹」 → 「장치」, README·스킬 문서의 낡은 참조까지 함께 고쳤다.
- 저작 데이터에 실린 이름은 손대지 않았다 — `AWxTriggerDevice::Gimmicks`·`GimmickStateRequirements`(배치 인스턴스 저작), `Gimmick.*` GameplayTag 네임스페이스(ST 상태 Tag·세이브 값), `Content/WorldObject/Gimmick/` 폴더.

### 후속 과제
- **에디터 작업(필수)**: BP 4종의 액터 디테일 `State Tree` 에 에셋을 다시 지정한다 — 지정 값이 삭제된 컴포넌트 서브오브젝트에 실려 있어 이관되지 않는다. `BP_Door`→`ST_Door` · `BP_CheckPoint`→`ST_CheckPoint` · `BP_Elevator`→`ST_Elevator` · `BP_TreasureChest`→`ST_TreasureChest`. 이어서 `LV_DevCombat`·`LV_OpenWorld`·`LevelDesign/LevelInstance/SiegeCannonEmplacement01` 을 저장해 배치 인스턴스 11건의 고아 서브오브젝트 기록을 정리한다.
- **인게임 미검증** — 빌드 확인까지만 마쳤다. 확인할 것: 문·상자 여닫기(발행→전이), 엘리베이터 층별 레버 잠금과 이동 중 플랫폼 위 캐릭터(틱 그룹 유지 확인), 레버의 `Event.Interact` 전달, 체크포인트 세이브→복원 시 트리거형 태스크 비발동, 리슨 서버+클라 1의 상태·연출 일치.
- **남은 낡은 이름**(저작 데이터라 리네임에 CoreRedirect·에셋 편집이 필요하다): `AWxTriggerDevice::Gimmicks`·`GimmickStateRequirements`, `Gimmick.*` 태그 네임스페이스, `Content/WorldObject/Gimmick/` 폴더.
- `Docs/Programmer/module_review_*.md` 는 과거 리뷰 스냅샷이라 컴포넌트 언급이 남아 있다 — `/module-review` 재실행 시 갱신된다.
