---
type: concept
title: "UI 표시 구조"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "VM·리졸버·Nameplate 등 화면 표시 연결 구조"
sources:
  - "[[작업 - cooldown-unification]]"
---

# UI 표시 구조

VM·리졸버·Nameplate 등 화면 표시 연결 구조을(를) 원자료별 요약에서 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- 아직 없음

## 구현 관찰

- 쿨다운 통합 뒤 어빌리티 슬롯 VM은 DynamicGrantedTags로 쿨다운 GE를 세고 주기는 IWxUIData::GetCooldownTime()으로 읽는다. ([[작업 - cooldown-unification]])

## 검증 범위

- 아직 없음

## 미결정·충돌

- 아직 없음

## 원자료

- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
