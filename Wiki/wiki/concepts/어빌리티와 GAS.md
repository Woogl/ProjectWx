---
type: concept
title: "어빌리티와 GAS"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "GA·GE·쿨다운·차단 태그 등 GAS 어빌리티 구조"
sources:
  - "[[작업 - ability-table-driven]]"
  - "[[작업 - cooldown-unification]]"
---

# 어빌리티와 GAS

GA·GE·쿨다운·차단 태그 등 GAS 어빌리티 구조을(를) 원자료별 요약에서 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다.

## 요구사항

- 아직 없음

## 확정 결정

- 어빌리티는 테이블 구동 대신 어빌리티마다 구체 타입 C++을 부모로 하는 데이터 전용 GA_ 에셋 하나를 두기로 사용자가 2026-09-25 확정했으며, 규칙 칸 잠금과 BP 그래프 로직 금지는 관례로 둔다. ([[작업 - ability-table-driven]])
- 공격 어빌리티는 UWxAbility_Attack_Light·Heavy·Air·DodgeCounter 네 타입으로 나누고 캐릭터 공통 발동 조건은 타입 규칙에 둔다. ([[작업 - ability-table-driven]])
- 행이 에셋과 1:1인 DT_Effect는 제거하고 값은 GE 에셋에 두며, 여러 노티파이·투사체가 행을 골라 쓰는 DT_Damage는 유지한다. ([[작업 - ability-table-driven]])
- 쿨다운은 그룹별 GE 파생 클래스 대신 공용 UWxEffect_Cooldown 하나와 어빌리티의 FGameplayTagContainer CooldownTags로 구분하기로 사용자가 승인했다. ([[작업 - cooldown-unification]])
- Ability.Pattern.N 태그는 BT 재정비에 필요할 수 있어 유지하고 Ability.Skill.N 제거는 BT 재정비 때 하기로 사용자가 정했다. ([[작업 - cooldown-unification]])

## 구현 관찰

- GA_ 복귀 후 BT가 부르는 GA_는 엔진 AssetTags에 Ability.Skill.N·Ability.Pattern.N 번호 태그를 더하고, 스펙에 SourceObject나 동적 태그를 싣지 않는다. ([[작업 - ability-table-driven]])
- 콤보·패턴 몽타주는 번호 섹션 1, 2…를 가진 몽타주 하나로 병합되었고 단계 전환은 같은 몽타주를 다음 섹션부터 새로 재생한다. ([[작업 - ability-table-driven]])
- ApplyCooldown은 CooldownTags를 스펙 DynamicGrantedTags에 붙이고, 같은 태그의 최대 남은 시간에 CooldownTime을 더한 값을 SetByCaller.Duration으로 넘겨 충전을 차례로 회복시킨다. ([[작업 - cooldown-unification]])
- 공용 쿨다운 GE는 엔진 WarnCooldownEffectWithoutTags 규칙을 만족하도록 Cooldown 부모 태그를 부여하고, 빈 CooldownTags면 CheckCooldown이 먼저 통과시킨다. ([[작업 - cooldown-unification]])

## 검증 범위

- GA_ 복귀는 Development·DebugGame 빌드, GA_ 40개·세트 9개 데이터 검증 커맨드릿, 단독·리슨 서버 PIE로 AI가 확인했고, 사람이 HGTest·분신·도플갱어·조작감·가드 경감·패시브 UP·락온을 인게임으로 통과 확인했다. ([[작업 - ability-table-driven]])
- 쿨다운 통합은 WxEditor Development 빌드와 GA_ 40개 데이터 검증을 통과했고, 사람이 회피 쿨다운·UI, 소환물 쿨다운 무시, 리슨 서버 네트워크 복제를 인게임으로 확인했다. ([[작업 - cooldown-unification]])

## 미결정·충돌

- 어빌리티 클래스·데이터·몽타주 사이 암묵적 계약을 드러내는 방법(선언과 편집 화면 표시 등)은 미결로 남았고, 타입별 몽타주 검증기는 제거된 채 필요 시 재검토하기로 했다. ([[작업 - ability-table-driven]])

## 원자료

- [[작업 - ability-table-driven]] — 어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다.
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
