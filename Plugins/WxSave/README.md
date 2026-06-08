# WxSave — 세이브/로드 시스템

> 체크포인트 슬롯에 월드 액터 상태와 플레이어 부활 위치를 저장/복원하는 도메인 플러그인. 사망·재접속 시 플레이어를 마지막 체크포인트로 되돌리고, World Partition 셀 왕복으로 인한 상태 손실을 방지한다.

## 책임
**담당**
- 슬롯 파일 단위 세이브/로드: `IWxSavableInterface` 액터의 `UPROPERTY(SaveGame)` 필드를 액터의 영속 `WxSaveId`(GUID) 키로 직렬화/역직렬화.
- 플레이어 부활 위치 보존: 저장 시점 첫 플레이어 Pawn Transform 을 슬롯에 캡처하고, GameMode 가 `ChoosePlayerStart` 에서 조회하도록 노출.
- World Partition 셀 스트리밍 자동 처리: 스트리밍-인 셀 액터 자동 복원, 스트리밍-아웃 직전 자동 캡처로 셀 왕복 손실 방지.
- `ServerTravel` 을 가로지르는 세이브 메모리 유지 (`GameInstanceSubsystem`).
- BP 진입점 제공: 정적 래퍼 `SaveSlot`/`LoadSlot`.

**경계 (비담당)**
- 무엇을 저장할지의 정의: 각 액터가 `UPROPERTY(SaveGame)` 플래그로 보존 필드를 지정한다(저장 대상 액터는 `[[WxWorld]]` 의 기믹·스포너 등).
- `IWxSavableInterface` 마커/후크 정의: `[[WxCore]]` 에 위치(WxSave 와 소비 도메인의 상호 직접 의존 방지).
- 부활 진입점의 실제 적용: `ChoosePlayerStart` 에서 부활 PlayerStart 생성은 `[[WxGame]]` 의 `AWxGameMode` 가 수행.

## 의존성
- **주요 의존**: `WxCore` (`IWxSavableInterface`). 그 외 엔진 기본(`Core`/`CoreUObject`/`Engine`)만 사용하며 특수 서브시스템 의존은 없다.
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 모듈 단일 진입점. 슬롯 저장/로드, 부활 Transform 조회, 셀 스트리밍 핸들러 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveGameLibrary` | BP 진입점. 서브시스템 API 의 정적 래퍼(`SaveSlot`/`LoadSlot`) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameLibrary.h` |
| `UWxSaveGame` | 슬롯 데이터. `WxSaveId`→스냅샷 맵 + 플레이어 부활 Transform | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `FWxActorRecord` | 한 액터의 스냅샷(Transform + 본체/컴포넌트별 직렬화 바이트) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `FWxComponentRecord` | 한 컴포넌트의 `UPROPERTY(SaveGame)` 직렬화 바이트 wrapper | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `IWxSavableInterface` | 저장 참여 마커 + `OnWxSaveRestored()` 후크 (WxCore 에 정의) | `Plugins/WxCore/Source/WxCore/Public/WxSavableInterface.h` |

## 확장 포인트 / 규약
- **새 저장 대상 추가**: 액터가 `IWxSavableInterface`(WxCore) 를 구현하고, 보존할 필드에 `UPROPERTY(SaveGame)` 플래그를 단다. 복원 직후 시각/인터랙션 동기화가 필요하면 `OnWxSaveRestored()` 를 오버라이드한다(BeginPlay 이전 호출될 수 있음).
- **안정 키 규약**: 액터는 `IWxSavableInterface::GetWxSaveId()` 가 반환하는 영속 `WxSaveId` 를 키로 사용한다. 이 값은 에디터에서 `GetActorGuid()`(에디터 전용 API)로부터 복사·직렬화되어 쿠킹 빌드/세션 간에도 안정적이다. 런타임 스폰 액터는 키가 불안정해 본 경로로 저장되지 않는다(플레이어 Pawn 은 Transform 만 별도 캡처).
- **컴포넌트 직렬화**: `AActor::Serialize` 가 컴포넌트의 `SaveGame` 필드를 자동으로 끌고 가지 않으므로, 컴포넌트는 `FName` 으로 별도 캡처/복원된다(`FObjectAndNameAsStringProxyArchive`, `ArIsSaveGame = true`).
- **부활 sentinel**: `PlayerRespawnTransform == FTransform::Identity` 는 "미설정"(신규 세션/체크포인트 미터치) 을 뜻한다. 따라서 회전·스케일 디폴트인 월드 원점에 체크포인트를 두지 않는다.
- **PIE 격리**: 모든 월드 핸들러는 `IsOwnedGameWorld` 로 자기 `GameInstance` 의 게임 월드만 처리한다.
- **로드 흐름**: `LoadSlot` 은 슬롯을 메모리에 올린 뒤 같은 맵으로 `ServerTravel` 하여 새 월드에서 자동 복원 + fresh Pawn 부활을 유도한다(`GameInstanceSubsystem` 이라 메모리가 트래블을 가로질러 유지됨). 저장은 `AsyncSaveGameToSlot` 으로 비동기 기록.

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` — 모듈의 공개 API와 셀 스트리밍 핸들러의 의도를 헤더 주석이 전부 설명한다.
2. `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp` — 캡처/복원 직렬화, 스트리밍 핸들러, ServerTravel 부활 흐름의 실제 구현.
3. `Plugins/WxCore/Source/WxCore/Public/WxSavableInterface.h` — 저장 대상이 되기 위한 계약(마커 + 복원 후크).

## 관련
- 소비처: `[[WxWorld]]` (기믹·스포너 등 저장 대상 액터), `[[WxGame]]` (`AWxGameMode::ChoosePlayerStart` 가 부활 Transform 사용).
- 계약 정의: `[[WxCore]]` (`IWxSavableInterface`).

---
*문서 기준 커밋 `af085f20` · 생성일 2026-06-09 · 소스 7파일 — `/readme-writer`로 갱신*
