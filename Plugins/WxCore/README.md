# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 공유하는 최하단 정의 계층. Native Gameplay Tag, 크로스 도메인 인터페이스 계약, 콜리전 채널 상수를 한곳에 모아 도메인끼리 직접 의존하지 않게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag의 단일 선언처 (`WxGameplayTags`)
- 도메인이 서로를 참조하지 않고 만나기 위한 얇은 인터페이스 계약 (`IWxInteractable`, `IWxSavable`)
- 콜리전 채널·프리셋 상수 (`ECC_WxAttack`)
- 에디터 저작용 로케이터 표시 헬퍼 (`FWxLocatorUtils`)

**경계 (비담당)**
- 태그가 가리키는 실제 시스템 로직 — 전투는 [[WxCombat]], 대화는 [[WxDialogue]], 세이브 직렬화는 [[WxSave]], 장치·상호작용 구현은 [[WxWorld]], UI 레이어·액션 수신은 [[WxUI]]가 담당한다. WxCore는 태그·계약만 정의하고 구현은 하지 않는다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전 프로젝트 Native Tag 선언의 유일한 위치. 태그 추가 시 이 헤더와 짝 cpp에만 작성 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 액터의 공용 계약. 소비 도메인이 WxWorld에 의존하지 않고 상호작용 대상이 됨 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | LSP 속성 복원 직후 후처리 훅. WxSave와 소비 도메인을 분리 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수. DefaultEngine.ini 등록 순서와 일치해야 함 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명/목록 텍스트 헬퍼 (`WITH_EDITOR` 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Private/WxGameplayTags.cpp` (선언과 정의를 이 한 쌍에만 두는 규약)
- 주요 네임스페이스:
  - `State.*` `Effect.*` `Movement.*` — ASC에 붙는 상태·GE 부여 태그
  - `Event.*` — 대미지 파이프라인·어빌리티가 ASC에 보내는 GameplayEvent (`Event.Hit`, `Event.DamageDealt`, `Event.Finisher`, `Event.AbilityAction.*` 등)
  - `HitReact.*` — 대미지 테이블이 저작하는 피격 반응 종류, `Event.Hit` 페이로드로 전달
  - `Ability.*` — 어빌리티 식별 태그(하나당 정확히 하나, "활성 중" 판정으로도 사용), 플레이어/적 구분
  - `Device.*` — 장치 State Tree 상태값(세이브 저장 대상), 코드에서 직접 읽지 않음
  - `Damage.*` `GameplayCue.*` `SetByCaller.*` `UI.*` — 대미지 판정 플래그, 큐, GE SetByCaller 키, CommonUI 레이어·액션

## 확장 포인트 / 규약
- 새 태그: `WxGameplayTags.h`에 `UE_DECLARE_...EXTERN`, 짝 cpp에 `UE_DEFINE_...` — 다른 파일에 흩뿌리지 않는다.
- 새 상호작용 대상: 액터(컴포넌트 아님)가 `IWxInteractable` 구현. `CanInteract`는 밖에서 켜는 진입점 없이 구현체가 자기 상태에서 파생 — 클라 표시와 서버 검증이 같은 답을 받는다.
- 세이브 후처리가 필요한 액터: `IWxSavable::OnSaveRestored` 구현. LSP 복원 직후 호출되며 BeginPlay 이전일 수 있다.
- 콜리전: `ECC_WxAttack` 값은 `DefaultEngine.ini`의 채널 등록 순서와 반드시 일치.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 태그 doc-comment가 곧 각 시스템의 계약 요약이다. 다른 Wx 모듈을 읽기 전 여기서 전투·상호작용·UI 흐름의 지도를 얻는다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 간 의존을 끊는 계약 패턴의 대표 예. 왜 계약이 WxCore에 있는지 헤더 주석이 설명한다.
3. `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` — foundation 규칙 확인: 엔진 모듈만 의존, Wx 참조 없음.

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]] [[WxInventory]] [[WxUI]] [[WxWorld]] [[WxAI]] [[WxDialogue]] [[WxQuest]] [[WxSave]])과 [[WxGame]]이 WxCore를 참조한다. 역방향은 없다.

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 11파일 — `/readme-writer`로 갱신*
