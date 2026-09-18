# WxCore — 공용 정의 foundation

> 여러 도메인 플러그인이 같은 단어로 이야기하기 위한 공용 어휘만 담는다. 게임플레이 태그, 상호작용·UI 데이터·소환물의 인터페이스 계약, 공격 콜리전 채널 상수가 전부이며, 게임 로직은 한 줄도 갖지 않는다.

## 책임
**담당**
- 프로젝트 전체가 쓰는 C++ Native Gameplay Tag의 단일 선언처
- 도메인 간 경계를 넘는 인터페이스 계약(`IWxInteractable`, `IWxUIData`, `IWxMinion`) 정의와 기본 구현
- 공격 판정용 커스텀 Object Channel 상수(`ECC_WxAttack`)의 C++ 측 고정
- 에디터 전용 로케이터 표시명 헬퍼

**경계 (비담당)**
- 태그를 실제로 붙이고 떼고 소비하는 주체 — 전투·리액션은 [[WxCombat]], 상호작용 실행은 [[WxWorld]]·[[WxDialogue]], 레이어/액션 처리는 [[WxUI]]
- 상호작용 대상 탐지와 프롬프트 표시 — 계약만 여기 두고 스캔·사거리 판정은 소비 측에 있다
- 소환물의 생성·수명·상한 집행 — `IWxMinion`이 선언한 값을 읽어 집행하는 쪽은 [[WxCombat]]의 `UWxMinionSubsystem`
- 채널 등록 자체 — `DefaultEngine.ini`의 프로젝트 설정이 진짜 정의이고 이 모듈은 그 값을 코드에서 부를 이름만 준다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 모듈 전체에서 가장 많이 참조되는 지점. 태그 이름 하나가 어느 시스템에 닿는지 주석으로 적혀 있어 사실상 프로젝트 게임플레이 계약의 색인이다 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상의 계약. 액터가 구현하며, 이 계약이 WxCore에 있어 인벤토리 픽업이 WxWorld에 의존하지 않고도 상호작용 대상이 된다 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | 표시용 데이터의 계약. WxUI가 도메인 플러그인(어빌리티·GE 컴포넌트)을 몰라도 아이콘·이름을 읽게 하는 역방향 의존 차단막 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `IWxMinion` | 소환 가능한 액터가 자기 상한·어그로 정책을 선언하는 계약. 유일하게 BlueprintNativeEvent라 BP 소환물도 구현할 수 있다 | `Plugins/WxCore/Source/WxCore/Public/WxMinion.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Type 상수. 전투·픽업·캐릭터 콜리전 설정이 모두 이 한 값을 참조한다 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | `WITH_EDITOR` 전용. StateTree 태스크·디테일 커스터마이제이션이 로케이터를 사람이 읽는 이름으로 보여줄 때 쓴다 | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 새 태그는 이 두 파일에만 추가한다.
- 주요 네임스페이스:
  - `State.*` — ASC에 붙는 지속 상태(락온·교전·대화·소환물 보유·래그돌)
  - `Effect.*` — GE가 부여하는 전투 상태(무적·가드·탈진·슈퍼아머·히트스톱). 애셋 태그로도 쓴다
  - `Event.*` — GameplayEvent 채널. 대미지 파이프라인·상호작용·장치·아이템 사용의 신호
  - `HitReact.*` — 공격이 요청하는 피격 반응 종류. `Event.Hit`의 TargetTags 페이로드로 전달된다
  - `Ability.*` — 어빌리티 식별 태그. 각 어빌리티가 AssetTags·ActivationOwnedTags 양쪽에 정확히 하나 갖는 규약이라 "태그 보유 = 활성 중"이 성립한다
  - `Cooldown.*` — `Ability.*`와 짝을 이루는 쿨다운 GE 태그. 순정 `CheckCooldown`이 이 태그로 쿨다운을 식별한다
  - `Damage.*` — 대미지 스펙의 성질·판정 결과(공격 표식·치명타·가드 가능·패리 가능·가드브레이크)
  - `SetByCaller.*` — GE 매그니튜드 주입 키
  - `Device.*` — 월드 장치의 StateTree 상태값. C++에서 읽지 않지만 선언은 여기 모은다
  - `GameplayCue.*` — 연출 큐
  - `Movement.*`, `UI.Layer.*`, `UI.Action.*`

## 확장 포인트 / 규약
- **새 태그**: 헤더에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `WXCORE_API`, cpp에 `UE_DEFINE_GAMEPLAY_TAG` 한 쌍. 다른 곳에서 Native Tag를 선언하지 않는다.
- **새 상호작용 대상**: 액터(컴포넌트 아님)가 `IWxInteractable`을 구현한다. `CanInteract`는 기본 `true`이며 구현체가 자기 상태에서 파생시켜야 클라 표시 게이트와 서버 검증이 같은 답을 낸다. 쿼리 콜리전이 켜진 프리미티브가 없으면 스캔에 걸리지 않는다.
- **새 UI 표시 데이터**: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트)이 `IWxUIData`를 구현한다. `GetMaxRecharges`만 기본값 1로 제공된다.
- **새 소환물**: `IWxMinion`과 `IGenericTeamAgentInterface`를 함께 구현한다(소환물은 주인의 팀을 물려받는다). 에디터 쪽 참조는 `MustImplement = "/Script/WxCore.WxMinion"` 메타로 강제된다.
- **의존 규칙**: foundation 모듈이므로 엔진 모듈 외에는 아무것도 참조하지 않는다. 다른 Wx 플러그인이 서로를 참조하지 않고 통신하려면 그 접점을 여기에 올린다.
- 리플리케이션 정책은 갖지 않는다 — 태그를 복제하는지 loose로 두는지는 발행하는 도메인이 정하며, 헤더 주석에 태그별로 적혀 있다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 한 파일로 프로젝트 게임플레이 흐름의 전체 지도를 얻는다. 태그마다 어느 시스템이 발행·소비하는지가 주석에 있다
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` · `WxUIData.h` — 도메인 플러그인이 서로를 참조하지 않는 이유가 이 두 계약에 있다
3. `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` — 히트 판정이 왜 메시에서만 일어나는지에 대한 근거. `DefaultEngine.ini`와 함께 본다

## 관련
- 상위: 모든 Wx 플러그인과 [[WxGame]]이 이 모듈을 참조한다. 태그 소비가 가장 두꺼운 곳은 [[WxCombat]]이고, `IWxInteractable` 구현은 [[WxWorld]]·[[WxDialogue]]·[[WxInventory]]·[[WxGame]]에, `FWxLocatorUtils` 사용은 [[WxWorld]]·[[WxQuest]]·[[WxUI]]의 StateTree 태스크에 있다

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 13파일 — `/readme-writer`로 갱신*
