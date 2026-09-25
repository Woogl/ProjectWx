---
title: "체크포인트 SaveGame 전환"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, savegame]
summary: "CheckpointSubsystem을 제거하고 체크포인트 기록·부활 조회·새 게임 초기화를 USaveGame 디스크 슬롯으로 전환했다."
---

# 체크포인트 SaveGame 전환

사용자 요청: `UWxCheckpointSubsystem` 제거, SaveGame으로 처리. 2026-09-25 작업 트리의 정적 확인 기록이며 인게임 실행 검증은 아니다.

- `UWxCheckpointSaveGame`은 `USaveGame`을 상속하며 SaveGame 프로퍼티로 레벨 패키지와 부활 Transform을 저장한다. Standalone에서만 동작하며 PIE 접두사를 제거한 맵이 일치할 때만 위치·회전을 반환한다.
- 기록은 `SaveGameToSlot`, 조회는 `LoadGameFromSlot`, 새 게임 초기화는 `DoesSaveGameExist`/`DeleteGameInSlot`을 사용한다. 일반 슬롯은 `WxCheckpoint`, PIE 슬롯은 `WxCheckpoint_PIE`, 사용자 인덱스는 0이다. PIE도 실행 간 저장이 유지된다.
- StateTree 기록 태스크는 복원·비플레이어·멀티플레이에서 저장하지 않는다. 저장 실패는 경고와 Failed로 전달한다.
- 부활 시 저장 없음·로드 실패·다른 맵·유효하지 않은 Transform은 PlayerStart 경로로 돌아간다. 새 게임은 삭제 실패 시 상태 문구를 표시하고 맵 이동을 시작하지 않는다.
- 저장 내용은 체크포인트 한 개다. 캐릭터·인벤토리·장치 상태나 이어하기 UI를 저장·구현하지 않는다. 새 게임 요청 후 맵 이동 실패 시 초기화된 체크포인트는 복구하지 않으며 기존 초기화 시점과 같다.

## 근거 파일과 SHA-256
- [Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSaveGame.h](../../../Plugins/WxWorld/Source/WxWorld/Public/System/WxCheckpointSaveGame.h): 43AF5519506034C3CD0FD306EF8A3883E14C254DE91FBA6A853F59B58B5C9931
- [Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSaveGame.cpp](../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSaveGame.cpp): 5BB26815E526C931020C67DA68C67D1A2E12D94CCE1EE11035A318D3CF7C31E3
- [Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RecordCheckpoint.cpp](../../../Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RecordCheckpoint.cpp): 7624BC8113346764F54E877F7E9DB2D489432109361DFC04B7C12E2BA7C9B5DD
- [Source/WxGame/Framework/WxRespawnLibrary.cpp](../../../Source/WxGame/Framework/WxRespawnLibrary.cpp): 418358758203E4DE20A87EDC7980B895E38F16A9026F9F6A296B7D3D9525EB08
- [Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp): E6B43C6B2A698FEFE7E20878BB931686D13C32DB6EE7247C0D7B8FC499945EF0
