# WxCombat — 코드 리뷰

> 직전 리뷰의 컷신 🟡 2건(시계 클램프로 월드 고정, 배율 저장값 복원)과 🟢 2건(비권위 GE 제거 경고, 호출자 없는 선언)은 해소됐다. 어빌리티 베이스·대미지 파이프라인·입력 버퍼·소환 로스터도 견고하다. 남은 위험의 중심은 복제 State 하나로 단순화된 컷신 컴포넌트다. "접속 전에 끝난 세션" 입양 규칙이 소유 클라의 대기 태스크까지 삼키는 경로가 있다.
> 이번 리뷰는 직전 리뷰(`4096004a4`) 이후 바뀐 소스를 전부 읽고 설치된 UE 5.8 엔진 소스와 대조했다. 코어 어빌리티·이펙트·무기·돌진·그로기 파일을 다시 읽어 직전 발견을 재검증했고, 187파일 규칙 스캔을 돌렸다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 시작과 종료가 한 번에 도착한 세션은 통지 없이 입양돼, 그 세션을 기다리던 소유 클라의 궁극기가 끝나지 않고 배타 액션을 계속 막는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:179-187`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:200-214`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:22-31`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:82-86`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:97-101`
- **범주**: 버그/정확성
- **문제**:
  - 프로퍼티 복제는 최신 값만 보장한다. 새 `Session.Id`가 `bPlaying=false`로 처음 도착하면 182행이 `bEndNotified`를 true로 두고 "접속 전에 끝난 세션"으로 입양한다. 그래서 이어지는 201-202행의 `NotifyEnded`는 아무 일도 하지 않는다.
  - 늦은 참가자에게는 맞는 처리다. 그러나 소유 클라는 예측 발동에서 이미 태스크를 만들어 구독해 두었다(`WxAbility_Ultimate.cpp:83-86`, 태스크 22행). 이 경로에서는 그 태스크가 통지를 영영 받지 못한다.
  - 서버는 컷신 완료 → 몽타주 → 정상 종료를 복제한다. GAS는 `SetRemoteInstanceHasEnded`를 세운 뒤 `EndAbility(..., false, false)`를 부른다(엔진 `GAS/Private/AbilitySystemComponent_Abilities.cpp:2193`, `:2204`). 궁극기는 `bLocalPresentationPending`(82행)이 서 있어 이 호출을 보류한다(97-101행). 그 뒤로는 로컬 인스턴스를 끝낼 주체가 없다.
  - 결과적으로 소유 클라에서 궁극기가 Exclusive·Blocking 상태로 활성인 채 남는다. `FindActivationGroupBlocker`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:142-143`)가 이후 모든 Exclusive 예측 발동(공격·스킬·회피)을 막는다. `Ability` 전체를 지목해 끊는 사망·그로기(`WxAbility_Death.cpp:28`, `WxAbility_Groggy.cpp:31`)가 오기 전까지 풀리지 않는다.
  - 발생 조건은 소유 클라가 `bPlaying=true` 스냅샷을 한 번도 받지 못한 채 서버가 정상 종료하는 경우다. 즉 클라 쪽 네트워크 정체나 히치가 컷신 길이 이상 이어져야 한다. 드물지만 결과가 소프트락이다.
- **제안**:
  - 입양 분기에서도 종료를 알린다. 182행을 없애고 새 세션이면 `bEndNotified`를 항상 false로 두면, 201-202행이 그 자리에서 알린다. 구독자는 시전자로 거르고, 늦은 참가자에게는 구독자가 없어 무해하다.
  - 한계도 있다. 같은 시전자의 "서버 커밋 실패로 취소된 세션"과 "그 직후 재예측한 발동"이 한 복제 창에 겹치면, 새 태스크가 이전 세션의 취소를 받는다. 결과는 재발동 1회가 취소되는 것이라 소프트락보다 가볍다.
- **확신도**: 중간(코드와 GAS 경로는 확인했다. 발현은 컷신 길이 이상의 클라 정체에 달렸다)

