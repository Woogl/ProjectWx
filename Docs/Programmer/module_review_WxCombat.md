# WxCombat — 코드 리뷰

> GAS 위에 올린 전투 모듈로서 권한 게이트·태그 수명·엔진 순정 경로 위임이 전반적으로 잘 지켜져 있고, 위험한 지점마다 근거 주석이 붙어 있어 건강한 편이다. 이번 리뷰는 ASC·어빌리티 전체·대미지 파이프라인(ExecCalc/AttributeSet/EffectContext)·무기/투사체·어빌리티 태스크·AnimNotify를 cpp까지 읽었고, GE 생성자·GameplayCue·타게팅 필터·MMC는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |
| ⚪ 철회(검증 후 오탐) | 1 |

`CLAUDE.md` 규칙 위반은 발견하지 못했다. 소스 153개 전부 저작권 첫 줄을 갖췄고, 람다·`FORCEINLINE`·헤더 인라인 정의가 없으며, 델리게이트 콜백은 전부 `Handle` prefix를 쓴다. `BlueprintCallable`은 `UWxCombatLibrary`(Blueprint Function Library) 한 곳뿐이고, Build.cs·uplugin이 참조하는 Wx 플러그인은 `WxCore`가 유일하다.

## 결과

### 1. 🟡 다중 충전 쿨다운이 런타임 생성 GE를 Spec.Def로 쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:107`
- **범주**: 버그/정확성
- **문제**: `MaxRecharges > 1`이면 어빌리티 인스턴스를 Outer로 `NewObject<UWxEffect_Cooldown>`을 만들어 쿨다운 GE 정의로 넘긴다. 이 오브젝트는 네트워크 주소가 없어(복제 서브오브젝트도, 이름이 안정적인 디폴트 서브오브젝트도 아님) 복제 대상이 아니다. `FGameplayEffectSpec::Def`는 복제되는 `UPROPERTY` 오브젝트 참조라, 서버가 적용한 충전 쿨다운이 소유 클라에 도착하면 `Def`가 널로 풀린다. 엔진은 이때 `FActiveGameplayEffect::PostReplicatedAdd`에서 에러 로그만 남기고 그 GE를 등록하지 않으므로(UE 5.8 `GameplayEffect.cpp:2811`), 클라의 예측분이 키 확정과 함께 걷힌 뒤에는 쿨다운이 사라져 `CheckCooldown`이 통과한다 — 재입력이 로컬에선 나가고 서버에선 거부되는 디싱크가 된다. 단일 충전(공유 CDO) 경로와 스탠드얼론/리슨 호스트는 영향이 없다.
- **제안**: 충전 수를 GE 인스턴스로 나르지 말고, 최대 충전 수는 `AbilityDataRow`에서 소비 측(ViewModel)이 직접 읽게 한다. GE는 공용 CDO 하나로 유지하면 복제 문제가 사라진다.
- **확신도**: 중간

### 2. 🟡 히트스톱이 서버에서만 적용돼 공격 소유 클라에는 보이지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:230`, 같은 파일 `:268`
- **범주**: 설계/구조
- **문제**: `HandleGameplayEffectAppliedToSelf`는 `EWxDamageResult::None`이면 곧장 빠져나가는데, 클라 예측 경로는 ExecCalc를 건너뛰어 항상 None이다. 따라서 `Event.HitStop` 발행도 `ApplyHitStop`도 권위 머신에서만 일어난다. `ApplyHitStop`이 부르는 `CurrentMontageSetPlayRate`는 권위 측에서 `AnimMontage_UpdateReplicatedData`로 시뮬 프록시에만 전파되고, 엔진의 `OnRep_ReplicatedAnimMontage`는 locally controlled 액터를 건너뛴다. 결과적으로 데디케이티드 서버에서는 정작 때린 플레이어 본인 화면만 역경직이 빠진다(리슨 호스트·스탠드얼론은 정상).
- **제안**: 타격 판정이 이미 서버 권위이므로, 히트스톱만 공격자에게 별도로 알리는 경로를 두거나(클라 전용 Cue 또는 공격자 대상 RPC), 멀티 미지원을 의도로 확정한다면 그 범위를 README/코드에 명시한다.
- **확신도**: 중간(현재 개발 형태가 단일 플레이·리슨 호스트라면 의도된 범위일 수 있음)

