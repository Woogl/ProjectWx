---
title: "WxUI — 화면 레이어와 표시 수명"
category: topic
sources:
  - "raw/notes/2026-09-25-ui-presentation-verification.md"
  - "raw/notes/2026-09-25-ui-data-interface-removal.md"
  - "raw/notes/2026-09-23-screen-classes-to-resolvers.md"
  - "raw/notes/2026-09-23-quest-presentation-vm.md"
  - "raw/notes/2026-09-22-current-ui.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-player-screen-owner.md"
  - "raw/notes/2026-09-23-ability-resolver-module.md"
  - "raw/notes/2026-09-23-ability-resolver-colocation.md"
  - "raw/notes/2026-09-23-dialogue-presentation-vm.md"
  - "raw/notes/2026-09-23-dialogue-screen-lifecycle.md"
  - "raw/notes/2026-09-23-boss-battle-three-layer.md"
  - "raw/notes/2026-09-23-item-viewmodel-unification.md"
  - "raw/notes/2026-09-23-interaction-list-vm.md"
  - "raw/notes/2026-09-24-nameplate-manager.md"
  - "raw/notes/2026-09-24-nameplate-manager-wxgame.md"
  - "raw/notes/2026-09-24-wxui-review-followups.md"
  - "raw/notes/2026-09-25-ability-data-on-ga.md"
  - "raw/notes/2026-09-25-performance-config-defaults.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, ui]
aliases: ["WxUI"]
confidence: medium
volatility: warm
verified: 2026-09-25
summary: "WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 공용 태그·GAS를 관찰하며 도메인 고유 표시 값은 WxGame 리졸버에서 받는다."
---

# WxUI — 화면 레이어와 표시 수명

WxUI는 CommonUI 레이어와 MVVM 표시를 관리하고, 도메인 상태는 공용 태그·표시 계약으로 관찰한다.

## 화면과 상태의 연결

`UWxUIManagerSubsystem`은 GameInstance 수명으로 기본 레이아웃·확인 팝업·일시정지를 관리한다. `UWxPrimaryGameLayout`은 태그별 위젯 스택을 가지며 LayerTags 배열 순서가 z-order다. Game·GameMenu·Menu·Modal을 같은 의미로 취급하지 않는다. 메뉴 활성 판정은 Menu/Modal이고 GameMenu는 포함하지 않는다.

사망 태그는 Menu에 사망 화면을 띄우고, `State.Dialogue`는 Game에 대화 화면을 올린다. 이 관찰과 화면 클래스의 주인은 PlayerController의 `UWxPlayerLayoutComponent`다. 전역 `UWxUIDeveloperSettings`에는 UI의 틀(레이아웃·확인 팝업)만 두고, 게임 화면은 컨트롤러 BP에서 지정해 모드별로 바꿀 수 있게 한다. 태그가 먼저 사라지면 진행 중인 비동기 대화 화면 요청을 취소한다. 로드 완료 콜백에서도 현재 태그를 재확인해 이미 끝난 대화 창이 뒤늦게 나타나는 것을 막는다.

## 폰 교체와 구독 해제

PlayerController의 `UWxPlayerLayoutComponent`는 로컬 컨트롤러에서만 화면을 만든다. 폰 교체 때 기존 HUD와 비동기 요청을 정리하고 새 폰 기준으로 다시 생성한다. ViewModel이 생성 당시 Pawn의 ASC를 참조하므로 폰만 바꾸고 HUD를 남겨서는 안 된다.

폰 교체 때 태그 관찰도 새 폰으로 갈아타며, 대화 창은 닫지만 사망 화면은 닫지 않는다. 부활이 폰을 교체하고, 사망 화면은 부활 요청이 완료될 때 스스로 비활성화되기 때문이다.

Attribute ViewModel은 초기 값을 읽은 뒤 속성 변경을 구독하고, Deinitialize에서 같은 ASC의 구독과 캐시를 해제한다. GAS 공통 조회는 WxUI에 유지하고, 구체 어빌리티·GE·캐릭터의 표시 값은 WxGame에서 전달한다.

Effect ViewModel은 유한 지속 효과의 남은 시간을 월드 타이머로 월드 틱마다 갱신하고, 효과가 사라지면 멈춘다. 월드가 정지된 동안에는 타이머와 남은 시간이 함께 멈춘다. 무한 지속 효과는 갱신을 걸지 않는다.

