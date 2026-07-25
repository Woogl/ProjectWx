# WxCombat — 코드 리뷰

> GAS 위에 얹은 어빌리티/대미지 파이프라인의 골격은 견고하고, 태그 누수·콜백 레이스 같은 어려운 실패 경로를 주석과 실패복구 코드로 꼼꼼히 방어한 흔적이 많다. 다만 히트 판정 액터(무기·투사체)의 네트워크 권위 모델이 나머지 모듈과 어긋나 있고, ExecutionCalculation이 순수 계산을 넘어 ASC 상태를 직접 바꾸는 구조적 부담이 남아 있다. 이번 리뷰는 어빌리티 실행/판정, 대미지 파이프라인(`WxExecCalc_Damage`·`FWxDamageInfo`·`WxCombatAttributeSet`), 무기/투사체 히트, 락온·AbilityTask 수명주기를 cpp까지 내려가 보았고, GE/Cue/MMC·타게팅 필터·AnimNotify는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 9 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 투사체 히트 처리에 권위 게이트가 없어 클라이언트가 복제 액터를 로컬 파괴한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:95-148`
- **범주**: 설계/구조
- **문제**: `HandleHitCollisionOverlap`(126-137행)과 `HandleHitCollisionHit`(140-148행)이 `HasAuthority()` 검사 없이 GE를 적용하고 `Destroy()`를 호출한다. 투사체는 `bReplicates = true`(19행)이고 스폰은 `UWxAbilityBase::SpawnProjectile`(`WxAbilityBase.cpp:87`)에서 명시적으로 서버 권위로 게이팅돼 있으므로, 파괴만 클라에서 자유롭게 일어나는 비대칭이 생긴다. 클라의 로컬 위치가 서버와 조금만 어긋나도 클라 쪽 투사체가 먼저 무언가를 스쳐 사라지고, 서버 투사체는 계속 날아가 실제 타겟을 맞힌다 — 플레이어 화면에서 투사체가 증발하거나 명중 연출이 통째로 누락된다. GE 적용 쪽은 `ApplyGameplayEffectSpecToSelf`가 권위 없으면 내부에서 거부하므로 이중 대미지는 나지 않지만, 그만큼 클라의 히트 처리 전체가 "파괴만 하는" 반쪽 경로가 된다. 같은 파일 112행의 `OtherComp->GetClosestPointOnCollision(...)`도 `WxWeaponBase.cpp:228`의 동일 로직과 달리 `OtherComp` 널 검사가 빠져 있다.
- **제안**: 두 핸들러 진입부에 `HasAuthority()` 게이트를 두고, 파괴는 서버에서만 수행해 복제로 클라에 전파한다. 클라 전용 임팩트 연출이 필요하면 `Destroyed()`의 Niagara 스폰(63-71행)이 복제 파괴 시에도 그대로 돌므로 로컬 파괴는 불필요하다. `OtherComp` 널 검사는 무기 쪽과 동일하게 맞춘다.
- **확신도**: 높음

### 2. 🟡 근접 무기 히트 판정이 클라에서 매 틱 돌지만 결과가 전부 버려진다 (주석이 사실과 다름)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:246-261`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:33`
- **범주**: 성능/안전
- **문제**: `ProcessHit`의 주석(248-249행)은 "클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며, 불일치하면 GAS가 자동 롤백한다"고 선언하지만, 모듈 어디에도 `FScopedPredictionWindow`가 없고 `UWxCombatLibrary::ApplyDamage`는 `ApplyGameplayEffectSpecToTarget(Spec, Target)`을 예측 키 인자 없이(기본 `FPredictionKey()`) 호출한다. GAS의 `HasNetworkAuthorityToApplyGameplayEffect`는 권위도 예측 키도 없으면 적용을 조용히 거부하므로, 클라에서의 히트는 대미지·큐·히트리액트를 하나도 만들지 못한다. 대신 비용은 그대로 든다 — `Tick`(164-212행)이 화면 안 모든 캐릭터의 무기에 대해 공격 구간 내내 매 프레임 `SweepMultiByObjectType`을 돌리고, 매 히트마다 GE Spec을 만들어 버린다(`FWxDamageInfo::MakeSpecs`).
- **제안**: 예측을 실제로 도입하든(어빌리티 활성화 스코프 안에서 예측 키를 실어 보냄), 클라 판정을 포기하고 `BeginAttack`/`Tick`/`ProcessHit`를 `HasAuthority()`로 게이팅하든 하나로 정한다. 최소한 주석을 현재 동작에 맞게 고쳐야 다음 사람이 "롤백이 되고 있다"고 오해하지 않는다.
- **확신도**: 높음

### 3. 🟡 무기의 히트 대상 캐시가 GC 추적 밖에 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:98,101`
- **범주**: 버그/정확성
- **문제**: `TSet<TObjectPtr<AActor>> HitActorsThisSwing`과 `FWxDamageInfo DamageInfo`(내부에 `TArray<TSubclassOf<UGameplayEffect>>` 보유)가 `UPROPERTY`가 아니다. `TObjectPtr`은 UPROPERTY 컨테이너 안에서만 GC 참조로 수집되므로 여기서는 사실상 raw 포인터다. `Tick`의 191-197행이 `AlreadyHit.Get()`을 `FCollisionQueryParams::AddIgnoredActor`에 넘기는데, 이 함수는 인자를 역참조해 `GetUniqueID()`를 읽는다. 한 스윙 도중 피격 대상이 파괴되고 그 사이 GC가 돌면 해제된 메모리를 읽는다. 스윙이 짧아 실제 재현은 드물지만 방어 장치가 전혀 없는 구조다.
- **제안**: 두 멤버에 `UPROPERTY()`를 붙이거나, 히트 목록을 `TSet<TWeakObjectPtr<AActor>>`로 바꾸고 순회 시 유효성을 검사한다(`AWxEffectZone::AppliedTargets`가 이미 후자 방식이다 — `WxEffectZone.h:53`).
- **확신도**: 중간