### 3. ⚪ (철회) 회피의 TargetData 델리게이트가 해제되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:76`, 같은 파일 `:323`
- **최초 주장**: 서버가 리모트 플레이어의 방향 데이터를 받으려고 `AbilityTargetDataSetDelegate`에 붙인 `HandleTargetDataReceived`를 `EndAbility`에서 떼지 않아, (1) 종료된 인스턴스에서 `StartDodge`가 돌고 (2) `AbilityTargetDataMap` 엔트리가 누적된다.
- **판정**: 오탐(2026-08-12 엔진 소스로 검증). 엔진 `UGameplayAbility::EndAbility`가 `ClearAbilityReplicatedDataCache(Handle, CurrentActivationInfo)`를 직접 호출하고(UE 5.8 `GameplayAbility.cpp:891`), 이 함수는 (Spec, PredictionKey) 엔트리를 통째로 제거한다. 델리게이트는 그 엔트리 안에 살므로 함께 걷힌다. 바인딩에 쓴 키와 엔진이 지우는 키도 같은 값이다 — `PreActivate`가 `CurrentActivationInfo`에 인자를 그대로 복사한다. 제거된 엔트리는 free 풀로 가고 `FindOrAdd`가 재사용 직전에 `ResetAll()`로 델리게이트를 비우므로, 재활용 엔트리가 옛 핸들러를 부르는 교차 호출도 없다.
- **잔여**: 서버 어빌리티가 이미 끝났거나 활성 자체가 거부된 뒤에 클라의 TargetData RPC가 도착하면 `ServerSetReplicatedTargetData`가 엔트리를 새로 만들고 아무도 지우지 않는다. GAS TargetData를 쓰는 모든 코드에 공통이고, 활성과 데이터 두 RPC가 신뢰 순서로 붙어 도착하므로 실제로 벌어질 여지는 사실상 없다.

### 4. 🟡 Attack과 Skill이 거의 통째로 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:66`~`:155`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:63`~`:133`
- **범주**: 중복/복잡도
- **문제**: `ComboSelector`·`MontageTask` 멤버, `EndAbility`의 EndTask+Reset 처리, `PlayMontage`, 몽타주 핸들러 4개, `CanActivateAbility`의 콤보 분기까지 주석 문구를 포함해 사실상 동일하다. 헤더까지 합치면 네 파일에 걸친 반복이라, 콤보 재발동 규칙을 바꿀 때 한쪽만 고치면 두 어빌리티의 동작이 조용히 갈린다. 실제로 `CanActivateAbility`는 이미 형태가 미세하게 달라져 있다(Attack은 `HasActiveCancelTarget` 경로가 추가되고 반환 표현식이 다름).
- **제안**: 콤보 몽타주 재생 규약만 담은 중간 베이스(예: `UWxAbility_ComboMontage`)로 올리고, Attack은 캔슬 진입 분기만 얹는다. 다만 이 프로젝트는 구조 추출보다 인플레이스 반복을 선호하는 방침이 있으므로, 유지하기로 한다면 두 파일이 짝이라는 사실을 주석으로 못박아 동시 수정 대상임을 남긴다.
- **확신도**: 높음(중복 자체는 사실이며, 처방 선택은 방침 문제)

