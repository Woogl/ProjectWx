# WxCombat — 코드 리뷰

> GAS 관용구를 정확히 따르고 주석으로 함정을 잘 남겨 둔, 전반적으로 잘 정돈된 모듈이다. 다만 "서버 권위 대미지 파이프라인 → 로컬 예측 어빌리티" 사이의 이벤트 전달과 홀드 입력 라우팅에 실제 동작을 깨는 구멍이 남아 있다. 이번 리뷰는 대미지 파이프라인(ExecCalc·EffectContext·ASC 발행), 어빌리티 13종, 무기/투사체, 락온, 타임딜레이션, AnimNotify·Cue·MMC 전반을 cpp까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 가드·회피의 반응 연출이 서버에서만 실행돼 소유 클라에는 보이지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:141-243`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:148-233`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:196-256`
- **범주**: 설계/구조
- **문제**: `HandleGameplayEffectAppliedToSelf`가 발행하는 `Event.PerfectGuard`·`Event.HitReact.*`·`Event.DodgeSuccess`는 대미지 GE가 실제로 적용된 머신에서만 나온다. 대미지 GE는 Instant+Execution이라 클라 예측 경로에서 ExecCalc가 스킵되고(`Damage/WxCombatEffectContext.h:32` 주석대로 `DamageResult`가 None), 피격자 클라는 애초에 예측 키를 갖지 않으므로 **서버에서만** 발행된다. 그런데 `UWxAbility_Guard`/`UWxAbility_Dodge`는 LocalPredicted이고, 이 이벤트를 `UAbilityTask_WaitGameplayEvent`(순수 로컬 발송)로 받는다. 결과적으로 소유 클라 인스턴스는 이벤트를 영원히 못 받아 `HandleGuardHitReact`(가드 피격·가드브레이크 몽타주), `HandlePerfectGuard`(패링 몽타주·MP 회복·슬로우), `HandleDodgeSuccess`(극한 회피 몽타주)가 돌지 않는다. 서버가 대신 재생한 몽타주도 소유 클라에는 닿지 않는다 — 엔진 `UAbilitySystemComponent::OnRep_ReplicatedAnimMontage`는 `if (!AbilityActorInfo->IsLocallyControlled())` 안에서만 적용한다(UE 5.8 `AbilitySystemComponent_Abilities.cpp:3303`). `UWxAbility_HitReact`만 `ServerInitiated`라 GAS가 활성화를 클라로 복제해 정상 동작한다.
- **제안**: 두 어빌리티의 반응 페이즈를 HitReact처럼 `ServerInitiated` 어빌리티로 분리하거나, ExecCalc 판정 결과를 이미 복제되는 채널(GameplayCue의 `FWxCombatEffectContext`)로 클라까지 보내 클라 인스턴스가 스스로 페이즈를 전환하게 한다. 스탠드얼론/리슨 서버 호스트에서는 증상이 없어 눈에 띄지 않는다.
- **확신도**: 높음

### 2. 🔴 홀드 가드 입력이 매 프레임 `InputPressed`를 흘려 퍼펙트 가드 몽타주를 1프레임 만에 끊는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:40-50`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:58-72`
- **범주**: 버그/정확성
- **문제**: 입력은 `ETriggerEvent::Triggered`로 바인딩되며(`Source/WxGame/Character/WxPlayerCharacter.cpp:106`), Hold 계열 트리거는 눌린 동안 매 프레임 들어온다(ASC 헤더 주석도 그렇게 명시). Guard는 자기 `Ability.Exclusive` 애셋 태그를 스스로 `BlockAbilitiesWithTag`로 막으므로 `TryActivateAbility`가 `CanActivateAbility`에서 실패하고(엔진은 retrigger 분기보다 `CanActivateAbility`를 먼저 본다 — UE 5.8 `AbilitySystemComponent_Abilities.cpp:1817-1836`), 곧장 `AbilitySpecInputPressed(Spec)` → `UWxAbility_Guard::InputPressed`로 떨어진다. `HandlePerfectGuard`가 `PerfectGuardMontage`로 전환한 바로 다음 프레임에 `InputPressed`가 `ActiveMontage == PerfectGuardMontage`를 보고 `PlayMontage(GuardMontage)`를 호출하므로, 가드 키를 누르고 있는 정상 플레이에서는 패링 연출이 사실상 재생되지 않는다.
- **제안**: `InputPressed`의 가드 복귀를 "누르고 있음"이 아니라 실제 재입력(릴리즈 이후의 새 프레스)에서만 타게 한다 — `InputReleased`에서 플래그를 세우고 `InputPressed`가 그 플래그를 소비하거나, 라우팅 측에서 Triggered의 레벨 신호와 엣지 신호를 구분해 전달한다. 반대로 "홀드 중에는 패링 연출을 스킵한다"가 의도라면 `PerfectGuardMontage`가 존재할 이유를 주석으로 남겨야 한다.
- **확신도**: 중간(Guard InputAction의 트리거 설정에 의존한다. 다만 홀드 재발동을 전제한 설계라 one-shot이 아닐 가능성이 높다)

### 3. 🟡 `OnGranted` 자동 발동에 네트 역할 게이트와 널 검사가 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:77-85`
- **범주**: 설계/구조
- **문제**: `OnGiveAbility`는 서버 `GiveAbility` 시점뿐 아니라 클라의 스펙 복제 도착 시에도 호출된다(엔진 `FGameplayAbilitySpecContainer::PostReplicatedAdd` → `GameplayAbilityTypes.cpp:295`). 게이트가 없어 LocalPredicted 어빌리티는 서버 부여 시 1회, 클라 복제 도착 시 1회 발동을 시도하고, 서버의 두 번째 활성화가 거절되면 `ClientActivateAbilityFailed`로 클라 예측 인스턴스가 취소돼 양쪽 상태가 어긋난다. 또 `ActorInfo->AbilitySystemComponent`는 `InitAbilityActorInfo` 전에 널일 수 있는데 검사 없이 `->TryActivateAbility(...)`로 역참조한다(클라에서 스펙 복제가 `InitAbilityActorInfo`보다 먼저 도착하는 경우).
- **제안**: `ActorInfo`/ASC 널 검사를 넣고, `NetExecutionPolicy`에 맞춰 한쪽에서만 발동하도록 게이트한다(로컬 실행형이면 `IsLocallyControlled()`, 서버 실행형이면 `IsNetAuthority()`).
- **확신도**: 중간(현재 `OnGranted`를 쓰는 어빌리티 BP가 있는지는 확인하지 못했다)

