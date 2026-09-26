---
title: "체크포인트 저장 명칭 통일"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, savegame]
summary: "사용자 요청으로 RecordCheckpoint API와 StateTree 태스크를 SaveCheckpoint로 변경하고 기존 에셋의 구조체 경로를 리다이렉트했다."
---

# 체크포인트 저장 명칭 통일

2026-09-25 사용자 후속 요청으로 `RecordCheckpoint`를 `SaveCheckpoint`로 변경했다. 저장 동작은 유지한다.

- `UWxCheckpointSaveGame::SaveCheckpoint`가 저장 API다.
- `FWxStateTreeTask_SaveCheckpoint`와 `FWxStateTreeTask_SaveCheckpointInstanceData`로 구조체·생성자·헤더·cpp·generated include·호출부를 함께 변경했다. 표시명은 `체크포인트 저장`이다.
- `Config/DefaultEngine.ini`의 CoreRedirects에 이전 두 구조체 경로를 새 경로로 매핑했다. 에셋 재저장은 수행하지 않았다.
- 이전 원자료 `2026-09-25-checkpoint-savegame.md`의 RecordCheckpoint 파일 경로는 변경 전 조사 시점의 경로다.

## 근거

- [저장 API](../../../Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSaveGame.h)
- [저장 태스크](../../../Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeTask_SaveCheckpoint.h)
- [리다이렉트](../../../Config/DefaultEngine.ini)
