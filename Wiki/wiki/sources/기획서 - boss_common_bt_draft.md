---
type: source
title: "기획서 - boss_common_bt_draft"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "기획서"
  - "보스"
  - "BT"
  - "AI"
summary: "보스 BT를 페이즈 전환 → 거리 조정 → 강공격군 → 일반공격군 → 비공격 행동 순의 Selector와 패턴군 내부 선택으로 구성하는 공통 초안"
source_type: design-doc
source_id: src-14ce24a072e8f0124b40
sha256: 57003a2a37044bb2215e5ef80b1232416e0cf4ffd5d551275ad1c0df9bbe248e
authority: primary
independence_key: "Docs/CombatDesign/boss_common_bt_draft.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - "Docs/CombatDesign/boss_common_bt_draft.md"
raw_copy: ".raw/captured/57003a2a37044bb2215e5ef80b1232416e0cf4ffd5d551275ad1c0df9bbe248e.md"
claim_ids:
  - clm-63562ccdea-c1
  - clm-63562ccdea-c2
  - clm-63562ccdea-c3
key_claims:
  - "boss_common_bt_draft 문서는 보스 BT의 Root Selector를 페이즈 전환, 거리 조정, 강공격 패턴군, 일반공격 패턴군, 비공격 행동 순으로 구성하도록 제안한다."
  - "boss_common_bt_draft 문서는 강공격 패턴군에만 쿨타임 재사용 제한을 두고 일반공격 패턴군은 같은 공격 연속 사용만 제한하는 fallback 루프로 쓰도록 제안한다."
  - "boss_common_bt_draft 문서는 보스의 그로기 상태 처리 여부와 상위 우선순위 반영 방식을 추후 세부화 항목으로 남긴다."
---

# 기획서 - boss_common_bt_draft

- 원본: `Docs/CombatDesign/boss_common_bt_draft.md`
- 원자료 사본: `.raw/captured/57003a2a37044bb2215e5ef80b1232416e0cf4ffd5d551275ad1c0df9bbe248e.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

`Docs/CombatDesign/boss_common_bt_draft.md`는 보스 몬스터의 **공통 BT 설계 기조**를 정한 초안(요구사항)이다. 개별 보스는 이 상위 규칙 위에 콘셉트별 세부 BT를 더한다. 핵심은 개별 패턴을 직접 나열하지 않고 **패턴군을 먼저 고른 뒤 그 안에서 실제 패턴을 고르는 구조**다. 기대 효과로 명확한 우선순위, 패턴 추가·교체가 쉬운 확장성, 같은 패턴 반복 감소를 든다.

## 요구사항: 상위 우선순위(Root Selector)

1. Phase Transition Sequence — 페이즈 전환 패턴
2. Reposition Sequence — 거리 조정 패턴
3. Heavy Attack Sequence — 강공격 패턴군
4. Normal Attack Sequence — 일반공격 패턴군
5. Utility / Taunt Sequence — 비공격 행동(도발·타겟 재설정 등)

상위 노드가 실패하면 다음 우선순위로 넘어간다.

## 요구사항: 패턴군별 규칙

| 패턴군 | 조건·규칙 |
|---|---|
| 페이즈 전환 | 체력 임계치 이하 + 해당 전환 미사용 + 전환 중 아님. 1회성 체크, 실행 후 페이즈 값 갱신과 다음 페이즈 BT·패턴 테이블 전환(예: HP 70%·30%) |
| 거리 조정 | 플레이어가 유효 공격 거리 밖이면 공격보다 먼저 실행(추격·돌진·점프 접근·재정렬 등) |
| 강공격 | 강공격 유효 거리 안 + 재사용 제한(쿨타임) 종료 + 최근 사용 패턴 제외. 실행 후 재사용 제한 시작 |
| 일반공격 | 별도 쿨타임 없음, 같은 공격 연속 사용만 제한. 강공격 불가 시 fallback 루프 |
| 비공격 행동 | 메인 루프보다 우선하지 않음, 과도한 빈도 제한(예: 루프 반복 시, 타겟 유실 시, 패턴 후 저확률, 페이즈 진입 후 1회) |

## 요구사항: 패턴군 내부 선택

사용 가능한 패턴만 후보로 넣고, 최근 사용 패턴을 빼고, 같은 패턴 연속 사용을 금지할 수 있으며, 남은 후보 중 랜덤 또는 가중치로 고른다.

## 미결정·충돌

원문 "추후 세부화가 필요한 항목": 유효 공격 거리 기준값, 강공격·일반공격 후보 목록, 최근 사용 패턴 체크 방식, 랜덤·가중치 규칙, 도발 실행 조건, 페이즈별 BT 교체 방식, **그로기 상태 처리 여부 및 상위 우선순위 반영 방식**.

그 밖에:

- 이 문서의 패턴군 이름(거리 조정·강공격·일반공격·비공격)은 [[기획서 - 적 규격서]]의 분류(조우·추적·일반·연속·가드 불가·필살기·페이즈 전환기)와 일대일로 대응되지 않는다. 조우 패턴과 필살기의 BT 위치가 없다.
- [[기획서 - WX_첫_보스_기획서]]의 커스터는 HP 60%에서 2페이즈, HP 10%에서 필살기(발악 패턴)를 쓰는데, 이 문서의 예시 임계치(70%·30%)와 다르고, 체력 조건 발악 패턴을 어느 Sequence에 둘지 정해지지 않았다.

## 관련 주제

- [[보스 전투]]
- [[적 AI와 몬스터]]

## 핵심 주장

- boss_common_bt_draft 문서는 보스 BT의 Root Selector를 페이즈 전환, 거리 조정, 강공격 패턴군, 일반공격 패턴군, 비공격 행동 순으로 구성하도록 제안한다. ^c1
- boss_common_bt_draft 문서는 강공격 패턴군에만 쿨타임 재사용 제한을 두고 일반공격 패턴군은 같은 공격 연속 사용만 제한하는 fallback 루프로 쓰도록 제안한다. ^c2
- boss_common_bt_draft 문서는 보스의 그로기 상태 처리 여부와 상위 우선순위 반영 방식을 추후 세부화 항목으로 남긴다. ^c3
