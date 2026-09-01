# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 참조하는 최하단 공용 정의 모듈. Native Gameplay Tag, 도메인 간 계약 인터페이스, 콜리전 채널 상수를 한곳에 모아, 소비 도메인이 서로를 직접 참조하지 않고도 같은 어휘로 맞물리게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언·정의 (`WxGameplayTags`)
- 도메인 간 계약 인터페이스 — 상호작용(`IWxInteractable`), UI 표시 데이터(`IWxUIData`)
- 콜리전 채널 상수(`ECC_WxAttack`)
- 저작 도구용 로케이터 표시 헬퍼(`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 계약의 실제 구현·소비는 각 도메인 몫 — 전투 [[WxCombat]], 월드/장치 [[WxWorld]], UI [[WxUI]], 인벤토리 [[WxInventory]] 등
- 태그가 참조하는 GE·어빌리티·GameplayCue 애셋과 그 로직은 해당 도메인에 있다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전역 Native Tag 선언. 태그 추가는 이 헤더와 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약(액터 구현). `CanInteract`/`OnInteracted`/`GetInteractionPrompt` | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터 계약. `GetTitle`/`GetDescription`/`GetIcon` | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수 (`ECC_GameTraceChannel1`) | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명 헬퍼(에디터 전용, `WITH_EDITOR`) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
C++ Native Tag로 전량 선언(`UE_DECLARE_GAMEPLAY_TAG_EXTERN`). 실제 정의는 짝 cpp.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`
- 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — ASC loose 태그로 발행하는 상태 (예: `State.Dialogue`)
  - `Effect.*` — GE가 부여하는 상태 (Invincible/Guard/PerfectGuard/Exhausted/SuperArmor)
  - `Movement.*` — 이동 상태 (InAir/Sprint)
  - `HitReact.*` — 피격 반응 종류, `Event.Hit` 페이로드로 전달
  - `Event.*` — GameplayEvent 트리거 (Hit·DamageDealt·Finisher·Death·UseItem·Device.Triggered 등)
  - `Device.*` — 장치 State Tree 상태값 (Button/Door/Elevator/TreasureChest/CheckPoint/Piston)
  - `GameplayCue.*` — 큐 (Hit·DamageFloater·AttackTelegraph 등)
  - `Damage.*` — 대미지 판정 플래그·결과 (Critical/CanGuard/CanParry/GuardBreak 등)
  - `Ability.*` — 어빌리티 식별 태그(하나씩, AssetTag=ActivationOwnedTag). 플레이어/적/공용
  - `Cooldown.*` — 어빌리티별 쿨다운 GE 태그
  - `SetByCaller.*` — GE SetByCaller 키 (Magnitude/Duration/Coeff.ATK/MoveSpeedScale)
  - `UI.*` — CommonUI 레이어(`UI.Layer.*`)·액션(`UI.Action.*`) 태그

## 확장 포인트 / 규약
- 태그 추가는 `WxGameplayTags.h`와 `WxGameplayTags.cpp` 두 파일에만 손댄다 — 다른 곳에 흩뿌리지 않는다.
- 도메인이 서로를 참조하지 않고 맞물려야 할 때, 그 접점(태그·인터페이스·상수)을 이 모듈에 올린다. 예: 상호작용 계약이 WxCore에 있어 WxInventory 픽업이 WxWorld에 의존하지 않고도 상호작용 대상이 된다.
- `IWxInteractable`은 액터가 구현한다(컴포넌트 아님). 대상 하나당 구현체 하나, 조회는 Cast 한 번.
- `ECC_WxAttack`은 `DefaultEngine.ini`의 채널 등록 순서와 일치해야 한다.
- 계약 인터페이스는 `NotBlueprintable` + `CannotImplementInterfaceInBlueprint` — C++ 구현만 허용.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 이 모듈의 8할. 각 태그 주석에 어느 시스템이 언제 발행/소비하는지가 적혀 있어 전투·장치·UI 흐름의 색인이 된다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 계약을 WxCore에 두는 이유(도메인 역참조 회피)가 헤더에 설명돼 있다.
3. `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` — 피격 판정이 메시에서만 일어나게 하는 채널 규약.

## 관련
- 상위: `WxCombat`, `WxWorld`, `WxUI`, `WxInventory`, `WxAI`, `WxDialogue`, `WxQuest`, `WxGame` — 모든 도메인/게임 모듈이 이 공용 정의를 소비한다. (WxCore는 어떤 Wx 플러그인도 참조하지 않는 DAG 최하단)

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 10파일 — `/readme-writer`로 갱신*
