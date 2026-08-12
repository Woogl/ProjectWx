# WxCombat — 코드 리뷰

> GAS 규약을 정확히 지키며 서버/클라 권위 분리와 태그 누수 방지까지 의식적으로 다룬 모듈이다 — 코딩 규칙 위반은 한 건도 없고, 위험 지점 대부분에 이미 근거 주석이 달려 있다. 이번 리뷰는 대미지 파이프라인(ExecCalc·EffectContext·AttributeSet·CombatLibrary), ASC 허브, 어빌리티 13종, 무기·투사체·락온·타임딜레이션을 cpp까지 읽었고 GE/MMC/Cue/타게팅 필터는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 가드 페이즈 상태가 서버에만 존재해, 원격 클라의 입력 릴리즈가 GuardBreak·PerfectGuard를 끊는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:26-39`, `:154-214`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:160-249`
- **범주**: 버그/정확성
- **문제**: `InputReleased`는 `ActiveMontage`가 GuardBreak/PerfectGuard일 때만 종료를 보류한다(`WxAbility_Guard.cpp:33`). 그런데 `ActiveMontage`를 그 값으로 바꾸는 `HandleGuardHitReact`/`HandlePerfectGuard`는 `Event.HitReact`·`Event.PerfectGuard` 수신으로만 호출되고, 그 이벤트는 `HandleGameplayEffectAppliedToSelf`에서 `SendGameplayEventToActor`로 발행된다(`WxAbilitySystemComponent.cpp:214`, `:235`). 이 핸들러는 판정 결과가 실린 서버에서만 통과하고(`:193` DamageResult::None 조기 반환), `SendGameplayEventToActor`는 복제되지 않는다. 결과적으로 LocalPredicted 가드의 **클라 인스턴스는 영원히 `ActiveMontage == GuardMontage`** 상태다. 원격 클라가 브레이크·패링 연출 도중 가드 키를 떼면 클라 인스턴스가 게이트를 통과해 `EndAbility(..., bReplicateEndAbility=true)`를 부르고, 이는 ServerEndAbility로 서버 인스턴스까지 끝낸다 — "스스로 끝나야 하는 페이즈는 입력 릴리즈로 끊지 않는다"는 의도가 원격 클라에서만 깨진다. 스탠드얼론·리슨 호스트에서는 두 인스턴스가 같은 객체라 재현되지 않는다.
- **제안**: 페이즈를 클라도 아는 상태로 올린다 — 브레이크/패링 진입 시 서버가 복제 루스 태그(예: `State.Guard.Break`)를 붙이고 `InputReleased` 게이트를 그 태그로 판정하거나, 종료 판단 자체를 서버 인스턴스로 옮긴다(클라 릴리즈는 서버에 의사만 전달).
- **확신도**: 중간 (이벤트 발행이 서버 전용인 것은 코드로 확정. 실제 체감 여부는 데디케이티드 구성에서만 확인 가능)

### 2. 🟡 `OnGiveAbility`가 ActorInfo의 ASC를 검증 없이 역참조하고, 아바타 준비 전에 패시브를 발동시킨다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:77-85`
- **범주**: 버그/정확성
- **문제**: `ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)`에 널 검사가 없다. `AbilitySystemComponent`는 `TWeakObjectPtr`이며 `InitAbilityActorInfo` 전에는 비어 있다. 서버 경로는 `AWxCharacterBase::InitAbilitySystem`이 `RefreshAbilityActorInfo` 후에 부여하므로 안전하지만(`Source/WxGame/Character/WxCharacterBase.cpp:185-200`), 클라이언트는 스펙 복제(`OnRep_ActivateAbilities`)로 `OnGiveAbility`가 불리므로 캐릭터의 액터 정보 초기화보다 먼저 도달할 수 있다. 그 경우 `ActivationPolicy == OnGranted`인 어빌리티에서 널 역참조가 된다. 아바타가 아직 없는 시점의 발동이라는 문제도 함께 있다(몽타주·캐릭터를 쓰는 패시브는 조용히 실패).
- **제안**: `ActorInfo`와 `ActorInfo->AbilitySystemComponent.IsValid()`를 검사하고, 자동 발동 시점을 `OnAvatarSet`(아바타 확정 시 호출)으로 옮긴다.
- **확신도**: 중간 (현재 `OnGranted`을 쓰는 어빌리티가 있는지는 BP 에셋이라 확인하지 못했다. 쓰이지 않는다면 잠재 결함)

