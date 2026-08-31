# WxCombat — 코드 리뷰

> 권위·예측 경계가 주석과 코드 양쪽에 꼼꼼히 드러나 있고 어빌리티/대미지 파이프라인의 순서 설계도 견고하다. 다만 "GE를 클래스로 걷어내는" 공유 소유권 지점과 락온 권위, 그리고 대미지 판정의 결정성에는 실패 경로가 남아 있다. 이번 리뷰는 README·Build.cs·uplugin으로 경계를 잡은 뒤 ASC·어빌리티 베이스와 파생 전체·AttributeSet·ExecCalc·CombatLibrary·무기/투사체·락온·타임딜레이션·AnimNotify·AbilityTask를 깊게 보고, Public 헤더 전체와 Effect/Cue/Targeting 전 파일을 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 구간 종료가 같은 클래스의 상태 GE를 전부 벗긴다 (주석의 설명이 엔진 동작과 반대다)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:30`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:27`
- **범주**: 버그/정확성
- **문제**: `RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1)`의 세 번째 인자는 "지울 인스턴스 개수"가 아니라 "각 인스턴스에서 뺄 스택 수"다. 엔진(`AbilitySystemComponent.cpp:1292`)은 Instigator가 `nullptr`이면 그 클래스의 활성 GE를 **전부** 매칭한 뒤 `RemoveActiveEffects(Query, StacksToRemove)`로 매칭된 하나하나에서 스택을 뺀다. `UWxEffect_Invincible`은 스택 설정이 없어(`WxEffect_Invincible.cpp:9`) StackCount가 1이므로, 스택 1 제거는 곧 전체 제거다. 결과적으로 회피 무적 ANS의 `NotifyEnd`가 `UWxAbility_Finisher`의 `ActivationOwnedEffects`(`WxAbility_Finisher.cpp:33`)나 컷신 태스크가 건 무적까지 같이 벗겨 낸다. 30행 바로 위의 주석("수량을 1로 잡아야 … 이미 걸린 처형 무적까지 벗기지 않는다")은 실제 동작과 정반대라 다음 수정자를 잘못된 방향으로 끌고 간다.
- **제안**: 적용 시 받은 `FActiveGameplayEffectHandle`을 ANS/태스크가 보관해 그 인스턴스만 `RemoveActiveGameplayEffect`로 걷는다. 예측 키 정합 뒤 핸들이 무효해지는 경로는 소유 어빌리티를 SourceObject로 실어 `FGameplayEffectQuery`로 좁히거나, 소유자별 전용 GE 클래스로 분리한다. 어느 쪽이든 30행의 주석을 사실에 맞게 고쳐야 한다.
- **확신도**: 높음

### 2. 🔴 서버 락온 RPC가 클라이언트가 고른 대상을 충분히 검증하지 않고, 거절해도 예측을 되돌리지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp:40`
- **범주**: 설계/구조
- **문제**: `ServerSetLockOnTarget_Implementation`은 전달된 컴포넌트가 `UWxLockOnPointComponent`이고 `CanBeLockedOn()`(사망 태그만 검사)인지만 본다. 클라이언트가 후보 선별에 쓴 `TargetingPreset`의 거리·팀·시야·화면 필터를 서버가 재검증하지 않으므로, 소유 클라가 임의의 복제된 락온 지점을 보내면 정상 후보 밖의 대상도 서버 `LockOnTarget`이 된다. 이 값은 서버 투사체 호밍(`WxProjectileBase.cpp:58`)과 모션 워핑 스냅(`WxRootMotionModifier_SnapToTarget.cpp:31`)이 그대로 소비한다. 반대 방향도 비어 있다 — 서버가 요청을 무시해도(50행 조건 불충족) 클라이언트가 32행에서 이미 로컬 적용한 값을 되돌리지 않고, 서버 `LockOnTarget`이 변하지 않았으면 RepNotify도 오지 않아 로컬 예측이 그대로 굳는다.
- **제안**: 서버에서 대상 소유 액터에 대해 적대 관계·최대 거리(가능하면 같은 Preset)를 다시 검사한다. 거절 시에는 권위값을 강제로 되돌리는 경로(예: 클라 전용 정정 RPC)를 두어 로컬 예측을 즉시 롤백한다.
- **확신도**: 높음

### 3. 🟡 대미지 ExecCalc의 크리 난수가 예측 클라와 서버에서 따로 굴러간다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:170`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드 공유가 없는 로컬 난수다. 무기 히트 경로는 `AWxWeaponBase::ProcessHit`(`WxWeaponBase.cpp:254`의 "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다")가 권위 검사 없이 돌고, `UWxCombatLibrary::ApplyDamage`가 어빌리티 활성화 예측 키를 실어 `ApplyGameplayEffectSpecToTarget`을 호출한다(`WxCombatLibrary.cpp:130`). 예측 키가 유효하면 엔진은 Instant GE의 Execution을 클라이언트에서도 실행하므로, 같은 히트에 대해 클라와 서버가 서로 다른 크리 결과를 낸다. 그 결과 클라의 예측 HP·GP·SP 델타가 서버와 어긋나 복제 정정 때 값이 튀고, 컨텍스트에 기록되는 `SetCritical`(192행)도 머신마다 달라진다.
- **제안**: 크리 판정을 권위에서만 굴리고(`ExecutionParams`의 타깃 ASC에서 `IsOwnerActorAuthoritative()` 확인), 클라는 컨텍스트/복제로 도착한 결과만 읽게 한다. 예측 단계에서도 값이 필요하면 예측 키·히트 인덱스 같은 양쪽이 공유하는 값에서 시드를 만들어 결정적으로 굴린다.
- **확신도**: 중간(단일 플레이에서는 드러나지 않아 의도적 유예일 수 있다)