### 4. 🟡 `UWxAbility_Guard::EndAbility`가 `ActorInfo` 널 검사 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:93-95`
- **범주**: 버그/정확성
- **문제**: 엔진의 `UGameplayAbility::IsEndAbilityValid`가 `ActorInfo` 널을 전제로 방어하고 있는 만큼 널로 들어올 수 있는 경로가 존재한다(활성화 전 인스턴스 정리 등). 같은 모듈의 Dodge(`WxAbility_Dodge.cpp:98`), HitReact(`WxAbility_HitReact.cpp:139`), Sprint(`WxAbility_Sprint.cpp:93`), Groggy(`WxAbility_Groggy.cpp:92`), Finisher(`WxAbility_Finisher.cpp:127`), LockOn(`WxAbility_LockOn.cpp:94`)은 모두 `if (ActorInfo …)`로 막는데 Guard만 빠졌다.
- **제안**: 동일하게 `ActorInfo` 널 가드를 추가한다.
- **확신도**: 높음

### 5. 🟡 대미지 ExecCalc가 계산 밖 부수효과를 수행해 재진입 위험을 남긴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:158`, `:166-178`, `:277-282`
- **범주**: 설계/구조
- **문제**: 커밋 `a436cd08`에서 연출·반응 이벤트 발행을 ASC로 뺐지만, ExecCalc 안에는 아직 (a) 공격자 ASC에 자원 회복 GE 적용(`UWxEffect_RecoverResource::ApplyTo`), (b) 공격자 DP 직접 가산(`SetNumericAttributeBase`), (c) **피격자 ASC의 어빌리티 취소**(`TargetASC->CancelAbilities`)가 남아 있다. 특히 (c)는 지금 실행 중인 바로 그 ASC를 건드린다 — Guard의 `EndAbility`가 `State.Guard` 루스 태그를 떼며 태그 카운트/Aggregator 콜백을 연쇄로 깨우는데, 이 시점은 `OutExecutionOutput`의 모디파이어가 아직 어트리뷰트에 반영되기 전이다.
- **제안**: 판정 결과만 `FWxCombatEffectContext`에 싣고, 가드 취소·자원 회복은 GE 적용이 끝난 뒤인 `UWxAbilitySystemComponent::HandleGameplayEffectAppliedToSelf`에서 수행한다(이미 그 경로가 있다).
- **확신도**: 중간(현재 관측된 문제는 없고 GAS가 대체로 버텨 주는 영역이다)

### 6. 🟡 `UWxAbility_Attack`과 `UWxAbility_Skill`이 사실상 동일 구현이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:24-155`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:25-133`
- **범주**: 중복/복잡도
- **문제**: 콤보 재발동 `CanActivateAbility` 분기, `EndAbility`의 EndTask+`ComboSelector.Reset()`, `PlayMontage`, 4개 몽타주 핸들러가 주석 문구까지 같다. 실제로 이미 갈라지기 시작했다 — Attack에만 `HasActiveCancelTarget` 기반 "끊고 들어가는 발동" 분기가 있다(`WxAbility_Attack.cpp:39-45, 85-102`). 한쪽만 수정하면 조용히 어긋난다.
- **제안**: 콤보 몽타주 어빌리티 공통 베이스를 만들어 몽타주 태스크 관리와 콤보 분기를 올리고, Attack은 캔슬 진입 분기만 오버라이드한다. (같은 몽타주 태스크 보일러플레이트는 Pattern·Ultimate·Death·Dodge에도 반복된다.)
- **확신도**: 높음

