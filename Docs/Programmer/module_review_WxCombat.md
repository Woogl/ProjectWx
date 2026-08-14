# WxCombat — 코드 리뷰

> 규칙 준수도와 주석 품질은 모듈 전체에서 매우 높고(Copyright·`Wx` prefix·`Handle` 콜백·람다/인라인 금지 위반 0건), GAS 위에 얹은 구조도 대체로 일관적이다. 다만 어빌리티 종료·몽타주 수명 관리와 ExecCalc의 부수효과에서 재진입·권위 관련 위험이 몇 군데 남아 있다. 이번 리뷰는 어빌리티 파이프라인(`WxAbilityBase` + 구체 어빌리티 13종), 대미지 판정(`WxExecCalc_Damage`/`WxExecCalc_Burn`/`FWxCombatEffectContext`), 무기·투사체 히트 경로, 락온/타게팅, AnimNotify, GE·MMC 전량을 cpp까지 읽었고, Cue 연출과 데이터 Row 헤더는 훑는 수준으로 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 캔슬 종료에서도 몽타주를 놓아줘, 취소된 어빌리티의 판정이 계속 돈다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:70`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:69`
- **범주**: 버그/정확성
- **문제**: `EndAbility`가 `bWasCancelled`와 무관하게 `KeepMontagePlayingAfterEnd()`를 먼저 부른다. 이 함수는 `MontageTask->EndTask()`로 태스크를 엔진의 ActiveTasks에서 떼어내므로(`WxAbilityBase.cpp:159-168`), 이후 `UGameplayAbility::EndAbility`가 `TaskOwnerEnded()`로 몽타주를 멈추는 경로가 사라진다. 콤보 재발동(`bWasCancelled=false`)에는 의도된 동작이지만, 진짜 취소에서는 몽타주가 고아로 남아 계속 재생된다. 취소자가 같은 슬롯 몽타주를 곧바로 틀면 가려지지만, 그렇지 않은 경로가 실제로 존재한다.
  - **그로기**: `UWxAbility_Groggy`는 `Ability.Exclusive`를 취소한 뒤 `HandleMontagePollTick`을 도는데, 이 폴러는 `ASC->GetCurrentMontage() != nullptr`이면 아무것도 하지 않는다(`WxAbility_Groggy.cpp:188-191`). 고아가 된 공격 몽타주가 남아 있으므로 그로기 몽타주는 그 공격이 끝날 때까지 시작되지 않고, 그동안 `WxAnimNotifyState_WeaponAttack`이 살아 있어 그로기 상태의 캐릭터가 스윙을 마저 끝내며 대미지를 넣는다.
  - **원격 플레이어의 회피(서버)**: `UWxAbility_Dodge`의 서버 인스턴스는 방향 TargetData가 도착할 때까지 몽타주를 재생하지 않는다(`WxAbility_Dodge.cpp:75-90`). 그 왕복 지연 동안 서버에서는 취소된 공격 몽타주가 계속 돌아 무기 히트 판정이 유지된다.
- **제안**: `KeepMontagePlayingAfterEnd()`를 `if (!bWasCancelled)`로 감싸 콤보 재발동 경로에서만 호출한다. Attack/Skill 둘 다 동일하게 고친다(아래 7번의 공통화와 함께 처리하면 한 곳으로 끝난다).
- **확신도**: 중간

