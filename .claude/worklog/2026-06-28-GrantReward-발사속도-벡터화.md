# GrantReward 발사 속도 벡터화 (LaunchSpeed → LaunchVelocity)

## 계획

### 목표
`FWxStateTreeTask_GrantReward`의 픽업 발사를 스칼라 `float LaunchSpeed`(항상 월드 Z 업)에서 방향+크기를 담은 `FVector LaunchVelocity`(기본값 `(0,0,300)`)로 바꿔, 디자이너가 발사 방향까지 제어하게 한다. 기본값은 기존 수직 발사와 동일하게 동작한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData `float LaunchSpeed = 300.f` → `FVector LaunchVelocity = FVector(0,0,300)`; 주석 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `EnterState`가 `Instance.LaunchVelocity` 전달 | 수정 |
| `Plugins/WxInventory/.../Public/WxRewardLibrary.h` | `GrantReward(..., float LaunchSpeed=300.f)` → `FVector LaunchVelocity=FVector(0,0,300)`; `@param` 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/WxRewardLibrary.cpp` | 시그니처 동기화; 발사를 `LaunchInDirection(LaunchVelocity, LaunchVelocity.Size())`로; 주석 갱신 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | 콜사이트를 `FVector::UpVector * LaunchSpeed`로 래핑(적 float 필드 유지, 동작 불변) | 수정 |

### 접근 방식
- **벡터를 끝까지 소비**: 태스크 필드만 벡터화하면 `GrantReward(float)`와 타입 불일치 + 방향 소실로 무의미. `GrantReward`까지 `FVector LaunchVelocity`로 연쇄 변경한다.
- **기존 발사 API 재사용**: `AWxItemPickup::LaunchInDirection(Direction, Speed)`가 내부에서 `Direction.GetSafeNormal() * Speed`라, `LaunchInDirection(LaunchVelocity, LaunchVelocity.Size())`면 최종 선속도가 정확히 `LaunchVelocity`가 된다. 영벡터면 발사 0(드랍).
- **적 동작 보존**: 적은 요청 범위 밖이라 `float LaunchSpeed` 유지, 콜사이트만 `(0,0,LaunchSpeed)`로 래핑.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData `float LaunchSpeed=300` → `FVector LaunchVelocity=(0,0,300)`; 필드·클래스 주석 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `EnterState`가 `Instance.LaunchVelocity` 전달 | 수정 |
| `Plugins/WxInventory/.../Public/WxRewardLibrary.h` | `GrantReward` 마지막 인자 `float LaunchSpeed=300` → `FVector LaunchVelocity=(0,0,300)`; `@param`·발사 주석 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/WxRewardLibrary.cpp` | 시그니처 동기화; 발사를 `LaunchInDirection(LaunchVelocity, LaunchVelocity.Size())`로; 주석 갱신 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.cpp` | 콜사이트를 `FVector::UpVector * LaunchSpeed`로 래핑(적 float 필드 유지) | 수정 |

### 구현·결정과 그 이유
- **벡터를 끝까지 소비**: 태스크 필드만 벡터화하면 `GrantReward(float)`와 타입 불일치 + 방향 소실로 무의미해, `GrantReward`까지 `FVector LaunchVelocity`로 연쇄 변경. 이름은 벡터 의미에 맞춰 `LaunchVelocity`로 통일(사용자 승인).
- **기존 발사 API 재사용**: `AWxItemPickup::LaunchInDirection(Direction, Speed)`가 내부에서 `Direction.GetSafeNormal() * Speed`라, `LaunchInDirection(LaunchVelocity, LaunchVelocity.Size())`면 최종 선속도가 정확히 `LaunchVelocity`가 된다. 영벡터면 발사 0(드랍). 새 API 추가 없이 의미 보존.
- **적 동작 보존**: 적은 요청 범위 밖이라 `float LaunchSpeed` 필드를 유지하고 콜사이트만 `(0,0,LaunchSpeed)`로 래핑해 기존 수직 발사를 그대로 재현.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **ST_TreasureChest**의 `Wx Grant Reward` 노드: 타입 변경으로 옛 `LaunchSpeed` 값이 사라지고 `LaunchVelocity`가 리셋됨 → 에디터에서 재입력(기존 동작 유지 시 `(0,0,300)`). `RewardRow` 바인딩은 영향 없음.
