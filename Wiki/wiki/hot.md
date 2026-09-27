---
type: meta
title: Hot Cache
status: developing
created: 2026-09-26
updated: 2026-09-27
tags:
  - meta
  - hot-cache
---

# Recent Context

## Last Updated

2026-09-27 · ingest-20260927T2200Z-task-records

## Key Recent Facts

- 원자료는 기획서 29건(같은 내용 사본 1쌍 포함 30개 파일), 옛 Wiki 결정 노트 66건, 완료 작업 기록 25건이다.
- 주제 페이지 18장이 원자료 요약을 요구사항·확정 결정·구현 관찰·검증 범위·미결정·충돌로 나눠 모은다.
- Wiki 쓰기는 Wiki 갱신 두 갈래(매일 클라우드 Routine, 대시보드에서 고른 AI의 즉시 갱신)만 한다. 결정 이력은 [[Wiki 운영]], 절차 정본은 저장소의 `Wiki/README.md`다.

## Recent Changes

- 새로 완료된 작업 기록 6건을 수집했다: [[작업 - 이동속도의-어트리뷰트화]], [[작업 - checkpoint-savegame]], [[작업 - spawner-library-removal]], [[작업 - headless-ai-testing]], [[작업 - workflow-wrapup-checks]], [[작업 - comment-cleanup-routine]].
- 이동 속도 어트리뷰트가 SPD(배율)에서 MOV(cm/s, DT_CharacterAttribute MOV 열)로 바뀌었다. 기획서 캐릭터 스탯 명세서는 아직 SPD 배율로 정의한다([[캐릭터 스탯과 전투 자원]] 미결정·충돌).
- 체크포인트 저장·부활·재시작·실패 처리와 스포너 재생성이 2026-09-27 헤드리스 자동화 테스트로 확인됐고, 부활 위치 컴포넌트 참조 결함(항상 PlayerStart 부활)이 고쳐졌다([[체크포인트와 리스폰]]).
- 헤드리스로 판정할 수 있는 테스트는 AI가 끝까지 하고, AI가 끝내지 못한 AI 항목은 AI 실패로 남는다([[작업 절차(Workflow)]]).
- 워크플로우 개선 마무리 확인 다섯 항목이 모두 통과해 워크플로우 개선 작업이 닫혔다. 일일 주석 정리 Routine의 절차는 comment-cleanup 스킬 6절로 옮겨졌다.

## Active Threads

- 기획서 캐릭터 스탯 명세서의 SPD 정의를 MOV(cm/s)로 고칠지는 기획서 쪽 결정이 남아 있다.
- 다음 원자료 재확인 기한은 전투·콘텐츠 초안 2026-11-25, 개별 시스템 규격 2026-12-25, 초기 기획·역기획·완료 작업 기록 2027-03-25, 옛 결정 노트 2027-09-26이다.
