# WxSave — 세이브/로드 시스템

> 슬롯 기반 세이브 파일에 플레이어 재개 지점·스탯과 `IWxSavable` 액터 상태를 캡처하고, 로드 시 저장된 맵으로 트래블해 복원한다. 로드도 사망 부활도 "마지막 저장 재개 지점" 하나로 처리하는 것이 설계 축이다.

## 책임
**담당**
- 메모리 SaveGame 의 수명·디스크 I/O·맵 트래블 오케스트레이션 (`UWxSaveGameSubsystem`). 슬롯은 맵 트래블을 가로질러 유지된다.
- 월드 수명 이벤트(레벨 초기화/스트리밍 인·아웃/맵 이탈)에 맞춘 `IWxSavable` 액터 자동 캡처·복원 (`UWxSaveWorldSubsystem`).
- 저장된 재개 지점·스탯을 새 세션 플레이어에 세우기 — 엔진 스폰 경로(`StartSpot`/`ChoosePlayerStart`)에 올라탐 (`UWxPlayerSpawnComponent`).
- 액터/컴포넌트 `UPROPERTY(SaveGame)` 필드의 바이트 직렬화 + 이기종 빌드 대비 레코드 단위 버전 헤더 관리 (`FWxActorRecord`).
- 저장 시점 플레이어 스냅샷 캡처/적용 — ASC 어트리뷰트 base 값과 재개 지점 트랜스폼.

**경계 (비담당)**
- 세이브 참여 마커·후크 인터페이스 `IWxSavable`/`GetSaveId()` 정의는 [[WxCore]] 소유 (`WxSavable.h`). WxSave 는 이를 소비만 한다.
- 저장 대상 액터의 실제 보존 필드 선정·복원 후처리(`OnWxSaveRestored`)와 저장 트리거(체크포인트/기믹)는 [[WxWorld]] 등 소비 도메인 몫.
- 세이브/로드 UI 는 [[WxUI]] 소관 — 이 모듈은 BP 정적 래퍼만 노출한다.
- 어트리뷰트 정의·구체 AttributeSet 타입은 GAS/전투 도메인 소관 — WxSave 는 이름-값 맵으로만 왕복한다.

## 의존성
- **주요 의존**: [[WxCore]] (`IWxSavable`), `GameplayAbilities`(ASC 어트리뷰트 base 캡처/적용), `ModularGameplay`(GameFramework 컴포넌트 등록). 참여 인터페이스가 WxCore 에 있어 WxSave 와 소비 도메인이 서로 직접 의존하지 않는다.
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (uplugin·Build.cs 모두 WxCore + 엔진 GameplayAbilities/ModularGameplay 만 의존)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 슬롯 소유·디스크 I/O·맵 트래블의 중심(GameInstance 수명). 저장/로드 전 흐름의 시작점 | `Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 이벤트별 savable 액터 자동 캡처/복원 + `RequestSaveFlush` + 플레이어 스탯 캡처/적용 | `Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 슬롯 데이터(`TravelData` + `ActorRecords` + `PlayerStats` + `PlayerTransform`) | `Source/WxSave/Public/WxSaveGame.h` |
| `FWxActorRecord` / `FWxComponentRecord` | 액터·컴포넌트 스냅샷(Transform + 바이트 + 컴포넌트 레코드 + 버전 헤더) | `Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 저장 재개 지점·스탯을 스폰 경로에 주입(컨트롤러 컴포넌트) | `Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `UWxSaveLibrary` | 세이브/로드/트래블/삭제 BP 정적 래퍼 | `Source/WxSave/Public/WxSaveLibrary.h` |

## 확장 포인트 / 규약
- **저장 대상 등록**: 액터에 `IWxSavable`([[WxCore]]) 구현 + 유효한 `GetSaveId()`(에디터 부여 영속 GUID, 쿠킹 빌드 안전) 반환, 보존 필드에 `UPROPERTY(SaveGame)` 플래그. 무효 GUID 반환 시 저장/복원에서 제외. 별도 등록 코드 불필요 — 월드 서브시스템이 스트리밍/트래블 이벤트에서 자동 처리.
- **커스텀 슬롯 데이터**: `UWxSaveGame` 서브클래싱 후 `StartNewSaveFile` 의 `SpecificClass` 로 지정.
- **이름 지정 슬롯**: 빈 슬롯 이름은 양쪽 다 "활성 슬롯"을 뜻한다. `SaveToFile(SlotName)` 은 이름을 넘기면 활성 슬롯을 그 이름으로 재지정 후 저장(이후도 그 슬롯 이어감), `LoadFromFile(SlotName)` 은 비면 활성 슬롯을 다시 읽는다(사망 리스폰 경로). 슬롯 목록 열거·메타데이터는 미지원.
- **재개 지점**: `SaveToFile` 플러시가 플레이어 트랜스폼을 `PlayerTransform` 으로 캡처(외부가 세팅하지 않음), 로드 후 `UWxPlayerSpawnComponent` 이 `TryGetPlayerTransform` 으로 소비. `Identity` 는 "미설정" sentinel → `ChoosePlayerStart` 폴백. 좌표는 맵 종속이라 `TravelData.Map` 일치 게이트로 유효성 판정(크로스맵 오적용 차단). 스탯은 맵 무관이라 `bHasPlayerStats` 만으로 적용.
- **슬롯 키잉**: `ActorRecords` 는 `FGuid`(전역 유일) 평면 맵이라 맵별 키잉 불필요.
- **버전 안전**: 레코드마다 `[FPackageFileVersion][FCustomVersionContainer]` 헤더 블롭을 보관 — 세션 넘어 이기종 빌드로 누적되는 레코드의 `FMemoryReader` 커스텀 버전 리셋 함정을 막는다. 빈 헤더는 구버전 레코드(현재 빌드 버전으로 읽음).
- **권한/타이밍**: `TravelFromSaveFile` 은 authority(서버) 전제 `ServerTravel`. 로드-트래블 시작~새 월드 `OnWorldBeginPlay`(→`ReportTravelFromSaveFileComplete`) 사이 자동 캡처는 전부 스킵된다(막 로드한 세이브 오염 방지).
- **콘솔**: `Wx.Save.Dump` → `UWxSaveGameSubsystem::LogSaveState` 로 현재 메모리 슬롯 덤프.

## 여기서부터 읽어라
1. `Source/WxSave/Public/WxSaveGame.h` — 무엇이 저장되는지(레코드·트래블 데이터·PlayerStats·PlayerTransform) 데이터 모델부터 잡으면 나머지가 읽힌다. 필드 주석에 설계 의도가 담겨 있다.
2. `Source/WxSave/Public/WxSaveGameSubsystem.h` — 시작/로드/저장/트래블 전체 API 와 슬롯 수명·트래블 가드(빈 슬롯 대칭, Identity sentinel).
3. `Source/WxSave/Private/WxSaveWorldSubsystem.cpp` — 언제·어떻게 자동 캡처/복원이 일어나는지(월드 이벤트별 실제 직렬화 구현).
4. `Source/WxSave/Public/WxPlayerSpawnComponent.h` — "세이브가 단일 원천, 마커는 파생물" 스폰 전략. GameMode `FrameworkComponents` 등록 필수 조건 주의.

## 관련
- 상위: [[WxCore]] (`IWxSavable` 정의 소유)
- 소비 도메인: [[WxWorld]] (기믹/스포너 등 savable 액터·저장 트리거), [[WxUI]] (세이브/로드 API 호출)

---
*문서 기준 커밋 `465b77a` · 생성일 2026-07-17 · 소스 11파일 — `/readme-writer`로 갱신*
