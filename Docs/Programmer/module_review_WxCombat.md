# WxCombat — 코드 리뷰

> 규모(161파일)에 비해 상태가 좋다. 리플리케이션 권위 게이팅·스코프 락·태스크 수명 정리가 대체로 일관되고, 까다로운 지점마다 근거 주석이 붙어 있어 의도를 확인하기 쉬웠다. 심각(🔴) 등급 결함은 찾지 못했고, 남은 것은 조용한 실패 경로 몇 개와 구조 중복이다. 이번 리뷰는 ASC·어빌리티 베이스·어트리뷰트/데미지 파이프라인·무기/투사체·락온·어빌리티 태스크·AnimNotify·GE/큐/타게팅 필터를 cpp 레벨까지 훑었고, BP 에셋과 DataTable 내용은 범위 밖이다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 데미지 컨텍스트 타입이 어긋나면 피격 파이프라인 전체가 조용히 사라진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:275-285`
- **범주**: 설계/구조
- **문제**: `CombatContext`는 실제로 340행 근처의 크리 태그(`Damage_Critical`) 부여 한 곳에만 쓰인다. 그런데 280행의 얼리 리턴이 그 하나 때문에 가드 캔슬(294행)·피격 이벤트 발송(317행)·`Event.DamageDealt`(328행)·데미지 플로터 큐(340행)까지 전부 막는다. 즉 컨텍스트가 `FWxCombatEffectContext`가 아니면 HP는 깎이는데 히트리액트·그로기 라우팅·연출이 통째로 증발하고, 그 사실을 알릴 로그나 `ensure`가 없다. 같은 조건을 보는 `UWxExecCalc_Damage`에는 진단이 있다(`WxEffect_Damage.cpp:135`의 `ensureMsgf`). 또 `GetScriptStruct() != FWxCombatEffectContext::StaticStruct()`는 정확 일치 비교라, 나중에 컨텍스트를 파생시키면 그 순간 같은 증상이 재현된다.
- **제안**: 조기 반환 조건을 `!ASC`만 남기고, `CombatContext`는 nullable로 받아 크리 태그 부여에만 사용한다. 타입 불일치는 `ensureMsgf`/`UE_LOG(LogWxCombat, Error, ...)`로 드러낸다. 파생을 허용하려면 `IsChildOf` 비교로 바꾼다.
- **확신도**: 높음

### 2. 🟡 i-frame(Effect.Invincible) 해제가 ANS의 NotifyEnd 하나에만 걸려 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:22-32`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp:11`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:95-102`
- **범주**: 버그/정확성
- **문제**: `UWxEffect_Invincible`은 `Infinite`라 스스로 만료되지 않는다. 회피는 `EndAbility`에서 판정 캡슐만 되돌리고(99행) GE는 손대지 않으며, 주석(97-98행)도 "무적 태그 자체는 구간을 소유한 ANS가 걷어낸다"고 명시한다. 따라서 몽타주가 비정상 종료돼 `NotifyEnd`가 유실되는 경로가 하나라도 생기면 `Effect.Invincible`이 영구히 남고, 그 시점부터 `UWxCombatLibrary::CheckDamage`(`WxCombatLibrary.cpp:51`)가 모든 피격을 `Evaded`로 흘리며 `UWxAbility_HitReact`의 `ActivationBlockedTags`(`WxAbility_HitReact.cpp:28`)까지 계속 닫힌다 — 무적 상태의 플레이어가 복구 불가로 고착된다. 같은 GE를 쓰는 `UWxAbilityTask_PlaySkillCutscene::OnDestroy`(`WxAbilityTask_PlaySkillCutscene.cpp:24-28`)는 정확히 이 목적의 안전망을 갖고 있어, 두 사용처의 처리가 비대칭이다.
- **제안**: 무작정 `RemoveActiveGameplayEffectBySourceEffect`를 회피 종료에 추가하면 정상 경로에서 두 번 걷혀 남의 스택까지 벗기므로 그대로는 안 된다. 최소한 (a) 어빌리티 종료 시 자기 소유 구간이 아직 열려 있는지 검사해 로그로 드러내거나, (b) ANS가 적용 핸들을 소유 ASC 기준으로 추적해 어빌리티가 회수할 수 있게 통로를 만든다.
- **확신도**: 중간

