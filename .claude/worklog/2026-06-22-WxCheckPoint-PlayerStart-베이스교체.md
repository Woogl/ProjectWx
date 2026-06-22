# WxCheckPoint: AWxGimmick → APlayerStart 베이스 교체

## 계획

### 목표
`WxCheckPoint`는 기믹 인프라(State enum·StateTree 에셋·IWxSavable 영속 상태)를 전혀 안 쓰는 유일한 예외라, 베이스를 `AWxGimmick`에서 `APlayerStart`로 교체한다. 상호작용 동작(힐 GE·소비 아이템 충전·오토스포너 리스폰)은 그대로 유지하고, 부활 지점(`ChoosePlayerStart`) 후보로 자연 편입한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | include·베이스·생성자 시그니처 교체, 클래스 주석 갱신 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 생성자를 `FObjectInitializer` 형태로, `bReplicates=true` 유지, 루트를 캡슐로 부착 | 수정 |

### 접근 방식
- **베이스 교체**: `public AWxGimmick` → `public APlayerStart`. include `Gimmick/WxGimmick.h` → `GameFramework/PlayerStart.h`. `APlayerStart`는 `FObjectInitializer` 생성자만 제공하므로 `AWxCheckPoint(const FObjectInitializer&)`로 바꿔 `Super(ObjectInitializer)` 위임.
- **루트/콜리전**: 루트는 `APlayerStart`(`ANavigationObjectBase`)의 CapsuleComponent. `NoCollision` 프로파일이라 새 충돌체 없음. `MeshComponent`(횃불)를 `RootComponent`에, `InteractionComponent`(Sphere)를 메시에 부착.
- **복제 유지**: `WxInteractionComponent`가 서버 Multicast RPC로 OnInteracted를 fire하므로 액터 복제 필요. 과거 `AWxGimmick`이 켜주던 `bReplicates=true`를 생성자에서 직접 유지.
- **세이브**: SaveGame 프로퍼티가 0이라 `IWxSavable` 상실로 잃는 영속 데이터 없음.
- **본문 불변**: `BeginPlay`(델리게이트 바인딩)·`HandleInteracted`(힐/충전/리스폰) 본문과 include는 그대로.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.h` | include `Gimmick/WxGimmick.h`→`GameFramework/PlayerStart.h`, 베이스 `AWxGimmick`→`APlayerStart`, 생성자 `AWxCheckPoint()`→`AWxCheckPoint(const FObjectInitializer&)`, 클래스 주석에 PlayerStart 상속 사유 추가 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 생성자 `Super(ObjectInitializer)` 위임, `bReplicates=true` 유지, 메시 부착 대상 `SceneRoot`→`RootComponent`(캡슐) | 수정 |

### 구현·결정과 그 이유
- **FObjectInitializer 생성자 강제**: `APlayerStart`는 `FObjectInitializer` 생성자만 노출하므로 무인자 생성자로는 `Super` 위임이 불가. 시그니처를 바꿔 `Super(ObjectInitializer)`로 위임했다.
- **`bReplicates=true` 명시 유지**: `WxInteractionComponent`가 서버 `TryInteract`→`MulticastInteracted`(NetMulticast)로 OnInteracted를 fire하는데, 멀티캐스트가 클라까지 전달되려면 소유 액터가 복제돼야 한다. 과거 `AWxGimmick`이 켜주던 설정이 사라지므로 생성자에서 직접 켜 동작 패리티를 보장했다.
- **루트는 PlayerStart 캡슐 그대로 사용**: `ANavigationObjectBase`가 캡슐을 `NoCollision` 프로파일로 설정(엔진 5.7 소스 확인)하므로 별도 콜리전 조정 없이 메시를 `RootComponent`에 붙이면 충돌 동작 변화가 없다. 감지는 그대로 `InteractionComponent`(Sphere)가 담당.
- **`IWxSavable` 상실 무영향**: `WxCheckPoint`엔 SaveGame 프로퍼티가 없어 저장되던 상태가 0. 인터페이스를 잃어도 잃는 영속 데이터가 없다.

### 계획 대비 달라진 점
- 계획대로. (`HandleInteracted`/`BeginPlay` 본문·include 무변경, 두 파일만 수정)

### 후속 과제
- **에디터 수동 후속(미검증)**: `BP_CheckPoint`를 열어 자동 리인스턴싱 결과 확인 후 재저장 — (a) 부모 계층이 APlayerStart로 정상 표시, (b) BP 추가 `Niagara`(연기)가 새 캡슐 루트에 재부착, (c) 횃불 메시·상호작용 영역 배치 유지. 빌드(WxEditor Development) 컴파일은 `Result: Succeeded`로 통과.
- **런타임 동작 미검증**: 상호작용 시 HP MaxHP 회복·소비 아이템 충전·오토스포너 리스폰이 종전과 동일한지 인게임 확인 필요.
