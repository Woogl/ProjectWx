# MVVM 원칙에 맞춘 뷰모델 재설계

상태: 완료 · 체크리스트 11/11 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- woogle 결정(2026-09-29): "아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다." 앞선 일감(viewmodel-quality-cleanup.md)의 코드 리뷰 중 공유 VM 조회 방식을 검토하다 나왔다.
- woogle 결정(2026-09-30): "네 이것을 우리 프로젝트의 UI 설계 원칙으로 합시다." 원칙 전문은 구현 계획 절 맨 앞. 이후 대화에서 월드 공간 위젯은 Manual 유지, 리졸버 문장 수정, 공유 기준·GC 소유·모델 정의를 보완했다.
- woogle 결정(2026-09-30): "퀘스트나 자막은 나중에 봅시다." 자막은 알려진 예외로 두고 후속 일감에서 본다.
- woogle 결정(2026-09-30): 루트 보관은 엔진 Global Collection을 쓴다("필요하다면 UMVVMGameSubsystem을 상속 받아서 우리 게임을 위한 서브시스템 만들어도 되잖아요"). 이어 "그냥 PlayerController가 뷰모델 만들어서 엔진 Global Collection에 등록, 제거 하는게 훨씬 더 단순하겠네요".
- woogle 결정(2026-09-30): 공유 뷰모델을 루트 없이 Global Collection에 각자 등록한다. "B로 하죠."
- woogle 결정(2026-09-30): 뷰모델 클래스는 모두 WxUI에 두고 도메인 타입을 쓰지 않는다(도메인 타입 뷰모델을 없애고 모듈화를 챙기자는 요청에서).
- woogle 결정(2026-09-30): "1번은 나중에 문제가 되었을 때 고민합시다. 2번은 방식 정해두죠. 3번은 필요할 때 mcp 툴 만드세요. 4번은 필요한 뷰모델에서 그렇게 쓰면 될 것 같아요. 5번은 작업할 때 문제가 되는 수준이라면 그렇게 하세요. 6번도 작업하면서 해결하죠." (1 PC가 커지면 컴포넌트로 떼기, 2 코드 조회 방식, 3 MVVM MCP 툴, 4 자동 갱신 플래그, 5 새 뷰모델 판단 순서, 6 기존 결정과의 충돌)
- woogle 결정(2026-09-30): "화면 분할은 대비 안해도 되요"
- woogle 결정(2026-09-30, 코드 리뷰 중): WBP가 이름 문자열로 Global Collection을 찾는 대신 리졸버로 받는다. "VM_PlayerCharacter, VM_Inventory 관련해서 리졸버를 만들면 이름 문자열 이슈 해결되지 않나요?" → "네. 그렇게 합시다"

## 요청

- 요청 · woogle 2026-09-29

> 아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다.

## 질문

| ID | 질문 | 선택지 | 추천 | 답변 |
| --- | --- | --- | --- | --- |
| Q1 | 루트 VM 클래스를 어느 모듈에 둘까요? 루트는 플레이어 Character VM과 Inventory VM을 타입으로 들어야 하는데 Inventory VM은 WxGame에 있고(WxInventory 타입 의존), WxUI는 WxCore만 의존합니다. | WxGame에 둔다(Inventory VM은 그대로) / Inventory VM을 WxUI로 옮기고 WxUI가 WxInventory에 의존 / Inventory VM을 표시부(WxUI)와 공급부(WxGame)로 쪼갠다 | WxGame에 둔다. 루트는 PC·폰·인벤토리를 직접 관찰하는 연결 코드라 이미 WxGame에 있는 InteractionList·Inventory VM과 같은 자리이고, 플러그인 의존을 늘리지 않습니다. | 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요. · woogle 2026-09-29 |
| Q2 | 루트를 어디에 보관할까요? | PC를 Outer로 두고 루트 리졸버 하나로 얻는다 / 엔진 Global Collection에 등록한다(스케치 원안) | PC Outer + 루트 리졸버. Global Collection은 게임 인스턴스 수명이라 맵 이동으로 PC가 바뀔 때 누군가 루트를 새 PC에 다시 붙이고 직접 빼야 하며, 로컬 플레이어 단위도 아닙니다. PC Outer면 PC와 함께 사라지고 조회 자리는 루트 하나뿐입니다. | 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요. · woogle 2026-09-29 |
| Q3 | 적·보스처럼 다른 캐릭터의 Character VM을 루트가 캐릭터 키로 소유할까요? | 소유하지 않는다: 네임플레이트·보스 바가 뷰마다 자기 Character VM을 만든다 / 루트가 캐릭터 키 맵으로 소유하고 사망·EndPlay 때 뺀다(스케치 원안) | 소유하지 않는다. 지금도 보스 바는 위젯마다 VM을 만들고, 같은 적을 두 뷰가 동시에 보는 경우는 보스 바와 그 보스 네임플레이트뿐이라 공유 이득이 작습니다. 루트가 소유하면 사망·스트리밍 아웃·파괴마다 빼는 규칙이 새로 생깁니다. | 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요. · woogle 2026-09-29 |
| Q4 | 이번 범위에 공유 VM(Character·AbilitySystem·Inventory)만 넣을까요, 위젯 하나만 쓰는 VM(Quest·Dialogue·InteractionList)도 루트 아래로 옮길까요? | 공유 VM만 / 셋 다 루트 아래로 | 공유 VM만. 세 VM은 위젯 하나가 자기 것을 만들어 쓰므로 여럿이 찾아 나눠 쓰는 로케이터 문제가 없고, 옮기면 루트 필드와 WBP 재연결만 늘어납니다. 필요해지면 같은 방식으로 뒤에 붙일 수 있습니다. | 공유 VM만 · woogle 2026-09-29 |
| Q5 | 새 VM 클래스 UWxViewModel_Player(WxGame, 로컬 플레이어 한 명의 루트: PlayerCharacter·Inventory 필드)를 만들어도 될까요? 이유: 플레이어의 Character VM과 Inventory VM을 함께 소유할 부모가 지금은 없고, 기존 VM 중 어느 것도 둘을 소유할 의미가 없습니다. | 만든다 / 이름을 바꿔 만든다(답변에 이름) | 만든다. | 만든다 · woogle 2026-09-29 |
| Q6 | 인벤토리 탭 키를 도메인을 모르는 무엇으로 바꿀까요? 지금 WBP_Inventory는 버튼 셋(장비·소모품·재화)의 OnClicked 이벤트 그래프에서 `SetCurrentCategory(EWxItemCategory)`를 부르고, 뷰모델은 WxInventory의 enum으로 거릅니다. 뷰모델을 WxUI로 옮기면 이 enum을 쓸 수 없습니다(WxCore에 도메인 enum을 두는 것도 금지). | 게임플레이 태그(WxCore에 `Item.Category.*` 네이티브 태그, PC가 enum을 태그로 바꿔 아이템 VM에 실음) / 정수 탭 인덱스(PC가 enum을 인덱스로 바꿈) / 카테고리별 명명 명령(`RequestShowEquipment` 등) | 게임플레이 태그. 타입이 있고 WBP에서 태그 선택기로 고를 수 있으며, 도메인 간 계약은 WxCore 태그로 발행한다는 기존 결정과 같습니다. 인덱스는 뜻 없는 숫자라 틀려도 드러나지 않고, 명명 명령은 WxUI 뷰모델이 인벤토리의 카테고리 목록을 알게 됩니다. | 태그로 합시다. · woogle 2026-09-30 |

