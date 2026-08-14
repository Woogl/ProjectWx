# WxEffect_StateWindow 제거

## 계획

### 목표
부여할 태그를 인자로 받는 범용 GE(`UWxEffect_StateWindow`)만 「태그마다 전용 GE」 규칙 밖에 남아 있다. 정의만 봐서는 무엇을 부여하는지 알 수 없고 GE 정의로 조회할 수도 없으며, `Effect.Invincible`이 두 클래스에서 나온다. 전용 GE로 통일하고 이 클래스를 없앤다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_PerfectGuard.h/.cpp` | HasDuration GE. `Effect.PerfectGuard` 부여·애셋 태그, 정적 `ApplyTo` | 신규 |
| `WxCombat/.../Effect/WxEffect_Invincible.h/.cpp` | 구간 길이를 받는 정적 `ApplyTo` 추가 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotifyState_Invincible.cpp` | `UWxEffect_Invincible::ApplyTo` 호출로 교체 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotifyState_PerfectGuard.cpp` | `UWxEffect_PerfectGuard::ApplyTo` 호출로 교체 | 수정 |
| `WxCombat/.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `UWxEffect_Invincible::ApplyTo` 호출로 교체 | 수정 |
| `WxCombat/.../Effect/WxEffect_StateWindow.h/.cpp` | 파일 삭제 | 삭제 |
| `WxCore/.../WxGameplayTags.h` | 규칙 4·`Effect.Invincible`·`Effect.PerfectGuard` 주석의 발행처 갱신 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | `ActivationOwnedEffects` 항목의 근거 문장 갱신 | 수정 |

### 접근 방식
- **한 클래스가 두 수명을 받는다**: `UWxEffect_Invincible`은 `Infinite` 정의를 유지하되, 구간이 정해진 호출자가 `Spec.SetDuration(Duration, true)`로 덮는다. `bDurationLocked`가 서면 이후 `CalculateDurationFromDef`의 무한 재계산이 막히므로 어빌리티 수명(무한)과 노티파이 구간(유한)을 같은 클래스로 처리한다.
- **구간을 Begin/End 짝으로 바꾸지 않는다**: 노티파이 상태 객체는 몽타주 에셋에 하나뿐이라 액터별 핸들을 못 들고, 결국 정의 조회로 제거해야 해서 같은 GE를 건 다른 출처까지 걷어간다. 처형이 `PreActivate`에서 회피를 캔슬하면 회피 몽타주 정지의 `NotifyEnd`가 다음 갱신에 도착하는데, 그때는 처형이 이미 `UWxEffect_Invincible`을 건 뒤라 처형의 무적이 벗겨진다. 지속시간 방식은 인스턴스가 각각 만료되므로 이 문제가 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_PerfectGuard.h/.cpp` | HasDuration GE. `Effect.PerfectGuard` 부여·애셋 태그, 정적 `ApplyTo` | 신규 |
| `WxCombat/.../Effect/WxEffect_Invincible.h/.cpp` | 구간 길이를 받는 정적 `ApplyTo` 추가, 두 수명을 다 받는다는 설명으로 클래스 주석 교체 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotifyState_Invincible.cpp` | `UWxEffect_Invincible::ApplyTo` 호출로 교체, 태그 헤더 의존 제거 | 수정 |
| `WxCombat/.../AnimNotify/WxAnimNotifyState_PerfectGuard.cpp` | `UWxEffect_PerfectGuard::ApplyTo` 호출로 교체, 태그 헤더 의존 제거 | 수정 |
| `WxCombat/.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `UWxEffect_Invincible::ApplyTo` 호출로 교체, 태그 헤더 의존 제거 | 수정 |
| `WxCombat/.../Effect/WxEffect_StateWindow.h/.cpp` | 파일 삭제 | 삭제 |
| `WxCore/.../WxGameplayTags.h` | 규칙 4·`Effect.Invincible`·`Effect.PerfectGuard` 주석의 발행처 갱신 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | `ActivationOwnedEffects` 제거 범위 서술 갱신 | 수정 |

### 구현·결정과 그 이유
- **`Effect.Invincible`은 클래스 하나로 합쳤다**: 정의는 무한으로 두고 구간이 정해진 호출자만 `Spec.SetDuration(Duration, true)`로 덮는다. `SetDuration`이 `bDurationLocked`를 세우면 적용 단계의 `CalculateDurationFromDef`가 무한 지속시간을 다시 싣지 못하므로, 어빌리티 수명과 노티파이 구간을 한 GE가 받는다. 태그마다 GE가 하나라는 규칙이 지켜지고 GE 정의로 조회·제거도 된다.
- **구간을 Begin/End 짝으로 바꾸지 않았다**: 노티파이 상태 객체는 몽타주 에셋에 하나뿐이라 액터별 핸들을 못 든다. 정의 조회로 제거하면 같은 GE를 건 다른 출처까지 걷어가는데, 처형이 `PreActivate`에서 회피를 캔슬하면 회피 몽타주 정지의 `NotifyEnd`가 다음 갱신에 도착해 이미 걸린 처형의 무적을 벗긴다. 지속시간 방식은 인스턴스가 각각 만료되므로 겹치는 출처가 서로를 건드리지 않는다.
- **`ApplyTo`를 GE마다 두는 중복을 감수했다**: 두 몸통이 사실상 같지만, 이 중복을 없애려면 태그를 인자로 받는 공용 GE가 다시 필요해져 없애려던 것이 돌아온다. `UWxEffect_Exhaust`·`UWxEffect_RecoverResource`가 이미 GE별 정적 `ApplyTo`를 두는 패턴이기도 하다.
- **`UWxEffect_PerfectGuard`는 HasDuration**: 어빌리티 활성 구간으로 쓰이는 일이 없어 지속시간 없는 적용을 열어 둘 이유가 없다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 인게임 회귀 미검증(빌드까지만 확인): 회피 i-frame·퍼펙트 가드 판정 구간, 궁극기 컷신 무적, 처형 무적이 회피 캔슬과 겹칠 때 유지되는지.
