# WxCombat — 코드 리뷰

> 프로젝트 규칙 준수는 전수 검사 기준 위반 0건이고(Copyright 첫 줄·`Wx` prefix·`Handle` 콜백·람다/인라인 금지·`BlueprintCallable` 제한·`WxCore` 외 Wx 의존 없음), GAS 위에 얹은 구조와 주석 밀도도 모듈 전반에서 높다. 남은 위험은 어트리뷰트 base 클램프 누락과 GE·어빌리티 사이의 정의 기준 일괄 조작 몇 군데에 몰려 있다. 이번 리뷰는 어빌리티 파이프라인(`WxAbilityBase` + 구체 어빌리티 13종), 대미지 판정(`WxExecCalc_Damage`·`WxExecCalc_Burn`·`FWxCombatEffectContext`·`WxCombatAttributeSet`), 무기·투사체 히트 경로, 락온/타게팅, AnimNotify·GE·MMC·AbilityTask 전량을 cpp까지 읽었고, Cue 연출과 데이터 Row 헤더는 훑는 수준으로 봤다. 판단이 갈리는 지점은 UE 5.8 GameplayAbilities 엔진 소스를 직접 확인해 근거를 달았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 (1번은 오탐으로 정정) |
| 🟡 개선 | 6 (2번은 조치 완료) |
| 🟢 사소 | 5 (8번은 조치 완료) |

## 결과

### 1. ✅ (정정) 캔슬로 끝난 공격·스킬이 몽타주를 놓아준다 — 오탐, 2026-08-20 조치 완료
- **판정**: 오탐. 원래 이 항목은 취소된 공격의 몽타주가 고아로 남아 히트 판정이 계속 돈다고 봤고, 그로기와 원격 회피(서버)를 근거 시나리오로 들었다. 둘 다 재현되지 않는다.
- **근거**: `UGameplayAbility::CancelAbility`는 `EndAbility`보다 **먼저** `OnGameplayAbilityCancelled`를 브로드캐스트하고, `UAbilityTask_PlayMontageAndWait`가 그 델리게이트에서 `StopPlayingMontage()`로 몽타주를 멈춘다(UE 5.8 `GameplayAbility.cpp`, `AbilityTask_PlayMontageAndWait.cpp`). 그로기의 `CancelAbilitiesWithTag`도, 회피가 공격을 미는 것도 모두 `CancelAbility` 경로라 `EndAbility`에 도달할 때는 이미 몽타주가 멈춰 있다.
- **조치**: 제안(`if (!bWasCancelled)`로 감싸기)은 재현되지 않는 위험에 대한 방어 코드가 되므로 채택하지 않고, `KeepMontagePlayingAfterEnd()` 자체를 제거해 몽타주 수명을 엔진 순정(=그것을 튼 어빌리티의 수명)으로 되돌렸다. 그 함수가 대신하던 두 가지(콤보 단 전환, 구 태스크 콜백 차단)는 엔진 종료 경로가 이미 처리한다. 상세는 `.claude/worklog/2026-08-20-몽타주-수명-엔진-순정화.md`.

