---
type: source
title: "결정 노트 - 2026-09-26-animnotify-label-acceptance"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "AnimNotify"
  - "에디터"
  - "테스트"
summary: "몽타주 타임라인에서 AnimNotify 17종 짧은 라벨의 값 일치와 겹침·잘림 없는 가독성을 사람이 확인한 범위와 그 경계"
source_type: decision-note
source_id: src-2b75767555b50e4be106
sha256: 139aa73bd085ee7b0bd4541ea2443aeb8dcb87a2d8bab1ab35702e03fc8397dd
authority: primary
independence_key: ".wiki/raw/notes/2026-09-26-animnotify-label-acceptance.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-26-animnotify-label-acceptance.md"
raw_copy: ".raw/captured/139aa73bd085ee7b0bd4541ea2443aeb8dcb87a2d8bab1ab35702e03fc8397dd.md"
claim_ids:
  - clm-83c2c43b9b-c1
  - clm-83c2c43b9b-c2
key_claims:
  - "사람은 몽타주 타임라인에서 WxCombat 15종·WxAI ReportNoise·WxInventory UseItem 등 17종 AnimNotify 짧은 라벨 값이 설정과 일치함을 확인했다."
  - "AnimNotify 라벨 가독성 확인은 확인한 몽타주 범위에 한정되며 모든 화면 배율과 임의 길이 에셋 이름의 가독성을 보장하지 않는다."
---

# 결정 노트 - 2026-09-26-animnotify-label-acceptance

- 원본: `.wiki/raw/notes/2026-09-26-animnotify-label-acceptance.md`
- 원자료 사본: `.raw/captured/139aa73bd085ee7b0bd4541ea2443aeb8dcb87a2d8bab1ab35702e03fc8397dd.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki 결정 노트 `2026-09-26-animnotify-label-acceptance.md`(제목 "AnimNotify 짧은 라벨의 사람 확인 범위", 수집일 2026-09-26).
- 출처: 작업 기록 `.agents/workflow/tasks/animnotify-labels.md`를 2026-09-26 읽었다. SHA-256 `d40b92a81c52df76f669259b5eb2b146776be098471a0f1099b0f7fbfdc3ffe3`으로 접수 해시와 일치한다.
- 체크리스트 사람 항목 2/2 통과, 근거 `이우성 2026-09-25`, 결과 기록 시각 `2026-09-25T17:14:48.161Z`.
- 관련 작업: [[작업 - animnotify-labels]]

## 사람의 판단 원문

> 사용자(이우성) 2026-09-25: "통과 · 17종 라벨 값"
> 사용자(이우성) 2026-09-25: "통과 · 라벨 가독성"

## 확인한 것

- 몽타주 타임라인에서 WxCombat 15종, WxAI `ReportNoise`, WxInventory `UseItem`의 짧은 라벨 값이 설정과 일치한다.
- 라벨이 겹치거나 잘리지 않고 읽힌다.
- 유지되는 표시 규칙: `종류: 대표 값 하나`, Row·에셋 이름 보존, 클래스명 끝 `_C` 제거, 미설정 `None`, 스냅 비활성 `Off`, 고정 표식 Recovery·Combo Window·Use Item.
- 작업 기록과 기존 원자료에 남은 화면 미검증 설명은 위 두 항목 범위에서 대체된다.

## 확인하지 않은 것

- 모든 몽타주·화면 배율·임의 길이 에셋 이름에서의 가독성 보장, 실행 동작·멀티플레이 검증으로 확대하지 않는다.
- 이번 수집에서 게임 코드·에셋 수정, 빌드·에디터 테스트 재실행은 없었다. 기록 속 과거 빌드 성공과 실행 명령은 관찰 자료다.
- 작업 상태와 사람 판단의 정본은 Workflow 기록이다.

## 관련 주제

- [[에디터 도구]]
- [[결정 노트 - 2026-09-25-animnotify-labels]]

## 핵심 주장

- 사람은 몽타주 타임라인에서 WxCombat 15종·WxAI ReportNoise·WxInventory UseItem 등 17종 AnimNotify 짧은 라벨 값이 설정과 일치함을 확인했다. ^c1
- AnimNotify 라벨 가독성 확인은 확인한 몽타주 범위에 한정되며 모든 화면 배율과 임의 길이 에셋 이름의 가독성을 보장하지 않는다. ^c2