### 2. 🟡 ExecCalc가 제3의 ASC를 직접 변경·GE 적용·어빌리티 취소해 GE 실행 중 재진입한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:120`, `:152`, `:206`, `:310`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어만 내는 대신 실행 중에 외부 상태를 직접 바꾼다. ① `ReflectPerfectGuard`가 공격자 ASC에 `SetNumericAttributeBase`로 DP를 가산하고(`:206`), ② `UWxEffect_RecoverResource::ApplyTo(SourceASC, ...)`가 공격자에게 GE를 통째로 적용하며(`:152`), ③ `ResolveHitReaction`이 대상 ASC의 `CancelAbilities`를 호출한다(`:310`). 특히 ③은 취소된 Guard 어빌리티의 `EndAbility` → `RemoveActivationOwnedEffect` → `ASC->RemoveActiveEffects`로 이어져, 지금 실행 중인 그 ActiveGameplayEffects 컨테이너를 같은 콜스택에서 변형한다. ①도 어트리뷰트 변경 훅을 타고 `PostAttributeChange`의 `HandleGameplayEvent(Event.Groggy)`까지 이어져 공격자의 그로기 어빌리티 활성화를 GE 실행 도중에 유발할 수 있다. 관측된 크래시는 아니지만 GAS가 ExecCalc를 부수효과 없는 계산으로 가정하는 지점을 넷 군데에서 벗어나 있고, 이 부수효과들은 GE 적용이 거부·롤백돼도 되돌아가지 않는다.
- **제안**: 최소한 어빌리티 취소(③)는 ExecCalc 밖으로 옮긴다 — 이미 `FWxCombatEffectContext`에 판정 결과를 실어 `HandleGameplayEffectAppliedToSelf`에서 연출을 발행하는 통로가 있으므로, "가드 해제"도 결과 플래그로 넘겨 그 훅에서 수행하면 된다. ①②도 같은 훅으로 이관하면 GE 적용 완료 후로 미뤄져 재진입이 사라진다.
- **확신도**: 중간(의도된 설계일 수 있음 — 주석상 예측/권위 트레이드오프를 인지하고 있다)

### 3. 🟡 클라이언트가 보낸 TargetData를 타입 검증 없이 `static_cast` 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:290`
- **범주**: 성능/안전
- **문제**: `HandleTargetDataReceived`는 서버가 `CallServerSetReplicatedTargetData`로 받은 데이터를 처리하는 지점인데, `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`로 무검증 다운캐스트한 뒤 `Direction`을 읽는다. 클라이언트가 다른(더 작은) `FGameplayAbilityTargetData` 파생 타입을 실어 보내면 구조체 경계 밖을 읽게 되고, 그 값이 그대로 회피 방향·섹션 선택에 쓰인다. 네트워크 입력을 신뢰하는 형태다.
- **제안**: `const FGameplayAbilityTargetData* Data = DataHandle.Get(0);` 후 `Data && Data->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()` 를 확인하고 캐스트한다(같은 패턴이 `WxEffect_Damage.cpp:82`, `WxAbilitySystemComponent.cpp:170`에는 이미 적용돼 있다).
- **확신도**: 높음

### 4. 🟡 `OnGranted` 자동 발동이 아바타·예측 상태를 검사하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:189-197`
- **범주**: 버그/정확성
- **문제**: `ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)` 한 줄뿐이라 세 가지가 빠져 있다. ① `ActorInfo`와 약참조 `AbilitySystemComponent`를 검사하지 않고 `operator->`로 역참조한다. ② `OnGiveAbility`는 `InitAbilityActorInfo`로 AvatarActor가 확정되기 전에도 불릴 수 있어, 아바타 없는 상태로 패시브가 발동/실패할 수 있다. ③ 어빌리티 스펙은 클라이언트에도 복제되고 그때 `PostReplicatedAdd`가 `OnGiveAbility`를 다시 부르므로, 기본 `LocalPredicted` 정책의 패시브가 클라에서도 발동을 시도해 서버 발동과 겹친다.
- **제안**: `AvatarActor` 유효성과 `!Spec.IsActive()`를 확인하고, 예측 중복을 피하려면 권위 또는 로컬 실행 여부로 게이팅한다(Lyra의 `TryActivateAbilityOnSpawn`가 같은 문제를 그렇게 다룬다). 발동 시점을 `OnAvatarSet`으로 옮기는 것도 대안이다.
- **확신도**: 중간

