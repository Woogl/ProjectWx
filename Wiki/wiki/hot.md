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

2026-09-26 · ingest-20260926T055050Z-b07

## Key Recent Facts

- [[결정 노트 - 2026-09-23-datatable-row-fixup]]: DataTable 행 이름을 바꾸면 그 행을 가리키던 FDataTableRowHandle을 자동 갱신하는 범용 에디터 플러그인 DataTableRowFixup의 결정·계약·검증 기록.
- [[결정 노트 - 2026-09-23-dialogue-presentation-vm]]: Dialogue VM을 WxUI의 순수 표시 데이터로 만들고 세션 연결은 WxGame Resolver, 진행 입력은 화면이 맡게 한 모듈 경계 결정과 정적 확인 기록.
- [[결정 노트 - 2026-09-23-hit-processing-functions]]: UWxEffectComponent_Hit 안에서 방어 판정·Spec 준비·결과 기반 반응을 새 타입 없이 함수 단위로 분리한 구조 개선 기록.
- [[결정 노트 - 2026-09-23-interaction-list-vm]]: 상호작용 스캐너 신호를 OnRowsChanged 하나로 합쳐 목록 VM이 행 VM을 전부 재생성하게 한 구조와 문구 출처 원칙에 대한 사용자 결정·정적 조사 기록.
- [[결정 노트 - 2026-09-23-item-viewmodel-unification]]: WxGame 인벤토리 아이템 VM을 WxUI 아이템 VM으로 단일화하고 PC당 공유 인벤토리 VM이 값을 공급하게 한 결정, MVVM 변환 함수 제약, WxToolset 도구 기록.
- [[결정 노트 - 2026-09-23-player-screen-owner]]: 사망·대화 화면 클래스와 태그 관찰을 UIManager 서브시스템·전역 설정에서 컨트롤러 BP의 UWxPlayerLayoutComponent로 옮긴 결정과 정적 조사 기록.
- [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]: UWxDialogueScreen·UWxQuestTracker C++ 위젯 클래스를 제거하고 WBP가 WxGame 리졸버가 연결한 WxUI 뷰모델로 구동되게 한 사용자 결정과 구현 기록.
- [[결정 노트 - 2026-09-23-zero-damage-hitstop]]: 히트스톱을 Hit Cue와 같은 조건(피해 0 초과 또는 퍼펙트 가드)으로 맞추고 Hit Cue 예측 발행 등 낡은 주석을 정정한 기록. 빌드 통과, 플레이 미검증.
- [[결정 노트 - 2026-09-24-ai-brain-control-single-owner]]: AI 비헤이비어 트리 정지·잠금을 AWxAIController 하나로 모은 결정. 사망은 StopLogic, 그로기는 Reaction 우선순위 리소스 잠금, 돌진은 브레인을 건드리지 않는다.
- [[결정 노트 - 2026-09-24-device-statetree-cleanup]]: 장치 StateTree 정리 기록. 복원 판정을 IsRestoring으로 옮기고 중복 장치를 지우며 몽타주 태스크를 WxCombat으로 이관하고 연출 태스크를 고쳤다.
- [[결정 노트 - 2026-09-24-interaction-contract-options-only]]: IWxInteractable의 CanInteract·GetInteractionPrompt를 없애고 GetInteractionOptions 하나로 자격과 문구를 답하게 한 상호작용 계약 통합 기록.

## Recent Changes

- [[결정 노트 - 2026-09-23-datatable-row-fixup]]
- [[결정 노트 - 2026-09-23-dialogue-presentation-vm]]
- [[결정 노트 - 2026-09-23-hit-processing-functions]]
- [[결정 노트 - 2026-09-23-interaction-list-vm]]
- [[결정 노트 - 2026-09-23-item-viewmodel-unification]]
- [[결정 노트 - 2026-09-23-player-screen-owner]]
- [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]
- [[결정 노트 - 2026-09-23-zero-damage-hitstop]]
- [[결정 노트 - 2026-09-24-ai-brain-control-single-owner]]
- [[결정 노트 - 2026-09-24-device-statetree-cleanup]]
- [[결정 노트 - 2026-09-24-interaction-contract-options-only]]

## Active Threads

- 정기 갱신 Routine이 매일 06:30(KST) 기획서·완료 작업 기록의 바뀐 것을 수집한다.
- 주제 페이지의 구현 관찰은 원자료 작성 시점 기준이며, 게임 동작 검증이 아니다.
