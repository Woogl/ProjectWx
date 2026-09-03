# ApplyEffect 기본 GE 레벨 교정

## 계획

### 목표
`UWxCombatLibrary::ApplyEffect` 가 어빌리티 없이 호출되면 GE 스펙을 Level 0 으로 만든다. GAS 관례상 레벨은 1부터이므로 폴백을 `1.f` 로 바꾼다. (`module_review_WxCombat.md` 발견 8)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp` | `:156` 레벨 폴백 `0.f` → `1.f` | 수정 |

### 접근 방식
- **엔진 관례 확인**: `UGameplayAbility::GetAbilityLevel()` 자체가 인스턴스가 없을 때 `1` 을 반환한다(`GameplayAbility.cpp:1805-1810`). 공개 API 기본값도 모두 1 이다 — `UGameplayAbility::MakeOutgoingGameplayEffectSpec(..., float Level = 1.f)`, `UAbilitySystemBlueprintLibrary::MakeSpecHandle(..., float InLevel = 1.0f)`, `MakeSpecHandleByClass(..., float Level = 1.0f)`. 즉 "어빌리티 문맥이 없을 때의 레벨"에 대한 엔진의 답이 1 이다.
- **`FGameplayEffectSpec` 생성자 기본값(`INVALID_LEVEL` = -1.f)은 쓰지 않는다**: 그 값은 "아직 정하지 않음" 표식이고, `SetLevel` 이 그대로 `Period.GetValueAtLevel(InLevel)` 등에 넘기므로(`GameplayEffect.cpp:1902-1914`) 커브를 -1 구간에서 읽는다. 0 과 같은 종류의 문제라 해결이 되지 않는다.
- **모듈 내 일관성**: 다른 스펙 생성부는 전부 1 을 넘긴다 (`WxDamageTableRow.cpp:17,55`, `WxEffect_HitStop.cpp:42`, `WxEffect_Exhaust.cpp:44`, `WxAbilitySet.cpp:45`). 이 한 곳만 예외였다.
- **도달 가능성**: 리뷰는 잠재 결함으로 봤으나 널 경로는 실제로 도달한다 — `WxPlayerCharacter.cpp:147` 이 `nullptr` 을 명시 전달하고, `WxAnimNotifyState_ApplyGameplayEffect.cpp:19` 의 `GetAnimatingAbility()` 도 널일 수 있다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp` | 레벨 폴백을 `1.f` 로 바꾸고 근거 주석 한 줄 추가 | 수정 |

### 구현·결정과 그 이유
- **`1.f` 채택**: 엔진이 "어빌리티 문맥 없음"에 대해 내놓는 답과 같은 값이다. `GetAbilityLevel()` 의 비인스턴스 반환값이 1 이므로, 삼항의 두 갈래가 같은 의미를 갖게 된다.
- **주석을 한 줄 남겼다**: 숫자만 보면 왜 0 이 아니라 1 인지 다시 묻게 되므로 근거를 짧게 붙였다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 같은 파일의 `ApplyAttributeChange` 도 `ApplyGameplayEffectToSelf` 에 레벨 `0.f` 를 넘긴다. 현재는 `FScalableFloat(Delta)` 가 커브 없는 상수라 무해하고, 그 함수는 발견 1(🔴 런타임 GE 이름 충돌)에서 통째로 손볼 대상이라 이번엔 건드리지 않았다.
