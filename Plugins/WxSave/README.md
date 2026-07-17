# WxSave — 세이브/로드 시스템

> 슬롯 파일 기반 세이브/로드 플러그인. 메모리 SaveGame 의 수명·디스크 I/O·맵 트래블을 오케스트레이션하고, 월드 수명 이벤트에 맞춰 `IWxSavable` 액터 상태·GAS 플레이어 스탯·재개 지점을 캡처/복원한다. 체크포인트 오토세이브와 사망 리스폰(리로드 트래블)을 지원한다.

## 책임
**담당**
- 활성 SaveGame 슬롯의 소유·수명 관리(맵 트래블을 가로질러 유지), 이름 지정 슬롯 저장/로드/삭제·존재확인, 슬롯 리셋 (`UWxSaveGameSubsystem`)
- 저장된 맵으로의 `ServerTravel` 및 로드-트래블 가드(`IsTravelingFromSaveFile`)로 막 로드한 세이브 오염 방지
- 월드 수명 이벤트(레벨 스트리밍 인/아웃, teardown, BeginPlay)에 맞춘 `IWxSavable` 액터 상태의 자동 캡처·복원 (`UWxSaveWorldSubsystem`)
- 액터+컴포넌트 `UPROPERTY(SaveGame)` 바이트 직렬화와 이기종 빌드 안전을 위한 레코드 단위 버전 헤더 처리
- 저장 시점 플레이어 스냅샷 캡처/제공 — ASC 어트리뷰트 base 값(`PlayerStats`)과 재개 지점 트랜스폼(`PlayerTransform`)

**경계 (비담당)**
- `IWxSavable` 인터페이스 및 안정 키 `GetWxSaveId()` 정의 → [[WxCore]] (`Plugins/WxCore/Source/WxCore/Public/WxSavable.h`)
- 저장된 `PlayerTransform` 을 소비하는 실제 스폰 선정(우선 적용 + `ChoosePlayerStart` 폴백)은 `UWxPlayerSpawnComponent`(게임 코드). 이 모듈은 값만 저장/제공한다.
- 저장을 트리거하는 체크포인트·기믹 액터는 [[WxWorld]] 소관. 세이브/로드 UI 는 [[WxUI]] 소관. 이 모듈은 BP 정적 래퍼만 노출한다.
- 어트리뷰트 정의·구체 AttributeSet 타입은 GAS/전투 도메인 소관. 이 모듈은 복제되는 base 값만 이름 기준으로 다룬다.

