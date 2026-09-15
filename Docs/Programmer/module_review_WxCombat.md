# WxCombat — 코드 리뷰

> 직전 리뷰 뒤 들어온 변경 가운데 퍼펙트 가드 주석 정정은 맞게 들어가 직전 6번이 해소됐다. 반면 그로기 종료를 드레인 GE 만료로 옮긴 변경은 처형이 기대던 그로기 해제(ResetGP)를 끊었다. 히트스톱의 CustomTimeDilation 전환은 직전 커밋이 일부러 피했던 "원격 클라 폰의 서버 사본 정지"를 되살렸다. 나머지는 직전 리뷰에서 넘어온 비권위 머신 처리, BT 일시정지 공유, 미사용 선언이다.
> 이번 리뷰는 직전 리뷰(`7d1d0374`) 이후 바뀐 소스 8파일을 diff로 읽고, 작업 기록 3건과 설치된 UE 5.8 엔진 소스에 대조했다. 직전 발견 7건은 현재 코드로 다시 검증했고, 187파일 규칙 스캔을 다시 돌렸다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 3 |
| 🟡 개선 | 1 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 그로기가 드레인 GE 만료로만 끝나게 바뀌어, 처형 뒤 적용하는 ResetGP가 더는 그로기를 풀지 못한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:96-109`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:116-130`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_ResetGP.h:10`
- **범주**: 버그/정확성
- **문제**: 4b5b7e95e가 GP 변경 구독(`HandleGPChanged`, GP ≤ 0이면 종료)을 지웠다. 이제 정상 종료는 드레인 GE의 자연 만료(98행이 조기 제거를 거른다)뿐이고, 예외 종료는 사망 폴링(114행)뿐이다.
  - 그런데 앞잡은 종료 때 대상에 `UWxEffect_ResetGP`(Instant, GP Override 0)를 걸어 그로기를 푼다(`WxAbility_Finisher.cpp:116-130`, 헤더 35행). `WxEffect_ResetGP.h:10`도 "GP 변경을 구독하는 그로기 어빌리티가 스스로 종료한다"를 전제로 적었다. ResetGP는 드레인 GE를 걷지 않으므로 이 경로가 조용히 죽었다.
  - 결과적으로 처형을 버틴 적은 남은 그로기 시간 동안 `Ability.Groggy`를 유지한다. BT 정지도 이어진다(66행). 피해자 몽타주(`UWxAbility_PlayMontageOnce`)가 끝나면 폴링(121-126행)이 그로기 몽타주를 처음부터 다시 튼다.
  - `AWxEnemyCharacter::CanInteract`(`Source/WxGame/Character/WxEnemyCharacter.cpp:105-113`)는 `Ability.PlayMontageOnce`가 빠지고 `Ability.Groggy`가 남으면 참이다. 따라서 같은 그로기 창에서 앞잡을 반복할 수 있다(C++ 쪽 처형 쿨다운 없음).
  - 그 동안 피격 GP 누적도 막힌다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:207-210`).
  - 옛 종료 방식을 전제한 주석이 더 있다: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:126`("어빌리티가 GP를 직접 보고 스스로 끝낸다"), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainGP.cpp:54`.
