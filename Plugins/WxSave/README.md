# WxSave — 세이브/로드 시스템

> 메모리 SaveGame 슬롯을 소유하고 디스크 I/O·맵 트래블·월드 단위 상태 플러시/복원을 오케스트레이션하는 세이브/로드 플러그인. `IWxSavable` 액터의 `UPROPERTY(SaveGame)` 필드와 플레이어 재개 지점·어트리뷰트 스냅샷을 슬롯에 담아, 로드/사망 부활을 마지막 세이브 하나로 재개한다.

## 책임
**담당**
- 활성 SaveGame 의 수명 관리와 슬롯 정체성(`SlotName`/`UserIndex`) 보유 — `UWxSaveGameSubsystem`(GameInstance 수명, 맵 트래블 가로질러 유지).
- 디스크 저장/로드/삭제·존재 확인, 저장된 맵으로의 `ServerTravel`, 로드 트래블 가드(`IsTravelingFromSaveFile`).
- 월드 수명 이벤트(영구 레벨 초기화·스트리밍 인/아웃·맵 이탈)에 맞춘 `IWxSavable` 액터 자동 캡처/복원 — `UWxSaveWorldSubsystem`.
- 액터+컴포넌트 `UPROPERTY(SaveGame)` 바이너리 직렬화와 커스텀 버전 헤더 보존(이기종 빌드 안전).
- 저장된 재개 지점에 플레이어를 스폰하고 어트리뷰트를 복원 — `UWxPlayerSpawnComponent`(엔진 `ChoosePlayerStart` 경로에 올라탐) + GAS ASC base 값 캡처/적용.
- BP 진입점(`UWxSaveLibrary`), 체크포인트 오토세이브용 StateTree 태스크, 콘솔 명령 `Wx.Save.Dump`.

**경계 (비담당)**
- `IWxSavable` 인터페이스 정의는 WxSave 가 아닌 **WxCore** 에 위치한다(WxSave 와 소비 도메인이 서로 직접 의존하지 않게 하기 위함).
- 명명 슬롯 UI(로드/삭제/덮어쓰기 흐름)는 미담당 — 라이브러리 API 를 호출하는 UI 몫이다.
- 어트리뷰트의 구체 `AttributeSet` 타입은 모름 — GAS ASC 만 알고 프로퍼티 이름 기반으로 base 값을 오간다.

## 의존성
- **주요 의존**: `WxCore`(`IWxSavable`), GameplayAbilities(GAS ASC 스탯 캡처/적용), StateTree(체크포인트 오토세이브 태스크), ModularGameplay(`UControllerComponent` 주입).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 없음 ✅ (Wx 의존은 `WxCore` 하나뿐).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxSaveGameSubsystem` | 활성 SaveGame 소유·디스크 I/O·맵 트래블·로드 가드 | `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` |
| `UWxSaveWorldSubsystem` | 월드 수명 이벤트 기반 `IWxSavable` 자동 캡처/복원·플러시 | `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` |
| `UWxSaveGame` | 슬롯 데이터 모델(트래블 데이터·플레이어 재개 지점·스탯·액터 레코드) | `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` |
| `UWxPlayerSpawnComponent` | 저장된 재개 지점 스폰 + 어트리뷰트 복원(컨트롤러 컴포넌트) | `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h` |
| `UWxSaveLibrary` | BP 진입점(저장/로드/삭제/존재/트래블 정적 래퍼) | `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` |
| `FWxStateTreeTask_SaveGame` | 체크포인트 오토세이브 ST 태스크(권위 측·라이브 전이 진입 시 플러시) | `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h` |
| `IWxSavable` | 저장 대상 마커+후크(정의는 WxCore) | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |

## 확장 포인트 / 규약
- 저장 대상이 되려면 액터(또는 그 컴포넌트)가 `IWxSavable`(WxCore)을 구현하고, `GetSaveId()` 로 에디터-부여 영속 `FGuid`(WxSaveId)를 안정적 키로 반환한다 — `IsValid()==false` 면 저장/복원 제외. 컴포넌트가 구현해도 직렬화 대상은 액터+전체 컴포넌트다(호스트 액터를 순수 BP 로 둘 수 있음; 인터페이스는 BP 구현 불가).
- 보존할 필드는 `UPROPERTY(SaveGame)` 플래그만 표시하면 되고, 필드 단위 직렬화는 그 플래그가 처리한다. 복원 직후 `OnSaveRestored()` 가 호출된다(BeginPlay 이전일 수 있음).
- `UWxPlayerSpawnComponent` 는 Experience 의 컴포넌트 주입 액션에 등록해야 부착된다 — 등록하지 않으면 재개 지점·스탯 복원이 조용히 동작하지 않는다.
- 슬롯 규약: 저장/로드에 빈 `SlotName` 을 넘기면 활성 슬롯을 그대로 이어간다(체크포인트 오토세이브·사망 리스폰 경로). 이름을 지정하면 활성 슬롯을 그 이름으로 재지정한다(명명 세이브).

## 여기서부터 읽어라
1. `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h` — 저장/로드/트래블의 공개 API 와 슬롯 규칙·가드 흐름이 모두 여기 문서화돼 있다. 시스템의 제어 중심.
2. `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h` — 언제 무엇이 자동 캡처/복원되는지(스트리밍·맵 이탈·로드 가드)의 데이터 흐름.
3. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 저장 대상이 지켜야 할 계약. 새 액터를 세이브에 태우려면 먼저 읽는다.
4. `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` — 슬롯에 실제로 무엇이 담기는지(레코드·재개 지점·스탯·버전 헤더)의 데이터 모델.

## 관련
- 상위: [[WxCore]] (`IWxSavable` 인터페이스 소유)
- 소비 도메인: [[WxWorld]] (세이브 대상 월드 오브젝트)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 13파일 — `/readme-writer`로 갱신*