### 4. 🟡 대미지 ExecutionCalculation이 계산이 아니라 상태 변경을 수행한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:88-102,133,210,330,351,375`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어를 채우는 것 외에 (a) 공격자 DP를 `SetNumericAttributeBase`로 직접 가산(210행), (b) `TargetASC->CancelAbilities`로 가드 어빌리티 취소(330행), (c) GameplayEvent 3종 송출(93·101·351행), (d) `ExecuteGameplayCue`(375행)를 GE 실행 도중에 수행한다. GAS는 ExecCalc를 부작용 없는 순수 계산으로 전제하며, 여기서 ASC를 건드리면 활성 GE 컨테이너를 순회·수정하는 중에 재진입이 발생한다. 실제로 330행의 `CancelAbilities`는 `UWxAbility_Guard::EndAbility`를 동기 호출해 `State.Guard` 루스 태그를 제거하는데, 같은 실행 스코프의 `ApplyHitReaction`은 그 직전에 읽은 `bIsGuarding` 스냅샷으로 분기를 정한 뒤다. 더 큰 결합은 순서 의존이다 — `UWxAbility_Guard::HandleGuardHitReact`의 주석(`WxAbility_Guard.cpp:231-233`)이 "ExecCalc는 SP 모디파이어를 큐잉한 직후 이벤트를 디스패치하므로 지금 `GetSP()`는 차감 전 값"이라는 사실에 가드 브레이크 판정을 걸고 있다. ExecCalc 내부 디스패치 순서를 바꾸는 순간 가드 브레이크가 조용히 깨진다.
- **제안**: 이벤트/큐/반사 대미지는 ExecCalc가 결과만 산출하고, 실제 발행은 GE 실행이 끝난 뒤(예: `UWxCombatAttributeSet::PostGameplayEffectExecute` 또는 전용 GE Component)로 미룬다. 최소한 SP 잔량 의존은 이벤트 페이로드에 "차감 후 예상 SP"를 명시적으로 실어 순서 의존을 없앤다.
- **확신도**: 중간(의도된 설계일 수 있음 — 다만 순서 의존이 주석으로만 방어되고 있다)

