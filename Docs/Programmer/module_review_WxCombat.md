# WxCombat — 코드 리뷰

> GAS 파운데이션·대미지 파이프라인·어빌리티군 모두 권위/예측 경계와 실행 순서를 주석으로 명시해 둔, 전반적으로 건강한 모듈이다. 심각한 결함은 발견되지 않았고 남은 것은 네트워크 입력 검증, 팀 판정 위치, 콤보 어빌리티 중복 정도다. 이번 리뷰는 대미지 파이프라인(Library→DamageInfo→ExecCalc→AttributeSet)·ASC/AbilityBase·전 어빌리티·무기/투사체·락온·TimeDilation을 cpp까지 읽었고, GE 정의 상수와 Cue 연출 코드는 훑는 수준으로 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 클라이언트가 보낸 TargetData를 타입 확인 없이 다운캐스트한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:287`
- **범주**: 성능/안전
- **문제**: 서버 인스턴스가 `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`로 곧장 캐스팅한다. `FGameplayAbilityTargetDataHandle::NetSerialize`는 클라가 지정한 ScriptStruct로 요소를 복원하므로, 조작된 클라이언트가 다른 `FGameplayAbilityTargetData` 파생 타입을 보내면 서버가 남의 레이아웃을 `FVector`로 읽는다(정의되지 않은 동작). 실제 피해는 잘못된 회피 섹션 선택 수준이지만, 검증 없는 네트워크 입력 캐스트라는 점 자체가 문제다.
- **제안**: `Data->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()` 확인 후 캐스팅하고, 불일치 시 `LocalDirection`을 ZeroVector로 두고 진행한다.
- **확신도**: 높음

### 2. 🟡 락온 대상 Server RPC가 아무 검증 없이 클라 값을 권위로 승격한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:39`
- **범주**: 설계/구조
- **문제**: `ServerSetLockOnTarget_Implementation`이 클라가 보낸 `USceneComponent*`를 그대로 `ApplyLockOnTarget`에 넘긴다. 거리·팀·`UWxLockOnPointComponent::CanBeLockedOn` 중 무엇도 서버에서 재확인하지 않는다. 이 값은 서버에서 투사체 호밍(`WxProjectileBase.cpp:59`)과 몽타주 스냅(`WxRootMotionModifier_SnapToTarget.cpp:31`)의 입력이 되므로, "서버 권위 복제"라는 설계 의도(README·헤더 주석)가 실질적으로는 클라 신뢰가 된다. 스냅 위치 이동은 `bTargetInSnapRange`(TargetingPreset 결과)로 한 번 더 걸러져 피해가 한정되지만, 그 방어는 우연에 가깝다.
- **제안**: `_Implementation`에서 `Cast<UWxLockOnPointComponent>` + `CanBeLockedOn()` + 소유 액터와의 거리(`MaxDistance`) 재검증 후 통과한 것만 반영한다. 실패 시 현재 값을 유지해 클라를 정합시킨다.
- **확신도**: 중간(싱글/협동 전용이면 의도된 단순화일 수 있음)

### 3. 🟡 `UWxAbility_Attack`과 `UWxAbility_Skill`이 콤보 상태 기계를 통째로 중복한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:23-78` / `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:22-81`, 헤더는 `WxAbility_Attack.h:38-43` / `WxAbility_Skill.h:43-47`
- **범주**: 중복/복잡도
- **문제**: 두 클래스의 `CanActivateAbility` 콤보 분기, `ActivateAbility`, `EndAbility`, `HandleMontageCompleted`, `ComboMontages`/`ComboIndex` 멤버가 문장 단위로 동일하다(주석 문구와 조기 반환 스타일만 다름). 콤보 규칙을 바꿀 때 두 곳을 같이 고쳐야 하고, 이미 `CanActivateAbility`의 판정 순서가 한쪽은 `&&` 체인, 다른 쪽은 다단 `if`로 갈라져 있어 미세한 드리프트가 시작됐다.
- **제안**: 콤보 로직을 담은 중간 베이스(예: `UWxAbility_ComboBase : UWxAbilityBase`)를 두고 `Attack`/`Skill`은 애셋 태그·`ActivationOwnedTags`만 생성자에서 다르게 선언한다.
- **확신도**: 높음

