# WxCore — 공용 정의 foundation

작업 단계: 구현

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> 여러 도메인 플러그인이 같은 단어로 이야기하기 위한 공용 어휘만 담는다. 게임플레이 태그, 상호작용·UI 데이터·스폰 대상의 인터페이스 계약, 공격 콜리전 채널 상수가 전부이며, 게임 로직은 한 줄도 갖지 않는다.

## 책임
**담당**
- 프로젝트 C++ Native Gameplay Tag의 유일한 선언처. 도메인 플러그인은 자체 태그를 선언하지 않는다([결정](../decisions/gameplay-tag-ownership.md), 2026-09-21 확인)
- 도메인 간 경계를 넘는 인터페이스 계약(`IWxInteractable`, `IWxUIData`, `IWxSpawnable`) 정의와 기본 구현
- 공격 판정용 커스텀 Object Channel 상수(`ECC_WxAttack`)의 C++ 측 고정
- 에디터 전용 로케이터 표시명 헬퍼

**경계 (비담당)**
- 태그를 실제로 붙이고 떼고 소비하는 주체 — 전투·리액션은 [WxCombat](WxCombat.md), 상호작용 실행은 [WxWorld](WxWorld.md)·[WxDialogue](WxDialogue.md), 레이어/액션 처리는 [WxUI](WxUI.md)
- 상호작용 대상 탐지와 프롬프트 표시 — 계약만 여기 두고 스캔·사거리 판정은 소비 측에 있다
- 소환물의 정책 선언과 생성·수명·상한 집행 — 전부 [WxCombat](WxCombat.md)의 `UWxMinionComponent`·`UWxMinionSubsystem`에 있다. 여기 있는 것은 스폰 대상 마커와 주인 태그 이름뿐이다
- 채널 등록 자체 — `DefaultEngine.ini`의 프로젝트 설정이 진짜 정의이고 이 모듈은 그 값을 코드에서 부를 이름만 준다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 모듈 전체에서 가장 많이 참조되는 지점. 태그 이름 하나가 어느 시스템에 닿는지 주석으로 적혀 있어 사실상 프로젝트 게임플레이 계약의 색인이다 | [Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h) |
| `IWxInteractable` | 상호작용 대상의 계약. 액터가 구현하며, 이 계약이 WxCore에 있어 인벤토리 픽업이 WxWorld에 의존하지 않고도 상호작용 대상이 된다 | [Plugins/WxCore/Source/WxCore/Public/WxInteractable.h](../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h) |
| `IWxUIData` | 표시용 데이터의 계약. WxUI가 도메인 플러그인(어빌리티·GE 컴포넌트)을 몰라도 아이콘·이름을 읽게 하는 역방향 의존 차단막 | [Plugins/WxCore/Source/WxCore/Public/WxUIData.h](../../../Plugins/WxCore/Source/WxCore/Public/WxUIData.h) |
| `IWxSpawnable` | 다른 주체가 스폰해 태어나는 액터의 계약. 처치 판정은 액터가 하고 `GetOnKilledDelegate()` 통지로 스폰 주체에 알린다. 스폰 주체를 아는 픽커([WxWorld](WxWorld.md) 스포너, [WxCombat](WxCombat.md) 소환 노티파이)가 `MustImplement`로 이 계약을 강제한다 | [Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h](../../../Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h) |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Type 상수. 전투·픽업·캐릭터 콜리전 설정이 모두 이 한 값을 참조한다 | [Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h](../../../Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h) |
| `FWxLocatorUtils` | `WITH_EDITOR` 전용. StateTree 태스크·디테일 커스터마이제이션이 로케이터를 사람이 읽는 이름으로 보여줄 때 쓴다 | [Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h](../../../Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h) |

## Gameplay Tags
- 선언: [Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h) / 정의: [Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp](../../../Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp)
- 모든 네이티브 태그를 이 두 파일에만 선언·정의한다([결정](../decisions/gameplay-tag-ownership.md)). 2026-09-21 전역 검색 기준 예외는 없다.
- 주요 네임스페이스:
  - `State.*` — ASC에 붙는 지속 상태(락온·교전·대화·래그돌). `State.MinionMaster.*` 는 소환물을 보유한 **주인** 쪽에 붙으며 잎이 소환물 종류다
  - `Effect.*` — GE가 부여하는 전투 상태(무적·가드·탈진·슈퍼아머·히트스톱). 애셋 태그로도 쓴다
  - `Event.*` — GameplayEvent 채널. 대미지 파이프라인·상호작용·장치·아이템 사용의 신호
  - `HitReact.*` — 공격이 요청하는 피격 반응 종류. `Event.Hit`의 TargetTags 페이로드로 전달된다
  - `Ability.*` — 어빌리티 식별·활성 상태에 사용하는 태그. 개별 어빌리티의 AssetTags와 ActivationOwnedTags 설정을 확인해야 하며 태그 존재만으로 모든 어빌리티의 활성 스펙을 단정하지 않는다.
  - `Cooldown.*` — 쿨다운 GE 식별 태그. WxCombat의 `CheckCooldown`은 충전 수·면제 태그를 먼저 확인한 뒤 필요하면 엔진 기본 검사로 위임한다.
  - `Damage.*` — 대미지 스펙의 성질·판정 결과(공격 표식·치명타·가드 가능·패리 가능·가드브레이크)
  - `SetByCaller.*` — GE 매그니튜드 주입 키
  - `Device.*` — 월드 장치의 StateTree 상태값. C++에서 읽지 않지만 선언은 여기 모은다
  - `GameplayCue.*` — 연출 큐
  - `Movement.*`, `UI.Layer.*`, `UI.Action.*`

