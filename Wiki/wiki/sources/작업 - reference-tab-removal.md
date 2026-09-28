---
type: source
title: "작업 - reference-tab-removal"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
summary: "상태 줄 없는 기록(모듈 리뷰 등)을 대시보드에서 숨기려고 리뷰·참고 탭을 없앤 2026-09-27 완료 작업 기록"
source_type: task-record
source_id: src-bb7fe4e0b6bd3930108f
sha256: eeedc24b3183c11185daf45ac04bb4e1fb5517b26a8650a4d3e3aa66621bbe8a
authority: primary
independence_key: ".agents/workflow/tasks/reference-tab-removal.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/reference-tab-removal.md"
raw_copy: ".raw/captured/eeedc24b3183c11185daf45ac04bb4e1fb5517b26a8650a4d3e3aa66621bbe8a.md"
claim_ids:
  - clm-eeedc24b31-c1
  - clm-eeedc24b31-c2
key_claims:
  - "2026-09-27 결정으로 상태 줄이 없는 기록(모듈 리뷰 등)은 할 일이 아니어서 대시보드 리뷰·참고 탭을 없애고 --tasks 출력에만 나오게 했으며, 모듈 리뷰에 상태 줄을 넣는 안은 기각했다."
  - "리뷰·참고 탭 제거는 워크플로우 Node 테스트 4개, 헤드리스 Edge 1440px 캡처, --tasks 출력으로 확인했고 woogle이 2026-09-27 코드 리뷰를 통과시켰다."
---

# 작업 - reference-tab-removal

- 원본: `.agents/workflow/tasks/reference-tab-removal.md`
- 원자료 사본: `.raw/captured/eeedc24b3183c11185daf45ac04bb4e1fb5517b26a8650a4d3e3aa66621bbe8a.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

사용자가 워크플로우 페이지의 리뷰·참고에 어떤 문서가 등록되는지, 웹에서 보여줄 필요가 있는지 물은 데서 시작한 작은 워크플로우 작업 기록이다. AI의 탭 제거 제안에 사용자가 "네, 진행해주세요"로 승인했다. 상태는 완료(체크리스트 4/4 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 결정

- 결정(2026-09-27): 상태 줄이 없는 기록(모듈 리뷰 등)은 할 일이 아니므로 대시보드에 보이지 않게 한다. 파일은 `tasks/`에 그대로 두고 `--tasks` 출력에만 나온다.
- 기각한 안: 모듈 리뷰에 상태 줄을 넣는 안. 상시 확인 대기로 남고, 작업 탭 추가 요청이 리뷰 문서에 승인·체크리스트를 쓰며, 재리뷰가 문서를 통째로 다시 써 그 기록을 지운다.

## 변경과 검증

- 변경 파일: `.agents/scripts/workflow-page/workflow.js`(그룹 셋, 상태 없는 기록 제외, 안내문 제거), `Workflow-Server.cjs`(`--tasks` 제목), 워크플로우 테스트 두 개의 단언, 작업 절차 상태 절 한 줄.
- AI: 워크플로우 Node 테스트 4개 exit 0, 헤드리스 Edge 1440px 캡처(필터 확인 대기 7·진행 중 0·완료 26, 전체 39건 중 모듈 리뷰 6건 제외), `--tasks` 출력에서 모듈 리뷰 6건이 「상태 없음(대시보드에 안 보임)」 아래에 나옴.
- 사람: woogle 2026-09-27 코드 리뷰 통과. 볼 점으로 상태 줄에 오타가 난 기록도 대시보드에서 빠지고 `--tasks`에만 남는다고 적혀 있다.
- 게임 코드·빌드와 무관하다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-27 결정으로 상태 줄이 없는 기록(모듈 리뷰 등)은 할 일이 아니어서 대시보드 리뷰·참고 탭을 없애고 --tasks 출력에만 나오게 했으며, 모듈 리뷰에 상태 줄을 넣는 안은 기각했다. ^c1
- 리뷰·참고 탭 제거는 워크플로우 Node 테스트 4개, 헤드리스 Edge 1440px 캡처, --tasks 출력으로 확인했고 woogle이 2026-09-27 코드 리뷰를 통과시켰다. ^c2
