---
type: source
title: "결정 노트 - 2026-09-25-workflow-convenience-review"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "워크플로우"
summary: "단순화한 Workflow를 AI·사람 관점에서 점검한 뒤 사용자가 식별값 검사 제거·코드 리뷰 체크리스트화·옛 기록 정리·작업 현황 자동 생성 네 개선을 승인한 기록"
source_type: decision-note
source_id: src-cae1fc9ebf92a65c146b
sha256: 5f911e9bdb39e68915a5027a634ce630404e49d71df79056922e099cd2c11ec4
authority: primary
independence_key: ".wiki/raw/notes/2026-09-25-workflow-convenience-review.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-25-workflow-convenience-review.md"
raw_copy: ".raw/captured/5f911e9bdb39e68915a5027a634ce630404e49d71df79056922e099cd2c11ec4.md"
claim_ids:
  - clm-ef4f3b5f12-c1
  - clm-ef4f3b5f12-c2
  - clm-ef4f3b5f12-c3
key_claims:
  - "2026-09-25 Workflow 편의 점검 후 사용자는 식별값 검사 제거·코드 리뷰 체크리스트화·옛 기록 정리·작업 현황 자동 생성 네 개선을 모두 선택했다."
  - "저장소 전체 코드 식별값 비교는 사람이 실제로 테스트한 빌드를 보장하지 못해 테스트 결과 서버에서 제거되었다."
  - "작업 현황은 tasks/index.md 표 대신 각 작업 기록 제목 아래의 상태·다음 행동 줄에서 task-records.js가 자동으로 만든다."
---

# 결정 노트 - 2026-09-25-workflow-convenience-review

- 원본: `.wiki/raw/notes/2026-09-25-workflow-convenience-review.md`
- 원자료 사본: `.raw/captured/5f911e9bdb39e68915a5027a634ce630404e49d71df79056922e099cd2c11ec4.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

옛 LLM Wiki 원자료 노트 `2026-09-25-workflow-convenience-review.md`(제목 "Workflow 편의 점검과 기록 기반 작업 현황", source `MANUAL`, ingested 2026-09-25)를 요약한다. 기준 HEAD `c9e2efec6`에 미커밋 작업 트리를 포함한 기록이다. [[결정 노트 - 2026-09-25-workflow-simplification]] 직후의 후속 점검이며, 노트 날짜 기준이라 현재 절차·스크립트와 다를 수 있다. 현재 정본은 `.agents/workflow/process/index.md`다.

## 사람의 판단 원문

> 사용자 2026-09-25: "지금 워크플로우를 확인해서 작업하기 편한지 점검해주세요. AI 입장과 사람 입장 모두요"

점검 결과를 본 뒤 사용자는 네 개선(식별값 검사 제거, 코드 리뷰를 체크리스트로, 옛 기록 정리, 작업 현황 자동 생성)을 모두 선택했다.

## 점검 근거(관찰)

- 다른 세션 두 곳이 새 테스트 체크리스트 형식을 그대로 썼고 서버 파서가 정상으로 읽었다.
- 체크포인트 테스트 결과 전달은 두 번 모두 확인 필요로 끝났다(남은 확인 8개씩). 원인은 외부 스크립트 변경으로 바뀐 코드 식별값과, 처리 AI(Codex)가 Git 저장소 소유권 보호로 버전을 대조하지 못한 것이었다.
- 최근 60분 동안 파일 64개가 바뀌고 미커밋 코드 변경이 34개였다. 저장소 전체 식별값은 창을 연 시점과 전달 시점만 비교해 사람이 실제로 테스트한 빌드를 보장하지 못했다.
- `tasks/index.md` 표는 여러 세션과 서버가 동시에 고치는 파일이었고, 작업 기록 27건 중 제목 아래에 다음 행동이 있는 기록은 5건이었다.
- 옛 절차 문서가 삭제 대기 중에도 "승인자 이름 입력 후 코드 리뷰 승인"을 지시했다.

## 구현 관찰

- 공용 해석기 `.agents/scripts/wiki-viewer/task-records.js`가 제목 아래 첫 `## ` 전까지에서 `상태: <확인 대기|진행 중|완료>[ · 설명]`과 `다음 행동:` 줄을 읽는다. 테스트 체크리스트 파서도 여기에 둔다.
- 서버 `Workflow-TestFeedback.cjs`: 코드 식별값 계산·비교를 없애고 작업 기록 해시 대조는 유지한다. `list`는 기록에서 만든 목록을 수정 시각 내림차순으로 돌려주고, 처리 결과에 따라 기록의 상태 두 줄을 고친다. 목차 등록 여부는 검사하지 않는다.
- 처리 프롬프트: 코드를 수정했다면 영향받는 사람 항목을 대기로 되돌리고 코드 리뷰 항목을 대기로 둔다. 다른 작업의 변경이나 코드 버전은 대조하지 않는다.
- 대시보드 분류는 확인 대기·진행 중·완료·리뷰·참고 네 개(노트 원문 표기)이며 리뷰·참고 기록에는 체크리스트 버튼이 없다. `node .agents/scripts/Wiki-AI.cjs --tasks`가 같은 목록을 터미널에 출력한다.
- 절차: 코드 리뷰는 확인하기 체크리스트의 사람 항목이다. 사람 판단은 정하기의 답변·구현 승인과 확인하기의 코드 리뷰·사람 항목 테스트 두 곳이다. `tasks/index.md`는 표 없는 안내 문서가 됐다.
- 이관: 작업 현황 표의 상태·확인 범위·다음 행동을 기록 15건의 상태 두 줄로 옮겼다. 모듈 리뷰 5건과 Ability Resolver 기록은 리뷰·참고로 분류했다.

## 검증 범위

- TestWorkflowTestFeedback, TestWorkflowFeedbackUI, TestWikiProviders, TestWikiViewer·TestWikiSpaces 통과. CheckWikiLinks 66문서 오류 0. 로컬 서버 재시작 뒤 protocol 4 확인. 헤드리스 Edge로 대시보드·체크리스트 창·도식 캡처.
- 이관 뒤 작업 현황 표와 기록 상태를 행마다 대조해 의도한 6건만 달랐다.
- 실제 AI 처리 요청과 사람의 화면 확인은 하지 않았다. 다른 작업의 코드 변경은 자동 감지하지 않는다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[작업 - workflow-review]]
- <!--wl-->결정 노트 - 2026-09-25-workflow-task-records
- <!--wl-->결정 노트 - 2026-09-25-workflow-test-feedback

## 핵심 주장

- 2026-09-25 Workflow 편의 점검 후 사용자는 식별값 검사 제거·코드 리뷰 체크리스트화·옛 기록 정리·작업 현황 자동 생성 네 개선을 모두 선택했다. ^c1
- 저장소 전체 코드 식별값 비교는 사람이 실제로 테스트한 빌드를 보장하지 못해 테스트 결과 서버에서 제거되었다. ^c2
- 작업 현황은 tasks/index.md 표 대신 각 작업 기록 제목 아래의 상태·다음 행동 줄에서 task-records.js가 자동으로 만든다. ^c3
