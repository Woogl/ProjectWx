# 기믹 State enum → GameplayTag 전환 (+ ST Required Event to Enter)

## 계획

### 목표
`AWxGimmick` 파생 7종의 자체 `EWx...State` enum 권위 상태를 GameplayTag 로 전환한다. SaveGame·서버 권위는 유지하고, ST 의 상태 선택을 엔진 표준 **Required Event to Enter**(상태 태그를 ST 이벤트로 보내 진입) 로 바꾼다. 상태 어휘를 태그로 통일해 ST 작업을 이벤트 구동으로 만들고 자식 코드를 가볍게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCore/.../WxGameplayTags.{h,cpp}` | `Gimmick.*` 상태 태그(7기믹) + `Gimmick.Restore` 마커 추가, `Event.GimmickStateChanged` 제거 | 수정 |
| `WxWorld/.../Gimmick/WxGimmick.{h,cpp}` | `Commit/Set/Get(FGameplayTag)`, OnRep 이 현재 상태 태그 송출, BeginPlay/OnWxSaveRestored 가 복원 시 상태 태그+Restore 마커 송출 | 수정 |
| `WxWorld/.../Gimmick/Wx{Door,Elevator,SpawnConsole,AlarmConsole,CutsceneTrigger,TreasureChest}.{h,cpp}` | enum 삭제, State `FGameplayTag` 화, 핸들러 태그화, `ReplicatedUsing` 통일 | 수정 |
| `WxGame/WorldObject/WxLaserCorridor.{h,cpp}` | 위와 동일 | 수정 |
| `WxWorld/.../Gimmick/WxGimmickStateTreeNodes.cpp` | 일회성/스냅 노드의 초기진입 판정을 복원 마커 인지로 보강(7개 노드) | 수정 |
| `WxInventory/.../Inventory/WxRewardStateTreeNodes.cpp` | Grant Reward 의 초기진입 판정 동일 보강 | 수정 |

### 접근 방식
- **상태 태그가 곧 권위 값이자 ST 이벤트**: 상태별 네이티브 태그 하나가 (a) 저장·복제되는 권위 값이고 (b) ST 로 보내는 이벤트(상태의 Required Event 와 매칭)다. 단일 어휘.
- **State 필드는 자식 소유 유지**: 대부분 Replicated+SaveGame, Cutscene 은 비저장 — 정책 차이 때문에 베이스 단일 필드로 호이스트하지 않는다. 베이스는 `GetGimmickState()` 가상 getter 로 현재 태그를 읽어 이벤트를 송출한다.
- **복원 동작 보존(핵심)**: Required Event 는 이벤트 게이트라 시작 시점엔 선택 안 됨 → 복원 시 베이스가 저장된 상태 태그를 이벤트로 재송출해 진입을 구동한다. 그 진입이 라이브 발동(보상 재지급·재스폰·FX 재생)으로 오인되지 않도록 `Gimmick.Restore` 마커 이벤트를 함께 보내고, 일회성 노드는 `FStateTreeExecutionContext::HasEventToProcess(Gimmick.Restore)` 로 이를 감지해 스냅·스킵한다(기존 `SourceStateID` 초기진입 판정에 OR 로 추가).

### 미수행(에디터 수동)
- ST 에셋 5종(Door/Elevator/SpawnConsole/CutsceneTrigger/TreasureChest) 의 EnterConditions 를 Enum Compare → Required Event to Enter(상태 태그) 로 재저작. `.uasset` 이라 코드로 불가 → 완료 기록에 체크리스트 제공.
- `ST_AlarmConsole`, `ST_LaserCorridor` 는 현재 미존재(두 기믹 ST 미배선 확인 필요).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCore/.../WxGameplayTags.{h,cpp}` | `Gimmick.<기믹>.<상태>` 16개 + `Gimmick.Restore` 마커 추가, `Event.GimmickStateChanged` 제거 | 수정 |
| `WxWorld/.../Gimmick/WxGimmick.{h,cpp}` | `Commit/Set/Get(FGameplayTag)`, OnRep→현재 상태 태그 송출, BeginPlay·OnWxSaveRestored→복원 송출, 헬퍼 `SendGimmickStateEvent(bRestoreEntry)` | 수정 |
| `WxWorld/.../Gimmick/Wx{Door,Elevator,SpawnConsole,AlarmConsole,CutsceneTrigger,TreasureChest}.{h,cpp}` | enum 삭제, State `FGameplayTag` 화(생성자 기본값), 핸들러 태그화, `ReplicatedUsing` 통일, ST 바인딩 노출 제거 | 수정 |
| `WxGame/WorldObject/WxLaserCorridor.{h,cpp}` | 위와 동일 | 수정 |
| `WxWorld/.../Gimmick/WxGimmickStateTreeNodes.cpp` | `IsInitialOrRestoreEntry` 헬퍼 + 7개 노드 초기진입 판정 교체 | 수정 |
| `WxInventory/.../Inventory/WxRewardStateTreeNodes.cpp` | Grant Reward 초기진입 판정에 복원 마커 OR 추가 | 수정 |