### 4. 🟡 투사체가 팀 판정 전에 소멸·연출까지 끝낸다 — 아군이 투사체를 흡수한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:75-114`
- **범주**: 버그/정확성
- **문제**: `HandleHitCollisionOverlap`은 소유자·인스티게이터만 제외하고 나머지 모든 Pawn에 대해 `PlayImpactFX()` → `Destroy()`를 수행한다("WxProjectile" 프로파일은 Pawn 전체에 Overlap, `Config/DefaultEngine.ini:40`). 팀 판정은 훨씬 아래 `UWxExecCalc_Damage::CheckDamage`에만 있어서, 아군·중립 Pawn을 스쳐도 투사체가 임팩트 이펙트를 터뜨리며 사라진다. 이는 `WxEffect_Damage.cpp:172` 주석이 못 박은 "아군·중립은 대미지도 연출도 발생하지 않는다"와 정면으로 어긋난다. `AWxWeaponBase::ProcessHit`(`WxWeaponBase.cpp:240`)도 같은 이유로 아군을 `HitActorsThisSwing`에 소모시켜 이후 그 스윙에서 무시한다.
- **제안**: 소멸/연출 판정 앞에 `CheckDamage(SourceASC, TargetASC) != EWxDamageResult::None`을 두거나(대미지 경로와 동일한 선판정 재사용), 팀 필터를 콜리전 프로파일 단계로 올린다.
- **확신도**: 중간(투사체를 몸으로 막는 것이 의도된 규칙일 수 있음)

### 5. 🟡 무기 히트 판정이 시뮬 프록시에서도 매 틱 스윕하고 GE Spec까지 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:154-201`, `235-248`
- **범주**: 성능/안전
- **문제**: `WxAnimNotifyState_WeaponAttack`은 복제된 몽타주를 따라 모든 머신에서 실행되고, 무기 액터도 복제된다. 그 결과 남의 캐릭터 공격을 구경만 하는 클라이언트에서도 `SweepMultiByObjectType`이 공격 구간 내내 매 틱 돌고, 히트가 잡히면 `ApplyDamage`가 `MakeEffectContext`(힙 할당)와 `MakeSpecs`(추가 이펙트마다 `FGameplayEffectSpec`)까지 만든 뒤 GAS가 예측 키 부재로 폐기한다. 시뮬 프록시에는 `GetAnimatingAbility()`가 없어 애초에 적용될 수 없는 작업이다. 오픈월드에서 동시 전투 액터가 늘어날수록 그대로 비례한다.
- **제안**: `ProcessHit` 진입부(또는 `BeginAttack`의 틱 활성화)에서 `GetOwner()->GetLocalRole() == ROLE_SimulatedProxy`이면 판정을 돌리지 않는다. 예측이 필요한 것은 권위 머신과 소유 클라뿐이다.
- **확신도**: 중간(시뮬 프록시의 로컬 연출을 위해 일부러 남긴 것일 수 있음)

### 6. 🟢 그로기 안전 타이머가 스스로 어빌리티를 끝내지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:164-172`
- **범주**: 버그/정확성
- **문제**: `HandleGroggySafetyTimeout`은 `UWxEffect_ResetDP` 적용만 하고 종료는 그로 인한 DP 변화 → `HandleDPChanged`에 위임한다. 최후 보루여야 할 경로가 다시 GE 적용 성공에 의존하므로, `MakeOutgoingGameplayEffectSpec`이 무효를 반환하거나 적용이 막히면(면역·핸들 만료 등) 타이머는 원샷이라 다시 오지 않고 그로기가 영구화된다.
- **제안**: ResetDP 적용 후 DP가 여전히 0보다 크면 `EndAbility`를 직접 호출하도록 폴백을 둔다.
- **확신도**: 중간

