# IWxMinion 제거 — IWxSpawnable 마커 + UWxMinionComponent 로 교체

## 계획

### 목표
`IWxMinion`을 지우고 픽커 필터는 기존 `IWxSpawnable`이, 데이터는 신규 `UWxMinionComponent`가 맡게 한다. UINTERFACE가 UPROPERTY를 못 가져 BP 저작 기본값이 C++ 기본값을 이기는 구조적 결함(`module_review_WxCore.md` 1번)을 없애고, 주인에게 붙는 태그를 소환물이 선언하게 해 종류별로 다른 태그를 쓸 수 있게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCore/Public/WxMinion.h`, `Private/WxMinion.cpp` | 삭제 | 삭제 |
| `WxCore/Public/WxSpawnable.h`, `Private/WxSpawnable.cpp` | WxWorld에서 이동. `OnSpawnedBy(AWxSpawner*)` → `OnSpawnedBy(AActor*)` | 이동·수정 |
| `WxCore/Public/WxGameplayTags.h`, `Private/WxGameplayTags.cpp` | `Effect.AggroIgnored` 선언 추가 | 수정 |
| `WxCombat/Public/Minion/WxMinionComponent.h`, `Private/Minion/WxMinionComponent.cpp` | 신규 | 신규 |
| `WxCombat/.../Minion/WxMinionSubsystem.h`·`.cpp` | 로스터 등재를 컴포넌트 등록으로, 상한·태그를 컴포넌트에서 읽기 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotify_SpawnMinion.h` | `MustImplement`를 `/Script/WxCore.WxSpawnable`로 | 수정 |
| `WxAI/.../WxBTService_UpdateTargetActor.cpp` | `CanBeAggroTarget`을 `Effect.AggroIgnored` 태그 조회로 | 수정 |
| `WxWorld/.../Spawnable/WxSpawnable.h`·`.cpp` | 삭제(WxCore로 이동) | 삭제 |
| `WxWorld/.../Spawnable/WxSpawner.h`·`.cpp` | include 경로, `MustImplement` 경로 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h`·`.cpp` | 컴포넌트 네이티브 부착, include 경로, `OnSpawnedBy(AActor*)` | 수정 |
| `WxCombat/.../Effect/WxEffect_AggroIgnored.h`·`.cpp` | 신규. 무한 지속으로 `Effect.AggroIgnored`를 부여 | 신규 |
| `Plugins/WxCore/README.md`, `Plugins/WxWorld/README.md` | 타입 위치·규약 갱신 | 수정 |

### 접근 방식
- **베이스에 네이티브 부착**: `AWxEnemyCharacter` 생성자에서 `CreateDefaultSubobject`. 소환물로 안 쓰이는 적도 갖지만 등록되지 않아 값이 무의미하고, 대신 컴포넌트가 CDO에 실려 `MinionClass.GetDefaultObject()`에서 상한을 읽을 수 있다. 덕분에 `SpawnMinion`의 기존 순서(선 정리 → 후 스폰)를 그대로 두고 BP 에셋 작업도 없이 C++ 기본값이 선다. SCS 부착이었다면 둘 다 불가능했다.
- **로스터 등재는 Instigator가 가른다**: 모든 적이 컴포넌트를 가지므로 보유 여부로는 못 가린다. 컴포넌트 `BeginPlay`에서 `GetMaster`가 유효할 때만 등록한다. 덕분에 `AddOnActorSpawnedHandler` 경로를 버릴 수 있다 — 엔진은 `OnActorSpawned`를 `PostSpawnInitialize` 직후에 쏘는데(`LevelActor.cpp:774`) 그때는 복제 스폰의 Instigator가 비어 있어 클래스로만 거를 수밖에 없었다.
- **어그로 무시는 GE로**: WxAI는 WxCombat을 못 보고 어그로는 소환 로스터와 다른 축이다. Infinite GE가 `Effect.AggroIgnored`를 부여하고 BT 서비스는 태그만 본다. 적용 통로는 `UWxAbilitySet::GrantedEffects`가 이미 갖고 있다.
- **주인 태그는 인자로**: `RefreshMasterStateTag(Master, StateTag)`. 호출부가 영향받는 태그를 이미 아니 캐시 멤버가 필요 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCore/Public/WxMinion.h`, `Private/WxMinion.cpp` | 삭제 | 삭제 |
| `WxCore/Public/WxSpawnable.h`, `Private/WxSpawnable.cpp` | WxWorld에서 이동, 시그니처를 `OnSpawnedBy(AActor*)`로, 계약 주석 추가 | 이동·수정 |
| `WxCore/Public/WxGameplayTags.h`, `Private/WxGameplayTags.cpp` | `Effect.AggroIgnored` 추가, `State.Minion.Active` 주석 정정 | 수정 |
| `WxCombat/Public/Minion/WxMinionComponent.h`, `Private/.../WxMinionComponent.cpp` | 신규 | 신규 |
| `WxCombat/.../Minion/WxMinionSubsystem.h`·`.cpp` | 로스터를 컴포넌트 등록 기반으로, 상한·태그를 컴포넌트에서 읽기 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotify_SpawnMinion.h` | `MustImplement` 경로, 주석 정정 | 수정 |
| `WxAI/.../WxBTService_UpdateTargetActor.cpp` | `CanBeAggroTarget`을 `Effect.AggroIgnored` 태그 조회로 | 수정 |
| `WxWorld/.../Spawnable/WxSpawnable.h`·`.cpp` | 삭제(WxCore로 이동) | 삭제 |
| `WxWorld/.../Spawnable/WxSpawner.h`·`.cpp` | include·`MustImplement` 경로 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h`·`.cpp` | `MinionComponent` 네이티브 부착, include 경로, `OnSpawnedBy(AActor*)` | 수정 |
| `Plugins/WxCore/README.md`, `Plugins/WxWorld/README.md` | 타입 위치·규약 갱신 | 수정 |

