# WxCombat — 코드 리뷰

> 여전히 건강한 모듈이다. 권위/예측 경계, 태그·GE 수명, 어트리뷰트 소비 순서 같은 GAS의 어려운 지점에 근거 주석이 남아 있고 널 가드도 일관되며, CLAUDE.md 코딩·모듈 규칙 위반은 전수 확인 결과 한 건도 없다. 치명적 결함은 이번에도 없고, 남은 것은 시뮬 프록시까지 도는 무기 판정 비용, "GE 정의 단위 전수 제거", 클라이언트 입력 무검증, 그리고 이번 커밋에서 콤보 게이트가 사라지며 생긴 데드 코드·낡은 계약 문서다. 이번 리뷰는 대미지 파이프라인(Library→TableRow→ExecCalc→AttributeSet)·어빌리티 베이스와 13개 구체 어빌리티·ASC·락온·무기/투사체·TimeDilation·AnimNotify·AbilityTask를 cpp까지 읽었고, Effect/Cue/Targeting 필터는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 7 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 무기 히트 판정과 대미지 파이프라인이 시뮬 프록시에서도 그대로 돈다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:152`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:233`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12`
- **범주**: 성능/안전
- **문제**: `UWxAnimNotifyState_WeaponAttack`은 권위·로컬 게이트가 없어 몽타주가 복제되는 모든 머신에서 `BeginAttack`을 부른다. 그 결과 `AWxWeaponBase::Tick`의 캡슐 Sweep이 화면 안 모든 공격 캐릭터마다 매 틱 돌고, 적중 시 `ProcessHit`→`UWxCombatLibrary::ApplyDamage`가 `MakeEffectContext`와 스펙 N개(`FWxDamageTableRow::MakeSpecs`)를 만들어 놓고 엔진의 권위 검사에서 버려진다. 같은 모듈의 투사체는 `AWxProjectileBase::HandleHitCollisionOverlap`에서 `HasAuthority()`로 대미지 경로를 잘라 내므로(`WxProjectileBase.cpp:83`) 두 대미지 소스의 게이트가 서로 다르다. 부수 효과로, 시뮬 프록시 스윕이 `EWxDamageResult::Evaded`를 내면 `WxCombatLibrary.cpp:47`이 로컬에서 `Event.DodgeSuccess`를 발행해 피격자 클라가 서버와 무관하게 퍼펙트 회피 몽타주를 재생할 수 있다.
- **제안**: ANS 또는 `ProcessHit` 진입부에서 `HasAuthority() || 로컬 조작 아바타`가 아닌 경우를 걸러 시뮬 프록시의 스윕·스펙 생성을 아예 막는다. 로컬 예측을 유지할 대상은 소유 클라뿐이다.
- **확신도**: 중간 (예측 응답성을 위해 클라 판정을 일부러 남긴 설계일 수 있으나, 시뮬 프록시분은 어느 쪽으로도 쓰이지 않는다)

### 2. 🟡 ActivationOwnedEffects 해제가 "그 GE 정의 전부"를 걷는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:97`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:146`
- **범주**: 버그/정확성
- **문제**: `RemoveActivationOwnedEffect`는 `FGameplayEffectQuery::EffectDefinition`으로 조회해 `RemoveActiveEffects(Query)`를 부른다. 이 쿼리는 CDO 일치만 보므로 **누가 걸었는지와 무관하게** 해당 정의의 활성 GE를 전부 제거한다. `UWxEffect_Invincible`은 세 곳이 같은 CDO로 적용한다 — `UWxAbility_Finisher`의 `ActivationOwnedEffects`(`WxAbility_Finisher.cpp:31`), `UWxAnimNotifyState_Invincible`의 지속시간 GE(`WxAnimNotifyState_Invincible.cpp:33`), `UWxAbilityTask_PlaySkillCutscene`(`WxAbilityTask_PlaySkillCutscene.cpp:96`). 회피 i-frame(ANS가 건 지속시간 GE, 회피가 끊겨도 스스로 만료되도록 설계됨 — `WxAbility_Dodge.cpp:94-97`) 도중 Finisher가 Reaction으로 끼어들었다가 끝나면, Finisher의 EndAbility가 그 i-frame까지 함께 벗긴다. 코드도 이 문제를 인지하고 있다(`Public/AbilitySystem/Ability/WxAbilityBase.h:83`의 TODO).
- **제안**: `ApplyGameplayEffectToOwner`가 돌려주는 핸들을 인스턴스별로 모아두고 그 핸들만 제거하거나(예측 롤백 문제는 `RemoveActiveGameplayEffect` 실패를 허용하면 된다), 쿼리에 `EffectSource`/인스티게이터 조건을 추가해 자기 것만 걷도록 좁힌다.
- **확신도**: 중간

### 3. 🟡 클라이언트가 보낸 TargetData를 타입 검사 없이 다운캐스트한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:285`
- **범주**: 성능/안전
- **문제**: `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`로 네트워크 수신 데이터를 무검증 변환한다. `CallServerSetReplicatedTargetData` 경로는 등록된 어떤 `FGameplayAbilityTargetData` 파생 타입도 실어 보낼 수 있으므로, 변조 클라이언트가 다른 타입을 보내면 서버가 남의 레이아웃에서 `Direction`을 읽어 쓰레기 벡터로 몽타주 섹션과 캐릭터 회전(`WxAbility_Dodge.cpp:173`의 `AddActorWorldRotation`)을 결정한다.
- **제안**: `DataHandle.Get(0)->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()` 확인 후 캐스트하고, 불일치면 영벡터로 폴백한다.
- **확신도**: 높음

