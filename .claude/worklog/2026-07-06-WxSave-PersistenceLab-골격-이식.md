# WxSave에 PersistenceLab 서브시스템 골격·함수명 1:1 이식

## 계획

### 목표

샘플 `PersistenceUtils` 플러그인의 서브시스템 골격(GameInstance/World 2분할)과 함수 이름을 WxSave에 1:1로 이식한다. C++ 통짜 포팅은 기각(Mass/IA/LSP 의존성 부재, 액터 저장 본체가 엔진 LSP에 있음) — 구조와 이름만 샘플을 따르고 저장 페이로드는 검증된 Wx GUID 레코드 방식을 유지한다. 클래스·파일명은 Persistence 이름으로 완전 이식하고, 구 BFL 함수는 즉시 삭제한다(BP 2곳 수동 수정·Test.sav 무효화 허용 — 사용자 결정).

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxPersistenceSaveGame.h` | `UWxPersistenceSaveGame`(SlotName·UserIndex·TravelData 보유) + `FWxPersistenceTravelData` + 기존 레코드 구조체 이동 | 신규 |
| `Plugins/WxSave/.../WxPersistenceGameSubsystem.h/.cpp` | 샘플 API 1:1(StartNewSaveFile/LoadFromFile/ReloadFromFile/SaveToFile/TravelFromSaveFile/ReportTravelFromSaveFileComplete 등) + Wx 유지분(PlayerStartTag·Dump) | 신규 |
| `Plugins/WxSave/.../WxPersistenceWorldSubsystem.h/.cpp` | 플러시/복원 오케스트레이션 분리(RequestSaveFlush·월드 델리게이트 핸들러·CaptureActor/RestoreActor 이동) | 신규 |
| `Plugins/WxSave/.../WxSaveFilePersistenceUtils.h/.cpp` | 샘플 BFL 5함수 + GetDefaultSlotName. SaveSlot/LoadSlot 삭제 | 신규 |
| `Plugins/WxSave/.../WxSaveGame.h`, `WxSaveGameSubsystem.h/.cpp`, `WxSaveGameLibrary.h/.cpp` | 신규 파일로 대체 | 삭제 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 타입·호출 교체(`SaveToFile()`) | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.cpp` | `TryGetSavedPawnSpawn`을 `GetSaveGame()->TravelData` 읽기로 재작성 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h` | doc-comment 갱신 | 수정 |

### 접근 방식

- **2분할 골격**: GameInstance 서브시스템은 SaveGame 수명·디스크 I/O·트래블·가드 플래그를, World 서브시스템은 월드 수명에 결부된 플러시/복원 오케스트레이션(월드 델리게이트 4종 + OnWorldBeginPlay의 트래블 완료 보고)을 맡는다. 슬롯 정체성(SlotName/UserIndex)은 SaveGame 안으로 들어가 `SaveToFile()`이 무인자가 된다.
- **샘플과 다른 Wx 의미론(주석 명시)**: LoadFromFile 파일 부재 시 새 슬롯 리셋 후에도 트래블(사망 리스폰이 의존), TravelFromSaveFile 의 Map 부재 시 현재 맵 리로드 폴백, 트래블은 ServerTravel 유지, RequestSaveFlush 는 Mass 부재로 전부 동기(시그니처만 seam 유지).
- **가드 이름 이식**: `bLoadTravelInProgress` → `bTravelingFromSaveFile` + `IsTravelingFromSaveFile()`/`ReportTravelFromSaveFileComplete()`(해제 시점: 새 월드 OnWorldBeginPlay — 복원과 구 월드 teardown 이후라 안전).
- **동작 등가가 목표**: 직전 작업에서 검증한 5개 개선(맵 트래블·teardown 플러시·버전 헤더·PIE 슬롯·폰 위치)의 동작은 그대로 유지한 채 구조와 이름만 바꾼다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxPersistenceSaveGame.h` | `UWxPersistenceSaveGame`(SlotName·UserIndex·TravelData) + `FWxPersistenceTravelData` + 기존 레코드 구조체 이동 | 신규 |
| `Plugins/WxSave/.../WxPersistenceGameSubsystem.h/.cpp` | 샘플 API 1:1 게임 서브시스템 + PlayerStartTag·Dump·`GetStableMapPackageName`·`WxPersistence::DefaultSlotName` | 신규 |
| `Plugins/WxSave/.../WxPersistenceWorldSubsystem.h/.cpp` | `RequestSaveFlush`·월드 델리게이트 4종 핸들러·`FlushMapTravelData`/`FlushSavableActors`·`CaptureActor`/`RestoreActor` 이동 | 신규 |
| `Plugins/WxSave/.../WxSaveFilePersistenceUtils.h/.cpp` | 샘플 BFL 5함수 + `GetDefaultSlotName`. `SaveSlot`/`LoadSlot` 미제공 | 신규 |
| `WxSaveGame.h`, `WxSaveGameSubsystem.h/.cpp`, `WxSaveGameLibrary.h/.cpp` | 신규 파일로 대체 | 삭제 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 타입 교체, `SaveSlot("Test")` → `SaveToFile()` | 수정 |
| `Source/WxGame/Framework/WxPlayerSpawningComponent.cpp` | `TryGetSavedPawnSpawn`을 `GetSaveGame()->TravelData` 읽기 + 맵 일치 판정으로 재작성 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h`, `Source/WxGame/README.md` | 서브시스템 언급 doc 갱신 | 수정 |

### 구현·결정과 그 이유
- **가드 소유를 게임 서브시스템으로 이동**: `bTravelingFromSaveFile`+`IsTravelingFromSaveFile()`은 트래블을 시작하는 쪽(게임 서브시스템)이 소유하고, 월드 서브시스템 핸들러들이 조회해 자동 캡처를 스킵한다. 해제는 새 월드 `OnWorldBeginPlay`에서 `ReportTravelFromSaveFileComplete` 보고로 — 복원(initialized-actors)과 구 월드 teardown 이후라 기존 해제점과 동등하게 안전하다(샘플 시점 동일).
- **Initialize 가 활성 SaveGame 을 항상 보장**: PIE 는 기본 슬롯 자동 로드(부재 시 새 슬롯), 비 PIE 는 새 슬롯 시작. 이로써 흩어져 있던 `EnsureSaveObject` 를 없애고 샘플식 "활성 SaveGame 없으면 경고" 가드로 단순화했다.
- **CaptureActor/RestoreActor 에 SaveGame 참조 인자**: 월드 서브시스템은 SaveGame 을 소유하지 않으므로, 루프당 1회 조회한 SaveGame 을 참조로 넘겨 액터당 서브시스템 조회를 피했다.
- **LoadFromFile 파일 부재 시 리셋+트래블 유지**: 샘플은 중단하지만 Wx 사망 리스폰(WBP_DeathScreen)이 파일 없이도 월드 리로드에 의존해 Wx 의미론을 유지했다(주석 명시). `TravelFromSaveFile`의 Map 부재 폴백(현재 맵 리로드)도 같은 이유.
- **컨트롤 로테이션 캡처를 샘플식 독립 플래그로**: `FlushMapTravelData`가 폰 부재 시에도 PC 의 시선은 캡처한다(샘플 동일). 소비 측(`TryGetSavedPawnSpawn`)은 종전대로 폰 트랜스폼 유효+맵 일치를 성공 조건으로 유지.

### 계획 대비 달라진 점
- 계획엔 없던 `Source/WxGame/README.md`의 구 클래스명 1줄도 함께 갱신(스테일 문서 방지).

### 후속 과제
- **WBP_MainMenu·WBP_DeathScreen 수동 수정 필요(기능 단절 중)**: `SaveSlot("Test")` → `SaveToFile()`, `LoadSlot("Test")` → `LoadFromFile(GetDefaultSlotName(), 0)`. 교체 전까지 해당 버튼 동작 안 함.
- 기존 `Test.sav` 는 클래스 리네임으로 무효(로드 시 빈 슬롯 시작) — 사용자 결정으로 허용.
- `Plugins/WxSave/README.md` 재생성(`/readme-writer`) — 구 API 서술 다수.
- PIE 수동 검증(직전 작업과 동일 시나리오: 가드 회귀·맵 트래블·왕복 플러시·자동 슬롯·WP 셀 왕복)은 에디터 실행 필요.