### 5. 🟡 서버가 클라이언트 TargetData를 타입 검증 없이 다운캐스트한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:404`
- **범주**: 성능/안전
- **문제**: `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`은 리모트 클라가 보낸 복제 TargetData를 실제 타입 확인 없이 그대로 캐스팅한다. 클라가 다른 `FGameplayAbilityTargetData` 파생 타입을 실으면 잘못된 오프셋에서 `Direction`을 읽는다(UB). `DataHandle`이 비어 `Get(0)`이 null을 반환하는 경우는 방어돼 있으나 타입 불일치는 방어되지 않는다.
- **제안**: `DataHandle.Get(0)->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()`를 확인한 뒤 캐스팅하고, 불일치 시 기본값(제로 벡터 = 백스텝)으로 폴백한다. 방향 벡터 자체도 서버에서 정규화·범위 검증하는 편이 안전하다.
- **확신도**: 높음

### 6. 🟡 `UWxAbility_Groggy::EndAbility`가 null일 수 있는 몽타주를 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:114`
- **범주**: 버그/정확성
- **문제**: `ASC->StopMontageIfCurrent(*GroggyMontage);`에 널 검사가 없다. 그런데 같은 클래스의 `ActivateAbility`(35-39행)가 `if (!GroggyMontage || !CommitAbility(...)) { EndAbility(...); return; }`로 **바로 이 오버라이드를 호출한다**. 즉 `GroggyMontage`가 미설정된 Groggy 어빌리티 BP는 활성화 즉시 null 역참조 경로를 탄다. `StopMontageIfCurrent`가 참조의 주소만 비교하는 구현이라 현재 엔진에서는 크래시로 드러나지 않지만 형식상 UB이며, 콜리 구현이 바뀌거나 체크 빌드에서는 그대로 터진다.
- **제안**: `if (GroggyMontage) { ASC->StopMontageIfCurrent(*GroggyMontage); }`로 감싼다.
- **확신도**: 높음

### 7. 🟡 컷신 태스크가 자기가 부여하지 않은 `State.Invincible`을 제거할 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:20-27,33-39,120-136`
- **범주**: 버그/정확성
- **문제**: `Activate`가 33-39행에서 `!World || !LevelSequence`로 조기 종료하면 `AddInvincibleTag`(42행)를 지나치지 못한 채 `EndTask()` → `OnDestroy` → `RemoveInvincibleTag`(22행)로 흐른다. `RemoveLooseGameplayTag`는 레퍼런스 카운트를 깎으므로, 같은 시점에 `ANS_Invincible`이 부여해 둔 무적 창을 대신 걷어낸다. 같은 파일의 타임 딜레이션은 `bTimeDilationActive` 플래그로 정확히 이 문제를 막고 있는데(140-144행), 무적 태그에만 같은 가드가 없다.
- **제안**: `bInvincibleTagAdded` 플래그를 두고 `AddInvincibleTag`가 실제로 부여했을 때만 세워, `RemoveInvincibleTag`가 그 경우에만 동작하게 한다.
- **확신도**: 높음

### 8. 🟡 글로벌 타임 딜레이션 경로가 둘로 갈라져 있고 어느 쪽도 중첩을 처리하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:28-33,39`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:51-53,138-151`
- **범주**: 설계/구조
- **문제**: `WxAbilityTask_SlowTime`은 서버 권위 + 복제 경로(`UWxTimeDilationComponent::SetGlobalTimeDilationAuthoritative`)를 쓰고, `WxAbilityTask_PlaySkillCutscene`은 같은 월드 전역 값을 `UGameplayStatics::SetGlobalTimeDilation`으로 로컬에서 직접 바꾼다. 둘 다 리프카운트가 없다. 구체적 실패: (a) 퍼펙트 가드 슬로우(`WxAbility_Guard.cpp:275`)나 극한 회피 슬로우(`WxAbility_Dodge.cpp:292`) 도중 궁극기 컷신이 시작되면 컷신이 `OriginalTimeDilation`으로 슬로우 값을 캡처해 컷신 종료 후 슬로우가 영구화된다. (b) 두 `SlowTime` 태스크가 겹치면 먼저 끝난 쪽의 `OnDestroy`가 무조건 `1.f`로 되돌려(30행) 나머지 슬로우를 조기 취소한다. (c) 컷신 경로는 복제되지 않아 멀티플레이에서 서버/클라 시간이 어긋난다.
- **제안**: 타임 딜레이션 요청을 `UWxTimeDilationComponent` 한 곳으로 모으고, 요청 핸들 스택(가장 강한 값 우선 또는 LIFO 복원)으로 중첩을 처리한다. 컷신 태스크도 이 컴포넌트를 경유하게 한다.
- **확신도**: 높음

