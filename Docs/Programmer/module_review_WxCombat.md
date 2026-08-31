# WxCombat — 코드 리뷰

> 전반적으로 건강한 모듈이다. 널 가드·권위 분기·태스크 수명 정리가 일관되고, 위험한 지점마다 "왜 이렇게 했는지"가 주석으로 남아 있어 의도 파악이 쉽다. 이번 리뷰는 ASC·어빌리티 베이스·어트리뷰트셋·대미지 파이프라인(ExecCalc/BFL/DamageTableRow)·무기/투사체 히트 판정·락온 계열·AnimNotify·어빌리티 태스크·GE 정의를 cpp까지 읽었고, BP 에셋 내부와 일부 소형 GE(`WxEffect_HealPercent`·`FullHP`·`NoCooldown`·`SuperArmor` 등)의 본문은 훑는 수준으로 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 크리 난수를 예측 클라와 서버가 각각 굴려 판정이 갈린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:170`
- **범주**: 설계/구조
- **문제**: `UWxCombatLibrary::ApplyDamage`가 공격 어빌리티의 활성화 예측 키를 실어 대미지 GE를 적용하고(`Private/WxCombatLibrary.cpp:88-91`, `:130`), 히트 판정 자체도 서버·클라가 각각 낸다(`Private/Weapon/WxWeaponBase.cpp:254` 주석). 엔진은 예측 키가 유효하면 비권위 머신에서도 GE를 적용하므로(`HasNetworkAuthorityToApplyGameplayEffect`) 소유 클라에서 ExecCalc가 그대로 돌고, 크리 판정만 `FMath::FRand()`로 로컬에서 새로 굴린다. 같은 히트인데 서버는 크리, 클라는 논크리가 되면 예측 HP 차감량과 `FWxCombatEffectContext::bCritical`, 나아가 `Damage.GuardBreak` 동적 태그까지 서버본과 어긋나 보정 시점에 HP 바가 튄다. 싱글에서는 드러나지 않는다.
- **제안**: 크리 판정은 권위에서만 굴리고 결과를 `FWxCombatEffectContext`(이미 복제된다)로 내려보내거나, 스펙에 서버가 정한 시드를 SetByCaller로 실어 양쪽이 같은 값을 뽑게 한다.
- **확신도**: 중간

### 2. 🟡 겹친 WeaponAttack 구간이 대미지 행과 피격 기록을 공유한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:45-73`
- **범주**: 버그/정확성
- **문제**: `ActiveAttackCount`는 판정 구간(ANS)의 중첩을 전제로 만든 참조 카운터인데, 정작 그 구간이 쓰는 상태는 단일 필드다. `BeginAttack`은 카운트와 무관하게 `HitActorsThisSwing.Empty()`(:54)와 `DamageInfo = InDamageInfo`(:57)를 무조건 수행한다. 콤보 전환(앞 ANS의 `NotifyEnd`가 새 `NotifyBegin`보다 늦게 오는 경우)만 상정한 코드라, 한 몽타주에 판정 구간이 실제로 둘 동시에 열리면 ① 앞 구간이 이미 때린 대상을 다시 때리고 ② 두 구간 모두 나중에 열린 쪽의 `DamageDataRow` 계수로 맞는다.
- **제안**: 구간별로 `(DamageRow, 히트 기록)`을 묶어 스택으로 들고 `EndAttack`에서 되돌리거나, 중첩을 허용하지 않는다고 확정하고 `ActiveAttackCount`를 단순 bool로 낮춰 의도를 코드에 드러낸다.
- **확신도**: 중간