### 3. 🟡 EndAbility 오버라이드가 `Super` 판정 전에 부수효과를 무조건 실행한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:75-94`, `WxAbility_Dodge.cpp:92-109`, `WxAbility_Sprint.cpp:92-123`, `WxAbility_Finisher.cpp:117-137`
- **범주**: 설계/구조
- **문제**: 이 오버라이드들은 태그 제거·GE 제거·타이머 해제를 `Super::EndAbility` **앞에서** 무조건 수행한다. 엔진의 `UGameplayAbility::EndAbility`는 (a) `IsEndAbilityValid`가 거짓이면 아무 것도 하지 않고, (b) `ScopeLockCount > 0`이면 실제 종료를 뒤로 미룬다. 즉 "종료되지 않았는데 정리만 끝난" 상태가 만들어질 수 있다 — 스코프 락 중 종료 요청이 들어오면 가드는 `State.Guard`를 즉시 잃은 채로 잠시 더 활성이고, 그 사이 들어온 피격은 ExecCalc에서 무방비로 판정된다. `UWxAbility_Sprint`는 반대로 GE 제거만 `Super` 뒤에 두어(`:109-122`) 같은 함수 안에서 순서가 엇갈린다.
- **제안**: 정리 블록을 `if (IsEndAbilityValid(Handle, ActorInfo))`로 감싸거나, `Super::EndAbility` 호출 이후 실제 종료 여부를 확인하고 정리한다. Sprint는 정리 위치를 한쪽으로 통일한다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — 현재 알려진 호출 경로에서는 재현 사례를 특정하지 못했고, 태그 제거는 `HasMatchingGameplayTag` 가드로 대부분 멱등하다)

### 4. 🟡 `UWxAbility_Attack`과 `UWxAbility_Skill`의 콤보 로직이 통째로 중복이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:24-155`, `WxAbility_Skill.cpp:25-133`
- **범주**: 중복/복잡도
- **문제**: 두 클래스의 `CanActivateAbility`(콤보 재발동 분기), `EndAbility`(MontageTask EndTask + bWasCancelled 시 ComboSelector.Reset), `PlayMontage`, 몽타주 핸들러 4종, 그리고 멤버(`ComboSelector`, `MontageTask`)가 사실상 동일하다. 실질적 차이는 Attack에만 있는 `HasActiveCancelTarget` 분기 하나뿐이다. 콤보 재발동은 이 모듈에서 가장 미묘한 제어 흐름(EndTask로 콜백을 끊어야 진행 상태가 보존된다)인데, 그 규약이 두 곳에 복제돼 있어 한쪽만 고치면 조용히 갈라진다.
- **제안**: `ComboSelector`+`MontageTask`+재발동 규약을 담은 중간 베이스(예: `UWxAbility_ComboBase : UWxAbilityBase`)로 올리고, Attack은 캔슬 진입 분기만 오버라이드한다.
- **확신도**: 높음

### 5. 🟢 접촉점 산출 블록이 무기와 투사체에 그대로 복제돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:210-233`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:108-130`
- **범주**: 중복/복잡도
- **문제**: `bFromSweep`이면 SweepResult를 쓰고 아니면 `GetClosestPointOnCollision`으로 ImpactPoint/Location을 채우는 15줄이 두 파일에 문자 단위로 동일하게 존재한다. 한쪽만 개선하면 히트 위치 기준이 갈라져 Cue 위치가 어긋난다.
- **제안**: `UWxCombatLibrary`에 오버랩 → FHitResult 변환 헬퍼를 하나 두고 양쪽이 부른다.
- **확신도**: 높음

### 6. 🟢 아바타를 잃은 상태로 회피가 끝나면 판정 캡슐이 켜진 채 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:312-322`
- **범주**: 버그/정확성
- **문제**: `DeactivateJudgementCapsule`은 아바타를 `ACharacter`로 캐스팅하지 못하면 콜리전을 끄기 전에 조기 반환한다. 사망·언포제스 등으로 아바타가 사라진 뒤 `EndAbility`가 이 경로를 타면, `ECC_Pawn` 오브젝트 타입의 판정 캡슐이 회피 시작 지점에 분리된 채 QueryOnly로 남는다. 이후 무기 스윕(`WxWeaponBase.cpp:189-198`, ECC_Pawn 오브젝트 쿼리)이 그 유령 콜라이더를 맞히고 `HitActorsThisSwing`의 1회 판정을 소모한다(대미지는 `State.Dead`로 막히므로 피해는 없다).
- **제안**: 콜리전 비활성화를 캐릭터 유효성 검사보다 먼저 수행하고, 재부착만 캐릭터가 있을 때 한다.
- **확신도**: 높음 (영향 범위는 작음)

