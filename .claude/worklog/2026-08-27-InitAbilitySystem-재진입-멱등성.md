# InitAbilitySystem 재진입 멱등성 확보

## 계획

### 목표
`AWxCharacterBase::InitAbilitySystem()` 이 재진입해도 이동 속도 기준값과 SPD 콜백 구독이 한 번만 서게 한다. 재빙의·PlayerState 재복제로 다시 들어오면 이미 SPD가 곱해진 `MaxWalkSpeed` 를 기준값으로 다시 잡아 배율이 누적되고, 콜백도 중복 구독된다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxCharacterBase.cpp` | `InitAbilitySystem()` 본문을 매번 도는 갱신과 1회성 초기화로 분리 | 수정 |

헤더 변경 없음. 새 멤버 필드 추가 없음.

### 접근 방식
- **1회성 구간만 가드**: 함수 첫머리 통짜 게이트는 틀렸다. `RefreshAbilityActorInfo()` 는 바뀐 컨트롤러를 `FGameplayAbilityActorInfo` 에 다시 물리는 호출이라 재진입마다 반드시 돌아야 한다. 그래서 `BaseWalkSpeed` 캡처와 SPD 구독만 골라 감싼다.
- **구독 여부 자체를 가드 조건으로**: 새 `bool` 플래그 대신 `IsBoundToObject(this)` 로 델리게이트에 이미 물려 있는지 직접 묻는다. SPD 값 변경 델리게이트에 `this` 를 바인딩하는 곳이 이 한 줄뿐이라 판정이 정확하고, 플래그와 실제 구독 상태가 어긋날 여지가 없다.
- **기준값 캡처를 같은 블록에**: `BaseWalkSpeed` 의 존재 이유가 SPD 콜백의 기준값이므로 구독과 수명이 정확히 일치한다.
- `GiveAbilitySet()` 은 `UWxAbilitySystemComponent::bAbilitySetGranted` 자체 가드가 이미 있어 손대지 않는다.

### 검토했으나 택하지 않은 대안
- **`BaseWalkSpeed` 필드 제거 + CDO의 `MaxWalkSpeed` 매번 조회**: 필드가 사라져 더 깔끔하지만, 레벨 배치 캐릭터가 CharacterMovement 인스턴스 오버라이드로 속도를 바꿔둔 경우 그 값을 무시하고 CDO 값으로 되돌린다. 동작 회귀라 배제.
- **`PostInitializeComponents` 로 구독 이관**: `APawn::PostInitializeComponents` 가 `SpawnDefaultController()` → `PossessedBy` → `InitAbilitySystem` → `GiveAbilitySet` 까지 태워, AI 캐릭터에서 구독이 `GiveAbilitySet` 뒤로 밀리고 초기 SPD 변경을 놓친다. 배제.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxCharacterBase.cpp` | `InitAbilitySystem()` 에서 `BaseWalkSpeed` 캡처와 SPD 콜백 구독을 `IsBoundToObject(this)` 가드 블록으로 묶고, `RefreshAbilityActorInfo()` 는 가드 밖 첫 줄로 올림 | 수정 |

### 구현·결정과 그 이유
- **통짜 게이트 대신 부분 가드**: 함수 첫머리에 초기화 플래그를 걸면 재빙의 때 `RefreshAbilityActorInfo()` 까지 죽는다. 그 호출은 바뀐 컨트롤러를 `FGameplayAbilityActorInfo` 에 다시 물리는 것이 목적이라 재진입마다 돌아야 한다.
- **새 `bool` 필드 대신 `IsBoundToObject(this)`**: 구독 여부를 직접 물으면 플래그와 실제 상태가 어긋날 여지가 없다. SPD 값 변경 델리게이트에 `this` 를 바인딩하는 곳이 이 한 줄뿐이라 판정도 정확하다.
- **`BaseWalkSpeed` 캡처를 같은 블록에**: 존재 이유가 SPD 콜백의 기준값이라 구독과 수명이 정확히 일치한다. 재진입 시 이미 스케일된 `MaxWalkSpeed` 를 다시 잡던 누적 버그가 이걸로 닫힌다.
- **`GiveAbilitySet()` 은 그대로**: `UWxAbilitySystemComponent::bAbilitySetGranted` 가 이미 재부여를 막는다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- WxEditor(Development) 빌드로 컴파일만 확인했다. 실제 재빙의·PlayerState 재복제 상황의 런타임 검증은 하지 않았다.
- `BaseWalkSpeed` 는 여전히 초기화자 없는 멤버다. 가드 블록이 콜백 구독보다 먼저 값을 채우므로 읽기 전 미초기화는 발생하지 않지만, 이 이슈 범위 밖이라 손대지 않았다.