### 3. 🟡 TimeDilation 복제가 엔진 WorldSettings 복제와 이중이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:103-132`, `Public/Time/WxTimeDilationComponent.h:50-51`
- **범주**: 중복/복잡도
- **문제**: `ApplyTimeDilation`은 `UGameplayStatics::SetGlobalTimeDilation` → `AWorldSettings::SetTimeDilation`으로 내려가는데, `AWorldSettings::TimeDilation`은 엔진이 이미 `DOREPLIFETIME`으로 전 클라에 복제한다(UE 5.8 `WorldSettings.cpp`). 즉 `ReplicatedTimeDilation`·`OnRep_ReplicatedTimeDilation`·late-join 대비 `BeginPlay` 재적용(:110-119)은 엔진이 하는 일을 한 번 더 하는 두 번째 채널이다. 값이 같아 증상은 없지만, 시간 배율의 진실 원천이 둘로 갈려 앞으로 이 컴포넌트를 손볼 때 어느 쪽이 이겼는지 추적해야 한다.
- **제안**: 복제 프로퍼티를 걷어내고 컴포넌트는 서버 전용 소유권 조정(`DilationOwner` 기반 Set/Clear)만 맡긴다. 값 전파는 엔진 WorldSettings 복제에 맡기면 late-join 처리도 자동으로 따라온다.
- **확신도**: 중간

### 4. 🟡 GE 수치가 C++ 상수에 박혀 있어 DT_Effect 경로가 놀고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Exceed.cpp:10,22,28`, `Public/AbilitySystem/Effect/WxEffect_DrainSP.h:21`, `Public/AbilitySystem/Effect/WxEffect_RegenSP.h:21`
- **범주**: 설계/구조
- **문제**: `UWxEffectComponent_Table`·`UWxMMC_EffectMagnitude`·`UWxMMC_EffectDuration`이 준비돼 있으나 C++ GE 중 이 경로를 쓰는 것이 하나도 없다(`grep` 결과 참조자는 `WxEffectComponent_Table.cpp` 자신뿐). 결과적으로 Exceed의 지속 6초·ATK/ASPD 1.2배, 스태미나 완전 소모 8초·완전 회복 4초 같은 순수 밸런스 수치가 코드에 남아 기획자가 못 만지고 빌드가 필요하다. 반면 `UWxEffect_Guard::DamageMultiplier`와 `UWxEffect_Exhaust::ConsumeDelay/ExhaustDuration`은 C++ 소비처(`WxEffect_Damage.cpp:96`, `WxCombatAttributeSet.cpp:139`)가 있어 성격이 다르다.
- **제안**: C++ 소비처가 없는 순수 밸런스 값부터 `DT_Effect` 행 + `UWxMMC_Effect*`로 옮긴다. C++이 읽어야 하는 값은 남기되 왜 남았는지 한 줄 남긴다.
- **확신도**: 중간

### 5. 🟢 Attack·Skill·Pattern이 콤보 인덱스 로직을 세 벌 갖고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:29-31,42,51`, `WxAbility_Skill.cpp:31-33,44,52`, `WxAbility_Pattern.cpp:29-31,42`
- **범주**: 중복/복잡도
- **문제**: `ComboIndex` 전진식·`ComboMontages` 조회·캔슬 시 `INDEX_NONE` 되돌림이 세 파일에 글자 단위로 같다. Pattern만 `HandleMontageBlendOut`에서 자동 연쇄(:48-60)를 얹어 갈라진다.
- **제안**: 프로젝트 방침(최소 인플레이스 우선, 약간의 반복 용인)을 고려하면 지금 당장 뽑아낼 필요는 없다. 다만 네 번째 콤보형 어빌리티가 생기면 `UWxAbilityBase`에 `ComboMontages`/`ComboIndex`를 올리는 쪽이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 ApplyAttributeChange가 호출마다 런타임 GE를 고정 이름으로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:186-197`
- **범주**: 성능/안전
- **문제**: `NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ApplyAttributeChange")))`는 호출마다 UObject를 만들고, 같은 Outer·같은 이름이라 앞 객체가 아직 GC되지 않았으면 엔진이 교체 경로를 타며 경고를 남긴다. 현재 호출처는 패리 GP 반사(`WxCombatAttributeSet.cpp:433`)와 치트뿐이라 빈도는 낮다. 지역 변수 이름 `InfoXP`(:192)도 참조 샘플에서 그대로 넘어온 흔적이다.
- **제안**: 이름을 `NAME_None`으로 두거나, SetByCaller 크기를 받는 작은 GE 클래스를 하나 두고 그걸 재사용한다.
- **확신도**: 중간