### 9. 🟡 락온 복제 수신 경로가 `State.LockedOn` 태그 이관을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:56-60,62-95`
- **범주**: 설계/구조
- **문제**: 이전 대상에서 `State.LockedOn`을 걷고 새 대상에 붙이는 로직이 `ApplyLockOnTarget`(69-89행) 안에만 있다. 그런데 복제 경로에서는 리플리케이션이 `LockOnTarget` 필드를 직접 덮어쓴 뒤 `OnRep_LockOnTarget`이 브로드캐스트만 하므로(59행) 이 로직을 전혀 타지 않는다. 소유 클라가 A를 예측 락온한 뒤 서버 값이 B로 도착하거나 서버가 대상을 비우면, A에는 `State.LockedOn`이 남고 B에는 붙지 않는다. 이 태그는 로컬 표시용이므로 잘못된 적에게 락온 마커가 고착된다.
- **제안**: `OnRep_LockOnTarget`이 이전 값을 인자로 받도록 바꿔(`OnRep_LockOnTarget(USceneComponent* OldTarget)`) 태그 이관 로직을 재사용하거나, 태그 이관을 별도 헬퍼로 빼서 권위·복제 두 경로가 공유하게 한다.
- **확신도**: 중간

### 10. 🟡 콤보 재발동 경로가 `Super::CanActivateAbility`를 건너뛰어 활성화 검사 일부가 무시된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:32-47`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:31-42`
- **범주**: 규칙 위반
- **문제**: 두 어빌리티 모두 `Spec->IsActive()`인 재발동 분기에서 `Super::CanActivateAbility`를 호출하지 않고 검사 일부(`ActivationBlockedTags`, `CheckCooldown`, `CheckCost`)만 손으로 재구현한다. CLAUDE.md 코딩 규칙 5(override 시 `Super::` 호출) 위반이며, 실질적으로 `ActivationRequiredTags`, Source/Target 태그 요구사항, BP의 `K2_CanActivateAbility` 오버라이드, 아바타 유효성 검사가 콤보 진행 중에만 조용히 무시된다. 예컨대 `UWxAbility_Attack`을 상속한 BP가 `ActivationRequiredTags`(무기 장착 등)를 걸어도 콤보 2타부터는 적용되지 않는다.
- **제안**: 우회해야 하는 것은 자기 애셋 태그로 인한 차단(`AreAbilityTagsBlocked`)뿐이므로, 재발동 분기에서도 `Super::CanActivateAbility`를 호출하되 그 차단만 예외 처리하거나(호출 전 `UnBlockAbilitiesWithTags` 스코프 등), 최소한 `DoesAbilitySatisfyTagRequirements`를 명시적으로 함께 호출한다.
- **확신도**: 높음

### 11. 🟢 `UWxAbilityTask_WaitInputActionTriggered::Activate`가 `Super::Activate()`를 호출하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitInputActionTriggered.cpp:23-33`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 코딩 규칙 5 위반. 같은 폴더의 `WxAbilityTask_LockOnTarget`(137행), `WxAbilityTask_SlowTime`(37행), `WxAbilityTask_PlaySkillCutscene`(31행)은 모두 호출하고 있어 이 파일만 비일관적이다. `UGameplayTask::Activate`가 사실상 로깅만 하므로 현재 동작 영향은 없다.
- **제안**: 함수 첫 줄에 `Super::Activate();`를 추가한다.
- **확신도**: 높음