### 4. 🟡 투사체가 `None` 판정을 유효 충돌처럼 연출하고 파괴한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:90`
- **범주**: 버그/정확성
- **문제**: `CheckDamage` 결과를 `Evaded`인지 여부로만 축약해 `None`이 `bEvaded == false`로 접힌다. 그래서 판정이 성립하지 않는 대상(ASC가 없는 액터, `Ability.Death`가 붙은 시체)에도 91~94행에서 임팩트 FX가 나가고, 권위 측에서는 124행 분기로 투사체까지 파괴된다. 클래스 주석이 명시한 계약("판정이 성립하지 않는 Pawn은 … 이펙트도 파괴도 없이 그대로 통과한다", `WxProjectileBase.h:19`)과 코드가 어긋난다.
- **제안**: `EWxDamageCheck`를 세 갈래로 그대로 분기한다. `Damaged`일 때만 임팩트와 파괴, `Evaded`는 회피 이벤트만, `None`은 충돌 자체를 무시하고 반환한다.
- **확신도**: 높음(시체 케이스는 사망 시 콜리전 해제가 가려 줄 수 있으나, ASC 없는 액터 경로는 그대로 남는다)

### 5. 🟢 GP 드레인이 즉시 틱 때문에 몽타주보다 한 주기 먼저 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainGP.cpp:16`
- **범주**: 버그/정확성
- **문제**: `bExecutePeriodicEffectOnApplication = true`로 적용 즉시 한 번 실행하는데, 틱당 차감량은 54행에서 `-(MaxGP / Duration) * Period`로만 계산한다. 첫 차감이 `t=0`에 일어나 총 실행 횟수가 하나 더 많으므로 GP는 의도한 `Duration`보다 `DrainPeriod`만큼 먼저 0에 닿고, `UWxAbility_Groggy::HandleGPChanged`가 그로기 몽타주 끝보다 이르게 어빌리티를 종료한다.
- **제안**: `bExecutePeriodicEffectOnApplication`을 끄거나, 즉시 실행을 유지한다면 실제 실행 횟수(`Duration / Period + 1`)를 기준으로 틱당 차감량을 계산한다.
- **확신도**: 높음

### 6. 🟢 콤보 진행 로직이 세 어빌리티에 그대로 복사되어 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:29`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:31`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:29`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언(`WxAbility_Attack.h:34/38`, `WxAbility_Skill.h:37/41`, `WxAbility_Pattern.h:31/34`)과 "다음 단 계산 → 몽타주 재생 → 실패 시 종료" 블록, `EndAbility`의 `bWasCancelled` 리셋이 세 클래스에 문자 그대로 중복된다. `WxAbility_Attack`과 `WxAbility_Skill`은 태그·카테고리를 빼면 구현이 동일하다. 콤보 규칙을 바꿀 때 세 곳을 같이 고쳐야 하고, 한 곳을 놓치면 종류별로 동작이 갈린다.
- **제안**: `ComboMontages`/`ComboIndex`와 진행·리셋을 `UWxAbilityBase`(또는 중간 베이스) 한 곳으로 올리고, 파생은 태그·그룹 선언만 남긴다. 진행 방식이 다른 `UWxAbility_Pattern`(`HandleMontageBlendOut`으로 자동 연쇄)은 그 훅만 오버라이드한다.
- **확신도**: 중간(의도된 설계일 수 있음 — 파생마다 독립적으로 진화시키려는 선택일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Public/` 전체 헤더, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지 파생(Attack·Skill·Pattern·Passive·Sprint·Death·GuardReact·BeingFinished·Ultimate), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 전체
- **규칙 점검 결과**: `Wx` prefix, 델리게이트 콜백의 `Handle` prefix, override의 `Super::` 호출, 소스 첫 줄 저작권(일부 파일은 UTF-8 BOM이 앞서지만 문구는 존재), 인라인 함수 정의 없음, 람다 없음, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 한 건(Blueprint Function Library라 허용), `WxCombat.Build.cs`/`uplugin`의 Wx 의존성은 `WxCore` 단독 — 모두 `CLAUDE.md` 규칙을 지킨다. 규칙 위반 발견 없음.
- **미검토 / 한계**: BP/WBP 내부 구조, 몽타주에 실제로 배치된 노티파이 구성, DataTable 행 값, `TargetingPreset` 에셋의 필터 조합은 범위 밖이다. 발견 3은 실제 네트워크 세션에서만 드러나므로 실측하지 않았고, 런타임 프로파일링도 수행하지 않았다. 직전 리뷰의 "카메라 노티파이가 첫 로컬 플레이어 시점을 바꾼다"는 현재 `WxAnimNotifyState_CameraMove.cpp:28`의 주석이 그 동작을 의도로 명시하고 있어 이번에는 제외했다.

---
*문서 기준 커밋 `cad31360` · 리뷰일 2026-08-31 · 소스 152파일 — `/module-review`로 갱신*