## 확장 포인트 / 규약
- **새 태그**: 헤더에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` + `WXCORE_API`, cpp에 `UE_DEFINE_GAMEPLAY_TAG` 한 쌍. 도메인 안에서만 쓰는 태그도 예외 없이 여기에 모은다([결정](../decisions/gameplay-tag-ownership.md)).
- **새 상호작용 대상**: 액터(컴포넌트 아님)가 `IWxInteractable`을 구현한다. `CanInteract`는 기본 `true`이며 구현체가 자기 상태에서 파생시켜야 클라 표시 게이트와 서버 검증이 같은 답을 낸다. 쿼리 콜리전이 켜진 프리미티브가 없으면 스캔에 걸리지 않는다.
- **선택지가 여럿인 대상**: `GetInteractionOptions` 를 재정의해 `FWxInteractionOption`(문구 + 값)을 여러 개 내놓으면 HUD 상호작용 목록에 그 수만큼 행이 생긴다(기본 구현은 `GetInteractionPrompt()` 하나). 고른 값은 스캐너 RPC → `Event.Interact` 의 `EventMagnitude` → `OnInteracted(Interactor, OptionValue)` 로 돌아온다. 클라가 보낸 값이므로 상호작용 어빌리티가 서버에서 지금의 선택지와 대조하고, 통과하지 못하면 실행도 퀘스트 `상호작용 대기` 통지도 하지 않는다. 계약을 컴포넌트로 되돌리지 않고 옵션 목록으로 키운다는 기존 방향의 적용이다(현재 사용처: [WxWorld](WxWorld.md) 엘리베이터 탑승칸 버튼의 층 선택).
- **새 UI 표시 데이터**: 저작 데이터를 쥔 쪽(어빌리티·GE 컴포넌트)이 `IWxUIData`를 구현한다. `GetMaxRecharges`만 기본값 1로 제공된다.
- **새 스폰 대상**: 배치 스포너가 생성할 액터는 C++에서 `IWxSpawnable`을 구현하고, 처치되면 서버에서 `GetOnKilledDelegate()`를 방송한다. 통지가 네이티브 델리게이트라 BP만으로는 구현할 수 없다(`CannotImplementInterfaceInBlueprint`). WxWorld의 `AWxSpawner`는 `FinishSpawning` 전에 이 통지를 구독해 처치 상태를 세운다. 적이 아닌 대상도 스포너가 스폰할 가능성이 있어 적 전용 판정 대신 이 계약을 유지한다(2026-09-21 사용자 결정). WxCombat 소환 노티파이도 이 인터페이스를 에디터 필터로 쓰지만 소환 런타임은 이 통지를 구독하지 않고 `UWxMinionComponent`와 Instigator로 정책·주인을 관리한다.
- **의존 규칙**: foundation 모듈이므로 엔진 모듈 외에는 아무것도 참조하지 않는다. 다른 Wx 플러그인이 서로를 참조하지 않고 통신하려면 그 접점을 여기에 올린다.
- 리플리케이션 정책은 갖지 않는다 — 태그를 복제하는지 loose로 두는지는 발행하는 도메인이 정하며, 헤더 주석에 태그별로 적혀 있다.

## 여기서부터 읽어라
1. [Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h) — 한 파일로 프로젝트 게임플레이 흐름의 전체 지도를 얻는다. 태그마다 어느 시스템이 발행·소비하는지가 주석에 있다
2. [Plugins/WxCore/Source/WxCore/Public/WxInteractable.h](../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h) · `WxUIData.h` — 도메인 플러그인이 서로를 참조하지 않는 이유가 이 두 계약에 있다
3. [Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h](../../../Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h) — 히트 판정이 왜 메시에서만 일어나는지에 대한 근거. `DefaultEngine.ini`와 함께 본다

## 검증 범위와 근거

[Build.cs](../../../Plugins/WxCore/Source/WxCore/WxCore.Build.cs)·[descriptor](../../../Plugins/WxCore/WxCore.uplugin), 태그 선언·정의, [상호작용 기본 구현](../../../Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp), [UI 데이터 기본 구현](../../../Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp), [스폰 계약](../../../Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h), [로케이터 표시 헬퍼](../../../Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp)를 확인했다. 과거 `IWxMinion`은 제거되었으며 소환 정책은 WxCombat에 있다.

`ECC_WxAttack`은 [DefaultEngine.ini](../../../Config/DefaultEngine.ini)의 GameTraceChannel1 등록과 일치한다. 실제 메시·캡슐 응답은 소비 클래스와 에셋 설정도 함께 확인해야 한다. 위 태그 분류는 탐색용이며 전체 소비 경로·에셋별 태그 부여 결과를 전수 검증한 것은 아니다.

## 관련 모듈
- 상위: 모든 Wx 플러그인과 [WxGame](WxGame.md)이 이 모듈을 참조한다. 태그 소비가 가장 두꺼운 곳은 [WxCombat](WxCombat.md)이고, `IWxInteractable` 구현은 [WxWorld](WxWorld.md)·[WxDialogue](WxDialogue.md)·[WxInventory](WxInventory.md)·[WxGame](WxGame.md)에, `FWxLocatorUtils` 사용은 [WxWorld](WxWorld.md)·[WxQuest](WxQuest.md)·[WxUI](WxUI.md)의 StateTree 태스크에 있다

---
*이관 원문의 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 13파일 — 원문 출처 보존; 현재 확인 범위는 상단과 검증 절 참고*
