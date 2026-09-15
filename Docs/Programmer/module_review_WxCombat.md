# WxCombat — 코드 리뷰

> 직전 리뷰 뒤 들어온 변경(사망 몽타주 소유 해제, 반응 게이트를 일반 피격에만 한정, HitReact 차단을 GE 선언으로 이동, 구간 GE의 예측 키 선택)은 엔진 경로와 맞게 들어갔다. 그 결과 직전 3·4·5번이 해소됐고, 변경분에서 새 결함은 나오지 않았다. 남은 결함은 비권위 머신의 시간 배율·GE 제거, 사유 수 없이 나눠 쥔 BT 일시정지, 그리고 호출자 없는 선언·낡은 소유 주석에 몰려 있다.
> 이번 리뷰는 직전 리뷰(`993d2a031`) 이후 바뀐 소스 17파일을 diff로 읽고 설치된 UE 5.8 엔진 소스와 대조했다. 직전 발견 9건은 현재 코드로 다시 검증했다. 그 밖에 187파일 규칙 스캔을 돌리고 어빌리티·대미지·무기·타겟팅·소환 cpp 전반을 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 1 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 컷신 태스크가 비권위 머신에서도 요청 배율 기준 1000배속으로 재생해, 원격 소유 클라에서는 궁극기 컷신이 첫 틱에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117-120`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:58-64`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:54`
- **범주**: 버그/정확성
- **문제**: 월드 배율은 권위에서만 건다(58-64행). 클라이언트에는 `AWorldSettings::TimeDilation` 복제로 한두 넷 업데이트 늦게 도착한다(엔진 `Engine/Source/Runtime/Engine/Classes/GameFramework/WorldSettings.h:741-742`). 그런데 재생 속도 보정(117-120행)은 머신을 가리지 않고 요청값 0.001을 기준으로 1000배를 건다. 시퀀스는 월드 게임 시간의 차분으로 진행하므로(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:303`, `:327-343`), 배율이 아직 1인 소유 클라에서는 한 프레임(약 16ms)에 16초가 흘러 곧바로 `OnFinished`에 닿는다.
  - 궁극기는 실행 정책을 바꾸지 않아 베이스 기본값 LocalPredicted다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:19`). 콘텐츠의 `GA_Template_Ultimate`·`GA_HGTest_Ultimate`도 정책을 재지정하지 않고 `CutsceneSequence`를 저작했다. 따라서 소유 클라가 서버보다 먼저 발동하고 이 창은 사실상 늘 열린다.
  - 결과적으로 `HandleCutsceneCompleted`(`WxAbility_Ultimate.cpp:64-77`)가 서버보다 컷신 길이만큼 먼저 `UltimateMontage`를 틀고, 뒤이어 도착한 0.001 배율이 그 몽타주를 세운다. `OnDestroy`(24-28행)도 무적 GE를 로컬에서 먼저 걷는다. 원격 플레이어는 컷신을 보지 못하고, 몽타주·노티파이 타이밍이 서버와 어긋난다.
  - 스탠드얼론과 리슨 서버 호스트는 배율이 같은 프레임에 걸려 드러나지 않는다.
- **제안**: 보정값을 요청값이 아니라 그 머신에 실제로 걸린 배율(`UGameplayStatics::GetGlobalTimeDilation`)에서 구하고, 배율이 바뀌면 다시 맞춘다. 또는 시퀀스를 월드 배율과 무관한 클럭(`EUpdateClockSource::Platform`, 엔진 `Engine/Source/Runtime/MovieScene/Public/MovieSceneFwd.h:68-74`)으로 돌려 보정 자체를 없앤다.
- **확신도**: 중간(엔진 틱 경로와 콘텐츠 정책은 확인, 네트워크 PIE 실측은 없다)

### 2. 🟡 돌진 modifier와 그로기가 사유 수 없는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 소환물이 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:65`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:173-191`
- **범주**: 설계/구조
- **문제**: `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `Engine/Source/Runtime/AIModule/Private/BehaviorTree/BehaviorTreeComponent.cpp:173-176`, `:187-194`). 돌진은 자기가 멈춘 두뇌를 기억했다가, 해제 때 `IsPaused()`이면 무조건 재개한다(179-183행).
  - 실패 순서는 이렇다. 돌진 중 GP가 차서 그로기가 뜨면, 그로기의 `Ability` 취소 지목(`WxAbility_Groggy.cpp:32`)이 돌진 스킬을 끝내 몽타주를 블렌드아웃시킨다. 그로기가 두뇌를 멈추지만(65행) 이미 멈춰 있어 아무 일도 없다.
  - 블렌드아웃이 끝나 몽타주 인스턴스가 종료되면 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부른다(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1759-1768`). 이 호출이 `UWxAnimNotifyState_Rush::BranchingPointNotifyEnd`(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`) → `CancelRush` → `ReleaseState`로 이어져, 그로기가 건 정지까지 풀린다.
  - 그 뒤 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다. 콘텐츠 문자열 검색상 돌진 노티파이는 `AM_Minion_Skill_2`에 있고, `BP_Minion`은 그로기를 담은 `ABS_Shared_Enemy`를 받는다.
