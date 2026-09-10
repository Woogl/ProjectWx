# WxCore — 공용 기반

> 모든 Wx 플러그인이 공유하는 최하위 기반 모듈. Native Gameplay Tag, 콜리전 채널, 상호작용·UI 데이터 인터페이스 같은 도메인 간 공용 정의를 한곳에 모아, 도메인 플러그인끼리 서로를 참조하지 않고도 같은 계약 위에서 맞물리게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag를 단일 지점(`WxGameplayTags`)에서 선언·정의
- 도메인 플러그인이 공유하는 얇은 인터페이스 계약: 상호작용(`IWxInteractable`), UI 표시 데이터(`IWxUIData`)
- 코드에서 참조하는 콜리전 채널 상수(`ECC_WxAttack`)
- 에디터 저작용 로케이터 표시 헬퍼(`FWxLocatorUtils`)

**경계 (비담당)**
- 태그를 실제로 소비·발행하는 로직은 각 도메인에 위임 — 전투 파이프라인은 [[WxCombat]], 상호작용 스캔·어빌리티는 [[WxWorld]], HUD/레이어는 [[WxUI]]
- 인터페이스의 구현체는 담지 않는다. 계약만 여기 두고 구현은 소비 도메인([[WxInventory]] 픽업, [[WxDialogue]] 등)이 자기 액터에 붙인다
- 게임플레이 시스템·액터·컴포넌트를 두지 않는다 (foundation 전용)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전 프로젝트 Native Tag의 유일한 선언처. State/Effect/Event/Ability/Device/UI 등으로 묶임 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상의 공용 계약. 액터가 구현(컴포넌트 아님), 조회는 Cast 한 번 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(아이콘·이름·설명·충전 수)의 공용 계약 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수. `DefaultEngine.ini` 의 `WxAttack` 등록 항목이 쓰는 `Channel` 값과 일치해야 함 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 에디터 전용. `FUniversalObjectLocator`의 표시명 생성 | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 태그 추가는 반드시 이 두 파일에만 작성한다(헤더 상단 규약)
- 주요 네임스페이스:
  - `State.*` — ASC에 붙는 상태(락온·교전·대화·소환 활성)
  - `Effect.*` — GE가 부여하는 태그(무적·가드·슈퍼아머·히트스톱 등), 애셋 태그 겸용
  - `Event.*` — 대미지 파이프라인 등이 서버에서 ASC로 보내는 Gameplay Event(피격·처형·아이템 사용)
  - `Ability.*` — 어빌리티 식별 태그(`Ability.X` = 그 어빌리티 활성 중). `Cooldown.*`가 이름을 따라감
  - `Damage.*` / `HitReact.*` — 대미지 성질과 피격 반응 종류
  - `Device.*` — 장치 State Tree 상태값(코드가 직접 읽지 않아도 여기서 정의)
  - `GameplayCue.*` / `UI.*` / `SetByCaller.*` / `Movement.*` — 큐, UI 레이어·액션, SetByCaller 매그니튜드, 이동 상태

## 확장 포인트 / 규약
- 새 전역 태그: `WxGameplayTags.h`에 `UE_DECLARE_...`, `.cpp`에 `UE_DEFINE_...`. 다른 곳에 흩뿌리지 않는다
- 새 상호작용 대상: 액터에 `IWxInteractable` 구현. `CanInteract`는 구현체가 자기 상태에서 파생(외부 on/off 진입점 없음)해 클라 표시 게이트와 서버 발동 검증이 같은 답을 받게 한다. 감지·사거리는 쿼리 콜리전 형상 위에서 도므로 프리미티브가 있어야 스캔에 걸린다
- 새 UI 표시원: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트 등)이 `IWxUIData` 구현, 대개 DataTable 행을 그대로 흘려보낸다
- `ECC_WxAttack`은 `ECC_GameTraceChannel1`에 고정 — `DefaultEngine.ini` 의 `WxAttack` 등록 항목이 쓰는 `Channel` 값과 동기화 필수 (줄 순서는 무관)

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 이 모듈의 중심. 태그 doc-comment가 각 도메인의 제어 흐름(대미지 파이프라인·가드·처형·장치 트리)을 요약해 준다
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 간 의존을 끊는 계약 패턴의 대표 예. 헤더 주석이 왜 액터 단위인지 설명
3. `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` — WxUI가 도메인에 의존하지 않는 이유

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]])과 게임 모듈 WxGame이 이 모듈에 의존한다. WxCore는 어떤 Wx 플러그인도 참조하지 않는 최하위 기반이다

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 11파일 — `/readme-writer`로 갱신*
