# WxCore — 공용 정의 foundation

> 모든 Wx 플러그인이 함께 참조하는 공용 어휘를 한곳에 모은다. Gameplay Tag, Collision Channel, 그리고 도메인 간 결합을 끊는 마커 인터페이스가 여기 산다.

## 책임
**담당**
- 프로젝트 전역 Gameplay Tag의 C++ Native 선언·정의 (State / Effect / Movement / Event / Gimmick / GameplayCue / Damage / Ability / SetByCaller / UI)
- 커스텀 Collision Channel 상수 (`ECC_WxAttack`)
- 도메인 경계용 마커 인터페이스 (`IWxInteractable`, `IWxSavable`) — 계약과 그 공용 헬퍼·기본 구현

**경계 (비담당)**
- 태그를 실제로 부여/소비하는 로직 — [[WxCombat]], [[WxUI]], [[WxAI]] 등 각 도메인
- 상호작용 스캔·어빌리티 실행, 상호작용 대상 구현 — [[WxWorld]] (인터페이스만 여기 두고 구현은 소비 도메인)
- 세이브 슬롯 직렬화·라이프사이클 — [[WxSave]]
- Collision Channel의 실제 등록값·프로파일 — `Config/DefaultEngine.ini`

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 매크로), `Engine`/`CoreUObject` (인터페이스·Collision 타입). Wx 플러그인 의존 없음.
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxCore.Build.cs`는 Core/CoreUObject/Engine/GameplayTags만 의존)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전역 Native Tag 선언부. 태그 추가는 이 헤더 + 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수. `.ini` 등록 순서와 일치 필요 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxInteractable` | 상호작용 대상 계약. `Find`/`IsMeshInRange` 등 static 헬퍼 포함, BP 구현 불가 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 참여 마커 + 후크(`GetSaveId`/`OnSaveRestored`). BP 구현 불가 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |

## Gameplay Tags
`WxGameplayTags.h`(선언) + `WxGameplayTags.cpp`(`UE_DEFINE_GAMEPLAY_TAG` 정의)에서 전량 관리한다. 태그 추가/변경은 반드시 이 두 파일에서만.

네임스페이스별 의미:
- `State.*` — 락온·전투·처형·콤보윈도우·대화 등 복제/loose 상태 표식
- `Effect.*` — GE가 부여하는 상태(무적·가드·퍼펙트가드·탈진·슈퍼아머). 애셋 태그로도 사용
- `Movement.*` — 체공·질주 (WxCharacterMovementComponent / Sprint 어빌리티가 토글)
- `Event.*` — HitReact 계열 및 처형/사망/그로기 등 Gameplay Event 라우팅 키
- `Gimmick.*` — 기믹 StateTree 상태값. 코드가 읽지 않고 세이브 슬롯에 저장되는 값
- `GameplayCue.*` — 연출 큐 (데미지 플로터·임팩트·텔레그래프)
- `Damage.*` — 대미지 분류(크리티컬·가드불가·패리유발)
- `Ability.*` — 어빌리티 식별/분류 태그. 규약은 헤더 주석 참조 (`Ability.X` = 활성 중)
- `SetByCaller.*` — GE SetByCaller 매그니튜드 키
- `UI.*` — CommonUI 레이어(`UI.Layer.*`) 및 액션(`UI.Action.*`) 태그

## 확장 포인트 / 규약
- **태그 추가**: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 `UE_DEFINE_GAMEPLAY_TAG` 짝으로. 소비는 각 도메인 모듈.
- **Collision Channel 추가**: 상수는 여기, 실제 채널/프로파일 등록은 `DefaultEngine.ini`. 둘의 순서·이름이 어긋나면 히트 판정이 조용히 깨진다.
- **인터페이스 계약을 WxCore에 두는 이유**: 소비 도메인(예: WxInventory 픽업, WxWorld 기믹)이 서로/제공 모듈(WxWorld, WxSave)에 직접 의존하지 않고도 자기 액터를 상호작용/세이브 대상으로 만들 수 있게 한다. 두 인터페이스 모두 액터 또는 그 컴포넌트가 구현하며(순수 BP 호스트 액터 지원), C++ 없이는 구현 불가.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트의 전역 어휘. 어떤 상태·이벤트·어빌리티가 존재하는지 한눈에.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 상호작용의 클라 스캔↔서버 권위 검증 규약이 헤더 주석에 상세.
3. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` — 세이브 참여 계약과 `WxSaveId` 키 규칙.

## 관련
- 상위: 사실상 모든 Wx 도메인 플러그인이 소비 — [[WxCombat]](Effect/Event/Damage/Ability/SetByCaller 태그, `ECC_WxAttack`), [[WxUI]](UI 태그), [[WxWorld]]([[IWxInteractable]]·Gimmick 태그·[[IWxSavable]]), [[WxSave]]([[IWxSavable]]), [[WxAI]]/[[WxInventory]]/[[WxDialogue]] 등.

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 9파일 — `/readme-writer`로 갱신*