### 7. 🟡 투사체가 단일 대미지 진입점을 우회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:46-61`, `:132-141`
- **범주**: 설계/구조
- **문제**: README와 `UWxCombatLibrary::ApplyDamage` 주석은 ApplyDamage가 모든 대미지의 단일 진입점이라고 못박지만, 투사체는 BeginPlay에 만들어 둔 Spec을 `TargetASC->ApplyGameplayEffectSpecToSelf`로 직접 적용한다. 그 결과 `SetByCaller.HitStop`이 실릴 자리가 없어 투사체만 히트스톱이 빠지고, 대미지 경로가 둘로 갈라져 이후 파이프라인 변경이 한쪽에만 반영되기 쉽다.
- **제안**: `FWxDamageInfo`만 들고 있다가 히트 시점에 `UWxCombatLibrary::ApplyDamage(SourceASC, TargetASC, DamageInfo, HitResult, HitStopDuration)`로 통일한다.
- **확신도**: 중간(스폰 시점 ATK 스냅샷이 의도된 설계일 수 있다)

### 8. 🟢 `SetLockOnTarget`의 권위 분기가 실제로는 같은 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:29-41`
- **범주**: 중복/복잡도
- **문제**: if/else 두 갈래가 모두 `ApplyLockOnTarget(InTarget)`을 호출하고 else만 RPC를 한 줄 더한다. 권위/비권위 처리가 다른 것처럼 읽혀 오독을 부른다.
- **제안**: `ApplyLockOnTarget(InTarget);` 한 번 호출 후 `if (!GetOwner()->HasAuthority()) ServerSetLockOnTarget(InTarget);`.
- **확신도**: 높음

### 9. 🟢 Dodge 서버 경로는 EndAbility 뒤에도 태스크를 계속 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:71-88`
- **범주**: 버그/정확성
- **문제**: 리모트 클라의 서버 인스턴스에서 `CallReplicatedTargetDataDelegatesIfSet`가 동기적으로 `HandleTargetDataReceived` → `StartDodge`를 태울 수 있고, 몽타주 재생이 실패하면 그 안에서 `EndAbility`가 불린다. 그런데 제어가 `ActivateAbility`로 돌아와 87-88행이 종료된 어빌리티에 `WaitGameplayTag`/`WaitGameplayEvent` 태스크를 붙인다. 로컬 경로(66-69행)는 같은 상황을 조기 반환으로 막고 있어 서버 경로만 빠진 셈이다.
- **제안**: 서버 분기도 `StartDodge` 결과(또는 `IsActive()`)를 보고 조기 반환한다.
- **확신도**: 중간

### 10. 🟢 대미지 표기가 적중마다 액터+위젯을 스폰하고 5초를 산다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_Damage.cpp:34-41`, `:54-65`
- **범주**: 성능/안전
- **문제**: 히트 1회당 `AWxDamageFloaterActor` 스폰 + `UWidgetComponent::InitWidget()`(UMG 위젯 생성)이 일어나고 `InitialLifeSpan = 5.f`라 다수 적을 상대하는 구간에서 위젯 인스턴스가 수십 개 누적된다. 스폰 위치도 `MyTarget->GetActorLocation()`이라 이미 계산해 넘긴 `Parameters.Location`(실제 임팩트 지점)을 쓰지 않는다.
- **제안**: 플로터 풀링(또는 단일 위젯의 텍스트 큐)로 전환하고, 수명을 실제 페이드 길이에 맞춘다. 스폰 위치는 `Parameters.Location`을 쓴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxComboMontageSelector.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/`(GE·MMC·`WxExecCalc_Burn.cpp` 전량), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/`(5종), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/`(SlowTime·PlaySkillCutscene·WaitMoving), `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/`(10종), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/`(필터 태스크 5종·`WxLockOnPointComponent.cpp`·`WxRootMotionModifier_SnapToTarget.cpp`), `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/` 주요 헤더, 소비 측 확인용 `Source/WxGame/Character/WxPlayerCharacter.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**: BP/WBP 내부(InputAction 트리거 설정, 어빌리티 BP의 `ActivationPolicy`·몽타주 지정, 데이터테이블 수치)는 범위 밖이라 발견 2·3의 실제 재현 여부는 그 설정에 달려 있다. `Plugins/WxBlueprintSnapshot/Snapshots/`가 비어 있어 BP 값 교차 확인은 하지 못했다. 규칙 위반은 자동 점검(Copyright 첫 줄, `FORCEINLINE`/인라인 정의, 람다, `BlueprintCallable` 남용, 델리게이트 콜백 `Handle` prefix, 타 Wx 플러그인 참조)까지 돌렸고 **위반 0건**이다.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 151파일 — `/module-review`로 갱신*
