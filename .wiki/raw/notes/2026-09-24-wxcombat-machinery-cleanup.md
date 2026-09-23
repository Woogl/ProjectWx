---
title: "WxCombat 불필요한 장치 정리: 쿨다운 무시 순정화·사망 BT 정지 단일화·락온 대상 사본 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, combat, ai, architecture]
summary: "소환물의 쿨다운 무시를 순정 Immunity·RemoveOther GE(UWxEffect_IgnoreCooldowns)로 바꾸고 NoCooldown·AbilityBase 태그 분기를 지웠다. 사망 BT 정지는 AI 컨트롤러만 하고, 락온 태스크는 UWxLockOnComponent의 대상을 매 틱 읽는다. 쓰지 않는 SendGameplayEvent 노티파이·InfiniteMP GE와 ASC 발동 실패 로그를 지웠다."
---

# WxCombat 불필요한 장치 정리

2026-09-24 작업 트리(미커밋). WxCombat 전체를 "불필요한 기계장치" 관점으로 점검한 뒤 사용자가 고른 항목을 고쳤다. WxEditor Development 빌드가 통과했고, 새 빌드에서 `ABS_Minion`·`ABS_Doppelganger`를 다시 읽어 확인했다. 인게임은 미검증.

## 쿨다운 무시는 순정 GE 컴포넌트로

- 문제: 쿨다운 무시가 두 벌이었다. `UWxEffect_IgnoreCooldowns`(태그 + `UWxAbilityBase::CheckCooldown`·`ApplyCooldown` 분기)는 어디서도 걸리지 않았다. 소환물 AbilitySet에는 `UWxEffect_NoCooldown`이 들어가 있었다.
- NoCooldown의 지속시간은 `SetByCaller.Duration`인데 `UWxAbilitySet::GiveToAbilitySystem`은 SetByCaller를 채우지 않는다. 엔진은 미설정 SetByCaller 지속시간을 경고 없이 1초로 둔다(`GameplayEffect.cpp` `AttemptCalculateDurationRelatedMagnitude`의 기본값 1.f). 그래서 소환물의 쿨다운 무시가 스폰 후 1초만 유지됐을 것으로 본다(플레이 미검증).
- 사용자 결정: "IgnoreCooldowns으로 바꿔넣어주세요. NoCooldown는 제거합니다." "IgnoreCooldowns 는 엔진 기본 GE 컴포넌트 방식으로 동작해야합니다."
- 현재: `UWxEffect_IgnoreCooldowns`는 Infinite GE다. `UImmunityGameplayEffectComponent`와 `URemoveOtherGameplayEffectComponent`가 `Cooldown` 부모 태그를 부여하는 GE를 막고 걷는다. EffectDefinition 쿼리는 CDO 정확 일치라 어빌리티별 파생 쿨다운 GE를 놓치므로 부여 태그로 잡는다. 두 AbilitySet의 NoCooldown 자리를 IgnoreCooldowns로 바꿨다.
- 지운 것: `UWxEffect_NoCooldown`, `Effect.IgnoreCooldowns` 태그, AbilityBase의 쿨다운 분기와 `ApplyCooldown` 오버라이드.
- 코스트 무시(`UWxEffect_IgnoreCosts`)는 태그와 AbilityBase 판정을 유지한다. 순정 CheckCost는 면역이 아니라 어트리뷰트를 본다.

## 사망 시 BT 정지는 AI 컨트롤러가 한다

- `UWxAbility_Death`와 `AWxAIController::HandlePawnDeath`가 둘 다 `StopLogic`을 불렀다.
- `Ability.Death`는 PreActivate에서 붙고(`GameplayAbility.cpp` PreActivate → ActivateAbility 순서) 태그 통지는 동기다. 그래서 `AWxCharacterBase::HandleDeathTagChanged` → `OnDeath` → 컨트롤러 정지가 어빌리티 본문보다 먼저 끝난다. 모든 AI 폰은 `AWxEnemyCharacter`의 `AIControllerClass`로 `AWxAIController`에 빙의된다.
- 사망 어빌리티의 호출을 지웠다. 그로기·돌진의 BT 일시정지는 모듈 리뷰 항목으로 남아 있다.

## 락온 대상은 UWxLockOnComponent 하나가 든다

- 카메라·회전 태스크가 대상 사본을 들고 변경 통지로 맞추고, 대상 액터 파괴 이벤트까지 따로 구독했다. 어빌리티는 대상이 바뀌면 회전 태스크를 재시작했다.
- 현재: 두 태스크가 매 틱 `UWxLockOnComponent::GetLockOnTarget()`을 읽는다. 파괴된 대상은 이 함수가 `IsValid`로 걸러 널로 답하므로 파괴 구독이 필요 없다. 어빌리티는 컴포넌트에만 쓰고, 도달하지 않던 폴백과 `LockOnTask` 멤버를 지웠다.
- `bOrientRotationToMovement`는 저장값 대신 무브먼트 아키타입에서 되돌린다(WxAI `UWxBTService_LockOn`과 같은 규약).
- `OnLockOnTargetChanged`는 `AWxEnemyCharacter`가 쓰므로 유지한다.

## 발동 실패 로그

- `UWxAbilitySystemComponent::NotifyAbilityFailed` 로그 오버라이드를 지웠다. 엔진이 자기 판정(태그·쿨다운·코스트 등)의 실패 사유를 `LogAbilitySystem` Verbose로 이미 남긴다.
- Wx 고유 실패(발동 그룹 점유·가드 입력·질주 SP·컷신 사용 중)에 엔진식 로그를 넣으려 했지만 되돌렸다.
  - `FScopedCanActivateAbilityLogEnabler`의 카운터가 GAS 모듈 밖으로 export되지 않아 LNK2001이 난다.
  - 게이트 없이 남기면 `WxViewModel_Ability`·`WxInteractionScannerComponent`가 `CanActivateAbility`를 조회할 때마다 로그가 쏟아진다.
  - 그래서 이 실패들은 로그가 없다. 지운 오버라이드도 이 경우 빈 사유만 찍었다.

## 그 밖의 정리

- 지운 클래스: 몽타주에서 쓰이지 않던 `UWxAnimNotify_SendGameplayEvent`(그것만 쓰던 `OtherAnimNotifyColor` 설정 포함), 사용처가 없던 `UWxEffect_InfiniteMP`.
- 설정 이름: 사용자 정정으로 `PresentationAnimNotifyColor`를 `CosmeticAnimNotifyColor`로 바꿨다.
- 질주의 SP 소모 GE는 `ActivationOwnedEffects`로 수명을 맡긴다.
- 히트스톱의 기준 배율은 저장값 대신 클래스 기본값에서 읽는다. `CustomTimeDilation`을 쓰는 곳이 히트스톱 컴포넌트뿐이다.
- `UWxAbilityTask_SlowTime`은 틱으로 시간을 재는 대신 순정 `UAbilityTask_WaitDelay`처럼 게임 시계 타이머로 끝낸다.
- 락온 후보 수집은 순정 `UTargetingSubsystem::GetTargetingResultsActors`를 쓴다.

근거: [IgnoreCooldowns](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_IgnoreCooldowns.cpp), [AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp), [사망 어빌리티](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp), [AI 컨트롤러](../../../Source/WxGame/Controller/WxAIController.cpp), [락온 어빌리티](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp), [락온 카메라 태스크](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp), [ASC](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp).
