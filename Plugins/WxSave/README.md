# WxSave — 세이브/로드 시스템

> 인메모리 SaveGame 하나를 중심으로 슬롯 디스크 I/O·맵 트래블·액터/플레이어 상태 캡처와 복원을 오케스트레이션한다. 로드도 사망 부활도 "마지막 저장 시점의 재개 지점" 하나로 재개하는 것이 설계 축이다.

## 책임
**담당**
- 활성 SaveGame 의 수명·디스크 I/O·슬롯 정체성 관리 (GameInstance 서브시스템, 맵 트래블을 가로질러 유지)
- 월드 수명 이벤트(초기화/스트리밍 인·아웃/맵 이탈)에 맞춘 `IWxSavable` 액터 상태 자동 캡처·복원 (World 서브시스템)
- 저장 시점 플레이어 스냅샷(트랜스폼·ASC 어트리뷰트) 캡처와, 로드/부활 후 스폰 경로에서의 복원
- BP 진입점(라이브러리)과 StateTree 체크포인트 오토세이브 태스크, `Wx.Save.Dump` 콘솔 명령

**경계 (비담당)**
- 무엇이 "저장 가능"인지 정의하는 `IWxSavable` 인터페이스·영속 GUID 부여는 [[WxCore]] 소유. 이 모듈은 그 인터페이스를 소비만 한다.
- 저장 UI(슬롯 목록·명명·덮어쓰기 확인)는 [[WxUI]] 몫. 이 모듈은 API만 노출한다.

## 의존성
- **주요 의존**: `WxCore`(IWxSavable), `GameplayAbilities`(ASC 어트리뷰트 캡처/복원), `StateTreeModule`(체크포인트 태스크), `ModularGameplay`(GameFrameworkComponent 등록)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (Wx 의존은 `WxCore` 뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 인메모리 SaveGame 소유·디스크 I/O·맵 트래블 오케스트레이션. 모든 저장/로드의 허브 | `Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 수명 이벤트에 맞춰 SaveGame 을 읽고 쓰는 플러시/복원 담당 | `Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 슬롯 데이터 컨테이너: 슬롯 정체성 + 트래블 데이터 + 플레이어 스냅샷 + 액터 레코드 맵 | `Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 저장 재개 지점·스탯을 엔진 스폰 경로(StartSpot)에 주입하는 컨트롤러 컴포넌트 | `Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `UWxSaveLibrary` | 서브시스템 공개 API 의 BP 정적 래퍼(로드/저장/트래블/슬롯 존재·삭제) | `Source/WxSave/Public/WxSaveLibrary.h` |
| `FWxStateTreeTask_SaveGame` | 라이브 전이 진입 시 권위 측 활성 슬롯 오토세이브(체크포인트) | `Source/WxSave/Public/WxStateTreeTask_SaveGame.h` |
| `FWxActorRecord` | 액터 1개의 스냅샷(Transform + 액터/컴포넌트 SaveGame 바이트 + 버전 헤더) | `Source/WxSave/Public/WxSaveGame.h` |

## 확장 포인트 / 규약
- **저장 대상 등록**: `IWxSavable`(WxCore) 을 구현한 액터가 자동 저장/복원 대상이다. 키는 `GetSaveId()` 가 반환하는 에디터-부여 영속 GUID(쿠킹 안전, 맵 전역 유일). 액터·컴포넌트의 `UPROPERTY(SaveGame)` 만 직렬화된다.
- **재개 지점 주입**: `UWxPlayerSpawnComponent` 는 GameMode 가 고른 `Experience` 에셋에 등록해야 부착된다 — 미등록 시 재개 지점·스탯 복원이 조용히 동작하지 않는다. `PlayerTransform` 이 단일 원천이고 `Identity` 는 "미설정" sentinel(→ 엔진 `ChoosePlayerStart` 폴백).
- **슬롯 규약**: 슬롯 정체성(SlotName/UserIndex)은 SaveGame 이 보유한다. `SaveToFile`/`LoadFromFile` 에 빈 SlotName 은 "활성 슬롯 그대로"(체크포인트·사망 리스폰 경로), 지정 SlotName 은 활성 슬롯 재지정(명명 세이브)이다.
- **직렬화 버전**: 레코드마다 `[FPackageFileVersion][FCustomVersionContainer]` 블롭을 저장해, 세션·빌드를 넘어 누적된 이기종 레코드를 안전하게 읽는다. 빈 배열은 구버전 레코드로 현재 빌드 버전으로 읽는다.
- **권한 모델**: 맵 트래블(ServerTravel)·오토세이브는 authority 전제. 클라 진입은 노옵. 로드/부활은 모두 맵 리로드를 거쳐 스폰 경로를 다시 탄다.
- **로드 트래블 가드**: `IsTravelingFromSaveFile()` 동안 월드 서브시스템의 자동 캡처가 전부 스킵돼, 막 로드한 세이브가 라이브 상태로 오염되는 것을 막는다.
- **콘솔**: `Wx.Save.Dump` → `UWxSaveGameSubsystem::LogSaveState` 로 현재 메모리 슬롯 덤프.

## 여기서부터 읽어라
1. `Source/WxSave/Public/WxSaveGameSubsystem.h` — 저장/로드/트래블의 허브. 슬롯·트래블·빈 SlotName 규약이 함수 주석에 다 있다.
2. `Source/WxSave/Public/WxSaveGame.h` — 무엇이 저장되는가(액터 레코드·플레이어 스냅샷·버전 헤더)의 데이터 모양.
3. `Source/WxSave/Public/WxSaveWorldSubsystem.h` — 언제 캡처/복원이 자동 발화하는가(월드 수명 이벤트 매핑).
4. `Source/WxSave/Public/WxPlayerSpawnComponent.h` — 로드/부활 후 플레이어가 어떻게 재개 지점에 서는가.

## 관련
- 상위: [[WxCore]] (IWxSavable·저장 대상 정의)

---
*문서 기준 커밋 `21e2e76` · 생성일 2026-07-27 · 소스 13파일 — `/readme-writer`로 갱신*
