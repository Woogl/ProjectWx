# WxCombat — 코드 리뷰

> GAS 위에 얹은 어빌리티·대미지 파이프라인의 골격은 견고하고, 태그 누수·콜백 레이스·콤보 재진입처럼 까다로운 실패 경로를 실패복구 코드와 주석으로 촘촘히 막아 놓았다. 프로젝트 코딩·모듈 규칙 위반도 거의 없다. 남은 위험은 전역 상태(타임 딜레이션)의 소유권과 히트 판정의 네트워크 권위 모델에 몰려 있다. 이번 리뷰는 어빌리티 베이스와 주요 파생(Attack/Skill/Dodge/Guard/HitReact/Finisher/Groggy/Death/LockOn/Ultimate), 대미지 파이프라인(`WxExecCalc_Damage`·`FWxDamageInfo`·`WxCombatAttributeSet`), 무기/투사체 히트, 입력 라우팅 ASC, 락온·타임딜레이션·모션워핑 계열을 cpp까지 내려가 보았고, GE/Cue/MMC·타게팅 필터·AnimNotify는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 전역 타임 딜레이션의 소유자가 둘이라 슬로우가 겹치면 월드가 슬로우에 갇힌다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:30`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:51-52`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:46-49`
- **범주**: 설계/구조
- **문제**: 전역 시간 배율을 바꾸는 주체가 두 갈래다. `WxAbilityTask_SlowTime`은 복제 컴포넌트(`UWxTimeDilationComponent`)를 거치는데 `OnDestroy`에서 참조 카운트 없이 무조건 `1.f`로 되돌리고, `WxAbilityTask_PlaySkillCutscene`은 컴포넌트를 우회해 `UGameplayStatics::SetGlobalTimeDilation`을 직접 호출한 뒤 `OriginalTimeDilation`으로 복원한다. 컴포넌트는 `FMath::IsNearlyEqual(ReplicatedTimeDilation, NewDilation)`이면 조기 반환하므로, 우회 호출로 캐시와 실제 값이 어긋나면 이후 요청이 통째로 무시된다.
  구체적 실패 시나리오(싱글플레이에서도 재현): 퍼펙트 가드 슬로우(`WxAbility_Guard.cpp:275`, 기본 0.4배·0.4초)가 도는 중 궁극기를 쓰면 컷신 태스크가 `OriginalTimeDilation=0.4`를 캡처하고 월드를 0.001로 만든다(`WxAbility_Ultimate.cpp:36`). 컷신 도중 슬로우 태스크가 만료되며 컴포넌트가 월드를 `1.0`으로 되돌리고 `ReplicatedTimeDilation=1.0`을 기록한다. 컷신이 끝나면 캡처해 둔 `0.4`를 복원한다 — 이제 월드는 0.4배인데 컴포넌트는 1.0이라고 믿으므로, 다음 슬로우 이펙트가 발동하기 전까지 게임 전체가 슬로우 모션에 갇힌다. 겹침 자체(극한 회피 `WxAbility_Dodge.cpp:292` + 퍼펙트 가드)만으로도 먼저 끝난 쪽이 남은 슬로우를 걷어간다.
- **제안**: 딜레이션 소유권을 `UWxTimeDilationComponent` 하나로 모으고(컷신 태스크도 이 경로 사용), 요청을 스택/참조 카운트로 관리해 마지막 요청이 사라질 때만 원래 값으로 복원한다. 하드코딩 `1.f` 복원을 없애면 위 시나리오가 통째로 사라진다.
- **확신도**: 중간(겹침 순서에 의존하지만 경로 자체는 코드로 확인됨)

### 2. 🟡 근접 무기 히트 판정이 클라에서 매 틱 돌지만 결과는 전부 버려진다 (주석이 사실과 다름)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:253-268`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:33`
- **범주**: 성능/안전
- **문제**: `ProcessHit`의 주석(255-256행)은 "클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며 불일치하면 GAS가 자동 롤백한다"고 선언하지만, 모듈 어디에도 `FScopedPredictionWindow`가 없고 `UWxCombatLibrary::ApplyDamage`는 `ApplyGameplayEffectSpecToTarget(Spec, Target)`을 예측 키 인자 없이(기본 `FPredictionKey()`) 호출한다. GAS의 `ApplyGameplayEffectSpecToSelf`는 `HasNetworkAuthorityToApplyGameplayEffect`(권위 또는 유효 예측 키)가 실패하면 그 자리에서 빈 핸들을 돌려주므로, 클라에서의 히트는 대미지·큐·히트리액트를 하나도 만들지 못한다. 그런데 비용은 그대로 든다 — `Tick`(171-219행)이 공격 구간 내내 매 프레임 `SweepMultiByObjectType`을 돌리고, 히트마다 `FWxDamageInfo::MakeSpecs`로 GE Spec을 만들어 버린다. 화면 안 교전 중인 캐릭터 수만큼 곱해진다. 같은 성격의 미게이팅이 `WxEffectZone.cpp:18-70`(오버랩 시 GE 적용, authority 검사 없음)에도 있다. 반대로 `WxAnimNotify_AreaAttack.cpp:20`과 `WxProjectileBase.cpp:103`은 제대로 게이팅돼 있어 모듈 안에서 모델이 갈린다.
- **제안**: 예측을 실제로 도입하든(어빌리티 활성화 스코프 안에서 유효 예측 키를 실어 보냄), 클라 판정을 포기하고 `BeginAttack`/`Tick`/`ProcessHit`를 `HasAuthority()`로 게이팅하든 하나로 정한다. 최소한 주석은 현재 동작에 맞게 고쳐야 다음 사람이 "롤백되고 있다"고 오해하지 않는다.
- **확신도**: 높음

### 3. 🟡 입력 라우팅이 `ActivatableAbilities`를 락 없이 순회하며 활성화한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:42-71`, 같은 파일 `81-103`
- **범주**: 버그/정확성
- **문제**: `for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())` 루프 안에서 `TryActivateAbility`/`AbilitySpecInputPressed`를 호출한다. 엔진의 `GiveAbility`·`ClearAbility`·`ClearAllAbilities`는 `AbilityScopeLockCount > 0`일 때만 변경을 pending 큐로 미루고, 락이 없으면 `ActivatableAbilities.Items`를 즉시 Add/RemoveAtSwap 한다. 즉 활성화 도중 어빌리티 목록이 바뀌면(GE의 `GrantedAbilities`, `RemoveAfterActivation` 스펙, 활성화가 유발한 액터 파괴 등) 순회 중인 참조와 이터레이터가 무효화된다. 엔진과 Lyra가 이 순회를 항상 `ABILITYLIST_SCOPE_LOCK()`으로 감싸는 이유가 이것이다. 현재 코드베이스에 확정 트리거가 보이지는 않지만, 발현하면 증상이 메모리 손상이라 원인 추적이 매우 어렵다.
- **제안**: 두 루프를 `ABILITYLIST_SCOPE_LOCK()`으로 감싸거나, 매칭되는 `FGameplayAbilitySpecHandle`을 먼저 수집한 뒤 루프 밖에서 활성화한다.
- **확신도**: 중간