### 2. ✅ DP만 base 값 클램프가 없어 MaxDP를 넘어 쌓이고, 그로기가 안전 타이머로만 끝난다 — 2026-08-20 조치 완료
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:153-217`
- **범주**: 버그/정확성
- **문제**: `PreAttributeChange`의 DP 클램프(`:83-90`)는 CurrentValue만 자른다 — 엔진 `FGameplayAttribute::SetNumericValueChecked`가 CurrentValue를 쓰기 직전에만 이 훅을 부르고, base는 `PreAttributeBaseChange`(미오버라이드)를 지나 그대로 들어간다. HP·MP·SP·UP는 `PostGameplayEffectExecute`에서 `Set*`(base 기록)로 한 번 더 클램프해 base가 범위 안에 남지만(`:192-216`), DP만 그 분기가 없다. 유입 경로는 두 곳이다 — ExecCalc의 DP 가산(`WxEffect_Damage.cpp:144`)과 퍼펙트 가드 반사(`WxEffect_Damage.cpp:206`). 그로기 직전 DP 95에 대미지 30이 들어오면 base는 125, 표시값만 100이 된다. `UWxMMC_DrainDP`는 지속시간 동안 정확히 MaxDP만큼만 빼도록 계산하므로(`WxEffect_DrainDP.cpp:39-55`) 드레인이 끝나도 base가 25 남아 DP가 0에 닿지 않고, 종료 판정인 `HandleDPChanged`가 발화하지 않는다. 결국 매번 `GroggySafetyTimerHandle`(`WxAbility_Groggy.cpp:76`)의 `GroggyDuration + 1초` 폴백으로 끝나며, 헤더가 "실패복구"로 적어 둔 경로(`WxAbility_Groggy.h:41`)가 상시 경로가 된다. 부수적으로 그로기 앞부분에서 DP 게이지가 만렙에 붙어 움직이지 않는다.
- **조치**: 제안대로 두 유입 경로를 각각 막았다. `PostGameplayEffectExecute`에 다른 vital과 같은 형태의 DP 분기를 넣어 ExecCalc의 DP 가산이 base를 범위 안에 남기게 하고, GE 모디파이어를 거치지 않아 그 되밀기가 닿지 않는 `ReflectPerfectGuard`는 기록 전에 MaxDP로 잘랐다. 그 함수의 주석이 상한까지 훅이 맡는다고 적혀 있던 것이 이번 버그의 씨앗이라 함께 정정했다. 덧붙여 반사 경로에는 대상 쪽 `!bIsGroggy`에 해당하는 공격자 게이트가 없어, 클램프 이후엔 그로기 중인 공격자에게 반사가 들어가면 같은 증상이 재발한다. 대칭을 맞춰 그로기 중이면 반사를 생략한다(연출·패링 이벤트는 그대로). 안전 타이머는 이제 예외 경로로 돌아가므로 폴백으로 남겼다. 상세는 `.claude/worklog/2026-08-20-DP-base-클램프.md`.
- **미검증**: 그로기가 몽타주 종료와 함께 끝나는지, 퍼펙트 가드 반사로 들어간 그로기도 같은지는 다음 PIE 세션에서 확인한다.

### 3. 🟡 `RemoveActivationOwnedEffect`가 정의 기준 전량 제거라 다른 출처의 같은 GE까지 벗긴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:77-90`, 호출부 `:112-116`
- **범주**: 설계/구조
- **문제**: 쿼리가 `Query.EffectDefinition = EffectClass` 하나뿐이라 그 클래스의 활성 GE를 소유자에게서 전부 걷는다. 예측 핸들이 무효해지는 문제를 피하려는 선택은 타당하지만, 같은 GE 클래스를 다른 주체가 걸어 둔 경우까지 구분 없이 지운다. 실제 겹치는 조합이 있다 — `UWxEffect_Invincible`은 `UWxAbility_Finisher`의 `ActivationOwnedEffects`이면서(`WxAbility_Finisher.cpp:35`) `UWxAnimNotifyState_Invincible`이 몽타주 구간에도 건다(`WxAnimNotifyState_Invincible.cpp:33`). 회피 i-frame 도중 처형이 발동해 회피를 취소하면(둘 다 `Trait.Exclusive`), 처형 종료가 아직 살아 있어야 할 ANS 무적 GE까지 함께 벗긴다. `WxAbilityBase.cpp:99` 주석대로 회피의 무적 GE는 스스로 만료되도록 설계돼 있으므로 이 제거는 설계 의도에서 벗어난다.
- **제안**: `QueryActiveCooldowns`(`:339-375`)가 쓰는 것과 같은 방식으로 소스를 함께 좁힌다 — 쿼리에 `CustomMatchDelegate`를 달아 `Spec.GetEffectContext().GetAbility()`가 이 어빌리티 CDO인 것만 지운다. ANS가 거는 GE는 컨텍스트에 어빌리티가 없으므로(`WxEffect_Invincible::ApplyTo`가 `MakeEffectContext`만 쓴다) 자연히 분리된다.
- **확신도**: 중간

### 4. 🟡 클라이언트가 보낸 TargetData를 타입 검증 없이 `static_cast` 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:290`
- **범주**: 성능/안전
- **문제**: `HandleTargetDataReceived`는 서버가 `CallServerSetReplicatedTargetData`로 받은 데이터를 처리하는 지점인데, `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`로 무검증 다운캐스트한 뒤 `Direction`을 읽는다. 핸들의 NetSerialize는 아카이브에 실린 ScriptStruct로 타입을 정하므로, 조작된 클라이언트가 더 작은 `FGameplayAbilityTargetData` 파생 타입(베이스 자체 포함)을 실어 보내면 구조체 경계 밖을 읽는다. 같은 검증 패턴이 이미 `WxEffect_Damage.cpp:82`와 `WxAbilitySystemComponent.cpp:178`에 있다.
- **제안**: `DataHandle.Get(0)`을 베이스 포인터로 받아 `GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()`를 확인한 뒤 캐스트한다.
- **확신도**: 높음

