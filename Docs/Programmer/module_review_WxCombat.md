# WxCombat — 코드 리뷰

> GAS 전투 흐름과 서버 권위 경계가 대체로 명확하고, 확인한 범위에서는 `AGENTS.md`의 prefix·Copyright·콜백 명명·인라인·`BlueprintCallable`·플러그인 의존성 규칙 위반이 없었다. 이번 리뷰는 README를 출발점으로 Build 설정, 공개 계약, 대미지·어빌리티·무기·락온·시간 조작의 위험 경로를 중심으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 컷신 종료가 자기 무적 GE를 식별하지 못한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:27`
- **범주**: 버그/정확성
- **문제**: 태스크는 108행에서 `UWxEffect_Invincible`을 적용하지만 그 핸들을 보관하지 않고, 종료 시 같은 정의의 활성 GE 중 한 건을 `RemoveActiveGameplayEffectBySourceEffect(..., nullptr, 1)`로 제거한다. 컷신 무적과 회피·애님 노티파이 무적 또는 다른 컷신 무적이 겹치면 먼저 찾아진 다른 무적을 지우거나 자기 무적을 남길 수 있어, 의도와 다른 피격 허용 또는 무적 잔류가 발생한다.
- **제안**: 이 태스크가 적용한 GE의 핸들을 수명과 함께 보관해 그 핸들만 제거하거나, 태스크별 식별 가능한 source object/태그를 넣어 정확히 조회·제거한다.
- **확신도**: 높음

### 2. 🟡 퍼펙트 가드 뒤에도 `AdditionalEffects`가 그대로 적용된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:92`
- **범주**: 버그/정확성
- **문제**: `MakeSpecs`가 대미지 GE와 `AdditionalEffects`를 차례로 만들고, 이 루프는 대미지 GE의 실제 결과를 확인하지 않은 채 모든 스펙을 대상에게 적용한다. 퍼펙트 가드는 `UWxExecCalc_Damage`에서 HP 피해 대신 반사만 출력하고 반환하지만, 같은 공격의 상태 이상·디버프는 계속 적용된다. 따라서 퍼펙트 가드가 피해를 막아도 공격 부가효과에 피격될 수 있다.
- **제안**: 대미지 판정 결과를 먼저 확정한 뒤 부가효과 적용 여부를 분기한다. 퍼펙트 가드에서도 남겨야 하는 효과가 있다면 데이터 행에 별도 정책을 두어 의도를 명시한다.
- **확신도**: 중간

### 3. 🟡 애님 노티파이의 상태 GE 적용 경로가 활성 어빌리티 부재에서 널을 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:125`
- **범주**: 버그/정확성
- **문제**: `ApplyEffect`는 `PredictingAbility->GetAbilityLevel()`을 먼저 호출하면서도 128행부터는 `PredictingAbility`가 null일 수 있음을 전제로 처리한다. 호출부 `WxAnimNotifyState_ApplyGameplayEffect`는 `ASC->GetAnimatingAbility()`를 그대로 넘기므로, GAS 어빌리티가 아닌 몽타주·시퀀스에서 이 노티파이가 실행되거나 활성 어빌리티가 이미 끝난 프레임에는 널 역참조로 크래시한다.
- **제안**: `PredictingAbility`가 없을 때 안전한 기본 레벨(예: 1)을 사용하고, 예측 키는 현재처럼 유효한 어빌리티가 있을 때만 설정한다. 노티파이를 GAS 어빌리티 전용으로 제한할 의도라면 호출부에서 이를 검증·로그로 드러낸다.
- **확신도**: 높음

### 4. 🟡 컷신 원점 계산이 없는 스켈레탈 메시를 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:88`
- **범주**: 버그/정확성
- **문제**: `AvatarCharacter` 존재만 확인한 뒤 `AvatarCharacter->GetMesh()->GetComponentTransform()`을 호출한다. `ACharacter`가 존재해도 메시 컴포넌트가 없거나 초기화 전이면 널 역참조가 되어 컷신 시작 시 크래시한다.
- **제안**: `GetMesh()` 결과를 별도로 검사하고, 없을 때는 이미 준비된 `AvatarActor->GetActorTransform()` 폴백을 사용한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전반, 나머지 `AbilitySystem/Ability/`, `AbilitySystem/Effect/`, `AbilitySystem/Cue/`, `AnimNotify/`, `Targeting/`, `Damage/`의 cpp 및 대응 헤더
- **미검토 / 한계**: BP/WBP·몬타주·DataTable 자산 내부와 실제 멀티플레이 런타임은 검토하지 않았다. 대미지 수치식과 타게팅 프리셋의 밸런스 타당성은 기획 범위로 보류했다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 152파일 — `/module-review`로 갱신*