버프 목록은 WxGame `UWxViewModelResolver_AbilitySystem`이 `UWxEffectComponent_UIData`를 읽어 제목·설명·아이콘을 전달하며, 아이콘을 채운 GE만 올린다. GE 에셋의 표시 데이터 소유권은 WxCombat에 유지한다. WxUI는 GE 추가·제거·스택·시간 갱신을 맡는다. 연결은 공유 AS VM에 한 번만 설정하며, 연결 전에 목록을 조회했어도 현재 활성 GE를 보충한다. `IWxUIData`는 2026-09-25 제거했다.

## 표시 VM의 위치와 연결

Wx 기능 모듈 사이의 신규 의존성은 WxCore를 제외하면 추가하지 않는다. WxCore도 공용 정의 범위를 유지하며 UI 이동을 위해 게임 로직이나 과도한 중계 계약을 넣지 않는다. 도메인 연결은 WxGame의 조립 책임이다.

VM은 WxUI에 모은다(2026-09-23 사용자 결정). 도메인 데이터가 필요한 표시는 세 층으로 나눈다.
- 모델(WxGame·도메인): 상태와 변경 델리게이트만 두고, VM·MVVM을 모른다.
- 연결: WxGame 리졸버가 맡는다. 도메인 연결용 C++ 위젯 부모 클래스는 두지 않는다. 사용자 이유: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없다."
- VM: WxUI의 표시 데이터와 GAS 공통 구독·갱신이다. 구체 도메인 타입 해석은 WxGame에 둔다(2026-09-25 합의).

리졸버는 위젯 클래스가 공유하는 const 객체라 상태를 들 수 없다. 그래서 구독은 VM을 소유자로 하는 약한 델리게이트(`FDelegate::CreateWeakLambda(VM, …)`)로 걸고, 해제는 `DestroyInstance`에서 `RemoveAll(VM)`으로 한다. 리졸버를 구독의 주인으로 두면 위젯 하나를 해제할 때 다른 위젯의 구독까지 끊긴다(08c73f513에서 고친 버그). 늦게 생긴 위젯을 위해 `CreateInstance`에서 현재 값을 한 번 반영한다.

도메인 델리게이트가 동적이고 인자가 VM의 Set 함수(UFUNCTION)와 맞으면 `AddUniqueDynamic(VM, &Set)`으로 직접 건다. 인자 없는 동적 델리게이트는 람다를 걸 수 없으므로 모델 쪽을 네이티브 멀티캐스트로 둔다. VM에서 도메인으로 가는 명령은 VM의 BlueprintCallable 함수와 네이티브 델리게이트로 전달한다. WBP의 MVVM 이벤트가 VM 함수를 부르고, 리졸버가 그 델리게이트를 도메인 함수에 약한 바인딩으로 잇는다. VM은 도메인을 모른다.

HUD 보스 바(`WBP_Nameplate_Boss`)가 이 규칙을 처음 적용한 사례다.
- WxGame의 `UWxViewModelResolver_BossCharacter`가 위젯마다 WxUI `UWxViewModel_Character`를 만들고, [게임 조립](game.md)의 `UWxBattleSubsystem`이 정한 현재 보스를 싣는다.
- 보스가 없으면 VM을 비우고, 그러면 가시성 바인딩이 바를 숨긴다.
- UIManager와 머리 위 Nameplate(`UWxNameplateManagerComponent`)는 보스를 모른다.
- WBP 로드·컴파일은 확인했지만 인게임 표시는 검증하지 않았다.

`UWxViewModelResolver_Ability`는 WxGame에 있다. 위젯 소유 Pawn의 ASC에서 AbilityTags에 대응하는 공유 슬롯 VM을 얻고, VM의 `FWxOnBoundAbilityChanged`를 리졸버 cpp의 정적 전달 함수로 연결한다. 슬롯 VM은 스펙의 기본 인스턴스 중 태그 조건을 충족하는 후보를 선택하고, 선택이 바뀌면 표시 값을 비운 뒤 제목·설명·아이콘·최대 충전 수·충전 한 칸의 시간을 받는다. 최초 매칭 전에 연결하며 충전·쿨다운 계산은 값을 받은 뒤 수행한다. 발동·비용 판정과 `TryActivateAbility`는 기존 GAS 경로를 유지한다. GA_ 프로퍼티는 WxCombat의 `UWxAbilityBase`가 소유한다([전투 어빌리티](../concepts/combat-abilities.md)).

