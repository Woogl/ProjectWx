# LaserCorridor 스폰 루프를 StateTree 태스크로 전환 (완전 전환)

## 계획

### 목표
`AWxLaserCorridor`의 주기 스폰 루프(`HandleSpawnTimer`+타이머)를 WxWorld 공용 StateTree 태스크 `Wx Laser Spawn`으로 빼고, 그 김에 남은 C++ 상태추종(`OnRep_State`/`RefreshLaserState`/`OnWxSaveRestored`/`ActiveLasers`)을 걷어내 다른 5개 기믹과 동일한 「State enum 권위 + StateTree Enum Compare 추종」 패턴으로 완전 전환한다. 사용자 결정: (1) 완전 전환, (2) 스폰/이동 태스크 분리 유지(태스크↔태스크 바인딩).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_LaserSpawn`(+InstanceData) 선언, `class UBoxComponent;` 전방선언, 개요 주석 1줄 | 수정(신규 태스크) |
| `WxGimmickStateTreeNodes.cpp` | `Wx Laser Spawn` `EnterState`/`Tick`/`ExitState`/`GetDescription` 정의, include 추가 | 수정(신규 태스크) |
| `WxLaserCorridor.h` | `ActiveLasers`·`SpawnTimerHandle`·`HandleSpawnTimer`·`RefreshLaserState`·`OnRep_State`·`OnWxSaveRestored`·`EndPlay`·`LaserZoneClass` 제거; `State`를 평이한 `Replicated`+노출로 | 수정 |
| `WxLaserCorridor.cpp` | 위 함수 정의·include 제거; `SetLaserCorridorState`/`BeginPlay`에서 `RefreshLaserState` 호출 제거 | 수정 |
| `Plugins/WxWorld/README.md` | 공통 태스크 목록에 `LaserSpawn` 추가 | 수정 |

### 접근 방식
- **스폰 태스크 일반화**: 공용 노드라 WxCombat 타입(`AWxEffectZone`) 참조 불가 → `TSubclassOf<AActor>`+엔진 타입만. 스폰할 BP는 ST 에셋에서 author. InstanceData: `ActorClass`(에셋), `SpawnVolume`(←CorridorBox), `Interval`(←SpawnInterval), `MoveSpeed`(←MoveSpeed, 수명용) + 런타임 `SpawnedActors`(LaserAdvance 바인딩 소스)·`TimeSinceLastSpawn`.
- **목록 소유 이동**: 태스크는 바인딩으로 액터 멤버에 못 쓰므로 레이저 목록을 액터(`ActiveLasers`)→스폰 태스크 인스턴스 데이터로 이동. `LaserAdvance.Actors ← LaserSpawn.SpawnedActors` 태스크↔태스크 바인딩(이 코드베이스 첫 사례, 정확 타입 일치). Active 상태에서 Spawn을 Advance보다 앞에 배치해 같은 프레임 신선도.
- **동작 보존**: 스폰/철거 모두 권위 한정(`EnterState`서 누적기=Interval로 첫 틱 즉시 스폰, `Tick`서 간격 스폰+IsValid 컬링, `ExitState`서 활성 레이저 전부 Destroy=비활성 분기 철거). 박스 -X 끝 스폰, YZ 스케일 핏, `(박스길이/MoveSpeed)` 수명 — 현 `HandleSpawnTimer` 로직 그대로.
- **액터 완전 전환**: `State`를 `ReplicatedUsing=OnRep_State`→평이한 `Replicated`+`VisibleAnywhere/AllowPrivateAccess`(Enum Compare 폴링). 인터랙션 토글은 ST의 `Wx Enable Interaction`으로. `RefreshLaserState`/`OnRep_State`/`OnWxSaveRestored`/타이머/`EndPlay` 제거.
- **사용자 에디터 작업**: `ST_LaserCorridor` 신설(Active: LaserSpawn+LaserAdvance+EnableInteraction(true), Disabled: EnableInteraction(false), State Enum Compare 전이, 완료 전이 없음) 후 `BP_LaserCorridor`에 할당.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_LaserSpawn`(+InstanceData `{ActorClass, SpawnVolume, Interval, MoveSpeed, SpawnedActors, TimeSinceLastSpawn}`) 추가, `class UBoxComponent;` 전방선언, 개요 주석 1줄 | 수정(신규 태스크) |
| `WxGimmickStateTreeNodes.cpp` | `Wx Laser Spawn` `EnterState`/`Tick`(권위 스폰)/`ExitState`(권위 철거)/`GetDescription` 정의, BoxComponent/World/Actor include | 수정(신규 태스크) |
| `WxLaserCorridor.h` | `ActiveLasers`·`SpawnTimerHandle`·`HandleSpawnTimer`·`RefreshLaserState`·`OnRep_State`·`OnWxSaveRestored`·`EndPlay`·`LaserZoneClass`·`AWxEffectZone` 전방선언 제거; `State`를 평이한 `Replicated`+`VisibleAnywhere/AllowPrivateAccess`로; 클래스 주석 갱신 | 수정 |
| `WxLaserCorridor.cpp` | 위 함수 정의·`TimerManager`/`WxEffectZone`/`World` include 제거; `HandleConsoleInteracted`에 `HasAuthority` 가드(타 기믹 일치); `SetLaserCorridorState`/`BeginPlay`에서 `RefreshLaserState` 호출 제거 | 수정 |
| `Plugins/WxWorld/README.md` | 공통 태스크 목록에 `LaserSpawn` 추가 | 수정 |

