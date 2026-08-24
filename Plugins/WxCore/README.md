# WxCore — 공용 정의 파운데이션

> 모든 Wx 도메인 플러그인이 공유하는 최하위 정의 계층. Native Gameplay Tag, 콜리전 채널, 그리고 도메인 간 결합을 끊는 크로스커팅 인터페이스(상호작용·세이브)를 한곳에 모은다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag의 단일 선언처 (`WxGameplayTags`)
- 코드에서 참조하는 커스텀 콜리전 채널 상수 (`ECC_WxAttack`)
- 상호작용 대상 계약 (`IWxInteractable`) — 감지/사거리/실행이 도메인에 의존하지 않도록 계약만 정의
- 세이브 참여 계약 (`IWxSavable`) — `WxSave`와 소비 도메인의 직접 의존을 끊는 마커 + 후크

**경계 (비담당)**
- 태그를 소비하는 실제 로직(어빌리티·GE·GameplayCue 구현)은 각 도메인 소유. WxCore는 태그 "이름"만 든다
- 상호작용 스캔/프롬프트/실행 어빌리티는 [[WxWorld]]·[[WxCombat]]·[[WxUI]] 등이 구현, 계약만 여기 둠
- 슬롯 직렬화·저장/로드 파이프라인은 [[WxSave]]. WxCore는 인터페이스 형태만 제공
- 콜리전 채널의 실제 이름/응답 매핑은 `DefaultEngine.ini`. 여기엔 코드용 별칭 상수만 존재

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전 도메인 Native Tag 선언 네임스페이스. 태그 추가는 이 헤더 + 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 액터만 구현(컴포넌트 불가) | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 라이프사이클 참여 계약. `GetSaveId()`/`OnSaveRestored()` | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel(`ECC_GameTraceChannel1` 별칭) | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxCoreModule` | 모듈 엔트리. 태그 등록 등 부트스트랩 | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
`WxGameplayTags.h`에서 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`로 선언하고 `WxGameplayTags.cpp`에서 정의한다. 태그 추가·수정은 반드시 이 두 파일에만 한다. 네임스페이스는 카테고리 루트별로 묶여 있으며, 각 루트가 어느 도메인에서 소비되는지가 파일 횡단 흐름의 핵심이다.

- `State_*` — ASC에 실린 상태(LockedOn/InCombat/BeingFinished/ComboWindow/Dialogue). 네임플레이트 표시·어빌리티 차단 조건
- `Effect_*` — GE가 부여하는 상태(Invincible/Guard/PerfectGuard/Exhausted/SuperArmor)
- `Movement_*` — 이동 상태(InAir/Sprint). SP 소모·회복 억제 조건
- `Event_*` — 어빌리티 트리거용 GameplayEvent(HitReact 계열, Finisher/Backstab, Device_Triggered, Death/Groggy/Ragdoll 등)
- `Device_*` — 장치 StateTree 상태값이자 세이브 슬롯 저장값. 코드가 읽지 않아도 태그는 여기서 정의
- `GameplayCue_*` — 연출 큐(Hit/PerfectGuard/DamageFloater/AttackTelegraph 계열 등)
- `Damage_*` — 대미지 성질 마커(Critical/Unblockable/ParryHitReact 등)
- `Ability_*` — 어빌리티 식별 태그. `Ability.X` = "그 어빌리티 활성 중" 규약(플레이어·적·공용·Pattern으로 구분)
- `SetByCaller_*` — GE SetByCaller 키(Duration/Recovery/DP/Coeff_ATK/RawDamage/MoveSpeedScale)
- `UI_Layer_*`, `UI_Action_*` — CommonUI 레이어 스택과 액션 라우팅 키

## 확장 포인트 / 규약
- **새 태그**: `WxGameplayTags.h`에 `WXCORE_API UE_DECLARE_...`, `WxGameplayTags.cpp`에 짝 `UE_DEFINE...`. 다른 곳에서 태그를 별도 선언하지 않는다
- **상호작용 대상 만들기**: 대상 액터가 `IWxInteractable` 구현. `OnInteracted`/`GetInteractionPrompt`는 순수 가상(필수), `CanInteract`/`SetInteractionEnabled`는 기본 구현 존재. 계약은 항상 액터가 들고 필요하면 내부 컴포넌트로 위임 — 대상 1개당 구현체 1개, 조회는 Cast 1회. 감지·사거리는 쿼리 콜리전 프리미티브 위에서 돈다
- **세이브 대상 만들기**: 액터가 `IWxSavable` 구현. `GetSaveId()`가 안정적 `FGuid` 반환(에디터 1회 부여·영속 UPROPERTY), 무효 반환 시 저장 제외. `UPROPERTY(SaveGame)` 필드가 슬롯에 기록되고 로드 후 `OnSaveRestored()` 호출(BeginPlay 이전일 수 있음)
- **콜리전**: `ECC_WxAttack` 상수는 `DefaultEngine.ini`의 채널 등록 순서와 반드시 일치. 순서 변경 시 상수도 갱신

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 모듈 본체. 어떤 태그가 있고 각 카테고리를 누가 소비하는지 doc-comment로 매핑돼 있음
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 상호작용 계약과 "액터가 든다" 설계 근거
3. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 세이브 계약과 도메인 결합 차단 의도

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxWorld]], [[WxSave]], [[WxUI]], [[WxInventory]], [[WxAI]], [[WxDialogue]], [[WxQuest]] 등)이 WxCore를 의존한다. WxCore는 다른 Wx 플러그인을 참조하지 않는 DAG 최하단(foundation)

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 9파일 — `/readme-writer`로 갱신*
