# WxCombat — 코드 리뷰

> 발동 그룹·액션 페이즈·표 수치·태그 발행이라는 축이 일관되고 수명주기 해제와 권위 경계도 대부분 제자리에 있는 건강한 모듈이다. 다만 노티파이 시점에 활성화 예측 키로 거는 GE가 소유 클라에서 영영 걷히지 않는 멀티플레이 결함이 하나 있다. 이번 리뷰는 소스 181파일을 대상으로 어빌리티·ASC·어트리뷰트·데미지 파이프라인·노티파이·태스크·타겟팅·무기/투사체/소환 cpp까지 읽었고, 예측·몽타주 해제에 관한 판단은 설치된 UE 5.8 GAS 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 노티파이 시점의 예측 GE가 소유 클라에서 제거되지 않아 무적 태그가 영구히 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:28-35`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:160-166`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:86-91`
- **범주**: 버그/정확성
- **문제**:
  - `NotifyBegin`은 `UWxCombatLibrary::ApplyEffect`로 `GetAnimatingAbility()`의 활성화 예측 키를 실어 무한 지속 GE(`UWxEffect_Invincible`·`UWxEffect_PerfectGuard`)를 건다. 클라에서 이 예측본을 걷는 경로는 GAS가 그 키에 묶는 "확인 도착" 델리게이트 하나뿐이다(엔진 `GameplayEffect.cpp:4525-4533`).
  - 서버는 활성화 예측 창을 닫는 순간 그 키를 확인·복제하고(엔진 `GameplayPrediction.cpp:519-528`), 클라는 도착 시 그 키의 델리게이트를 한 번 소비한다(같은 파일 `:614`). 노티파이가 그보다 늦으면(무적 구간이 몽타주 첫 프레임이 아닌 일반적 배치) 뒤늦게 등록한 델리게이트는 다시 불리지 않고, 5.8 기본 `StaleKeyBehavior=2`는 오래된 항목을 실행 없이 버린다(같은 파일 `:27`, `:680`).
  - `NotifyEnd`는 권위에서만 제거하고(28-35행) 서버 제거는 복제본만 지우므로, 원격 클라 플레이어는 첫 회피 이후 `Effect.Invincible`을 계속 들고 있다. 헤더의 "나머지는 복제를 따른다"(`Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.h:17`)는 예측본에는 성립하지 않는다.
  - 증상: 그 클라의 `CheckDamage`가 자기에게 오는 적 타격을 전부 `Evaded`로 보고 로컬에서만 `Event.DodgeSuccess`를 보낸다(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:50`, `:95-103`). 다음 회피에서는 `WaitGameplayTagAdd`가 이미 붙은 태그로 즉시 발화해 판정 캡슐이 회피 시작 지점에 내내 박히므로(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:223-229`), 서버와 다른 극한 회피가 그 클라 화면에만 뜬다.
  - 같은 원인으로 `ApplyDamage`가 같은 키로 대상 ASC에 거는 Instant GE(86-91·128·140행)는 예측 클라에서 무한 지속으로 바뀐 채 적중마다 대상의 활성 GE 배열에 쌓인다. 모디파이어를 가진 Instant 추가 효과라면 그 수치까지 클라 쪽에 누적된다.
  - 스탠드얼론·리슨 서버 호스트는 권위라 드러나지 않는다.
- **제안**: 노티파이발 예측본의 클라 정리 경로를 명시적으로 둔다. `UWxAbilitySystemComponent`에 "이 머신이 예측으로 건 인스턴스(`PredictionKey.IsLocalClientKey() && !PredictionKey.WasReceived()`)를 클래스로 찾아 `RemoveActiveGameplayEffect_AllowClientRemoval`로 걷는" 좁은 함수를 두고 `NotifyEnd`의 비권위 분기에서 부른다. `ApplyDamage`의 대상 예측본은 공격자 화면 큐가 목적이므로 적용 직후 같은 함수로 걷거나, 이미 확인된 키를 싣지 않는 쪽을 택한다.
- **확신도**: 중간(엔진 5.8 소스 경로로 확인했고 PIE "클라이언트로 플레이" 재현은 하지 않았다 — 회피 1회 후 클라 `showdebug abilitysystem`에서 `Effect.Invincible` 잔존 여부로 바로 확인된다)

### 2. 🟡 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:20-22`). `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 두어, 슬롯 2~4 BP가 애셋 태그만 바꾸고 이 값을 놓치면 두 스킬이 `Cooldown.Skill.1` 하나에 서로 막힌다. `OnGiveAbility`의 진단(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:280-285`)은 "쿨다운 태그 없음"만 잡아 이 실수를 통과시킨다. 같은 파일 10-15행의 애셋 태그 기본값은 빠뜨려도 안전한 방향이지만 쿨다운 기본값은 반대 방향이며, `.claude/worklog/2026-09-12-스킬-2-3-4-쿨다운-GE-추가.md`도 "지정 전까지는 넷 모두 슬롯 1 GE를 물려받는다"를 후속 과제로 남겨 두었다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌린다 — 그러면 기존 Error 진단이 슬롯 BP의 지정 누락을 잡는다.
- **확신도**: 중간(실제 슬롯 BP의 지정 여부는 범위 밖)

