# WxCore — 공용 정의 foundation

> 모든 Wx 플러그인이 공유하는 최하단(foundation) 정의 모듈. Native Gameplay Tag 카탈로그, 콜리전 채널 상수, 도메인 간 결합을 끊는 계약 인터페이스(IWxInteractable·IWxSavable)를 한곳에서 소유한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언·정의 (State/Effect/Movement/Event/Device/GameplayCue/Damage/Ability/SetByCaller/UI)
- 커스텀 콜리전 채널 별칭 상수 (`ECC_WxAttack`)
- 도메인 경계를 잇는 순수 계약 인터페이스 (상호작용·세이브)

**경계 (비담당)**
- 인터페이스의 실제 구현·소비 — 각 도메인이 담당 ([[WxWorld]], [[WxInventory]], [[WxSave]] 등)
- 태그를 실제로 부여/소비하는 어빌리티·이펙트 로직 — [[WxCombat]], [[WxUI]] 등
- 콜리전 채널의 프로파일 응답 등록 — `DefaultEngine.ini`

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `namespace WxGameplayTags` | 모든 Native Tag의 단일 선언처. 태그 추가는 이 헤더와 짝 cpp에만 | `Source/WxCore/Public/WxGameplayTags.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 별칭(`ECC_GameTraceChannel1`). ini 등록 순서와 일치 필수 | `Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxInteractable` | 상호작용 대상 계약. 액터만 구현, 조회는 Cast 한 번 | `Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | WxSave 슬롯 라이프사이클 참여 마커+후크. `GetSaveId()` 키로 직렬화 | `Source/WxCore/Public/WxSavable.h` |
| `FWxCoreModule` | 모듈 부트스트랩(현재 기본 구현) | `Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
- 선언: `Source/WxCore/Public/WxGameplayTags.h` / 정의: `Source/WxCore/Private/WxGameplayTags.cpp` (두 파일만 수정)
- 주요 네임스페이스(루트 → 의미):
  - `State.*` — 액터 순간 상태(LockedOn·InCombat·BeingFinished·Dialogue), 네임플레이트/어포던스 조건
  - `Effect.*` — GE가 부여하는 상태 태그(Invincible·Guard·PerfectGuard·Exhausted·SuperArmor), 애셋 태그 겸용
  - `Movement.*` — 이동 상태(InAir·Sprint), 콤보/SP 조건
  - `Event.*` — GameplayEvent 트리거(HitReact 계열·Finisher·Backstab·Death·Groggy·Device.Triggered 등)
  - `Device.*` — 장치 StateTree 상태값. 코드가 읽지 않고 세이브 슬롯에 저장되는 값
  - `GameplayCue.*` — 큐 태그(Hit·DamageFloater·AttackTelegraph 색상별 등)
  - `Damage.*` — 대미지 성질 마커(Critical·CanCritical·Unblockable·ParryHitReact)
  - `Ability.*` — 어빌리티 식별 태그. 하나당 정확히 하나, 활성 여부와 동치(성질 분류는 별도 `Trait.*`)
  - `SetByCaller.*` — GE SetByCaller 키(Duration·Recovery·DP·Coeff.ATK·RawDamage·MoveSpeedScale)
  - `UI.*` — CommonUI 레이어(`Layer.*`) 및 액션(`Action.*`)

## 확장 포인트 / 규약
- **인터페이스는 액터만 구현한다** — 능력이 컴포넌트에 있어도 계약은 호스트 액터가 들고 위임. 대상 하나당 구현체 하나(Cast 한 번). `UINTERFACE(NotBlueprintable, CannotImplementInterfaceInBlueprint)`로 C++ 전용.
- **계약을 WxCore에 두는 이유**: 소비 도메인이 구현 도메인(WxWorld·WxSave)에 직접 의존하지 않도록 결합을 끊는다. 새 상호작용/세이브 대상은 이 인터페이스만 구현하면 된다.
- **태그·채널 추가 규약**: 태그는 선언 헤더+짝 cpp 두 곳만. `ECC_WxAttack`는 `DefaultEngine.ini` 채널 등록 순서와 반드시 일치.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 전 시스템이 참조하는 태그 카탈로그. 도메인 간 프로토콜의 사전
2. `Source/WxCore/Public/WxInteractable.h` — 계약을 WxCore에 두는 아키텍처 의도가 헤더 주석에 응축
3. `Source/WxCore/Public/WxSavable.h` — 세이브 참여 규약과 `GetSaveId()` 불변 키의 근거

## 관련
- 상위: 모든 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]], [[WxSave]])과 [[WxGame]]이 WxCore를 참조. WxCore는 어떤 Wx도 참조하지 않는 DAG 최하단

---
*문서 기준 커밋 `e1999dc` · 생성일 2026-08-24 · 소스 9파일 — `/readme-writer`로 갱신*