### 4. 🟡 퍼펙트 가드 반사가 ExecCalc 안에서 소스 ASC의 어트리뷰트를 직접 쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:199-211`
- **범주**: 설계/구조
- **문제**: Execution의 계약은 "타깃에 대한 OutputModifier만 내보낸다"인데, 여기서는 소스 ASC의 DP를 `SetNumericAttributeBase`로 직접 갈아쓴다. GE 스펙·컨텍스트·태그·면역·복제/예측 경로를 전부 우회하므로 반사 DP는 추적·차단·롤백이 불가능하고, 그로기 유발이라는 큰 게임플레이 결과가 GE 파이프라인 밖에서 결정된다. 같은 함수 137행에서는 자원 회복을 `UWxEffect_RecoverResource::ApplyTo`(GE 경로)로 처리하고 있어 한 파일 안에서 방식이 엇갈린다.
- **제안**: DP 가산 전용 Instant GE를 만들어 `SourceASC`에 적용한다. 회복 쪽과 동일한 경로가 되어 로그·면역·복제가 표준 처리된다.
- **확신도**: 중간(의도된 단축일 수 있음)

### 5. 🟡 락온 종료가 `bOrientRotationToMovement`를 저장값이 아니라 `true`로 하드코딩 복원한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:45`, 같은 파일 `103`
- **범주**: 설계/구조
- **문제**: 활성화 시 `false`로 끄고 종료 시 무조건 `true`로 되돌린다. 기본값이 `false`인 폰(스트레이프 이동 등)이나, 같은 플래그를 끄고 있는 다른 연출(회전·루트모션 구간)이 락온 해제 시점과 겹치면 그쪽 상태가 조용히 덮어써져 CMC 회전이 튄다. 락온은 대상 소실·거리 초과·사망 등 임의 타이밍에 해제되므로 겹칠 여지가 넓다.
- **제안**: 활성화 시점의 값을 멤버에 저장했다가 그 값으로 복원한다.
- **확신도**: 중간

