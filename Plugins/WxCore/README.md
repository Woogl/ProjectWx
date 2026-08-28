# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 함께 참조하는 최하위 공용 계층. Gameplay Tag, 콜리전 채널, 도메인 간 계약 인터페이스를 한곳에 모아 도메인 플러그인끼리 직접 의존하지 않게 만든다.

## 책임
**담당**
- 프로젝트 전역에서 쓰는 C++ Native Gameplay Tag 선언·정의 (`WxGameplayTags`)
- 커스텀 Object Channel 등 콜리전 상수 (`WxCollisionChannels`)
- 도메인 경계를 넘는 계약 인터페이스: 상호작용 대상(`IWxInteractable`), 세이브 참여 액터(`IWxSavable`)
- 저작 도구용 로케이터 표시 헬퍼(`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 태그·인터페이스의 실제 소비/구현은 각 도메인이 한다 — 상호작용 실행은 [[WxWorld]]·[[WxInventory]], 세이브 슬롯 라이프사이클은 [[WxSave]], 전투 어빌리티·대미지 파이프라인은 [[WxCombat]], UI 레이어는 [[WxUI]]
- 어빌리티의 성질(배타 그룹 등)은 태그가 아니라 `EWxAbilityActivationGroup`([[WxCombat]])이 선언

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전 도메인 태그의 단일 선언처. 태그 추가는 이 파일 + cpp 에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 액터가 구현(컴포넌트 아님) | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 슬롯 참여 마커 + 후크. 액터가 구현 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명 헬퍼(에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — 락온/전투/대화 등 액터 상태 (네임플레이트·어빌리티 차단 조건)
  - `Effect.*` — GE가 부여하는 상태(무적·가드·탈진·슈퍼아머 등), 애셋 태그 겸용
  - `Event.Hit.*` / `Event.DamageDealt` — 대미지 파이프라인이 서버에서 ASC에 보내는 이벤트. `Event.Hit` 부모 매칭으로 구독할 것(정확 매칭은 반응 자식 히트를 놓침)
  - `Movement.*` / `Damage.*` — 공중·질주 상태, 크리·가드·패리 가능 여부 플래그
  - `Ability.*` — 어빌리티 식별 태그(활성 여부 = 태그 보유). 플레이어/적 공용·전용 구분
  - `Device.*` — 장치의 StateTree 상태값(세이브에 저장). 코드가 읽고 쓰진 않되 태그는 여기 정의
  - `SetByCaller.*` — GE 매그니튜드 키(Duration·DP·ATK 계수·RawDamage 등)
  - `GameplayCue.*` / `UI.Layer.*` / `UI.Action.*` — 큐, CommonUI 레이어·액션

## 확장 포인트 / 규약
- 새 태그: 반드시 `WxGameplayTags.h` 선언 + `WxGameplayTags.cpp` 정의 쌍으로만 추가. 다른 곳에서 정의 금지
- 새 상호작용 대상: 대상 **액터**에 `IWxInteractable` 구현(컴포넌트에 능력이 있어도 계약은 액터가 들고 위임). 감지·사거리는 쿼리 콜리전 프리미티브 위에서 도므로 최소 하나 필요
- 새 세이브 참여 액터: `IWxSavable` 구현 + `UPROPERTY(SaveGame)` 필드 + 안정적 `GetSaveId()`(에디터에서 1회 부여해 영속 UPROPERTY로 보관)
- 콜리전 채널 상수는 `DefaultEngine.ini`의 채널 등록 순서와 일치해야 함

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 태그 doc-comment가 각 시스템의 데이터·제어 흐름 지도 역할을 한다. 여기부터 보면 전 도메인 배선이 잡힌다
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` / `WxSavable.h` — 도메인 경계를 어떻게 인터페이스로 끊는지(왜 액터가 계약을 드는지) 설명

## 관련
- 상위: 모든 Wx 도메인 플러그인이 참조하는 유일한 공용 의존처. [[WxCombat]] [[WxWorld]] [[WxSave]] [[WxInventory]] [[WxUI]] 등이 여기 태그·인터페이스를 소비

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 11파일 — `/readme-writer`로 갱신*