### 3. 🟡 HitReact의 `HitReact_Normal` 기본값이 도달 불가라, HitReactTag를 비운 대미지 행은 반응이 아예 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:67-81`
- **범주**: 버그/정확성
- **문제**: 67행이 세운 기본값 `HitReact_Normal`을 70행이 무조건 덮어쓴다. 이 어빌리티는 GameplayEvent 트리거 전용이라 `TriggerEventData`가 항상 존재하므로 67행 초기값은 실행되지 않는 코드다. `FWxDamageTableRow::HitReactTag`(`Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h:22`)는 기본값이 무효 태그이므로, 기획자가 그 칸을 비워 둔 대미지 행은 77행에서 곧바로 종료돼 피격 몽타주가 전혀 나오지 않는다. 반면 `UWxAbility_GuardReact::SelectMontage`(`WxAbility_GuardReact.cpp:121`)는 같은 상황에서 `GuardHitReactMontage`로 폴백하므로 두 반응 경로의 동작이 어긋나 있다.
- **제안**: 67행 의도대로 폴백을 살리려면 70행 결과가 무효일 때 `HitReact_Normal`로 되돌린다. 반대로 "태그 없음 = 무반응"이 의도라면 67행 초기값을 지우고 그 규약을 `FWxDamageTableRow::HitReactTag` 주석에 명시한다.
- **확신도**: 중간 (의도된 설계일 수 있음)

### 4. 🟡 AnimNotify → GameplayEvent → 수신 컴포넌트 3세트가 통째로 중복
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileComponent.cpp:13-58`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp:14-69`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp:31-79` / `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp:9-25`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnMinion.cpp:8-24`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp:8-24`
- **범주**: 중복/복잡도
- **문제**: 컴포넌트 3종의 `BeginPlay`(`GenericGameplayEventCallbacks.FindOrAdd(Tag).AddUObject`), `EndPlay`(`Find(Tag)->Remove` + 멤버 리셋), 핸들러 프리앰블(`HasAuthority` 게이트 → `OptionalObject`를 노티파이 타입으로 캐스트 → `OptionalObject2`를 메시로 캐스트해 `Mesh->GetOwner() != Owner` 검증)이 토씨 하나 다르지 않게 3벌 존재한다. 노티파이 3종의 `Notify` 본문도 이벤트 태그만 다르고 동일하다. 네 번째 기능을 붙일 때 같은 40여 줄을 또 복사해야 하고, 검증 규약이 한 곳에서만 바뀌면 조용히 갈라진다.
- **제안**: 이벤트 태그를 프로텍티드 멤버로 갖는 공용 베이스 컴포넌트를 두고 바인딩·해제·검증 프리앰블을 그쪽으로 올린 뒤, 파생은 검증을 통과한 노티파이/메시를 받는 훅만 구현한다. 노티파이 쪽도 같은 방식으로 `Notify` 본문을 베이스로 올린다.
- **확신도**: 높음

### 5. 🟡 `UWxAbility_Attack`과 `UWxAbility_Skill`이 생성자를 뺀 전부 동일
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54` / `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`
- **범주**: 중복/복잡도
- **문제**: `ActivateAbility`(커밋 → `ComboIndex` 순환 → `PlayMontage`), `EndAbility`(취소 시 `ComboIndex = INDEX_NONE`), `HandleMontageCompleted`가 세 함수 모두 완전히 같다. 헤더의 `ComboMontages`·`ComboIndex` 선언도 각각 중복이며(`WxAbility_Attack.h:33-38`, `WxAbility_Skill.h:36-41`), `UWxAbility_Pattern`(`WxAbility_Pattern.h:30-34`)이 같은 두 멤버를 세 번째로 또 선언한다. 두 클래스의 실질 차이는 생성자의 애셋 태그와 `CooldownGameplayEffectClass`뿐이다.
- **제안**: 콤보 진행(멤버 2개 + 3함수)을 공통 베이스로 올리고, `Attack`/`Skill`은 생성자만 남긴다. `Pattern`은 진행 방식이 달라(블렌드아웃 자동 진행) `HandleMontageBlendOut`만 오버라이드하면 같은 베이스를 공유할 수 있다.
- **확신도**: 높음

### 6. 🟡 `CancelRecoveringAbilities`가 배타 발동마다 스펙 수만큼 힙 할당을 낸다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:305-327` (317행)
- **범주**: 성능/안전
- **문제**: `Spec.GetAbilityInstances()`는 `ReplicatedInstances`와 `NonReplicatedInstances`를 합친 `TArray`를 값으로 돌려주므로, 활성 스펙 하나당 `TArray` 할당이 발생한다. 이 함수는 모든 비-Independent 어빌리티의 `ActivateAbility`(`WxAbilityBase.cpp:185`)에서 불린다. 같은 파일 123행 주석이 "사본을 만드는 GetAbilityInstances 대신 두 배열을 직접 훑는다"며 이 비용을 이미 인지하고 있고, `UWxAbilityBase::FindActivationGroupBlocker`(`WxAbilityBase.cpp:111-112`)는 "모든 Wx 어빌리티는 기반 생성자가 InstancedPerActor를 강제하므로 스펙당 인스턴스는 하나뿐"이라며 `GetPrimaryInstance()`를 쓴다. 세 곳 중 여기만 규약이 다르다.
- **제안**: 내부 for 루프를 없애고 `Spec.GetPrimaryInstance()` 한 번 조회로 통일한다(`FindActivationGroupBlocker`와 동일한 근거).
- **확신도**: 높음

