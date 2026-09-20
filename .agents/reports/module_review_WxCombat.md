# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 전반이 예외 경로·네트워크 권위·엔진 호출 규약까지 꼼꼼히 다뤄져 있고, 위험한 판단마다 근거 주석이 붙어 있어 전반적으로 건강하다. 크래시·권위 붕괴급 결함은 이번 범위에서 나오지 않았고, 남은 것은 구조 중복과 면제 범위·경계 규약 쪽이다. 이번 리뷰는 대미지 파이프라인(Wrapper GE → ExecCalc → Hit/DamageResponse 컴포넌트 → 어트리뷰트셋), ASC·입력 버퍼·어빌리티 베이스의 발동/캔슬 모델, 락온·모션워핑·컷신·소환물·투사체/무기를 cpp까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 Attack·Skill·Pattern 어빌리티가 콤보 구현을 3중으로 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-53`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19-46`
- **범주**: 중복/복잡도
- **문제**: 세 클래스가 `ComboMontages`·`ComboIndex` 멤버(각 헤더 29-34행대)와 `ActivateAbility`의 커밋→인덱스 전진→`PlayMontage` 흐름, `EndAbility`의 `bWasCancelled` 시 `ComboIndex = INDEX_NONE` 초기화를 글자 단위로 같게 들고 있다. Attack과 Skill은 쿨다운 GE 지정 외에 차이가 전혀 없고, Pattern만 전진 훅이 `HandleMontageCompleted` 대신 `HandleMontageBlendOut`이다. 콤보 인덱스 규칙(터미널 단 회귀, 취소 시 초기화)을 고칠 때 세 곳을 같이 고쳐야 하고, 한 곳만 놓치면 캐릭터 종류에 따라 콤보가 다르게 도는 버그가 된다.
- **제안**: `UWxAbilityBase`와 세 클래스 사이에 콤보 상태만 쥐는 중간 베이스(예: `UWxAbility_ComboBase`)를 두고, 인덱스 전진·초기화·몽타주 재생을 거기로 올린다. 세 서브클래스는 생성자의 태그·그룹·쿨다운 클래스와 "어느 몽타주 종료 훅에서 다음 단으로 가는가"만 남긴다.
- **확신도**: 높음

### 2. 🟡 "태그 하나를 Target/Asset 양쪽에 부여" 보일러플레이트가 7개 GE에 복사돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_IgnoreCooldowns.cpp:8-23`, `.../WxEffect_IgnoreCosts.cpp:8-23`, `.../WxEffect_IgnoreAggro.cpp:8-24`, `.../WxEffect_IgnoreAbilityTags.cpp:12-27`, `.../WxEffect_PerfectGuard.cpp:9-24`, `.../WxEffect_Exhaust.cpp:14-24`, `.../WxEffect_GuardReduction.cpp:14-24`, `.../WxEffect_SuperArmor.cpp:13-29`
- **범주**: 중복/복잡도
- **문제**: `CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>` → `FInheritedTagContainer.Added.AddTag` → `SetAndApplyTargetTagChanges` → `GEComponents.Add`를 AssetTags까지 두 벌씩, 8개 생성자에서 동일하게 반복한다. 실수하기 쉬운 지점(서브오브젝트 이름 충돌, `GEComponents.Add` 누락)이 8배로 늘어 있다. 같은 모듈 안에 이미 올바른 형태의 헬퍼가 있다 — `UWxEffect_Cooldown::GrantCooldownTag`(`.../WxEffect_Cooldown.cpp:27-34`).
- **제안**: `GrantCooldownTag`와 같은 방식의 보호 헬퍼(예: 공용 베이스 GE의 `GrantStateTag(const FGameplayTag&)`)를 하나 두고 각 GE 생성자가 한 줄로 부르게 바꾼다. Asset 태그가 필요 없는 GE를 위해 인자 하나로 갈라 두면 된다.
- **확신도**: 높음

