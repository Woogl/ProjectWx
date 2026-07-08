# WxSave — 세이브/로드 시스템

> 슬롯 파일 기반 세이브/로드를 담당한다. 메모리 SaveGame 의 수명·디스크 I/O·맵 트래블을 오케스트레이션하고, 월드 수명 이벤트에 맞춰 `IWxSavable` 액터 상태와 플레이어 위치·GAS 스탯을 캡처/복원한다.

## 책임
**담당**
- 활성 SaveGame 슬롯의 소유·수명 관리(맵 트래블을 가로질러 유지), 이름 지정 슬롯 저장/로드/삭제·존재확인, 슬롯 리셋 (`UWxPersistenceGameSubsystem`)
- 저장된 맵으로의 ServerTravel 및 로드-트래블 가드(`IsTravelingFromSaveFile`)로 막 로드한 세이브 오염 방지
- 월드 수명 이벤트(레벨 스트리밍 인/아웃, teardown)에 맞춘 `IWxSavable` 액터 상태의 자동 캡처·복원 (`UWxPersistenceWorldSubsystem`)
- 액터+컴포넌트의 `UPROPERTY(SaveGame)` 바이트 직렬화와 이기종 빌드 안전을 위한 레코드 단위 버전 헤더 처리
- 플레이어 ASC 어트리뷰트 base 값 스냅샷 캡처/적용, 부활/시작 지점 식별자(`PlayerStartTag`)의 슬롯 기록
- 명시(이름 지정) 저장 시 플레이어 폰 트랜스폼 캡처(`TravelData.PlayerTransform`) → 로드 시 저장 지점 복원. 오토세이브·사망 리스폰은 위치를 미기록으로 두어 체크포인트 PlayerStart 로 폴백

**경계 (비담당)**
- 무엇을 저장할지 표시하는 `IWxSavable` 인터페이스(안정 GUID 키 `GetWxSaveId()` 포함) 정의는 [[WxCore]]의 `WxSavable.h`
- 저장된 `PlayerStartTag`/폰 트랜스폼을 소비하는 실제 플레이어 스폰 선정(`ChoosePlayerStart`) 은 GameMode 등 게임 코드. 이 모듈은 태그·트랜스폼을 저장/제공만 한다.

