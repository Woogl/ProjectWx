# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 공유하는 계약을 한곳에 모은 foundation 도메인. Gameplay Tag, 상호작용·세이브 인터페이스, 콜리전 채널 상수를 선언해 도메인 플러그인들이 서로 직접 의존하지 않고도 협업하게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언 (`WxGameplayTags`) — State/Effect/Movement/Event/Device/GameplayCue/Damage/Ability/Trait/SetByCaller/UI 네임스페이스.
- 도메인 간 공용 인터페이스 계약: `IWxInteractable`(상호작용 대상), `IWxSavable`(세이브 참여).
- 공용 상수: `ECC_WxAttack` 콜리전 채널.

**경계 (비담당)**
- 태그를 부여/소비하는 로직 — 각 도메인([[WxCombat]], [[WxWorld]], [[WxUI]] 등)이 자기 태그를 발행·판정한다. WxCore는 선언만 한다.
- 상호작용의 스캔·사거리 검증·어빌리티 흐름은 [[WxWorld]]/[[WxCombat]], 세이브 슬롯 라이프사이클은 [[WxSave]]가 구현한다. WxCore는 계약(인터페이스)만 제공.

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 선언). 그 외 Core/CoreUObject/Engine 빌드 기본만.
- 규칙: WxCore는 다른 Wx 플러그인 참조 금지 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전역 Native Tag 선언부. 태그 추가는 이 헤더와 짝 cpp에만 | `Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 소비 도메인이 WxWorld 의존 없이 자기 액터를 상호작용 대상으로 | `Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 슬롯 라이프사이클 참여 마커 + 후크. WxSave와 소비 도메인을 분리 | `Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수 (`ECC_GameTraceChannel1`) | `Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxCoreModule` | 모듈 진입점 (태그 등록 등) | `Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
- 선언: `Source/WxCore/Public/WxGameplayTags.h` (짝: `Source/WxCore/Private/WxGameplayTags.cpp`)
- 주요 네임스페이스:
  - `State.*` — 락온/전투/처형/콤보/대화 등 액터 상태 마커
  - `Effect.*` — GE가 부여하는 상태(무적/가드/탈진/슈퍼아머 등), 애셋 태그 겸용
  - `Movement.*` — InAir/Sprint 이동 상태
  - `Event.*` — HitReact 계열, 처형·백스탭·사망·그로기 등 게임플레이 이벤트. `Event.Device.Triggered`는 발동 장치가 연결 장치 트리에 보내는 기본 "눌렸다" 이벤트(`AWxDevice::TriggerEvent` 기본값); 상태가 아닌 동작 요청이 생기면 `Event.Device.<장치>.<동사>`로 그때 만든다
  - `Ability.*` — 어빌리티 식별 태그(정확히 하나, `Ability.X` 활성 = 그 어빌리티 활성). `Trait.*`는 성질 분류 마커(`Trait.Exclusive` 액션 슬롯 점유)
  - `SetByCaller.*` — GE 계산용 키(Duration/Recovery/Coeff/RawDamage/MoveSpeedScale)
  - `GameplayCue.*` — 히트·가드·텔레그래프 등 연출 큐
  - `Damage.*` — Critical/Unblockable/ParryHitReact 등 대미지 특성
  - `Device.*` — 문/엘리베이터/보물상자/체크포인트/버튼 StateTree 상태값(세이브 대상), 코드 비참조. 발동 장치가 이 태그를 `TriggerEvent`로 보내면 뜻은 언제나 "그 상태로 가 달라"는 요청 하나뿐이다(엘리베이터 버튼 = `Device.Elevator.1F`/`2F`, 받는 ST는 `On Event(그 태그) → 그 상태`); 그 외 용도로 상태 태그를 이벤트에 쓰지 않는다
  - `UI.*` — CommonUI 레이어(`UI.Layer.*`) 및 액션(`UI.Action.*`)

## 확장 포인트 / 규약
- **태그 추가**는 반드시 `WxGameplayTags.h`(선언)와 `WxGameplayTags.cpp`(정의) 두 파일에만. 소비 모듈은 `WXCORE_API`로 노출된 extern 태그를 참조한다.
- **인터페이스 정합의 이유**: `IWxInteractable`/`IWxSavable`는 둘 다 `NotBlueprintable`(BP 구현 불가)이지만 구현 위치가 갈린다.
  - `IWxInteractable`은 **액터 전용**이다 — 능력이 컴포넌트에 담겨도(대화·장치) 계약은 호스트 액터(`AWxDialogueActor`·`AWxDevice`)가 들고 그 컴포넌트로 넘긴다. 대상 하나당 구현체도 하나라 조회는 `Cast<IWxInteractable>` 한 번이다.
  - `IWxSavable`도 **액터 전용**이다. 영속 상태를 컴포넌트가 만들어도 계약은 호스트 액터가 든다(`AWxDevice`가 자기 StateTree의 상태 Tag를 자기 필드로 소유). 직렬화 대상은 어느 쪽이든 액터와 그 컴포넌트 전체다.
- **콜리전**: `ECC_WxAttack` 상수는 `DefaultEngine.ini`의 채널 등록 순서와 일치해야 하며, 히트박스는 이 Object Type을, 투사체는 "WxProjectile" 프리셋을 쓴다.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전 시스템의 태그 어휘가 주석과 함께 한눈에. 다른 모듈을 이해하는 색인.
2. `Source/WxCore/Public/WxInteractable.h` — 상호작용 계약과 클라 스캔/서버 검증이 같은 답에 수렴하는 규약.
3. `Source/WxCore/Public/WxSavable.h` — 세이브 참여 방식(`GetSaveId`/`OnSaveRestored`)과 WxSave 분리 원리.

## 관련
- 상위: 거의 모든 도메인·GameFeature 플러그인이 소비 — [[WxCombat]], [[WxWorld]], [[WxInventory]], [[WxUI]], [[WxAI]], [[WxDialogue]], [[WxQuest]], [[WxSave]].

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 9파일 — `/readme-writer`로 갱신*
