# WxSave — 세이브/로드 시스템

> 슬롯 단위 SaveGame 의 수명·디스크 I/O·맵 트래블과, `IWxSavable` 액터·플레이어(재개 지점·스탯)의 캡처/복원을 담당한다. 로드와 사망 부활을 재개 지점 하나로 통합한다.

## 책임
**담당**
- 메모리 SaveGame 소유와 디스크 슬롯 I/O(저장·로드·존재확인·삭제), 슬롯 정체성(SlotName/UserIndex) 관리 — `UWxSaveGameSubsystem`
- 저장된 맵으로의 ServerTravel 오케스트레이션과 트래블 완료 가드(`IsTravelingFromSaveFile`)
- 월드 수명 이벤트(레벨 초기화·스트리밍 인/아웃·teardown)에 맞춘 `IWxSavable` 액터 자동 캡처/복원 — `UWxSaveWorldSubsystem`
- 액터+컴포넌트의 `UPROPERTY(SaveGame)` 직렬화(버전 헤더 포함)와 GUID 키 스냅샷 저장 — `FWxActorRecord`
- 플레이어 재개 지점(트랜스폼)·GAS 어트리뷰트 스냅샷의 캡처/복원과 스폰 경로 주입 — `UWxPlayerSpawnComponent`
- BP 진입점 노출 및 StateTree 체크포인트 오토세이브 태스크

**경계 (비담당)**
- savable 대상의 저장 정체성(`GetSaveId`)·직렬화 계약 정의는 [[WxCore]] (`IWxSavable`)
- 저장 대상이 되는 월드 오브젝트(기믹·스포너)의 상태 정의는 [[WxWorld]]
- 명명 슬롯 UI(로드/삭제/덮어쓰기 화면)는 [[WxUI]] (본 모듈은 BP API 만 노출)

## 의존성
- **주요 의존**: `WxCore`(`IWxSavable`), 엔진 GameplayAbilities(GAS 어트리뷰트 캡처/복원), ModularGameplay(`UControllerComponent` 주입), StateTree(체크포인트 태스크)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | GameInstance 서브시스템 — SaveGame 소유·디스크 I/O·트래블의 최상위 오케스트레이터 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | World 서브시스템 — 월드 수명에 맞춘 플러시(`RequestSaveFlush`)·복원, savable 액터 캡처 | `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 슬롯 데이터 컨테이너 — 트래블 데이터·플레이어 스냅샷·`ActorRecords`(GUID→레코드) 보관 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `FWxActorRecord` | 한 액터 스냅샷 — Transform·본체/컴포넌트 바이트·버전 헤더 블롭 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 컨트롤러 컴포넌트 — PostLogin 에서 재개 지점을 StartSpot 에 주입, 빙의 시 스탯 복원 | `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `FWxStateTreeTask_SaveGame` | StateTree 태스크 — 라이브 전이 진입 시 권위 측에서 활성 슬롯에 체크포인트 오토세이브 | `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h` |
| `UWxSaveLibrary` | Blueprint Function Library — 서브시스템 공개 API 의 정적 BP 래퍼 | `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` |
| `IWxSavable` | 저장 대상 마커+후크(정의는 WxCore) | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |

## 확장 포인트 / 규약
- **저장 대상 추가**: 대상 액터(또는 그 컴포넌트)에 WxCore 의 `IWxSavable` 을 구현하고 `UPROPERTY(SaveGame)` 로 저장 필드를 표시한다. `GetSaveId()` 의 에디터-부여 영속 GUID 가 슬롯 키가 되므로 쿠킹 빌드에서도 안정적. `FindSavable` 이 액터 직접 구현 → 컴포넌트 순으로 찾으므로 호스트 액터를 순수 BP 로 둘 수 있다. 복원 직후 후처리는 `OnSaveRestored()`.
- **커스텀 슬롯 데이터**: `UWxSaveGame` 을 상속하고 `StartNewSaveFile` 에 `SpecificClass` 로 넘긴다.
- **버전 안전성**: 각 레코드가 `[FPackageFileVersion][FCustomVersionContainer]` 헤더 블롭을 자체 보관해, 세션을 넘어 이기종 빌드로 누적된 레코드도 올바른 커스텀 버전으로 복원한다(`FMemoryReader` 의 버전 리셋 함정 회피).
- **권한/리플리케이션**: 세이브 파일은 서버가 소유한다. 트래블·저장·체크포인트 태스크는 authority 전제이며 클라 진입은 노옵. 스폰 컴포넌트는 클라 PC 사본에도 붙으므로 로그인 구독을 권위로 한정하는 것이 자기 책임. Experience 컴포넌트 주입 액션에 미등록 시 재개 지점·스탯 복원이 조용히 실패한다.
- **재개 지점 단일화**: 로드도 사망 부활도 맵 리로드를 거쳐 `UWxPlayerSpawnComponent` 의 StartSpot 주입 경로를 다시 타므로, `PlayerTransform` 하나가 두 경우를 모두 재개시킨다(Identity 는 미설정 sentinel → `ChoosePlayerStart` 폴백, 좌표는 `TravelData.Map` 일치 게이트로 크로스맵 오적용 차단). 체크포인트는 ST 태스크의 `ResumePoint` 로 자리 확정, 비우면 저장 시점 폰 위치.
- **콘솔**: `Wx.Save.Dump` → `UWxSaveGameSubsystem::LogSaveState`, 현재 메모리 슬롯 상태 덤프.

## 데이터 흐름 (파일 횡단)
- **저장**: `SaveToFile` → `UWxSaveWorldSubsystem::RequestSaveFlush` 가 `FlushMapTravelData`·`FlushPlayerTransform`·`FlushPlayerStats` + 전체 `IWxSavable` `CaptureActor` 로 SaveGame 을 채운 뒤 → `ContinueSaveToFileToDisk` 가 비동기 디스크 기록(`IsSaveInProgress` 폴링). 체크포인트 경로는 `FWxStateTreeTask_SaveGame` 이 `ResumePoint` 를 실어 이 흐름을 트리거.
- **로드**: `LoadFromFile` → `TravelFromSaveFile` 이 `TravelData.Map` 으로 ServerTravel → 새 월드의 `UWxSaveWorldSubsystem` 이 `RestoreActor` 로 `IWxSavable` 복원 → `UWxPlayerSpawnComponent` 가 재개 트랜스폼을 StartSpot 으로 주입하고 빙의 시 스탯 적용 → `OnWorldBeginPlay` 가 `ReportTravelFromSaveFileComplete` 로 트래블 가드 해제.

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` — 저장/로드/트래블 전체 흐름의 진입점. 각 API 주석에 빈 슬롯 규칙·트래블 가드 등 계약이 명시돼 있다.
2. `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` — 언제 무엇이 자동 캡처/복원되는지(스트리밍·teardown·트래블 스킵)의 오케스트레이션.
3. `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` — 저장 데이터 스키마와 버전 헤더·GUID 키잉의 근거.
4. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 저장 대상이 구현하는 인터페이스 계약(다른 모듈).

## 관련
- 상위: [[WxCore]] (`IWxSavable` 저장 계약) · [[WxWorld]] (savable 월드 오브젝트) · [[WxUI]] (명명 슬롯 화면)

---
*문서 기준 커밋 `c549ea2` · 생성일 2026-07-31 · 소스 13파일 — `/readme-writer`로 갱신*