- **제안**: GP가 0에 닿는 조건을 드레인 자연 만료와 함께 종료 조건으로 되살린다(먼저 오는 쪽이 끝낸다). 만료 경로의 GP 0 정리(106행)는 그대로 둔다. 드레인 마지막 틱이 한 주기 먼저 0을 만들면 종료가 그만큼 당겨질 뿐이다.
  - "만료만 종료" 설계를 유지하려면 처형이 그로기를 직접 끝내는 경로를 따로 만들어야 한다. 이때 두 가지 제약이 있다. Override라 태그 취소가 먹지 않는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:204-207`). 드레인 GE를 걷어도 98행 조기 제거 필터에 걸린다.
  - 어느 쪽이든 위 주석 3곳을 함께 맞춘다.
- **확신도**: 높음(코드 경로 확인. 4b5b7e95e 작업 기록도 원래 증상의 원인을 입증하지 못했다고 적었다)

### 2. 🔴 히트스톱 배율이 원격 클라 폰의 서버 사본에도 걸려, 서버가 아직 느려지지 않은 클라 무브를 잘라 보정이 난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp:55-72`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`
- **범주**: 버그/정확성
- **문제**: 히트스톱 GE는 서버에서 예측 없이 건다(`WxEffect_HitStop.cpp:26`). 그래서 서버 사본은 적용 즉시, 소유 클라는 태그 복제가 도착한 뒤에 배율이 걸린다. 컴포넌트는 머신·역할을 가리지 않고 `CustomTimeDilation`을 0.001배로 낮춘다(64-68행).
  - 원격 클라 폰의 서버 사본은 이동과 포즈를 클라 무브로만 진행한다(엔진 `Engine/Source/Runtime/Engine/Private/Character.cpp:2451`의 `bOnlyAllowAutonomousTickPose`).
  - 서버는 무브 델타를 `1.75 × MaxMoveDeltaTime × 액터 배율`로 자른다(엔진 `Engine/Source/Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:12637-12642`, 호출 `:10022`, 스칼라 기본값 `:226`). 배율이 걸린 서버 사본에서는 이 한도가 약 0.22ms다. 아직 배율이 없는 클라의 16ms 무브가 그만큼으로 잘리고, 타임스탬프는 그대로 넘어가 나머지 시간이 버려진다(`:10029`).
  - 이 창의 길이는 대략 min(RTT, 히트스톱 길이)다. 루트모션 공격 중이면 서버 몽타주 위치가 그만큼 뒤처져 `ClientAdjustRootMotionPosition`으로 클라 몽타주가 되감긴다(`:11131-11138`). 일반 이동이면 위치 보정이 온다. 서버의 무기 판정 노티파이도 같은 만큼 늦는다.
  - 공격자·피격자 양쪽에 걸리므로, 원격 클라 플레이어는 적중마다 이 창을 겪는다.
  - 직전 커밋 b5f00cfc는 바로 이 이유로 서버 사본에는 정지를 걸지 않았다(`.claude/worklog/2026-09-15-히트스톱-시간소유-머신-정지.md` 「접근 방식」). dc5589cae 작업 기록(`.codex/worklog/2026-09-16-HitStop-CustomTimeDilation.md`)은 서버 델타 한도가 액터 배율을 쓴다는 점을 확인했지만, 2인 PIE는 하지 않았다.
  - 스탠드얼론, 리슨 호스트 자기 폰, 서버 AI는 시간을 스스로 굴려 드러나지 않는다.
- **제안**: 권위 머신의 원격 조종 폰(메시 `bOnlyAllowAutonomousTickPose`, b5f00cfc가 쓰던 판정)에는 배율을 걸지 않는다. 그러면 서버 사본은 클라가 이미 느려진 델타로 보낸 무브를 그대로 재생해, 두 머신이 같은 시간을 쓴다.
- **확신도**: 중간(엔진 서버 무브 경로는 소스로 확인했다. 네트워크 PIE 실측은 없다)

### 3. 🔴 컷신 태스크가 비권위 머신에서도 요청 배율 기준 1000배속으로 재생해, 원격 소유 클라에서는 궁극기 컷신이 첫 틱에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117-120`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:58-64`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:54`
- **범주**: 버그/정확성
- **문제**: 월드 배율은 권위에서만 건다(58-64행). 클라이언트에는 `AWorldSettings::TimeDilation` 복제로 한두 넷 업데이트 늦게 도착한다(엔진 `Engine/Source/Runtime/Engine/Classes/GameFramework/WorldSettings.h:741-742`). 그런데 재생 속도 보정(117-120행)은 머신을 가리지 않고 요청값 0.001을 기준으로 1000배를 건다.
  - 시퀀스는 월드 게임 시간의 차분으로 진행한다(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:303`, `:340-343`). 그래서 배율이 아직 1인 소유 클라에서는 한 프레임(약 16ms)에 16초가 흘러 곧바로 `OnFinished`에 닿는다.
  - 궁극기는 실행 정책을 바꾸지 않아 베이스 기본값 LocalPredicted다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:19`). 콘텐츠의 `GA_Template_Ultimate`·`GA_HGTest_Ultimate`도 정책을 재지정하지 않고 `CutsceneSequence`를 저작했다. 따라서 소유 클라가 서버보다 먼저 발동하고, 이 창은 사실상 늘 열린다.
  - 그러면 `HandleCutsceneCompleted`(`WxAbility_Ultimate.cpp:64-77`)가 서버보다 컷신 길이만큼 먼저 `UltimateMontage`를 틀고, 뒤이어 도착한 0.001 배율이 그 몽타주를 세운다. 태스크 `OnDestroy`(`WxAbilityTask_PlaySkillCutscene.cpp:22-28`)도 무적 GE를 로컬에서 먼저 걷는다.
  - 원격 플레이어는 컷신을 보지 못하고, 몽타주·노티파이 타이밍이 서버와 어긋난다. 스탠드얼론과 리슨 서버 호스트는 배율이 같은 프레임에 걸려 드러나지 않는다.
