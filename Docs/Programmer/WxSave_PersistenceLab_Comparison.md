# WxSave ↔ PersistenceLab — 골격 이식 후 비교 분석

현행 WxSave 플러그인과 그 골격의 출처인 PersistenceLab 샘플(`PersistenceUtils` 플러그인)의 대응 관계·차이·남은 흡수 후보를 정리한다. 샘플 자체의 단독 분석은 [Sample_PersistenceLab_Save_System.md](Sample_PersistenceLab_Save_System.md) 참조. 이식 이력은 2026-07-06 워크로그 2건(장점 흡수 → 골격·함수명 이식)에 있다.

---

## 한 문장 요약

> WxSave 는 샘플의 **서브시스템 2분할 골격과 함수 이름을 1:1로 이식**했지만, 저장 페이로드는 엔진 실험 플러그인(LSP/Mass/IA) 대신 **WxCore 계약 `IWxSavable` + GUID 키 레코드**라는 자체 방식을 유지한다. 저장은 authority 전용(스탠드얼론 전제).

두 시스템을 가르는 축은 두 개다.

- **골격(누가 무엇을 소유하나)** — 동일: GameInstance 서브시스템이 SaveGame 수명·디스크 I/O·트래블·가드를, World 서브시스템이 월드 수명에 결부된 플러시/복원 오케스트레이션을 소유
- **페이로드(무엇을 어떻게 직렬화하나)** — 상이: 샘플은 LSP ini 등록 프로퍼티 + IAM 블롭 + Mass 스냅샷의 3계층·맵별 키잉 / Wx 는 `IWxSavable` 액터의 SaveGame 프로퍼티 블롭·GUID 평면 키잉 1계층

---

## 골격 대응 관계

| Sample-PersistenceLab-main | WxSave | 비고 |
| --- | --- | --- |
| `UPersistenceSaveGame` | `UWxPersistenceSaveGame` | `SlotName`/`UserIndex`/`TravelData` 동일. `SavedStatePerMap` 대신 `ActorRecords`+`PlayerStartTag` |
| `FPersistenceTravelData` | `FWxPersistenceTravelData` | 필드 구성 동일(`Map`·폰 트랜스폼·컨트롤 로테이션·플래그 2개) |
| `UPersistenceGameSubsystem` | `UWxPersistenceGameSubsystem` | 함수명 1:1: `GetSaveGame` `IsTravelingFromSaveFile` `StartNewSaveFile` `LoadFromFile` `ReloadFromFile` `TravelFromSaveFile` `SaveToFile` `SetPersistenceTravelData` `ReportTravelFromSaveFileComplete` `ContinueSaveToFileToDisk` |
| `UPersistenceWorldSubsystem` | `UWxPersistenceWorldSubsystem` | `RequestSaveFlush`(+`FOnSaveFlushComplete`)·`FlushMapTravelData` 이식. 페이로드 워커는 `FlushSavableActors`/`CaptureActor`/`RestoreActor`(Wx 고유) |
| `USaveFilePersistenceUtils` | `UWxSaveFilePersistenceUtils` | BFL 5함수 동일 + `GetDefaultSlotName`(Wx 추가) |
| `UPersistenceUtilsSettings` | (없음) | Wx 는 설정 클래스 미도입 — 아래 흡수 후보 |

Wx 유지분(샘플에 없음): `SetPlayerStartTag`/`GetPlayerStartTag`, `GetStableMapPackageName`(PIE 접두사 제거 맵 키 표현의 단일 출처), `Wx.Save.Dump` 콘솔 명령.

---

## 저장 페이로드의 근본 차이

샘플에서 액터 프로퍼티 저장의 실체는 플러그인 코드가 아니라 **엔진 Level Streaming Persistence 플러그인 + `DefaultEngine.ini` 등록**이고, 그 위에 Instanced Actors 블롭과 Mass 스냅샷 계층이 얹힌다. 데이터는 `SavedStatePerMap`(맵 이름 키) → 레벨 키 → 매니저 키로 계층화된다.

Wx 는 이를 채택하지 않았다:

- **LSP/Mass/IA 미채택 이유** — 셋 다 experimental 이고 Mass/IA 는 Wx 가 아예 쓰지 않는 시스템이다. 샘플 C++ 를 통째로 이식하면 코드의 6~7할이 죽은 의존성이 되고, 정작 기믹/스포너 상태를 저장하는 본체는 따라오지 않는다.
- **Wx 방식** — `IWxSavable`(WxCore 계약)을 구현한 액터를 `TActorIterator` 로 수집해, 에디터에서 1회 부여된 영속 GUID(`GetWxSaveId`, 쿠킹 빌드 안전)를 키로 `FWxActorRecord` 에 직렬화한다. 레코드는 Transform + 액터 본체 블롭 + 컴포넌트 FName 별 블롭 + **레코드당 버전 헤더**(`[FPackageFileVersion][FCustomVersionContainer]` 별도 UPROPERTY 블롭)로 구성된다.
- **맵별 키잉이 불필요한 이유** — GUID 가 맵을 넘어 전역 유일하므로 평면 `TMap<FGuid, FWxActorRecord>` 로 충돌 없이 다중 맵 상태가 공존한다. 샘플의 맵/레벨 키잉은 이름 기반 식별(매니저 FName·레벨 패키지)이라서 필요했던 구조다.
- 현재 `IWxSavable` 구현체는 `AWxGimmick` 계열(State 태그)과 `AWxSpawner`(`bIsKilled`) 2계열이다.

---

## 의도적 의미론 차이

이식하면서 샘플과 다르게 남긴 지점들. 전부 코드 주석에 근거가 명시돼 있다.

| 지점 | 샘플 | Wx | 이유 |
| --- | --- | --- | --- |
| `LoadFromFile` 파일 부재 | nullptr 반환, 중단 | 같은 슬롯 새 SaveGame 으로 리셋 후에도 트래블 | 사망 리스폰(WBP_DeathScreen)이 파일 없이도 월드 리로드에 의존 |
| `TravelFromSaveFile` Map 부재 | 경고 후 중단 | 현재 맵 리로드로 폴백 | 위와 동일(구버전 파일 호환 겸) |
| 트래블 수단 | `UGameplayStatics::OpenLevel` | `ServerTravel(bAbsolute=true)` | 스탠드얼론에서 기능 차 없음, 기존 검증 경로·authority 게이트와 일관 |
| 폰 위치 복원 | 저장 트랜스폼에 `APlayerStartPIE` 스폰(엔진 관례 의존) | `AWxGameMode::SpawnDefaultPawnFor`/`FinishRestartPlayer` 오버라이드 + `UWxPlayerSpawningComponent::TryGetSavedPawnSpawn` 판정(맵 일치 게이트) | 샘플 주석 스스로 GameMode 오버라이드가 가장 신뢰성 높다고 권고 — Wx 는 스포닝을 직접 소유 |
| `RequestSaveFlush` | Mass 스냅샷을 FrameEnd 페이즈 경계로 지연 + teardown 페일세이프 | 전부 동기, 완료 델리게이트는 즉시 발화(비동기 도입 대비 seam 만 유지) | Wx 에 페이즈 제약이 있는 작업(Mass)이 없음 |
| Initialize 의 SaveGame 보장 | PIE 에서 `PIETestFile` 자동 로드/생성 | 모드 무관 항상 `StartNewSaveFile`(자동 로드 없음 — 매 시작이 빈 새 슬롯) | 체크포인트/UI 가 활성 SaveGame 을 전제 — 흩어진 EnsureSaveObject 를 init 보장 + null 경고 가드로 대체. PIE 반복 테스트용 자동 로드는 제거(매 PIE 는 신선한 시작) |
| 버전 헤더 | IAM 블롭에 내장(2패스 직렬화) | 레코드의 별도 UPROPERTY 블롭(1패스) | 헤더가 블롭 밖이라 본체 포맷 불변·구버전 하위호환이 공짜 |
| 델리게이트 콜백 네이밍 | `On*` | `Handle*` | Wx 코딩 규칙 6 이 우선 |
| 로드 가드 소유 | `bTravelingFromSaveFile`(게임 서브시스템) — 동일 | 동일 이식. 해제는 새 월드 `OnWorldBeginPlay` 의 보고(샘플 시점 동일) | 트래블 중 자동 캡처가 막 로드한 세이브를 덮어쓰는 오염 방지 |

---

## 샘플에 있고 Wx 에 없는 것 (향후 흡수 후보)

| 기능 | 샘플 구현 | Wx 도입 전제 |
| --- | --- | --- |
| 저장 직전 게임 코드 확장점 | `FPersistenceUtilsDelegates::OnPreSave` | 인벤토리·스탯 등 액터 외 상태의 영속화가 생길 때(WxCore 계약으로) |
| 객체별 pre/post 훅 | `IPersistedObject::PrePersistObject`/`PostRestoreObject` | Wx 는 복원 후 훅(`OnWxSaveRestored`)만 있음 — 런타임 상태↔SaveGame 프로퍼티 변환이 필요한 액터가 생길 때 `PrePersist` 상당 추가 |
| 크로스 세션 액터 참조 | `FPersistableActorReference` + `UPersistableActorReferenceManager` + `UPersistableReferencedActorComponent` | 저장 대상이 다른 액터를 참조로 기억해야 할 때(예: GE 인스티게이터, AI 타겟) |
| 런타임 스폰 액터 재스폰 | LSP `RuntimeRespawnedActorClasses`(ini) | 드롭 아이템·투사체 등 런타임 스폰물을 세션 넘어 유지할 때 — Wx 레코드 방식으론 스폰 파이프라인 신설 필요 |
| 맵 배치 액터 파괴 영속 | LSP `bPersistAllActorDestruction` | 파괴가 상태 태그로 표현 안 되는 액터가 생길 때(현재 기믹/스포너는 태그·bool 로 충분) |
| 설정 클래스 | `UPersistenceUtilsSettings`(DeveloperSettings, ini) | 자동 플러시·폰 복원 등의 토글이 늘어나면(현재는 전부 고정 동작) |

