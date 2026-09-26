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
  - "[[작업 - wiki-regeneration]]"
  - "[[작업 - workflow-review]]"
---

# Wiki 운영

팀 Wiki의 운영 규칙과 전환 이력에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- 옛 .wiki 재생성 작업은 문서 작성일과 인간 검증일을 구분하고 C++ 정적 확인이 빌드·실행·바이너리 에셋 검증을 대신하지 않는다는 기준을 따랐다. ([[작업 - wiki-regeneration]])

## 구현 관찰

- 2026-09-22 옛 .wiki 전체 재생성은 새 원자료 15개를 수집하고 기존 16개 기사를 재작성·1개를 추가했으며 참조가 교체된 legacy 19개를 삭제했다. ([[작업 - wiki-regeneration]])
- 구 워크플로우 잔재 제거로 Wiki 뷰어의 문서 이미지 기능(Export-Wiki.ps1 PNG 묶기, wikiImageSource, IMG 허용)이 사용자 결정에 따라 삭제되었다. ([[작업 - workflow-review]])

## 검증 범위

- 옛 .wiki 재생성은 lint·링크 검사·뷰어 테스트·Edge 렌더링으로만 검증되었고 빌드와 게임 실행은 하지 않았다. ([[작업 - wiki-regeneration]])

## 미결정·충돌

- 아직 없음

## 원자료

- [[작업 - wiki-regeneration]] — 옛 .wiki LLM Wiki를 현재 코드·설정·기획 기준으로 전면 재생성하고 대체된 legacy 문서를 정리한 2026-09-22 완료 작업 기록
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
