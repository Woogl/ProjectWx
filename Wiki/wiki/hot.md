---
type: meta
title: Hot Cache
status: developing
created: 2026-09-26
updated: 2026-09-26
tags:
  - meta
  - hot-cache
---

# Recent Context

## Last Updated

2026-09-26 · ingest-20260926T055103Z-b09

## Key Recent Facts

- [[결정 노트 - 2026-09-25-checkpoint-single-slot]]: 사용자 결정으로 PIE와 일반 플레이가 WxCheckpoint 저장 슬롯 하나를 공유하고 슬롯 세분화는 나중으로 미룬 기록
- [[결정 노트 - 2026-09-25-cooldown-single-ge]]: 쿨다운 그룹별 GE 파생 클래스를 공용 UWxEffect_Cooldown 하나로 합치고 어빌리티 CooldownTags와 SetByCaller 대기열로 충전을 차례 회복시킨 결정
- [[결정 노트 - 2026-09-25-exclusive-submission-cleanup]]: 사용자 요청으로 Exclusive 차단의 임시 C++ 테스트·검증 스크립트를 제거하고 차단 관련 주석을 태그 차단·취소 면역 기준으로 정정한 기록
- [[결정 노트 - 2026-09-25-exclusive-tag-blocking]]: Exclusive 발동 차단을 순정 BlockAbilitiesWithTag로 옮기고 콤보 자기 재발동과 도플갱어 소유자 발동 조건 면제 범위를 정한 결정과 검증
- [[결정 노트 - 2026-09-25-row-preview-empty-struct]]: DataTable Row 미리보기에서 초기 기본값과 같은 구조체 셀을 {}로 축약하도록 한 사용자 요청과 구현 관찰, 컴파일만 통과한 상태
- [[결정 노트 - 2026-09-25-row-preview-tooltip]]: 사용자 후속 요청으로 Row 미리보기의 축약 셀과 툴팁에 같은 텍스트를 표시하도록 바꾸어 원문 툴팁 유지 정책을 대체한 기록
- [[결정 노트 - 2026-09-25-save-checkpoint-rename]]: 사용자 후속 요청으로 RecordCheckpoint API와 StateTree 태스크를 SaveCheckpoint로 이름을 바꾸고 CoreRedirects로 기존 구조체 경로를 이어 준 기록
- [[결정 노트 - 2026-09-25-spawner-library-removal]]: 사용자 요청으로 UWxSpawnerLibrary를 제거하고 AWxSpawner::RespawnAll C++ 전용 함수로 플레이어 부활·StateTree 일괄 재생성을 연결한 기록
- [[결정 노트 - 2026-09-25-ui-data-interface-removal]]: IWxUIData 인터페이스를 제거하고 WxGame 리졸버가 어빌리티·GE 데이터를 WxUI VM에 전달하도록 모듈 책임을 나눈 사용자 합의와 구현 관찰
- [[결정 노트 - 2026-09-25-workflow-convenience-review]]: 단순화한 Workflow를 AI·사람 관점에서 점검한 뒤 사용자가 식별값 검사 제거·코드 리뷰 체크리스트화·옛 기록 정리·작업 현황 자동 생성 네 개선을 승인한 기록
- [[결정 노트 - 2026-09-25-workflow-simplification]]: 사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록

## Recent Changes

- [[결정 노트 - 2026-09-25-checkpoint-single-slot]]
- [[결정 노트 - 2026-09-25-cooldown-single-ge]]
- [[결정 노트 - 2026-09-25-exclusive-submission-cleanup]]
- [[결정 노트 - 2026-09-25-exclusive-tag-blocking]]
- [[결정 노트 - 2026-09-25-row-preview-empty-struct]]
- [[결정 노트 - 2026-09-25-row-preview-tooltip]]
- [[결정 노트 - 2026-09-25-save-checkpoint-rename]]
- [[결정 노트 - 2026-09-25-spawner-library-removal]]
- [[결정 노트 - 2026-09-25-ui-data-interface-removal]]
- [[결정 노트 - 2026-09-25-workflow-convenience-review]]
- [[결정 노트 - 2026-09-25-workflow-simplification]]

## Active Threads

- 정기 갱신 Routine이 매일 06:30(KST) 기획서·완료 작업 기록의 바뀐 것을 수집한다.
- 주제 페이지의 구현 관찰은 원자료 작성 시점 기준이며, 게임 동작 검증이 아니다.