### 5. 🟡 락온이 `bOrientRotationToMovement`를 저장 없이 하드코딩 값으로 복원한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:46-49`, `:101-104`
- **범주**: 설계/구조
- **문제**: 활성화에서 `false`로 끄고 종료에서 무조건 `true`로 되돌린다. 원래 값을 읽지 않으므로 ⓐ 기본값이 `false`인 아바타(스트레이프형 AI·특수 폰)는 락온을 한 번 쓰면 영구히 `true`가 되고, ⓑ 같은 플래그를 만지는 다른 어빌리티(턴어라운드 등)와 겹치면 락온 종료가 그쪽 상태까지 덮는다. 더불어 두 곳 모두 `GetCharacterMovement()`의 널 검사가 없다.
- **제안**: 활성화 시점의 값을 어빌리티 인스턴스에 담아 두고 종료에서 그 값으로 되돌리거나, 소유권 충돌이 예상되면 CMC 플래그 직접 조작 대신 별도 상태 태그/GE로 표현한다.
- **확신도**: 중간

### 6. 🟡 Attack과 Skill의 콤보 제어 로직이 통째로 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:24-88` ↔ `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:27-87`
- **범주**: 중복/복잡도
- **문제**: `CanActivateAbility`의 콤보 재발동 분기, `EndAbility`(`KeepMontagePlayingAfterEnd` + `bWasCancelled` 시 `MontageSelector.Reset()`), `HandleMontageCompleted`가 주석까지 포함해 사실상 동일하다. 실제로 1번 결함이 두 파일에 똑같이 복제돼 있어, 한쪽만 고치면 다른 쪽이 남는다.
- **제안**: 콤보 규약을 공통 중간 베이스(예: `UWxAbility_Combo`)로 올리고 Attack/Skill은 차이나는 부분(Attack의 `HasActiveCancelTarget` 분기)만 남긴다.
- **확신도**: 높음

### 7. 🟡 무적/퍼펙트가드 ANS와 GE의 `ApplyTo`가 서로 복붙 수준으로 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Invincible.cpp:14-33` ↔ `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_PerfectGuard.cpp:14-33`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp:27-48` ↔ `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_PerfectGuard.cpp:27-48`
- **범주**: 중복/복잡도
- **문제**: ANS 쪽은 "몽타주 EffectivePlayRate로 TotalDuration 보정" 블록이 주석까지 동일하고, GE 쪽 `ApplyTo`는 GE 클래스만 다른 완전 동일 구현이다. `SetDuration(..., true)`나 예측 키 규약이 바뀌면 네 곳을 같이 고쳐야 한다.
- **제안**: `ApplyTo`는 `TSubclassOf<UGameplayEffect>`를 받는 공용 헬퍼 하나로 모으고, ANS는 태그(또는 GE 클래스)를 프로퍼티로 받는 단일 ANS로 합치거나 재생속도 보정 계산만 공용 함수로 뽑는다.
- **확신도**: 높음

### 8. 🟢 `WxAnimNotifyState_WeaponAttack`만 `MeshComp` 널 검사가 빠져 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:16`, `:32`
- **범주**: 버그/정확성
- **문제**: 모듈의 다른 노티파이는 전부 `MeshComp ? MeshComp->GetOwner() : nullptr` 형태로 방어하는데(`WxAnimNotifyState_CameraMove.cpp:22`, `WxAnimNotify_StartRecovery.cpp:12` 등) 여기만 `MeshComp->GetOwner()`를 바로 부른다. 엔진이 널을 넘기는 일은 드물지만 방어 수준이 파일마다 갈리는 것 자체가 함정이다.
- **제안**: 다른 노티파이와 같은 가드로 통일한다.
- **확신도**: 높음

### 9. 🟢 `FWxCombatEffectContext::NetSerialize`가 실패를 삼킨다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp:26`, `:34`, `:42`
- **범주**: 버그/정확성
- **문제**: 베이스 `FGameplayEffectContext::NetSerialize`의 반환값과 `HitReactTag.NetSerialize`가 채운 `bOutSuccess`를 모두 무시하고 마지막에 `bOutSuccess = true`로 덮는다. 컨텍스트 안의 오브젝트 참조가 아직 매핑되지 않은 상황(액터 복제 순서 문제 등)에서 실패가 성공으로 보고돼, 재시도 없이 반쯤 채워진 컨텍스트로 Cue가 재생될 수 있다.
- **제안**: 베이스 반환값과 각 단계의 `bOutSuccess`를 누적해 최종값으로 돌려준다.
- **확신도**: 높음