---

## Wx 에 있고 샘플에 없는 것

- **스트리밍-아웃 자동 캡처** — `HandleLevelRemovedFromWorld` 가 WP 셀 이탈 시 상태를 메모리에 기록한다. 샘플은 이 몫을 LSP 가 엔진 레벨에서 대신한다.
- **teardown 전체 메모리 플러시** — `HandleWorldBeginTearDown` 이 맵 이탈 시 savable 전체를 캡처해 같은 세션 맵 왕복 상태를 유지한다(샘플의 `bAutoSaveWhenLeavingMap` 상당을 설정 없이 상시 수행).
- **PlayerStartTag 체크포인트 부활 체계** — 좌표가 아닌 PlayerStart 식별자 저장. `AWxCheckPoint` 상호작용 → `SetPlayerStartTag` → `SaveToFile()` 흐름과 `UWxPlayerSpawningComponent` 의 태그 탐색 폴백.
- **레코드 단위 버전 헤더** — 모든 페이로드 블롭이 커스텀 버전 마이그레이션 가능. 샘플은 IAM 블롭 경로만 자체 버전 관리하고 LSP/Mass 는 각자 정책을 따른다.
- **`Wx.Save.Dump`** — 메모리 슬롯 상태(슬롯·맵·폰·레코드별 바이트/헤더) 덤프 콘솔 명령.

---

## 주의할 점

- **BP 진입점 이름이 바뀌었다** — 구 `UWxSaveGameLibrary::SaveSlot/LoadSlot` 은 삭제됐다. WBP_MainMenu·WBP_DeathScreen 은 `UWxSaveFilePersistenceUtils::SaveToFile`/`LoadFromFile` 노드로 교체돼야 동작한다.
- **`RequestSaveFlush` 는 이름과 달리 동기다** — 완료 델리게이트가 반환 전에 발화한다. 비동기 플러시 작업을 추가하면 샘플처럼 지연 완료(재진입 병합 포함)로 되돌려야 한다.
- **맵 키 표현은 `GetStableMapPackageName` 하나로 통일돼 있다** — 트래블 데이터 스탬프와 맵 일치 판정이 이 함수를 공유한다. 다른 표현(`GetMapName` 등)을 섞으면 PIE 접두사 때문에 조용한 복원 실패가 난다.
- **저장은 명시적 `SaveToFile` 만 디스크에 쓴다** — teardown/스트리밍 캡처는 전부 메모리 SaveGame 갱신이다.

---

### 참조 코드

Wx 는 저장소 루트(`C:\Wx`) 기준, 샘플은 `C:\Sample-PersistenceLab-main` 기준 경로다.

| 타입 | 위치 | 역할 |
| --- | --- | --- |
| `UWxPersistenceSaveGame` / `FWxPersistenceTravelData` / `FWxActorRecord` | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` | 슬롯 데이터·트래블 데이터·GUID 레코드 |
| `UWxPersistenceGameSubsystem` | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceGameSubsystem.h` (+cpp) | SaveGame 수명·디스크 I/O·트래블·가드 |
| `UWxPersistenceWorldSubsystem` | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceWorldSubsystem.h` (+cpp) | 플러시/복원 오케스트레이션·캡처/복원 워커 |
| `UWxSaveFilePersistenceUtils` | `Plugins/WxSave/Source/WxSave/Public/WxSaveFilePersistenceUtils.h` (+cpp) | BP 진입점 |
| `IWxSavable` | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` | 저장 옵트인 계약(GUID·복원 훅) |
| `AWxCheckPoint` | `Source/WxGame/WorldObject/WxCheckPoint.cpp` | 저장 트리거(태그 등록→SaveToFile) |
| `UWxPlayerSpawningComponent` / `AWxGameMode` | `Source/WxGame/Framework/` | 세이브 폰 트랜스폼 우선 스폰·태그 폴백 |
| `UPersistenceGameSubsystem` 외 샘플 전반 | `Plugins/PersistenceUtils/Source/PersistenceUtils/` (샘플 루트 기준) | 이식 원본 — 상세는 [Sample_PersistenceLab_Save_System.md](Sample_PersistenceLab_Save_System.md) |