### 5. 🟡 락온이 `bOrientRotationToMovement`를 저장 없이 하드코딩 값으로 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:46-49`, `:101-104`
- **범주**: 설계/구조
- **문제**: 활성화에서 `false`로 끄고 종료에서 원래 값과 무관하게 `true`로 쓴다. 주석은 "복구"라고 적었지만 실제로는 고정값 대입이라, 기본값이 `false`인 아바타(스트레이프형 폰 등)는 락온을 한 번 쓰면 영구히 `true`가 되고, 같은 플래그를 소유한 다른 시스템과 겹치면 락온 종료가 그쪽 상태까지 덮는다.
- **제안**: 활성화 시점 값을 어빌리티 인스턴스에 담아 두고 종료에서 그 값으로 되돌린다. 소유권 충돌이 예상되면 CMC 플래그 직접 조작 대신 회전 모드를 상태 신호로 표현하는 쪽이 낫다.
- **확신도**: 중간

### 6. 🟡 ExecCalc가 계산 결과 대신 외부 상태를 직접 바꾼다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:120`, `:152`, `:310`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어 외에 세 가지 부수효과를 낸다. ① `ReflectPerfectGuard`가 공격자 ASC에 `SetNumericAttributeBase`로 DP를 가산하고, ② `UWxEffect_RecoverResource::ApplyTo(SourceASC, ...)`가 공격자에게 GE를 통째로 적용하며, ③ `ResolveHitReaction`이 대상 ASC의 `CancelAbilities`를 호출한다. 엔진의 `RemoveActiveEffects`가 자체 스코프 락을 걸므로(UE 5.8 `GameplayEffect.cpp:5695-5698`) 컨테이너 파손 위험은 낮지만, 두 가지가 남는다 — 이 부수효과들은 GE 적용이 거부되거나 예측 롤백돼도 되돌아가지 않고, ①은 어트리뷰트 훅을 타고 `PostAttributeChange`의 `HandleGameplayEvent(Event.Groggy)`까지 이어져 공격자의 그로기 발동(어빌리티 취소·GE 적용·타이머 등록 포함)을 대상 GE 실행 콜스택 안에서 유발한다.
- **제안**: 최소한 ③은 ExecCalc 밖으로 옮긴다. 이미 `FWxCombatEffectContext`에 판정을 실어 `HandleGameplayEffectAppliedToSelf`에서 연출을 발행하는 통로가 있으므로 "가드 해제"도 결과 플래그로 넘겨 그 훅에서 수행하면 되고, ①②를 같이 옮기면 순서 문제도 함께 사라진다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 예측/권위 트레이드오프를 주석이 인지하고 있다)

### 7. 🟡 Attack과 Skill의 콤보 제어 로직이 통째로 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:24-88` ↔ `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:27-87`
- **범주**: 중복/복잡도
- **문제**: `CanActivateAbility`의 콤보 재발동 분기, `EndAbility`(`bWasCancelled` 시 `MontageSelector.Reset()`), `HandleMontageCompleted`가 주석까지 포함해 사실상 동일하다. 한쪽만 고치면 다른 쪽이 남는 구조라, 1번 조치도 두 파일에 나란히 적용해야 했다.
- **제안**: 콤보 규약만 담은 공통 중간 베이스를 두고 Attack은 차이나는 부분(`HasActiveCancelTarget` 분기)만 남긴다.
- **확신도**: 높음

