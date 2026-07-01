# WxInteractionComponent: SphereComponent → SceneComponent + 기존 메시 콜리전을 볼륨으로 재사용

## 계획

### 목표
`UWxInteractionComponent`가 `USphereComponent`를 상속해 자기 자신이 쿼리 볼륨이던 것을 순수 `USceneComponent`로 바꾸고, 쿼리 볼륨은 이미 붙어있던 메시 컴포넌트의 콜리전을 그대로 재사용한다. 상호작용 영역 형상이 구로 고정되던 제약을 없애기 위함.

### 접근 방식
- **비파괴적 채널 응답 방식**: 메시의 `ObjectType`을 바꾸면 본래 콜리전이 깨지므로, 메시의 `WxInteractable`(DefaultResponse=Ignore) **응답만 Overlap으로** 켠다. 스캐너는 `OverlapMultiByObjectType` → `OverlapMultiByChannel(WxInteractable)`로 전환. 응답을 켠 볼륨만 잡히고 다른 메시엔 무영향.
- **볼륨 지정**: 컴포넌트에 `CollisionVolume`(`UPrimitiveComponent*`) 추가. 소유자가 `SetCollisionVolume(<메시>)`로 지정 — 모든 사이트에서 이미 `SetHighlightTarget`에 넘기는 그 메시와 동일하므로 한 줄씩만 추가.
- **활성/비활성**: 볼륨의 `WxInteractable` 응답을 Overlap↔Ignore 토글(CollisionEnabled·ObjectType·기타 응답 불변).
- **역참조**: 오버랩 결과가 이제 볼륨 메시이므로 `UWxInteractionComponent::FindByCollisionVolume(prim)` 정적 함수로 소유 액터에서 해당 상호작용 컴포넌트를 되찾는다.
- **서버 사거리 검증**: `GetScaledSphereRadius` 대신 `GetInteractionReachRadius()`(`Bounds.SphereRadius`)·`GetInteractionLocation()`(볼륨 위치) 사용.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Interaction/WxInteractionComponent.h` | 상속 SceneComponent화, CollisionVolume·접근자·역참조·BeginPlay 추가 | 수정 |
| `Plugins/WxWorld/.../Private/Interaction/WxInteractionComponent.cpp` | 생성자 스피어 제거, BeginPlay/ApplyInteractionCollision 재작성, 접근자·역참조 구현 | 수정 |
| `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp` | 스캐너 채널 오버랩·역참조 전환, 사거리 검증 볼륨 기준화 | 수정 |
| C++ 소유자 9개 파일(LaserCorridor·CheckPoint·EnemyCharacter·TreasureChest·SpawnConsole·Elevator·Door·CutsceneTrigger·AlarmConsole) | `SetCollisionVolume(<메시>)` 한 줄씩 추가 | 수정 |
| `Plugins/WxCore/.../Public/WxCollisionChannels.h` | WxInteractable 주석 갱신 | 수정 |

### 후속(BP 핸드오프)
`BP_ItemPickup`은 BP에서 상호작용 컴포넌트를 추가하고 `WxItemPickup`(WxInventory)은 WxWorld를 참조 못 하므로, 기획자가 BP에서 `CollisionVolume`을 지정해야 한다. 픽업 메시는 `PhysicsOnly`라 쿼리 미사용 → 쿼리 가능 프리미티브로 지정 또는 CollisionEnabled 조정 필요.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Interaction/WxInteractionComponent.h` | 상속 `USphereComponent`→`USceneComponent`, `CollisionVolume`·접근자(`GetInteractionLocation`/`GetInteractionReachRadius`)·역참조(`FindByCollisionVolume`)·`BeginPlay` 추가 | 수정 |
| `Plugins/WxWorld/.../Private/Interaction/WxInteractionComponent.cpp` | 생성자 스피어 설정 제거, `BeginPlay`에서 부착 부모 자동 채택+응답 초기화, `ApplyInteractionCollision`을 WxInteractable 응답 토글로 변경, 접근자·역참조 구현 | 수정 |
| `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp` | 스캐너 `OverlapMultiByObjectType`→`OverlapMultiByChannel` + 역참조 수집, 정렬·사거리 검증을 볼륨 기준(`GetInteractionLocation`/`GetInteractionReachRadius`)으로 | 수정 |
| `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.h` | 클래스 주석의 수집 방식 설명 갱신 | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxElevator.cpp` | 플랫폼 인터랙션에 `SetCollisionVolume(PlatformMesh)` 명시(부착 부모가 SceneComponent라 자동 채택 불가) | 수정 |
| `Plugins/WxCore/.../Public/WxCollisionChannels.h` | WxInteractable 주석을 응답 기반 방식으로 갱신 | 수정 |

### 구현·결정과 그 이유
- **비파괴적 채널 응답**: 메시의 ObjectType을 바꾸면 본래 콜리전(블로킹·피격)이 깨지므로, `WxInteractable`(DefaultResponse=Ignore) **응답만 Overlap/Ignore로 토글**하고 스캐너를 `OverlapMultiByChannel`로 전환. 메시의 CollisionEnabled·ObjectType·다른 응답은 불변.
- **볼륨 자동 채택**: 구현 중 피드백을 반영해, 각 소유자가 명시 지정하는 대신 `BeginPlay`에서 부착 부모 프리미티브를 `CollisionVolume`으로 자동 채택. 11개 인터랙션 컴포넌트 중 10개가 대상 메시에 직접 부착돼 있어 C++ 명시 호출이 사라짐. `SetCollisionVolume`은 오버라이드용으로 유지.
- **역참조**: 오버랩 결과가 볼륨 메시이므로 `FindByCollisionVolume`(소유 액터에서 해당 볼륨을 참조하는 인터랙션 컴포넌트 탐색)로 되찾음.
- **서버 사거리**: 임의 형상을 `Bounds.SphereRadius`로 보수적으로 감싼 상한 사용(변조 클라 방어는 유지, 형상 무관).

### 계획 대비 달라진 점
- **볼륨 지정 방식**: 승인 계획은 소유자마다 `SetCollisionVolume(<메시>)` 한 줄씩 추가였으나, 사용자 피드백으로 **부착 부모 자동 채택**으로 변경. 명시 호출은 부착 부모가 메시가 아닌 엘리베이터 플랫폼 하나만 남김.

### 후속 과제
- **BP 핸드오프**: `BP_ItemPickup`은 BP에서 인터랙션 컴포넌트를 추가하고 `WxItemPickup`(WxInventory)은 WxWorld 미참조라 C++ 지정 불가. 인터랙션 컴포넌트를 쿼리 가능 프리미티브에 부착하면 자동 채택되지만, 픽업 `MeshComponent`는 `PhysicsOnly`(쿼리 미사용)라 그대로면 안 잡힘 → 기획자가 그 메시의 CollisionEnabled를 `QueryAndPhysics`로 바꾸거나 쿼리 가능한 별도 볼륨에 부착/지정 필요.
- 인게임 동작(상자·문·콘솔·체크포인트·적 피니셔, 멀티플레이 원격 클라 상호작용) 미검증(컴파일만 확인).