### 3. 🟡 사망 몽타주가 정상 완료되면 메시 본 갱신 강제가 시체에 영구히 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:66-68`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23-43`
- **범주**: 성능/안전
- **문제**: `UWxAbilitySystemComponent`는 몽타주 재생 시 메시를 `AlwaysTickPoseAndRefreshBones`로 올리고 `AnimatingAbility`가 해제될 때만 되돌린다(23-43·79-110행). 엔진은 이 해제를 어빌리티 종료(엔진 `AbilitySystemComponent_Abilities.cpp:1240-1244`)나 태스크의 인터럽트·블렌드아웃(`bAllowInterruptAfterBlendOut=false`일 때, 엔진 `AbilityTask_PlayMontageAndWait.cpp:33-39`)에서만 한다. 베이스는 그 플래그를 true로 넘기고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:245-246`), `UWxAbility_Death`는 끝나지 않으며 완료 훅도 비어 있다. 그래서 사망 몽타주를 끝까지 재생한 시체는 파괴될 때까지 서버(플레이어 사망이면 소유 클라도)에서 렌더 여부와 무관하게 매 프레임 본을 갱신한다 — `ACharacter` 기본값 `AlwaysTickPose`라면 보이지 않을 때 건너뛸 비용이다. `PendingDestroyTime` 기본값 0은 "파괴하지 않음"이라(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Death.h:45-47`) 오픈월드에서 시체가 쌓일수록 서버 비용이 는다. 래그돌로 떨어지는 인터럽트 경로는 태스크가 해제하므로 해당 없다.
- **제안**: `UWxAbility_Death::HandleMontageCompleted`에서 `ClearAnimatingAbility(this)`를 불러 강제를 푼다(몽타주 자체는 멈추지 않는다).
- **확신도**: 중간(엔진 해제 경로는 소스로 확인, 실제 비용은 시체 수명 저작에 달림)

