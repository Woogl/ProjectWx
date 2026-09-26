---
title: "체크포인트 단일 슬롯 결정"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, savegame]
summary: "사용자 결정에 따라 PIE와 일반 플레이가 WxCheckpoint 슬롯을 공유한다. 슬롯 세분화는 추후 진행한다."
---

# 체크포인트 단일 슬롯 결정

2026-09-25 사용자 결정: 하나로 통일하고 나중에 세분화한다.

`UWxCheckpointSaveGame::GetSlotName()`은 World 인자 없이 항상 `WxCheckpoint`를 반환한다. 저장·조회·새 게임 초기화 모두 같은 슬롯과 사용자 인덱스 0을 사용한다. 과거의 PIE 분리 설명을 대체한다. 기존 WxCheckpoint_PIE 파일의 자동 이관·삭제는 구현하지 않았다.

근거: [저장 구현](../../../Plugins/WxWorld/Source/WxWorld/Private/System/WxCheckpointSaveGame.cpp).