### 10. 🟢 무기와 투사체의 오버랩 히트 결과 산출 코드가 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:212-230` ↔ `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:113-131`
- **범주**: 중복/복잡도
- **문제**: `bFromSweep` 분기 → `GetClosestPointOnCollision` → 실패 시 `OtherComp->GetComponentLocation()` 폴백까지 18줄이 동일하다. 임팩트 위치는 Cue 위치(`UWxAbilitySystemGlobals::InitGameplayCueParameters`)로 그대로 이어지므로 두 경로가 갈라지면 연출 위치가 어긋난다.
- **제안**: `UWxCombatLibrary`에 `MakeHitResultFromOverlap(...)` 같은 공용 함수로 뽑는다.
- **확신도**: 높음

### 11. 🟢 대미지 플로터가 히트마다 액터+위젯을 새로 스폰하고 5초를 산다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`, `:52`
- **범주**: 성능/안전
- **문제**: Cue 실행마다 `AWxDamageFloaterActor`를 스폰하고 `InitWidget()`으로 UMG 위젯을 새로 만든다. `InitialLifeSpan = 5.f`라 다단 히트 콤보와 화상 틱(0.5초마다, `WxExecCalc_Burn::BurnPeriod`)이 겹치는 구간에서는 수십 개가 동시에 살아 있게 된다. 풀링도 상한도 없다.
- **제안**: 플로터 액터/위젯을 풀링하거나, 위젯 하나가 여러 수치를 표시하는 방식으로 바꾼다. 최소한 lifespan을 연출 길이에 맞춰 줄인다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Burn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp`(10종 전량), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`(5종), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/*.cpp`(GE·MMC 전량), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/*.cpp`(6종), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitMoving.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전량(스캔), `Source/WxGame/Character/WxCharacterBase.cpp`(호출부 확인용)
- **미검토 / 한계**:
  - 규칙 위반은 `.claude/CLAUDE.md`의 명시 항목만 기계적으로 전수 검사했고(Copyright 첫 줄·`Wx` prefix·`FORCEINLINE`/인라인 정의·람다·`BlueprintCallable`·`Handle` 콜백 prefix·`WxCore` 외 Wx 의존) **위반 0건**이라 별도 항목을 싣지 않았다. `WxCombat.Build.cs`의 Wx 의존은 `WxCore` 하나뿐이고, 소스 include도 `WxGameplayTags.h`/`WxCollisionChannels.h`(둘 다 WxCore)만 나온다.
  - 1번 결함의 "서버에서 AnimNotify가 실제로 발화하는지"는 프로젝트의 `VisibilityBasedAnimTickOption` 설정에 좌우된다 — 코드 주석("클라와 서버가 같은 히트 판정과 GE 적용을 수행한다", `WxWeaponBase.cpp:237`)을 근거로 발화한다고 보고 판단했으며, 실측하지는 않았다.
  - 2번의 재진입은 정적 분석 결과이고 실제 크래시/오동작을 재현해 확인하지는 않았다. 엔진의 `FActiveGameplayEffectsContainer` 스코프 락이 어디까지 막아 주는지는 UE 5.8 소스로 직접 확인하지 않았다.
  - `FWxAbilityTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`의 필드 구성과 밸런스 의미, DataTable 콘텐츠 자체는 보지 않았다.
  - BP 서브클래스가 지정하는 값(어빌리티 애셋 태그 슬롯, 몽타주 세트, 콜리전 프로파일 `WxProjectile`, `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록 여부)은 범위 밖이라 검증하지 않았다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 152파일 — `/module-review`로 갱신*
