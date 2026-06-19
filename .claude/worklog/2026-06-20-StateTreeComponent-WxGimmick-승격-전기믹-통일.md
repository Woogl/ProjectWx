# StateTreeComponent 를 AWxGimmick 으로 승격해 전 기믹 StateTree 통일

## 계획

### 목표
기믹 상태머신이 Door/Elevator(StateTree)와 세 콘솔/상자(C++ OnRep+ApplyState)로 이원화돼 있다. `UStateTreeComponent` 를 공통 부모 `AWxGimmick` 으로 끌어올리고, 세 콘솔/상자도 StateTree 구동으로 전환해 *C++ 는 권위 State 만 소유, StateTree 가 복제 State 를 추종하며 비주얼·인터랙션·사이드이펙트를 전부 적용* 하는 단일 계약으로 통일한다. 사이드이펙트까지 전부 StateTree 태스크로 옮긴다(옵션 B).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmick.h/.cpp` | `GimmickStateTree`(UStateTreeComponent) 추가, 생성자에서 `SetStartLogicAutomatically(false)`. StartLogic 은 자식이 호출 | 수정 |
| `WxDoor.h/.cpp` · `WxElevator.h/.cpp` | 자체 StateTree 멤버·생성·StartLogic 제거 → base `GimmickStateTree` 사용 | 수정 |
| `WxTreasureChest.*` · `WxAlarmConsole.*` · `WxSpawnConsole.*` | State 를 `Replicated`(OnRep 제거)로, `ApplyState`/`OnRep_State` 제거, BeginPlay 에서 `StartLogic`, 사이드이펙트(애니/FX/Respawn)·관련 필드 제거 | 수정 |
| `WxGimmickStateTreeNodes.h/.cpp` | 태스크 3종 추가: `PlaySkeletalAnim` / `PlayFx` / `TriggerSpawners` | 신규(추가) |

### 접근 방식
- **State 권위 + StateTree 추종**: 각 기믹의 State enum(복제+SaveGame)은 C++ 권위. StateTree 가 Enum Compare 전이로 복제 State 를 폴링해 추종(Door/Elevator 기존 패턴을 전 기믹으로 확장). RepNotify 불필요 → `Replicated`.
- **초기 진입 vs 라이브 전이**: 새 태스크 3종은 기존 공용 노드처럼 `!Transition.SourceStateID.IsValid()` 로 분기. 복원/시작=스냅 또는 스킵, 라이브 발동=재생/실행. 기존 `ApplyState`(복원)·`HandleInteracted`(발동) 로직과 1:1 매핑.
  - PlaySkeletalAnim: 초기=끝프레임 스냅, 라이브=PlayAnimation
  - PlayFx: 초기=스킵, 라이브=Niagara attach + Sound
  - TriggerSpawners: 초기=스킵, 라이브 && 권위에서만 Respawn
- **StartLogic 타이밍**: base BeginPlay 가 부르지 않고 각 자식 BeginPlay 끝에서 호출(Elevator 는 초기 위치 스냅 후 시작해야 하므로 타이밍 제어를 자식에 둔다).
- **에디터 후속(사용자)**: 컴포넌트명이 `GimmickStateTree` 로 통일돼 BP_Door/BP_Elevator 의 ST 에셋 할당이 끊김 → 재할당. ST_TreasureChest/AlarmConsole/SpawnConsole 신규 author + BP 할당. (C++ 만으로는 세 기믹이 일시 회귀)

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmick.h/.cpp` | StateTree 컴포넌트(서브오브젝트명 GimmickStateTree) 추가(SetStartLogicAutomatically(false)). `ApplyState` 후크·`OnWxSaveRestored` override 제거 | 수정 |
| `WxGimmickStateTreeNodes.h/.cpp` | `PlaySkeletalAnim`/`PlayFx`/`TriggerSpawners` 태스크 추가 | 수정(추가) |
| `WxDoor.h/.cpp` · `WxElevator.h/.cpp` | 자체 StateTree 제거 → base `GimmickStateTree` 사용 | 수정 |
| `WxTreasureChest.*` · `WxAlarmConsole.*` · `WxSpawnConsole.*` | State 를 `Replicated` 로, `ApplyState`/`OnRep_State` 제거, BeginPlay 에서 StartLogic, 사이드이펙트·관련 필드 제거 | 수정 |
| `WxLaserCorridor.h/.cpp` (WxGame) | base 의존 끊기: `ApplyState` override → 자체 `RefreshLaserState`, `OnWxSaveRestored` override 자체 추가 | 수정 |

### 구현·결정과 그 이유
- **복원을 StateTree 가 자동 추종하므로 base 의 복원 후크 제거**: 전이 조건(Enum Compare)이 매 틱 복제/복원된 State 를 폴링하므로, `ApplyState`/`OnWxSaveRestored`/`OnRep_State` 가 모두 불필요해진다. 세 콘솔/상자의 State 는 RepNotify 없는 `Replicated` 로 단순화했다(Door/Elevator 와 동일 계약).
- **사이드이펙트 전부 ST 태스크화(옵션 B)**: 애니/FX/Respawn 을 `!Transition.SourceStateID.IsValid()` 로 초기/라이브를 가르는 태스크로 옮겼다 — 기존 `ApplyState`(복원=스냅/스킵)·`HandleInteracted`(발동=재생/실행) 분기와 1:1 대응. C++ `HandleInteracted` 는 권위 State setter 만 남는다.
- **StartLogic 은 자식 BeginPlay 끝에서**: base 가 일괄 호출하면 자식 셋업(특히 Elevator 초기 위치 스냅) 전에 돌아 순서가 깨진다. 컴포넌트는 공통화하되 시작 호출만 자식이 쥔다.
- **WxLaserCorridor 는 StateTree 제외**: 주기 스폰·Tick 이동·활성 레이저 관리라는 지속 게임플레이 시뮬레이션이 본질이라 상태 적용기인 StateTree 에 부적합. base 의존만 끊고 자체 메서드로 기존 패턴을 유지했다(사용자 결정).

### 계획 대비 달라진 점
- **WxLaserCorridor(WxGame) 가 plan 에서 누락**: base `ApplyState` 제거로 빌드가 깨져 발견. 사용자 확인 후 StateTree 통일에서 제외하고 base 의존만 끊는 것으로 처리.
- **WxCutsceneTrigger/WxCheckPoint 는 무영향**: State 없는 트리거 기믹이라 ApplyState/복원 후크를 안 써 base 변경과 무관(상속된 GimmickStateTree 는 미사용).
- **WxElevator enum 이 작업 중 사용자에 의해 3단계(Closed/AtStart/AtEnd)로 동시 수정됨**: enum/문 로직은 건드리지 않고 StateTree 컴포넌트 전환만 적용.
- **UE 5.7 `Context.GetOwner()` 가 `TNotNull<UObject*>` 반환**: 새 태스크에서 `Cast<AActor>` 로 풀어 대입.

### 후속 과제
- **에디터 작업(사용자)**: BP_Door/BP_Elevator 의 `GimmickStateTree` 에 ST_Door/ST_Elevator 재할당(컴포넌트명 변경으로 끊김). ST_TreasureChest/AlarmConsole/SpawnConsole 신규 author + BP 할당. (완료 전까지 세 기믹 일시 회귀)
- **PIE 검증**: 각 기믹 동작 + 저장/복원 스냅 + 리슨 서버 2인 클라 추종.