## 구현 계획

구현 승인: woogle 2026-09-30

**UI 설계 원칙 (woogle 채택 2026-09-30)**

1. 층과 의존 방향: 모델 → 뷰모델 → 뷰. 모델은 뷰모델을, 뷰모델은 위젯을 모릅니다.
   - 모델: 도메인 상태·로직(ASC, 인벤토리·대화·퀘스트 컴포넌트, 도메인 서브시스템(BattleSubsystem 등)).
   - 뷰모델: 표시용 값(FieldNotify), 표시 상태(선택한 탭 등), 뷰의 요청을 받는 명령.
   - 뷰: WBP.
   - 조립 층: 모델을 관찰해 뷰모델에 값을 넣고, 뷰모델을 뷰에 넣고, 수명을 맞추는 곳(PC, 리졸버, 네임플레이트 관리 컴포넌트). 뷰모델을 알아도 됩니다.
2. 모듈: 모든 뷰모델 클래스와 위젯은 WxUI에 두고 도메인 타입을 쓰지 않습니다. 엔진 타입(ASC 등)은 직접 관찰해도 됩니다. 도메인 플러그인은 모델, WxGame은 조립 층입니다.
3. 조립 층의 역할 분담: 위젯마다 만드는 뷰모델(Quest, Dialogue, 보스 바)은 리졸버가, 플레이어 공유 뷰모델(Character, Inventory)은 로컬 PC가, 월드 공간 위젯의 뷰모델(적 네임플레이트)은 위젯을 만든 관리자가 만들고 값을 넣습니다.
4. 소유와 수명
   - 부모가 자식을 `UPROPERTY`로 들어 소유합니다. Outer는 소유가 아닙니다.
   - 뷰를 위해 새로 만든 뷰모델은 그 뷰(MVVM View의 소스)가 소유하고, 뷰와 함께 사라집니다.
   - 소스(모델)를 열쇠로 뷰모델을 찾는 저장소는 두지 않습니다.
   - 플레이어 공유 뷰모델
     - 로컬 `AWxPlayerController`가 Character·Inventory 뷰모델을 한 번만 만들어 각자 이름으로 엔진 Global Collection에 등록합니다(`VM_PlayerCharacter`, `VM_Inventory`). 원격 PC는 등록하지 않습니다.
     - Outer는 Global Collection 객체로 둡니다. 월드 객체로 두면 제거를 놓쳤을 때 게임 인스턴스 수명의 컬렉션이 옛 월드를 붙잡습니다.
     - 등록은 PC의 컴포넌트가 시작되기 전(`Super::BeginPlay` 앞)에, 제거는 `EndPlay`에서 합니다. 수명은 PC와 같습니다.
     - Global Collection이 강하게 들고, PC는 값을 넣기 위해 참조를 듭니다.
     - 빙의가 바뀌면 항목을 교체하지 않고 같은 Character 뷰모델을 새 ASC로 다시 초기화합니다(Global Collection의 Add는 같은 이름이 있으면 교체하지 않음). 빙의가 해제되면 비웁니다.
     - 인벤토리 변경을 Inventory 뷰모델에 넣는 일도 PC가 맡습니다.
   - 로컬 플레이어는 하나로 전제합니다(화면 분할 대비 안 함).
5. 조회
   - 조회로 얻는 뷰모델은 Global Collection에 둔 것(플레이어 공유 뷰모델, 앱 전체에 하나인 것)뿐입니다. 그 밖은 모두 주인에게서 받습니다.
   - WBP는 플레이어 리졸버(`UWxViewModelResolver_Player`, WxUI)로 받습니다. 리졸버가 위젯이 기대하는 클래스로 Global Collection에서 찾으므로 WBP에 이름 문자열이 없고, 플레이어 공유 VM은 클래스마다 하나만 등록합니다(woogle 2026-09-30 코드 리뷰 중 결정). 등록보다 먼저 떠서 등록을 따라가야 하는 위젯만 Global Collection 생성 방식으로 받습니다(리졸버 소스는 컬렉션 변경을 따라가지 않음).
   - 코드는 각 뷰모델 클래스에 둔 Global Collection 조회 정적 함수로 받습니다. 이름 상수도 그 클래스에 둡니다(자막 뷰모델과 같은 방식).
