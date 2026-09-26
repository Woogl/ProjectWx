---
title: "SpawnerLibrary 제거와 C++ 일괄 재생성"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, world, spawner]
summary: "UWxSpawnerLibrary를 제거하고 AWxSpawner::RespawnAll C++ 전용 함수로 부활·StateTree 호출을 연결했다."
---

# SpawnerLibrary 제거와 C++ 일괄 재생성

2026-09-25 사용자 요청: UWxSpawnerLibrary를 제거하고 가능하면 BP 작업이 없도록 처리.

- 기존 C++ 호출부는 WxRespawnLibrary와 StateTreeTask_RespawnSpawners 두 곳이었다. Content/Plugins의 uasset·umap 바이너리 문자열 검색에서 WxSpawnerLibrary·TryRespawnAll은 발견되지 않았다. 실제 에셋 로드 검증은 아니다.
- 일괄 재생성은 `AWxSpawner::RespawnAll(const UWorld*)`로 이동했다. UFUNCTION을 붙이지 않은 C++ 전용 API다. 두 호출부를 직접 연결했으며 플레이어 부활 성공 후 스트리밍 완료 시점과 StateTree 라이브 진입 시점은 유지한다.
- 현재 로드된 스포너 중 Manual을 제외하고 수집한 뒤 Respawn한다. 클라이언트 월드는 건너뛰고 개별 Respawn의 권한·영구 처치 제한도 유지한다. 재생성 콜백에서 다른 스포너가 사라질 수 있어 목록은 약참조로 보관하고 호출 직전에 유효성을 확인한다.
- BP에서 별도의 스포너 재생성 노드를 호출할 필요가 없다. 플레이어 부활 요청 진입점 자체의 UI 연결은 이번 범위가 아니다.

## 근거

- [스포너 API](../../../Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h)
- [스포너 구현](../../../Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp)
- [부활 연결](../../../Source/WxGame/Framework/WxRespawnLibrary.cpp)
- [StateTree 연결](../../../Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_RespawnSpawners.cpp)
