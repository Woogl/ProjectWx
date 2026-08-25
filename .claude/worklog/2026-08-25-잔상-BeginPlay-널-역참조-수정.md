# 잔상 BeginPlay 널 역참조 수정

## 계획

### 목표
`AWxGhostTrail::BeginPlay`가 널 가드 삼항 바로 다음 줄부터 같은 `OwnerCharacter`를 무조건 역참조해, Owner가 `ACharacter`가 아니면 확정 크래시가 난다. 어떤 Owner를 받아도 크래시 없이 동작하고, 잔상을 만들지 못한 경우 그 사실이 로그로 남게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp` | `BeginPlay`의 삼항 가드를 조기 반환으로 바꾸고, `GetMesh()` 널 검사 추가, 실패 경로에 경고 로그. `WxCombatModule.h` include 추가 | 수정 |

### 접근 방식
- **가드 하나로 통합**: 기존 `if (!Owner) return;` + 삼항 이중 구조를 없애고, `Cast<ACharacter>(GetOwner())` 결과 하나에서만 조기 반환한다. `Cast`가 널 입력에 널을 돌려주므로 앞선 게이트는 중복이었고, 그 중복이 "가드했다"는 착각을 만들어 이번 결함을 낳았다.
- **메시는 검사하고 캡슐은 검사하지 않는다**: UE 5.8 기준 `ACharacter::Mesh`는 `CreateOptionalDefaultSubobject`라 널일 수 있고(`Character.cpp:119`) `UPoseableMeshComponent::CopyPoseFromSkeletalComponent`에 내부 널 가드가 없다(`PoseableMeshComponent.cpp:291-296`). 반면 `CapsuleComponent`는 `CreateDefaultSubobject`로 항상 생성되므로(`Character.cpp:77`) 가드를 새로 달지 않는다.
- **실패 시 `Destroy()`를 부르지 않는다**: BeginPlay 중 파괴하면 `UWorld::SpawnActor`가 nullptr을 반환하고(`LevelActor.cpp:763-767`), 호출부 `HandleGameplayCue`가 반환값을 검사하지 않은 채 `SpawnedGhostTrail->SetLifeSpan()`을 불러 새 크래시가 된다. 조기 반환만 하고 수명은 기존 `LifeSpan`에 맡긴다.
- **경고 로그**: 이 큐는 디자이너가 `GC_GhostTrail`에 `BP_GhostTrail`을 물려 쓰는 구조라, 조용히 아무 일도 하지 않으면 원인 추적이 어렵다. 모듈 기존 관용구(`WxTimeDilationComponent.cpp:46,97`)와 같은 `LogWxCombat` Warning을 쓴다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp` | `BeginPlay`의 이중 가드를 조기 반환 하나로 통합, `GetMesh()` 널 검사 추가, 두 실패 경로에 `LogWxCombat` 경고. `WxCombatModule.h` include 추가 | 수정 |

### 구현·결정과 그 이유
- **삼항 대신 조기 반환**: 원래 코드의 삼항은 "널이어도 계속 진행"을 뜻해 바로 다음 줄의 무조건 역참조와 모순됐다. 만들 대상이 없으면 진행 자체를 멈추는 쪽이 이 함수의 유일한 정당한 동작이다.
- **가드를 하나로**: `Cast`가 널 입력에 널을 돌려주므로 앞선 `if (!Owner)`는 순수 중복이었다. 게이트가 둘이라 "이미 막았다"는 착각이 생기기 쉬운 구조였고, 하나로 줄여 그 여지를 없앴다.
- **메시는 검사, 캡슐은 미검사**: 엔진이 보장하는 것을 중복 방어하지 않는다는 기준을 적용했다. 캡슐은 항상 생성되지만 메시는 Optional 서브오브젝트라 실제로 널일 수 있고, 포즈 복사 함수가 널을 막아 주지 않아 두 번째 크래시 지점이었다.
- **`Destroy()` 대신 조기 반환**: 스폰 도중 파괴하면 `SpawnActor`가 nullptr을 반환하는데 호출부가 그 반환값을 검사하지 않아, 크래시를 옮기기만 했을 것이다. 정리는 이미 걸려 있는 `LifeSpan`에 맡겼다.
- **경고 로그**: 잔상 대상은 디자이너가 큐 에셋에서 물리는 값이라, 조용히 아무 일도 안 하면 원인 추적이 어렵다. 두 실패 사유를 각각 다른 문구로 남겨 로그만 보고 구분되게 했다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 같은 파일 `UWxCueNotify_GhostTrail::HandleGameplayCue`의 리뷰 🟡 2번(EventType 미필터·`MyTarget` 널 미검사·`SpawnActor` 반환값 미검사)은 이번 범위 밖이라 남아 있다. 특히 반환값 미검사는 이번에 `Destroy()`를 배제한 이유와 직접 맞물린다 — 그 항목을 고치면 실패한 잔상을 즉시 파괴하는 선택지도 열린다.