### 3. 🟡 WxCombat이 자체 네이티브 GameplayTag 네임스페이스를 선언해 WxCore 단일 출처를 깬다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.h:10-13`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.cpp:7-10`
- **범주**: 설계/구조
- **문제**: `Effect.IgnoreAbilityTags`만 `WxCombatGameplayTags` 네임스페이스에서 선언·정의되는데, 의미가 나란한 형제 태그 `Effect.IgnoreAggro`/`Effect.IgnoreCosts`/`Effect.IgnoreCooldowns`는 전부 `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:63-65`에 있다. 그 결과 `WxAbilityBase.cpp`는 같은 파일 안에서 `WxCombatGameplayTags::Effect_IgnoreAbilityTags`(196행)와 `WxGameplayTags::Effect_IgnoreCooldowns`(340행)·`Effect_IgnoreCosts`(374행)를 섞어 쓴다. 모듈 README가 명시한 경계("이 모듈은 네이티브 태그를 직접 선언하지 않고 WxCore의 것만 소비한다")와도 어긋나고, 다른 도메인이 이 태그를 치트·디버그 경로에서 쓰려 하면 WxCombat을 참조해야 해서 플러그인 의존 규칙에 걸린다.
- **제안**: `Effect.IgnoreAbilityTags` 선언·정의를 `WxCore`의 `WxGameplayTags`로 옮기고 `WxCombatGameplayTags` 네임스페이스를 제거한다.
- **확신도**: 높음

### 4. 🟡 콤보 창 면제가 GE가 건 BlockAbilityTags까지 통째로 무력화한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:194-208` (특히 202-205)
- **범주**: 설계/구조
- **문제**: `DoesAbilitySatisfyTagRequirements`가 `IsActive() && ActionPhase == ComboWindow`일 때 `Super`를 아예 부르지 않고 true를 반환한다. 엔진의 해당 함수는 이 어빌리티가 선언한 `ActivationBlockedTags`/`ActivationRequiredTags`만 보는 것이 아니라 `AbilitySystemComponent.GetBlockedAbilityTags()`도 함께 검사한다(UE 5.8 `GameplayAbility.cpp`의 `CheckForBlocked(GetAssetTags(), ...GetBlockedAbilityTags())`). 그 통로는 `UWxEffect_Invincible`·`UWxEffect_SuperArmor`의 `UBlockAbilityTagsGameplayEffectComponent`와 `UWxAbility_Death::BlockAbilitiesWithTag`가 쓰는 바로 그 통로다. 즉 콤보 창이 열려 있는 동안에는 "효과가 막겠다고 선언한 어빌리티"라는 선언이 재발동에 한해 무시된다. 주석이 근거로 든 "피격·그로기·사망은 공격을 먼저 끊는다"는 취소가 반드시 성립할 때만 성립하는 전제이고, `CancelAbilitiesWithTag` 지목이 빠진 신규 차단 GE가 생기면 조용히 새는 구조다.
- **제안**: 면제 범위를 이 어빌리티가 선언한 조건으로 좁힌다 — `Super` 호출은 유지하되 실패 사유 태그(`OptionalRelevantTags`)가 `ActivateFailTagsBlocked`/`Missing` 중 자기 선언분인 경우에만 통과시키거나, 콤보 재발동 전용 경로에서 `ActivationRequiredTags`/`ActivationBlockedTags`만 직접 재평가한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟡 데미지 플로터가 피격 1건마다 액터 + UserWidget을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:32-39`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:42-53`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고 `InitWidget()`으로 위젯을 새로 생성하며, 수명은 `InitialLifeSpan = 5.f`다. 발행처(`.../Effect/WxEffectComponent_DamageResponse.cpp:30-37`)는 서버의 `ExecuteGameplayCue`이므로 전 클라이언트에 멀티캐스트되고, 화면 밖 전투의 피해까지 각 머신에서 액터+위젯을 만든다. 다단 히트 스킬이나 AOE 한 방이면 프레임 하나에 수십 개가 생기고 5초간 살아 있다. 위젯 생성은 GC·Slate 양쪽에 비싼 작업이다.
- **제안**: 플로터를 풀링하거나(액터+위젯 재사용), 최소한 수명을 연출 길이에 맞게 줄이고 화면 밖·원거리 피해는 큐 수신 측에서 생략한다.
- **확신도**: 중간

### 6. 🟢 오버랩에서 HitResult를 만드는 블록이 무기와 투사체에 그대로 복사돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:198-221`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:150-168`
- **범주**: 중복/복잡도
- **문제**: `bFromSweep`이면 SweepResult, 아니면 `GetClosestPointOnCollision`으로 ImpactPoint/Location을 채우고 실패 시 컴포넌트 위치로 떨어지는 로직이 두 파일에 동일하게 있다. 이 값은 `UWxAbilitySystemGlobals`가 큐 위치로 쓰므로 한쪽만 고치면 무기 히트와 투사체 히트의 임팩트 연출 기준이 갈린다.
- **제안**: `UWxCombatLibrary`에 `MakeOverlapHitResult(UPrimitiveComponent* Self, UPrimitiveComponent* Other, bool bFromSweep, const FHitResult& SweepResult)` 같은 정적 함수로 한 곳에 모은다.
- **확신도**: 높음

