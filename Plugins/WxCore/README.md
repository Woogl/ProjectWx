# WxCore — 공용 정의 파운데이션

> 모든 Wx 도메인 플러그인이 공유하는 최하단(foundation) 정의를 담는다. 프로젝트 전역 Gameplay Tag, 커스텀 콜리전 채널, 그리고 도메인 간 직접 의존을 끊어 주는 경량 인터페이스(계약)를 제공한다.

## 책임

**담당**
- 프로젝트 전역 Native Gameplay Tag 단일 선언소 (State/Movement/Event/Gimmick/GameplayCue/Damage/Ability/SetByCaller/UI).
- 커스텀 콜리전 채널 상수(`ECC_WxAttack`) — `DefaultEngine.ini` 등록 순서와 짝을 이룸.
- 도메인 간 결합을 끊는 계약 인터페이스: `IWxInteractable`(상호작용), `IWxSavable`(세이브 참여).
- 계약 인터페이스의 구현체 조회 헬퍼(`IWxInteractable::Find`, `IsMeshInRange`)와 무동작 기본 구현.

**경계 (비담당)**
- 상호작용의 실제 스캔·입력·어빌리티 흐름 — 계약만 정의하고 실행은 [[WxWorld]]·[[WxCombat]] 등 소비 도메인에 위임.
- 세이브 슬롯의 직렬화·저장/로드 라이프사이클 — 마커 인터페이스만 두고 실행은 [[WxSave]]가 담당.
- Tag를 발행/소비하는 로직(어빌리티·AttributeSet·ANS 등)은 각 도메인 소유. 여기서는 이름만 만든다.

## 의존성
- **주요 의존**: 없음(Wx 모듈). 엔진: `GameplayTags`(Native Tag 선언).
- 규칙: WxCore는 DAG 최하단 foundation이며 다른 Wx 플러그인을 참조하지 않는다 — 위반 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (네임스페이스) | 전역 Native Tag 선언 — 다른 모듈은 여기서만 참조 | `Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 소비 도메인이 [[WxWorld]] 의존 없이 자기 액터를 상호작용 대상으로 만들게 함 | `Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 참여 마커+후크. [[WxSave]]와 소비 도메인의 상호 직접 의존을 끊음 | `Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수 | `Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxCoreModule` | 모듈 진입점(StartupModule/ShutdownModule) | `Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
- 선언: `Source/WxCore/Public/WxGameplayTags.h` / 정의: `Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — 어빌리티 활성과 어긋날 수 있는 조건 상태(Guard/Invincible/InCombat 등)
  - `Movement.*` — 이동 상태(InAir/Sprint)
  - `Event.*` — GAS 이벤트 트리거(HitReact 계열, Interact/Finisher/Death/Groggy 등)
  - `Gimmick.*` — StateTree 상태 라벨이자 세이브에 담기는 기믹 상태(코드가 직접 읽지 않음)
  - `GameplayCue.*` — 큐 태그(Damage/Hit/PerfectGuard/AttackTelegraph 등)
  - `Damage.*` — 대미지 판정 플래그(Critical/Unblockable 등)
  - `Ability.*` — 어빌리티 식별·차단 태그(`Ability.Exclusive`가 차단·캔슬의 유일 지목 대상)
  - `SetByCaller.*` — GE SetByCaller 키
  - `UI.*` — CommonUI 레이어·액션 태그

## 확장 포인트 / 규약
- **새 Tag 추가**: `WxGameplayTags.h`에 `WXCORE_API UE_DECLARE_...`, `.cpp`에 `UE_DEFINE_...`를 짝으로 추가한다. 점(.)은 언더스코어(_)로 치환한 변수명을 쓴다. 어빌리티 태그 규칙(식별 태그 1개를 AssetTags·ActivationOwnedTags 양쪽에, 분류 마커 `Ability.Exclusive`는 AssetTags에만)은 헤더 상단 주석이 규정.
- **상호작용 대상 추가**: 액터가 직접 `IWxInteractable`를 구현하거나(픽업·적), 컴포넌트가 구현한다(기믹·대화 — 호스트 액터를 순수 BP로 둘 때). BP에서는 구현 불가(`CannotImplementInterfaceInBlueprint`). 소비처는 전부 `IWxInteractable::Find`를 거치므로 구현체 위치가 바뀌어도 조회는 한 곳.
- **세이브 참여**: `IWxSavable`을 구현하고 `GetSaveId()`로 안정 GUID를 반환, 보존 필드에 `UPROPERTY(SaveGame)`을 표시한다. 복원 후처리는 `OnSaveRestored()`.
- **콜리전 채널 추가**: 상수는 여기에, 실제 채널 번호(`ECC_GameTraceChannelN`)와 등록은 `DefaultEngine.ini`와 반드시 순서 일치.

## 여기서부터 읽어라
1. `Source/WxCore/Public/WxGameplayTags.h` — 전투/상호작용/UI 전반의 어휘. 다른 모듈을 읽기 전 태그 의미를 먼저 잡는다.
2. `Source/WxCore/Public/WxInteractable.h` — 상호작용 계약과 클라 스캐너/서버 어빌리티가 수렴하는 방식(주석에 상세).
3. `Source/WxCore/Private/WxInteractable.cpp` — `Find`의 액터/컴포넌트 갈래와 `IsMeshInRange`의 쿼리 콜리전 전제.

## 관련
- 상위(소비처): 사실상 모든 Wx 도메인 — 상호작용은 [[WxWorld]]·[[WxCombat]]·[[WxInventory]]·[[WxDialogue]], 세이브는 [[WxSave]], Tag는 [[WxCombat]]·[[WxAI]]·[[WxUI]] 등이 참조.

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 9파일 — `/readme-writer`로 갱신*