6. 뷰모델 받는 법
   - 플레이어 공유 뷰모델: 플레이어 리졸버로(5번).
   - 조상 위젯과 같은 위젯 전용 뷰모델: Context. Outer 사슬을 따라 찾으므로 부모 WBP에 배치된 위젯이나 부모가 소유자로 만든 항목 위젯에만 씁니다. 한 트리에 같은 클래스가 둘 이상일 수 있으면 이름을 맞춥니다.
   - 부모 뷰모델의 자식: PropertyPath 또는 부모 쪽 바인딩.
   - 목록: ListView·패널의 항목 뷰모델 확장.
   - 월드 공간 위젯: 위젯을 만든 쪽이 Manual로.
   - 키별 자식(스킬 슬롯·어트리뷰트·아이템): 리졸버가 부모 뷰모델에게 키로 요청합니다. 부모가 플레이어 공유 뷰모델이면 Global Collection에서 받습니다.
   - Global Collection 생성 방식으로 받는 소스의 자동 갱신(`Global Viewmodel Collection Update`, 엔진 기본값 꺼짐)은 등록보다 먼저 뜨거나 항목이 바뀌는 것을 반영해야 하는 소스에서만 켭니다.
   - 리졸버는 위젯이 뜰 때 한 번만 불리므로, 다시 초기화되는 부모(플레이어 Character)의 키별 자식을 리졸버로 받는 위젯은 부모가 다시 초기화될 때 Destruct → Construct를 거쳐야 합니다(풀 재사용 포함, MVVM View가 이때 소스를 다시 초기화함). 그렇지 않으면 자식을 PropertyPath로 받습니다. 지금 HUD는 빙의가 바뀌면 스택에서 빠졌다가 다시 들어가므로 앞의 경우입니다.
   - 비는 것이 정상 흐름이 아닌 소스는 선택적(optional)으로 두지 않습니다. Manual로 넣는 쪽은 실패를 로그로 남깁니다.
7. 리졸버는 위젯에 뷰모델을 배달만 합니다. 상태를 두지 않습니다(WBP 클래스가 공유). 배달할 뷰모델은 그 위젯용으로 새로 만들거나(이때 모델 연결·정리도 리졸버가 맡음) 주인(Global Collection, 부모 뷰모델)에게서 받아 옵니다. 모델을 열쇠로 뷰모델을 직접 찾지 않고, 자기가 만든 뷰모델만 정리합니다.
8. 공유: 여러 뷰가 같은 데이터를 보고 표시 상태가 어긋나면 안 되거나 다시 열어도 이어져야 할 때만 공유합니다(플레이어 Character, Inventory). 따로 만들어도 값이 어긋나지 않거나 공유하려면 열쇠 조회·제거 규칙이 생기면 따로 만듭니다(적·보스 Character). 같은 위젯 트리 안은 Context, 플레이어 단위나 앱 전체에 하나인 것은 Global Collection으로 나눕니다.
9. 위젯과 참조
   - 위젯에는 바인딩과 변환 함수만 둡니다. 입력은 뷰모델 명령(`Request~`)으로 보냅니다.
   - 도메인에 닿는 명령은 뷰모델이 네이티브 델리게이트로 내보내고, 그 뷰모델에 값을 넣는 조립 층이 받아 모델을 호출합니다(`UWxViewModel_Dialogue::OnAdvanceRequested`처럼).
   - 뷰모델은 모델을 약참조로 듭니다.
   - 키별 자식은 키 종류가 유한할 때만 쌓아 두고, 무한히 늘 수 있으면 제거 규칙을 둡니다.

알려진 예외(후속 일감)

- 자막: StateTree 노드가 뷰모델을 직접 호출해 1번에 어긋납니다. 자막 상태를 GameState 컴포넌트에 두는 안이 1순위이고, 퀘스트 멀티플레이 정책과 함께 봅니다.
- InteractionList 뷰모델: WxGame에 있고 도메인 타입을 써 2번에 어긋납니다. 뷰모델은 WxUI로, 값 넣기는 조립 층으로 옮깁니다.

**Q1~Q5 결정 (Q1~Q3 위임받은 판단, 2026-09-30 원칙 논의로 Q1·Q2·Q5 변경)**

- Q1: 뷰모델 클래스는 모두 WxUI에 둡니다(원칙 2). 처음에는 루트 VM을 WxGame에 두기로 했으나, 도메인 타입을 쓰는 뷰모델을 없애 모듈 경계를 지키기로 하면서 Inventory VM도 WxUI로 옮깁니다. 도메인 값은 PC가 넣습니다.
- Q2: 로컬 PC가 플레이어 공유 뷰모델을 만들어 엔진 Global Collection에 각자 등록합니다.
  - 처음 계획(PC Outer + 루트 리졸버)은 Outer가 GC에서 객체를 살려 두지 않아, HUD가 없는 틈(빙의 교체 뒤 비동기 푸시, 빙의 해제 상태)에 루트와 Inventory VM이 수거될 수 있었습니다. 수거되면 인벤토리 탭 유지(09-23 결정)가 깨집니다.
  - 로컬 플레이어 서브시스템·PC 컴포넌트 보관도 검토했으나, 엔진이 제공하는 Global Collection이 강한 참조와 위젯 연결을 함께 주므로 PC가 만들어 등록하는 쪽이 가장 단순합니다. `UMVVMGameSubsystem` 상속은 위젯 쪽 조회가 기본 클래스로 고정돼 있어 쓰지 않습니다.
- Q3: 적·보스의 Character VM은 그 뷰를 만드는 쪽이 만들어 넣습니다.
- Q5: 루트 클래스 `UWxViewModel_Player`는 만들지 않습니다. 공유 뷰모델을 Global Collection에 각자 등록하면 묶어 둘 부모가 필요 없어서입니다(woogle 2026-09-30 "B로 하죠").

**목표 구조**

- 로컬 PC가 `VM_PlayerCharacter`(Character VM)와 `VM_Inventory`(Inventory VM)를 만들어 Global Collection에 등록하고 값을 넣습니다.
- Character VM이 자기 AbilitySystem VM을 만들고 소유합니다. AbilitySystem VM은 슬롯·어트리뷰트·이펙트 VM을, Inventory VM은 아이템 VM을 소유합니다.
- 적 네임플레이트와 보스 바는 뷰마다 자기 Character VM을 갖습니다.
- 코드에서 `FindObjectWithOuter` 조회는 모두 사라집니다.

**코드 변경**

