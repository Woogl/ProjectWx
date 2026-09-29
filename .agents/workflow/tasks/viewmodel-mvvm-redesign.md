# MVVM 원칙에 맞춘 뷰모델 재설계

상태: 확인 대기 · 착수 대기
다음 행동: 착수를 지시하면 AI가 조사해 질문과 구현 계획을 돌려준다.

- woogle 결정(2026-09-29): "아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다." 앞선 일감(viewmodel-quality-cleanup.md)의 코드 리뷰 중 공유 VM 조회 방식을 검토하다 나왔다.

## 요청

- 요청 · woogle 2026-09-29

> 아까 얘기했던 MVVM 원칙에 맞게 뷰모델 재설계하는 것은 새 일감으로 만들고, 일단 이 일감은 완료 처리합시다.

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