### 12. 🟢 `PrevCapsuleRotation`은 쓰기만 하고 읽지 않는 데드 멤버다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:105`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:76,211`
- **범주**: 중복/복잡도
- **문제**: 두 곳에서 대입만 되고 어디서도 읽히지 않는다. `Tick`의 스윕(203행)은 시작 위치만 `PrevCapsuleLocation`을 쓰고 회전은 현재 값(`CurrRotation`)을 넘긴다. 헤더 주석(103행)은 "직전 프레임 위치/회전을 Sweep 시작점으로 사용"이라고 적혀 있어 실제와 다르다.
- **제안**: 멤버와 대입을 제거하고 주석을 위치 전용으로 수정한다. 회전 보간이 원래 의도였다면 스윕에 반영한다.
- **확신도**: 높음

### 13. 🟢 태그 윈도우 AnimNotifyState 3종이 태그만 다른 동일 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp:8-32`, `.../WxAnimNotifyState_Invincible.cpp:8-32`, `.../WxAnimNotifyState_PerfectGuard.cpp:8-32`
- **범주**: 중복/복잡도
- **문제**: 세 파일이 `NotifyBegin`에서 루스 태그 추가, `NotifyEnd`에서 제거하는 완전히 동일한 구조이며 상수 태그 하나만 다르다(헤더 포함 6개 파일). 새 상태 창을 추가할 때마다 같은 코드가 늘어난다.
- **제안**: `FGameplayTag`를 프로퍼티로 가진 공용 베이스(예: `UWxAnimNotifyState_GameplayTagWindow`)를 두고, 기존 3종은 생성자에서 태그만 지정하는 얇은 파생으로 남긴다(디자이너 UX와 기존 몽타주 참조는 유지된다).
- **확신도**: 중간(의도된 설계일 수 있음 — 클래스 분리 자체가 디자이너용 이름표 역할을 한다)

### 14. 🟢 `UWxCombatLibrary::ApplyDamage`의 반환값이 실제 적용 성공을 반영하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:33-34`
- **범주**: 버그/정확성
- **문제**: `ApplyGameplayEffectSpecToTarget`의 반환 핸들을 버리고 무조건 `bAppliedAny = true`로 둔다. 권위가 없어 적용이 거부되거나(항목 2 참조) 대상이 GE를 튕겨내도 `true`가 나간다. 현재 호출부(`WxWeaponBase::ProcessHit`, `WxAbility_Finisher::ApplyFinisherDamage`)가 반환값을 무시하고 있어 드러나지 않을 뿐, BP Function Library로 노출된 공개 API라 오용되기 쉽다.
- **제안**: 반환된 `FActiveGameplayEffectHandle`의 유효성을 `bAppliedAny`에 반영한다.
- **확신도**: 높음

