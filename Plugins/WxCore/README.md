# WxCore — 공용 정의 모듈

> 프로젝트 전체 플러그인이 공유하는 공용 정의(Gameplay Tag, 콜리전 채널, 도메인 간 인터페이스/추상 베이스)를 한곳에 모아두는 foundation 모듈. 구현은 두지 않고 선언/상수/계약만 제공한다.

## 책임
**담당**
- 프로젝트 전체에서 쓰는 Gameplay Tag의 C++ Native Tag 선언 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 정의 (`WxCollision::WxAttack`)
- 도메인 간 결합을 끊기 위한 공용 인터페이스/추상 베이스 선언 (`IWxSavable`, `IWxInteractionSource`, `UWxAbilityComponent`)

**경계 (비담당)**
- 실제 시스템 구현 일체. foundation 규칙상 직접 구현을 두지 않는다. 전투는 [[WxCombat]], 세이브 로직은 [[WxSave]], 상호작용/월드 오브젝트는 [[WxWorld]] 등이 담당
- Gameplay Tag를 dispatch/소비하는 어빌리티·이펙트 로직은 각 도메인 모듈에 위치
- `IWxSavable`/`IWxInteractionSource`의 구현체, `UWxAbilityComponent`의 구체 구현체는 각 도메인 모듈이 상속해 정의
- 콜리전 채널의 실제 ini 등록은 프로젝트 설정(DefaultEngine.ini)이 담당
- 도메인 컨텐츠/데이터 타입(어트리뷰트·아이템·어빌리티 클래스 등)은 여기 신설 금지 — 각 도메인 모듈로

## 의존성
- **주요 의존**: `GameplayTags` (Native Tag 선언용). 그 외는 빌드 기본(Core/CoreUObject/Engine)뿐
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅. WxCore는 의존 그래프 최하단 foundation이므로 어떤 Wx 플러그인도 참조하지 않아야 하며, Build.cs가 엔진 모듈만 의존하여 이를 만족한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 프로젝트 전 영역 Gameplay Tag 선언부 (State/Event/ANS/Cue/Damage/Ability/Input/SetByCaller/UI). 다른 모듈이 읽기 전 게임 구조를 잡는 어휘집 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `WxCollision` (namespace) | 커스텀 콜리전 채널 상수. `WxAttack = ECC_GameTraceChannel1`, ini 등록과 동기화하는 단일 출처 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `IWxSavable` | WxSave 슬롯 저장/로드 라이프사이클 참여 마커 + 후크 (`GetWxSaveId`, `OnWxSaveRestored`). WxSave↔소비 도메인 직접 의존 차단 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `IWxInteractionSource` | 상호작용 발행 컴포넌트의 공용 계약 (`GetOnInteractedDelegate`, `SetInteractionText`). 구현체는 WxWorld, 소비처(픽업 등)가 WxWorld에 의존 없이 BP에서 탐색 | `Plugins/WxCore/Source/WxCore/Public/WxInteractionSource.h` |
| `UWxAbilityComponent` | 어빌리티(`UWxAbilityBase`)에 Instanced 서브오브젝트로 붙는 컴포넌트의 추상 베이스. GE의 `UGameplayEffectComponent`에 대응하는 도메인 간 공유 앵커 | `Plugins/WxCore/Source/WxCore/Public/WxAbilityComponent.h` |
| `FWxCoreModule` | 모듈 진입점 (Startup/Shutdown, 별도 부트스트랩 없음) | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
이 모듈이 프로젝트의 유일한 C++ Native Tag 선언처다.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` (정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`)
- 주요 네임스페이스:
  - `State.*` — 캐릭터 상태 (Dead, Groggy, LockOn, InCombat, Invincible, Guard, PerfectGuard, HitReact, SuperArmor), 주로 ASC에 부여
  - `Event.*` — GameplayEvent dispatch 태그 (HitReact 계열, DodgeSuccess, PerfectGuard, UseItem)
  - `ANS.*` — AnimNotifyState 구간 (WeaponCollision, ComboWindow)
  - `GameplayCue.*` — Cue 트리거 (Damage, PerfectGuard, BuffATK, Exceed, Burn, HitStop, Metamorphose)
  - `Damage.*` — 대미지 판정 결과/속성 (Critical, Unblockable, ParryHitReact)
  - `Ability.*` / `Input.*` — 어빌리티·입력 매핑 (Attack, Dodge, Sprint, Guard, Skill_N, Ultimate, Interact, UseItem, AI Pattern_N 등)
  - `SetByCaller.*` — GE SetByCaller 키 (Duration, Recovery_UP/MP, ReflectDP, Coeff_ATK, RawDamage)
  - `UI.Layer.*` / `UI.Action.*` — UI 레이어 스택(Game/GameMenu/Menu/Modal) 및 CommonUI 액션(Inventory/MainMenu)

## 확장 포인트 / 규약
- 태그 추가: `WxGameplayTags.h`에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `WxGameplayTags.cpp`에 정의를 같이 작성. 변수명은 점(.)을 언더스코어(_)로 치환 (`State.Dead` → `State_Dead`). 다른 모듈에서 임의 선언 금지
- 세이브 대상 액터: `IWxSavable`을 구현하고 `GetWxSaveId()`로 세션 불변 `FGuid`를 반환(보존 필드에 `UPROPERTY(SaveGame)` 표시). 복원 후처리는 `OnWxSaveRestored()` 오버라이드. 인터페이스를 WxCore에 둠으로써 WxSave ↔ 소비 도메인(예: WxWorld)의 직접 의존을 끊는다
- 상호작용 발행: `IWxInteractionSource`를 통해 소비 도메인이 WxWorld 구현체에 의존하지 않고 델리게이트 바인딩/프롬프트 갱신. 델리게이트는 서버+모든 클라이언트에서 fire (최대 4인 멀티)
- 공유 어빌리티 컴포넌트: 도메인 모듈에서 `UWxAbilityComponent`를 상속해 구체 컴포넌트 정의 (예: WxUI의 UI 데이터 컴포넌트). 베이스만 WxCore에 두어 도메인 간 공유 앵커로 사용
- 콜리전 채널 추가 시 `DefaultEngine.ini`의 채널 등록 순서와 `WxCollisionChannels.h` 상수가 일치해야 함
- 여기엔 정의/공용 계약만 둔다. 리플리케이션·권한 로직은 갖지 않으며 소비 도메인이 책임진다

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전체 태그 체계를 한눈에 파악 (각 태그 주석에 소비처 명시되어 시스템 색인 역할)
2. `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` / `WxInteractionSource.h` — 도메인 결합 분리 패턴의 대표 예시 (인터페이스 후크)
3. `Plugins/WxCore/Source/WxCore/Public/WxAbilityComponent.h` — 같은 분리 패턴의 또 다른 형태 (추상 베이스 앵커)

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxQuest]], [[WxSave]])과 게임 모듈 [[WxGame]]이 WxCore를 참조

---
*문서 기준 커밋 `6e6d0ae` · 생성일 2026-06-18 · 소스 8파일 — `/readme-writer`로 갱신*
