---
type: source
title: "결정 노트 - 2026-09-26-cooldown-play-acceptance"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "쿨다운"
  - "GAS"
  - "테스트"
summary: "공용 쿨다운 GE 통합 후 회피 UI 진행률·소환물 쿨다운 무시·리슨 서버와 클라이언트 복제·코드 리뷰를 사람이 통과시킨 확인 범위"
source_type: decision-note
source_id: src-b6d36e66728227406218
sha256: 1b1ac274ccc99ee5a0c1f2401597cbad82f408872f620216cc592223cc8c38b0
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-cooldown-play-acceptance.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-cooldown-play-acceptance.md"
raw_copy: ".raw/captured/1b1ac274ccc99ee5a0c1f2401597cbad82f408872f620216cc592223cc8c38b0.md"
claim_ids:
  - clm-14cf63ee66-c1
  - clm-14cf63ee66-c2
  - clm-14cf63ee66-c3
key_claims:
  - "사람은 공용 쿨다운 GE 통합 후 리슨 서버와 클라이언트 PIE에서 클라이언트의 쿨다운 표시·차례 회복이 서버와 같음을 확인했다."
  - "공용 쿨다운 GE 통합 후에도 소환물의 어빌리티는 쿨다운을 무시하고 발동됨이 사람 테스트로 확인되었다."
  - "공용 쿨다운 GE의 사람 확인은 전용 서버·지연·패킷 손실 조건과 모든 어빌리티 조합을 포함하지 않는다."
---

# 결정 노트 - 2026-09-26-cooldown-play-acceptance

- 원본: `.wiki/raw/notes/2026-09-26-cooldown-play-acceptance.md`
- 원자료 사본: `.raw/captured/1b1ac274ccc99ee5a0c1f2401597cbad82f408872f620216cc592223cc8c38b0.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-cooldown-play-acceptance.md`(제목 "공용 쿨다운 GE의 사람 확인 범위", 수집일 2026-09-26).
- 출처: 작업 기록 `.agents/workflow/tasks/cooldown-unification.md`를 2026-09-26 읽었다. SHA-256 `c7908997522bed7e1f8577ac6828fc7877f36e5e2b2d39bdbeca0a44cb301e1c`로 접수 해시와 일치한다.
- 체크리스트 사람 항목 4/4 통과, 근거 `이우성 2026-09-25`, 결과 기록 시각 `2026-09-25T17:17:46.283Z`.
- 관련 작업: [[작업 - cooldown-unification]]

## 사람의 판단 원문

> 사용자(이우성) 2026-09-25: "통과 · 회피 쿨다운·UI 진행률"
> 사용자(이우성) 2026-09-25: "통과 · 소환물 쿨다운 무시"
> 사용자(이우성) 2026-09-25: "통과 · 네트워크 복제"
> 사용자(이우성) 2026-09-25: "통과 · 코드 리뷰"

## 확인한 것

- 회피 1회 사용 뒤 쿨다운과 UI 진행률이 맞다.
- 소환물의 어빌리티가 기존처럼 쿨다운을 무시하고 발동된다.
- 리슨 서버와 클라이언트 PIE에서 클라이언트의 쿨다운 표시·차례 회복이 서버와 같다.
- 코드 리뷰 대상: 공용 `UWxEffect_Cooldown`과 `ApplyCooldown`·`CheckCooldown` 변경.
- 유지되는 구조: 공용 GE·동적 쿨다운 태그·충전당 GE 하나·적용 전 SetByCaller 대기열 계산. 당시 남았던 소환물 쿨다운 무시와 네트워크 복제 미확인은 위 범위에서 대체된다.

## 확인하지 않은 것

- 전용 서버, 지연·패킷 손실 조건, 모든 어빌리티 조합까지 검증한 것으로 확대하지 않는다.
- 이번 수집에서 게임 코드·에셋 수정, 빌드·자동화·PIE 재실행은 없었다. 기록 속 빌드·데이터 검증·삭제된 임시 자동화 테스트 결과는 과거 관찰 자료다.
- 작업 상태와 사람 판단의 정본은 Workflow 기록이다.

## 관련 주제

- [[어빌리티와 GAS]]
- [[결정 노트 - 2026-09-25-cooldown-single-ge]]

## 핵심 주장

- 사람은 공용 쿨다운 GE 통합 후 리슨 서버와 클라이언트 PIE에서 클라이언트의 쿨다운 표시·차례 회복이 서버와 같음을 확인했다. ^c1
- 공용 쿨다운 GE 통합 후에도 소환물의 어빌리티는 쿨다운을 무시하고 발동됨이 사람 테스트로 확인되었다. ^c2
- 공용 쿨다운 GE의 사람 확인은 전용 서버·지연·패킷 손실 조건과 모든 어빌리티 조합을 포함하지 않는다. ^c3
