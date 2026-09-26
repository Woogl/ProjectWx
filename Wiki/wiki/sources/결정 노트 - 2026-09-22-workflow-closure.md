---
type: source
title: "결정 노트 - 2026-09-22-workflow-closure"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "Workflow 실행기에 승인 코드 버전 대조, 테스트 수용과 정리 완료 분리, 승인자 이름 기록을 구현한 2026-09-22 요청·구현·모의 테스트 범위 기록"
source_type: decision-note
source_id: src-190324fcc51209d69ca0
sha256: 3a87f612e6308146b063389aac2bbc003d65494a2c0d9aef306d5722b02465d4
authority: primary
independence_key: ".wiki/raw/notes/2026-09-22-workflow-closure.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-22-workflow-closure.md"
raw_copy: ".raw/captured/3a87f612e6308146b063389aac2bbc003d65494a2c0d9aef306d5722b02465d4.md"
claim_ids:
  - clm-ec374ad6ca-c1
  - clm-ec374ad6ca-c2
  - clm-ec374ad6ca-c3
  - clm-ec374ad6ca-c4
key_claims:
  - "2026-09-22 Workflow 실행기는 검증 시작과 accept 때 현재 코드 버전을 마지막 approve의 codeVersion과 대조해 다르면 구현 단계로 되돌리도록 구현됐다."
  - "2026-09-22 Workflow 실행기의 finish는 승인자 이름·wikiStatus·wikiEvidence·cleanupEvidence가 모두 있어야 complete와 closure를 저장한다."
  - "2026-09-22 Workflow 승인자 이름은 직접 입력한 표시명이며 계정 인증이 아니다."
  - "2026-09-22 Workflow closure 작업의 검증은 자동 테스트와 모의 DOM에 한정되며 실제 AI 호출·브라우저 육안·UE 실행은 하지 않았다."
---

# 결정 노트 - 2026-09-22-workflow-closure

- 원본: `.wiki/raw/notes/2026-09-22-workflow-closure.md`
- 원자료 사본: `.raw/captured/3a87f612e6308146b063389aac2bbc003d65494a2c0d9aef306d5722b02465d4.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: `.wiki/raw/notes/2026-09-22-workflow-closure.md`(제목 "Workflow 승인 버전·정리 완료·승인자 기록").
- frontmatter: `source: MANUAL`, `ingested: 2026-09-22`, 태그 `wx, workflow`.
- 성격: 사용자 요청 → 구현 관찰 → 자동 테스트 범위. 조사 시점(2026-09-22) 기준이며 이후 Workflow는 여러 차례 개편되어 현재 코드와 다를 수 있다.

## 사용자 요청 (노트의 간접 기록)

노트는 요청을 직접 인용하지 않고 이렇게 적는다.

> 노트 2026-09-22: "AI 워크플로우 점검의 P2 두 항목 개선 요청 후 P1도 추가 요청했다. 코드 리뷰 승인 버전 보장, 테스트 수용과 최종 정리 완료 분리, 승인자 누락을 수정했다."

## 구현 관찰

- **승인 버전 대조**: `Workflow-Execution.cjs`는 검증 시작과 `accept` 시 현재 코드 버전을 마지막 `approve`의 `codeVersion`과 비교한다. 다르면 `blocked/implement`로 되돌려 재검토·리뷰를 요구한다. 재시도 직전 코드가 바뀌었으면 검증을 시작하지 않는다.
- **수용과 정리 분리**: `accept`는 `cleanup` 상태를 저장한다. `finish`는 승인자 이름, `wikiStatus`(`reflected`/`skipped`), `wikiEvidence`, `cleanupEvidence`가 모두 있어야 `complete`와 `closure`를 저장한다. `closure`는 확인 시각과 수용 코드 버전도 보존한다.
- 실행기는 실제 Wiki 작업을 자동 실행하거나 기록 내용의 진위를 인증하지 않는다. 자료 정리는 AI가 실제 수행하고 결과를 준비한다.
- **승인자**: `approve`·`accept`는 이름이 비면 거부하고 `decisions.actor`를 저장한다. 이름은 직접 입력한 표시명이며 계정 인증이 아니다. 과거 승인자 신원은 추정하지 않는다.
- **과거 기록 호환**: `readExecution`은 `closure` 없는 과거 `complete`를 `cleanup`으로 해석하며 원본 파일을 자동 수정하지 않는다. 대시보드는 정리 대기와 완료를 따로 표시한다.

## 검증 범위

- `TestWorkflowExecution.cjs` 통과: 승인 버전과 다른 코드의 재시도·최종 수용 거부, 동일 버전 재시도 허용, 승인자 누락·불완전 정리 거부, 재시작·재전송, 과거 기록 보존.
- `TestWikiViewer.cjs` 통과: 승인자·정리 입력, 재렌더 보존, 대시보드 분류를 **모의 DOM**으로 검증.
- 실제 AI 호출·브라우저 육안·UE 실행은 **미실시**.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-22 Workflow 실행기는 검증 시작과 accept 때 현재 코드 버전을 마지막 approve의 codeVersion과 대조해 다르면 구현 단계로 되돌리도록 구현됐다. ^c1
- 2026-09-22 Workflow 실행기의 finish는 승인자 이름·wikiStatus·wikiEvidence·cleanupEvidence가 모두 있어야 complete와 closure를 저장한다. ^c2
- 2026-09-22 Workflow 승인자 이름은 직접 입력한 표시명이며 계정 인증이 아니다. ^c3
- 2026-09-22 Workflow closure 작업의 검증은 자동 테스트와 모의 DOM에 한정되며 실제 AI 호출·브라우저 육안·UE 실행은 하지 않았다. ^c4
