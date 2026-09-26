---
title: "체크포인트 구조체 리다이렉트 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, savegame]
summary: "ST_CheckPoint를 SaveCheckpoint 구조체로 리세이브하고 CoreRedirects 두 항목을 제거한 뒤 별도 프로세스에서 재로드·컴파일·저장을 검증했다."
---

# 체크포인트 구조체 리다이렉트 제거

사용자 요청에 따라 기존 RecordCheckpoint/RecordCheckpointInstanceData 리다이렉트를 적용한 상태에서 ST_CheckPoint를 ResavePackages로 저장했다. Content/Plugins의 uasset·umap 문자열 검색에서 이전 구조체를 참조한 패키지는 이 하나였다.

Config/DefaultEngine.ini의 두 StructRedirects와 빈 CoreRedirects 섹션을 제거하고 새 UnrealEditor-Cmd 프로세스로 같은 패키지를 다시 로드·컴파일·저장했다. 두 실행 모두 종료 코드 0이며 StateTree 컴파일과 저장 성공을 확인했다. 이전 이름은 에셋 검색에서 발견되지 않고 새 SaveCheckpoint 이름이 저장돼 있다. 공통 경고 1건은 MCP 플러그인의 라이선스 안내이며 로드·컴파일 오류는 없다. 인게임 플레이는 검증하지 않았다.

- [리세이브 에셋](../../../Content/WorldObject/Gimmick/ST_CheckPoint.uasset)
- [엔진 설정](../../../Config/DefaultEngine.ini)
- 실행 로그: Saved/Logs/CheckpointResave.log, Saved/Logs/CheckpointWithoutRedirects.log

기존 2026-09-25-save-checkpoint-rename 원자료의 리다이렉트 추가 설명은 마이그레이션 당시 기록이다. 현재 설정에는 리다이렉트가 없다.
