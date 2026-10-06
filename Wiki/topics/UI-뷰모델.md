# UI 뷰모델

UI의 MVVM 구성(층과 의존 방향, 뷰모델의 소유·공유, 리졸버, 바인딩)과 작성 규칙을 모은 문서다.
지금 유효한 규칙 요약은 [UI 설계 원칙](UI-설계-원칙.md)에 있고, 이 문서는 구현 사실과 결정 이력을 담는다.
레이어·입력 모드·일시정지는 [UI 구조](UI-구조.md)에 있다.

## 구현

### 층과 클래스
- 뷰모델·리졸버·변환 함수는 `Source/WxGame/UI/MVVM/`에 있다. UI 폴더 밖에서 뷰모델을 만들고 다루는 코드는 조립 층인 `AWxPlayerController`뿐이고, 캐릭터·전투·퀘스트·대화·인벤토리 코드는 뷰모델을 모른다.

| 뷰모델 | 누가 만들고 소유하나 | 위젯이 받는 길 |
|---|---|---|
| `UWxViewModel_Character`(이름, `AbilitySystem`) | 플레이어 것은 로컬 PC, 적·보스 것은 뷰마다 | `UWxViewModelResolver_Player`, 네임플레이트는 Manual, 보스 바는 `UWxViewModelResolver_BossCharacter` |
| `UWxViewModel_AbilitySystem` | Character VM이 ASC마다 새로 만들어 소유 | Character VM의 `AbilitySystem` 필드 |
| `UWxViewModel_Attribute`(현재·최대 쌍) | AbilitySystem VM이 쌍마다 지연 생성 | 변환 함수 `GetAttributeViewModel` |
| `UWxViewModel_Ability`(스킬 슬롯) | AbilitySystem VM이 `AbilityTags` 컨테이너(정확 일치)마다 | `UWxViewModelResolver_Ability`(플레이어) 또는 `GetAbilityViewModel` |
| `UWxViewModel_Effect` | AbilitySystem VM이 활성 GE마다(아이콘 없는 효과 제외) | AbilitySystem VM의 목록 |
| `UWxViewModel_Inventory` | 로컬 PC가 만들어 `VM_Inventory`로 등록 | `UWxViewModelResolver_Player` |
| `UWxViewModel_Item` | Inventory VM이 슬롯마다, 정의별 합계 | `UWxViewModelResolver_Item`(정의 지정), 목록 |
| `UWxViewModel_Quest`·`QuestObjective` | 리졸버가 위젯마다 | `UWxViewModelResolver_Quest`(GameState `UWxQuestComponent` 저널 구독) |
| `UWxViewModel_Dialogue` | 리졸버가 위젯마다 | `UWxViewModelResolver_Dialogue`(PC `UWxDialogueSessionComponent` 구독) |
| `UWxViewModel_InteractionList`·`Interaction` | 리졸버가 위젯마다 | `UWxViewModelResolver_InteractionList`(PC `UWxInteractionScannerComponent` 구독) |
| `UWxViewModel_Subtitle` | Global Collection에 `VM_Subtitle` 하나 | `UWxViewModelResolver_Subtitle` |
| `UWxViewModel_Indicator` | `AWxIndicator`가 만들어 Manual로 | — |
| `UWxViewModel_Damage`(피해량, 치명타 여부) | `AWxDamageFloaterActor`가 피해마다 만들어 Manual로 | — |

