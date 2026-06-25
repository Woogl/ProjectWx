# WxSpawnerSubsystem 제거 → 백레퍼런스 + TActorIterator 전환

## 계획

### 목표
상태 없는 레지스트리였던 `UWxSpawnerSubsystem`을 제거한다. 등록/해제 보일러플레이트와 매 사망마다의 역조회 순회를 걷어내는 것이 목적이다. 처치 상태(`bIsKilled`)는 원래부터 각 `AWxSpawner`가 SaveGame 으로 보유하므로 마이그레이션은 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxSpawnerSubsystem.h` / `.cpp` | 서브시스템 전체 | 삭제 |
| `WxSpawnerLibrary.cpp` | `TryRespawnAll`이 TActorIterator collect-first 로 직접 순회 | 수정 |
| `WxCheckPoint.cpp` | 서브시스템 호출 → `UWxSpawnerLibrary::TryRespawnAll(this)` 위임 | 수정 |
| `WxEnemyCharacter.h` / `.cpp` | `OnSpawnedBy` 오버라이드 + `OwningSpawner` 백레퍼런스, `HandleDeath`에서 직접 `MarkKilled` | 수정 |
| `WxSpawner.cpp` | BeginPlay/EndPlay 의 등록·해제 블록 제거 | 수정 |
| `WxSpawner.h` / `.cpp` | 미사용이 된 `GetSpawnedActor()` 제거, 주석 갱신 | 수정 |
| `Plugins/WxWorld/README.md` | 서브시스템 행/문구 제거 | 수정 |

### 접근 방식
- **역조회 (MarkSpawnableKilled 대체)**: 기존 `IWxSpawnableInterface::OnSpawnedBy(Spawner)` 훅을 재활용해 적이 스폰 직후 스포너 WeakPtr 를 저장. 사망 시 순회 없이 `Spawner->MarkKilled()` 직접 호출. `GetOwner()`는 빙의 시 `APawn::PossessedBy`가 컨트롤러로 재할당하므로 쓰지 않는다.
- **일괄 리스폰 (TryRespawnAll)**: `Respawn()`이 루프 안에서 액터를 Destroy/Spawn 하므로, 대상 스포너를 `TArray`로 먼저 모으고 순회를 끝낸 뒤 일괄 호출해 이터레이터 무효화를 회피(collect-first).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxSpawnerSubsystem.h` / `.cpp` | 서브시스템 전체 제거 | 삭제 |
| `WxSpawnerLibrary.cpp` | `TryRespawnAll` 이 TActorIterator collect-first 로 직접 순회 | 수정 |
| `WxSpawnerLibrary.h` | 클래스 주석을 신규 동작에 맞게 갱신 | 수정 |
| `WxCheckPoint.cpp` | 서브시스템 호출 → `UWxSpawnerLibrary::TryRespawnAll(this)` 위임, include 교체 | 수정 |
| `WxEnemyCharacter.h` / `.cpp` | `OnSpawnedBy` 오버라이드 + `OwningSpawner` 백레퍼런스, `HandleDeath` 에서 직접 `MarkKilled` | 수정 |
| `WxSpawner.cpp` | BeginPlay/EndPlay 등록·해제 블록 제거, include 제거 | 수정 |
| `WxSpawner.h` / `.cpp` | 미사용된 `GetSpawnedActor()` 제거, 주석 명칭 갱신 | 수정 |
| `Plugins/WxWorld/README.md` | 서브시스템 행 → 라이브러리 행 교체, 책임 문구 갱신 | 수정 |

### 구현·결정과 그 이유
- **역조회를 순회 아닌 백레퍼런스로**: 매 적 사망마다 월드 전수 순회를 피하기 위해 `OnSpawnedBy` 훅으로 스폰 시점에 스포너 WeakPtr 를 적에 심었다. 이는 기존 서브시스템의 존재 이유(전수 순회 회피)를 순회 0회로 더 강하게 달성한다. `GetOwner()` 를 쓰지 않은 건 `APawn::PossessedBy` 가 빙의 시 Owner 를 AI 컨트롤러로 재할당하기 때문.
- **일괄 리스폰만 TActorIterator + collect-first**: 체크포인트 상호작용 시에만 호출돼 빈도가 낮아 전수 순회 비용이 무의미. 단 `Respawn()` 이 루프 내부에서 액터를 Destroy/Spawn 하므로 대상을 먼저 `TArray` 로 모은 뒤 순회를 끝내고 호출해 이터레이터 무효화를 차단.
- **호출부 단일화**: `WxCheckPoint` 가 순회 로직을 중복 구현하지 않고 `UWxSpawnerLibrary::TryRespawnAll` 로 위임해 리스폰 순회 구현을 한 곳에 둔다.
- **`GetSpawnedActor()` 제거**: 유일 호출처였던 서브시스템 삭제로 미사용이 됨. (`SpawnedActor` 멤버 자체는 Respawn/EndPlay/OnWxSaveRestored 에서 계속 사용하므로 유지.)

### 계획 대비 달라진 점
- 계획대로. (추가로 `WxSpawnerLibrary.h` 의 stale 클래스 주석, `WxGimmickStateTreeNodes`의 `TriggerSpawners` 무관함을 점검·확인.)

### 후속 과제
- 런타임 새너티(적 처치 마킹 / 체크포인트 일괄 리스폰 / `bNeverRevive` 보스 비부활)는 빌드 검증만 수행, 인게임 실측은 미수행.
