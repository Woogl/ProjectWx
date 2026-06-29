# WxSave — 세이브/로드 시스템

> 등록된 액터(`IWxSavable`)의 상태와 플레이어 부활 체크포인트를 슬롯 파일에 직렬화하고, World Partition 셀 왕복·맵 리로드를 가로질러 복원하는 런타임 플러그인.

## 책임
**담당**
- `IWxSavable` 액터의 `UPROPERTY(SaveGame)` 필드 + Transform 을 `FObjectAndNameAsStringProxyArchive`(`ArIsSaveGame=true`)로 바이트 직렬화/역직렬화.
- 슬롯 단위 디스크 영속화: `AsyncSaveGameToSlot` 비동기 저장, `LoadGameFromSlot` 로드.
- World Partition 셀 스트리밍-인/아웃 시 메모리 슬롯과 액터 상태를 자동 동기화(스트리밍-아웃 캡처, 스트리밍-인 복원).
- `LoadSlot` 의 맵 리로드(`ServerTravel`) 오케스트레이션 — 복원은 리로드된 새 월드의 스트리밍 핸들러가 수행.
- 부활/시작 진입점 `PlayerStartTag` 보관/조회 (사망 후 부활 + 레벨 시작 지점).
- BP 노출(`UWxSaveGameLibrary`)과 콘솔 진단(`Wx.Save.Dump`).

**경계 (비담당)**
- 무엇을 저장할지의 선언: 각 액터가 `IWxSavable`([[WxCore]])을 구현하고 `UPROPERTY(SaveGame)` 를 마킹한다. WxSave 는 마킹된 것을 기계적으로 직렬화만 한다.
- 부활 위치를 실제 액터로 해석하는 로직 + 태그 갱신 시점: GameMode 의 `ChoosePlayerStart` 가 `GetPlayerStartTag()` 의 Tag 를 `FindPlayerStart` 에 넘기고, 선택된 PlayerStart 태그를 `SetPlayerStartTag` 로 되기록한다(WxGame 측 GameMode).
- 체크포인트 액터/상호작용 자체: 호출 측이 `SetPlayerStartTag` 호출 + `SaveSlot` 영속을 수행한다(WxGame/WxWorld 측 체크포인트 액터).

## 의존성
- **주요 의존**: [[WxCore]](`IWxSavable` 인터페이스, `GetWxSaveId()`). 엔진: `UGameInstanceSubsystem`(ServerTravel 가로질러 메모리 유지), `UGameplayStatics` 비동기 슬롯 IO, `FWorldDelegates`(LevelAdded/Removed/OnWorldInitializedActors) 셀 스트리밍 후크.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`.uplugin`/`Build.cs` 의존은 `WxCore` 뿐).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 모듈 단일 진입점. 저장/로드/복원/스트리밍 핸들러를 모두 보유한 `GameInstanceSubsystem`(ServerTravel 가로질러 유지) | `Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveGameLibrary` | BP 진입점. `SaveSlot`/`LoadSlot` 를 정적 래퍼로 노출(서브시스템 위임) | `Source/WxSave/Public/WxSaveGameLibrary.h` |
| `UWxSaveGame` | 슬롯 데이터 컨테이너. `TMap<FGuid, FWxActorRecord>` + `PlayerStartTag` 보관 | `Source/WxSave/Public/WxSaveGame.h` |
| `FWxActorRecord` | 액터 1개의 스냅샷(Transform + 본체 바이트 + 컴포넌트별 바이트), 슬롯 맵의 value | `Source/WxSave/Public/WxSaveGame.h` |
| `IWxSavable` | 저장 대상 마킹 인터페이스. WxCore 소속(여기선 의존만) | `../WxCore/Source/WxCore/Public/WxSavable.h` |

## 확장 포인트 / 규약
- **세이브 대상 추가**: 액터에 `IWxSavable`([[WxCore]]) 구현 + `GetWxSaveId()` 가 에디터-부여 영속 GUID 반환. 이 GUID 가 슬롯 키이므로 쿠킹 빌드에서도 안정적이어야 한다(`GetActorGuid()` 는 WITH_EDITOR 전용이라 사용 불가). 무효 GUID 는 저장/복원에서 제외. 보존할 필드는 `UPROPERTY(SaveGame)` 로 마킹(액터 본체·컴포넌트 모두 지원).
- **복원 후처리**: `OnWxSaveRestored()` 오버라이드로 시각/인터랙션 동기화. BeginPlay 이전 호출 가능성 주의.
- **컴포넌트 필드**: `Actor::Serialize` 가 컴포넌트 SaveGame 필드를 자동으로 끌지 않으므로 컴포넌트 FName 키로 별도 캡처/복원(`FWxComponentRecord` wrapper — TMap value 로 TArray 직접 불가).
- **부활/시작 지점 연동**: 호출 측이 `SetPlayerStartTag(Tag)` 로 Tag 기록(레벨 시작·체크포인트 상호작용) → 저장 시 디스크 영속 → 리로드 후 GameMode 가 `GetPlayerStartTag()` 조회. 좌표가 아닌 Tag 만 저장하여 실제 배치 액터를 엔진이 찾는다. `NAME_None` 은 미설정 sentinel(기본 PlayerStart 폴백).
- **권한 모델**: `LoadSlot` 는 authority(서버) 전제 — `ServerTravel` 로 맵을 리로드하고, 액터 복원/부활은 리로드된 새 월드의 핸들러가 담당한다. 즉시 in-place 복원이 아니다.
- **PIE 격리**: 모든 핸들러가 `IsOwnedGameWorld` 로 자기 GameInstance 월드만 처리.

## 여기서부터 읽어라
1. `Source/WxSave/Public/WxSaveGameSubsystem.h` — 헤더 doc-comment 에 저장/로드/스트리밍 복원 전체 흐름이 정리돼 있다.
2. `Source/WxSave/Private/WxSaveGameSubsystem.cpp` — `CaptureActor`/`RestoreActor` 의 ProxyArchive 직렬화, `ServerTravel` 리로드, 셀 스트리밍 핸들러 구현.
3. `../WxCore/Source/WxCore/Public/WxSavable.h` — 저장 참여 계약(키·후크) 전문.

## 관련
- 상위: [[WxCore]] — `IWxSavable` 인터페이스 정의처.
- 소비: 저장 대상 액터(`IWxSavable` 구현 측), 부활 처리 GameMode(WxGame), 체크포인트 액터([[WxWorld]]).

---
*문서 기준 커밋 `97577fb` · 생성일 2026-06-29 · 소스 7파일 — `/readme-writer`로 갱신*
