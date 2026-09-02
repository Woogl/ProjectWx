# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 공유하는 최하단 계약·상수 모음. 도메인 플러그인들이 서로를 직접 참조하지 않고도 태그·인터페이스·콜리전 채널로 맞물리게 하는 접점이다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언 (전투·장치·UI·어빌리티·쿨다운 등 전 도메인 공용 태그의 단일 출처)
- 도메인 간 공용 인터페이스 계약: 상호작용(`IWxInteractable`), UI 표시 데이터(`IWxUIData`)
- 콜리전 채널 상수(`ECC_WxAttack`) — `DefaultEngine.ini` 등록 순서와 짝

**경계 (비담당)**
- 태그를 실제로 발행·소비하는 로직은 각 도메인이 담당 ([[WxCombat]], [[WxWorld]], [[WxUI]] 등). WxCore는 태그의 이름만 소유한다.
- 상호작용 감지·사거리·픽업 등 구현은 [[WxWorld]]·[[WxInventory]]가, 어빌리티·GE는 [[WxCombat]]가 계약을 구현한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags::*` | 전 도메인 공용 Native Tag의 단일 선언처 | `Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 액터가 구현하는 계약 (`CanInteract`/`OnInteracted`/`GetInteractionPrompt`) | `Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(아이콘·이름·설명·충전)의 계약 | `Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수 | `Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 에디터 전용 로케이터 표시명 헬퍼 | `Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Source/WxCore/Public/WxGameplayTags.h` / 정의: `Source/WxCore/Private/WxGameplayTags.cpp` (태그 추가 시 이 두 파일에만 작성)
- 주요 네임스페이스:
  - `State.*` / `Effect.*` — ASC에 걸리는 상태·GE 부여 태그
  - `Event.*` — 대미지·처형·상호작용·소환 등 GameplayEvent 트리거
  - `HitReact.*` / `Damage.*` — 피격 반응 종류와 대미지 판정 결과
  - `Ability.*` / `Cooldown.*` — 어빌리티 식별 태그와 짝이 되는 쿨다운 태그
  - `Device.*` — 장치 State Tree 상태값
  - `GameplayCue.*` — 연출 큐
  - `SetByCaller.*` — GE SetByCaller 키
  - `UI.Layer.*` / `UI.Action.*` — CommonUI 레이어·액션

## 확장 포인트 / 규약
- 새 공용 태그: `WxGameplayTags.h` 선언 + `.cpp` 정의 두 곳에만. 소비 도메인에서 참조.
- 상호작용 대상 추가: 대상 **액터**가 `IWxInteractable`을 구현(컴포넌트 아님, 대상당 구현체 하나). 능력이 컴포넌트에 있어도 계약은 액터가 들고 넘긴다.
- UI 표시 데이터 노출: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트 등)이 `IWxUIData` 구현.
- 콜리전: `ECC_WxAttack`은 `DefaultEngine.ini` 채널 등록 순서와 반드시 일치.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 태그 doc-comment가 전투/장치/UI 데이터·제어 흐름의 지도 역할을 한다. 다른 모듈을 볼 때 함께 편다.
2. `Source/WxCore/Public/WxInteractable.h` — 상호작용 계약이 왜 액터 단위이고 도메인 경계를 어떻게 끊는지 설명.
3. `Source/WxCore/Public/WxUIData.h` — WxUI가 도메인에 의존하지 않는 이유.

## 관련
- 상위: 모든 Wx 플러그인이 이 모듈을 참조한다(DAG 최하단). 특히 [[WxCombat]]·[[WxWorld]]·[[WxUI]]·[[WxInventory]]가 태그·인터페이스 계약의 주 소비처.

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 11파일 — `/readme-writer`로 갱신*