1. WxUI `UWxViewModel_Character`
   - `GetOrCreate`를 지우고 `Initialize(UAbilitySystemComponent*, FText)`로 바꿉니다. Initialize는 자기를 Outer로 AbilitySystem VM을 새로 만들어 `UPROPERTY`로 듭니다. `Deinitialize`는 AbilitySystem과 이름을 비우고 통지합니다.
   - 플레이어 공유본 조회 정적 함수와 `VM_PlayerCharacter` 이름 상수를 둡니다.
2. WxUI `UWxViewModel_AbilitySystem`: `GetOrCreate`를 지우고 `Initialize(ASC)`를 공개합니다(Character VM만 부름). 헤더의 "ASC 하나당 하나" 문구는 소유 관계에 맞게 고칩니다.
3. `UWxViewModel_Inventory`를 WxGame에서 WxUI로 옮깁니다.
   - 도메인 타입(`UWxItemDefinition`·`EWxItemCategory`·`UWxInventoryComponent`)을 빼고, 값을 받는 함수와 표시 로직(탭 필터링)만 남깁니다. 아이템 VM 키는 정의 객체를 `UObject*`로 받습니다.
   - 탭 키를 도메인을 모르는 키로 바꿉니다. 목록으로 할지 이름 있는 필드로 할지, 태그를 쓸지는 WBP_Inventory의 탭 연결을 본 뒤 질문으로 정합니다.
   - 조회 정적 함수와 `VM_Inventory` 이름 상수를 둡니다. `GetOrCreate`와 `UWxViewModelResolver_Inventory`는 지웁니다.
   - WBP 참조는 `DefaultEngine.ini` ClassRedirects로 잇고, WBP 리세이브 뒤 `BatchFiles/CheckRedirects.bat`로 지웁니다.
4. WxGame `AWxPlayerController`
   - `BeginPlay`에서 `Super` 앞, 로컬 PC일 때만: 두 VM을 Global Collection 객체를 Outer로 만들어 등록하고 참조를 듭니다. 지금 폰으로 Character VM을 초기화합니다.
   - `OnPossessedPawnChanged`: 새 폰으로 Character VM을 다시 초기화하고, 폰이 없으면 비웁니다.
   - 인벤토리 컴포넌트를 관찰해 Inventory VM에 값을 넣습니다(지금 Inventory VM에 있는 스택·인스턴스 변경 처리와 카테고리 판정을 옮겨 옴).
   - `EndPlay`에서 두 VM을 컬렉션에서 뺍니다.
5. Character VM 초기화와 이펙트 표시 연결(지금 `UWxViewModelResolver_AbilitySystem::GetOrCreate`의 람다)은 PC, 보스 리졸버, 네임플레이트 관리자가 함께 쓰므로 WxGame `Source/WxGame/MVVM/`의 함수 하나로 옮깁니다.
6. 리졸버
   - `UWxViewModelResolver_Ability`(WxGame): 플레이어 Character VM 조회 → AbilitySystem → `GetOrCreateAbilityViewModel`. 표시·바인드 조건 람다는 그대로 둡니다.
   - `UWxViewModelResolver_Item`: Inventory VM 조회 → `GetOrCreateItemViewModel`. Inventory VM과 함께 WxUI로 옮깁니다.
   - `UWxViewModelResolver_Attribute`(WxUI, 이동 없음): 플레이어 Character VM 조회 → AbilitySystem → `GetOrCreateAttributeViewModel`.
7. 삭제: `UWxViewModelResolver_AbilitySystem`, `UWxViewModelResolver_PlayerCharacter`, `UWxViewModelResolver_Inventory`. 쓰던 WBP는 아래 재연결에서 바꾸므로 리다이렉트를 두지 않습니다.
8. 다른 캐릭터(Q3)
   - `UWxNameplateManagerComponent`: 위젯마다 위젯을 Outer로 Character VM을 만들어 5번 함수로 초기화하고, 지금처럼 Manual로 넣습니다.
   - `UWxViewModelResolver_BossCharacter`: 위젯마다 만드는 구조는 그대로 두고, 초기화를 5번 함수로 합니다.
9. 주석은 바뀐 소유 관계만 한 줄씩 고칩니다. Quest·Dialogue·InteractionList·Subtitle은 손대지 않습니다(Q4).

**WBP 재연결**

- WBP_Nameplate_Player, WBP_PlayerSkills: Character VM을 Resolver에서 Global Collection `VM_PlayerCharacter`로 바꿉니다.
- WBP_AcquiredItemList, WBP_Inventory: Inventory VM을 Resolver에서 Global Collection `VM_Inventory`로 바꿉니다. WBP_Inventory의 탭 바인딩은 새 탭 키로 다시 연결합니다.
- 자동 갱신은 등록보다 먼저 뜰 수 있는 소스에서만 켭니다.
- 수정하지 않는 WBP: WBP_GameLayout, WBP_Ability, WBP_ItemQuickSlot, WBP_TotalGold, WBP_AcquiredItemEntry, WBP_StaminaBar, WBP_Nameplate_Enemy, WBP_Nameplate_Boss. 옮긴 클래스는 리다이렉트로 이어집니다.
- 에디터 작업 방법: unreal-mcp로 합니다. 생성 방식·Global Collection 이름·자동 갱신을 바꿀 수단이 없으면 `WxMVVMToolset`에 함수를 만듭니다(woogle 2026-09-30 "필요할 때 mcp 툴 만드세요"). 그래도 안 되면 구현 중에 질문 표로 알리고 새 계획으로 다시 승인받습니다.

**검증**

