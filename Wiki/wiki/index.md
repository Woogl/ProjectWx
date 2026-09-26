---
type: meta
title: Wiki Index
status: evergreen
created: 2026-09-26
updated: 2026-09-26
tags:
  - meta
  - index
---

# Wiki Index

WX의 게임 규칙·구현·결정을 모은 claude-obsidian vault입니다. 정기 갱신 Routine만 씁니다. 큰 그림은 [[overview]], 최근 맥락은 [[hot]], 이력은 [[log]]에 있습니다.

## 주제

- [[UI 표시 구조]] — VM·리졸버·Nameplate 등 화면 표시 연결 구조
- [[모듈 구조와 코드 정리]] — 모듈 배치 원칙과 모듈별 정리 결정
- [[어빌리티와 GAS]] — GA·GE·쿨다운·차단 태그 등 GAS 어빌리티 구조
- [[에디터 도구]] — AnimNotify 분류·라벨, DataTable 행 미리보기·참조 갱신 등 편집기 기능
- [[적 AI와 몬스터]] — 일반·정예 몬스터 규격, BT·AI 제어, 순찰

## 원자료: 기획서

- 아직 없음

## 원자료: 작업 기록

- [[작업 - ability-table-driven]] — 어빌리티를 DataTable 행으로 구동하려던 전환 작업 기록으로, 여러 단계 구현 끝에 GA_ 에셋 방식으로 복귀해 테이블화 없이 체크리스트 7/7 통과로 마무리됐다.
- [[작업 - animnotify-categories]] — AnimNotify 17종을 6개 분류 색상으로 묶고 공용 색상 설정을 WxCore의 에디터 전용 설정으로 옮긴 작업 기록으로, 체크리스트 6/6 통과로 완료됐다.
- [[작업 - animnotify-labels]] — AnimNotify 17종의 타임라인 표시 이름을 종류: 대표 값 형식의 짧은 라벨로 바꾼 작업 기록으로, 사람 확인 2/2 통과로 완료됐다.
- [[작업 - cooldown-unification]] — 쿨다운 그룹별 UWxEffect_Cooldown 파생 클래스를 공용 GE 하나로 통합하고 CooldownTags로 구분하게 바꾼 작업 기록으로, 사람 확인 4/4 통과로 완료됐다.
- [[작업 - datatable-row-preview]] — DataTable 행 미리보기에서 데이터 없는 구조체를 {}로 축약하고 셀과 툴팁을 같은 텍스트로 맞춘 에디터 작업 기록으로, 사람 확인 3/3 통과로 완료됐다.
- [[작업 - datatable-row-rename-reference-update]] — DataTable 행 이름 변경 시 FDataTableRowHandle 참조를 자동 갱신하는 에디터 플러그인 DataTableRowFixup을 추가한 작업으로 2026-09-23 완료됐다.

## 원자료: 결정 노트

- 아직 없음