### 구현·결정과 그 이유
- **베이스에 네이티브 부착이 설계를 결정했다**: `AWxEnemyCharacter` 생성자에서 붙이면 컴포넌트가 CDO에 실려 `MinionClass.GetDefaultObject()->FindComponentByClass<>()`로 스폰 전에 상한을 읽을 수 있다. 덕분에 "선 정리 → 후 스폰" 순서를 그대로 뒀고, BP 디테일 오버라이드도 BP CDO에 반영되므로 에셋 작업 없이 C++ 기본값이 선다. SCS 부착이었다면 둘 다 불가능해 스폰 순서를 뒤집어야 했다.
- **로스터 등재 기준은 Instigator**: 모든 적이 컴포넌트를 가지므로 보유 여부로는 소환물을 못 가린다. 컴포넌트 `BeginPlay`에서 `GetMaster`가 유효할 때만 등록한다. 이 덕에 `AddOnActorSpawnedHandler` 경로를 통째로 버렸다 — 엔진은 `OnActorSpawned`를 `PostSpawnInitialize` 직후에 쏘는데(`LevelActor.cpp:774`) 그때는 복제 스폰의 Instigator가 비어 클래스로만 거를 수 있었다. `BeginPlay`는 서버·클라 모두 Instigator가 들어온 뒤다.
- **어그로 무시는 C++에서 제거**: WxAI는 WxCombat을 참조할 수 없고, 어그로는 소환 로스터와 다른 축이다. 무한 GE가 `Effect.AggroIgnored`를 부여하고 BT 서비스는 태그만 본다 — 바로 위 `IsActorDead`가 이미 쓰는 통로라 새 장치가 아니다. 결과적으로 WxAI에서 미니언이라는 개념이 사라졌다.
- **`IWxSpawnable`을 WxCore로**: WxCombat 소환 노티파이의 픽커 필터가 이 계약을 가리켜야 하는데 WxWorld를 참조할 수 없다. 옮기면서 `AWxSpawner*` 인자를 `AActor*`로 낮춰 foundation 헤더가 도메인 타입을 이름으로도 언급하지 않게 했다 — `IWxInteractable`이 `AActor* Interactor`를 쓰는 것과 같은 모양이다.
- **주인 태그는 인자로**: `RefreshMasterStateTag(Master, StateTag)`. 로스터가 바뀐 쪽이 영향받는 태그를 이미 아니 직전 태그 집합을 캐시할 필요가 없다.
- **어그로 무시 GE는 BP가 아니라 C++**: 이 모듈의 태그 부여 GE가 전부 `WxEffect_*` C++ 클래스라 같은 형태를 따랐다. `UWxEffect_SuperArmor` 뼈대 그대로 TargetTags·AssetTags 한 쌍만 달고, 걷어낼 쪽(도발 등)이 쿼리로 집을 수 있게 애셋 태그도 뒀다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 어그로를 끌지 않을 소환물의 AbilitySet(`ABS_Minion`·`ABS_Doppelganger` 등) `GrantedEffects`에 `WxEffect_AggroIgnored`를 넣어야 한다. `unreal-mcp`가 ConnectionRefused라 에셋을 만지지 못했고, 넣기 전까지 어그로 무시는 꺼진 채로 동작한다. 두 BP가 기존 `IsAggroIgnored`를 저작했는지는 BP 내부라 확인하지 못했다.
- 도플갱어에 별도 주인 태그를 주려면 `BP_Doppelganger`의 `MinionComponent.MasterStateTag`만 바꾸면 된다. 상한 정리가 종류를 가리지 않는 문제(`module_review_WxCore.md` 2번)는 이번 범위 밖이다.
