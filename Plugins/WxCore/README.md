# WxCore — 공용 정의 (foundation)

> 모든 Wx 도메인 플러그인이 공유하는 최하단 정의 계층. 프로젝트 전역 Gameplay Tag, 커스텀 콜리전 채널, 그리고 도메인 간 결합을 끊는 공용 인터페이스(상호작용·세이브)를 한곳에 모은다.

## 책임
**담당**
- 프로젝트 전역 **Native Gameplay Tag** 단일 선언처 (`WxGameplayTags`)
- 커스텀 **콜리전 채널** 상수 정의 (`ECC_WxAttack`)
- 도메인 간 의존을 끊는 **공용 인터페이스**: 상호작용 계약(`IWxInteractable`), 세이브 참여 마커(`IWxSavable`)

**경계 (비담당)**
- 위 정의를 **소비·구현**하는 실제 시스템(전투·상호작용·세이브 로직)은 각 도메인 플러그인 소관. WxCore는 계약과 상수만 제공한다.
- 태그의 실제 dispatch/부여, 저장 슬롯 직렬화, 상호작용 스캔 등 동작은 여기에 없다.

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 선언), `Engine`/`CoreUObject` (인터페이스·콜리전). 특별한 서브시스템 의존 없음.
- 규칙: foundation으로서 다른 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 프로젝트 전역 Native Tag 네임스페이스. 태그는 이 파일에서만 추가 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상의 공용 계약. 소비 도메인이 WxWorld 의존 없이 자기 액터를 상호작용 대상으로 만든다 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 슬롯 라이프사이클 참여 마커 + 후크. WxSave와 소비 도메인의 직접 의존을 끊는다 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스용 Object Channel 상수. `DefaultEngine.ini` 등록 순서와 일치 필수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |

## Gameplay Tags
C++ Native Tag가 이 모듈의 본체다. 태그 추가는 `WxGameplayTags.h` + `WxGameplayTags.cpp`에만 한다.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`
- 주요 네임스페이스:
  - `State.*` — 캐릭터 상태(Dead, Ragdoll, Groggy, LockOn, InCombat, Guard, Dodge 등)
  - `Event.*` — GAS 이벤트(HitReact 계열, Finisher/Backstab, DodgeSuccess, Interact 등)
  - `Ability.*` — 어빌리티 식별 태그. 차단·캔슬은 `Ability.Exclusive`만 지목
  - `SetByCaller.*` — GameplayEffect 값 주입 키(Duration, Coeff.ATK, RawDamage 등)
  - `GameplayCue.*` — 큐(Damage, PerfectGuard, AttackTelegraph 색상별 등)
  - `Damage.*` — 대미지 판정 플래그(Critical, Unblockable, ParryHitReact 등)
  - `UI.Layer.*` / `UI.Action.*` — CommonUI 레이어·액션
  - `Gimmick.*` — 월드 기믹의 StateTree 상태 라벨(세이브에 담기는 상태값)
  - `StateTree.Interact`, `Quest.Fail`, `ANS.ComboWindow`

## 확장 포인트 / 규약
- **새 태그**: `WxGameplayTags.h`/`.cpp` 두 파일에만 추가. 코드가 읽지 않는 기믹 상태 이름도 여기서만 만든다.
- **상호작용 대상 만들기**: 액터가 직접 `IWxInteractable` 구현(액터 고유 동작) 또는 컴포넌트가 구현(호스트를 순수 BP로). `IWxInteractable::Find`가 두 갈래를 흡수하므로 소비처는 조회 하나만 거친다. BP 구현 불가(`CannotImplementInterfaceInBlueprint`).
- **세이브 대상 만들기**: `IWxSavable` 구현 + 보존 필드에 `UPROPERTY(SaveGame)`. `GetSaveId()`가 유효하지 않으면 저장/복원 제외.
- **콜리전**: `ECC_WxAttack`은 `DefaultEngine.ini` 채널 등록 순서와 반드시 일치.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 태그가 곧 시스템 간 계약. 전투·UI·상호작용 흐름의 어휘가 전부 여기 주석에 있다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 상호작용의 공용 계약. 표식/활성/사거리/자격 판정의 책임 분배를 설명한다.
3. `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp` — `Find`(액터/컴포넌트 갈래)와 `IsMeshInRange`(쿼리 콜리전 전제)의 구현.
4. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 세이브 참여 마커의 규약과 라이프사이클.

## 관련
- 상위: 모든 Wx 도메인 플러그인이 소비 — [[WxCombat]](태그·`ECC_WxAttack`), [[WxWorld]]/[[WxInventory]](`IWxInteractable`), [[WxSave]](`IWxSavable`), [[WxUI]]/[[WxAI]]/[[WxQuest]]/[[WxDialogue]](태그)

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 9파일 — `/readme-writer`로 갱신*