### 6. 🟡 궁극기 컷신을 발동 순간 동기 로드한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:33`
- **범주**: 성능/안전
- **문제**: `CutsceneSequence`는 `TSoftObjectPtr<ULevelSequence>`(`WxAbility_Ultimate.h:28`)인데 `ActivateAbility` 안에서 `LoadSynchronous()`로 즉시 로드한다. LevelSequence는 참조 에셋(카메라·머티리얼·사운드 등)까지 함께 끌어오므로 궁극기를 처음 쓰는 순간 프레임 히치가 난다 — 하필 연출이 시작되는 지점이다.
- **제안**: 어빌리티 부여 시점이나 전투 진입 시점에 비동기 프리로드(`FStreamableManager::RequestAsyncLoad`)로 잡아 두고, 활성화 시에는 이미 로드된 포인터만 쓴다.
- **확신도**: 높음

### 7. 🟡 헤더에 인라인 함수 정의가 있다 (코딩 규칙 6 위반)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:44`
- **범주**: 규칙 위반
- **문제**: `FWxInputActionTriggeredSignature& OnInputActionTriggered() { return OnInputActionTriggeredDelegate; }` — CLAUDE.md 코딩 규칙 6("인라인 함수 정의를 금지한다")에 걸린다. 모듈 147개 소스 중 유일한 사례라 일관성 측면에서도 튄다.
- **제안**: 선언만 헤더에 두고 정의를 `WxAbilitySystemComponent.cpp`로 옮긴다.
- **확신도**: 높음

### 8. 🟢 `AWxWeaponBase::PrevCapsuleRotation`은 쓰기만 하고 읽지 않는 데드 멤버다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:111`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:76`, `218`
- **범주**: 중복/복잡도
- **문제**: `BeginAttack`과 `Tick`에서 갱신되지만 어디서도 읽히지 않는다. Sweep(`WxWeaponBase.cpp:210`)은 시작·끝 모두 `CurrRotation`을 쓴다. "직전 프레임 회전을 반영한다"는 오해만 남긴다.
- **제안**: 멤버와 두 대입문을 제거한다(회전 보간이 실제로 필요해지면 그때 Sweep 인자로 반영).
- **확신도**: 높음

### 9. 🟢 사망 어빌리티의 래그돌 지연 타이머가 정리되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:83`
- **범주**: 버그/정확성
- **문제**: `DeathMontage`가 없을 때 0.15초 타이머를 걸지만 `UWxAbility_Death`에는 `EndAbility` 오버라이드가 없어 `RagdollDelayTimerHandle`을 해제하는 경로가 존재하지 않는다. 어빌리티가 그 사이 종료되면 뒤늦게 `EnableRagdoll()`이 돌아 `State.Ragdoll` 태그가 붙는다.
- **제안**: `EndAbility`를 오버라이드해 `ClearTimer(RagdollDelayTimerHandle)`를 호출한다(`WxAbility_Groggy.cpp:96-97`이 쓰는 것과 같은 패턴).
- **확신도**: 중간

### 10. 🟢 HitReact에 주석 처리된 로직 블록이 남아 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:75-85`, 같은 파일 `20-21`
- **범주**: 중복/복잡도
- **문제**: "패턴 중 Normal 피격 무시" 분기가 통째로 주석으로 남아 있어, 폐기된 규칙인지 잠시 꺼둔 것인지 다음 세션이 판단할 수 없다. 피격 반응은 그로기/가드/Unblockable/Knock 계열로 분기가 이미 복잡해 이런 잔재의 비용이 특히 크다.
- **제안**: 되살릴 계획이면 조건을 데이터(태그·플래그)로 노출하고, 아니면 삭제한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Invincible.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_LineTrace.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_ScreenBounds.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxMMC_CooldownDuration.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxMMC_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_RecoverResource.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitInputActionTriggered.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxEffectZone.cpp`
- **미검토 / 한계**: (1) `WxAbility_Pattern`, `WxExecCalc_Burn`, `WxMMC_LinearDrain`, 나머지 `WxEffect_*`/`WxCueNotify_*`, `WxTargetingFilterTask_Team/GameplayTag/InputDirection`은 구조만 확인하고 수치·타이밍은 검증하지 않았다. (2) 발견 1의 겹침 시나리오와 발견 3의 컨테이너 무효화는 코드 경로 추적으로 도출한 것이며 실제 재현 테스트는 하지 않았다. (3) 콤보 DataTable·GE 에셋 설정값·TargetingPreset 등 데이터 자산과 BP/WBP 내부는 범위 밖이다.

---
*문서 기준 커밋 `c37b6fa6` · 리뷰일 2026-07-31 · 소스 147파일 — `/module-review`로 갱신*