### 8. ✅ 널 가드가 파일마다 들쭉날쭉하다 — 2026-08-20 조치 완료
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp:12`, `:25`; `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:16`, `:32`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:48`, `:103`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:70`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:156`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:193`, `:345`
- **범주**: 성능/안전
- **문제**: 같은 폴더의 다른 노티파이가 전부 `MeshComp`를 검사하는데 ComboWindow와 WeaponAttack만 곧장 `MeshComp->GetOwner()`를 부른다. 락온·히트리액트는 `GetCharacterMovement()`를, `ApplyHitStop`과 `QueryActiveCooldowns`는 `GetWorld()`를 검사 없이 역참조한다. `WxAbilityBase.cpp:193`의 `ActorInfo->AbilitySystemComponent->TryActivateAbility`도 약참조를 그대로 `operator->` 한다(현재 호출부는 안전하지만 계약이 명시돼 있지 않다). 크래시를 관측한 곳은 없고 전부 실무상 유효할 가능성이 높지만, 모듈 내 다른 코드가 지키는 규칙과 어긋나 읽는 쪽이 어느 쪽이 의도인지 판단할 수 없다.
- **조치**: 판단 기준을 하나로 세우고 어긋난 6곳만 이웃 코드와 같은 형태로 맞췄다. 출처가 호출자에 달린 값 — 외부에서 넘어오는 함수 인자(`MeshComp`), 캐스트 결과에서 다시 꺼내는 하위 객체(`GetCharacterMovement()`), 약참조(`ActorInfo->AbilitySystemComponent`) — 은 가드하고, 유효 확인을 마친 액터·등록된 컴포넌트의 `GetWorld()`는 가드하지 않는다.
- **가드하지 않기로 한 곳**: 그 기준에서 `ApplyHitStop`(`WxAbilitySystemComponent.cpp:156`)과 `QueryActiveCooldowns`(`WxAbilityBase.cpp:344`)의 `GetWorld()` 무가드는 누락이 아니라 의도다. 전자는 바로 위에서 `GetAnimatingAbility()`·`GetCurrentMontage()`를 통과한 시점이고 후자는 활성 GE를 들고 있는 등록된 ASC라, 둘 다 호출 시점에 살아있는 월드가 보장된다. 같은 이유로 `WxAbilityBase.cpp:74`의 `Avatar->GetWorld()`도 유지했다. 상세는 `.claude/worklog/2026-08-20-WxCombat-널가드-일관화.md`.

### 9. 🟢 회피 판정 캡슐이 아바타를 못 찾으면 콜리전을 켠 채 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:275-285`
- **범주**: 버그/정확성
- **문제**: `DeactivateJudgementCapsule`이 `!JudgementCapsule || !Character`에서 함께 조기 반환한다. 캡슐은 무장 시 아바타에서 분리되므로(`:272`), 종료 시점에 아바타 캐스팅이 실패하면 월드에 QueryOnly `ECC_Pawn` 볼륨이 그 자리에 남아 공격 쿼리에 계속 잡힌다.
- **제안**: `SetCollisionEnabled(NoCollision)`을 캐릭터 널 검사 앞으로 옮기고, 재부착만 캐릭터 유효 시 수행한다.
- **확신도**: 중간

### 10. 🟢 복붙 수준의 중복 블록이 세 군데 남아 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Invincible.cpp:21-33` ↔ `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_PerfectGuard.cpp:21-33`; `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:212-232` ↔ `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:112-133`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_AttackTelegraph.cpp` ↔ `WxCueNotify_Burn.cpp` ↔ `WxCueNotify_Exceed.cpp`
- **범주**: 중복/복잡도
- **문제**: ① 몽타주 실효 재생속도로 GE 지속시간을 보정하는 10줄이 두 ANS에 주석까지 동일하다. ② 스윕이 아닌 Overlap에서 `GetClosestPointOnCollision`으로 `FHitResult`를 합성하는 18줄이 무기와 투사체에 동일하다. ③ 액터 Cue 3종의 `OnActive`/`OnRemove`(Niagara 부착 스폰 → 제거 시 Deactivate)가 부착 대상만 다르고 나머지가 같다. 어느 것도 동작 결함은 아니지만 셋 다 한쪽만 고치면 다른 쪽이 어긋나는 형태다.
- **제안**: ①은 `WxEffect_*::ApplyTo` 쪽으로 재생속도 인자를 넘기는 방식으로, ②는 `UWxCombatLibrary`의 공용 헬퍼 하나로 모은다. ③은 구조 변경 부담 대비 이득이 작으니 그대로 두거나 공통 베이스 Cue 액터 하나로만 정리한다.
- **확신도**: 높음