- **제안**: 재개 여부를 기억한 플래그가 아니라 현재 상태에서 판정한다. 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(`WxAbility_Groggy.cpp:76`)가 재개를 맡는다. 두 곳이 "지금 멈춰 있어야 하는가" 판정 하나를 공유하면, 일시정지 주체가 늘어도 같은 충돌이 생기지 않는다.
- **확신도**: 중간(엔진 경로는 소스로 확인했다. 돌진 몽타주의 블렌드아웃이 0이면 해제가 그로기 일시정지보다 먼저 와 드러나지 않고, 발현은 소환물 MaxGP 저작에 달렸다)

### 3. 🟢 비권위 머신의 종료 경로 GE 제거는 권위 게이트에 막혀 경고만 남기고, 컷신 태스크만 게이트를 우회해 복제 GE를 먼저 걷는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:225-232`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:242-250`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본값이 0이다(엔진 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemPrivate.h:21`, 프로젝트 ini 재정의 없음). 그래서 비권위 머신의 `RemoveActiveGameplayEffect`는 핸들 유효성과 무관하게 Warning을 찍고 false를 돌려준다(엔진 `.../Private/AbilitySystemComponent.cpp:1249-1260`).
  - `ActivationOwnedEffectHandles`는 적용에 실패한 빈 핸들까지 담는다(230행). 비권위에서 예측 키가 없으면 엔진이 빈 핸들을 준다(엔진 `.../Private/Abilities/GameplayAbility.cpp:2040-2053`). 종료 때는 머신 구분 없이 제거를 부른다(245-248행).
  - 그래서 소유 클라에서 궁극기(`WxAbility_Ultimate.cpp:20`)와 처형(`WxAbility_Finisher.cpp:32`, ServerInitiated라 클라 핸들이 늘 비어 있다)이 끝날 때마다 효과 수만큼 경고가 난다. 질주의 속도·드레인 제거도 같다.
  - 실제 정리는 예측본은 키 확인(엔진 `.../Private/GameplayEffect.cpp:4527-4528`)이, 서버본은 복제가 맡으므로 동작은 맞다. 다만 242행 주석("효과가 새지 않는다")은 클라 제거가 동작하는 것처럼 읽힌다.
  - 반대로 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 권위 검사 없이 컨테이너를 직접 부른다(엔진 `.../Private/AbilitySystemComponent.cpp:1292-1318`, 게이트가 있는 `RemoveActiveEffects`는 `:1832-1840`). 그래서 소유 클라가 복제된 무적 GE를 서버보다 먼저 로컬에서 걷는다. 1번과 겹치면 컷신 길이만큼 이르다.