- 빌드: WxEditor Development. 리다이렉트 추가, WBP 리세이브, 리다이렉트 제거 뒤 각각 다시 빌드합니다.
- 헤드리스 임시 자동화 테스트(`Source/WxGame/Tests/`, LV_DevCombat -game -nullrhi). 결과 기록 뒤 지우고 다시 빌드합니다.
  - 등록: `VM_PlayerCharacter`·`VM_Inventory`가 있고 Outer가 컬렉션입니다. 리슨 서버 + 클라이언트에서 원격 PC는 등록하지 않고, 클라이언트는 자기 것을 등록합니다(`Super::BeginPlay` 앞의 로컬 판정 확인).
  - HUD 위젯이 등록된 VM과 같은 인스턴스를 받습니다(등록과 HUD 초기화 순서 확인). 인벤토리 화면을 열고 닫아도 같은 Inventory VM이고 카테고리가 유지됩니다.
  - HUD를 걷은 상태에서 GC를 강제로 돌린 뒤 HUD를 다시 띄워도 같은 VM이고 카테고리가 유지됩니다.
  - 슬롯·어트리뷰트·아이템 리졸버가 공유 VM 아래 VM을 돌려줍니다. 회피 쿨다운, SP 변화, 아이템 획득이 값에 반영됩니다.
  - 사망 → 부활 뒤 같은 Character VM이 새 폰 ASC로 다시 초기화되고, 새 HUD의 스킬 슬롯이 새 ASC를 봅니다.
  - 맵 이동 뒤 옛 PC의 VM은 빠지고 새 PC가 다시 등록합니다. 옛 월드 누수 경고가 없습니다.
  - 적 네임플레이트 VM은 그 적의 HP를, 보스 바는 교전 중인 보스를 보여 주고 교전이 끝나면 비워집니다.
- 코드 확인: grep으로 `FindObjectWithOuter`가 MVVM 코드에 남지 않았는지, WxUI 뷰모델에 도메인 타입 include가 없는지 확인합니다.

**테스트 체크리스트 초안**

| 항목 | 확인 방법 | 담당 |
| --- | --- | --- |
| 빌드 | WxEditor Development 빌드(리다이렉트 추가·리세이브·제거 뒤 각각) | AI |
| 등록·로컬 판정 | 임시 자동화 테스트: 두 VM 등록·Outer, 리슨 서버 + 클라이언트에서 원격 PC 미등록 | AI |
| 공유·탭 유지 | 임시 자동화 테스트: HUD 위젯과 인스턴스 일치, 인벤토리 재오픈 뒤 탭 유지 | AI |
| GC 틈 | 임시 자동화 테스트: HUD 걷은 뒤 GC 강제 → 재푸시 뒤 같은 VM·탭 유지 | AI |
| 리졸버 값 반영 | 임시 자동화 테스트: 쿨다운·SP·아이템 획득 | AI |
| 부활·맵 이동 | 임시 자동화 테스트: 사망 → 부활 뒤 재초기화·스킬 슬롯, 맵 이동 뒤 재등록·누수 없음 | AI |
| 네임플레이트·보스 바 값 | 임시 자동화 테스트 | AI |
| 코드 리뷰 | 변경 파일과 볼 점: PC의 등록 시점·로컬 판정·재초기화·인벤토리 값 넣기, Inventory VM의 WxUI 이동과 탭 키, Character VM의 AbilitySystem 소유, 리졸버의 조회 경로, WBP 생성 방식 변경과 필요한 소스의 자동 갱신 | 사람 |
| HUD·인벤토리 표시 | 게임: HUD 체력·스킬·스태미나를 보고, 인벤토리를 열어 탭을 바꾼 뒤 다시 엽니다. 퀵슬롯, 적 네임플레이트, 보스 바, 사망 → 부활도 봅니다. 전과 같이 보이고 값이 따라가야 합니다. | 사람 |

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 빌드 | WxEditor DebugGame·Development 빌드(리다이렉트 추가 뒤, 임시 테스트 삭제 뒤) | AI | 통과 | 플레이어 리졸버 반영 뒤 다시 확인: 두 구성 모두 Result: Succeeded. 임시 테스트와 WxEditor.Build.cs 임시 의존을 지운 상태로 다시 빌드 |
| 위젯 BP 컴파일·리다이렉트 | 헤드리스 CompileAllBlueprints(-BlueprintBaseClass=WidgetBlueprint), BatchFiles/CheckRedirects.bat | AI | 통과 | 플레이어 리졸버 반영 뒤 다시 확인: 위젯 BP 88개 성공, 0 error·0 warning. Inventory VM 이동 리다이렉트는 WBP 2개 리세이브 뒤 SAFE 판정으로 제거. 지운 클래스(PlayerCharacter·Inventory 리졸버)를 참조하는 에셋 0건 |
| 등록·로컬 판정 | 헤드리스 에디터 PIE 임시 테스트 Wx.Temp.PlayerViewModel.Net(리슨 서버 + 원격 클라이언트, LV_DevCombat) | AI | 통과 | 플레이어 리졸버 반영 뒤 같은 테스트 다시 통과(클라이언트 AcquiredItemList 인스턴스 일치 추가). 호스트·클라이언트가 각자 자기 게임 인스턴스에 VM_PlayerCharacter·VM_Inventory 등록(서로 다른 인스턴스, PC가 든 것과 같음). 서버의 원격 PC 1개는 두 VM 모두 없음. 각 HUD가 자기 VM을 받고 클라이언트 HP VM이 클라이언트 폰 ASC 값과 같음. 클라이언트 인벤토리 슬롯 VM 1개 = 컴포넌트 1개 |
| 공유·탭 유지 | 임시 테스트 Wx.Temp.PlayerViewModel.Standalone(단독 PIE) | AI | 통과 | 플레이어 리졸버 반영 뒤 이 항목부터 네임플레이트·보스 바까지 같은 테스트 다시 통과. 두 VM의 Outer가 컬렉션. Nameplate_Player·PlayerSkills·AcquiredItemList가 등록본을 받음. 인벤토리 화면 생성 → 등록본, 소모품 탭 3개 모두 소모품, 화면 제거·GC 뒤 재생성 → 같은 VM·탭 유지 |
| GC 틈 | 같은 테스트: 빙의 해제로 HUD를 걷은 상태에서 GC 강제 | AI | 통과 | 두 VM 생존·등록 유지, 탭 유지, Character VM은 비워짐 |
| 리졸버 값 반영 | 같은 테스트 | AI | 통과 | 슬롯 4개·UP 어트리뷰트 VM의 Outer가 플레이어 AbilitySystem VM, TotalGold 아이템 VM의 Outer가 Inventory VM. UP 7 → VM 7.0. 회피 발동 → 충전 2→1·쿨다운 1.30초. 포션 획득 → LastAcquiredItem(합쳐진 스택 때문에 통지가 1개씩 두 번이라 AcquiredCount 1), 슬롯 VM 1→3 |
| 부활·맵 이동 | 같은 테스트: UnPossess → 옛 폰 파괴 → GameMode RestartPlayer, 이어서 OpenLevel(LV_DevCombat) | AI | 통과 | 부활: 같은 Character VM, 새 AbilitySystem VM, 이름=새 폰, 새 폰 HP 33 반영, 새 HUD의 회피 슬롯이 새 AbilitySystem VM 아래. 사망 어빌리티가 아니라 빙의 교체 경로로 재현했다. 맵 이동: 새 PC가 새 두 VM 등록, 새 HUD가 새 VM, GC 뒤 옛 월드·옛 두 VM 해제 |
| 네임플레이트·보스 바 값 | 같은 테스트: 샌드백 스폰 → Character.Boss·State.Engaged 태그 + 교전 통지, 종료 | AI | 통과 | 보스 바 VM=샌드백 이름·자기 AbilitySystem·위젯 소유. 네임플레이트 위젯 1개의 VM=샌드백·위젯 소유, 적 HP 12 반영. 교전 종료 → 보스 바 VM 비워짐 |
| 코드 확인 | grep | AI | 통과 | Source·Plugins에 FindObjectWithOuter 0건, WxUI MVVM에 도메인(Items·Inventory·Character·Battle·Quest) include 0건, 옛 공유 문구 0건 |
| 코드 리뷰 | 변경 파일: WxPlayerController(등록 시점·로컬 판정·SetPawn 재초기화·인벤토리 값 넣기), WxViewModel_Character·AbilitySystem·Inventory(WxUI로 이동)·Item·Attribute·WxViewModelUtils, WxGameViewModelUtils, 리졸버 Ability·Item(신규 파일)·BossCharacter, WxNameplateManagerComponent, WxGameplayTags(Item.Category.*), WxInventoryComponent(전역 준비·종료 통지 삭제). 신규 WxViewModelResolver_Player(WxUI). 에셋: WBP_Nameplate_Player·WBP_PlayerSkills의 VM_PlayerCharacter와 WBP_AcquiredItemList·WBP_Inventory의 Inventory VM을 옛 리졸버→플레이어 리졸버(플레이어 캐릭터 소스는 선택적), WBP_Inventory 탭 버튼 그래프를 enum→Item.Category 태그로. 볼 점: 원칙 4·6번과 맞는지, PC에 모인 값 넣기 코드의 크기 | 사람 | 통과 | woogle 2026-09-29 |
| HUD·인벤토리 표시 | 게임: HUD 체력·스킬·스태미나를 보고, 인벤토리를 열어 탭을 바꾼 뒤 닫았다 다시 엽니다. 퀵슬롯·골드, 아이템 획득 알림, 적 네임플레이트, 보스 바, 사망 → 부활도 봅니다. 전과 같이 보이고 값이 따라가야 합니다. | 사람 | 통과 | woogle 2026-09-29 |