### 7. 🟢 `PreviousMontageTickOption`이 초기화되지 않은 멤버
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:89`
- **범주**: 성능/안전
- **문제**: 지금은 `EnableAnimatingMontageMeshTick`이 항상 이 값을 먼저 쓰고 `MontageTickMesh`를 세우므로(`WxAbilitySystemComponent.cpp:67-68`) 복원 경로가 사실상 가드된다. 다만 초기화가 없어, 두 멤버의 갱신 순서나 가드 조건이 바뀌는 순간 쓰레기 값이 메시의 `VisibilityBasedAnimTickOption`으로 들어간다. 같은 헤더의 다른 스칼라 멤버(`HitStopResumePlayRate`, `InputBufferDuration`)는 모두 초기값을 갖고 있어 여기만 예외다.
- **제안**: `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` 등 명시적 기본값을 준다.
- **확신도**: 높음

### 8. 🟢 `ApplyAttributeChange`가 호출마다 고정 이름의 트랜지언트 GE를 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:186-197`
- **범주**: 성능/안전
- **문제**: `NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ApplyAttributeChange")))`는 같은 이름의 이전 객체가 아직 GC되지 않았으면 그 자리를 재사용하며 기존 객체를 파괴한다(`StaticAllocateObject`). 호출마다 UObject를 하나 만드는 비용도 있다. 현재 호출부가 퍼펙트가드 GP 반사 한 곳(`WxCombatAttributeSet.cpp:362`)뿐이라 실해는 낮지만, 이 함수는 public API라 호출부가 늘면 위험도가 같이 오른다.
- **제안**: 이름을 넘기지 않아 자동 고유 이름을 받게 하거나, 다른 GE들처럼 전용 GE 클래스 + SetByCaller 조합으로 바꾼다.
- **확신도**: 중간

### 9. 🟢 데미지 공식이 단일 호출 헬퍼 5단으로 쪼개져 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:71-107`
- **범주**: 중복/복잡도
- **문제**: `CalculateDefenseMultiplier`·`CalculateBaseDamage`·`CalculateCriticalMultiplier`·`CalculateGuardMultiplier` 네 개가 각각 한 줄짜리이고 전부 `CalculateFinalDamage` 한 곳에서만 불린다. 결과적으로 전투 밸런스의 핵심 수식이 파일 안에서 다섯 조각으로 흩어져, 한 히트의 최종 계수를 확인하려면 5개 함수를 따라가야 한다.
- **제안**: `CalculateFinalDamage` 하나로 접거나(수식 6줄), 최소한 방어/크리처럼 의미가 분리되는 두 단계만 남긴다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`
- **훑은 파일**: 나머지 어빌리티 전체(`WxAbility_Attack/Skill/Pattern/Sprint/Ultimate/Passive/Death/PlayMontageOnce`), `WxAbilitySet.cpp`, `WxAbilitySystemGlobals.cpp`, `WxCombatEffectContext.cpp`, `WxDamageTableRow.cpp`, `AnimNotify/*` 전체, `AbilitySystem/Effect/*` 전체, `AbilitySystem/Cue/*` 전체, `Targeting/WxTargetingFilterTask_*`, `WxLockOnPointComponent.cpp`, `WxProjectileComponent.cpp`, `WxMinionComponent.cpp`, `WxFinisherDamageComponent.cpp`, 대응 헤더와 `WxCombat.Build.cs`·`WxCombat.uplugin`
- **규칙 준수 확인 결과**: 161개 소스 전부 Copyright 첫 줄 존재, `Wx` prefix 누락 타입 없음, `FORCEINLINE`·헤더 인라인 정의 0건, 람다 0건, 델리게이트 바인딩 28건 모두 `Handle` prefix, `BlueprintCallable`은 `UWxCombatLibrary`(BP Function Library) 1건뿐, 다른 Wx 플러그인 include 0건(`WxCore`만 참조). 오버라이드 `Super::` 미호출은 베이스가 빈 구현이거나 의도적 대체인 경우만 확인됨. CLAUDE.md 위반 없음.
- **미검토 / 한계**: BP/WBP 및 DataTable(`WxAbilityTableRow`/`WxEffectTableRow`/`WxDamageTableRow`) 실데이터는 범위 밖이라, 데이터 기입 누락으로만 드러나는 문제(3번 항목이 그 부류)는 코드 근거까지만 제시했다. 멀티플레이 실측(예측 대미지 롤백, 크리 RNG의 서버/클라 불일치)은 정적 분석으로만 따졌고 실제 세션에서 재현하지는 않았다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 161파일 — `/module-review`로 갱신*
