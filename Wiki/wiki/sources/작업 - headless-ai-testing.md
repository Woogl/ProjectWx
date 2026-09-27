---
type: source
title: "작업 - headless-ai-testing"
created: 2026-09-27
updated: 2026-09-27
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
summary: "헤드리스로 판정할 수 있는 테스트는 AI가 끝까지 하고 판정할 수 없는 부분만 사람 항목으로 두도록 작업 절차와 서버를 바꾼 2026-09-27 작업 기록으로, 체크리스트 4/4 통과로 완료됐다."
source_type: task-record
source_id: src-57ab0fb2b119c6dd0d6b
sha256: 9bc5aa0c5b27785660500cbc848027e1be0e6353819e8dea249161f8105e6e86
authority: primary
independence_key: ".agents/workflow/tasks/headless-ai-testing.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/headless-ai-testing.md"
raw_copy: ".raw/captured/9bc5aa0c5b27785660500cbc848027e1be0e6353819e8dea249161f8105e6e86.md"
claim_ids:
  - clm-64f972a5a1-c1
  - clm-64f972a5a1-c2
  - clm-64f972a5a1-c3
  - clm-64f972a5a1-c4
key_claims:
  - "이우성은 2026-09-27 테스트 체크리스트에서 AI가 헤드리스 환경에서 테스트할 수 있는 모든 것은 AI가 끝까지 테스트하고, 도저히 AI가 테스트할 수 없는 부분만 사람이 테스트하도록 요청했다."
  - "headless-ai-testing 작업으로 웹 처리에서 AI가 미실행·대기로 돌려준 AI 항목은 사람 항목으로 넘어가지 않고 담당 AI 그대로 실패로 남게 됐다(Workflow-TestFeedback.cjs handOver)."
  - "headless-ai-testing 작업은 작업 절차에 사람 항목을 코드 리뷰와 화면·소리·조작감·연출처럼 헤드리스로 판정할 수 없는 것만 두고, 임시 테스트 코드·에셋은 결과를 근거에 남긴 뒤 인계 전에 지우도록 적었다."
  - "headless-ai-testing 변경은 워크플로우 Node 테스트 4개, handOver 결함 주입, 문서 링크 검사(40개 문서 오류 0), 헤드리스 Edge 캡처로 확인했고 woogle이 2026-09-27 코드 리뷰를 통과시켰다."
---

# 작업 - headless-ai-testing

- 원본: `.agents/workflow/tasks/headless-ai-testing.md`
- 원자료 사본: `.raw/captured/9bc5aa0c5b27785660500cbc848027e1be0e6353819e8dea249161f8105e6e86.md` (수집 2026-09-27, 재확인 기한 2027-03-25)

## 개요

체크포인트 SaveGame 전환처럼 AI가 헤드리스로 판정할 수 있는 사망·부활·재시작 흐름을 사람 항목으로 넘기는 일이 있어, 테스트 담당 기준을 바꾼 워크플로우 작업 기록이다. AI 대화의 명확한 지시라 구현 승인으로 보고 만들기부터 진행했고, 상태는 완료(체크리스트 4/4 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 요청과 결정

> 이우성 2026-09-27: "우리 프로젝트의 워크플로우 관련해서, 테스트 체크리스트에서 AI가 headless 환경에서 테스트할 수 있는 모든 것은 AI가 끝까지 테스트하고, 도저히 AI가 테스트할 수 없는 부분만 사람이 테스트하도록 합시다."

- 구현 승인: 이우성 2026-09-27(대화: 요청 원문의 지시).

## 구현 결과

- 작업 절차 테스트 체크리스트 절: 헤드리스(화면 없이 AI가 돌리는 빌드·에디터·게임 실행)로 판정할 수 있으면 플레이 흐름이어도 AI 항목이고, AI가 임시 자동화 테스트 등을 만들어 끝까지 테스트한다. 사람 항목은 코드 리뷰와 헤드리스로 판정할 수 없는 것만 두고, 섞인 항목은 나눈다. AI 항목은 미실행·대기로 남기지 않고, 끝내지 못하면 실패와 막힌 이유를 적는다.
- 예시 표·상태 줄 예시에서 「부활 시 적 재생성」을 AI 항목으로 옮기고 사람 항목 예시는 「부활 연출」로 바꿨다.
- 서버 `Workflow-TestFeedback.cjs` handOver: AI가 미실행·대기로 돌려준 AI 항목을 사람 항목으로 넘기던 것을, 담당은 AI 그대로 두고 실패(「AI가 끝내지 못함: 이유」)로 바꿨다. 사람은 실패 확인에서 추가 요청으로 다시 테스트하게 하거나 사람 항목으로 넘긴다.

## 검증 범위

- AI(도구 실행): 워크플로우 Node 테스트 4개 exit 0(끝내지 못한 AI 항목이 담당 AI·실패로 남고 상태가 실패 확인이 되는 단언 추가), handOver를 옛 동작으로 되돌린 결함 주입에서 테스트 실패 후 복원, `CheckDocLinks.ps1` 링크 오류 0(40개 문서), `Export-WorkflowPage` 뒤 헤드리스 Edge 작업 절차 화면 캡처.
- 사람: 코드 리뷰 woogle 2026-09-27 통과.
- 이 작업은 게임 코드·빌드와 무관하다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[작업 - checkpoint-savegame]]
- [[작업 - spawner-library-removal]]

## 핵심 주장

- 이우성은 2026-09-27 테스트 체크리스트에서 AI가 헤드리스 환경에서 테스트할 수 있는 모든 것은 AI가 끝까지 테스트하고, 도저히 AI가 테스트할 수 없는 부분만 사람이 테스트하도록 요청했다. ^c1
- headless-ai-testing 작업으로 웹 처리에서 AI가 미실행·대기로 돌려준 AI 항목은 사람 항목으로 넘어가지 않고 담당 AI 그대로 실패로 남게 됐다(Workflow-TestFeedback.cjs handOver). ^c2
- headless-ai-testing 작업은 작업 절차에 사람 항목을 코드 리뷰와 화면·소리·조작감·연출처럼 헤드리스로 판정할 수 없는 것만 두고, 임시 테스트 코드·에셋은 결과를 근거에 남긴 뒤 인계 전에 지우도록 적었다. ^c3
- headless-ai-testing 변경은 워크플로우 Node 테스트 4개, handOver 결함 주입, 문서 링크 검사(40개 문서 오류 0), 헤드리스 Edge 캡처로 확인했고 woogle이 2026-09-27 코드 리뷰를 통과시켰다. ^c4
