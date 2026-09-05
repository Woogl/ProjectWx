# WxCore — 공용 기반 정의

> 모든 Wx 플러그인이 공유하는 최하단(foundation) 계약과 상수를 담는다. 도메인들이 서로를 직접 참조하지 않고도 태그·인터페이스·충돌 채널을 통해 맞물리게 하는 접점이다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언 (전투·장치·UI·어빌리티 등)
- 도메인 간 공용 인터페이스 계약 (`IWxInteractable`, `IWxUIData`)
- 커스텀 충돌 채널 상수 (`ECC_WxAttack`)
- 에디터 저작용 로케이터 표시 헬퍼 (`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 태그를 실제로 소비·발행하는 로직은 각 도메인이 구현 — 전투 [[WxCombat]], 상호작용 대상 액터 [[WxWorld]], UI 레이어 [[WxUI]]
- WxCore는 어떤 Wx 플러그인도 참조하지 않는다 (DAG 최하단)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전 프로젝트 Native Tag를 한곳에 모은 네임스페이스 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 액터가 구현하는 공용 계약 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(아이콘·이름·설명)의 공용 계약 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명 헬퍼 (`WITH_EDITOR`) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` (정의는 `Private/WxGameplayTags.cpp`)
- 태그 추가는 이 두 파일에만 작성한다 (헤더 규약 명시)
- 주요 네임스페이스:
  - `State.*` / `Effect.*` / `Movement.*` — ASC에 붙는 상태·효과 태그
  - `HitReact.*` / `Event.*` / `Damage.*` — 대미지 파이프라인이 주고받는 이벤트·판정 결과
  - `Ability.*` / `Cooldown.*` / `SetByCaller.*` — 어빌리티 식별·쿨다운·계산 입력
  - `Device.*` — 장치 State Tree 상태값
  - `GameplayCue.*` — 연출 큐
  - `UI.Layer.*` / `UI.Action.*` — HUD 레이어와 CommonUI 액션

## 확장 포인트 / 규약
- 새 태그: 헤더에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, cpp에 `UE_DEFINE_GAMEPLAY_TAG_STATIC` 를 짝으로 추가. 다른 파일에는 흩지 않는다.
- 상호작용 대상: 액터가 `IWxInteractable`을 구현한다 — 컴포넌트는 구현하지 않으며, 능력이 컴포넌트에 있어도 액터가 계약을 들고 위임한다. 소비 도메인(예: 인벤토리 픽업)이 WxWorld에 의존하지 않고 자기 액터를 상호작용 대상으로 만드는 통로.
- UI 표시 데이터: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트 등)이 `IWxUIData`를 구현해 WxUI가 도메인 의존 없이 값을 읽는다.
- `ECC_WxAttack`은 `DefaultEngine.ini`의 채널 등록 순서와 반드시 일치해야 한다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전체 시스템의 지도. 태그 주석이 각 도메인의 제어 흐름을 요약한다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 간 계약이 왜 WxCore에 있는지 보여주는 대표 예.

## 관련
- 상위: 모든 Wx 도메인 플러그인이 WxCore에 의존한다 — [[WxCombat]] [[WxWorld]] [[WxUI]] [[WxInventory]] [[WxAI]] 등

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 11파일 — `/readme-writer`로 갱신*
