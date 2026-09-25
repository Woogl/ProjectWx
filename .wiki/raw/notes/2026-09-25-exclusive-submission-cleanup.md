---
title: "Exclusive 차단 테스트 제거와 주석 정정"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, combat, testing]
summary: "사용자 요청으로 Exclusive 차단의 임시 C++ 테스트와 전용 friend, GA 검증 스크립트를 제거했다. 점유·그룹 우회 설명을 태그 차단과 취소 면역으로 정정했으며, 앞선 회귀 결과는 삭제 전 검증 근거로 보존한다."
---

# Exclusive 차단 테스트 제거와 주석 정정

사용자는 "테스트 코드 제거하고, 주석 정정해주세요.", "다 끝나면 제출해주세요."라고 요청했다. 승인과 제출·확인 상태는 [작업 기록](../../../.agents/workflow/tasks/exclusive-tag-blocking.md)을 따른다.

- `Source/WxEditor/Tests/WxAbilityBlockingTests.cpp`와 `WxAbilityBlockingTestTypes.h`, 베이스의 `FWxExclusiveAbilityBlockingTest` friend를 제거했다.
- `Saved/Tests/ValidateExclusiveAbilityTags.py`와 `ValidateExclusiveAbilityTagsAsc.py`도 제거했다. 실행 로그와 JSON 보고서는 삭제 전 검증 근거로 남긴다.
- 앞선 AssetDefaults·HookRules·Lifecycle 성공 3건과 GA 40개 관계 1,600건·점프 40건은 임시 테스트 실행 당시의 결과다. 테스트 소스가 제출본에 포함된다는 뜻이 아니다.
- 차단 관련 12개 파일의 주석을 점검했다. Override는 취소 면역이며 발동 차단 여부는 실제 에셋 태그와 차단 목록이 결정한다. 콤보는 자기 차단 기여만 제외하고 Recovery는 자기 태그 차단을 해제한다.
- 처형·가드 반응은 공통 목록에 식별 태그가 없어 진입한다. 사망 상태를 나타내는 소유 태그와 어빌리티 발동을 막는 BlockAbilitiesWithTag는 별개다.
- 주석 정정 전후 주석을 제외한 코드 해시를 비교해 12개 파일 모두 동일함을 확인했다. 이 비교의 기준은 테스트 friend를 제거한 직후이며, 테스트 삭제 자체를 코드 변경 0이라고 표현하지 않는다.
- 사람의 코드 리뷰·실제 입력·도플갱어 BT 타이밍·UI·예측/복제는 미확인이다. 제출 요청을 플레이 검증 통과로 대신하지 않는다.

이 기록은 [ASC 공통화](2026-09-25-ability-block-policy-centralization.md)의 차단 정책을 바꾸지 않는다.