- **제안**: 보정값을 요청값이 아니라 그 머신에 실제로 걸린 배율(`UGameplayStatics::GetGlobalTimeDilation`)에서 구하고, 배율이 바뀌면 다시 맞춘다. 또는 시퀀스를 월드 배율과 무관한 클럭(`EUpdateClockSource::Platform`, 엔진 `Engine/Source/Runtime/MovieScene/Public/MovieSceneFwd.h:68-74`)으로 돌려 보정 자체를 없앤다.
- **확신도**: 중간(엔진 틱 경로와 콘텐츠 정책은 확인했다. 네트워크 PIE 실측은 없다)

### 4. 🟡 돌진 modifier와 그로기가 사유 수 없는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 소환물이 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:66`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:188-206`
- **범주**: 설계/구조
- **문제**: `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `Engine/Source/Runtime/AIModule/Private/BehaviorTree/BehaviorTreeComponent.cpp:173-176`, `:187-194`). 돌진은 자기가 멈춘 두뇌를 기억했다가, 해제 때 `IsPaused()`이면 무조건 재개한다(179-183행).
  - 실패 순서는 이렇다. 돌진 중 GP가 차서 그로기가 뜨면, 그로기의 `Ability` 취소 지목(`WxAbility_Groggy.cpp:32`)이 돌진 스킬을 끝내 몽타주를 블렌드아웃시킨다. 그로기가 두뇌를 멈추지만(66행) 이미 멈춰 있어 아무 일도 없다.
  - 블렌드아웃이 끝나 몽타주 인스턴스가 종료되면, 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부른다(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1759-1768`). 이 호출이 `UWxAnimNotifyState_Rush::BranchingPointNotifyEnd`(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`) → `CancelRush` → `ReleaseState`로 이어져, 그로기가 건 정지까지 푼다.
  - 그 뒤 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다. 콘텐츠 문자열 검색상 돌진 노티파이는 `AM_Minion_Skill_2`에 있고, `BP_Minion`은 그로기를 담은 `ABS_Shared_Enemy`를 받는다.
- **제안**: 재개 여부를 기억한 포인터가 아니라 현재 상태에서 판정한다. 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(`WxAbility_Groggy.cpp:77`)가 재개를 맡는다. 두 곳이 "지금 멈춰 있어야 하는가" 판정 하나를 공유하면, 일시정지 주체가 늘어도 같은 충돌이 생기지 않는다.
- **확신도**: 중간(엔진 경로는 소스로 확인했다. 돌진 몽타주의 블렌드아웃이 0이면 해제가 그로기 일시정지보다 먼저 와 드러나지 않고, 발현은 소환물 MaxGP 저작에 달렸다)

### 5. 🟢 그로기 몽타주의 자동 블렌드아웃과 GE 만료 사이 창에서, 폴링이 그로기 몽타주를 처음부터 다시 튼다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:121-126`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:153-163`
- **범주**: 버그/정확성
- **문제**: 드레인 GE 길이는 `GroggyMontage->GetPlayLength()`(153행)이고, 종료는 그 만료다(96-109행).
  - 엔진은 마지막 섹션의 남은 재생 시간이 블렌드아웃 트리거 시간(기본 0.25초) 이하가 되면 몽타주를 멈추고 즉시 활성 목록에서 뺀다(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:2619-2636`, `Engine/Source/Runtime/Engine/Private/Animation/AnimInstance.cpp:3695-3710`). 그래서 GE 만료 전 약 0.25초 동안 `GetCurrentMontage()`가 null이다(엔진 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:3772-3781`).
  - 0.1초 폴링이 이 창에서 126행으로 그로기 몽타주를 처음부터 다시 틀고, 곧 만료가 `StopMontageIfCurrent`(86행)로 멈춘다. 끝 무렵 자세가 그로기 시작 포즈 쪽으로 끌렸다가 풀린다. 서버 재생이라 복제로 모든 머신에 보인다.
  - `AM_Shared_Groggy`는 uasset 문자열 검색상 블렌드아웃 값을 재지정하지 않았다(기본값).
  - 변경 전 GP 0 종료도 드레인 마지막 틱(재생 길이 이내 한 주기)에 났으므로, 같은 창이 있었다.
- **제안**: 폴링이 "그로기 몽타주가 스스로 블렌드아웃 중"인 경우를 건너뛰게 한다. 예를 들어 `GetInstanceForMontage(GroggyMontage)`가 남아 있고 중단되지 않았으면 재생하지 않는다. 또는 그로기 몽타주의 `bEnableAutoBlendOut`을 꺼 GE 만료의 정지가 끝을 맡게 한다.
- **확신도**: 중간(엔진 경로는 확인했다. 체감은 몽타주 첫·끝 포즈 차이에 달렸다)

### 6. 🟢 비권위 머신의 종료 경로 GE 제거는 권위 게이트에 막혀 경고만 남기고, 컷신 태스크만 게이트를 우회해 복제 GE를 먼저 걷는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:225-232`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:242-250`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본값이 0이다(엔진 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemPrivate.h:21`, 프로젝트 ini 재정의 없음). 그래서 비권위 머신의 `RemoveActiveGameplayEffect`는 핸들 유효성과 무관하게 Warning을 찍고 false를 돌려준다(엔진 `.../Private/AbilitySystemComponent.cpp:1249-1260`).
  - `ActivationOwnedEffectHandles`는 적용에 실패한 빈 핸들까지 담는다(230행). 비권위에서 예측 키가 없으면 엔진이 빈 핸들을 준다(엔진 `.../Private/Abilities/GameplayAbility.cpp:2040-2053`). 종료 때는 머신 구분 없이 제거를 부른다(245-248행).
  - 그래서 소유 클라에서 궁극기(`WxAbility_Ultimate.cpp:20`)와 처형(`WxAbility_Finisher.cpp:32`, ServerInitiated라 클라 핸들이 늘 비어 있다)이 끝날 때마다 효과 수만큼 경고가 난다. 질주의 속도·드레인 제거도 같다.
  - 실제 정리는 예측본은 키 확인이, 서버본은 복제가 맡으므로 동작은 맞다. 다만 242행 주석("효과가 새지 않는다")은 클라 제거가 동작하는 것처럼 읽힌다.
  - 반대로 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 권위 검사 없이 컨테이너를 직접 부른다(엔진 `.../Private/AbilitySystemComponent.cpp:1292-1318`, 게이트가 있는 `RemoveActiveEffects`는 `:1832-1840`). 그래서 소유 클라가 복제된 무적 GE를 서버보다 먼저 로컬에서 걷는다. 3번과 겹치면 컷신 길이만큼 이르다.