### 2. 🟡 돌진 modifier와 그로기가 사유를 세지 않는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 소환물이 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:31`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:64`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:172-190`
- **범주**: 설계/구조
- **문제**:
  - `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `Engine/Source/Runtime/AIModule/Private/BehaviorTree/BehaviorTreeComponent.cpp:176`, `:191-194`). 돌진은 자기가 멈춘 두뇌를 기억했다가, 해제 때 `IsPaused()`면 무조건 재개한다(179-183행).
  - 돌진 중 GP가 차서 그로기가 뜨면, 그로기의 `Ability` 취소 지목(31행)이 돌진 스킬을 끝내고 몽타주를 블렌드아웃시킨다. 그로기의 정지 요청(64행)은 이미 멈춘 두뇌에 다시 bool을 세울 뿐이다.
  - 블렌드아웃이 끝나 몽타주 인스턴스가 종료되면, 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부른다(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1766-1768`). 이 호출이 `BranchingPointNotifyEnd`(23-30행) → `CancelRush` → `ReleaseState`로 이어져 그로기가 기대던 정지까지 푼다.
  - 그 뒤 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다.
- **제안**: 재개 여부를 기억한 포인터가 아니라 현재 상태로 판정한다. 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(`WxAbility_Groggy.cpp:75`)가 재개를 맡는다.
- **확신도**: 중간(엔진 경로는 소스로 확인했다. 발현은 돌진 몽타주를 쓰는 소환물의 MaxGP 저작에 달렸다)

