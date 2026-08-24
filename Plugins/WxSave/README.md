# WxSave — 세이브/로드 시스템

> 인메모리 SaveGame 의 수명·디스크 I/O·맵 트래블을 오케스트레이션하고, 재접속·사망 부활 시 플레이어 재개 지점/스탯과 월드 오브젝트 상태를 복원한다.

## 책임
**담당**
- 활성 SaveGame 의 수명 관리와 슬롯 정체성(SlotName/UserIndex) 보유 — GameInstance 서브시스템이 맵 트래블을 가로질러 유지.
- 저장 오케스트레이션: 라이브 상태(트래블 데이터·플레이어 트랜스폼/스탯·`IWxSavable` 액터)를 SaveGame 에 플러시한 뒤 비동기 디스크 기록.
- 로드 → ServerTravel → 새 월드 복원까지의 흐름과, 그 사이 자동 캡처를 막는 트래블 가드.
- 월드 수명 이벤트(레벨 스트리밍 인/아웃, 맵 이탈)에 맞춘 `IWxSavable` 액터 상태의 자동 캡처/복원 — UPROPERTY(SaveGame) 리플렉션 직렬화 + 버전 헤더 관리.
- 저장된 재개 지점·스탯을 새 세션 플레이어에 세우기(엔진 기본 스폰 경로에 올라타는 컨트롤러 컴포넌트).
- BP 진입점 래퍼와 StateTree 체크포인트 오토세이브 태스크, 콘솔 명령 `Wx.Save.Dump`.

**경계 (비담당)**
- 저장 대상 여부/영속 GUID 부여의 계약(`IWxSavable`, `GetSaveId()`)은 [[WxCore]] 가 정의 — 본 모듈은 그 인터페이스를 소비만 한다.
- 저장/로드를 언제 트리거할지(사망 화면·메뉴 UI 등)는 호출부([[WxUI]] 등)와 StateTree 그래프의 몫.
- 어트리뷰트 값의 의미·AttributeSet 정의는 GAS/전투 도메인([[WxCombat]]) 소관 — 본 모듈은 base 값을 이름 기준으로 스냅샷/복원만 한다.
- 어떤 컴포넌트를 컨트롤러에 주입할지는 Experience 설정의 몫(등록 안 하면 재개/스탯 복원이 조용히 비활성).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 저장/로드/트래블의 오케스트레이터, SaveGame 소유자 (진입점 1순위) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 단위 플러시/복원, 레벨 스트리밍·teardown 시 자동 캡처 | `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 디스크에 직렬화되는 데이터 컨테이너(트래블 데이터·플레이어 트랜스폼/스탯·액터 레코드) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 저장된 재개 지점·스탯을 새 세션 플레이어에 세우는 컨트롤러 컴포넌트 | `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `UWxSaveLibrary` | BP 진입점 — 서브시스템 공개 API 의 정적 래퍼 | `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` |
| `FWxStateTreeTask_SaveGame` | StateTree 체크포인트 오토세이브 태스크(권위 측·라이브 전이 한정) | `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h` |
| `FWxActorRecord` | 액터+컴포넌트의 SaveGame 프로퍼티 직렬화 결과 + 버전 헤더 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |

## 확장 포인트 / 규약
- **액터를 저장 대상으로 만들기**: [[WxCore]] 의 `IWxSavable` 를 구현하고 에디터에서 영속 GUID(`GetSaveId()`)를 부여한다. 월드 서브시스템이 스트리밍/트래블 시점에 자동 캡처·복원한다. 저장 필드는 `UPROPERTY(SaveGame)` 로 표시하며, 아키타입 기본값과 다른 프로퍼티만 기록된다(`ShouldSave`).
- **저장 트리거**: BP/코드는 `UWxSaveLibrary` 또는 서브시스템 API(`SaveToFile`/`LoadFromFile`/`TravelFromSaveFile`)로 요청. 슬롯 이름을 비우면 활성 슬롯에 그대로 기록/재읽기(체크포인트·사망 리스폰 경로), 지정하면 활성 슬롯을 재지정(명명 세이브 경로).
- **체크포인트 오토세이브**: StateTree 그래프에 `FWxStateTreeTask_SaveGame` 를 배치. `ResumePoint` 를 물리면 그 자리로 재개 지점 확정, 비우면 저장 시점 폰 위치를 캡처. 초기 진입/클라 진입은 노옵.
- **플레이어 스폰 커스터마이즈**: 재개 지점은 SaveGame 이 단일 원천이고, `UWxPlayerSpawnComponent` 가 엔진 `ChoosePlayerStart`/`ShouldSpawnAtStartSpot` 경로에 올라탄다. 이 컴포넌트를 Experience 컴포넌트 주입 액션에 반드시 등록해야 동작.
- **버전 안전성**: 레코드마다 `[FPackageFileVersion][FCustomVersionContainer]` 블롭(`VersionHeader`)을 보관해 이기종 빌드 누적을 견딘다. 빈 배열은 구버전 레코드로 취급.
- **비동기 기록 대기**: 디스크 쓰기는 비동기이므로 요청 직후 `IsSaveInProgress()` 로 확인하고, 진행 중이면 `OnSaveCompleted`(일회성) 에 붙는다.

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` — 저장/로드/트래블의 전체 API 와 슬롯·트래블 가드 규약. 헤더 doc-comment 가 각 경로(오토세이브·사망 리스폰·명명 세이브)의 대칭 규칙을 상세히 설명한다.
2. `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` — 자동 캡처/복원이 언제 어느 이벤트에서 걸리는지, 트래블 중 스킵 규칙, 직렬화 판정(`ShouldSave`/`CaptureActor`/`RestoreActor`).
3. `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` — 디스크로 나가는 데이터 형태와 각 필드의 sentinel/게이트 규칙.
4. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 저장 대상 계약(`IWxSavable`) 정의. 본 모듈이 소비하는 인터페이스.

## 관련
- 상위: 저장 대상 액터를 구현하는 도메인 모듈([[WxWorld]] 의 `WxDevice`·`WxSpawner` 등이 `IWxSavable` 구현), 저장/로드 UI 를 트리거하는 [[WxUI]], 스탯 스냅샷이 대상으로 삼는 GAS 어트리뷰트([[WxCombat]]).
- 함께 보기: 저장 계약을 정의하는 [[WxCore]].

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 13파일 — `/readme-writer`로 갱신*
