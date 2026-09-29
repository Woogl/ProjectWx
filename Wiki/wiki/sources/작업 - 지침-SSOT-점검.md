---
type: source
title: "작업 - 지침-SSOT-점검"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "지침"
  - "SSOT"
summary: "프로젝트 지침의 SSOT를 점검해 AGENTS.md 절차 요약을 정본 링크로 바꾸고 모듈 리뷰·주석 정리·Git 예외 규칙과 Wiki 도구 버전의 사본을 정리한 2026-09-29 완료 작업 기록"
source_type: task-record
source_id: src-89305af912a4c4ce859e
sha256: 1965e44cb0b39e827752bb3baa83595d8a79e71dfa2747ef3b50ecc7fa967b95
authority: primary
independence_key: ".agents/workflow/tasks/지침-SSOT-점검.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/지침-SSOT-점검.md"
raw_copy: ".raw/captured/1965e44cb0b39e827752bb3baa83595d8a79e71dfa2747ef3b50ecc7fa967b95.md"
claim_ids:
  - clm-1965e44cb0-c1
  - clm-1965e44cb0-c2
  - clm-1965e44cb0-c3
key_claims:
  - "2026-09-29 지침 SSOT 점검으로 AGENTS.md의 절차 요약을 작업 절차 링크로 바꾸고, 모듈 리뷰 기록 보존 규칙을 정본으로 옮기며, 주석 정리의 worklog 금지 충돌을 정본 참조로 바꾸고, Git 스킬·무인 Routine의 실행 범위를 정본에서 구분했다."
  - "같은 작업에서 Wiki 갱신 sparse 사본에 작업 절차 정본을 넣고, claude-obsidian 버전은 Wiki/README.md 설정 스크립트의 태그만 정본으로 삼아 본문의 버전 사본을 없앴다."
  - "지침 SSOT 점검은 문서 링크 검사와 TestWorkflowTestFeedback.cjs로 확인했고, 코드 리뷰는 2026-09-30 사용자 지시로 AI 검토로 대신해 체크리스트 3/3으로 완료됐다."
---

# 작업 - 지침-SSOT-점검

- 원본: `.agents/workflow/tasks/지침-SSOT-점검.md`
- 원자료 사본: `.raw/captured/1965e44cb0b39e827752bb3baa83595d8a79e71dfa2747ef3b50ecc7fa967b95.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

사용자 요청 "이 프로젝트에 지침의 SSOT가 잘 지켜지고 있는지 점검해주시고, 안지켜진 부분 있으면 고쳐주세요."(2026-09-29)로 지침의 중복·충돌과 정본 접근 누락을 정리한 작업 기록이다. 구현은 커밋 `653203d43`에 들어갔다. 상태는 완료(체크리스트 3/3 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 조사에서 찾은 것

- 주석 정리 스킬은 작업별 worklog를 금지해 변경 작업의 기록을 요구하는 정본과 충돌했다.
- 모듈 리뷰의 기록 보존·상태 규칙이 스킬에도 따로 있었다.
- 일반 작업의 커밋 금지와 Git 스킬·기존 무인 실행(일일 Routine)의 경계가 정본에 없었다.
- Wiki 갱신 sparse 사본은 AGENTS.md가 참조하는 작업 절차 정본을 빠뜨렸다.
- Wiki 도구(claude-obsidian) 버전이 `Wiki/README.md` 본문과 설정 스크립트에 중복돼 있었고, 서버는 이미 설정 스크립트 태그를 읽고 있었다.

## 구현 결과

- AGENTS.md의 절차 요약을 작업 절차 링크로 바꾸고 SSOT 원칙은 작업 절차에 둔다.
- 모듈 리뷰의 기록 보존 규칙을 정본으로 옮기고, 주석 정리의 작업 기록 금지 문구를 정본 참조로 바꿨다. Git 스킬·무인 Routine의 실행 범위를 정본에서 구분하고 기존 실행 방법은 유지했다.
- Wiki 갱신 sparse 사본에 작업 절차 정본을 넣고(`Workflow-Server.cjs`), Wiki 도구 버전은 README 설정 스크립트의 태그만 정본으로 삼아 본문의 버전 사본을 없앴다.
- 후속 정리(2026-09-29, 사용자 "네 제거합시다"): 이미 없는 과거 Claude 로그 폴더의 `.gitignore` 제외 규칙을 지웠다. Claude Code 2.1.283의 AGENTS.md 직접 로딩 지원을 확인해 CLAUDE.md 추가 제안을 철회했고, 실제 Claude 세션의 로딩 확인은 하지 않았다.

## 검증 범위

- AI: `CheckDocLinks.ps1` 문서 51개 링크 오류 0, 지침 5개 파일 상대 링크 13개와 제목 앵커 통과, Wiki 버전 태그가 한 곳뿐임 확인, `TestWorkflowTestFeedback.cjs` 종료 코드 0(sparse 초기·재실행, README 태그 사용 포함), `node --check`, `git diff --check`.
- 코드 리뷰는 2026-09-30 사용자 원문 “이 일감도 직접 테스트해보고 문제 없으면 완료처리 합시다.”에 따라 AI 검토로 대신했고, 기록은 사람이 직접 코드 리뷰를 했다고 적지 않는다. 외부 클라우드 Routine 실행이나 실제 Wiki 갱신·푸시는 하지 않았다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]

## 핵심 주장

- 2026-09-29 지침 SSOT 점검으로 AGENTS.md의 절차 요약을 작업 절차 링크로 바꾸고, 모듈 리뷰 기록 보존 규칙을 정본으로 옮기며, 주석 정리의 worklog 금지 충돌을 정본 참조로 바꾸고, Git 스킬·무인 Routine의 실행 범위를 정본에서 구분했다. ^c1
- 같은 작업에서 Wiki 갱신 sparse 사본에 작업 절차 정본을 넣고, claude-obsidian 버전은 Wiki/README.md 설정 스크립트의 태그만 정본으로 삼아 본문의 버전 사본을 없앴다. ^c2
- 지침 SSOT 점검은 문서 링크 검사와 TestWorkflowTestFeedback.cjs로 확인했고, 코드 리뷰는 2026-09-30 사용자 지시로 AI 검토로 대신해 체크리스트 3/3으로 완료됐다. ^c3
