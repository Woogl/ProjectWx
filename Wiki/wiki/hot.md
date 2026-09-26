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

2026-09-26 · ingest-20260926T055059Z-b08

## Key Recent Facts

- [[결정 노트 - 2026-09-24-module-principles]]: Wx 플러그인 모듈화 목적을 재사용에서 게임 내부 도메인 경계 강제로 다시 정하고 책임 기반 배치 원칙을 확정한 대화 기록
- [[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]: NameplateManager를 WxUI에서 WxGame 컨트롤러로 옮겨 적과 락온 대상을 직접 읽게 하고 LockOnTargetQuery·마커 컴포넌트·옛 리다이렉트를 지운 기록
- [[결정 노트 - 2026-09-24-nameplate-manager]]: 적마다 위젯을 만들던 Nameplate와 락온 태스크의 레티클 생성을 없애고 플레이어 컨트롤러의 NameplateManager가 로컬에서 붙이고 떼게 한 구조 기록
- [[결정 노트 - 2026-09-24-wxcombat-cleanup]]: 구간 GE 노티파이가 자기 핸들만 걷게 하고 처형 피해를 처형 어빌리티가 직접 적용하며 퍼펙트 가드 Cue를 Hit Cue로 통합한 WxCombat 정리 네 건 기록
- [[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]]: 소환물 쿨다운 무시를 순정 GE 컴포넌트로 바꾸고 사망 BT 정지를 AI 컨트롤러로 단일화하며 락온 대상 사본을 없앤 WxCombat 장치 정리 기록
- [[결정 노트 - 2026-09-24-wxcore-cleanup]]: 읽는 곳 없는 Damage.Guarded·Device.Locked 태그와 빈 FWxCoreModule, WxAI 자동화 테스트 두 개를 지운 WxCore 정리 기록
- [[결정 노트 - 2026-09-25-ability-block-policy-centralization]]: 어빌리티 자식 생성자의 공통 차단 코드를 지우고 ActivationGroup·태그 기반 계산을 ASC ApplyAbilityBlockAndCancelTags 확장 지점으로 통합한 결정과 검증 기록
- [[결정 노트 - 2026-09-25-ability-data-on-ga]]: 어빌리티 테이블 구동을 시도했다가 AI 작업 편의를 기준으로 데이터 전용 GA_로 돌아가고 DT_Ability·DT_Effect를 지운 결정과 구현·검증 기록
- [[결정 노트 - 2026-09-25-animnotify-labels]]: AnimNotify 17종의 타임라인 라벨을 종류와 대표 값 하나 형식으로 줄이기로 한 사용자 합의와 GetNotifyName 구현 근거를 정적으로 정리한 기록
- [[결정 노트 - 2026-09-25-checkpoint-redirect-cleanup]]: ST_CheckPoint를 SaveCheckpoint 구조체로 리세이브한 뒤 CoreRedirects 두 항목을 지우고 새 프로세스에서 재로드·컴파일·저장을 확인한 기록
- [[결정 노트 - 2026-09-25-checkpoint-savegame]]: UWxCheckpointSubsystem을 없애고 체크포인트 기록·부활 조회·새 게임 초기화를 USaveGame 디스크 슬롯으로 옮긴 작업의 정적 확인 기록

## Recent Changes

- [[결정 노트 - 2026-09-24-module-principles]]
- [[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]
- [[결정 노트 - 2026-09-24-nameplate-manager]]
- [[결정 노트 - 2026-09-24-wxcombat-cleanup]]
- [[결정 노트 - 2026-09-24-wxcombat-machinery-cleanup]]
- [[결정 노트 - 2026-09-24-wxcore-cleanup]]
- [[결정 노트 - 2026-09-25-ability-block-policy-centralization]]
- [[결정 노트 - 2026-09-25-ability-data-on-ga]]
- [[결정 노트 - 2026-09-25-animnotify-labels]]
- [[결정 노트 - 2026-09-25-checkpoint-redirect-cleanup]]
- [[결정 노트 - 2026-09-25-checkpoint-savegame]]

## Active Threads

- 정기 갱신 Routine이 매일 06:30(KST) 기획서·완료 작업 기록의 바뀐 것을 수집한다.
- 주제 페이지의 구현 관찰은 원자료 작성 시점 기준이며, 게임 동작 검증이 아니다.
