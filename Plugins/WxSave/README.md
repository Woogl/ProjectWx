# WxSave — 세이브/로드 시스템

> 메모리 SaveGame 을 단일 원천으로, 플레이어 재개 지점·스탯과 `IWxSavable` 액터 상태를 맵 트래블을 가로질러 유지·직렬화한다. 로드도 사망 부활도 맵 리로드를 거쳐 같은 재개 지점 하나로 처리한다.

## 책임
**담당**
- 활성 SaveGame 의 수명·디스크 I/O·슬롯 정체성 관리 (`UWxSaveGameSubsystem`)
- 월드 수명 이벤트(초기화·스트리밍 인/아웃·teardown)에 맞춘 상태 플러시/복원 오케스트레이션 (`UWxSaveWorldSubsystem`)
- 액터+컴포넌트의 `UPROPERTY(SaveGame)` 바이트 직렬화 및 버전 헤더 처리
- 저장된 재개 지점·스탯을 새 세션에 세우기 (`UWxPlayerSpawnComponent` — 엔진 스폰 경로 편승)
- BP/StateTree 진입점 노출 (`UWxSaveLibrary`, `FWxStateTreeTask_SaveGame`)

**경계 (비담당)**
- 무엇이 저장 대상인가의 정의 — `IWxSavable` 인터페이스와 `WxSaveId` 는 [[WxCore]] 소유 (`Plugins/WxCore/Source/WxCore/Public/WxSavable.h`)
- 어트리뷰트의 구체 정의 — GAS `AttributeSet` 은 [[WxCombat]] 등 도메인 모듈 (여기선 ASC base 값만 이름 기반 캡처/적용)
- 저장/로드/삭제를 언제 호출할지 — UI 슬롯 흐름은 [[WxUI]]

## 의존성
- **주요 의존**: WxCore(`IWxSavable`), 엔진 GameplayAbilities(ASC base 값 캡처/적용), StateTree(오토세이브 태스크), ModularGameplay(`UControllerComponent` 주입)
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 활성 슬롯 소유·디스크 I/O·맵 트래블. 모든 저장/로드 요청의 종착점 | `Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 이벤트별 자동 플러시/복원, 액터 직렬화 실행 | `Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 디스크 슬롯 데이터 구조(트래블·플레이어·액터 레코드) | `Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 저장 재개 지점·스탯을 스폰 경로에 세우는 컨트롤러 컴포넌트 | `Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `UWxSaveLibrary` | 서브시스템 API 의 BP 정적 래퍼 | `Source/WxSave/Public/WxSaveLibrary.h` |
| `FWxStateTreeTask_SaveGame` | 라이브 전이 진입 시 권위 측 체크포인트 오토세이브 | `Source/WxSave/Public/WxStateTreeTask_SaveGame.h` |
| `FWxActorRecord` / `FWxComponentRecord` | 액터·컴포넌트 스냅샷(트랜스폼·바이트·버전 헤더) | `Source/WxSave/Public/WxSaveGame.h` |

## 확장 포인트 / 규약
- **새 저장 대상 추가**: 액터가 직접 또는 그 컴포넌트가 [[WxCore]] 의 `IWxSavable` 을 구현하고 `GetSaveId()` 로 에디터-부여 영속 GUID 를 반환하면, 월드 서브시스템이 자동 캡처/복원한다. 저장하려는 필드에 `UPROPERTY(SaveGame)` 플래그를 단다. 컴포넌트 갈래 덕에 호스트 액터는 순수 BP 여도 된다.
- **레코드 키**: `WxSaveId` GUID 가 맵을 넘어 전역 유일하므로 `ActorRecords` 는 맵별 키잉 없는 평면 `TMap<FGuid, FWxActorRecord>`.
- **버전 안전**: `FWxActorRecord::VersionHeader` 에 패키지/커스텀 버전 블롭을 레코드당 1개 보관 — 이기종 빌드로 누적되는 레코드를 안전 역직렬화. 빈 배열은 구버전 폴백.
- **권한 모델**: 트래블은 `ServerTravel`(authority 전제), 오토세이브 태스크·로그인 구독은 권위 측에서만 동작. 클라 진입은 노옵.
- **재개 지점 규약**: `PlayerTransform` 이 유일 원천 — Identity 는 "미설정" sentinel(엔진 `ChoosePlayerStart` 폴백), 좌표는 맵 종속이라 `TravelData.Map` 일치 게이트로 유효성 판정. 스탯은 맵 무관이라 `bHasPlayerStats` 만으로 적용.
- **비동기 저장**: 직렬화는 동기, 디스크 쓰기는 비동기. 기다리는 쪽은 `IsSaveInProgress()` 로 가르고 `OnSaveCompleted`(일회성) 에 붙는다.
- 콘솔 명령 `Wx.Save.Dump` → `LogSaveState()`.

## 여기서부터 읽어라
1. `Source/WxSave/Public/WxSaveGame.h` — 무엇이 저장되는지. 필드 doc-comment 가 재개 지점·스탯·레코드 키잉의 설계 근거를 담고 있다.
2. `Source/WxSave/Public/WxSaveGameSubsystem.h` — 저장/로드/트래블의 공개 API 와 슬롯·비동기 규약. 모든 흐름의 허브.
3. `Source/WxSave/Public/WxSaveWorldSubsystem.h` — 월드 이벤트별 자동 캡처/복원과 액터 직렬화가 실제로 언제 걸리는지.
4. `Source/WxSave/Public/WxPlayerSpawnComponent.h` — 로드/부활 후 플레이어가 저장 지점에서 다시 서는 엔진 편승 메커니즘.

## 관련
- 상위: [[WxUI]](명명 슬롯 저장/로드 흐름), Experience(컴포넌트 주입으로 `UWxPlayerSpawnComponent` 부착), [[WxCore]](`IWxSavable` 계약 소유)

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 13파일 — `/readme-writer`로 갱신*