### 3. 🟢 컷신 시계가 일시정지를 모르고 흘러, 스탠드얼론에서 일시정지 후 재개하면 남은 장면을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:43-58`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:123`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:230`
- **범주**: 버그/정확성
- **문제**:
  - 시계는 `FPlatformTime::Seconds()`로 경과를 잰다(45·57행). 반면 일시정지 중에는 시퀀스 플레이어가 틱되지 않는다. 틱 매니저는 `bTickWhenPaused` 클라이언트만 돌리고(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:380`), 레벨 시퀀스 플레이어는 소유 액터의 `bTickEvenWhenPaused`(기본 false)를 물려받는다(`Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickInterval.cpp:29`, `Engine/Source/Runtime/MovieScene/Public/MovieSceneSequencePlaybackSettings.h:62`).
  - 따라서 재개 후 첫 조회에서 위치가 정지한 시간만큼 앞으로 뛴다. 정지가 남은 길이보다 길면 컷신이 곧바로 끝나고 후속 몽타주로 넘어간다.
  - 프로젝트는 스탠드얼론에서 `ShouldPauseGame` 위젯이 뜨면 게임을 멈춘다(`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:153-168`). 컷신 설정은 이동·시선 입력만 막고 HUD는 숨기지 않는다(271-273행).
- **제안**: 일시정지에는 멈추고 월드 배율은 받지 않는 `UWorld::GetAudioTimeSeconds()`로 시계를 바꾼다(엔진 `Engine/Source/Runtime/Engine/Classes/Engine/World.h:2018-2019`, `Engine/Source/Runtime/Engine/Private/LevelTick.cpp:1586-1590`). 시계를 만들 때(290행) 월드 약참조를 넘기면 된다. 권위 길이 안전망(123·230행)도 같은 시계로 맞춘다.
- **확신도**: 중간(엔진 경로는 확인했다. 컷신 중 일시정지 UI가 열리는지는 BP 입력 설정에 달렸다)

### 4. 🟢 관전 머신은 궁극기 시퀀스를 미리 로드하지 않아, 서버가 월드를 재개한 뒤에도 로드 시간만큼 시네마틱 입력 잠금이 이어진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:26-34`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:254-261`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:271-273`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:401-407`
- **범주**: 설계/구조
- **문제**:
  - 선로드는 `OnGiveAbility`에서만 한다. 어빌리티 스펙은 소유 클라에만 복제되므로(`COND_ReplayOrOwner`, 엔진 `GAS/Private/AbilitySystemComponent.cpp:1861-1862`) 서버와 소유 클라만 시퀀스를 미리 들고 있다. 다른 클라는 State를 받고서야 비동기 로드를 건다(254-261행).
  - 각 머신이 로드 후 처음부터 재생하고, 서버가 자기 기준으로 배율·무적을 복원하는 것은 승인된 방침이다(`.codex/worklog/2026-09-16-컷신-개별-재생.md`). 다만 관전 클라가 늦는 폭은 RTT/2에 로드 시간을 더한 값이 된다.
  - 이 꼬리 동안 관전 머신은 이동·시선 입력이 막혀 있다(271-273행). 그 사이 서버 월드는 이미 1배로 돌아 적이 움직이고, 관전자에게는 무적도 없다. 에디터 PIE는 에셋이 메모리에 있어 드러나지 않는다.
- **제안**: 방침과 충돌하지 않는 쪽은 관전 머신의 로드 시간을 없애는 것이다. 모든 머신에 존재하는 캐릭터 BP나 GameState에 시퀀스 소프트 참조를 두고 미리 로드한다. 쿡 빌드 멀티에서 꼬리 길이를 한 번 재고 판단한다.
- **확신도**: 낮음(의도된 트레이드오프의 연장일 수 있다. 쿡 빌드 로드 시간은 측정하지 않았다)

### 5. 🟢 무기 판정이 소유 클라·시뮬 프록시에서도 콜리전과 매 틱 스윕을 켜지만, 그 결과를 쓰는 곳이 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:50-78`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:140-189`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12-26`
- **범주**: 성능/안전
- **문제**:
  - 공격 노티파이는 몽타주가 도는 모든 머신에서 `BeginAttack`을 부른다. `BeginAttack`은 권위를 가리지 않고 형상 콜리전을 켜고 액터 틱을 연다(64-75행). 이후 매 틱 형상별 `SweepMultiByChannel`(180행)과 오버랩 이벤트가 `ProcessHit`로 모인다.
  - 비권위 머신에서는 `ApplyDamage`가 권위 검사에서 곧바로 false를 돌려주고(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:45`), 히트스톱도 권위에서만 건다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`).
  - 즉 클라의 스윕·오버랩은 소비처 없는 비용이다. 관련성 범위 안에서 동시에 공격하는 적 수만큼 모든 클라에 곱해진다.
- **제안**: `BeginAttack` 첫머리에서 소유 액터가 권위가 아니면 반환한다. 그러면 `ActiveAttackCount`가 0으로 남아, `EndAttack`과 사망 시 `CancelAttack`(`Source/WxGame/Character/WxCharacterBase.cpp:264`)은 그대로 아무 일도 하지 않는다.
- **확신도**: 중간(소비처가 없다는 것은 코드로 확인했다. 실제 비용은 동시 교전 규모에 달렸다)

### 6. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 새 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**:
  - 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21-22`). 그런데 `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 둔다.
  - `OnGiveAbility` 진단(`WxAbilityBase.cpp:310-315`)은 "쿨다운 태그 없음"만 잡으므로 이 누락을 통과시킨다.
  - 현재 `GA_HGTest_Skill_2`·`GA_Minion_Skill_2`·`GA_Doppelganger_Skill_1`·`GA_Doppelganger_Skill_2`는 `CooldownGameplayEffectClass`를 재지정해 두어(uasset 문자열 검색) 드러나지 않는다. 새 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나로 서로 막히는 잠복 함정이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌리면 기존 Error 진단이 누락을 잡는다. 이때 기본값에 기대는 슬롯 1 BP(`GA_Template_Skill`·`GA_HGTest_Skill_1`·`GA_Minion_Skill_1`, 재지정 문자열 없음)에는 `UWxEffect_Cooldown_Skill_1`을 명시 지정해야 쿨다운이 사라지지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음. 같은 생성자가 애셋 태그도 슬롯 1을 안전 기본값으로 깐다고 밝히고 있다(10행). 발현은 이후 슬롯 BP 저작에 달렸다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Public/Cutscene/WxSkillCutsceneComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Ultimate.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotify_SpawnMinion.h`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnMinion.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, 교차 확인용 `Source/WxGame/Character/WxCharacterBase.cpp`·`Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`·`Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, 직전 리뷰 이후 커밋 diff 전체(`4096004a4..5eb1a754`)
- **미검토 / 한계**:
  - 변경이 없는 나머지 파일(Attack·Pattern·Passive·PlayMontageOnce·Guard·GuardReact, 대부분의 Effect·Cue, Targeting 필터·소터, `WxAbilityTask_LockOnCamera.cpp`, `WxProjectileBase.cpp`, `WxLockOnComponent.cpp`, `WxFinisherDamageComponent.cpp`)는 이번에 통독하지 않았다. 직전 리뷰의 통독 결과에 기댔다.
  - 멀티플레이·쿡 빌드 실측이 없다. 엔진 경로는 설치 엔진(`C:\Program Files\Epic Games\UE_5.8`) 루트 기준이고, `GAS/`는 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/`를 줄여 쓴 것이다.
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열 검색은 보조 근거로만 썼다(슬롯 BP 쿨다운 재지정, `State.Minion.Active` 사용처).
  - 규칙 스캔(187파일 기계 검사): 첫 줄 Copyright 누락 0건, `FORCEINLINE`·`inline`·헤더 함수 본문 0건. Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include`(`WxGameplayTags.h`·`WxUIData.h`·`WxCollisionChannels.h`·`Minion/WxMinion.h`) 모두 `WxCore`뿐이다. 새로 추가된 `HairStrandsCore`/`HairStrands`는 엔진 플러그인이다.
  - 직전 리뷰(`4096004a4`) 대비 해소 확인:
    - **1번(시계가 종료 임계에 못 닿음)**: 시계가 상한 없이 경과를 더하고(`WxSkillCutsceneComponent.cpp:56-57`), 권위 길이 안전망이 넷 모드와 무관하게 "시퀀스가 실제로 돌지 않을 때"를 덮는다(228-234행).
    - **2번(배율 저장값 복원)**: 실제로 박은 값이 그대로일 때만 1로 되돌린다(403-407행). `WxAbilityTask_SlowTime.cpp:37-40`과 규칙이 같다.
    - **5번(비권위 GE 제거 경고)**: 제거를 권위에서만 하고 빈 핸들을 담지 않는다(`WxAbilityBase.cpp:229-233`, `:247-255`, `WxAbility_Sprint.cpp:105-117`).
    - **8번(호출자 없는 선언·도달 불가 분기)**: `EWxAbilityActivationPolicy`, `IsBackstab`/`bBackstab` 멤버, `Reserve`·`IsForAvatar`와 준비 마감 대입이 모두 사라졌다.
  - 올리지 않은 항목:
    - 컷신 중 다른 플레이어의 슬로우 타임이 새로 걸리면 `WxAbilityTask_SlowTime.cpp:59`가 0.001을 덮어써 동결이 풀린다. 월드가 0.001배라 새 극한 회피·퍼펙트가드가 성립할 틈이 거의 없고, 끝난 뒤 배율이 갇히지도 않는다.
    - Groom 보정(`WxSkillCutsceneComponent.cpp:368-399`)이 전투 모듈에서 아바타의 Niagara 내부(틱 그룹)까지 만진다. Groom 수명을 이미 아는 `UWxMetaHumanComponent`(WxGame)로 옮기는 안은 작성자 워크로그(`.claude/worklog/2026-09-16-컷신-그룸-시뮬-속도.md`)에 보류로 남아 있다. 배율을 1로 되돌린 뒤 남는 틱 그룹은 배치 모드가 컴포넌트 틱을 끄므로 무해하다(엔진 `Engine/Plugins/FX/Niagara/Source/Niagara/Private/NiagaraComponent.cpp:4733`). 종료 직후 시전자 참조가 널로 재해석되면 복원이 건너뛰어지는 경로도 같은 워크로그에 기록돼 있다.
    - 무적 ANS 종료의 클래스 단위 스택 제거(`WxAnimNotifyState_ApplyGameplayEffect.cpp:35`)가 컷신 무적 GE까지 걷을 수 있다. 회피 i-frame은 후딜 전에 닫혀 궁극기 발동과 겹치기 어렵고, 겹쳐도 그 창은 월드가 0.001배인 동안이라 실효가 거의 없다.
    - 새 컷신 코드의 cpp 정의 순서와 분기용 멤버 플래그(`bLocalPresentationPending`), .cpp 익명 namespace는 `CLAUDE.md`가 아닌 사용자 메모리 규칙이라 발견에서 뺐다.
    - 직전 리뷰의 "올리지 않은 항목"은 같은 근거로 유지한다. 콤보 창의 태그 요건 면제, Attack·Skill·Pattern 콤보 코드 중복, `WxAnimNotifyState_CameraMove`의 적 몽타주 로컬 뷰 전환, 히트스톱·타격 큐의 비예측 방침, `WxAnimNotify_AreaDamage`의 비권위 타겟팅 실행, `AWxProjectileBase::Reflect` 역참조, 사망 해제 대기 오차가 해당한다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 187파일 — `/module-review`로 갱신*