### 4. 🟡 락온 대상 Server RPC가 클라이언트 지정값을 무검증 수용한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:39`
- **범주**: 설계/구조
- **문제**: `ServerSetLockOnTarget_Implementation`은 받은 `USceneComponent*`를 그대로 `ApplyLockOnTarget`에 넘긴다. 거리(`MaxDistance`)·팀·`UWxLockOnPointComponent::CanBeLockedOn` 판정은 모두 클라이언트 측 `UWxAbility_LockOn`에만 있다. 그런데 이 복제값은 서버 권위 소비처가 읽는다 — `AWxProjectileBase::BeginPlay`의 호밍 타겟 지정(`WxProjectileBase.cpp:58-71`)과 `UWxRootMotionModifier_SnapToTarget`의 스냅 타겟(`WxRootMotionModifier_SnapToTarget.cpp:31-37`). 즉 클라이언트가 서버가 스폰한 투사체의 추적 대상을 임의로 지정할 수 있다. 헤더가 표방하는 "서버 권위"는 복제 권위일 뿐 값 검증 권위는 아니다.
- **제안**: `ServerSetLockOnTarget_Implementation`에서 최소한 `UWxLockOnPointComponent`인지·`CanBeLockedOn()`인지·소유 액터가 사거리 안인지를 재검사하고, 실패 시 이전 값을 유지한다.
- **확신도**: 높음 (사실 관계는 확실하나, PvE 코옵만 상정한 의도된 신뢰 모델일 수 있음)

### 5. 🟡 홀드 입력이 매 프레임 어빌리티 전수 스캔과 쿨다운 전수 조회를 유발한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:38`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:370`
- **범주**: 성능/안전
- **문제**: `AbilityInputActionTriggered`는 `ETriggerEvent::Triggered`에 물려 있어(`Source/WxGame/Character/WxPlayerCharacter.cpp:112`) 홀드형 입력(가드·질주)이 눌린 동안 **매 프레임** 호출된다. 한 프레임마다 (a) `GetActivatableAbilities()` 전수 순회 + 스펙마다 `Cast<UWxAbilityBase>`, (b) 미활성 매칭 어빌리티에 `TryActivateAbility` → 프로젝트 `CheckCooldown` → `QueryActiveCooldowns` → `ASC.GetActiveEffects(Query)`가 활성 GE 전수 스캔 + `TArray` 힙 할당, (c) 활성 어빌리티에는 `Spec.GetAbilityInstances()`(값 반환 = 할당) + 인스턴스마다 `InvokeReplicatedEvent`가 반복된다. 전투 중 활성 GE가 많은 캐릭터일수록 비용이 커진다.
- **제안**: 매칭 InputAction → Spec 핸들 맵을 한 번 만들어 캐시하고(AbilitySet 부여 시점), 발동 시도는 상태 전이(Started/새 프레임 진입) 기준으로 좁힌다. `QueryActiveCooldowns`는 `GetActiveEffects`가 아니라 캐시나 콜백 기반으로 대체하는 것을 검토한다.
- **확신도**: 높음