### 7. 🟢 투사체 오버랩이 같은 적대 판정을 두 번 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:127`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:136-137`
- **범주**: 중복/복잡도
- **문제**: 127행에서 `IsHostile(GetInstigator(), OtherActor)`가 아니면 이미 반환했는데, 136-137행의 `bEvaded` 계산이 `IsHostile(SourceASC->GetAvatarActor(), TargetASC->GetAvatarActor())`로 사실상 같은 쌍을 다시 판정한다. 두 판정이 갈릴 수 있는 것은 ASC의 Avatar가 Instigator/OtherActor와 다른 예외적 조립뿐이며, 그런 조립이라면 앞 게이트가 이미 잘못 걸러낸 상태다.
- **제안**: 136-137행의 `IsHostile` 항을 빼고 `bEvaded`를 `SourceASC && TargetASC && !Death && Invincible`로 줄인다.
- **확신도**: 중간

### 8. 🟢 AbilitySet 부여 경로에 권위 가드가 없고 외부 호출자에 의존한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-62`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:45-63`
- **범주**: 설계/구조
- **문제**: `GiveToAbilitySystem`은 `SetNumericAttributeBase`로 HP/SP/ATK 등을 직접 세우고 `GiveAbility`까지 부르는데, 권위 검사가 하나도 없다. 유일한 가드는 소비 모듈 쪽 `Source/WxGame/Character/WxCharacterBase.cpp:202-205`의 `if (HasAuthority())`다. 공개 API(`GiveAbilitySets`)라 다른 호출자가 생기면 클라이언트에서 어트리뷰트 베이스를 로컬로 덮어써 복제값과 싸우고, `GiveAbility`는 엔진 Error 로그를 남긴다. `bAbilitySetsGranted`가 이미 true로 서 버려 이후 서버 부여도 막힌다.
- **제안**: `UWxAbilitySystemComponent::GiveAbilitySets` 진입부에 `IsOwnerActorAuthoritative()` 조기 반환을 두고(플래그를 세우기 전), WxGame 쪽 가드는 그대로 둔다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_*.cpp` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/*.cpp` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingSorterTask_InputDirection.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingPreview.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_{Attack,Skill,Pattern,Sprint,Death,Passive,PlayMontageOnce,Guard,Finisher,Ultimate}.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_{SlowTime,WaitMoving,PlaySkillCutscene,RotateToTarget}.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, Public 헤더 전체
- **확인 결과(결함 아님으로 판정)**: `ShouldAbilityRespondToEvent`의 `Payload` 역참조는 엔진이 `TriggerEventData` non-null일 때만 호출하므로 안전. Instant GE에 대한 `FActiveGameplayEffectHandle::WasSuccessfullyApplied()`는 true를 돌려주므로 `WxEffectComponent_Hit`의 `bDamageApplied` 판정은 옳다. `UWorld::GetAudioTimeSeconds()`가 TimeDilation을 받지 않는다는 컷신 전제도 엔진 구현과 일치한다. `TargetingPreset->GetTargetingTaskSet()`은 멤버 주소라 null이 아니다. `AbilitySystemComponent`의 `ABILITYLIST_SCOPE_LOCK` 사용과 모션워핑 modifier 순회 중 상태 변경도 엔진 규약상 안전하다. CLAUDE.md 규칙 스캔(첫 줄 저작권, `FORCEINLINE`/헤더 인라인 정의, `BlueprintCallable` 오용, 델리게이트 콜백 `Handle` 접두사, WxCore 외 Wx 플러그인 참조)은 전부 위반 0건이다.
- **미검토 / 한계**: 멀티플레이 실측(예측 롤백·릴러번시 손실 중 락온/컷신 세션 전이)은 정적 읽기만으로 판단하지 않았다. `UWxAbility_Dodge`의 서버측 TargetData 대기가 끝내 도착하지 않는 경우 Exclusive 점유가 남는지, `UWxSkillCutsceneComponent`의 세션 중첩 경합은 타이밍 의존이라 결론을 내리지 못했다. BP/WBP 내부 구조와 DataTable 실제 값은 범위 밖이다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 201파일 — `/module-review`로 갱신*