`UWxViewModelResolver_PlayerCharacter`도 WxGame에 있다. Character VM에는 구체 캐릭터 대신 이름·초상화와 연결된 AbilitySystem VM을 전달한다. 캐릭터 공유본은 AS VM을 Outer로 삼으며, 재조회 시 초기화나 이미지 요청을 반복하지 않는다. 보스 리졸버의 위젯별 VM과 NameplateManager도 같은 값 전달 경로를 쓴다.

슬롯의 마지막 어빌리티를 회수하면 엔진이 인스턴스를 Garbage로 표시한다. 따라서 후보와 `CachedAbility.Get()`이 모두 null이어도 이전 표시를 정리해야 한다. `IsExplicitlyNull()`로 처음부터 빈 슬롯과 무효화된 참조를 구분한다. 2026-09-25 회귀 테스트 3개(슬롯 재연결·효과의 늦은 연결·캐릭터 공유)를 통과했고, 이동한 리졸버를 쓰는 위젯을 포함한 Blueprint 97개가 리다이렉트 없이 컴파일됐다. 실제 플레이·원격 복제는 별도 확인 범위다.

대화 창(`WBP_DialogueScreen`, 부모 `UWxActivatableWidget`)은 WxGame `UWxViewModelResolver_Dialogue`가 만든 WxUI `UWxViewModel_Dialogue`로 구동된다. VM은 Speaker·LineText·HasSpeaker와 SetLine, 진행 명령 `RequestAdvance`만 가진다. 리졸버가 세션 대사를 VM에 걸고, VM의 `OnAdvanceRequested`를 세션 `Advance`에 잇는다. 진행 버튼의 MVVM 이벤트 목적지는 `WxViewModel_Dialogue.RequestAdvance`다. 2026-09-23 이전의 `UWxDialogueScreen`(활성화 수명으로 연결)은 제거했다. 구독 수명과 인게임 확인은 [대화](dialogue.md)의 수명과 연출에 있다.

Quest·QuestObjective VM도 WxUI의 표시 데이터다. 퀘스트 추적기(`WBP_QuestTracker`, 부모 `UserWidget`)는 WxGame `UWxViewModelResolver_Quest`가 만든 Quest VM으로 구동되며, 2026-09-23 이전의 `UWxQuestTracker`는 제거했다. 목표별 행 VM은 Quest VM이 소유한다. 자세한 계약은 [퀘스트](quests.md)의 조립과 범위를 참고한다.

아이템 표시는 WxUI `UWxViewModel_Item` 하나로 통일했다(2026-09-23 사용자 결정, 커밋 `ba396fc16`). 이 VM은 원본 타입을 해석하지 않고 setter로 값을 받기만 한다. 값은 WxGame `UWxViewModel_Inventory`가 공급한다.
- 인벤토리 VM은 PC당 공유본이다(`GetOrCreate(PC)`, Outer = PC). `Resolver_Inventory`와 `Resolver_Item`이 같은 공유본을 쓰므로 리졸버가 `DestroyInstance`에서 정리하지 않는다.
- 슬롯 VM의 `SourceObject`는 ItemInstance, 고정 아이템의 합계 VM은 ItemDefinition이다. 합계 VM은 ItemDef를 키로 공유하며 인벤토리가 없을 때도 정적 정보로 만들어진다.
- 클라에서 인벤토리가 위젯보다 늦게 복제될 수 있다. VM은 교체하지 않고 인벤토리의 등장·제거를 관찰해 내부 연결만 바꾼다.
- 선택 탭(`CurrentCategory`)과 거른 목록(`CategorizedItems`)도 공유본에 있어, 인벤토리를 다시 열어도 마지막 탭이 유지된다. 인벤토리 창은 하나뿐이라 선택이 부딪히지 않는다. 사용자가 변환 함수 라이브러리보다 단순한 방법을 요청해 이 방식으로 정했다.
- 퀵슬롯 사용은 Ability VM(AbilityTags = `Ability.UseItem`)의 `TryActivateAbility`로 요청한다.
- 획득 토스트 VM은 획득 시점의 값으로 한 번만 채우고 이후 갱신하지 않는다.
- 빌드와 관련 위젯 6개 컴파일은 확인했고, 런타임 표시·사용은 검증하지 않았다.