### 4. 🟢 클라이언트의 종료 경로 GE 제거가 권위 게이트에 막혀 경고만 남긴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:218-225`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:106-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본 0이라 비권위 `RemoveActiveGameplayEffect`가 Warning을 찍고 false를 돌려준다(엔진 `AbilitySystemComponent.cpp:1249-1260`). `UWxAbilityBase::EndAbility`는 `ActivationOwnedEffects` 핸들을, `UWxAbility_Sprint::EndAbility`는 속도 배율 핸들을 머신 구분 없이 제거하므로 소유 클라에서 가드·질주·궁극기·처형이 끝날 때마다 경고가 난다. 실제 정리는 활성화 창 안에서 건 예측본이 키 확인으로, 서버본이 복제로 이뤄져 동작은 맞지만, 218행 주석("효과가 새지 않는다")과 컷신 태스크 26행 주석은 클라 제거가 동작한다고 읽힌다. 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 클라에서 조용히 0을 돌려준다.
- **제안**: 제거 호출은 권위에서만 하고 클라는 핸들만 비운다. 주석은 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 5. 🟢 Pattern이 중간 단 재생 실패로 끝나면 `ComboIndex`가 남아 다음 발동이 앞 단을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:56-59`
- **범주**: 버그/정확성
- **문제**: `HandleMontageBlendOut`은 인덱스를 먼저 올린 뒤 `PlayMontage`가 실패하면 `bWasCancelled=false`로 종료하는데, `EndAbility`는 취소일 때만 인덱스를 되돌린다(38-46행). `ComboMontages`에 빈 슬롯이 있으면 인덱스가 배열 중간에 남아 다음 발동이 그 뒤 단부터 시작한다. 같은 클래스의 `ActivateAbility` 실패 경로(32-35행)와 `UWxAbility_Attack`·`UWxAbility_Skill`은 모두 취소로 끝내 리셋된다.
- **제안**: 56-59행에서 종료 전에 `ComboIndex = INDEX_NONE`을 둔다(인플레이스 한 줄, 콤보 공통화는 하지 않는다).
- **확신도**: 중간(빈 슬롯 저작이 있어야 발현)

### 6. 🟢 `EWxAbilityActivationPolicy::OnGiven`은 사용처가 없고, 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:287-293`
- **범주**: 중복/복잡도
- **문제**: C++과 `Content` 에셋 어디에도 `OnGiven`을 고른 곳이 없다. 게다가 `OnGiveAbility`는 스펙 복제 도착 시 클라에서도 불리므로(엔진 `GameplayAbilityTypes.cpp:295`) 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다 — LocalPredicted 어빌리티라면 클라 예측 발동과 서버발 활성 통지가 겹쳐 경합한다. 쓰지 않는 선택지가 권위 게이트 없이 남아 처음 쓰는 사람이 이 함정을 밟는다.
- **제안**: 선택지를 걷는다. 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(경합 양상)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 파생 어빌리티 15개 전부(Attack·Skill·Pattern·PlayMontageOnce·LockOn·Dodge·Guard·GuardReact·HitReact·Groggy·Death·Finisher·Ultimate·Sprint·Passive), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, 대응 Public 헤더
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatModule.cpp`
- **규칙 스캔(전 파일 기계 검사)**: 첫 줄 `// Copyright Woogle. All Rights Reserved.` 181파일 전부 통과. `FORCEINLINE`·`inline`·헤더 함수 본문·람다 0건. 타입 prefix `Wx` 누락 0건. `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include`의 Wx 참조는 모두 `WxCore`(`WxGameplayTags.h`·`WxUIData.h`·`WxCollisionChannels.h`·`Minion/WxMinion.h`)뿐이다.
- **미검토 / 한계**:
  - 멀티플레이 실측이 없다. 1·3·4·6번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Runtime\GameplayAbilities`) 소스 경로로 확인한 결론이며 PIE 재현은 하지 않았다.
  - BP·DataTable 저작 값(GA_*·GE_*·DT_* 행, 몽타주 노티파이 배치, 시체 수명)은 범위 밖이다. 2·3·5번의 실제 발현 여부는 저작에 달려 있다.
  - 작업 트리의 `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`에 커밋되지 않은 주석 수정이 있어 작업 트리 기준으로 읽었다. 직전 리뷰의 "무기 히트 예측 모델 주석" 항목은 그 수정으로 해소됐다.
  - 직전 리뷰 항목 중 다음은 이번에 올리지 않았다. 콤보 창의 태그 요건 전체 면제는 `.claude/worklog/2026-09-10-콤보-창-재발동-태그-면제.md`가 폭을 알고 택한 결정이다. Attack·Skill·Pattern 콤보 중복은 확정된 감수 사항이라 발견 5번의 인플레이스 리셋만 남겼다. 락온 지점 순서는 C++ 기본 구성이 지점 하나(`Source/WxGame/Character/WxEnemyCharacter.cpp:36`)이고 `GetComponents`가 사실상 삽입 순서라 실해가 없다. 락온 회전 플래그 복원 게이트는 ASC가 무효한 종료 경로가 실질적으로 없다.
  - 직전 리뷰가 확인하지 못한 "`ActivationOwnedTags`가 비권위 머신에 복제되는가"는 해소됐다 — UE 5.8 `GameplayAbilitiesDeveloperSettings::ReplicateActivationOwnedTags` 기본 true, 복제 상태 `CountToOwner`(태그는 전원·카운트는 소유자)라 `UWxMinionSubsystem`의 `Ability.Death` 구독은 모든 머신에서 성립한다.

---
*문서 기준 커밋 `6bde8a033` · 리뷰일 2026-09-13 · 소스 181파일 — `/module-review`로 갱신*
