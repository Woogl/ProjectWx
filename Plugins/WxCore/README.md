# WxCore — 공용 정의 파운데이션

> 모든 Wx 플러그인이 공유하는 정의(GameplayTags, 콜리전 채널, 공용 인터페이스)의 단일 출처. 도메인 간 결합을 끊어 주는 최하위 계층으로, 다른 Wx 플러그인을 참조하지 않는다.

## 책임
**담당**
- 프로젝트 전역 Native GameplayTag의 선언·정의 (State/Effect/Event/Ability/Damage/Device/UI 등 모든 네임스페이스)
- 도메인이 서로를 알지 않고도 계약을 나누게 하는 공용 인터페이스: `IWxInteractable`(상호작용), `IWxUIData`(UI 표시 데이터), `IWxMinion`(소환물)
- 공용 상수: 커스텀 콜리전 채널 `ECC_WxAttack`
- 에디터 저작 헬퍼 `FWxLocatorUtils` (로케이터 표시명, 에디터 전용)

**경계 (비담당)**
- 태그를 실제로 발행·소비하는 시스템 로직 — 전투 파이프라인 [[WxCombat]], 상호작용/장치 [[WxWorld]], UI 레이어 [[WxUI]], 소환물 서브시스템 [[WxAI]]
- 인터페이스 구현체는 각 소비 도메인이 자기 액터/컴포넌트에 둔다 (WxCore는 계약만 제공)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전역 Native Tag 선언부. 태그 추가는 이 파일과 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약(액터가 구현). 소비 도메인이 WxWorld 없이 자기 액터를 상호작용 대상으로 만듦 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(아이콘·이름·설명·충전 칸) 계약 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `IWxMinion` | 소환 가능 액터 계약(상한·어그로 무시). GenericTeamAgentInterface도 함께 구현 요구 | `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스의 Object Channel 상수. DefaultEngine.ini 등록값과 일치해야 함 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명 헬퍼(에디터 전용, WITH_EDITOR) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 태그 추가·변경은 이 두 파일에만 (헤더 `UE_DECLARE_...` + cpp `UE_DEFINE_...` 짝).
- 주요 네임스페이스:
  - `Event.*` — 시스템 간 GameplayEvent 계약 (`Event.Hit`, `Event.DamageDealt`, `Event.Finisher`, `Event.UseItem` 등 대미지·처형·아이템 파이프라인의 이벤트 축)
  - `Ability.*` — 어빌리티 식별 태그. "Ability.X = 그 어빌리티가 활성 중"이 성립하도록 AssetTags/ActivationOwnedTags 양쪽에 넣는 규약
  - `Cooldown.*` — 어빌리티별 쿨다운 GE가 부여, 이름은 `Ability.X`를 따름
  - `State.*` / `Effect.*` — ASC에 붙는 상태·GE 부여 태그 (락온·교전·래그돌 / 무적·가드·경직)
  - `Damage.*` — 공격/판정 결과 표식 (`Damage.Attack`, `Damage.Critical`, `Damage.CanParry` 등)
  - `Device.*` — 장치의 State Tree 상태값 (코드가 아니라 STree가 읽고 씀)
  - `HitReact.*` / `GameplayCue.*` / `UI.*` / `SetByCaller.*` — 피격 반응, 큐, UI 레이어·액션, GE SetByCaller 키

## 확장 포인트 / 규약
- 새 태그: `WxGameplayTags.h`에 `WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(...)` 추가 후 `WxGameplayTags.cpp`에 `UE_DEFINE_GAMEPLAY_TAG(..., "...")` 짝을 맞춘다.
- 상호작용 대상: 소비 도메인의 **액터**가 `IWxInteractable` 구현(컴포넌트는 구현하지 않음). 액터에 쿼리 콜리전 프리미티브가 있어야 스캔·사거리에 걸린다.
- UI 표시 데이터: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트 등)이 `IWxUIData` 구현.
- 소환물: `IWxMinion` + `GenericTeamAgentInterface`를 함께 구현(주인 팀 상속).
- 콜리전: `ECC_WxAttack`은 `DefaultEngine.ini`의 WxAttack 등록 항목 값과 반드시 일치. 투사체는 "WxProjectile" 프리셋 사용.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 모듈의 심장. doc-comment가 각 태그를 누가 발행·소비하는지 짚어 주므로 전투/AI/UI 시스템의 데이터 흐름 지도로 읽힌다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` / `WxUIData.h` — 도메인 결합을 끊는 계약 패턴(계약은 WxCore, 구현은 소비 도메인)의 실례.
3. `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` — 의존이 엔진 모듈뿐임을 확인(파운데이션 규칙 검증).

## 관련
- 상위: 모든 Wx 도메인 플러그인 — [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]] 이 WxCore의 태그·인터페이스를 공유한다.

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 13파일 — `/readme-writer`로 갱신*
