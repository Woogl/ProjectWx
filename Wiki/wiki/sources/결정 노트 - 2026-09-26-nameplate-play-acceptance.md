---
type: source
title: "결정 노트 - 2026-09-26-nameplate-play-acceptance"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "Nameplate"
  - "UI"
  - "테스트"
summary: "NameplateManager 로컬 표시의 교전·락온·대상 전환·사망·크기와 위치·리슨 호스트와 원격 클라이언트 구분 6항목을 사람이 확인한 범위"
source_type: decision-note
source_id: src-0648b218c0503ea2c433
sha256: ac6825062490b11f88ac1431eb10434ea8ba2810f5ae18ad79cbd79e3fad8583
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-nameplate-play-acceptance.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-nameplate-play-acceptance.md"
raw_copy: ".raw/captured/ac6825062490b11f88ac1431eb10434ea8ba2810f5ae18ad79cbd79e3fad8583.md"
claim_ids:
  - clm-cfa197c03a-c1
  - clm-cfa197c03a-c2
  - clm-cfa197c03a-c3
key_claims:
  - "사람은 사망한 적의 Nameplate가 락온 중이어도 숨겨짐을 확인했다."
  - "사람은 리슨 서버 호스트와 원격 클라이언트가 각자 자기 락온 대상에만 Reticle과 Nameplate를 보는 것을 확인했다."
  - "Nameplate의 3000cm 밖 숨김 확인에도 락온 대상에는 NameplateManager 거리 제한을 적용하지 않는 락온 예외가 유지된다."
---

# 결정 노트 - 2026-09-26-nameplate-play-acceptance

- 원본: `.wiki/raw/notes/2026-09-26-nameplate-play-acceptance.md`
- 원자료 사본: `.raw/captured/ac6825062490b11f88ac1431eb10434ea8ba2810f5ae18ad79cbd79e3fad8583.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-nameplate-play-acceptance.md`(제목 "Nameplate 로컬 표시의 사람 확인 범위", 수집일 2026-09-26).
- 출처: 작업 기록 `.agents/workflow/tasks/nameplate-manager.md`의 상태·다음 행동·최종 구현·테스트 체크리스트·사용자 결과를 2026-09-26 읽었다. SHA-256 `c5d61d85dd68ebaa2246b56b558169725be041ca787b225e67b7e0f111e00ba5`로 접수 해시와 일치한다.
- 사람 체크리스트 6/6 통과, 근거 `이우성 2026-09-25`, 결과 기록 시각 `2026-09-25T17:11:01.445Z`.
- 관련 작업: [[작업 - nameplate-manager]]

## 사람의 판단 원문

> 사용자(이우성) 2026-09-25: "통과 · 교전 표시"
> 사용자(이우성) 2026-09-25: "통과 · 락온 표시"
> 사용자(이우성) 2026-09-25: "통과 · 락온 대상 전환"
> 사용자(이우성) 2026-09-25: "통과 · 사망 표시"
> 사용자(이우성) 2026-09-25: "통과 · 크기·위치"
> 사용자(이우성) 2026-09-25: "통과 · 호스트·클라이언트 구분"

## 확인한 표시 계약

- 교전 전에는 숨기고, 적이 인식하면 표시하며, 추적 종료 시 즉시 숨긴다.
- 교전하지 않은 적도 락온하면 Nameplate와 Reticle을 표시한다. 락온 해제 시 교전 중이면 Nameplate를 유지하고, 아니면 숨긴다.
- 락온 대상을 바꾸면 Reticle이 새 부위로 즉시 이동한다.
- 사망한 적은 락온 중이어도 Nameplate를 숨긴다.
- 거리별 크기 변화와 3000cm 밖 숨김. 캡슐 윗면 약 90cm 위에 표시하며 공격·피격 모션에 흔들리지 않는다.
- 리슨 서버 호스트와 원격 클라이언트가 각자 자기 락온에만 Reticle과 Nameplate를 본다.

## 해석 경계

- 거리 항목은 최종 구현의 락온 예외와 함께 해석한다. 락온 대상에는 NameplateManager의 거리 제한을 적용하지 않고 락온 가능 거리는 락온 기능이 정한다. 이 테스트 결과로 락온 예외가 제거되었다고 해석하지 않는다.

## 확인하지 않은 것

- 이번 처리는 사람 결과의 편찬이며 AI가 빌드·게임·멀티플레이를 다시 실행한 결과가 아니다.
- 기존 WxGame 이동 원자료의 인게임 미검증 설명은 위 여섯 항목 범위에서만 보완되고, 기존 원자료는 당시 관찰로 보존한다. 작업 상태·승인 정본은 Workflow다.

## 관련 주제

- [[UI 표시 구조]]
- [[기획서 - Nameplate_System]]
- [[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]

## 핵심 주장

- 사람은 사망한 적의 Nameplate가 락온 중이어도 숨겨짐을 확인했다. ^c1
- 사람은 리슨 서버 호스트와 원격 클라이언트가 각자 자기 락온 대상에만 Reticle과 Nameplate를 보는 것을 확인했다. ^c2
- Nameplate의 3000cm 밖 숨김 확인에도 락온 대상에는 NameplateManager 거리 제한을 적용하지 않는 락온 예외가 유지된다. ^c3