## 구현 진행 · 2026-09-30

- 계획 보정
  - Item 리졸버는 도메인 키(아이템 정의)를 쓰는 조립 층이라 WxGame에 둔다(새 파일 `WxViewModelResolver_Item`). 아이템 VM을 정의로 채우는 일은 PC가 맡고, 리졸버는 PC에게 받는다.
  - 인벤토리 컴포넌트의 전역 준비·종료 통지(`OnAnyInventoryReady`·`OnAnyInventoryEnded`)는 옛 Inventory VM만 썼다. PC가 자기 컴포넌트에 `Super::BeginPlay` 뒤 직접 붙으므로 지웠다.
  - 빙의 해제 때 PC가 Character VM을 비우는 시점이 HUD를 걷기 전이라, `VM_PlayerCharacter.AbilitySystem` 경로가 빈 값으로 평가되며 MVVM 오류 로그가 났다(실제 부활도 옛 폰 빙의 해제를 거침). 빙의 해제로 비는 것은 정상 흐름이라 원칙 6번대로 WBP_Nameplate_Player·WBP_PlayerSkills의 `VM_PlayerCharacter`를 선택적으로 두었고, 다시 돌린 테스트에서 오류가 사라졌다.
  - 빌드는 에디터 구성에 맞춰 DebugGame으로 하고 마지막에 Development도 확인했다. 헤드리스 테스트는 리슨 서버 + 클라이언트까지 한 프로세스에서 보려고 `-game` 대신 헤드리스 에디터 PIE로 돌렸다.
- 코드 리뷰 반영(2026-09-30): WBP 4개의 플레이어 공유 VM을 이름 문자열 대신 `UWxViewModelResolver_Player`로 받게 바꾸고 원칙 5·6번 문장을 고쳤다. MCP로 WBP_PlayerSkills를 컴파일할 때 InputData_KMB가 컴파일 중에 처음 로드되며 엔진 ensure(BlueprintCompilationManager.cpp:331)가 한 번 났지만 컴파일 오류는 없었다.
- `InitializeCharacter`를 `UWxViewModel_Character`에 두지 않는 이유: `AWxCharacterBase`(WxGame)·`UWxEffectComponent_UIData`(WxCombat)를 알아야 해서 WxUI에 두면 순환 의존이거나 원칙 2번 위반이다. 도메인을 모르는 부분(`Initialize(ASC, 이름)`)만 VM에 있다.
- 이번 작업과 무관한 기존 경고: AcquiredItemList가 첫 초기화 때 null 항목을 ListView에 넣는 "Cannot add null item into ListView" 경고(LastAcquiredItem이 처음엔 null이라는 기존 주석대로), ST_Quest_Main1의 없는 태그 Quest.Fail.
## 배경 · 2026-09-29

착수할 때 현재 코드와 다시 대조한다. 아래는 검토 당시(viewmodel-quality-cleanup 완료 시점)의 사실이다.

