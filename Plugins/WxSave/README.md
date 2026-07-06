# WxSave — 세이브/로드 시스템

> 메모리 SaveGame 슬롯의 수명과 디스크 I/O, 맵 트래블을 오케스트레이션해 재접속·부활 시 플레이어 위치와 savable 액터 상태를 복원한다. 저장 대상 액터는 `IWxSavable`(WxCore) 을 구현해 스스로 등록한다.

## 책임
**담당**
- 활성 SaveGame 슬롯의 소유·수명 관리(맵 트래블을 가로질러 유지), 디스크 저장/로드, 슬롯 리셋
- 저장된 맵으로의 ServerTravel 및 로드-트래블 가드(`IsTravelingFromSaveFile`)로 막 로드한 세이브 오염 방지
- 월드 수명 이벤트(레벨 스트리밍 인/아웃, teardown)에 맞춘 `IWxSavable` 액터 상태의 자동 캡처·복원
- 액터+컴포넌트의 `UPROPERTY(SaveGame)` 바이트 직렬화와 이기종 빌드 안전을 위한 버전 헤더 처리
- 부활/시작 지점 식별자(`PlayerStartTag`)의 슬롯 기록

**경계 (비담당)**
- savable 액터의 실제 상태·`GetWxSaveId()` 구현 → [[WxWorld]] 등 각 도메인 (인터페이스 정의는 [[WxCore]]의 `IWxSavable`)
- 저장된 `PlayerStartTag`/`PawnTransform` 를 소비하는 플레이어 스폰 로직 → GameMode의 `ChoosePlayerStart`
- 체크포인트 액터 정의 → [[WxWorld]] (`AWxCheckPoint`)

## 의존성
- **주요 의존**: [[WxCore]] (`IWxSavable` 인터페이스). 엔진 서브시스템 `UGameInstanceSubsystem`·`UWorldSubsystem`, `USaveGame`(SaveGameToSlot/LoadGameFromSlot).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (uplugin·Build.cs 모두 WxCore만 의존)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxPersistenceGameSubsystem` | 슬롯 소유·디스크 I/O·맵 트래블 오케스트레이션의 중심. 저장/로드 전 흐름의 시작점 | `Source/WxSave/Public/WxPersistenceGameSubsystem.h` |
| `UWxPersistenceWorldSubsystem` | 월드 수명 이벤트에 맞춘 savable 액터 자동 캡처/복원 + `RequestSaveFlush` | `Source/WxSave/Public/WxPersistenceWorldSubsystem.h` |
| `UWxPersistenceSaveGame` | 슬롯 데이터 컨테이너(TravelData + `ActorRecords` + `PlayerStartTag`) | `Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `UWxSaveFilePersistenceUtils` | BP 진입점. 게임 서브시스템 공개 API의 정적 래퍼 | `Source/WxSave/Public/WxSaveFilePersistenceUtils.h` |
| `FWxPersistenceTravelData` | 트래블 맵 + 폰 트랜스폼 + 컨트롤 로테이션 (위치 복원 데이터) | `Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `FWxActorRecord` | 액터 상태 스냅샷(Transform + 바이트 + 컴포넌트별 레코드 + 버전 헤더) | `Source/WxSave/Public/WxPersistenceSaveGame.h` |
| `WxPersistence::DefaultSlotName` | 개발 기본 슬롯 `"Test"` (체크포인트·PIE 자동로드·UI 공유) | `Source/WxSave/Public/WxPersistenceGameSubsystem.h` |

## 확장 포인트 / 규약
- **새 세이브 대상 추가**: 액터가 [[WxCore]]의 `IWxSavable` 을 구현하고 안정적 `WxSaveId`(에디터 부여 GUID) 를 반환하면, 월드 서브시스템이 스트리밍/트래블 이벤트에서 자동으로 캡처·복원한다. 별도 등록 코드 불필요.
- **직렬화 대상 필드**: 저장하려는 프로퍼티에 `UPROPERTY(SaveGame)` 을 지정한다. 액터 본체와 컴포넌트 각각 직렬화되며, 버전 헤더가 레코드 단위로 붙어 이기종 빌드 누적에도 안전하다.
- **슬롯 키잉**: `ActorRecords` 는 `FGuid`(전역 유일) 평면 맵이라 맵별 키잉이 필요 없다.
- **권한 모델**: `TravelFromSaveFile` 은 authority(서버) 전제 ServerTravel. 로드-트래블 중 자동 캡처는 전부 스킵된다.
- **콘솔**: `Wx.Save.Dump` → `LogSaveState` 로 현재 메모리 슬롯 덤프.

## 여기서부터 읽어라
1. `Source/WxSave/Public/WxPersistenceGameSubsystem.h` — 저장/로드/트래블 전체 API와 각 함수의 흐름 주석. 시스템의 진입점.
2. `Source/WxSave/Public/WxPersistenceSaveGame.h` — 무엇이 저장되는지(레코드·트래블 데이터·PlayerStartTag)의 데이터 모델.
3. `Source/WxSave/Public/WxPersistenceWorldSubsystem.h` — 언제 자동 캡처/복원이 일어나는지(월드 이벤트별 표).
4. `Source/WxSave/Private/WxPersistenceWorldSubsystem.cpp` — `CaptureActor`/`RestoreActor` 직렬화·버전 헤더 구현.

## 관련
- 상위: [[WxWorld]] (`IWxSavable` 구현 액터 — 스포너·기믹·체크포인트), [[WxCore]] (`IWxSavable` 인터페이스 정의), [[WxUI]]·GameMode(스폰 경로)에서 세이브 API 소비.

---
*문서 기준 커밋 `7a536dd` · 생성일 2026-07-06 · 소스 9파일 — `/readme-writer`로 갱신*
