# 어빌리티 비용 조회의 deprecated API 정리

## 계획

### 목표
빌드마다 뜨던 `FGameplayEffectSpec::GetModifierMagnitude(int32, bool)` deprecated 경고를 없앤다. UE 5.6 에서 예고된 제거 대상이라 다음 엔진 버전에서 컴파일이 깨진다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | `GetCost` 의 `GetModifierMagnitude(ModifierIndex, true)` 를 신 오버로드로 | 수정 |

### 접근 방식
- **인자만 제거**: 신 오버로드는 스택 반영 여부를 호출부 인자가 아니라 GE 애셋의 `bFactorInStackCount` 로 판단한다. 즉 판단 주체가 호출부에서 데이터로 옮겨간 API 변경이다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | `:430` 의 `bFactorInStackCount` 인자 제거 | 수정 |

### 구현·결정과 그 이유
- **이 호출부에서는 동작이 바뀌지 않음을 확인하고 인자만 뗐다**: 겉보기엔 `true` 를 넘기던 것을 애셋 값에 맡기는 의미 변화지만, 여기 쓰이는 `CostSpec` 은 표시용으로 갓 만들어 어디에도 적용하지 않는 스펙이다. `FGameplayEffectSpec` 생성자가 `StackCount(1)` 로 시작하고(`GameplayEffect.cpp:1540`), `ComputeStackedModifierMagnitude` 는 StackCount 가 1이면 모든 ModifierOp 에서 입력을 그대로 돌려준다(Override 는 무변경, 나머지는 `(x - bias) * 1 + bias`). 따라서 `bFactorInStackCount` 가 무엇이든 결과가 같아 순수 무동작 변경이다.
- **호출부에서 스택 배수를 직접 곱하는 보정을 넣지 않은 이유**: 위 이유로 보정할 차이가 없다. 오히려 신 API 쪽이 낫다 — 훗날 비용 GE 를 스택형으로 저작하면 표시가 애셋 설정을 따라가고, 호출부가 `true` 를 박아 두는 것보다 저작 의도에 맞는다.
- **주석을 남기지 않은 이유**: 코드 한 줄에서 읽히지 않는 근거라 워크로그에 둔다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 프로젝트 내 다른 `GetModifierMagnitude` 호출부는 없다(전수 확인). 빌드 경고 0.