### MVVM 원칙과 목표 구조

- 원칙: 부모 VM이 자식 VM을 만들고 소유한다. 뷰는 VM을 스스로 찾지 않고 부모 뷰나 부모 VM에게서 받는다(WPF의 DataContext 상속). 앱 수준 저장소는 최상위 루트 하나를 얻는 데만 쓴다.
- 이미 원칙대로인 부분: AbilitySystem VM이 슬롯·어트리뷰트·이펙트 VM을 키로 소유한다. Inventory VM은 아이템 VM을, Quest VM은 목표 VM을 소유한다.
- 벗어난 부분: AbilitySystem·Character·Inventory VM을 소스별로 찾아서 공유한다(로케이터 방식, 지금은 Outer 키 + `WxViewModel::GetOrCreate<T>`). 또 위젯마다 리졸버로 VM을 얻는다.
- 검토 당시 스케치
  - 플레이어 루트 VM 하나를 엔진 Global Collection에 둔다(자막과 같은 방식).
  - 루트는 플레이어 Character VM, Inventory VM, 다른 캐릭터의 Character VM(키: 캐릭터)을 소유한다.
  - Character VM이 자기 AbilitySystem VM을 소유한다(지금은 AbilitySystem VM이 Character VM의 공유 키다).
  - HUD는 루트만 받고, 자식 위젯은 Context나 PropertyPath로 받는다. 태그 인자가 필요한 스킬 슬롯만 리졸버가 부모 VM에게 요청한다.
  - 네임플레이트 관리자(WxGame 연결 코드)는 루트에게 대상의 Character VM을 요청한다.

### 선결·결정할 것

- 새 VM 클래스(루트)가 필요하다. 신규 VM 클래스는 이유를 먼저 설명하고 동의를 받는다.
- Inventory VM이 WxGame에 있어, WxUI의 루트가 타입으로 들 수 없다. Inventory VM을 WxUI로 옮기는 문제(`UWxItemDefinition`·`EWxItemCategory` 의존)를 먼저 풀어야 한다.
- 루트가 다른 캐릭터 VM들의 수명을 어떻게 관리할지 정해야 한다(약참조, 또는 사망 시 제거). 부활·맵 이동 때 플레이어 VM을 교체하는 방식도 정해야 한다.
- 여러 WBP의 VM 연결을 Context·PropertyPath로 다시 만든다(에디터 작업). 대상: HUD(WBP_GameLayout), WBP_PlayerSkills, WBP_StaminaBar, WBP_Ability, 네임플레이트 3종, 인벤토리·퀵슬롯·토스트·골드 위젯 등.
- 기존 규칙과 대조한다: VM은 WxUI·연결은 WxGame 리졸버·모델은 VM을 모름, UIManager는 개별 VM을 모름, 전용 서브시스템 금지, 인벤토리를 다시 열어도 탭 유지(2026-09-23 결정).

### 검토한 엔진 사실 (UE 5.8 소스)

- 뷰모델 생성 방식: Manual, CreateInstance, GlobalViewModelCollection, PropertyPath, Resolver, Context(5.8 신규, 프로젝트 설정 기본 허용).
- Context: 자식 위젯이 `GetTypedOuter<UUserWidget>()` 사슬을 거슬러 올라가며, 조상 위젯의 MVVM 뷰에서 같은 클래스(이름을 지정했으면 같은 이름)의 VM을 찾는다(`MVVMViewClass.cpp:214`). 위젯 트리 밖(월드 공간 네임플레이트, 다른 레이어에 뜬 화면)과는 공유하지 않는다.
- `UContextDataWidgetExtension`(위젯에 임의 데이터를 붙이는 확장)은 API 매크로가 없어 모듈 밖으로 export되지 않는다.
- Global Collection: 키는 (클래스, 이름)이고 이름은 WBP에서 설계 시점에 고정된다. 게임 인스턴스 수명 동안 강한 참조로 들고 있으며 `RemoveViewModel`로 직접 뺀다. 항목이 바뀌면 이 방식으로 바인딩된 뷰가 새 VM으로 갈아탄다(`UMVVMView::HandleViewModelCollectionChanged`).
- 로컬 플레이어 단위 컬렉션은 없다.

### 검토 뒤 채택하지 않은 대안

- 전용 VM 서브시스템(월드 서브시스템 + (소스, 클래스) → VM 약참조 맵): 로케이터를 명시적으로 만들 뿐 MVVM 원칙에 더 가까워지지 않는다. 이 재설계로 갈 거라면 거쳐 갈 필요가 없다.
- 정적 약참조 레지스트리: 엔진이 이미 들고 있는 관계를 한 번 더 저장한다. 템플릿 정적 변수는 모듈마다 따로 생길 위험이 있다.
- `FUObjectAnnotationSparse`: 엔진 주석상 "희소·느림·임시·에디터 전용 외부 데이터"용이고 GC를 모른다.
- 모든 VM을 Global Collection으로: 적 네임플레이트, 태그별 스킬 슬롯, 정의별 퀵슬롯 아이템처럼 인스턴스마다 다른 VM이 필요한 곳은 이름이 고정이라 불가능하다.

## 조사 · 2026-09-30

배경과 달라진 점과 착수 시점 사실만 적는다.

