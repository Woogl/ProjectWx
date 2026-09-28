---
type: source
title: "작업 - wxgame-review-fixes"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "WxGame"
  - "코드-리뷰"
summary: "WxGame 코드 리뷰 지적 세 개(ASC 재등록, 사망·이벤트 구독, 새 게임 검증 순서)를 단순한 방식으로 고치고 레벨 재표시 헤드리스 테스트에서 무기 겹침 구독 중복까지 고친 완료 작업 기록"
source_type: task-record
source_id: src-6b1e68159d9bf1eda424
sha256: e649e119d7dcbd6ba4531128d58e59bdb4b3bf3f0918387696c64698d659ea1d
authority: primary
independence_key: ".agents/workflow/tasks/wxgame-review-fixes.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/wxgame-review-fixes.md"
raw_copy: ".raw/captured/e649e119d7dcbd6ba4531128d58e59bdb4b3bf3f0918387696c64698d659ea1d.md"
claim_ids:
  - clm-e649e119d7-c1
  - clm-e649e119d7-c2
  - clm-e649e119d7-c3
  - clm-e649e119d7-c4
key_claims:
  - "2026-09-25 사용자 요청으로 WxGame 코드 리뷰 지적 세 개를 고쳤고, 사용자 단순화 지시로 부여 상태 enum과 OnRegister·OnUnregister·EndPlay 오버라이드 없이 실제 스펙 보유 여부로 판단한다."
  - "수정 뒤 GiveAbilitySets는 없는 스펙만 채우고 bAbilitySetsInitialized로 속성·GE를 최초 한 번만 적용하며, bDeathHandled·bDeathNotified로 사망 처리와 처치 보상을 객체 수명당 한 번으로 제한하고, 새 게임은 체크포인트 삭제 전에 선택 Pawn 클래스를 검증한다."
  - "2026-09-27 헤드리스 레벨 재표시 테스트에서 AWxWeaponBase::PostInitializeComponents의 무기 겹침 구독 중복 ensure를 찾아 AddUniqueDynamic으로 고쳤고, 재표시 전투와 새 게임 캐릭터 선택이 통과했다."
  - "woogle은 2026-09-28 WxGame 리뷰 수정의 코드 리뷰와 재표시 뒤 연출을 통과시켜 체크리스트 9/9로 완료됐다."
---

# 작업 - wxgame-review-fixes

