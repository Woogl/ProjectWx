---
type: source
title: "결정 노트 - 2026-09-25-exclusive-submission-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "GAS"
  - "테스트"
summary: "사용자 요청으로 Exclusive 차단의 임시 C++ 테스트·검증 스크립트를 제거하고 차단 관련 주석을 태그 차단·취소 면역 기준으로 정정한 기록"
source_type: decision-note
source_id: src-3bd37c07acf8bbad4701
sha256: 63a9bcb713a22d060efe50277c576b84b6c3b7c629bb4a21ad71280c4b48bfb1
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-exclusive-submission-cleanup.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-exclusive-submission-cleanup.md"
raw_copy: ".raw/captured/63a9bcb713a22d060efe50277c576b84b6c3b7c629bb4a21ad71280c4b48bfb1.md"
claim_ids:
  - clm-11befcd369-c1
  - clm-11befcd369-c2
  - clm-11befcd369-c3
key_claims:
  - "사용자 요청에 따라 Exclusive 차단용 임시 C++ 테스트 파일, 전용 friend, GA 검증 Python 스크립트가 제거되었다."
  - "주석 정정 전후 주석을 제외한 코드 해시가 차단 관련 12개 파일 모두에서 동일했다."
  - "Exclusive 차단 제출 시점에 사람의 코드 리뷰·실제 입력·도플갱어 BT 타이밍·UI·예측/복제는 확인되지 않았다."
---

# 결정 노트 - 2026-09-25-exclusive-submission-cleanup

- 원본: `.wiki/raw/notes/2026-09-25-exclusive-submission-cleanup.md`
- 원자료 사본: `.raw/captured/63a9bcb713a22d060efe50277c576b84b6c3b7c629bb4a21ad71280c4b48bfb1.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-exclusive-submission-cleanup.md`(제목 "Exclusive 차단 테스트 제거와 주석 정정", source `MANUAL`, ingested 2026-09-25)를 요약한다. [[결정 노트 - 2026-09-25-exclusive-tag-blocking]]의 후속 정리 작업이며, 승인과 제출·확인 상태는 작업 기록 `.agents/workflow/tasks/exclusive-tag-blocking.md`를 따른다고 적혀 있다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-25: "테스트 코드 제거하고, 주석 정정해주세요."
> 사용자 2026-09-25: "다 끝나면 제출해주세요."

## 제거한 것

- `Source/WxEditor/Tests/WxAbilityBlockingTests.cpp`, `WxAbilityBlockingTestTypes.h`, 베이스의 `FWxExclusiveAbilityBlockingTest` friend.
- `Saved/Tests/ValidateExclusiveAbilityTags.py`, `ValidateExclusiveAbilityTagsAsc.py`. 실행 로그와 JSON 보고서는 삭제 전 검증 근거로 남긴다.
- 앞선 AssetDefaults·HookRules·Lifecycle 성공 3건과 GA 40개 관계 1,600건·점프 40건은 임시 테스트 실행 당시 결과이며, 테스트 소스가 제출본에 포함된다는 뜻이 아니다.

## 주석 정정 내용(구현 관찰)

- 차단 관련 12개 파일의 주석을 점검했다. Override는 취소 면역이며 발동 차단 여부는 실제 에셋 태그와 차단 목록이 결정한다.
- 콤보는 자기 차단 기여만 제외하고, Recovery는 자기 태그 차단을 해제한다.
- 처형·가드 반응은 공통 목록에 식별 태그가 없어 진입한다. 사망 상태 소유 태그와 BlockAbilitiesWithTag는 별개다.

## 검증 범위

- 주석 정정 전후로 주석을 제외한 코드 해시를 비교해 12개 파일 모두 동일함을 확인했다. 비교 기준은 테스트 friend를 제거한 직후이며, 테스트 삭제 자체를 코드 변경 0이라고 표현하지 않는다.
- 사람의 코드 리뷰·실제 입력·도플갱어 BT 타이밍·UI·예측/복제는 미확인이다. 노트는 제출 요청을 플레이 검증 통과로 대신하지 않는다고 명시한다.
- 이 기록은 [[결정 노트 - 2026-09-25-ability-block-policy-centralization]]의 차단 정책을 바꾸지 않는다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[결정 노트 - 2026-09-25-exclusive-tag-blocking]]

## 핵심 주장

- 사용자 요청에 따라 Exclusive 차단용 임시 C++ 테스트 파일, 전용 friend, GA 검증 Python 스크립트가 제거되었다. ^c1
- 주석 정정 전후 주석을 제외한 코드 해시가 차단 관련 12개 파일 모두에서 동일했다. ^c2
- Exclusive 차단 제출 시점에 사람의 코드 리뷰·실제 입력·도플갱어 BT 타이밍·UI·예측/복제는 확인되지 않았다. ^c3