- 공용 `WxViewModel::GetOrCreate<T>` 템플릿은 없어졌고, AbilitySystem·Character·Inventory VM이 각자 `GetOrCreate`에서 `FindObjectWithOuter`로 공유본을 찾는다.
- 공유 조회 호출처: 리졸버 Ability·AbilitySystem·PlayerCharacter·BossCharacter·Inventory·Item(WxGame), Attribute(WxUI), `UWxNameplateManagerComponent`(적 네임플레이트, Manual 푸시).
- `UWxViewModelResolver_AbilitySystem`은 어떤 WBP도 쓰지 않고 정적 `GetOrCreate`(이펙트 표시 연결)만 쓰인다.
- 보스 바는 이미 위젯마다 Character VM을 만들고 `UWxBattleSubsystem::OnCurrentBossChanged`로 채운다(메모의 GameState 슬롯 방식은 지난 구조).
- WBP 사용처(에셋 문자열 검색): PlayerCharacter 리졸버 = Nameplate_Player·PlayerSkills(둘 다 WBP_GameLayout 안), Inventory 리졸버 = Inventory·AcquiredItemList, Item 리졸버 = AcquiredItemEntry·ItemQuickSlot·TotalGold, Ability 리졸버 = Ability·ItemQuickSlot·PlayerSkills, Attribute 리졸버 = PlayerSkills·StaminaBar. WBP_StaminaBar는 BP_Template·BP_HGTest의 위젯 컴포넌트(월드 공간)다.
- `UWxPlayerLayoutComponent`가 빙의가 바뀔 때마다 HUD를 걷고 다시 푸시한다. 그래서 부활 때 HUD 위젯은 새로 만들어진다.
- UE 5.8: 생성 방식 enum은 Manual·CreateInstance·GlobalViewModelCollection·PropertyPath·Resolver·Context(`MVVMBlueprintViewModelContext.h:19`). PropertyPath 소스는 부모 필드 변경 때 `UMVVMView::EvaluateSource`로 다시 평가된다(`MVVMView.cpp:719`).
- unreal-mcp는 이번 세션에서 연결되지 않아(ECONNREFUSED) WBP 내부 설정은 직접 보지 못했다.


## AI 조사 결과 · 2026-09-29T15:33:20.993Z

<!-- test-feedback:request-d4717743-4364-457e-b126-ab68e3d142c6:1 -->
- 전달한 사람: woogle
- 처리 AI: Claude Code
- 처리 결과: 구현 승인 필요

답변:

> Q1: 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요.
> Q2: 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요.
> Q3: 가장 MVVM 디자인 패턴을 잘 지키는 방법을 찾아서 제안해주세요.
> Q4: 공유 VM만
> Q5: 만든다

AI 요약:

> 추가 질문은 없습니다. Q1~Q3은 맡겨 주신 대로 MVVM 원칙에 가장 가까운 쪽으로 정했습니다. 원칙은 세 가지입니다. 부모 VM이 자식 VM을 만들어 소유하고, 뷰는 VM을 스스로 찾지 않고 자기를 만든 쪽이나 부모에게서 받으며, 앱 수준 조회는 루트 하나만 둡니다. 이에 따라 루트 VM은 WxGame에 두고(Q1), PC를 Outer로 해서 리졸버 한 곳에서만 얻고(Q2), 적·보스 VM은 루트가 들지 않고 그 뷰를 만드는 쪽이 만들어 넣습니다(Q3). 범위는 공유 VM만이고(Q4), UWxViewModel_Player는 새로 만듭니다(Q5). 현재 코드를 다시 대조해 보니 공유 조회(FindObjectWithOuter)는 AbilitySystem·Character·Inventory VM 세 곳에 있습니다. 호출처도 기록에 적힌 리졸버 7개와 네임플레이트 관리자로 같습니다. 구현을 승인해 주시면 아래 계획대로 진행합니다.

> 근거: 읽음: .agents/workflow/tasks/viewmodel-mvvm-redesign.md (Q1~Q5 답변 전 상태, 기존 계획·조사 2026-09-30)

> 근거: 읽음: .agents/workflow/process/index.md (정하기 규칙: 더 물을 것이 없으면 계획과 승인 요청)

> 근거: Grep 'GetOrCreate\(|FindObjectWithOuter' Source·Plugins: FindObjectWithOuter는 WxViewModel_AbilitySystem.cpp:20, WxViewModel_Character.cpp:15, WxViewModel_Inventory.cpp:22 세 곳. 호출처는 Resolver_PlayerCharacter/BossCharacter/AbilitySystem/Ability, Inventory·Item 리졸버(WxViewModel_Inventory.cpp:253,258), WxViewModel_Attribute.cpp:73, WxNameplateManagerComponent.cpp:127. Subtitle은 따로(Q4 범위 밖).

> 근거: 읽음: Plugins/WxUI/.../WxViewModel_Character.h — GetOrCreate(AbilitySystem VM, 이름), AbilitySystem VM을 Outer로 공유

> 근거: 읽음: Plugins/WxUI/.../WxViewModel_AbilitySystem.h — GetOrCreate(ASC)와 private Initialize, 자식 VM은 GetOrCreateAttributeViewModel·GetOrCreateAbilityViewModel로 소유

> 근거: 읽음: Source/WxGame/MVVM/WxViewModel_Inventory.h — WxInventory 타입(UWxItemDefinition·EWxItemCategory) 의존, Resolver_Inventory·Resolver_Item이 같은 헤더에 있음

> 근거: 읽음: WxViewModelResolver_AbilitySystem.cpp — 정적 GetOrCreate가 이펙트 표시 연결 람다(UWxEffectComponent_UIData)를 담당

> 근거: 읽음: WxViewModelResolver_PlayerCharacter.cpp, WxViewModelResolver_BossCharacter.cpp(위젯마다 Character VM을 만들고 OnCurrentBossChanged로 Initialize/Deinitialize), WxViewModelResolver_Ability.cpp

> 근거: 읽음: WxViewModel_Attribute.cpp:61-76 — WxUI의 Attribute 리졸버가 PC→폰→ASC로 직접 조회

> 근거: 읽음: WxNameplateManagerComponent.cpp:100-144 — 적 네임플레이트에 공유 Character VM을 SetViewModelByClass(Manual)로 넣음

> 근거: 읽음: Plugins/WxUI/.../WxPlayerLayoutComponent.h — HUD를 띄우는 쪽이 WxUI에 있어 WxGame 루트 타입을 들 수 없음(Q2 판단 근거)

> 근거: Grep WxUI.Build.cs: WxUI는 Wx 모듈 중 WxCore만 의존(Q1 판단 근거)


## 사용자 테스트 결과 · 2026-09-29T18:20:17.324Z

<!-- test-feedback:request-a0d40851-e4b8-4e3b-a415-2ee81117de0f:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
> 통과 · HUD·인벤토리 표시
