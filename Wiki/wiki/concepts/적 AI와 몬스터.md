---
type: concept
title: "적 AI와 몬스터"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "일반·정예 몬스터 규격, BT·AI 제어, 순찰"
sources:
  - "[[작업 - ability-table-driven]]"
---

# 적 AI와 몬스터

일반·정예 몬스터 규격, BT·AI 제어, 순찰을(를) 원자료별 요약에서 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- ABS_Soldier의 속성 행은 지금 동작을 유지하도록 TemplateEnemy로 재연결하기로 사용자가 정했고, 분신과 도플갱어는 함께 존재할 수 없다고 사용자가 확인했다. ([[작업 - ability-table-driven]])

## 구현 관찰

- BT 노드(ActivateAbility·ObserveAbility·MirrorMovement)는 GA_ 복귀 후 스펙 동적 태그 없이 CDO 에셋 태그로 어빌리티를 고르며, 솔저 BT가 Ability.Pattern.N 에셋 태그로 패턴을 발동함을 PIE로 확인했다. ([[작업 - ability-table-driven]])

## 검증 범위

- 아직 없음

## 미결정·충돌

- 아직 없음

## 원자료

- [[작업 - ability-table-driven]] — 어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다.
