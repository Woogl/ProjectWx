# ActivationOwnedEffects 도입

## 계획

### 목표
"어빌리티가 도는 동안 유지되는 조건"을 거는 방법이 세 갈래(처형의 `ActivationOwnedTags` 직접 발행, 궁극·가드의 수동 GE 적용/제거 복붙)로 갈려 있다. `UWxAbilityBase`에 `ActivationOwnedEffects`를 두어 한 경로로 모으고, `Effect.*` 태그를 `ActivationOwnedTags`로 올리던 예외를 없앤다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_Invincible.h/.cpp` | Infinite GE. `Effect.Invincible`을 부여 태그·애셋 태그로 발행 | 신규 |
| `WxCombat/.../Ability/WxAbilityBase.h/.cpp` | `ActivationOwnedEffects` 프로퍼티, `ActivateAbility`/`EndAbility` 오버라이드, `RemoveActivationOwnedEffect` | 수정 |
| `WxCombat/.../Ability/WxAbility_Finisher.cpp` | `ActivationOwnedTags`의 `Effect.Invincible` → `ActivationOwnedEffects` | 수정 |
| `WxCombat/.../Ability/WxAbility_Ultimate.h/.cpp` | 수동 적용·`EndAbility` 오버라이드 삭제, 생성자에서 `ActivationOwnedEffects` 등록 | 수정 |
| `WxCombat/.../Ability/WxAbility_Guard.cpp` | 수동 적용·`EndAbility` 제거 블록 삭제, 가드 브레이크는 `RemoveActivationOwnedEffect`로 | 수정 |
| `WxCore/.../WxGameplayTags.h` | 규칙 4의 예외 문장·`Effect.*` 태그 주석 갱신 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 공통 파이프라인 ①③⑤에 `ActivationOwnedEffects` 반영 | 수정 |

### 접근 방식
- **`ActivationOwnedTags`의 GE판**: `TArray<TSubclassOf<UGameplayEffect>>`를 활성 구간에 묶는다. `ActivateAbility`에서 `ApplyGameplayEffectToOwner`로 걸고 `EndAbility`에서 걷으며, 두 훅 모두 구체 어빌리티가 이미 `Super`를 부르므로 추가 규약이 없다.
- **핸들 대신 정의 쿼리로 제거**: 예측 GE 핸들은 서버본 도착 시 무효해지지만, `RemoveActiveEffects`가 엔진에서 이미 권위 게이팅되므로 제거는 어차피 서버에서만 일어난다. 핸들 멤버를 두지 않고 `FGameplayEffectQuery::EffectDefinition`으로 찾는다 — 기존 `QueryActiveCooldowns`와 같은 방식이고, 태그 기준보다 좁아 같은 태그를 발행하는 다른 GE를 건드리지 않는다.
- **범위 밖**: 스프린트의 이동속도·SP 소모 GE는 SetByCaller 크기를 스펙에 실어야 해서 클래스만 나열하는 이 경로에 맞지 않는다. 현행 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_Invincible.h/.cpp` | Infinite GE. `Effect.Invincible`을 부여 태그·애셋 태그로 발행 | 신규 |
| `WxCombat/.../Ability/WxAbilityBase.h/.cpp` | `ActivationOwnedEffects` 프로퍼티, `ActivateAbility`/`EndAbility` 오버라이드, `RemoveActivationOwnedEffect` | 수정 |
| `WxCombat/.../Ability/WxAbility_Finisher.cpp` | `ActivationOwnedTags`의 `Effect.Invincible` → `ActivationOwnedEffects` | 수정 |
| `WxCombat/.../Ability/WxAbility_Ultimate.h/.cpp` | 생성자에서 `ActivationOwnedEffects` 등록, 수동 적용·`EndAbility` 오버라이드 삭제 | 수정 |
| `WxCombat/.../Ability/WxAbility_Guard.cpp` | 생성자에서 `ActivationOwnedEffects` 등록, 수동 적용·`EndAbility` 제거 블록 삭제, 가드 브레이크는 `RemoveActivationOwnedEffect` | 수정 |
| `WxCore/.../WxGameplayTags.h` | 규칙 4의 예외 문장을 발행처 병기로 고치고 새 등록처 명시, `Effect.Invincible` 주석 갱신 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 공통 파이프라인 ①에 `ActivationOwnedEffects` 항목 신설, ③⑤ 서술 갱신 | 수정 |

### 구현·결정과 그 이유
- **`ActivateAbility`/`EndAbility` 훅**: 커밋 훅(`CommitExecute`)이 아니라 활성/종료 쌍에 걸었다. `ActivationOwnedTags`와 같은 구간을 뜻해야 이름이 거짓말을 하지 않고, 커밋하지 않는 어빌리티(피격·사망 등)도 나중에 쓸 수 있다. 대가로 커밋 실패 시 같은 콜스택 안에서 걸었다 걷는 왕복이 생기는데, 모든 소비자가 동기 코드라 관측되지 않는다.
- **핸들을 들지 않고 정의로 조회해 제거**: 예측 GE 핸들은 서버본 도착 시 무효해져 멤버로 들어도 못 쓴다. 게다가 `RemoveActiveEffects`는 엔진이 권위로 게이팅하므로(`AbilitySystemComponent.cpp:1832`) 제거는 어차피 서버에서만 일어나고 클라는 복제로 받는다 — 즉 핸들을 들 이유가 없다. 기존 `RemoveActiveEffectsWithGrantedTags`보다 좁아, 같은 `Effect.Invincible`을 발행하는 `WxEffect_StateWindow`(노티파이 구간)를 처형 종료가 걷어가는 일도 원천 차단된다.
- **`UWxEffect_Invincible` 신설**: 처형만 태그를 `ActivationOwnedTags`로 직접 올리는 예외였는데, 이를 없애려면 어빌리티 수명을 갖는 GE가 필요했다. 지속시간이 정해진 무적은 `WxEffect_StateWindow`가 계속 맡아, 「수명을 누가 쥐느냐」로 두 GE가 갈린다.
- **가드 브레이크만 조기 제거를 남겼다**: 브레이크 연출은 완주해야 해서 어빌리티가 살아 있는 채로 방어 판정만 떼야 한다. 이 한 사례를 위해 `RemoveActivationOwnedEffect`를 열어 두되, 제거 코드가 어빌리티마다 복제되지 않도록 Base가 소유한다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 인게임 회귀 미검증(빌드까지만 확인): 가드 유지·브레이크 시 `Effect.Guard` 해제 시점, 궁극기 슈퍼 아머 구간, 처형 연출 중 무적과 캔슬 시 누수 여부.