- 원본: `.agents/workflow/tasks/wxgame-review-fixes.md`
- 원자료 사본: `.raw/captured/e649e119d7dcbd6ba4531128d58e59bdb4b3bf3f0918387696c64698d659ea1d.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

`module_review_WxGame.md`(WxGame 코드 리뷰)의 지적 세 개를 해결한 구현 작업 기록이다. 2026-09-26 워크플로우 종합 점검에서 리뷰 문서에 있던 구현 부분을 이 기록으로 옮겼다. 상태는 완료(체크리스트 9/9 통과)다.

## 요청과 결정

> 사용자 2026-09-25: "WxGame — 코드 리뷰 의 지적사항을 모두 올바른 방법으로 해결해주세요."

> 사용자 2026-09-25: "EWxAbilitySetGrantState  이라는 State가 추가된 것도 마음에 안들어요", "OnRegister, Unregister 자체를 아예 override하지 않았으면 해요. EndPlay도요", "단순하고 직관적인 코드가 좋거든요"

> 사용자 2026-09-25: "테스트 코드 제거 후 제출해주세요.", "네, 제거하고 제출합시다"(기존 UI 테스트 `WxUIPresentationTests.cpp`의 회귀 검사 3개 삭제)

## 구현 결과

1. ASC 재등록: `GiveAbilitySets`는 실제 보유한 스펙을 확인해 없는 어빌리티만 채운다. `bAbilitySetsInitialized` 하나로 속성·GE를 최초 한 번만 적용해 HP/SP 초기화와 효과 중복을 막는다. 미등록·비권위 ASC는 부여하지 않고, SP 변화 구독은 객체당 한 번이다.
2. 사망·이벤트 구독: 캐릭터 사망·래그돌 태그 구독을 객체당 한 번으로 제한하고, `bDeathHandled`·`bDeathNotified`로 사망 처리와 시체 BeginPlay 재진입의 처치·보상 중복을 막는다. WxAI 컨트롤러 이벤트는 `AddUniqueDynamic`, 피격 이벤트는 유효한 구독 핸들이 없을 때만 등록한다.
3. 새 게임 검증 순서: 체크포인트 삭제 전에 선택 클래스를 동기 로드해 Pawn 상속과 Abstract·Deprecated·NewerVersionExists 플래그를 확인한다. 실패하면 상태 문구만 바꾸고 거절하며, 유효한 클래스는 `TSubclassOf<APawn>`으로 유지해 목적지에서 다시 로드하지 않는다.
4. 2026-09-27 추가: 레벨 재표시 때 WxCombat `AWxWeaponBase::PostInitializeComponents`가 무기 겹침 처리기를 한 번 더 등록해 엔진 ensure가 났다. 엔진은 ensure 뒤에도 등록을 더해 처리기가 여러 번 불리므로 `AddUniqueDynamic`으로 고쳤다.

## 검증 범위

- 2026-09-25 AI: Editor Development 빌드, 임시 회귀(어빌리티 재등록 두 차례 왕복, 사망 알림·구독 일회성, 새 게임 검증), 테스트 소스 제거 확인. 자동 회귀는 등록·초기화·태그·저장 API만 본다.
- 2026-09-27 AI 헤드리스: 게임 맵이 모두 World Partition이라 적 1명을 둔 임시 레벨을 `LoadLevelInstance`로 불러 두 번 숨겼다 보였다. 숨길 때마다 능력 7→0, 보일 때마다 7로 재부여, 효과 중복 없음, HP 80 유지, 재표시 뒤 공격·피격·사망(처치 통지 1회, 보상 100 = 대조군), 래그돌. 숨기면 AI 컨트롤러가 떨어지고 다시 보이면 새 컨트롤러가 빙의한다. 새 게임은 LV_FrontEnd에서 New Game → BP_HGTest → LV_DevCombat → Yes 뒤 플레이어 폰이 BP_HGTest_C였다.
- 사람: woogle 2026-09-28 코드 리뷰·재표시 뒤 연출 통과.

## 관련 주제

- [[모듈 구조와 코드 정리]]
- [[체크포인트와 리스폰]]
- [[적 AI와 몬스터]]

## 핵심 주장

- 2026-09-25 사용자 요청으로 WxGame 코드 리뷰 지적 세 개를 고쳤고, 사용자 단순화 지시로 부여 상태 enum과 OnRegister·OnUnregister·EndPlay 오버라이드 없이 실제 스펙 보유 여부로 판단한다. ^c1
- 수정 뒤 GiveAbilitySets는 없는 스펙만 채우고 bAbilitySetsInitialized로 속성·GE를 최초 한 번만 적용하며, bDeathHandled·bDeathNotified로 사망 처리와 처치 보상을 객체 수명당 한 번으로 제한하고, 새 게임은 체크포인트 삭제 전에 선택 Pawn 클래스를 검증한다. ^c2
- 2026-09-27 헤드리스 레벨 재표시 테스트에서 AWxWeaponBase::PostInitializeComponents의 무기 겹침 구독 중복 ensure를 찾아 AddUniqueDynamic으로 고쳤고, 재표시 전투와 새 게임 캐릭터 선택이 통과했다. ^c3
- woogle은 2026-09-28 WxGame 리뷰 수정의 코드 리뷰와 재표시 뒤 연출을 통과시켜 체크리스트 9/9로 완료됐다. ^c4
