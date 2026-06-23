# Wx Grant Reward 태스크 — RewardComponent를 Context 오너에서 자동 탐색

## 계획

### 목표
`FWxStateTreeTask_GrantReward`가 보상 지급에 쓸 `UWxRewardComponent`를 ST 에셋 수동 바인딩 대신 EnterState 시 Context 오너에서 자동 탐색하게 한다. 상자엔 RewardComponent가 하나뿐이라 바인딩이 기계적 반복이므로 그 단계를 제거한다. `DirectGrantTarget`(연 폰)은 오너가 모르므로 바인딩 필드로 존치.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData에서 `RewardComponent` 필드·미사용 전방선언 제거, doc 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `EnterState`에서 `FindComponentByClass<UWxRewardComponent>()`로 해결, 주석 갱신 | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | "RewardComponent 바인딩" → "오너 자동 탐색, DirectGrantTarget만 바인딩" | 수정 |

### 접근 방식
- **바인딩 필드 제거 → 자동 탐색**: 권위 가드 통과 후 `Owner->FindComponentByClass<UWxRewardComponent>()`. 못 찾으면 기존대로 `Failed`. 라이브 진입 1회뿐이라 캐싱 없이 그 자리 탐색.
- **방어적 이중 경로 미도입(YAGNI)**: "있으면 바인딩, 없으면 자동" 같은 폴백 없이 항상 자동.
- `FindComponentByClass`는 `const AActor*` const 멤버, .cpp가 이미 `WxRewardComponent.h` 포함 → 추가 의존 없음.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/.../Public/Inventory/WxRewardStateTreeNodes.h` | InstanceData `RewardComponent` 필드·`class UWxRewardComponent;` 전방선언 제거, 클래스 doc을 "Context 오너 자동 탐색"으로 갱신 | 수정 |
| `Plugins/WxInventory/.../Private/Inventory/WxRewardStateTreeNodes.cpp` | `EnterState`에서 `Owner->FindComponentByClass<UWxRewardComponent>()`로 해결, 주석 갱신 | 수정 |
| `Docs/Programmer/Reward_Grant_Flow.md` | 바인딩 서술을 "RewardComponent 자동 탐색, DirectGrantTarget만 바인딩"으로 갱신 | 수정 |

### 구현·결정과 그 이유
- **EnterState에서 그 자리 탐색(캐싱 없음)**: 태스크가 라이브 진입 1회만 동작하므로 `FindComponentByClass` 비용은 무시 가능. 인스턴스 데이터에 캐시할 이유가 없어 권위 가드 직후 단발 탐색했다.
- **항상 자동, 폴백 없음(YAGNI)**: "바인딩 있으면 그걸, 없으면 자동" 같은 이중 경로는 두지 않았다. 상자엔 RewardComponent가 하나뿐이라 자동 탐색이 항상 정답이고, 못 찾으면 잘못 배치된 것이므로 기존과 동일하게 `Failed`.
- **의존성 무변화**: 같은 플러그인 타입이고 .cpp가 이미 `WxRewardComponent.h`를 포함, `FindComponentByClass`는 `const AActor*`의 const 멤버라 추가 include·캐스트 불필요.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **콘텐츠 배선(사용자)**: ST_TreasureChest Open 상태에 `Wx Grant Reward` 배치 시 이제 `DirectGrantTarget`(→ OpeningActor)만 바인딩하면 된다(RewardComponent 바인딩 칸은 사라짐). 직전 작업 후속의 "RewardComponent=WxReward 바인딩" 단계는 불필요해졌다.