### 구현·결정과 그 이유
- **스폰 태스크 일반화로 모듈 디커플링**: `AWxEffectZone`이 WxCombat 소속이라 WxWorld 공용 노드에 둘 수 없다. 스폰할 클래스를 `TSubclassOf<AActor>`, 통로를 `UBoxComponent`로만 받아 엔진 타입에 가둬, 어떤 기믹이든 박스 통로 주기 스폰에 재사용 가능한 범용 노드가 됐다(LaserAdvance가 `Actors`+`Velocity`로 일반화된 것과 동일 원칙).
- **목록 소유를 액터→태스크로 이전**: StateTree 태스크는 바인딩으로 액터 멤버에 쓸 수 없으므로(단방향 복사), 스폰을 태스크로 빼면서 레이저 목록을 액터 `ActiveLasers`에서 스폰 태스크 인스턴스 데이터 `SpawnedActors`로 옮겼다. LaserAdvance는 이 목록을 태스크↔태스크 바인딩으로 읽는다(분리 유지). 액터는 더 이상 목록·타이머·철거를 들지 않는다.
- **완전 전환으로 패턴 일치**: 인터랙션 토글을 `Wx Enable Interaction`으로, 철거를 스폰 태스크 `ExitState`로 옮기니 `RefreshLaserState`/`OnRep_State`/`OnWxSaveRestored`/`EndPlay`가 모두 불필요해졌다. `State`를 평이한 `Replicated`로 바꿔 StateTree Enum Compare가 추종하게 해, 액터가 "State 권위 + 인터랙션 확정"만 들고 나머지는 전부 StateTree가 처리하는 다른 5개 기믹과 동일 구조가 됐다.
- **동작·네트워킹 보존**: 스폰/철거는 현 `HandleSpawnTimer`/`RefreshLaserState`와 동일하게 권위 한정(박스 -X 끝 스폰, YZ 스케일 핏, `박스길이/MoveSpeed` 수명, IsValid 컬링으로 목록 유계). 진입 첫 틱 즉시 스폰은 누적기를 `Interval`로 채워 재현. 스폰체는 Transient라 복원 시에도 새로 스폰을 재개(초기 진입/라이브 구분 불필요).

### 계획 대비 달라진 점
- **이동(LaserAdvance) 권위 모델 미변경**: 작업 중 "이동은 서버 권위로 하지 마" 지시가 있어, 레이저가 복제 액터(`AWxEffectZone bReplicates=true`)이고 스폰이 권위 전용이라 클라 이동 목록이 비는 점·복제 이동과 로컬 이동 충돌을 들어 네트워크 모델을 확인 요청했고, 사용자가 "현재 상태로 냅두자"로 정리해 LaserAdvance는 현행(서버 권위) 그대로 뒀다. 스폰도 이동과 동일하게 권위 유지라 일관.
- **HandleConsoleInteracted에 HasAuthority 가드 추가**: 기존엔 setter 가드에만 의존했으나, SpawnConsole/AlarmConsole/Chest와 동일하게 Handle에서 가드하도록 맞췄다(사소).
- 동시 진행 중인 PlayLevelSequence 태스크 작업이 같은 두 공용 노드 파일을 함께 편집 중이며, 본 변경과 충돌 없이 공존(빌드 동시 통과).

### 후속 과제
- **에디터 작업(사용자)**: `Content/WorldObject/Gimmick/ST_LaserCorridor.uasset` 신설 — Active(LaserSpawn→LaserAdvance→EnableInteraction(true), 완료 전이 없음)·Disabled(EnableInteraction(false)), State Enum Compare 전이. LaserSpawn에 `ActorClass`=레이저 벽 BP, `SpawnVolume`←CorridorBox, `Interval`←SpawnInterval, `MoveSpeed`←MoveSpeed 지정. LaserAdvance `Actors`←LaserSpawn.SpawnedActors, `Velocity`=통로 forward×속도. `BP_LaserCorridor`에 할당.
- **태스크↔태스크 바인딩 검증**: `LaserAdvance.Actors ← LaserSpawn.SpawnedActors`는 이 코드베이스 첫 태스크간 바인딩 — 에디터에서 정상 노출·매 틱 신선도 확인.
- **PIE 검증 미완**: 레이저 주기 스폰·전진, 콘솔 상호작용 시 정지+활성 레이저 즉시 제거, 리슨 서버 클라 복제 추종, 세이브/스트리밍 복원 후 Active 재개.
