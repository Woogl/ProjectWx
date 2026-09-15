# HitReact 차단을 슈퍼아머·무적 GE 쪽 선언으로 이동

## 계획

### 목표
`UWxAbility_HitReact`의 `ActivationBlockedTags`가 `Effect.SuperArmor`·`Effect.Invincible`을 직접 나열하고 있어, 효과의 의미가 GE와 수신 어빌리티 두 곳에 흩어져 있다. 막는 선언을 GE의 `UBlockAbilityTagsGameplayEffectComponent`(`Ability.HitReact` 차단)로 옮겨 "효과가 막는 건 효과가 선언하고, HitReact의 `ActivationBlockedTags`에는 라우팅 조건(`Ability.Guard`)만 남긴다"로 규칙을 맞춘다. 동작은 바뀌지 않는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_SuperArmor.cpp` | `UBlockAbilityTagsGameplayEffectComponent`를 `CreateDefaultSubobject` + `GEComponents.Add`로 부착해 `Ability.HitReact` 차단. 기존 TargetTags·AssetTags 유지 | 수정 |
| `WxCombat/.../Effect/WxEffect_Invincible.cpp` | 같은 방식으로 `Ability.HitReact` 차단 추가. `Effect.Invincible` 부여는 대미지 경로가 읽으므로 유지. 패리 역경직 경로 주석 | 수정 |
| `WxCombat/.../Ability/WxAbility_HitReact.cpp` | `ActivationBlockedTags`에서 `Effect_Invincible`·`Effect_SuperArmor` 삭제, 외부 차단 주석 확장. `Ability.Guard`는 유지 | 수정 |
| `WxCombat/.../Effect/WxEffect_SuperArmor.h`, `WxEffect_Invincible.h` | 클래스 주석에 HitReact 차단 한 구절 추가 | 수정 |

### 접근 방식
- **엔진 표준 GE 컴포넌트로 차단**: GE 추가·제거 시 `AddActiveGameplayEffectGrantedTagsAndModifiers`가 태그 부여와 `BlockAbilitiesWithTags`를 같은 자리에서 처리한다(서버 전용 조건 없음, 억제 시 함께 해제). 발동 검사도 `DoesAbilitySatisfyTagRequirements`에서 BlockedAbilityTags와 ActivationBlockedTags를 연달아 보므로 판정 시점이 같다.
- **중첩 안전**: `BlockedAbilityTags`는 카운트 컨테이너라 궁극기 슈퍼아머와 컷신 무적이 겹쳐도 한쪽 해제가 다른 쪽을 풀지 않는다.
- **패리 포함**: `Event.Hit.Parry`도 같은 HitReact를 발동하므로 동일하게 막힌다.
- **Guard는 이동 대상 아님**: HitReact·GuardReact가 같은 태그를 봐야 하는 라우팅 조건이다.
- **에셋 영향 없음**: `.uasset` 전체에 `SuperArmor` 참조가 없어 BP가 `ActivationBlockedTags`를 덮어쓴 곳이 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxEffect_SuperArmor.cpp` | `Ability.HitReact`를 막는 BlockAbilityTags 컴포넌트 부착 | 수정 |
| `WxCombat/.../Effect/WxEffect_Invincible.cpp` | 같은 컴포넌트 부착 + 패리 경로 주석 | 수정 |
| `WxCombat/.../Ability/WxAbility_HitReact.cpp` | `ActivationBlockedTags`에서 `Effect.Invincible`·`Effect.SuperArmor` 삭제, 외부 차단 주석을 사망·무적·슈퍼아머로 확장 | 수정 |
| `WxCombat/.../Effect/WxEffect_SuperArmor.h`, `WxEffect_Invincible.h` | 클래스 주석에 HitReact 차단 명시 | 수정 |

### 구현·결정과 그 이유
- **GE Component는 `CreateDefaultSubobject` + `GEComponents.Add`**: 기존 TargetTags·AssetTags 부착과 같은 방식이다. 베이스 GE 생성자에서 `FindOrAddComponent`를 쓰지 않는 프로젝트 규칙을 따랐다.
- **부여 태그(`Effect.SuperArmor`·`Effect.Invincible`)는 유지**: 무적 태그는 대미지 경로·투사체·회피 태스크가 직접 읽는다. 슈퍼아머 태그는 조건으로 읽는 곳이 사라졌지만, `Effect.*` 섹션의 "GE가 부여하고 애셋 태그로도 쓴다" 규칙과 디버그 관찰을 위해 남겼다.
- **무적 쪽 차단에만 주석**: 무적 대상에게는 `Event.Hit`이 가지 않으므로 차단이 불필요해 보인다. 하지만 공격자에게 가는 `Event.Hit.Parry`는 이 차단으로 막힌다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- PIE 미검증(빌드만 통과). 확인 항목:
  - 궁극기 중 평타에 맞으면 경직 없이 대미지만 들어간다.
  - 회피 무적 구간에서 맞으면 DodgeSuccess가 발생하고 경직이 없다.
  - 공격자가 무적일 때 퍼펙트 가드에 막혀도 패리 역경직이 없다.
  - 효과가 끝난 뒤 다시 경직이 발생한다.
