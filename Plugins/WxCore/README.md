# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 인용하는 공용 정의의 원천. Native Gameplay Tag, 콜리전 채널, 도메인 간 인터페이스 계약을 여기 한곳에서 선언해 도메인 플러그인끼리 서로를 참조하지 않고도 같은 어휘를 공유하게 한다.

## 책임
**담당**
- C++ Native Gameplay Tag 선언의 단일 원천 (`WxGameplayTags`)
- 프로젝트 공용 콜리전 채널 상수 (`ECC_WxAttack`)
- 도메인 간 상호작용 계약 인터페이스 (`IWxInteractable`)
- 저작 도구용 로케이터 표시 헬퍼 (`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 태그를 실제로 발행·소비하는 게임플레이 로직 — 각 도메인 ([[WxCombat]], [[WxWorld]], [[WxUI]] 등)
- 상호작용 구현체(대화·장치·픽업) — [[WxWorld]]·[[WxDialogue]]·[[WxInventory]]가 각자 액터에 구현

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 모든 Native Gameplay Tag 선언 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상의 공용 계약(액터가 구현) | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | UniversalObjectLocator 표시명 헬퍼(에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `.../Private/WxGameplayTags.cpp`
- 태그 추가·수정은 이 두 파일에만 작성한다.
- 주요 네임스페이스:
  - `State.*` — 폰 ASC의 loose 상태 태그 (대화 중 등)
  - `Effect.*` — GE가 부여하는 상태(무적·가드·슈퍼아머·탈진 등)
  - `Movement.*` / `HitReact.*` — 이동 상태 / 피격 반응 종류
  - `Event.*` — 대미지 파이프라인·장치·처형·아이템 등 GameplayEvent 트리거
  - `Device.*` — 장치 State Tree 상태값(세이브 대상)
  - `GameplayCue.*` — 연출 큐
  - `Damage.*` — ExecCalc 판정 플래그(크리티컬·가드브레이크·패리 가능 등)
  - `Ability.*` — 어빌리티 식별 태그(활성 여부 겸용)
  - `SetByCaller.*` — GE SetByCaller 키
  - `UI.*` — CommonUI 레이어·액션

## 확장 포인트 / 규약
- **태그 소비**: 각 도메인은 새 태그를 자기 모듈에서 만들지 말고 이 파일에 선언한 뒤 인용한다. 그래야 대미지 파이프라인(공격자/피격자)·장치 트리·UI가 같은 태그 인스턴스를 공유한다.
- **콜리전 채널**: `ECC_WxAttack`은 `DefaultEngine.ini`의 채널 등록 순서(`ECC_GameTraceChannel1`)와 반드시 일치해야 한다.
- **상호작용 계약**: 대상 액터가 `IWxInteractable`을 구현한다(컴포넌트 아님, BP 구현 금지). `CanInteract`의 켜짐 여부는 구현체가 자기 상태에서 파생하며 밖에서 켜고 끄지 않는다 — 클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다. 계약이 WxCore에 있어 소비 도메인이 [[WxWorld]]에 의존하지 않고도 자기 액터를 상호작용 대상으로 만든다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 모듈의 실질적 본체. 프로젝트 전역 태그 어휘와 각 태그의 발행·소비 주체가 주석에 정리돼 있다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 경계를 가로지르는 유일한 인터페이스 계약.

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]]·[[WxInventory]]·[[WxUI]]·[[WxWorld]]·[[WxAI]]·[[WxDialogue]]·[[WxQuest]])과 [[WxGame]]이 WxCore를 참조한다. WxCore는 다른 Wx 플러그인을 참조하지 않는다(foundation).

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 9파일 — `/readme-writer`로 갱신*