- **제안**: 제거 호출은 권위에서만 하고, 비권위에서는 핸들만 비운다(빈 핸들은 애초에 담지 않는다). 컷신 태스크의 정의 기반 제거도 권위로 한정한다. `WxAbilityBase.cpp` 242행과 `WxAbilityTask_PlaySkillCutscene.cpp` 26행 주석은 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 4. 🟢 무기 판정이 소유 클라·시뮬 프록시에서도 콜리전과 매 틱 스윕을 켜지만, 그 결과를 쓰는 곳이 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:50-78`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:140-189`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12-26`
- **범주**: 성능/안전
- **문제**: 공격 노티파이는 네이티브 분기점이라 몽타주가 도는 모든 머신에서 `BeginAttack`을 부른다. `BeginAttack`은 권위를 가리지 않고 형상 콜리전을 켜고 액터 틱을 연다(64-75행). 그 뒤 매 틱 형상별 `SweepMultiByChannel`(180행)과 오버랩 이벤트가 `ProcessHit`로 모인다.
  - 하지만 비권위 머신에서는 `ApplyDamage`가 권위 검사에서 곧바로 false를 돌려주고(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:45`), 히트스톱도 권위에서만 건다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`).
  - 즉 클라이언트의 스윕·오버랩은 소비처가 없는 비용이다. 관련성 범위 안에서 동시에 공격하는 적 수만큼 모든 클라에 곱해진다.
- **제안**: `BeginAttack` 첫머리에서 소유 액터가 권위가 아니면 반환한다. 그러면 `ActiveAttackCount`가 0으로 남아 `EndAttack`, 그리고 사망 시의 `CancelAttack`(`Source/WxGame/Character/WxCharacterBase.cpp:274`)은 그대로 아무 일도 하지 않는다.
- **확신도**: 중간(소비처가 없다는 것은 코드로 확인, 실제 비용은 동시 교전 규모에 달렸다)

### 5. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 새 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21-22`). 그런데 `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 둔다.
  - `OnGiveAbility` 진단(`WxAbilityBase.cpp:304-309`)은 "쿨다운 태그 없음"만 잡으므로 이 누락을 통과시킨다.
  - 현재 콘텐츠의 `GA_HGTest_Skill_2`·`GA_Minion_Skill_2`는 `CooldownGameplayEffectClass`를 재지정해 두어(uasset 문자열 검색) 지금은 드러나지 않는다. 새로 만드는 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나로 서로 막히는 잠복 함정이다.
  - 같은 생성자의 애셋 태그 기본값(10-15행)은 빠뜨려도 안전한 방향이지만, 쿨다운 기본값은 반대 방향이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌리면 기존 Error 진단이 누락을 잡는다. 이때 기본값에 기대는 슬롯 1 BP(`GA_Template_Skill_1`·`GA_HGTest_Skill_1`·`GA_Minion_Skill_1`, 재지정 문자열 없음)에는 `UWxEffect_Cooldown_Skill_1`을 명시 지정해야 쿨다운이 사라지지 않는다.
- **확신도**: 중간(발현은 이후 슬롯 BP 저작에 달렸다)

### 6. 🟢 가드·퍼펙트 가드 헤더가 처리 주체를 `UWxEffectComponent_DamageResponse`로 가리키지만, 실제 처리는 `UWxEffectComponent_Hit`에 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Guard.h:15`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Guard.h:22`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Cue/WxCueNotify_PerfectGuard.h:12`
- **범주**: 설계/구조
- **문제**: 세 주석이 모두 DamageResponse를 주체로 적었지만, 코드의 실제 주체는 다르다.
  - 퍼펙트 가드의 GP 반사·역경직 이벤트·큐(15행, 큐 헤더 12행)는 `UWxEffectComponent_Hit::ProcessPerfectGuard`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:161-198`, 큐는 197행)가 처리한다.
  - "가드를 Cancel한 뒤 Event.Hit을 보낸다"(22행)는 `ProcessDamageTaken`(같은 파일 124-128행, 146행)이 한다.
  - `UWxEffectComponent_DamageResponse`는 실행 결과를 컨텍스트에 기록하고 대미지 플로터 큐를 내는 일만 한다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp:19-37`).
  - 가드 흐름을 추적하는 사람이 엉뚱한 파일로 가게 된다.
- **제안**: 세 주석의 주체를 `UWxEffectComponent_Hit`으로 고친다.
- **확신도**: 높음

