# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 참조하는 최하위 기반 모듈. Gameplay Tag, 콜리전 채널, 도메인 간 공용 인터페이스처럼 여러 시스템이 공유해야 하는 정의를 한곳에 모아, 소비 도메인끼리 서로를 직접 참조하지 않고도 계약을 주고받게 한다.

## 책임
**담당**
- C++ Native Gameplay Tag 전체 선언 (전투·UI·장치·어빌리티·쿨다운 등 프로젝트 태그 사전)
- 도메인 간 공용 인터페이스 정의 (`IWxInteractable`, `IWxUIData`, `IWxMinion`)
- 프로젝트 공용 상수 (콜리전 채널 `ECC_WxAttack`)
- 에디터 전용 로케이터 표시 헬퍼 (`FWxLocatorUtils`)

**경계 (비담당)**
- 인터페이스의 실제 구현체·시스템 로직은 소비 도메인이 든다 (상호작용 판정은 [[WxWorld]], 태그 발행/소비 어빌리티는 [[WxCombat]] 등). WxCore는 계약만 정의한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 프로젝트 전 태그를 선언하는 네임스페이스. 태그 추가는 이 헤더와 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 액터가 구현하는 공용 계약. 소비 도메인이 WxWorld 의존 없이 자기 액터를 상호작용 대상으로 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | 아이콘·이름·설명 등 UI가 그대로 표시하는 데이터 계약. WxUI가 도메인 의존 없이 읽음 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `IWxMinion` | 소환 가능한 액터의 계약(상한·어그로 무시). BlueprintNativeEvent | `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h` |
| `ECC_WxAttack` | 히트박스 Object Channel 상수. DefaultEngine.ini 등록 값과 일치해야 함 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 저작 도구용 로케이터 표시명 헬퍼 (에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 태그 추가·수정은 반드시 이 두 파일에만 작성 (헤더 상단 규약).
- 주요 네임스페이스:
  - `State.*` — ASC에 붙는 상태 (LockedOn, Engaged, Dialogue, Minion.Active, Ragdoll)
  - `Effect.*` — GE가 부여하는 전투 효과 (Invincible, GuardReduction, PerfectGuard, SuperArmor, HitStop 등)
  - `Event.*` — GameplayEvent 트리거 (Hit, DamageDealt, Finisher, Death, Interact, UseItem, Device.Triggered 등)
  - `HitReact.*` — 피격 반응 종류 (Normal/KnockBack/KnockDown/KnockUp)
  - `Damage.*` — 대미지 표식·판정 결과 (Attack, Critical, GuardBreak, CanGuard, CanParry 등)
  - `Ability.*` / `Cooldown.*` — 어빌리티 식별 태그와 짝 쿨다운 태그
  - `Device.*` — 장치 StateTree 상태값 (코드가 읽지 않고 태그만 정의)
  - `UI.Layer.*` / `UI.Action.*` — CommonUI 레이어·액션
  - `GameplayCue.*`, `Movement.*`, `SetByCaller.*`

## 확장 포인트 / 규약
- 도메인은 서로를 직접 참조하지 않고 WxCore 인터페이스로 계약을 주고받는다. 대상 쪽이 인터페이스를 구현하면 소비 쪽은 WxCore만 알면 된다.
- 어빌리티 식별 규약: 각 어빌리티는 자신의 `Ability.X` 태그 하나를 AssetTags·ActivationOwnedTags 양쪽에 넣어 "그 태그 = 지금 활성"이 성립하게 한다. 짝 쿨다운은 같은 이름의 `Cooldown.X`.
- `IWxInteractable`은 액터가 구현하고 컴포넌트에 위임하되, 감지·사거리는 쿼리 콜리전 형상 위에서 돈다. `CanInteract`는 구현체가 내부 상태에서 파생 (클라 표시 게이트와 서버 검증이 같은 답).
- `ECC_WxAttack`은 `DefaultEngine.ini`의 WxAttack Object Channel 값과 일치해야 하며, 어긋나면 모든 히트 판정이 다른 채널을 가리킨다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트의 시스템 지도. doc-comment로 각 태그의 발행자·소비자·의도가 정리되어 있어 전투/UI/장치 흐름의 진입점.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 간 계약이 어떻게 결합을 끊는지 보여주는 대표 예.

## 관련
- 하위 의존처: 모든 Wx 도메인 플러그인 ([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]])이 WxCore를 참조한다.

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 13파일 — `/readme-writer`로 갱신*
