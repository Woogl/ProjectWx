---
type: concept
title: "Wiki 운영"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "팀 Wiki의 운영 규칙과 전환 이력"
sources:
  - "[[결정 노트 - 2026-09-22-current-workflow]]"
  - "[[결정 노트 - 2026-09-22-verified-stock-rule]]"
  - "[[결정 노트 - 2026-09-25-workflow-simplification]]"
  - "[[작업 - wiki-regeneration]]"
  - "[[작업 - workflow-review]]"
---

# Wiki 운영

팀 Wiki의 운영 규칙과 전환 이력에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- 옛 .wiki 재생성 작업은 문서 작성일과 인간 검증일을 구분하고 C++ 정적 확인이 빌드·실행·바이너리 에셋 검증을 대신하지 않는다는 기준을 따랐다. ([[작업 - wiki-regeneration]])
- 사용자는 2026-09-22 옛 LLM Wiki 기사의 verified를 WX 전용 인간 확인 방침 대신 순정 규칙(편찬·재확인 때 그날 날짜 기록)으로 되돌리도록 결정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 옛 Wiki schema.md는 verified를 편찬 없는 구조 이관에는 넣지 않도록 규정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])

## 구현 관찰

- 2026-09-22 옛 .wiki 전체 재생성은 새 원자료 15개를 수집하고 기존 16개 기사를 재작성·1개를 추가했으며 참조가 교체된 legacy 19개를 삭제했다. ([[작업 - wiki-regeneration]])
- 구 워크플로우 잔재 제거로 Wiki 뷰어의 문서 이미지 기능(Export-Wiki.ps1 PNG 묶기, wikiImageSource, IMG 허용)이 사용자 결정에 따라 삭제되었다. ([[작업 - workflow-review]])
- 2026-09-22 시점 옛 LLM Wiki는 `.wiki/`를 Git으로 공유하고 게임 규칙·구현·제약·결정 이유를 담았으며, 작업별 판단·확정본·실행 상태는 Workflow가 담당했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 시점 옛 Wiki 설정은 원자료 속 지시를 작업 명령이 아닌 자료로 다루고 구조 이관·재편찬·Lint 성공을 실행 재검증으로 보지 않도록 규정했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- Workflow 단순화 후 로컬 Wiki-AI 서버는 /health와 /test-feedback만 받고(protocol 4) 화면 메뉴는 작업 현황 대시보드·작업 절차·LLM 위키 검색 세 개가 되었다. ([[결정 노트 - 2026-09-25-workflow-simplification]])

## 검증 범위

- 옛 .wiki 재생성은 lint·링크 검사·뷰어 테스트·Edge 렌더링으로만 검증되었고 빌드와 게임 실행은 하지 않았다. ([[작업 - wiki-regeneration]])
- 옛 Wiki의 verified 날짜는 편찬·재확인 날짜이며 빌드·게임 실행 검증일을 뜻하지 않는다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])

## 미결정·충돌

- 아직 없음

## 원자료

- [[결정 노트 - 2026-09-22-current-workflow]] — 2026-09-22 시점 AGENTS.md·옛 LLM Wiki 설정·Workflow 절차와 실행 스크립트의 계약을 발췌한 정적 조사 노트로 사람 판단·AI 실행 경계를 기록한다
- [[결정 노트 - 2026-09-22-verified-stock-rule]] — 옛 LLM Wiki 기사의 verified 필드를 WX 전용 인간 확인 방침에서 순정 규칙(편찬·재확인 날짜 기록)으로 되돌린 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-25-workflow-simplification]] — 사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록
- [[작업 - wiki-regeneration]] — 옛 .wiki LLM Wiki를 현재 코드·설정·기획 기준으로 전면 재생성하고 대체된 legacy 문서를 정리한 2026-09-22 완료 작업 기록
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