### 구현·결정과 그 이유
- **상태 태그가 권위 값이자 ST 이벤트**: 상태별 태그 하나가 저장·복제 권위 값이면서 그 상태의 Required Event to Enter 와 매칭되는 ST 이벤트를 겸한다. ST 자산 어휘를 태그로 단일화.
- **State 필드는 자식 소유 유지**: Door/Elevator/Chest/Alarm/Laser 는 Replicated+SaveGame, Cutscene 은 비저장. 저장 정책이 갈려 베이스 단일 필드로 호이스트하지 않고, 베이스는 `GetGimmickState()` 가상 getter 로 현재 태그를 읽는다.
- **복원 동작 보존(핵심)**: Required Event 는 이벤트 게이트라 시작 시점에 선택되지 않음 → 복원 시 베이스가 저장된 상태 태그를 ST 이벤트로 재발행해 진입을 구동한다. 그 진입이 라이브 발동(보상 재지급·재스폰·FX 재생)으로 오인되지 않도록 `Gimmick.Restore` 마커를 함께 보내고, 일회성 노드는 `FStateTreeExecutionContext::HasEventToProcess(Gimmick.Restore)` 로 감지해 스냅·스킵한다(기존 `SourceStateID` 판정에 OR).
- **클라 이벤트 송출 통일**: 폴링형이던 Door/Elevator/Chest/Laser 도 `ReplicatedUsing=OnRep_GimmickState` 로 바꿔, 모든 기믹이 OnRep 에서 상태 태그 이벤트를 발행한다(클라가 Required Event 로 진입).

### 검증
- WxEditor(Development) 빌드 **성공**(`Result: Succeeded`). WxCore/WxWorld/WxInventory/WxGame 컴파일·링크 통과, 경고는 엔진 측 기존 deprecation 뿐.
- 기능 검증은 ST 에셋 재저작 이후 에디터에서 수행해야 한다(아래 후속 과제).

### 계획 대비 달라진 점
- 계획대로. 복원 스냅/스킵 보존 방식은 계획의 두 후보 중 **마커 이벤트 + `HasEventToProcess`**(UE 5.7 공개 API) 로 확정 — 페이로드/노드 대수술 없이 1줄 OR 로 해결.

### 후속 과제 (에디터 수동 — 코드로 불가)
- **ST 에셋 5종 재저작**: ST_Door / ST_Elevator / ST_SpawnConsole / ST_CutsceneTrigger / ST_TreasureChest
  1. 각 top-level 상태의 옛 Enum Compare enter 조건(삭제된 `State` enum 바인딩) 제거.
  2. 비기본 상태에 **Required Event to Enter = 해당 `Gimmick.<기믹>.<상태>` 태그** 설정. 기본(resting) 상태(Idle/Close/Closed/Active)는 Required Event 없이 시작 시 선택되게 둔다.
  3. 제거된 `Event.GimmickStateChanged` 를 참조하던 전이를, 상태 태그 이벤트로 재선택되도록 갱신(루트의 On Event 전이 등).
  4. 재저작 후 검증: 문 개폐, 상자 1회 보상, 스폰 콘솔 1회 스폰, 알람 FX, 컷신 재생/복귀 — 그리고 **세이브→로드 시 재발동(보상·스폰·FX) 없음**과 스냅 진입.
- **ST_AlarmConsole / ST_LaserCorridor 부재 확인**: 두 기믹이 참조하는 ST 에셋이 현재 없음 — 미배선인지 확인 후 동일 패턴으로 작성.
- **세이브 호환**: 기존 슬롯의 enum(uint8) State 는 새 `FGameplayTag` 와 타입이 달라 복원되지 않음(개발 단계라 허용 가정).
