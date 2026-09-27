---
type: source
title: "작업 - checkpoint-savegame"
created: 2026-09-27
updated: 2026-09-27
status: developing
tags:
  - "source"
  - "작업-기록"
  - "체크포인트"
summary: "UWxCheckpointSubsystem을 SaveGame 슬롯으로 바꾼 체크포인트 작업 기록으로, 2026-09-27 헤드리스 자동화 테스트로 사망·부활·재시작·실패 처리를 확인하고 부활 위치 컴포넌트 참조 결함을 고쳐 체크리스트 4/4 통과로 완료됐다."
source_type: task-record
source_id: src-5b9c25cebf177d2df0e0
sha256: cb8ffbe580deeced4b5d1f4d3e3063365b7607d7f1bfc9b64507dac59e7fdcb4
authority: primary
independence_key: ".agents/workflow/tasks/checkpoint-savegame.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/checkpoint-savegame.md"
raw_copy: ".raw/captured/cb8ffbe580deeced4b5d1f4d3e3063365b7607d7f1bfc9b64507dac59e7fdcb4.md"
claim_ids:
  - clm-8cd541d961-c1
  - clm-8cd541d961-c2
  - clm-8cd541d961-c3
  - clm-8cd541d961-c4
  - clm-8cd541d961-c5
key_claims:
  - "2026-09-27 헤드리스 게임(LV_DevCombat -game -nullrhi) 임시 자동화 테스트에서 실제 BP_CheckPoint에서 쉰 뒤 사망해 사망 화면의 RequestRespawn으로 부활하면 체크포인트 위치에서 부활하고, 새 게임은 체크포인트 슬롯을 지웠다."
  - "2026-09-27 헤드리스 테스트에서 PIE 이름 월드가 저장한 WxCheckpoint 슬롯을 새 프로세스의 일반 월드가 읽었고, 재시작한 게임에서 사망하면 저장된 체크포인트에서 부활했다."
  - "체크포인트 저장 실패 시 태스크 실패와 이전 체크포인트 유지, 슬롯 없음·다른 레벨·빈 파일·잘린 파일의 체크포인트 없음 처리, 새 게임의 슬롯 삭제 실패 시 이동 중단은 2026-09-27 자동화 테스트로 확인했고, 체크포인트 없음에서 RestartPlayer로 가는 부활 분기는 코드로만 확인했다."
  - "사용자가 2026-09-25 BP_CheckPoint의 RespawnPoint 컴포넌트를 Capsule로 바꾼 뒤 ST_CheckPoint의 체크포인트 저장 태스크가 여전히 RespawnPoint를 가리켜 저장이 실패했고 그동안 부활은 항상 PlayerStart였으며, 2026-09-27 태스크가 Capsule을 가리키도록 고쳐 `22d82a746`로 제출됐다."
  - "UE 저장 파일이 아닌 임의 내용의 체크포인트 슬롯은 엔진 LoadGameFromSlot이 크래시하지만, 엔진 순정 동작이고 사람이 파일을 직접 바꿔야 생기는 경우라 조치하지 않았다."
---

# 작업 - checkpoint-savegame

- 원본: `.agents/workflow/tasks/checkpoint-savegame.md`
- 원자료 사본: `.raw/captured/cb8ffbe580deeced4b5d1f4d3e3063365b7607d7f1bfc9b64507dac59e7fdcb4.md` (수집 2026-09-27, 재확인 기한 2027-03-25)

## 개요

`UWxCheckpointSubsystem`을 없애고 체크포인트 저장·부활 조회·새 게임 초기화를 `UWxCheckpointSaveGame` 슬롯으로 옮긴 작업의 기록이다. 2026-09-25 구현·이름 변경(SaveCheckpoint)·단일 슬롯·리다이렉트 정리는 옛 결정 노트에도 있고, 이 기록은 2026-09-27 헤드리스 확인과 부활 위치 결함 수정까지 담는다. 상태는 완료(체크리스트 4/4 통과)다.

## 요청과 결정

- 사용자 요청: UWxCheckpointSubsystem을 제거하고 SaveGame으로 처리. 후속 요청으로 RecordCheckpoint를 SaveCheckpoint로 바꿨고, 후속 결정으로 PIE와 일반 플레이가 WxCheckpoint 슬롯 하나를 쓴다(기존 WxCheckpoint_PIE 파일은 자동 이관·삭제하지 않음).
- 체크리스트 정리(2026-09-27): '재시작 뒤 부활'은 없는 이어하기 흐름을 가리켰다. 프런트엔드에는 새 게임뿐이고 새 게임은 체크포인트를 지운다(`UWxGameFlowSubsystem::RequestNewGame` → `ResetCheckpoint`). 그래서 'PIE·일반 플레이 슬롯 공유'와 한 항목으로 합쳤다.
- 부활 위치 수정: 사용자 지시("직접 확인해서 처리해주세요")로 AI가 임시 에디터 자동화 코드로 ST_CheckPoint Resting 상태의 「체크포인트 저장」 태스크가 가리키는 컴포넌트를 `RespawnPoint`에서 `Capsule`로 바꾸고 컴파일·저장했다. 사용자가 9/25 `38d4dde08`에서 컴포넌트 이름을 바꾼 뒤로 저장이 「부활 위치 컴포넌트가 없습니다」 경고와 함께 실패했고, 그동안 부활은 항상 PlayerStart였다.
- 제출: ST_CheckPoint 수정과, 이동속도 커밋(`02af2e32b`)에 섞여 main에 올라갔던 임시 테스트 파일의 삭제를 사용자가 `22d82a746`로 제출했다(2026-09-27).