### 15. 🟢 널 가드·정리 순서가 파일마다 비일관적이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:154`, `.../Ability/WxAbility_Guard.cpp:117`, `.../Ability/WxAbility_Sprint.cpp:54-63`
- **범주**: 버그/정확성
- **문제**: (a) `SetLastPressedInputAction`이 `GetOwnerActor()->HasAuthority()`를 널 검사 없이 호출한다. (b) `UWxAbility_Guard::EndAbility`가 `ActorInfo->AbilitySystemComponent.Get()`을 바로 역참조하는데, 같은 시그니처의 `WxAbility_Dodge.cpp:112`·`WxAbility_LockOn.cpp:93`·`WxAbility_Groggy.cpp:92`는 모두 `if (ActorInfo)`로 감싼다. (c) `UWxAbility_Sprint::EndAbility`만 `Super::EndAbility`를 **먼저** 호출한 뒤 속도 GE를 제거해, 모듈의 다른 모든 어빌리티(정리 → Super)와 순서가 반대다. 부모가 태그 해제·옵저버 통지를 마친 뒤에도 스프린트 버프가 살아 있는 창이 생기고, `Super`가 `IsEndAbilityValid` 실패로 조기 리턴한 경우에도 버프만 제거된다.
- **제안**: 세 지점을 나머지 파일의 관례에 맞춘다(널 가드 추가, `Super::EndAbility`는 정리 후 마지막 호출).
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `.../AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../AbilitySystem/WxAbilitySystemComponent.cpp`, `.../AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Weapon/WxWeaponBase.cpp`, `.../Weapon/WxProjectileBase.cpp`, `.../AbilitySystem/Ability/WxAbility_Attack.cpp`, `.../WxAbility_Dodge.cpp`, `.../WxAbility_Guard.cpp`, `.../WxAbility_HitReact.cpp`, `.../WxAbility_Groggy.cpp`, `.../WxAbility_Finisher.cpp`, `.../WxAbility_LockOn.cpp`, `.../WxAbility_Skill.cpp`, `.../WxAbility_Sprint.cpp`, `.../WxAbility_Death.cpp`, `.../WxAbility_Ultimate.cpp`, `.../WxAbility_Pattern.cpp`, `.../AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `.../Task/WxAbilityTask_PlaySkillCutscene.cpp`, `.../Task/WxAbilityTask_SlowTime.cpp`, `.../Task/WxAbilityTask_WaitInputActionTriggered.cpp`, `.../Targeting/WxLockOnManagerComponent.cpp`, `.../Time/WxTimeDilationComponent.cpp`, `.../AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `.../AnimNotify/WxAnimNotify_AreaAttack.cpp` 및 대응 헤더(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `.../Public/Weapon/WxWeaponBase.h`, `.../Public/WxEffectZone.h`, `.../Public/AbilitySystem/Task/WxAbilityTask_LockOnTarget.h` 등)
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Burn.cpp`, `.../Effect/WxMMC_CooldownDuration.cpp`, `.../Effect/WxMMC_Cost.cpp`, `.../Effect/WxMMC_LinearDrain.cpp`, `.../Effect/WxEffect_RecoverResource.cpp`, `.../Cue/WxCueNotify_Damage.cpp`, `.../Cue/WxCueNotify_AttackTelegraph.cpp`, `.../Targeting/WxTargetingFilterTask_LineTrace.cpp`, `.../Targeting/WxTargetingFilterTask_ScreenBounds.cpp`, `.../Targeting/WxLockOnPointComponent.cpp`, `.../Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `.../AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `.../AnimNotify/WxAnimNotifyState_ComboWindow.cpp`, `.../AnimNotify/WxAnimNotifyState_Invincible.cpp`, `.../AnimNotify/WxAnimNotifyState_PerfectGuard.cpp`, `.../AnimNotify/WxAnimNotifyState_SnapToTarget.cpp`, `.../AnimNotify/WxAnimNotify_FinisherDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxEffectZone.cpp`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`
- **미검토 / 한계**:
  - 나머지 `AbilitySystem/Effect/WxEffect_*.cpp`(Burn/Cooldown/Cost/Damage/DrainDP/Exceed/FullHP/HealPercent/InfiniteMP/Kill/NoCooldown/RegenSP/ResetDP/Sprint)는 생성자에서 모디파이어만 선언하는 데이터 클래스로 판단해 개별 통독하지 않았다.
  - `WxTargetingFilterTask_Team/InputDirection/GameplayTag`, `WxCueNotify_Burn/Exceed/PerfectGuard`, `WxAnimNotify_StartRecovery/SpawnProjectile/SendGameplayEvent`는 열지 않았다.
  - 기계적 규칙 점검은 전수 스캔으로 확인했다 — 첫 줄 저작권(147/147 통과), `BlueprintCallable`(BP Function Library 1곳뿐), 델리게이트 콜백 `Handle` prefix(56곳 전부 준수), WxCore 외 Wx 플러그인 의존 없음, 바인딩 람다 없음. `Super::` 미호출도 스크립트로 전수 스캔했고, 순수 가상 구현(`Execute_Implementation`, `CalculateBaseMagnitude_Implementation`, `ShouldFilterTarget`, 모듈 Startup/Shutdown)을 제외한 실제 위반은 항목 10·11이 전부다.
  - 멀티플레이 관련 지적(항목 1·2·9)은 정적 분석 근거이며 실제 네트워크 세션에서 재현 검증하지 않았다.
  - BP/WBP 내부(디폴트값, 몽타주·데이터테이블 실제 설정)는 리뷰 범위 밖이므로 "몽타주 미설정" 같은 전제가 실제 에셋에서 발생하는지는 확인하지 않았다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 147파일 — `/module-review`로 갱신*