상호작용 목록은 WxGame `UWxViewModel_InteractionList`가 스캐너를 구독해 행 VM(WxUI `UWxViewModel_Interaction`)을 신호마다 다시 만든다. 계약과 주입 전환 시 주의점은 [월드](world.md)의 HUD 목록 연결에 있다.

인벤토리 VM과 상호작용 목록 VM은 도메인 컴포넌트를 직접 구독하므로 아직 WxGame에 있다. 세 층 규칙으로 WxUI에 옮길 수 있지만 따로 설계가 필요하다(미결정). 상호작용 목록은 VM에서 도메인으로 가는 명령(`RequestInteract`·`RequestCycle`)의 전달 방식을(대화 진행이 쓴 VM 명령 델리게이트가 후보), 인벤토리 VM은 공개 헤더의 `WxItemDefinition` 의존(`EWxItemCategory`)을 정해야 한다.

MVVM 변환 함수는 위젯 블루프린트 자신의 Pure·const 함수이거나 `UBlueprintFunctionLibrary`의 정적 Pure 함수여야 한다(UE 5.8 엔진 제약). VM 클래스의 정적 함수는 엔진이 거부한다. MVVM 암시적 변환기는 enum을 다루지 않으므로, enum을 숫자로 바꿔 넘기면 표시 VM에 의미 없는 숫자 필드가 생긴다. 바인딩을 도구로 편집하는 방법은 [편집기 도구](../references/editor-tools.md)의 MVVM 절에 있다.

위젯 속성 바인딩(레거시 Bind)은 새로 만들 수 없다. 2026-09-25 `DefaultEditor.ini`에서 UMG와 에디터 유틸리티 위젯의 `PropertyBindingRule`을 Lyra와 같은 `Prevent`로 바꿨다. 표시 값은 MVVM 바인딩으로 연결한다. `Prevent`는 새 바인딩만 막고 기존 바인딩은 경고 없이 컴파일된다. 적용 시점 WBP_·EUW_ 41개에서 레거시 바인딩은 문자열 검색으로 찾지 못했다.

## 머리 위 Nameplate와 락온 Reticle

적 머리 위 Nameplate와 락온 Reticle은 보는 사람마다 다른 로컬 표시다. 커밋 `aaf557a09`에서 도입했고, 2026-09-24에 NameplateManager를 WxGame으로 옮겼다. Lyra처럼 게임 쪽 NameplateManager가 붙이고 뗀다.

- **위치와 구동:** WxGame `UWxNameplateManagerComponent`가 `AWxPlayerController`에 붙는다. 로컬 컨트롤러(리슨 호스트 포함)에서만 틱하며 Nameplate와 Reticle을 붙이고 뗀다.
  - 적 상태와 락온(WxCombat)을 직접 읽는 연결 코드라서 WxGame에 있다. 세 층(모델·연결·VM) 중 연결에 해당한다.
  - 위젯 클래스, 거리, 스케일, 높이 설정도 여기에 있다. 위젯 클래스 값은 `BP_PlayerController`에 있다.
  - 옛 WxUI 클래스 경로로 저장돼 있던 `BP_PlayerController`는 다시 저장했고, 리다이렉트는 두지 않는다.
- **대상 찾기:** `TActorIterator<AWxEnemyCharacter>`로 찾는다. BeginPlay 전인 적(스트리밍 직후)은 건너뛴다. 별도 마커 컴포넌트는 없다(옛 `UWxNameplateSourceComponent` 삭제).
- **표시 조건:** `IsAlive() && (LockOn 대상 || (거리 안 && State.Engaged))`.
  - 교전하지 않은 적도 락온하면 뜬다(사용자 결정).
  - 락온 가능 거리는 락온 쪽이 정하므로 LockOn 대상에는 거리 조건을 두지 않는다.
  - `State.Engaged`는 적이 `RefreshEngagement`로 한 곳에서 계산해 붙이는 월드 상태다. 뒤잡 판정도 같은 태그를 읽는다.
  - WBP에는 가시성 조건을 두지 않는다. 붙어 있으면 보인다.
