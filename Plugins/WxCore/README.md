# WxCore — 공용 정의 기반 모듈

> 모든 Wx 도메인 플러그인이 공유하는 최소 계약과 상수의 단일 소스. 태그·인터페이스·콜리전 채널을 여기에 모아, 도메인끼리 직접 의존하지 않고도 서로의 값을 읽고 서로의 대상이 되게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag의 단일 선언처 (`WxGameplayTags`)
- 도메인 경계를 넘는 공용 인터페이스 계약 — 상호작용(`IWxInteractable`), UI 표시 데이터(`IWxUIData`)
- 코드가 참조하는 콜리전 채널 상수 (`ECC_WxAttack`)
- 에디터 저작 도구용 로케이터 표시 헬퍼 (`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 태그를 소비하는 로직 — 전투는 [[WxCombat]], AI는 [[WxAI]], UI 위젯은 [[WxUI]] 등 각 도메인이 구현한다. WxCore는 태그를 선언만 하고 읽거나 쓰지 않는다.
- `IWxInteractable`·`IWxUIData`의 구현체 — 대상 도메인([[WxWorld]], [[WxInventory]], [[WxCombat]] 등)이 자기 액터·데이터에 붙인다.
- 콜리전 채널의 프로파일·응답 설정 — `DefaultEngine.ini`가 저작한다. 헤더는 그 ini 값과 일치해야 하는 상수만 제공한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전역 Native Tag 선언 네임스페이스. 태그 추가는 이 헤더와 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 액터가 구현(컴포넌트 불가), 소비 도메인이 WxWorld 의존 없이 대상이 됨 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(아이콘·이름·설명·충전) 계약. WxUI가 도메인 의존 없이 읽음 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수. ini 등록값과 일치 필수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 저작 도구용 UOL 표시명 헬퍼 (에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
프로젝트 전역 태그를 C++ Native Tag로 선언한다. 이 모듈의 사실상 중심이며, 태그 이름과 doc-comment가 여러 도메인 사이의 계약 문서 역할을 한다.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — 락온·교전·대화·소환·래그돌 등 ASC에 붙는 상태
  - `Effect.*` — GE가 부여하는 전투 효과(무적·가드·퍼펙트가드·초갑주·히트스톱 등)
  - `Event.*` — 대미지 파이프라인·상호작용·아이템·처형이 ASC에 보내는 게임플레이 이벤트
  - `Ability.*` / `Cooldown.*` — 어빌리티 식별 태그(활성 여부 = 태그 유무)와 짝 쿨다운
  - `HitReact.*` / `Damage.*` — 피격 반응 종류와 대미지 판정 표식
  - `Device.*` — 장치 State Tree 상태값(코드가 읽지 않고 정의만)
  - `GameplayCue.*` — 연출 큐
  - `SetByCaller.*` — GE Magnitude 주입 키
  - `UI.Layer.* / UI.Action.*` — CommonUI 레이어와 액션

## 확장 포인트 / 규약
- **새 태그 추가**: `WxGameplayTags.h`에 `UE_DECLARE_...`, `WxGameplayTags.cpp`에 `UE_DEFINE_...`를 짝으로 넣는다. 다른 파일에서 프로젝트 공용 태그를 새로 선언하지 않는다.
- **상호작용 대상 만들기**: 대상 액터가 `IWxInteractable`을 구현한다. `CanInteract`는 밖에서 켜고 끄는 진입점 없이 구현체가 자기 상태에서 파생한다(클라 표시 게이트와 서버 발동 검증이 같은 답). 감지·사거리는 쿼리 콜리전 형상 위에서 돈다.
- **UI에 노출할 데이터**: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트 등)이 `IWxUIData`를 구현한다. 충전 개념이 없으면 `GetMaxRecharges`는 기본값 1을 그대로 쓴다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 이 모듈의 핵심. 태그별 doc-comment가 전투·AI·UI·장치의 데이터/제어 흐름 계약을 담고 있어, 도메인 코드를 읽기 전 지도가 된다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 상호작용 계약과 그 설계 이유(액터 단위, Cast 한 번). 왜 컴포넌트가 아니라 액터가 구현하는지.
3. `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` / `WxCollisionChannels.h` — 나머지 두 계약. 짧고 독립적이라 필요할 때만.

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]])과 게임 모듈 [[WxGame]]이 WxCore를 참조한다. WxCore는 어떤 Wx 플러그인도 참조하지 않는 foundation 모듈이다.

---
*문서 기준 커밋 `ffc6360` · 생성일 2026-09-10 · 소스 11파일 — `/readme-writer`로 갱신*