### 7. 🟢 호출자 없는 선언 2건 — 그중 `EWxAbilityActivationPolicy::OnGiven`은 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:311-317`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h:49`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:52-55`
- **범주**: 중복/복잡도
- **문제**:
  - `OnGiven`은 C++ 어디서도 고르지 않고, 콘텐츠 uasset 문자열 검색에도 저장된 값이 없다. 그런데 `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(엔진 `.../Private/GameplayAbilityTypes.cpp:295`), 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다. LocalPredicted 기본값에서는 클라 예측 발동과 서버발 활성이 겹친다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
  - `UWxAbility_Finisher::IsBackstab()`은 public이지만 UFUNCTION이 아니고 저장소 전체에 호출자가 없다. 내부 코드는 `bBackstab`을 직접 읽는다.
- **제안**: 둘 다 걷는다. `OnGiven`을 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(`OnGiven` 경합 양상)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_SuperArmor.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, 대응 Public 헤더(변경된 헤더 9개 포함)
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지(Attack·Skill·Pattern·Passive·PlayMontageOnce), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지(Cooldown·Cost·Table·AddAttribute·Exhaust·RegenSP·DrainSP·DrainGP·GuardReduction·PerfectGuard·Exceed·NoCooldown·InfiniteMP·ResetGP·HealPercent·Hit), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지(SlowTime·WaitMoving·RotateToTarget), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·LockOnPoint·SnapToTarget, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`
- **미검토 / 한계**:
  - 읽지 않은 파일: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingPreview.cpp`(에디터 전용 프리뷰), `WxEffect_FullHP.cpp`·`WxEffect_MoveSpeedScale.cpp`(단순 GE 정의), `WxCombatModule.cpp`.
  - 규칙 스캔(187파일 기계 검사) 결과:
    - 첫 줄 Copyright 누락 0건
    - `FORCEINLINE`·`inline`·람다 0건
    - 헤더 안 함수 본문 0건(GAS 표준 `ATTRIBUTE_ACCESSORS` 매크로 전개 제외)
    - Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include` 모두 `WxCore`뿐이다.
    - `WxAbility_Finisher.cpp:13`·`WxAnimNotifyState_SnapToTarget.cpp:10`의 익명 namespace는 CLAUDE.md 규칙이 아니라서 규칙 위반으로 올리지 않았다.
  - 멀티플레이 실측이 없다. 1·2·3·7번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이며, PIE 재현은 하지 않았다. 문서의 엔진 경로는 이 엔진 루트 기준이다.
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열 검색은 보조 근거로만 썼다(궁극기 정책, 돌진 노티파이·그로기 배치, 슬롯 BP 쿨다운 재지정, `OnGiven` 저장값 없음, 무적·슈퍼아머 태그 직접 부여 에셋 없음).
  - 직전 리뷰(`993d2a031`) 대비 해소 확인:
    - **3번(사망 AnimatingAbility)**: 재생 길이 대기 뒤 `ClearAnimatingAbility`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:91-103`)로 해소됐다. 해제 뒤에도 몽타주 태스크의 인터럽트 통지는 현재 몽타주 비교와 무관하게 나가므로(엔진 `.../Private/Abilities/Tasks/AbilityTask_PlayMontageAndWait.cpp:19-58`) 래그돌 폴백이 유지된다.
    - **4번(구간 GE 예측 키)**: 엔진 표준 키 선택 `GetPredictionKeyForNewAction()`(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:102`)으로 바뀌었다. 애님 틱에서 도는 노티파이는 빈 키를 받는다.
    - **5번(행 주석과 반응 게이트)**: 게이트가 `Event.Hit` 정확 일치에만 걸리도록(`WxAbility_HitReact.cpp:38-54`, `WxAbility_GuardReact.cpp:40-56`) 동작이 되돌려져, 행 주석(`Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h:20-23`)과 일치한다.
  - 변경분 검증 요점(결함 없음):
    - 응답 훅의 페이로드 널 검사 제거는 엔진이 페이로드가 있을 때만 훅을 부르므로 안전하다(엔진 `.../Private/AbilitySystemComponent_Abilities.cpp:1800-1808`).
    - GE 쪽 `Ability.HitReact` 차단은 HitReact의 애셋 태그(`WxAbility_HitReact.cpp:12-14`)와 맞는다. GuardReact는 애셋 태그가 달라 가드 브레이크 처리를 막지 않는다.
    - 히트 컴포넌트의 무반응 가드 해제 분기 제거는 안전하다. GuardReact가 모든 가드 브레이크를 받아 커밋 전에 가드를 끊고(`WxAbility_GuardReact.cpp:72-81`), 가드와 가드 리액션이 같은 `ABS_Shared_Player`에 들어 있다.
  - 올리지 않은 항목:
    - 사망 해제 대기가 히트스톱(메시 `GlobalAnimRateScale=0`) 동안 몽타주보다 앞서는 오차는 작업 기록(`.claude/worklog/2026-09-15-사망-몽타주-소유-해제.md`)이 무해로 판단해 받아들인 것이다.
    - HitReact 직접 발동 차단의 제거는 호출 경로 부재를 확인한 결정이다(`.claude/worklog/2026-09-15-반응-어빌리티-응답훅-단일화.md`).
    - `AWxProjectileBase::Reflect`의 쏜 쪽 역참조는 오버랩 핸들러의 적대 판정이 널 Instigator를 먼저 거른다.
    - 콤보 창의 태그 요건 면제, Attack·Skill·Pattern 콤보 코드 중복, `WxAnimNotifyState_CameraMove`의 적 몽타주 로컬 뷰 전환, 히트스톱·타격 큐의 비예측 방침, `WxAnimNotify_AreaDamage`의 비권위 타겟팅 실행, 노티파이 종료의 클래스 단위 스택 제거는 직전 리뷰와 같은 근거(결정 기록·주석 명시·결과 폐기)로 올리지 않았다.

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 187파일 — `/module-review`로 갱신*