- **락온 대상과 Reticle:** 락온 대상 지점은 빙의 캐릭터의 `UWxLockOnComponent::GetLockOnTarget()`을 매 틱 직접 읽는다.
  - Reticle은 그 지점(부위 컴포넌트)에 붙고, 지점이 바뀌면 다시 붙는다.
  - 락온 태스크는 대상 ASC에 태그를 붙이거나 위젯을 만들지 않는다.
  - Reticle은 `UWxLockOnComponent`에 두지 않는다. LockOnComponent는 AI와 시뮬 프록시에도 붙는 복제 모델이기 때문이다.

| 항목 | 규칙 |
|---|---|
| 거리 | 새로 붙이려면 `MaxVisibilityDistance`(3000cm) − `VisibilityDistanceHysteresis`(200cm) 안쪽이어야 하고, 이미 붙은 것은 3000cm까지 유지한다. 경계에서 붙였다 떼기를 반복하지 않기 위해서다. |
| 높이 | 대상 캡슐에 붙인다. 높이는 붙일 때 한 번, 캡슐 반높이 + `HeadClearance`(C++ 기본 90cm, 사용자 의도값)로 정한다(사용자 지시로 메시 기본 포즈 바운드 방식을 대체). 애니메이션 바운드와 `head` 본은 모션마다 흔들려 쓰지 않는다. |
| 수명 | 위젯 컴포넌트는 대상 액터 소유로 만들어 대상 파괴 때 함께 사라진다. NameplateManager의 EndPlay에서도 직접 뗀다. |
| VM | 대상 AS VM의 `UWxViewModel_Character` 공유본을 MVVM View에 넣는다. 공유본 수명은 View가 유지한다. |

마커 컴포넌트를 지우면서 적 BP 5종과 `LV_DevCombat` 배치 액터 1개를 다시 저장해 옛 데이터를 없앴다. 빌드, 관련 BP 컴파일, 레벨 재로드 시 경고 0건은 확인했다. 인게임 표시와 리슨 서버·원격 클라이언트에서 각자 자기 락온만 보이는지는 검증하지 않았다. 확인 항목은 [작업 자료](../../../.agents/workflow/tasks/nameplate-manager.md)에 있다.

## 일시정지와 제약

활성 `UWxActivatableWidget`의 ShouldPauseGame 요청을 보고 Standalone에서만 정지를 조정한다. 메뉴가 있다는 사실만으로 멀티플레이 월드를 정지하지 않는다.

정지는 `FCanUnpause` 대리자와 함께 건다. 게임모드는 해제 때 대리자를 건 정지만 되묻고 대리자 없는 정지는 그냥 지운다(UE 5.8 `AGameModeBase::ClearPause`). 그래서 UI가 정지를 원하는 동안에는 다른 해제가 이 정지를 풀지 못한다. 반대로 대리자 없이 건 다른 정지는 UIManager가 해제할 때 함께 지워진다. 2026-09-24 C++·에셋 검색으로는 다른 정지 주체가 없었고, 새로 추가하면 대리자를 함께 건다.

레이아웃과 추적 PC는 단수이므로 현재 구조는 로컬 플레이어 하나를 전제로 하며 스플릿스크린 지원으로 해석하지 않는다.

진입점: [UIManager](../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp), [HUD·사망·대화 화면 수명](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp), [NameplateManager](../../../Source/WxGame/Controller/WxNameplateManagerComponent.cpp), [설정](../../../Config/DefaultGame.ini). 화면 클래스 값은 `BP_PlayerController`에 있다. 사망·부활·대화 화면 동작은 2026-09-23 사용자가 인게임에서 확인했고, 그 밖의 WBP 바인딩과 화면 품질은 에디터·실행 확인이 필요하다.

## 관련 문서

- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer](../references/editor-tools.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [UI 표시 연결 회귀 검증과 제거된 슬롯 정리](../../raw/notes/2026-09-25-ui-presentation-verification.md) — 자동화·에셋 검증 범위, 무효화된 약한 참조의 빈 슬롯 정리
- [UI 데이터 인터페이스 제거와 리졸버 연결](../../raw/notes/2026-09-25-ui-data-interface-removal.md) — 2026-09-25 사용자 합의와 구현

- [대화·퀘스트 화면 클래스 제거와 리졸버 연결](../../raw/notes/2026-09-23-screen-classes-to-resolvers.md) — 화면 클래스 제거, VM 명령 델리게이트
- [Quest 표시 VM 분리](../../raw/notes/2026-09-23-quest-presentation-vm.md)

- [근거 1](../../raw/notes/2026-09-22-current-ui.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [근거 3](../../raw/notes/2026-09-23-player-screen-owner.md)
- [근거 4](../../raw/notes/2026-09-23-ability-resolver-module.md)
- [근거 5](../../raw/notes/2026-09-23-ability-resolver-colocation.md)
- [근거 6](../../raw/notes/2026-09-23-dialogue-presentation-vm.md)
- [근거 7](../../raw/notes/2026-09-23-dialogue-screen-lifecycle.md) — 이력: 제거된 대화 화면 클래스 방식
- [보스 표시 세 층 구조](../../raw/notes/2026-09-23-boss-battle-three-layer.md)
- [아이템 VM 단일화](../../raw/notes/2026-09-23-item-viewmodel-unification.md)
- [상호작용 목록 VM](../../raw/notes/2026-09-23-interaction-list-vm.md)
- [Nameplate·Reticle을 로컬 NameplateManager로](../../raw/notes/2026-09-24-nameplate-manager.md)
- [NameplateManager를 WxGame으로](../../raw/notes/2026-09-24-nameplate-manager-wxgame.md)
- [WxUI 리뷰 후속](../../raw/notes/2026-09-24-wxui-review-followups.md) — 일시정지 해제 규칙, Effect VM 월드 타이머
- [어빌리티·GE 데이터를 에셋 한 곳으로](../../raw/notes/2026-09-25-ability-data-on-ga.md) — 슬롯 VM의 기본 인스턴스 판정, GE 표시 데이터 컴포넌트
- [60FPS 강연 검토 후 적용한 프로젝트 기본 설정 3건](../../raw/notes/2026-09-25-performance-config-defaults.md) — 위젯 속성 바인딩 `Prevent`

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 2026-09-23에 보스 표시 세 층 구조 원자료(커밋 `4352e9100`)를 편찬해 추가했다. 같은 날 refresh에서 원자료 해시 대조로 누락을 찾아 아이템 VM 단일화(`ba396fc16`)·상호작용 목록 VM(`f98eef471`)·MVVM 변환 함수 제약을 HEAD `7d2a20408` 기준으로 추가하고, 표시 VM 절을 규칙→사례→예외 순으로 재배치했다. 이어서 대화·퀘스트 화면 클래스 제거 원자료(커밋 `570e72562`·`6daf3f804`)를 편찬해 연결 주체를 리졸버로 바꾸고 VM 명령 전달 규칙을 추가했다. 2026-09-24 refresh에서 NameplateManager 원자료(커밋 `aaf557a09`)를 편찬해 머리 위 표시 절을 추가하고, 이 절은 HEAD `ca84c9aac` 코드와 대조했다. 같은 날 두 번째 refresh에서 WxUI 리뷰 후속 원자료(커밋 `1b15a61ff`·`eb92e99a7`)를 편찬해 일시정지 해제 규칙과 Effect VM 갱신 주기를 HEAD `d76e48717` 코드·UE 5.8 엔진 소스와 대조해 추가했다(인게임 미검증). 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-25 refresh: 슬롯 VM의 기본 인스턴스 판정(`0473e201b`)과 버프 목록의 GE 표시 데이터 조회(`7c52ce0ce`)를 HEAD `d63ce0630`의 `WxViewModel_Ability.cpp`·`WxViewModel_AbilitySystem.cpp`·`WxEffectComponent_UIData.h`와 대조해 추가했고, 슬롯·버프 목록의 인게임 표시와 GE_ 에셋의 아이콘 값은 확인하지 않았다.

2026-09-25 두 번째 refresh: 위젯 속성 바인딩 `Prevent`(`be1c832e4`)를 `Config/DefaultEditor.ini`와 UE 5.8 `WidgetEditingProjectSettings.h`로 대조해 추가했다. 레거시 바인딩 0건은 uasset 문자열 검색 결과이며 에디터에서 위젯을 열어 확인하지 않았다.

</details>