## 의존성
- **주요 의존**: `WxCore`(`IWxSavable`), `GameplayAbilities`(플레이어 ASC 어트리뷰트 base 값 캡처/적용 — `CapturePlayerStats`/`ApplyPlayerStats`). 엔진 서브시스템 `UGameInstanceSubsystem`·`UWorldSubsystem`, `USaveGame`(SaveGameToSlot/LoadGameFromSlot).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (uplugin·Build.cs 모두 WxCore + 엔진 GameplayAbilities 만 의존)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxPersistenceGameSubsystem` | 슬롯 소유·디스크 I/O·맵 트래블 오케스트레이션의 중심. 저장/로드 전 흐름의 시작점 (GameInstance 수명) | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceGameSubsystem.h` |
| `UWxPersistenceWorldSubsystem` | 월드 수명 이벤트에 맞춘 savable 액터 자동 캡처/복원 + `RequestSaveFlush` + GAS 스탯 캡처/적용 | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceWorldSubsystem.h` |
| `UWxPersistenceSaveGame` | 슬롯 데이터 컨테이너(TravelData + `ActorRecords` + `PlayerStats` + `PlayerStartTag`) | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `FWxActorRecord` / `FWxComponentRecord` | 액터·컴포넌트 상태 스냅샷(Transform + 바이트 + 컴포넌트별 레코드 + 버전 헤더) | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `FWxPersistenceTravelData` | 로드 시 트래블 대상 맵(`FSoftObjectPath Map`) + 명시 저장 시점 폰 트랜스폼(`FTransform PlayerTransform`, Identity=미기록 sentinel) | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `UWxSaveFilePersistenceUtils` | BP 진입점. 게임 서브시스템 공개 API 의 정적 래퍼(이름 지정 Save/Load/Reload/Travel/Delete·존재확인) | `Plugins/WxSave/Source/WxSave/Public/WxSaveFilePersistenceUtils.h` |
| `WxPersistence::DefaultSlotName` | 개발 기본 슬롯 `"Test"` (체크포인트·PIE 자동로드 폴백; UI 는 슬롯 이름을 직접 지정) | `Plugins/WxSave/Source/WxSave/Public/WxPersistenceGameSubsystem.h` |

## 확장 포인트 / 규약
- **새 세이브 대상 추가**: 액터가 [[WxCore]]의 `IWxSavable` 을 구현하고 안정적 `WxSaveId`(에디터 부여 영속 GUID, 쿠킹 빌드 안전) 를 반환하면 월드 서브시스템이 스트리밍/트래블 이벤트에서 자동 캡처·복원한다. 별도 등록 코드 불필요. 저장할 필드엔 `UPROPERTY(SaveGame)` 지정.
- **커스텀 슬롯 데이터**: `UWxPersistenceSaveGame` 을 서브클래싱하고 `StartNewSaveFile` 의 `SpecificClass` 로 지정한다.
- **이름 지정 슬롯**: `SaveToFile(SlotName)` 은 슬롯 이름을 넘기면 활성 슬롯을 그 이름으로 재지정한 뒤 저장한다(빈 이름은 활성 슬롯 유지 — 체크포인트 오토세이브 경로). `LoadFromFile`·`DeleteSaveFile`·`DoesSaveFileExist` 로 슬롯 단위 로드/삭제/조회. 슬롯 목록 열거·메타데이터는 미지원.
- **저장 지점 복원**: `SaveToFile(SlotName)`(비빈 이름 = 명시 저장)만 `RequestSaveFlush` 에 캡처 게이트를 넘겨 폰 트랜스폼을 `TravelData.PlayerTransform` 에 기록한다. 로드 후 GameMode `SpawnDefaultPawnAtTransform` 가 저장 위치(맵 일치 시)를 스폰 지점보다 우선 적용하고, `FinishRestartPlayer` 가 로드 직후 카메라(컨트롤 로테이션)를 복원된 폰 회전 Yaw 로 맞춘다(시선은 별도 저장 없이 폰 회전에서 파생). 오토세이브(`SaveToFile()`)는 `FlushMapTravelData` 의 TravelData 재구성으로 위치가 Identity 로 리셋돼 미기록이 되고, 사망 리스폰은 `PlayerStartTag` 체크포인트로 폴백한다.
- **슬롯 키잉**: `ActorRecords` 는 `FGuid`(전역 유일) 평면 맵이라 샘플의 맵별 키잉이 필요 없다.
- **버전 안전성**: 레코드마다 `[FPackageFileVersion][FCustomVersionContainer]` 헤더 블롭을 보관해 이기종 빌드 누적 복원 시 `FMemoryReader` 커스텀 버전 리셋 함정을 막는다(빈 헤더는 구버전 레코드로 현재 빌드 버전 적용).
- **권한 모델**: `TravelFromSaveFile` 은 authority(서버) 전제 ServerTravel. 로드-트래블 중 자동 캡처는 전부 스킵된다.
- **콘솔**: `Wx.Save.Dump` → `LogSaveState` 로 현재 메모리 슬롯 덤프.

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` — 무엇이 저장되는지(레코드·트래블 데이터·PlayerStats·PlayerStartTag) 데이터 모델부터 잡으면 나머지가 읽힌다. 각 필드 주석에 설계 의도가 있다.
2. `Plugins/WxSave/Source/WxSave/Public/WxPersistenceGameSubsystem.h` — 시작/로드/저장/트래블 전체 API 와 슬롯 수명.
3. `Plugins/WxSave/Source/WxSave/Public/WxPersistenceWorldSubsystem.h` — 언제 자동 캡처/복원이 일어나는지(월드 이벤트별 정리).
4. `Plugins/WxSave/Source/WxSave/Private/WxPersistenceGameSubsystem.cpp` — `Wx.Save.Dump` 콘솔 명령과 실제 I/O·트래블 구현.

## 관련
- 상위: [[WxCore]] (`IWxSavable` 정의 소유). 저장 대상 액터를 제공하는 도메인([[WxWorld]] 등 — 스포너·기믹·체크포인트)과 로드 후 스폰을 처리하는 GameMode, 세이브 API 를 호출하는 [[WxUI]] 가 이 모듈의 소비자다.

---
*문서 기준 커밋 `81ac3e4` · 생성일 2026-07-07 · 소스 9파일 — `/readme-writer`로 갱신*