### 7. 🟢 일부 AnimNotifyState가 `MeshComp`를 검증 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp:12`, `:25`, `WxAnimNotifyState_Invincible.cpp:12`, `:25`, `WxAnimNotifyState_PerfectGuard.cpp:12`, `:25`, `WxAnimNotifyState_WeaponAttack.cpp:16`, `:32`
- **범주**: 성능/안전
- **문제**: 같은 폴더의 `WxAnimNotify_StartRecovery.cpp:12`, `WxAnimNotifyState_SnapToTarget.cpp:19`, `WxAnimNotifyState_CameraMove.cpp:22`는 `MeshComp` 널 검사를 하는데 위 네 파일은 하지 않는다. 이들은 무적·퍼펙트가드 같은 루스 태그를 여닫는 노티파이라, 여기서 예외가 나면 태그가 열린 채로 남는다. 같은 부류의 `GetWorld()` 미검증 역참조가 `WxAbilitySystemComponent.cpp:155`, `WxAbilityBase.cpp:201`, `WxAbilityTask_SlowTime.cpp:21`, `:41`에도 있다.
- **제안**: 널 가드를 붙여 파일 간 규약을 통일한다.
- **확신도**: 중간 (현재 호출 경로에서 널이 오는 사례는 확인되지 않았다 — 일관성·방어 목적)

### 8. 🟢 투사체가 클라이언트에서도 쓰지 않을 대미지 Spec을 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:71-91`, `:46-61`
- **범주**: 성능/안전
- **문제**: `BeginPlay`가 무조건 `InitializeDamageSpec`을 호출해 EffectContext와 GE Spec 배열을 만들지만, 소비처인 `HandleHitCollisionOverlap`은 `HasAuthority()` 뒤에서만 쓴다(`:103-141`). 클라이언트에서는 투사체 1발마다 DataTable 조회 + Spec 힙 할당이 버려진다. 탄막형 패턴에서 누적된다.
- **제안**: `InitializeDamageSpec` 호출을 `HasAuthority()` 안으로 옮긴다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `WxAbility_Guard.cpp`, `WxAbility_Dodge.cpp`, `WxAbility_Attack.cpp`, `WxAbility_Skill.cpp`, `WxAbility_Finisher.cpp`, `WxAbility_HitReact.cpp`, `WxAbility_Groggy.cpp`, `WxAbility_Sprint.cpp`, `WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `WxAbilityTask_PlaySkillCutscene.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체(GE·MMC·ExecCalc_Burn), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전반, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`
- **규칙 점검 결과**: CLAUDE.md 코딩·모듈 규칙 위반 없음 — 소스 153개 전부 `// Copyright Woogle. All Rights Reserved.`로 시작하고, `FORCEINLINE`·인라인 정의 0건, 람다 0건, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 한 곳(Blueprint Function Library)뿐이며, 델리게이트 콜백은 전부 `Handle` prefix다. `Build.cs`·`.uplugin` 모두 Wx 의존은 `WxCore`만이고 타 Wx 플러그인 헤더 include도 없다.
- **미검토 / 한계**: 어빌리티·GE·무기의 실제 데이터(BP 파생 클래스, DataTable 행, 몽타주 섹션·노티파이 배치)는 범위 밖이라 "몽타주 섹션 이름이 `EWxDodgeDirection`과 일치하는가", "`ActivationPolicy=OnGranted`를 쓰는 BP가 있는가" 같은 데이터 의존 규약은 확인하지 못했다(BP 스냅샷 디렉터리가 비어 있다). 발견 1·3의 네트워크 시나리오는 데디케이티드 서버 + 원격 클라 구성에서만 재현되므로 실측하지 않았다. `WxAnimNotifyState_CameraMove`의 에디터 프리뷰 경로(`#if WITH_EDITOR`)는 훑기만 했다.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 153파일 — `/module-review`로 갱신*
