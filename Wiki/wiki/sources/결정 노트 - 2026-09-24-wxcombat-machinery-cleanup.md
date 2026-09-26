---
type: source
title: "결정 노트 - 2026-09-24-wxcombat-machinery-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "전투"
  - "GAS"
  - "AI"
summary: "소환물 쿨다운 무시를 순정 GE 컴포넌트로 바꾸고 사망 BT 정지를 AI 컨트롤러로 단일화하며 락온 대상 사본을 없앤 WxCombat 장치 정리 기록"
source_type: decision-note
source_id: src-f065af6c9d30c9f6d88f
sha256: 5b170df115d5c1ae53a3e41757022e59a6545aaba8cfb473f85312e0947daf4e
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-wxcombat-machinery-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-wxcombat-machinery-cleanup.md"
raw_copy: ".raw/captured/5b170df115d5c1ae53a3e41757022e59a6545aaba8cfb473f85312e0947daf4e.md"
claim_ids:
  - clm-9da17d1f4b-c1
  - clm-9da17d1f4b-c2
  - clm-9da17d1f4b-c3
  - clm-9da17d1f4b-c4
key_claims:
  - "2026-09-24 사용자는 UWxEffect_NoCooldown을 제거하고 IgnoreCooldowns가 엔진 기본 GE 컴포넌트 방식으로 동작하도록 결정했다."
  - "노트는 SetByCaller 미설정 때문에 이전 소환물의 쿨다운 무시가 스폰 후 1초만 유지됐을 것으로 추정하지만 플레이로 확인하지 않았다."
  - "이 정리 이후 사망 시 BT 정지는 AWxAIController::HandlePawnDeath만 수행하고 UWxAbility_Death의 StopLogic 호출은 삭제됐다."
  - "이 정리 이후 락온 카메라·회전 태스크는 대상 사본 없이 매 틱 UWxLockOnComponent::GetLockOnTarget()을 읽는다."
---

# 결정 노트 - 2026-09-24-wxcombat-machinery-cleanup

- 원본: `.wiki/raw/notes/2026-09-24-wxcombat-machinery-cleanup.md`
- 원자료 사본: `.raw/captured/5b170df115d5c1ae53a3e41757022e59a6545aaba8cfb473f85312e0947daf4e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-24. 2026-09-24 작업 트리(미커밋) 기록이다.
- WxCombat 전체를 "불필요한 기계장치" 관점으로 점검한 뒤 사용자가 고른 항목을 고쳤다.
- 검증: WxEditor Development 빌드 통과, 새 빌드에서 `ABS_Minion`·`ABS_Doppelganger`를 다시 읽어 확인. 인게임은 미검증. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 쿨다운 무시는 순정 GE 컴포넌트로

- 문제(구현 관찰): 쿨다운 무시가 두 벌이었다. `UWxEffect_IgnoreCooldowns`(태그 + `UWxAbilityBase` 분기)는 어디서도 걸리지 않았고 소환물 AbilitySet에는 `UWxEffect_NoCooldown`이 들어 있었다.
- NoCooldown 지속시간은 `SetByCaller.Duration`인데 `UWxAbilitySet::GiveToAbilitySystem`은 SetByCaller를 채우지 않고, 엔진은 미설정 값을 경고 없이 1초로 둔다. 그래서 소환물 쿨다운 무시가 스폰 후 1초만 유지됐을 것으로 추정한다(플레이 미검증).

> 사용자 2026-09-24: "IgnoreCooldowns으로 바꿔넣어주세요. NoCooldown는 제거합니다." "IgnoreCooldowns 는 엔진 기본 GE 컴포넌트 방식으로 동작해야합니다."

- 현재(노트 시점): `UWxEffect_IgnoreCooldowns`는 Infinite GE이고 `UImmunityGameplayEffectComponent`·`URemoveOtherGameplayEffectComponent`가 `Cooldown` 부모 태그를 부여하는 GE를 막고 걷는다(EffectDefinition 쿼리는 CDO 정확 일치라 파생 쿨다운 GE를 놓치므로 부여 태그로 잡음). 두 AbilitySet의 NoCooldown 자리를 바꿨다.
- 지운 것: `UWxEffect_NoCooldown`, `Effect.IgnoreCooldowns` 태그, AbilityBase 쿨다운 분기·`ApplyCooldown` 오버라이드. 코스트 무시(`UWxEffect_IgnoreCosts`)는 태그·AbilityBase 판정을 유지한다.

## 사망 시 BT 정지는 AI 컨트롤러가 한다

- `UWxAbility_Death`와 `AWxAIController::HandlePawnDeath`가 둘 다 `StopLogic`을 불렀다. `Ability.Death`는 PreActivate에서 붙고 태그 통지가 동기라 컨트롤러 정지가 어빌리티 본문보다 먼저 끝나므로 사망 어빌리티의 호출을 지웠다.
- 그로기·돌진의 BT 일시정지는 모듈 리뷰 항목으로 남아 있다(미결).

## 락온 대상은 UWxLockOnComponent 하나가 든다

- 이전: 카메라·회전 태스크가 대상 사본과 변경 통지·파괴 구독을 따로 들었다.
- 현재: 두 태스크가 매 틱 `UWxLockOnComponent::GetLockOnTarget()`을 읽고, 파괴된 대상은 `IsValid`로 걸러 널을 받는다. 어빌리티의 폴백과 `LockOnTask` 멤버를 지웠다. `OnLockOnTargetChanged`는 `AWxEnemyCharacter`가 쓰므로 유지.

## 발동 실패 로그

- `UWxAbilitySystemComponent::NotifyAbilityFailed` 로그 오버라이드를 지웠다(엔진이 `LogAbilitySystem` Verbose로 이미 남김).
- Wx 고유 실패(발동 그룹 점유·가드 입력·질주 SP·컷신 사용 중)의 엔진식 로그는 `FScopedCanActivateAbilityLogEnabler` 미export(LNK2001)와 UI 조회 시 로그 폭주 때문에 되돌렸다. 이 실패들은 로그가 없다.

## 그 밖의 정리

- 지운 클래스: `UWxAnimNotify_SendGameplayEvent`, `UWxEffect_InfiniteMP`. 사용자 정정으로 `PresentationAnimNotifyColor`를 `CosmeticAnimNotifyColor`로 바꿨다.
- 질주 SP 소모 GE는 `ActivationOwnedEffects`로 수명을 맡기고, `UWxAbilityTask_SlowTime`은 게임 시계 타이머로 끝내며, 락온 후보 수집은 순정 `UTargetingSubsystem::GetTargetingResultsActors`를 쓴다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[적 AI와 몬스터]]
- [[플레이어 캐릭터와 조작]]

## 핵심 주장

- 2026-09-24 사용자는 UWxEffect_NoCooldown을 제거하고 IgnoreCooldowns가 엔진 기본 GE 컴포넌트 방식으로 동작하도록 결정했다. ^c1
- 노트는 SetByCaller 미설정 때문에 이전 소환물의 쿨다운 무시가 스폰 후 1초만 유지됐을 것으로 추정하지만 플레이로 확인하지 않았다. ^c2
- 이 정리 이후 사망 시 BT 정지는 AWxAIController::HandlePawnDeath만 수행하고 UWxAbility_Death의 StopLogic 호출은 삭제됐다. ^c3
- 이 정리 이후 락온 카메라·회전 태스크는 대상 사본 없이 매 틱 UWxLockOnComponent::GetLockOnTarget()을 읽는다. ^c4
