---
type: overview
title: Vault Overview
status: developing
created: 2026-09-26
updated: 2026-09-26
tags:
  - overview
---

# Vault Overview

WX(Unreal Engine 5 오픈월드 액션 RPG)의 기획 요구사항, 사람의 확정 결정, 코드 구현 관찰, 검증 범위를 모은 claude-obsidian vault입니다. 쓰기는 claude-obsidian 순정 트랜잭션으로 하는 Wiki 갱신(매일 클라우드 Routine 「Wiki 정기 갱신」과 Workflow 대시보드에서 고른 AI의 **Wiki 갱신**)만 하고, 사람은 Obsidian에서 읽기만 합니다. 갱신 절차는 저장소의 `Wiki/README.md`에 있고, 전환과 운영 결정의 이력은 [[Wiki 운영]]에 있습니다.

## 구성

- 원자료 페이지(`wiki/sources/`): 수집한 원본 하나마다 한 장입니다. 이름 앞머리로 종류를 구분합니다.
  - `기획서 - …`: `Docs/CombatDesign`·`Docs/SystemDesign`·`Docs/LevelDesign`의 Markdown 기획서(요구사항).
  - `작업 - …`: 상태가 완료인 작업 기록(`.agents/workflow/tasks/`). 요청·질문과 답변·구현 결과·테스트 체크리스트를 담습니다.
  - `결정 노트 - …`: 옛 LLM Wiki(`.wiki/raw/notes/`)에서 옮겨 온, 사용자 결정 원문이 있는 조사·결정 노트. 노트 날짜 기준의 기록이라 지금 코드와 다를 수 있습니다.
- 주제 페이지(`wiki/concepts/`): 원자료 요약을 주제별로 모으고 요구사항·확정 결정·구현 관찰·검증 범위·미결정·충돌로 나눕니다. 문장 끝 링크가 출처입니다.
- 목록은 [[index]], 최근 맥락은 [[hot]], 갱신 이력은 [[log]]에 있습니다.

## 읽을 때 주의

- 기획서끼리 충돌하는 규칙이 많습니다(예: 패링·가드, 자원 체계, 스킬 키). 각 주제 페이지의 미결정·충돌 절을 먼저 봅니다.
- 구현 관찰은 원자료가 쓰인 시점의 정적 확인입니다. 빌드 통과, 사람의 인게임 확인과 구분해 적었고, 문서 갱신이나 lint 통과는 게임 동작 검증이 아닙니다.
- 원자료 사본은 `.raw/captured/<SHA-256>` 이름으로 보관하고 레저(`wiki/meta/ledgers/`)가 출처와 주장을 추적합니다.
