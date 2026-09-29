---
type: meta
title: Hot Cache
status: developing
created: 2026-09-26
updated: 2026-09-29
tags:
  - meta
  - hot-cache
---

# Recent Context

## Last Updated

2026-09-29 · ingest-20260929-task-records-b2

## Key Recent Facts

- 원자료는 기획서 29건(같은 내용 사본 1쌍 포함 30개 파일), 옛 Wiki 결정 노트 66건, 완료 작업 기록 45건이다.
- 주제 페이지 18장이 원자료 요약을 요구사항·확정 결정·구현 관찰·검증 범위·미결정·충돌로 나눠 모은다.
- Wiki 쓰기는 Wiki 갱신 두 갈래(매일 클라우드 Routine, 대시보드에서 고른 AI의 즉시 갱신)만 한다. 결정 이력은 [[Wiki 운영]], 절차 정본은 저장소의 `Wiki/README.md`다.

## Recent Changes

- 새로 완료된 작업 기록 4건을 수집했다: [[작업 - BP-HGTest-개선]], [[작업 - Doppelganger-캐릭터-개선]], [[작업 - viewmodel-quality-cleanup]], [[작업 - viewmodel-mvvm-redesign]].
- 2026-09-30 woogle이 UI 설계 원칙을 채택했다. 플레이어 공유 VM(Character·Inventory)은 로컬 PC가 엔진 Global Collection에 등록하고 WBP는 플레이어 리졸버로 받으며, Inventory VM은 WxUI로 옮겨졌다([[UI 표시 구조]]). 옛 2026-09-23 인벤토리 VM 주장 1건은 deprecated로 보존했다.
- 2026-09-29 도플갱어는 중력 없는 NoCollision 분신이 되어 Master 우측 50cm를 따라가고 몽타주가 끝날 때마다 제자리로 돌아온다([[적 AI와 몬스터]]).
- 2026-09-29 BP_HGTest 궁극기 1·2는 UP 100을 쓰고, 미니언·분신 소환 중 UP를 회복하며, 분신 소환 중에는 전기 루프 GameplayCue가 켜진다([[어빌리티와 GAS]], [[캐릭터 스탯과 전투 자원]]).
- 새로 완료된 작업 기록 6건을 수집했다: [[작업 - workflow-codex-mcp-connection]], [[작업 - workflow-codex-terminal-no-daemon]], [[작업 - workflow-remove-redundant-buttons]], [[작업 - workflow-table-error-recurrence]], [[작업 - 지침-SSOT-점검]], [[작업 - wxcombat-review-quick-fixes]].
- 2026-09-30 `PlayMontageInternal`은 몽타주 동기 재생 실패 때 `false`를 돌려주고, 무기·범위 피해 판정은 공격자 캐릭터에 권위가 없으면 시작하지 않는다([[어빌리티와 GAS]], [[피해 파이프라인]]). WxCombat 리뷰 지적 2(겹친 슬로모션)는 보류됐다.
- 워크플로우: Codex 구현 실행은 연결이 거부된 로컬 unreal-mcp만 빼고, 터미널 이어하기는 Codex에 `--no-daemon`을 붙이며, 표 없는 "추가 질문 없음" 절을 허용하고 표 오류를 대시보드·`--tasks`에 보인다([[작업 절차(Workflow)]]).
- 2026-09-29 지침 SSOT 점검으로 AGENTS.md 절차 요약이 정본 링크로 바뀌고 claude-obsidian 버전은 README 설정 스크립트 태그만 정본이 됐다([[Wiki 운영]]).
- 2026-09-28 GetAbilityBlockTags·ActivationGroup이 삭제되고 액션은 Ability.Action 태그 아래로 옮겨졌다. 차단은 타입 생성자의 BlockAbilitiesWithTag가 정한다([[어빌리티와 GAS]]).

## Active Threads

- 2026-09-30 UI 설계 원칙의 알려진 예외인 자막 뷰모델과 WxGame InteractionList 뷰모델은 후속 일감으로 남았다([[UI 표시 구조]]).
- MCP 연결 수정은 사전 확인 뒤 서버가 꺼지는 경우를 다루지 않고, 터미널 이어하기 첫 시도 한 번의 실패 원인은 확인되지 않았다([[작업 절차(Workflow)]]).
- 콤보 반복 검사 중 첫 입력 누락 1회는 원인 미확인이며 추가 로그 뒤 15회 재현되지 않았다. ([[작업 - combo-stage-desync-after-rejection]])
- 피해 파이프라인의 상태 전이 순서 정리와 가드 방향 등 기획 확인 항목은 기록에 대기로 남아 있다([[피해 파이프라인]]).
- 기획서 캐릭터 스탯 명세서의 SPD 정의를 MOV(cm/s)로 고칠지는 기획서 쪽 결정이 남아 있다.
- 다음 원자료 재확인 기한은 전투·콘텐츠 초안 2026-11-25, 개별 시스템 규격 2026-12-25, 초기 기획·역기획·완료 작업 기록 2027-03-25~28, 옛 결정 노트 2027-09-26이다.