### 6. 🟡 ComboWindow 게이트가 삭제되면서 ANS·태그가 소비자 없는 데드 코드가 됐고, 헤더는 없는 계약을 설명한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp:16`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Attack.h:12`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Skill.h:51`
- **범주**: 중복/복잡도
- **문제**: 이번 커밋 범위에서 `UWxAbility_Attack`·`UWxAbility_Skill`의 `CanActivateAbility` 오버라이드(`State.ComboWindow` 안에서만 콤보 재발동 허용)가 제거됐고, 지금 콤보 진행은 `UWxAbilityBase::CanActivateAbility`의 배타 그룹 판정 — 즉 `StartRecovery`가 `Exclusive_Recovery`로 전이한 뒤에만 재발동이 통과하는 경로 — 하나로 일원화됐다. 그 결과 `State.ComboWindow` 태그를 읽는 코드가 저장소 전체에 하나도 없다. `UWxAnimNotifyState_ComboWindow`는 태그를 붙였다 떼기만 하고, 몽타주에 배치된 노티파이는 이제 아무 효과가 없다. 문서도 어긋난다: `WxAbility_Attack.h:12`와 `WxAbility_Skill.h:51,56`은 여전히 "State.ComboWindow 구간의 재발동이 다음 단으로 넘긴다"고 적고, `WxAbility_HitReact.cpp:21`은 "공격·스킬의 콤보 재발동 분기는 활성 Spec만 보고 자체 판정한다"는 사라진 분기를 근거로 든다.
- **제안**: 콤보 창을 후딜(StartRecovery)로 일원화한 것이 확정이면 ANS·`State.ComboWindow` 태그와 배치된 노티파이를 함께 지우고, 세 헤더/주석을 실제 규칙으로 고쳐 쓴다. 창을 되살릴 계획이면 소비 지점을 다시 붙인다.
- **확신도**: 높음 (태그 소비자 부재는 전수 검색으로 확인)

### 7. 🟡 락온 종료가 `bOrientRotationToMovement`를 저장값 복원이 아니라 `true`로 하드코딩한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:48`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:104`
- **범주**: 설계/구조
- **문제**: `ActivateAbility`가 `false`로 끄고 `EndAbility`가 무조건 `true`로 되돌린다. 이 플래그는 이 모듈 밖에서도 토글된다(`Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:244`, `:254`). 지금은 `UWxCharacterMovementComponent` 기본값이 `true`(`Source/WxGame/Character/WxCharacterMovementComponent.cpp:20`)라 우연히 맞지만, 스트레이프 캐릭터를 추가하거나 다른 소유자가 이 플래그를 끈 상태에서 락온이 끝나면 상태가 뒤집힌다.
- **제안**: 활성화 시점의 값을 기억해 종료 시 그 값으로 되돌리거나, 회전 정책을 플래그 직접 조작이 아닌 이동 컴포넌트의 상태 API로 옮긴다.
- **확신도**: 중간

### 8. 🟢 `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`이 데드 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:70`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 `RemoveFromAbilitySystem` 호출부가 없다. `AbilitySetGrantedHandles`는 `GiveAbilitySet()`에서 채워지기만 하고 소비되지 않아, 부여 취소 경로가 실제로는 존재하지 않는다.
- **제안**: 회수 시나리오(장비 교체·Experience 전환)가 계획에 없다면 핸들 수집과 함수를 함께 지우고, 있다면 호출부를 붙인다.
- **확신도**: 높음

### 9. 🟢 무적/퍼펙트가드 ANS가 통째로 중복이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Invincible.cpp:21`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_PerfectGuard.cpp:21`
- **범주**: 중복/복잡도
- **문제**: 두 파일의 `NotifyBegin`은 PlayRate 보정 계산부터 `ApplyTo(ASC, TotalDuration / PlayRate, ASC->GetAnimatingAbility())` 호출까지 GE 클래스만 다르고 완전히 같다. 보정 로직을 한쪽만 고치면 조용히 어긋난다.
- **제안**: 적용할 GE 클래스를 `TSubclassOf`로 들고 있는 공통 베이스 ANS 하나로 합치거나, PlayRate 보정만 공용 헬퍼로 뺀다.
- **확신도**: 높음