- 스킬 슬롯은 어빌리티 에셋 태그로 어빌리티를 지목한다([어빌리티 구현 구조](어빌리티-구현-구조.md#결정)). 부여가 바뀌면 슬롯 VM이 `AbilitySpecDirtiedCallbacks`를 직접 받아 다음 틱에 대상을 다시 고른다.
  - 엔진은 이 델리게이트를 권한 측에서만 부르므로 원격 클라에서는 다음 태그 변화 때 따라간다.
- AbilitySystem VM은 초기화 때 활성 GE로 이펙트 목록을 만들고 GE 추가·제거 통지로 고친다.

### 플레이어 공유 뷰모델
- 로컬 `AWxPlayerController`가 `BeginPlay`의 `Super` 앞에서 Character·Inventory VM을 Global Collection 객체를 Outer로 만들어 `VM_PlayerCharacter`·`VM_Inventory`로 등록한다. PC 컴포넌트가 시작되며 HUD를 띄우므로 그보다 먼저 등록한다. 원격 PC는 등록하지 않는다.
- Outer를 월드 객체로 두지 않는 것은 제거를 놓쳤을 때 게임 인스턴스 수명의 컬렉션이 옛 월드를 붙잡지 않게 하려는 것이다. `EndPlay`에서 컬렉션에서 빼고 비운다.
- `SetPawn`마다 같은 Character VM을 새 폰의 ASC로 다시 초기화한다. ASC가 같으면 이름만 고치고, 다르면 AbilitySystem VM을 새로 만들며, 빙의가 풀리면 비운다.
- Inventory VM은 `Super::BeginPlay` 뒤(시작 아이템 지급 뒤) `Initialize(인벤토리 컴포넌트)`로 인벤토리를 직접 구독한다. 탭(`CurrentCategory`, `Item.Category.*`)은 PC가 사는 동안 같은 인스턴스라 화면을 다시 열어도 유지된다.
- 코드는 각 VM 클래스의 `FindPlayer`로, WBP는 위젯이 기대하는 클래스로 컬렉션을 찾는 `UWxViewModelResolver_Player`로 받는다. 이름 상수는 각 클래스의 `GetPlayerContext` 한 곳에 있다.
- 키별 자식 중 슬롯·아이템은 Ability·Item 리졸버가 플레이어 공유 VM에게 키로 요청해 받고, 어트리뷰트는 변환 함수 `GetAttributeViewModel`로 받는다.

### 위젯마다 만드는 뷰모델
- Quest·Dialogue·InteractionList·BossCharacter 리졸버는 위젯을 Outer로 VM을 새로 만들고, 모델의 지금 값으로 한 번 채운 뒤 모델 델리게이트에 그 VM을 소유자로 구독한다. `DestroyInstance`에서는 그 VM의 구독을 끊고, 스스로 보스 ASC를 구독하는 BossCharacter의 Character VM은 `Deinitialize`까지 한다(ed7a5a640).
- 보스 바의 현재 보스 추적은 [네임플레이트](네임플레이트.md#구현)에 있다.
- 월드 공간 위젯(적 네임플레이트, 인디케이터, 데미지 플로터)은 위젯을 만든 쪽이 VM을 만들어 `SetViewModelByClass`로 넣는다. 위젯의 소스는 Manual이고, 실패하면 경고 로그를 남긴다.
  - 데미지 플로터의 값은 표시 뒤 바뀌지 않으므로 `UWxViewModel_Damage` 필드에 FieldNotify가 없고, `WBP_DamageFloater`의 바인딩은 모두 OneTime이다.
  - 피해량은 `ToText (Float)`(내림, 자릿수 구분 없음, 소수 0자리)로, 치명타 표시 `!!`는 별도 텍스트 `CriticalText`의 Visibility를 `bIsCritical`로 켠다.

### 자막
- 자막은 화면당 하나라 `UWxViewModel_Subtitle::GetOrCreate`가 Global Collection에 `VM_Subtitle` 하나만 둔다. 표시 위젯과 문구를 거는 StateTree 노드가 같은 인스턴스를 찾아가야 해서다.
- StateTree 태스크 `FWxStateTreeTask_PrintSubtitle`이 자막 표 행을 걸고, 받은 핸들이 지금 자막의 것일 때만 걷는다. 슬롯이 하나라 나중 요청이 이긴다.

### 명령과 바인딩 보조
- 뷰의 명령은 뷰모델의 `BlueprintCallable` 함수다. `UWxViewModel_Dialogue::RequestAdvance`는 약참조로 든 대화 세션을 직접 부른다.
- 스킬·아이템 슬롯 클릭(`WBP_Ability`·`WBP_ItemQuickSlot`)은 버튼의 `OnButtonBaseClicked`를 MVVM 이벤트로 `TryActivateAbility`에 잇는다. 인벤토리 탭 버튼은 같은 방식으로 `SetCurrentCategory`에 잇고, 탭 태그는 이벤트 인자 리터럴이다.
- 목록 항목 위젯은 `OnListItemObjectSet`에서 받은 항목을 자기 VM으로 넣는다(`WBP_Effect`·`WBP_Interaction`·`WBP_ItemSlot`·`WBP_QuestObjective`·`WBP_AcquiredItemEntry`).
- 상호작용 목록 VM은 명령 없이 표시만 한다. 상호작용 입력은 캐릭터가 받는다([상호작용](상호작용.md#흐름)).
- 획득 토스트는 Inventory VM의 `OnItemAcquired` 델리게이트를 `WBP_AcquiredItemList`가 MVVM 이벤트 바인딩(`AcquiredItemList.AddItem`)으로 받는다. 획득 VM은 이벤트 인자로 `LastAcquiredItem`을 읽는다.
  - 한 번의 획득이라도 스택 병합·새 덩어리마다 `OnInventoryStackChanged`가 따로 와 알림이 나뉠 수 있다(`AddItemDefinition`). 10-06 인게임 확인에서 문제로 보지 않았다.
- `UWxMVVMConversionLibrary`는 가시성 변환(`Conv_PositiveFloatToSlateVisibility`·`Conv_GameplayTagToSlateVisibility`·`Conv_ObjectToSlateVisibility`)과 자식 VM 조회(`GetAttributeViewModel`·`GetAbilityViewModel`)를 준다.
- 아이콘은 VM이 소프트 참조를 비동기 로드해(`WxViewModel::RequestImageAsync`, 같은 필드의 이전 요청은 취소) 로드된 객체를 `Icon` 필드로 내놓는다.

### 엔진 동작에 기댄 곳
- `FMVVMViewModelContext`는 이름이 None인 키를 거부하고, 등록과 조회는 같은 컨텍스트를 써야 한다. 그래서 이름을 VM 클래스의 정적 함수 한 곳에 둔다.
- `AddViewModelInstance`는 같은 키가 이미 있으면 교체하지 않고 실패한다. 교체하려면 `RemoveViewModel` 뒤 Add해야 한다. 플레이어 VM을 빙의마다 교체하지 않고 다시 초기화하는 이유다.
- 뷰는 초기화 때 소스를 한 번만 가져간다. Global Collection 소스는 WBP의 `bGlobalViewModelCollectionUpdate`(기본 꺼짐)를 켜야 등록·교체·제거를 따라가고, 리졸버 소스는 따라가지 않는다.
- 뷰가 해제될 때 엔진이 리졸버의 `DestroyInstance`를 부른다(기본 구현은 아무것도 하지 않는다). 리졸버가 공유 인스턴스를 돌려줬다면 여기서 해제하면 안 된다. 그래서 `UWxViewModelResolver_Player`는 이것을 오버라이드하지 않는다.
- `UWidgetComponent`가 만든 위젯에는 컴포넌트나 오너로 가는 역참조가 없다. Outer는 게임 인스턴스이고 소유 플레이어는 보는 쪽 로컬 플레이어라, 리졸버가 위젯만 보고 어느 액터의 것인지 알 수 없다.
  - Global Collection 이름도 WBP에 컴파일 타임 상수로 박혀 같은 WBP를 쓰는 액터끼리 나눌 수 없다.
  - 그래서 액터마다 소스가 다른 월드 공간 위젯은 만든 쪽이 Manual로 넣고, 리졸버는 위젯만으로 소스를 찾을 수 있을 때(로컬 플레이어, 월드의 현재 보스)만 쓴다.
- 바인딩 목적지는 인자 하나인 세터만 될 수 있다(`IsValidForDestinationBinding`). 엔진의 지연 로드 이미지 세터(`SetBrushFromLazyTexture`·`SetBrushFromSoftTexture`)는 인자가 둘이라 목적지가 못 되고, 변환 함수는 값을 돌려주는 순수 함수라 비동기 로드를 표현하지 못한다. 아이콘을 VM이 직접 로드해 내놓는 이유다.
- 변환 함수의 const 참조 구조체 인자에는 바인딩 패널이 리터럴 편집 위젯을 만들지 않는다.
- 상태 바인딩은 뷰 초기화 때 현재 값으로 한 번 실행되지만, 이벤트 바인딩은 실행되지 않는다. 이벤트 소스로 뷰모델의 BlueprintAssignable 델리게이트를 쓸 수 있다(컴파일러가 ViewModel 소스를 지원한다).
- 이벤트 바인딩은 델리게이트 인자를 목적지 함수로 넘기지 못한다. 목적지 인자는 위젯·뷰모델 프로퍼티 경로나 리터럴로만 잇는다.
- 엔진 ListView 뷰모델 확장(Details의 Viewmodel Extension)은 `OnEntryWidgetGenerated`에서 항목 VM을 넣는데, 이 알림은 다음 틱으로 미뤄진다(`UListViewBase::FinishGeneratingEntry`). 그래서 새 항목이 한 프레임 동안 디자이너 기본값으로 그려진다. 또 `SetListItems` 바인딩이 없으면 컴파일 경고를 낸다.

## 결정
- 2026-09-30 UI 설계 원칙을 채택했다. 원칙 목록은 10-06에 [UI 설계 원칙](UI-설계-원칙.md)으로 옮겨, 그 뒤 바뀐 것(10-01에 빠진 두 항목, 10-06에 더한 수명·입력·이벤트 기준)과 함께 지금 유효한 형태로 모았다. 당시 '입력은 뷰모델 명령으로 보낸다'는 10-06에 '게임플레이 입력은 캐릭터가 받는다'로 좁혀졌다. (사용자 결정 "네 이것을 우리 프로젝트의 UI 설계 원칙으로 합시다", 커밋 bfb526f5c)
- 2026-09-30 플레이어 공유 뷰모델을 로컬 PC가 만들어 엔진 Global Collection에 등록한다. 소스를 Outer로 두고 `FindObjectWithOuter`로 찾아 공유하던 방식, 루트 뷰모델(`UWxViewModel_Player`), PC Outer 보관, 서브시스템 저장소는 다시 넣지 않는다. Outer는 GC에서 객체를 살려 두지 않아 HUD가 없는 틈(빙의 교체 뒤 비동기 push, 빙의 해제)에 공유 VM이 수거되고 인벤토리 탭 유지가 깨질 수 있었다. `UMVVMGameSubsystem` 상속은 위젯 쪽 조회가 기본 클래스로 고정돼 쓰지 않는다. (사용자 결정 "그냥 PlayerController가 뷰모델 만들어서 엔진 Global Collection에 등록, 제거 하는게 훨씬 더 단순하겠네요", 커밋 bfb526f5c)
  - WBP는 이름 문자열 대신 플레이어 리졸버로 받는다. 그래서 플레이어 공유 VM은 클래스마다 하나만 등록한다. (사용자 결정, 코드 리뷰 중)
  - 인벤토리 탭 키는 `Item.Category.*` 게임플레이 태그다. (사용자 결정)
- 2026-10-01 플러그인 통합([게임 프레임워크 구조](게임-프레임워크-구조.md#결정))으로 원칙 중 '뷰모델은 도메인 타입을 쓰지 않는다'와 '도메인 명령은 델리게이트로 내보낸다'가 빠졌다. 뷰모델은 모델 컴포넌트를 약참조로 들고 직접 부르며, 중간 델리게이트나 계약 인터페이스를 새로 만들지 않는다. (커밋 20883a590)
- 2026-09-23 소스가 위젯보다 늦게 나타나는 전역 UI는 '월드 쪽 발행자 + 리졸버가 위젯별 VM을 채우고 구독, `DestroyInstance`에서 그 VM의 구독만 해제' 모양을 따른다. 보스 바([네임플레이트](네임플레이트.md#결정))에서 정했고 퀘스트·대화·상호작용 목록 리졸버도 같다. (커밋 4352e9100)
  - 10-03부터 보스 리졸버는 스스로 구독하는 Character VM을 `Deinitialize`까지 한다. (커밋 ed7a5a640)
- 2026-09-05 모델(액터·컴포넌트)은 뷰모델을 만들거나 초기화하지 않는다. 모델은 일어난 사실만 델리게이트·태그로 발행하고, 표시 데이터로 옮기는 일은 뷰모델 층이 한다. 모델이 뷰모델을 알면 UI 헤더가 게임플레이 코드로 새고 표시 정책이 두 층에 흩어진다. `AWxEnemyCharacter`가 보스 뷰모델을 발행하는 안을 이 이유로 기각했다. (사용자 결정 "WxEnemyCharacter는 모델이기 때문에 ViewModel을 직접적으로 알면 안 된다", 커밋 6571b5f0e)
- 2026-07-23 뷰가 도메인에 명령을 보내는 길은 뷰모델의 `UFUNCTION(BlueprintCallable)` 명령 함수다. 당시 예는 대화 VM의 `Request~`였고, 10-06의 클릭 사용·탭 전환 명령 이름은 `TryActivateAbility`·`SetCurrentCategory`다. MVVM 바인딩은 프로퍼티와 위젯 사이 데이터 흐름만 다뤄 다른 지정자로 대신할 수 없다. 표시 전용 프로퍼티·핸들러에는 붙이지 않는다. (사용자 결정, 커밋 c56c55d73)
  - 당시 `BlueprintCallable`을 함수 라이브러리·비동기 액션 팩토리로 한정하던 코딩 규칙의 예외로 승인했다. 그 규칙은 09-12에 코딩 규칙에서 빠졌다. (커밋 e994d09f9)
- 2026-07-29 위젯 이벤트(버튼 클릭 등)는 이벤트 그래프 노드 대신 MVVM Event 바인딩으로 뷰모델 명령에 잇는다. 뷰에는 선언적 배선만 남고 실행 코드는 C++ 뷰모델에 모인다. 창 닫기 같은 수명 문제는 뷰모델 플래그로 두지 않고 창을 띄운 쪽이 도메인 종료 신호를 받아 닫는다. (사용자 결정, `WBP_DialogueScreen`)
- 2026-08-23 변환 함수에서 디자이너가 바인딩 패널에 리터럴로 채울 구조체 인자(태그·어트리뷰트)는 값으로 받는다. 바인딩으로 이어지는 소스 값 인자는 참조여도 된다. `GetAbilityViewModel`의 태그 컨테이너를 const 참조로 두자 WBP에서 태그를 입력할 수 없었다. (사용자 확인)
- 2026-10-06 뷰모델 전수 점검 결과를 반영했다. (사용자 결정, 커밋 4d0a0a33a·0cd0539a9·e710ca684·a8dfefa4a·b7620a287)
  - 슬롯 재매칭은 슬롯 VM이 부여 변경을 직접 구독한다. 부모가 자식에게 갱신을 지시하면 자식을 직접 쓰는 새 소비자에서 표시가 조용히 멈춘다.
  - 이펙트 목록은 초기화 때 만든다. Character VM을 받는 세 WBP(Nameplate_Player·Enemy·Boss)가 모두 읽어 첫 조회까지 미루는 이득이 없었고, Getter 안 `const_cast`와 재진입 방어만 남겼다.
  - 쓰이지 않던 `UWxViewModelResolver_Attribute`를 지웠다. 어트리뷰트 바는 모두 `GetAttributeViewModel`로 받는다.
  - 대사 변경 통지는 네이티브 델리게이트다. 동적 델리게이트 때문에 `UWxViewModel_Dialogue::SetLine`이 UFUNCTION으로 열려 BP가 세션을 거치지 않고 표시값을 쓸 수 있었다.
  - `UWxViewModel_Item`의 `Instance`를 약참조로 바꾸는 안은 다시 넣지 않는다. `Refresh`가 `Instance` 유무로 슬롯과 정의 단위 합계를 가르므로 약참조가 비면 슬롯 VM이 말없이 합계 VM이 된다.
  - 뷰모델은 만든 쪽이 정리한다. 09-30 원칙에 '만든다'만 있고 '정리한다'가 없어, 네임플레이트가 위젯만 지우고 VM은 GC 전까지 적 ASC를 구독한 채 남았다.
- 2026-10-06 위젯 그래프에 둘 수 있는 것을 '뷰 안에서 끝나는 연출과 화면 이동'으로 정했다. 도메인 호출은 뷰모델 명령과 MVVM 이벤트 바인딩으로 한다. 09-30의 '위젯에는 바인딩과 변환 함수만 둔다'는 토스트 등장 애니메이션 뒤 목록에서 빠지기, 메뉴의 화면 push, 데미지 숫자 표시 같은 실제 WBP와 맞지 않았다. (사용자 결정 "제안해주신대로 합시다")
  - 이에 맞춰 인벤토리 탭과 아이템 퀵슬롯 클릭을 그래프 노드에서 MVVM 이벤트로 옮겼다. 탭 태그 같은 이벤트 인자 리터럴은 `WxMVVMToolset.SetEventArgumentValue`로 넣는다.
  - 목록 항목 VM 지정(`OnListItemObjectSet`)은 그래프에 남긴다. 엔진 ListView 뷰모델 확장으로 옮겨 보았으나 새 항목이 한 프레임 동안 디자이너 기본값(물약 아이콘, "Interact", "Quest Objective")으로 그려져 되돌렸다. 엔진이 항목 알림을 같은 틱에 하기 전에는 다시 넣지 않는다. (엔진 소스 확인)
- 2026-10-06 Claude 메모리에만 있던 뷰모델 지침 둘을 옮겼다. 다른 AI와 팀원도 같은 기준을 보게 하려는 것이다. (사용자 결정)
  - 새 표시·알림 채널은 새 뷰모델 클래스 대신 기존 뷰모델에 필드나 델리게이트를 더한다. 사용자가 새 타입이 늘어나는 것을 원치 않는다. 새 뷰모델 클래스가 정말 필요하면 이유를 설명하고 동의를 받는다.
  - 뷰모델은 스스로 구독하고 스스로 갱신한다(어빌리티·이펙트 VM의 월드 타이머). 티커 수를 줄이려고 소유자가 공유 티커로 대신 돌리면 계약이 주석에만 남아, 그 뷰모델을 직접 쓰는 새 소비자에서 표시가 조용히 멈춘다. 옛 `UWxViewModel` 베이스의 공유 티커는 다시 넣지 않는다.
- 2026-10-06 게임플레이 입력(상호작용·스킬·아이템 사용 키)은 위젯이 받지 않고 캐릭터가 받는다. BP에서 실행하는 함수를 줄이려는 것이다. VM 명령은 UI 안에서만 뜻이 있는 조작(대사 넘기기·탭 전환)과 마우스로 슬롯을 눌러 쓰는 클릭 사용(`TryActivateAbility`)에만 둔다. (사용자 결정, 커밋 d9768ca8d)
  - 이 목적으로 상호작용 VM 구조를 다시 짤 필요는 없다. 선택이 바뀌면 행 VM을 다시 만드는 09-23 방식(f98eef471)을 유지한다. 행 VM을 유지하고 선택만 갱신하던 이전 구조로 돌아가면 동기화 코드와 갱신 경로 둘이 되살아난다.
- 2026-10-06 한 번만 반응해야 하는 신호(획득 알림)는 상태 필드가 아니라 VM 델리게이트와 MVVM 이벤트 바인딩으로 보낸다. 필드에 넣었다 통지 없이 비우던 방식은 뷰 초기화 때 null로 실행돼 경고를 냈고 수신 바인딩에 즉시 실행을 강제했다. WBP에서 null을 거르는 안은 BP 로직을 늘리고 우회가 남아 기각했다. (사용자 결정, 커밋 5b8803a69)
- 2026-10-07 데미지 플로터 위젯에 값을 넣던 `IWxDamageFloaterInterface`를 걷어내고 `UWxViewModel_Damage`를 새로 만들었다. 기존 VM 중 피해 한 번의 값을 담을 곳이 없어서다. (사용자 결정)
  - 표시 뒤 값이 바뀌지 않으므로 바인딩은 모두 OneTime이다.
  - 치명타는 텍스트에 `!!`를 붙이지 않고 별도 텍스트의 Visibility로 켠다.
  - Lyra 대응(`ULyraNumberPopComponent_NiagaraText`)은 위젯 없이 Niagara로 숫자를 그리지만, 지금 위젯 외형을 유지하려고 VM을 택했다.

## 미결
- 자막은 StateTree 노드가 뷰모델을 직접 불러 '모델은 뷰모델을 모른다'에 어긋나는 알려진 예외다. 자막 상태를 GameState 컴포넌트에 두는 안이 1순위였고, 퀘스트 멀티플레이 정책과 함께 보기로 미뤘다(09-30 "퀘스트나 자막은 나중에 봅시다").
- `WBP_FrontEnd`는 새 게임 요청과 0.1초 타이머의 이동 상태 폴링을, `WBP_DeathScreen`은 부활 요청(`RequestRespawn`)을 그래프에서 직접 부른다. 각자 뷰모델을 두는 안을 사용자가 검토 중이다(10-06).
- 원격 클라에서는 어빌리티 부여·제거 통지가 오지 않아 슬롯이 다음 태그 변화 때 따라간다. 지금 플레이어 어빌리티는 서버에서 한 번 부여돼 폰과 함께 오므로 드러나지 않고, 런타임 스킬 교체가 생기면 그때 정한다(10-06).

## 관련
- [UI 설계 원칙](UI-설계-원칙.md)
- [UI 구조](UI-구조.md)
- [네임플레이트](네임플레이트.md)
- [어빌리티 구현 구조](어빌리티-구현-구조.md)
- [아이템과 회복](아이템과-회복.md)
- [상호작용](상호작용.md)

## 출처
- 사용자 대화로 정한 지난 결정: Claude 메모리 기록에서 옮기고 HEAD 2a3baca6a 코드로 확인 (2026-10-06 조회)
- `.agents/workflow/tasks/viewmodel-mvvm-redesign.md` (bfb526f5c, 10-01 삭제)
- 사용자 대화: 뷰모델 전수 점검과 수정 (2026-10-06, 커밋 42b9e878b~5b8803a69, 인게임 확인 완료)
- 사용자 대화: UI 지침 점검 후속 (2026-10-06, 인게임 확인 완료)
- 엔진 `UListViewBase::FinishGeneratingEntry`·`UMVVMBlueprintViewExtension_ListViewBase::Precompile` (UE 5.8)
- `Source/WxGame/UI/MVVM/` (5b8803a69)
- `Content/UI/Widget/WBP_AcquiredItemList.uasset` (5b8803a69)
- `Source/WxGame/UI/MVVM/WxViewModel_Character.cpp` (ed7a5a640)
- `Source/WxGame/UI/MVVM/WxViewModelResolver_Player.cpp` (d37e1dd32)
- `Source/WxGame/UI/MVVM/WxMVVMConversionLibrary.h` (ed7a5a640)
- `Source/WxGame/UI/MVVM/WxViewModelUtils.cpp` (d37e1dd32)
- `Source/WxGame/UI/Subtitle/` (6998ca09e)
- `Source/WxGame/Player/WxPlayerController.cpp` (20883a590)
- `Source/WxGame/UI/WxNameplateManagerComponent.cpp` (a8dfefa4a)
- `Source/WxGame/UI/IndicatorSystem/WxIndicator.cpp` (d37e1dd32)
