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
  - "[[기획서 - Nameplate_System]]"
  - "[[작업 - cooldown-unification]]"
---

# UI 표시 구조

VM·리졸버·Nameplate 등 화면 표시 연결 구조에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- Nameplate_System 기획서는 일반/네임드 몬스터 네임플레이트에 HP·DP 게이지를 좌측 기준 잔량 방식으로 표시하고, 이름은 표시하지 않으며, 디버프는 하단에 쿨타임 형식으로 표시하도록 요구한다. ([[기획서 - Nameplate_System]])
- Nameplate_System 기획서는 네임플레이트를 숨김 상태로 시작해 인식 또는 카메라 락온 시 표시하고, 추적 종료 시 페이드 없이 즉시 숨기도록 요구한다. ([[기획서 - Nameplate_System]])
- Nameplate_System 기획서는 한번 표시된 네임플레이트를 벽에 가려져도 가림 처리 없이 계속 보여 주고, 동시 표시 상한 없이 겹치면 카메라에 가까운 적을 위에 그리도록 요구한다. ([[기획서 - Nameplate_System]])

## 확정 결정

- 아직 없음

## 구현 관찰

- 쿨다운 통합 뒤 어빌리티 슬롯 VM은 DynamicGrantedTags로 쿨다운 GE를 세고 주기는 IWxUIData::GetCooldownTime()으로 읽는다. ([[작업 - cooldown-unification]])

## 검증 범위

- 아직 없음

## 미결정·충돌

- Nameplate_System 기획서의 락온 시 표시 규칙은 은폐된 적을 락온 대상에서 빼는 잠정안이며 협의 예정 상태다. ([[기획서 - Nameplate_System]])

## 원자료

- [[기획서 - Nameplate_System]] — 일반·네임드 몬스터 네임플레이트의 HP·DP 표시 구성, 시야·청각·피격 인식 규칙, 표시·숨김 조건과 보스 전용 규칙을 정의한 기획서
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