## 검증 범위

- AI(2026-09-27 임시 자동화 테스트, 전용 UserDir, 확인 뒤 삭제):
  - 사망·부활·새 게임: BP_CheckPoint에서 쉰 뒤 치트로 사망 → WBP_DeathScreen에서 RequestRespawn → 체크포인트(300,0,182)에서 부활(PlayerStart는 -200,0,90), HP 회복·사망 화면 닫힘. 새 게임은 슬롯을 지우고 이동을 요청했다(이동은 테스트에서 취소).
  - 재시작 뒤 슬롯 공유: 저장 프로세스와 불러오기 프로세스를 따로 띄워 같은 슬롯을 읽었고, 같은 UserDir의 새 게임 프로세스에서 상호작용 없이 사망하자 저장된 체크포인트에서 부활했다.
  - 실패 처리: 읽기 전용 슬롯에서 「체크포인트 저장에 실패했습니다」와 함께 실패하고 이전 체크포인트 유지. 슬롯 없음·다른 레벨·빈 파일·잘린 파일은 체크포인트 없음. 새 게임은 슬롯 삭제 실패 시 이동하지 않고 안내 문구·슬롯 유지. 부활 함수 자체는 사망 화면 위젯·로컬 플레이어가 필요해 실행하지 않았고, RestartPlayer 분기는 코드로 확인했다.
  - 수정 뒤 임시 코드와 WxEditor 임시 모듈 의존을 되돌리고 테스트 없는 WxEditor Development 빌드가 통과했다.
- 이전 사람 통과(이우성 2026-09-25, 사망·부활·새 게임)는 저장이 되지 않던 때여서 2026-09-27 헤드리스 규칙으로 AI가 다시 확인했다. 코드 리뷰는 이우성이 2026-09-25 통과시켰다.
- 참고: 슬롯이 없을 때 조회하면 엔진이 「Failed to read file」 경고를 한 번 남긴다. 임의 내용 슬롯의 엔진 크래시(FName 길이 assert)는 조치하지 않았다.

## 관련 주제

- [[체크포인트와 리스폰]]
- [[결정 노트 - 2026-09-25-checkpoint-savegame]]
- [[결정 노트 - 2026-09-25-checkpoint-single-slot]]
- [[작업 - headless-ai-testing]]

## 핵심 주장

- 2026-09-27 헤드리스 게임(LV_DevCombat -game -nullrhi) 임시 자동화 테스트에서 실제 BP_CheckPoint에서 쉰 뒤 사망해 사망 화면의 RequestRespawn으로 부활하면 체크포인트 위치에서 부활하고, 새 게임은 체크포인트 슬롯을 지웠다. ^c1
- 2026-09-27 헤드리스 테스트에서 PIE 이름 월드가 저장한 WxCheckpoint 슬롯을 새 프로세스의 일반 월드가 읽었고, 재시작한 게임에서 사망하면 저장된 체크포인트에서 부활했다. ^c2
- 체크포인트 저장 실패 시 태스크 실패와 이전 체크포인트 유지, 슬롯 없음·다른 레벨·빈 파일·잘린 파일의 체크포인트 없음 처리, 새 게임의 슬롯 삭제 실패 시 이동 중단은 2026-09-27 자동화 테스트로 확인했고, 체크포인트 없음에서 RestartPlayer로 가는 부활 분기는 코드로만 확인했다. ^c3
- 사용자가 2026-09-25 BP_CheckPoint의 RespawnPoint 컴포넌트를 Capsule로 바꾼 뒤 ST_CheckPoint의 체크포인트 저장 태스크가 여전히 RespawnPoint를 가리켜 저장이 실패했고 그동안 부활은 항상 PlayerStart였으며, 2026-09-27 태스크가 Capsule을 가리키도록 고쳐 `22d82a746`로 제출됐다. ^c4
- UE 저장 파일이 아닌 임의 내용의 체크포인트 슬롯은 엔진 LoadGameFromSlot이 크래시하지만, 엔진 순정 동작이고 사람이 파일을 직접 바꿔야 생기는 경우라 조치하지 않았다. ^c5