### 10. 🟢 히트스톱 복원 배속이 어빌리티의 PlayRate 오버라이드를 무시한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:221`
- **범주**: 버그/정확성
- **문제**: `HandleHitStopElapsed`는 항상 ASC의 ASPD 기반 `GetMontagePlayRate()`로 복원한다. 그러나 `UWxAbilityBase::GetMontagePlayRate()`는 "몽타주 길이 자체가 규칙인 어빌리티는 1을 반환하도록 오버라이드한다"는 계약이고, 실제로 Dodge·Guard·HitReact·Finisher·Death가 `1.f`로 오버라이드한다. 지금은 히트스톱을 발동시키는 어빌리티(Attack/Skill/Pattern) 중 오버라이드하는 것이 없어 잠재 상태지만, PlayRate를 1로 고정한 공격 어빌리티를 추가하면 히트스톱 후 그 몽타주가 ASPD 배속으로 튄다.
- **제안**: 얼릴 때의 원래 PlayRate를 타이머 델리게이트에 함께 실어 그 값으로 복원한다.
- **확신도**: 중간

### 11. 🟢 Attack·Skill·Pattern의 콤보 진행 코드가 3중 복제다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:17`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:15`
- **범주**: 중복/복잡도
- **문제**: 세 어빌리티가 `ComboMontages` 배열 + `ComboIndex` + 동일한 진행 로직을 각자 들고 있고, Attack과 Skill의 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted`는 문자 그대로 같다(Pattern만 완주 시 다음 단으로 이어 간다). 이미 드리프트 흔적이 보인다 — `ComboMontages`의 에디터 카테고리가 Attack은 `"Wx"`, Skill은 `"Wx|Ability"`, Pattern은 `"Wx"`로 갈렸고(`WxAbility_Attack.h:33`, `WxAbility_Skill.h:75`, `WxAbility_Pattern.h:29`) 같은 뜻의 주석도 한쪽에만 남았다.
- **제안**: 인플레이스 선호에 맞춘다면 최소한 카테고리·주석을 맞춰 드리프트를 되돌리고, 이후에도 세 곳이 같이 움직여야 한다면 배열·인덱스·진행 함수만 중간 베이스로 올린다.
- **확신도**: 중간 (약간의 반복을 용인하는 것이 프로젝트 성향이라 구조 추출은 판단 대상)

### 12. 🟢 삭제된 `FWxDamageInfo`를 가리키는 선언·주석이 남았다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h:10`, `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxCombatEffectContext.h:29`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Burn.cpp:78`
- **범주**: 중복/복잡도
- **문제**: `FWxDamageInfo`는 `FWxDamageTableRow`로 통합되며 사라졌는데, `WxCombatLibrary.h`에 전방 선언이 남아 있고(정의 없는 타입이라 다른 곳에서 참조하면 링크 단계까지 가서야 드러난다) 주석 두 곳이 `FWxDamageInfo::MakeSpecs`·`FWxDamageInfo의 AdditionalEffects`로 없는 타입을 가리킨다.
- **제안**: 전방 선언을 지우고 주석의 타입명을 `FWxDamageTableRow`로 고친다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/*.cpp`(Cooldown·Cost·Invincible·Guard·PerfectGuard·Burn·Exhaust·AddDP·DrainDP·DrainSP·RecoverResource 등), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_{SlowTime,WaitMoving,PlaySkillCutscene}.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_{Death,Ultimate}.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`
- **확인했고 문제 없던 항목**: CLAUDE.md 규칙 위반은 없다 — `WxCore` 외 Wx 플러그인 참조 없음(`.uplugin`·`Build.cs`·소스 include 전수 확인), `Wx` prefix 전수 일치(비-Wx 선언은 전부 엔진 타입 전방 선언), `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 한 곳뿐(BP Function Library), `FORCEINLINE`/인라인 정의 0건, 람다 0건, 델리게이트 바인딩 26건 전부 `Handle` prefix, 저작권 첫 줄 전 파일 존재(일부 파일은 UTF-8 BOM이 앞에 붙어 있으나 문구 자체는 정상). `UWxAbility_Death::HandleMontageCompleted`가 `Super::`를 부르지 않는 것은 사망 몽타주 종료 후에도 어빌리티를 살려 `Ability.Death`를 유지하려는 의도로 보여 위반으로 세지 않았다.
- **미검토 / 한계**: 데이터 자산(어빌리티/대미지 DataTable 행 값, AbilitySet 에셋, GE·Cue 블루프린트 파생 클래스, InputAction의 트리거 구성)의 실제 설정값은 보지 않았다. 6번의 "콤보가 후딜 창에서만 이어진다"는 결론은 코드 경로 추적에 근거한 것으로, 실제 콤보 손맛이 의도대로인지는 인게임 검증이 필요하다. 멀티플레이 실측(예측 롤백, 1번의 시뮬 프록시 스윕 실비용)은 정적 분석만 했다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 152파일 — `/module-review`로 갱신*