- **제안**: 제거 호출은 권위에서만 하고, 비권위에서는 핸들만 비운다(빈 핸들은 애초에 담지 않는다). 컷신 태스크의 정의 기반 제거도 권위로 한정한다. `WxAbilityBase.cpp` 242행과 `WxAbilityTask_PlaySkillCutscene.cpp` 26행 주석은 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 7. 🟢 무기 판정이 소유 클라·시뮬 프록시에서도 콜리전과 매 틱 스윕을 켜지만, 그 결과를 쓰는 곳이 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:50-78`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:140-189`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12-26`
- **범주**: 성능/안전
- **문제**: 공격 노티파이는 몽타주가 도는 모든 머신에서 `BeginAttack`을 부른다. `BeginAttack`은 권위를 가리지 않고 형상 콜리전을 켜고 액터 틱을 연다(64-75행). 그 뒤 매 틱 형상별 `SweepMultiByChannel`(180행)과 오버랩 이벤트가 `ProcessHit`로 모인다.
  - 하지만 비권위 머신에서는 `ApplyDamage`가 권위 검사에서 곧바로 false를 돌려주고(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:45`), 히트스톱도 권위에서만 건다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`).
  - 즉 클라이언트의 스윕·오버랩은 소비처가 없는 비용이다. 관련성 범위 안에서 동시에 공격하는 적 수만큼 모든 클라에 곱해진다.
- **제안**: `BeginAttack` 첫머리에서 소유 액터가 권위가 아니면 반환한다. 그러면 `ActiveAttackCount`가 0으로 남아, `EndAttack`과 사망 시의 `CancelAttack`(`Source/WxGame/Character/WxCharacterBase.cpp:264`)은 그대로 아무 일도 하지 않는다.
- **확신도**: 중간(소비처가 없다는 것은 코드로 확인했다. 실제 비용은 동시 교전 규모에 달렸다)

### 8. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 새 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21-22`). 그런데 `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 둔다.
  - `OnGiveAbility` 진단(`WxAbilityBase.cpp:304-309`)은 "쿨다운 태그 없음"만 잡으므로 이 누락을 통과시킨다.
  - 현재 콘텐츠의 `GA_HGTest_Skill_2`·`GA_Minion_Skill_2`는 `CooldownGameplayEffectClass`를 재지정해 두어(uasset 문자열 검색) 지금은 드러나지 않는다. 새로 만드는 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나로 서로 막히는 잠복 함정이다.
  - 같은 생성자의 애셋 태그 기본값(10-15행)은 빠뜨려도 안전한 방향이지만, 쿨다운 기본값은 반대 방향이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌리면 기존 Error 진단이 누락을 잡는다. 이때 기본값에 기대는 슬롯 1 BP(`GA_Template_Skill`·`GA_HGTest_Skill_1`·`GA_Minion_Skill_1`, 재지정 문자열 없음)에는 `UWxEffect_Cooldown_Skill_1`을 명시 지정해야 쿨다운이 사라지지 않는다.
- **확신도**: 중간(발현은 이후 슬롯 BP 저작에 달렸다)

### 9. 🟢 호출자 없는 선언 2건 — 그중 `EWxAbilityActivationPolicy::OnGiven`은 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:311-317`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h:49`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:52-55`
- **범주**: 중복/복잡도
- **문제**:
  - `OnGiven`은 C++ 어디서도 고르지 않고, 콘텐츠 uasset 문자열 검색에도 저장된 값이 없다. 그런데 `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(엔진 `.../Private/GameplayAbilityTypes.cpp:295`), 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다. LocalPredicted 기본값에서는 클라 예측 발동과 서버발 활성이 겹친다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
  - `UWxAbility_Finisher::IsBackstab()`은 public이지만 UFUNCTION이 아니고 저장소 전체에 호출자가 없다. 내부 코드는 `bBackstab`을 직접 읽는다.
- **제안**: 둘 다 걷는다. `OnGiven`을 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(`OnGiven` 경합 양상)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Groggy.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxHitStopComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_HitStop.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainGP.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_ResetGP.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, 대응 Public 헤더(변경된 헤더 6개 포함)
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`(GP·반사 분기), `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`·`WxAbilityTask_LockOnCamera.cpp`·`WxAbilityTask_RotateToTarget.cpp`(시간 원천), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`(시간 원천), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingPreview.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_FullHP.cpp`·`WxEffect_MoveSpeedScale.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatModule.cpp`, 교차 확인용 `Source/WxGame/Character/WxEnemyCharacter.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**:
  - 직전 리뷰에서 전반을 읽은 나머지 파일(어빌리티·이펙트·큐·노티파이·타겟팅·소환)은 기준 커밋까지 바뀌지 않아 다시 통독하지 않았다.
  - 규칙 스캔(187파일 기계 검사) 결과:
    - 첫 줄 Copyright 누락 0건
    - `FORCEINLINE`·`inline`·람다 0건
    - Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include` 모두 `WxCore`뿐이다.
  - 작업트리에 커밋되지 않은 `Plugins/WxCombat/Source/WxCombat/Public/Cutscene/`·`Private/Cutscene/` 4파일(다른 세션이 진행 중인 공용 스킬 컷신 작업, `.codex/worklog/2026-09-16-공용-스킬-컷신.md`)이 있다. 기준 커밋에 없어 검토와 파일 수에서 뺐다. 이 작업이 3·6번의 컷신 태스크를 대체할 수 있다.
  - 멀티플레이 실측이 없다. 2·3·4·6·9번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이며, PIE 재현은 하지 않았다. 문서의 엔진 경로는 이 엔진 루트 기준이다.
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열 검색은 보조 근거로만 썼다(`AM_Shared_Groggy` 블렌드아웃 기본값, 궁극기 정책, 돌진 노티파이·그로기 배치, 슬롯 BP 쿨다운 재지정, `OnGiven` 저장값 없음).
  - 직전 리뷰(`7d1d0374`) 대비 해소 확인:
    - **6번(가드·퍼펙트 가드 주석 주체)**: `WxAbility_Guard.h:15`·`:22`와 `WxCueNotify_PerfectGuard.h:12`가 `UWxEffectComponent_Hit`으로 정정됐다. 새로 고친 `WxCombatAttributeSet.h:123` 주석도 실제 흐름(DamageResponse가 컨텍스트에 옮기고 Hit이 처리)과 맞다.
  - 변경분 검증 요점(결함 없음):
    - 그로기 만료 콜백의 재진입은 안전하다. 만료 브로드캐스트 중 `EndAbility`→`StopGroggyDrain`이 같은 핸들을 다시 제거해도, 엔진이 제거 대기 효과를 건너뛴다(엔진 `.../Private/GameplayEffect.cpp:4744-4759`). 자연 만료는 `bPrematureRemoval=false`로 들어온다(`:5462`).
    - 비권위 머신의 그로기 종료는 드레인 핸들이 비어 있어 제거를 부르지 않으므로, 6번 경고 대상이 아니다.
    - `CustomTimeDilation`을 쓰는 코드는 `UWxHitStopComponent`뿐이다(저장소 grep). 저장·복원이 다른 쓰기와 겹치지 않는다. GE 수명은 월드 타이머라 액터 배율과 무관하고(`GameplayEffect.cpp:4484`), 입력 버퍼는 실시간(`WxInputBufferComponent.cpp:74`, `:114`)을 써서 영향이 없다.
  - 올리지 않은 항목:
    - 히트스톱 동안 캐릭터 ASC의 틱 태스크(`WxAbilityTask_LockOnCamera`의 보간 등)도 0.001배로 느려진다. dc5589cae 작업 기록이 "액터의 다른 Tick 컴포넌트도 함께 느려진다"로 받아들인 결과이고, 0.1초 수준이라 올리지 않았다.
    - 직전 리뷰의 "올리지 않은 항목"은 같은 근거로 유지한다. 콤보 창의 태그 요건 면제, Attack·Skill·Pattern 콤보 코드 중복, `WxAnimNotifyState_CameraMove`의 적 몽타주 로컬 뷰 전환, 히트스톱·타격 큐의 비예측 방침, `WxAnimNotify_AreaDamage`의 비권위 타겟팅 실행, 노티파이 종료의 클래스 단위 스택 제거, `AWxProjectileBase::Reflect` 역참조, 사망 해제 대기 오차가 해당한다.

---
*문서 기준 커밋 `e0106372a` · 리뷰일 2026-09-16 · 소스 187파일 — `/module-review`로 갱신*