### 7. 🟢 락온 종료의 이동 회전 복원이 무관한 조건에 묶여 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:104,114-123`
- **범주**: 버그/정확성
- **문제**: `bOrientRotationToMovement` 복원이 `ActorInfo->AbilitySystemComponent.IsValid()` 블록 안에 들어 있다. ASC 유효성은 락온 타겟 해제·태스크 정리에는 필요하지만 이동 컴포넌트 복원과는 무관하다. 그 게이트가 한 번이라도 실패하면 `SavedOrientRotationToMovement`가 Set인 채 남고, 다음 활성화가 그 `false`를 "이전 값"으로 다시 저장해 이후 영구히 이동 방향 회전이 꺼진다.
- **제안**: 복원 블록만 ASC 게이트 밖으로 꺼내 `ActorInfo` 유효성만 보게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 8. 🟢 AWxGhostTrail::EndPlay가 Super만 부르는 빈 오버라이드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:40-43`
- **범주**: 중복/복잡도
- **문제**: 선언(`Public/AbilitySystem/Cue/WxCueNotify_GhostTrail.h:102`)과 정의 모두 아무 일도 하지 않는다.
- **제안**: 지운다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Private/WxCombatLibrary.cpp`, `Private/Damage/WxDamageTableRow.cpp`, `Private/Damage/WxCombatEffectContext.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/Weapon/WxProjectileComponent.cpp`, `Private/Targeting/WxLockOnComponent.cpp`, `Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/Finisher/WxFinisherDamageComponent.cpp`, `Private/AbilitySystem/Ability/WxAbility_{LockOn,Dodge,Guard,GuardReact,HitReact,Groggy,Death,Finisher,Sprint,Attack,Skill,Pattern,Ultimate,Passive,PlayMontageOnce}.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_{LockOnCamera,SlowTime,RotateToTarget,PlaySkillCutscene,WaitMoving}.cpp`, `Private/AnimNotify/` 전체, `Private/AbilitySystem/WxAbilitySet.cpp`, `Private/AbilitySystem/WxAbilitySystemGlobals.cpp`
- **훑은 파일**: `Private/AbilitySystem/Effect/` 나머지 GE(`Cost`·`Cooldown`·`Exhaust`·`Guard`·`Invincible`·`PerfectGuard`·`Kill`·`Exceed`·`Drain*`·`Regen*`·`Table`), `Private/AbilitySystem/Cue/` 전체, `Private/Targeting/WxTargetingFilterTask_*.cpp`, `Private/Targeting/WxLockOnPointComponent.cpp`, `Public/` 헤더 전반, `WxCombat.Build.cs`, `WxCombat.uplugin`
- **미검토 / 한계**:
  - 규칙 위반은 기계 검사로 전수 확인했고 발견 없음 — `FORCEINLINE`/인라인 정의 0건, `BlueprintCallable`은 BFL(`UWxCombatLibrary::ApplyDamage`) 한 곳뿐, 델리게이트 콜백 전부 `Handle` 접두사, 타입 전부 `Wx` 접두사, 저작권 첫 줄 159개 파일 전부 존재(BOM 때문에 단순 grep으로는 누락처럼 보임), 람다 0건. 의존성도 `WxCore` 외 다른 Wx 플러그인 참조 없음.
  - BP 파생 어빌리티가 `ActivateAbility`/`ActivateAbilityFromEvent` 이벤트를 구현했을 때 네이티브 `Super::ActivateAbility` 호출과 겹치는 경로는 실제 BP 에셋을 못 봐 판단 보류했다(BP 내부는 범위 밖).
  - `UWxCueNotify_DamageFloater`가 히트마다 액터+위젯을 스폰하는 비용, `UWxEffect_Cost`가 코스트 0인 어빌리티에서도 MP/UP/SP 3개 모디파이어를 매번 실행하는 비용은 확인했으나 실측 없이 문제로 적지 않았다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 159파일 — `/module-review`로 갱신*
