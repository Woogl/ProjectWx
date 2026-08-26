# WxCore — 공용 정의 파운데이션

> 모든 Wx 플러그인이 공유하는 최소 정의 모듈. C++ Native Gameplay Tag 허브, 콜리전 채널 상수, 그리고 도메인 간 직접 의존을 끊어 주는 액터측 공용 인터페이스를 제공한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언 (State / Effect / Movement / Event / Device / GameplayCue / Damage / Ability / SetByCaller / UI)
- Object Collision Channel 상수 (`ECC_WxAttack`) — `DefaultEngine.ini` 등록 순서와 일치
- 상호작용 계약 인터페이스 `IWxInteractable`
- 세이브/로드 참여 계약 인터페이스 `IWxSavable`

**경계 (비담당)**
- 태그를 소비하는 실제 게임 시스템 구현은 도메인 플러그인에 위임 — 전투는 [[WxCombat]], 상호작용/장치는 [[WxWorld]], 세이브 라이프사이클은 [[WxSave]], UI 레이어는 [[WxUI]]로. WxCore는 정의만 두고 로직을 갖지 않는다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전역 Native Tag 선언 허브 | `Source/WxCore/Public/WxGameplayTags.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수 | `Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxInteractable` | 상호작용 대상(액터)의 공용 계약 | `Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 슬롯 저장/복원 참여 마커 + 후크 | `Source/WxCore/Public/WxSavable.h` |

## Gameplay Tags
WxCore가 프로젝트의 유일한 Native Tag 선언 지점이다. 태그 추가 시 `WxGameplayTags.h`와 `WxGameplayTags.cpp` 두 파일에만 손댄다.
- 선언: `Source/WxCore/Public/WxGameplayTags.h`
- 정의: `Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — 락온/전투중/처형피대상/대화 등 액터 상태
  - `Effect.*` — GE가 부여하는 무적/가드/탈진/슈퍼아머 등
  - `Event.*` — 대미지 파이프라인 이벤트(`Event.Hit.*`는 부모 매칭으로 구독)
  - `Ability.*` — 어빌리티 식별 태그(활성 중 표식)
  - `Device.*` — 장치 StateTree 상태값(세이브 대상)
  - `GameplayCue.*` / `Damage.*` / `SetByCaller.*` / `UI.*`

## 확장 포인트 / 규약
- `IWxInteractable` / `IWxSavable`는 **액터만** 구현한다. 능력·영속 상태가 컴포넌트에 담기더라도 계약은 호스트 액터가 들고 컴포넌트로 위임한다 (대상 하나당 구현체 하나, 조회는 Cast 한 번).
- 계약을 WxCore에 둠으로써 소비 도메인이 서로를 직접 참조하지 않는다 (예: WxInventory 픽업이 WxWorld에 의존하지 않고 상호작용 대상이 됨, WxSave 소비 도메인이 WxSave에 직접 의존하지 않음).
- `IWxSavable::GetSaveId()`는 에디터에서 1회 부여되는 불변 `FGuid`. `IsValid()==false`면 저장/복원 제외.
- `ECC_WxAttack`은 `DefaultEngine.ini`의 채널 등록 순서와 반드시 일치해야 한다.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전역 태그 어휘가 여기 다 있다. 시스템 이해의 출발점.
2. `Source/WxCore/Public/WxInteractable.h` — 상호작용 계약의 설계 의도(왜 액터 단위인가)가 doc-comment에 상세히 담김.
3. `Source/WxCore/Public/WxSavable.h` — 세이브 참여 규약과 SaveGame 필드/트랜스폼 처리 규칙.

## 관련
- 상위(소비 도메인): [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]], [[WxSave]] 및 게임 모듈 WxGame. 모든 Wx 플러그인이 WxCore를 참조하며, WxCore는 어떤 Wx도 참조하지 않는다.

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 9파일 — `/readme-writer`로 갱신*