### 5. 🟢 AbilitySet 회수 경로가 호출자 없는 데드 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:65`
- **범주**: 중복/복잡도
- **문제**: `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`은 저장소 전체에 호출자가 없고, ASC가 들고 있는 `AbilitySetGrantedHandles`도 채워지기만 할 뿐 아무도 읽지 않는다. 호출자 없는 방어적 선언을 두지 않는다는 프로젝트 방침과 어긋나고, 읽는 사람에게 "부여를 되돌리는 경로가 있다"는 잘못된 인상을 준다.
- **제안**: 회수 요구가 실제로 생길 때 다시 추가하고 지금은 제거한다.
- **확신도**: 높음

### 6. 🟢 그로기의 State.Groggy 구독이 엔진 동작과 중복이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:51`, 같은 파일 `:146`
- **범주**: 중복/복잡도
- **문제**: 이 어빌리티는 `EGameplayAbilityTriggerSource::OwnedTagPresent`로 등록돼 있는데(`:28`), 엔진은 태그 카운트가 0이 되면 해당 스펙을 스스로 취소한다(UE 5.8 `AbilitySystemComponent_Abilities.cpp:2687`). 어빌리티가 부여 시점에 걸리는 엔진 콜백이 먼저 돌기 때문에 `HandleGroggyTagChanged`는 이미 종료된 어빌리티에서 호출돼 무효 처리된다. 즉 `bWasCancelled=false`로 끝내려던 의도는 실제로 적용되지 않고, 코드만 남아 오해를 만든다. `State.Dead` 구독은 태그가 달라 여전히 필요하다.
- **제안**: `GroggyTagDelegateHandle`과 `HandleGroggyTagChanged`를 제거하고, 종료가 엔진 취소로 이뤄진다는 사실을 주석으로 남긴다.
- **확신도**: 중간

### 7. 🟢 락온 종료가 회전 모드를 하드코드 true로 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:46`, 같은 파일 `:101`
- **범주**: 설계/구조
- **문제**: 진입 시 `bOrientRotationToMovement = false`, 종료 시 무조건 `true`다. 진입 전 값을 복원하는 게 아니라 특정 값을 가정하는 구조라, 이 플래그를 쓰는 다른 주체와 겹치면 조용히 덮어쓴다. 현재 `WxAI`의 `WxAIPerceptionComponent`도 같은 플래그를 직접 토글하고 있어(적 전용이라 지금은 대상이 겹치지 않는다) 쓰기 주체가 이미 둘이다. 또 `GetCharacterMovement()` 반환값을 널 검사 없이 역참조한다.
- **제안**: 진입 시점 값을 저장했다 복원하거나, 회전 모드를 계산 원천이 발행하는 상태로 일원화한다.
- **확신도**: 낮음(현재 기본값이 true라 동작상 문제는 없음)

### 8. 🟢 SetLockOnTarget의 두 분기가 같은 일을 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:29`
- **범주**: 중복/복잡도
- **문제**: `if (HasAuthority())`와 `else`가 둘 다 `ApplyLockOnTarget(InTarget)`을 부르고, else만 서버 RPC를 덧붙인다. 분기가 갈리는 것처럼 보여 읽는 쪽이 차이를 찾게 만든다.
- **제안**: `ApplyLockOnTarget`을 앞으로 빼고, 권위가 아닐 때만 `ServerSetLockOnTarget`을 호출한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 아래 GE·MMC 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, 나머지 `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp`, 대응 Public 헤더 전반
- **미검토 / 한계**: 어빌리티·GE·데이터테이블의 실제 에셋 값과 BP 서브클래스 설정은 범위 밖이라 코드 계약만 봤다. 발견 1·2는 UE 5.8 엔진 소스를 읽어 메커니즘을 확인했을 뿐 PIE 멀티(데디케이티드 서버) 실측은 하지 않았다. GameplayCue·타게팅 필터·소형 GE 생성자는 수치 타당성까지 파고들지 않았다.

---
*문서 기준 커밋 `5c81cd70` · 리뷰일 2026-08-12 · 소스 153파일 — `/module-review`로 갱신*