### 7. 🟢 히트스톱 복원 타이머 핸들이 하나뿐이라 앞선 몽타주가 얼어붙을 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:144-149`, `216-223`
- **범주**: 버그/정확성
- **문제**: `HitStopResumeTimer` 하나를 재사용하므로, 첫 히트로 몽타주 A를 0.001배로 얼린 뒤 복원 전에 어빌리티가 교체되어 몽타주 B에 두 번째 히트스톱이 걸리면 `SetTimer`가 핸들을 덮어써 A의 복원 델리게이트가 사라진다. A가 이미 정지·블렌드아웃 중이면 눈에 띄지 않지만, 재생 속도가 남아 있는 상태로 A가 살아 있을 경우 복원 주체가 없다. 주석(`:146`)은 "연속 적중이면 타이머가 재설정된다"고만 하고 몽타주가 바뀌는 경우를 다루지 않는다.
- **제안**: 새 히트스톱을 걸기 전에 기존 타이머가 가리키던 몽타주를 즉시 복원하거나, 얼린 몽타주별로 핸들을 들고 관리한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 쿨다운 경로의 널 가드가 형제 오버라이드와 일관되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:295`, `367`
- **범주**: 버그/정확성
- **문제**: 같은 파일의 `GetCooldownTimeRemaining`(`:329`)·`GetCooldownTimeRemainingAndDuration`(`:346`)·`ApplyCooldown`(`:273`)은 모두 `ActorInfo ? ... : nullptr`로 가드하는데, `CheckCooldown`만 `ActorInfo->AbilitySystemComponent.Get()`을 무방비로 역참조한다. 또 `QueryActiveCooldowns`는 `ASC.GetWorld()->GetTimeSeconds()`를 널 검사 없이 부른다(미등록·소유자 소멸 시 널 가능). 현재 호출 경로에서는 둘 다 성립하지만, 같은 파일 안의 방어 수준이 갈려 있어 이후 호출자가 늘면 깨지기 쉽다.
- **제안**: `CheckCooldown`에 다른 오버라이드와 같은 가드를 넣고, `QueryActiveCooldowns`는 `const UWorld* World = ASC.GetWorld(); if (!World) return 0;`로 시작한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체(20+ GE 정의·MMC), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터 태스크 5종, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitMoving.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/` 헤더 전반, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`
- **기계적 확인**: 모든 소스 첫 줄 Copyright ✅(BOM만 일부 존재), `FORCEINLINE`/인라인 정의 0건 ✅, 람다 0건 ✅, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 1건뿐(BP Function Library라 적합) ✅, Delegate 콜백 `Handle` prefix 전수 준수 ✅, WxCore 외 Wx 플러그인 참조 0건 ✅(`.uplugin`·`Build.cs`·include 전수), 오버라이드 `Super::` 누락은 전량 "완전 대체" 성격(getter·pure virtual·ExecCalc·필터 술어)이라 위반 아님 ✅ — **규칙 위반 발견 없음**
- **미검토 / 한계**: GE 클래스들의 수치 상수(계수·지속시간)가 기획서와 일치하는지는 대조하지 않았다. `WxCueNotify_*`의 연출 타이밍과 Niagara 파라미터, `WxAnimNotifyState_CameraMove`의 `#if WITH_EDITOR` 프리뷰 경로는 동작 확인 없이 코드만 읽었다. BP 서브클래스(어빌리티·무기·투사체·큐)의 실제 설정값과 몽타주 노티파이 배치는 리뷰 범위 밖이므로, 이 모듈이 데이터에 거는 규약(콤보 섹션 이름, `SnapToTarget` 워프 타겟 이름, 쿨다운 마커 GE)이 에셋에서 지켜지는지는 검증하지 못했다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 153파일 — `/module-review`로 갱신*