### 11. 🟢 `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`이 호출자 없는 데드 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h:22`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-34`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체에서 호출자가 없다. `UWxAbilitySystemComponent`가 `AbilitySetGrantedHandles`를 멤버로 들고 있으나(`WxAbilitySystemComponent.h:77`) 회수 시점이 정의돼 있지 않아, 부여 취소 경로가 있는 것처럼 보이지만 실제로는 없다.
- **제안**: AbilitySet 교체·해제 요구가 아직 없다면 함수와 멤버를 지운다. 나중에 필요해지면 그때 추가하는 편이 프로젝트의 "호출자 없는 방어적 선언 금지" 방향과 맞는다.
- **확신도**: 높음

### 12. 🟢 쿨다운 조회가 입력을 쥐고 있는 동안 매 프레임 힙 할당을 낸다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:339-375`
- **범주**: 성능/안전
- **문제**: `AbilityInputActionTriggered`는 홀드 트리거에서 매 프레임 들어오고(`WxAbilitySystemComponent.h:22-29`) 매번 `TryActivateAbility` → `CheckCooldown` → `QueryActiveCooldowns`로 이어진다. 이 함수는 `ASC.GetActiveEffects(Query)`로 `TArray`를 값 반환받아(`:351`) 프레임마다 할당하고, 반환된 핸들마다 `GetActiveGameplayEffect`로 활성 목록을 다시 선형 탐색한다(`:353`). UI 뷰모델도 같은 API를 부른다. 현재 어빌리티 수에서는 무시할 수준이지만 핫패스에 있는 패턴이다.
- **제안**: 활성 GE 목록을 한 번만 순회해 개수와 최장 잔여를 동시에 구하거나(핸들 배열을 거치지 않는 형태), 그대로 두더라도 이 함수가 프레임 호출 대상이라는 점을 주석으로 남긴다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Burn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainDP.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp`(10종 전량), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/*.cpp`(GE·MMC 전량), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/*.cpp`(6종), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitMoving.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`(5종), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전량(스캔), `Source/WxGame/Character/WxCharacterBase.cpp`·`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`(호출부 확인용)
- **미검토 / 한계**:
  - 규칙 위반은 `.claude/CLAUDE.md`의 명시 항목만 기계적으로 전수 검사했고 **위반 0건**이라 별도 항목을 싣지 않았다(Copyright 첫 줄·`Wx` prefix·`FORCEINLINE`/인라인 정의·람다·`BlueprintCallable`·`Handle` 콜백 prefix·`WxCore` 외 Wx 의존). `WxCombat.Build.cs`의 Wx 의존은 `WxCore` 하나뿐이고, 소스 include에 나오는 Wx 외부 헤더도 `WxGameplayTags.h`/`WxCollisionChannels.h`(둘 다 WxCore)뿐이다.
  - 이전 리뷰(2026-08-15, `e9440f73`)가 🟡로 올렸던 "`OnGranted` 자동 발동이 클라이언트에서도 중복 실행된다"는 항목은 이번에 취소했다. UE 5.8에서 `UGameplayAbility::OnGiveAbility`는 `UAbilitySystemComponent::GiveAbility`(권위 전용, `AbilitySystemComponent_Abilities.cpp:301-325`)에서만 호출되고 `FGameplayAbilitySpec::PostReplicatedAdd`는 이를 부르지 않는다 — 클라 중복 발동은 일어나지 않는다. 남은 널 역참조 우려만 8번으로 옮겼다.
  - 같은 리뷰의 "ExecCalc 재진입이 실행 중인 GE 컨테이너를 파손한다"는 근거도 약화했다. `RemoveActiveEffects`가 자체 `GAMEPLAYEFFECT_SCOPE_LOCK`을 잡는 것을 엔진 소스로 확인했으므로 6번은 롤백·순서 문제로 범위를 좁혔다.
  - 2번의 그로기 지연은 정적 분석과 엔진 소스 대조로 도출했고 PIE로 실측하지는 않았다. 대미지가 정확히 MaxDP에 떨어지는 경우에는 증상이 나타나지 않는다.
  - 1번의 "서버에서 AnimNotify가 실제로 발화하는가"는 프로젝트의 `VisibilityBasedAnimTickOption` 설정에 좌우된다. 코드 주석(`WxWeaponBase.cpp:237` "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다")을 근거로 발화한다고 보고 판단했으며 실측하지는 않았다.
  - `FWxAbilityTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`의 밸런스 의미와 DataTable 콘텐츠, BP 서브클래스가 지정하는 값(어빌리티 슬롯 태그, 몽타주 세트, `WxProjectile` 콜리전 프로파일, `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록 여부)은 범위 밖이다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 153파일 — `/module-review`로 갱신*