## 의존성
- **주요 의존**: `WxCore`(`IWxSavable`), `GameplayAbilities`(플레이어 ASC 어트리뷰트 base 캡처/적용 — `CapturePlayerStats`/`ApplyPlayerStats`), 엔진 서브시스템 `UGameInstanceSubsystem`·`UWorldSubsystem`·`USaveGame`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (uplugin·Build.cs 모두 WxCore + 엔진 GameplayAbilities 만 의존)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 슬롯 소유·디스크 I/O·맵 트래블 오케스트레이션의 중심. 저장/로드 전 흐름의 시작점 (GameInstance 수명) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 수명 이벤트별 savable 액터 자동 캡처/복원 + `RequestSaveFlush` + GAS 스탯 캡처/적용 | `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 슬롯 데이터 컨테이너(`TravelData` + `ActorRecords` + `PlayerStats` + `PlayerTransform`) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `FWxActorRecord` / `FWxComponentRecord` | 액터·컴포넌트 스냅샷(Transform + 바이트 + 컴포넌트별 레코드 + 버전 헤더) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `FWxSaveTravelData` | 트래블 대상 맵(`FSoftObjectPath Map`, null=구버전/미기록→현재 맵 리로드 폴백) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `UWxSaveLibrary` | BP 진입점. 게임 서브시스템 공개 API 의 정적 래퍼(Save/Load/Travel/Delete·존재확인·활성 슬롯 조회) | `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` |

## 확장 포인트 / 규약
- **새 세이브 대상 추가**: 액터가 [[WxCore]]의 `IWxSavable` 을 구현하고 안정적 `WxSaveId`(에디터 부여 영속 GUID, 쿠킹 빌드 안전)를 반환하면 월드 서브시스템이 스트리밍/트래블 이벤트에서 자동 캡처·복원한다. 별도 등록 코드 불필요. 저장할 필드엔 `UPROPERTY(SaveGame)` 지정(액터 본체 + 컴포넌트별).
- **커스텀 슬롯 데이터**: `UWxSaveGame` 을 서브클래싱하고 `StartNewSaveFile` 의 `SpecificClass` 로 지정.
- **이름 지정 슬롯**: 빈 슬롯 이름은 양쪽 다 "활성 슬롯"을 뜻한다 — `SaveToFile(SlotName)` 은 이름을 넘기면 활성 슬롯을 그 이름으로 재지정 후 저장하고(이후 저장도 그 슬롯을 이어감), `LoadFromFile(SlotName)` 은 이름을 넘기면 그 슬롯을, 비면 활성 슬롯을 다시 읽는다(사망 리스폰 경로). 슬롯 목록 열거·메타데이터는 미지원.
- **재개 지점 복원**: `SaveToFile` 플러시가 저장 시점의 플레이어 트랜스폼을 `PlayerTransform` 으로 캡처하고, 로드 후 `UWxPlayerSpawnComponent` 이 `TryGetPlayerTransform` 으로 소비한다. 로드도 사망 부활도 이 값 하나로 재개하므로 좌표 원천은 하나다(체크포인트 등 외부가 세팅하지 않는다). `Identity` 는 "미설정" sentinel — 이때 스폰은 `ChoosePlayerStart` 폴백. 좌표라 맵 종속이므로 `TravelData.Map` 일치 게이트와 함께 유효성 판정. 스탯은 맵 무관이라 `bHasPlayerStats` 만으로 적용된다.
- **슬롯 키잉**: `ActorRecords` 는 `FGuid`(전역 유일) 평면 맵이라 맵별 키잉이 필요 없다.
- **버전 안전성**: 레코드마다 `[FPackageFileVersion][FCustomVersionContainer]` 헤더 블롭을 보관해 이기종 빌드 누적 복원 시 `FMemoryReader` 커스텀 버전 리셋 함정을 막는다(빈 헤더는 구버전 레코드로 현재 빌드 버전 적용).
- **권한/타이밍**: `TravelFromSaveFile` 은 authority(서버) 전제 `ServerTravel`. 로드-트래블 시작~새 월드 `OnWorldBeginPlay`(→`ReportTravelFromSaveFileComplete`) 사이 자동 캡처는 전부 스킵된다.
- **콘솔**: `Wx.Save.Dump` → `LogSaveState` 로 현재 메모리 슬롯 덤프.

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` — 무엇이 저장되는지(레코드·트래블 데이터·PlayerStats·PlayerTransform) 데이터 모델부터 잡으면 나머지가 읽힌다. 필드 주석에 설계 의도가 담겨 있다.
2. `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` — 시작/로드/저장/트래블 전체 API 와 슬롯 수명·트래블 가드.
3. `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` — 언제 자동 캡처/복원이 일어나는지(월드 이벤트별 정리).
4. `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp` — `Wx.Save.Dump` 콘솔 명령과 실제 I/O·트래블 구현.

## 관련
- 상위: [[WxCore]] (`IWxSavable` 정의 소유). 저장 대상 액터를 제공하는 도메인([[WxWorld]] 등), 로드 후 스폰을 처리하는 `UWxPlayerSpawnComponent`, 세이브 API 를 호출하는 [[WxUI]] 가 소비자다.

---
*문서 기준 커밋 `842f761` · 생성일 2026-07-14 · 소스 9파일 — `/readme-writer`로 갱신*
