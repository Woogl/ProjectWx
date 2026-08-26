# WxCore — 코드 리뷰

> 선언과 계약만 담은 얇은 파운데이션이고 로직이 거의 없어 전반적으로 매우 건강하다. CLAUDE.md 코딩·모듈 규칙 위반은 한 건도 없다. 이번 리뷰는 `Plugins/WxCore/Source/WxCore` 의 11개 소스 전부를 통독했고, 발견의 사실 확인을 위해 소비 측(`Source/WxEditor`, `Plugins/Wx*`)의 호출 지점과 UE 5.8 엔진 구현(`AActor::GetActorLabel`, `FUniversalObjectLocator`)까지 함께 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `GetDisplayName` 이 표시 조회만으로 대상 액터를 변형한다

- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp:19`
- **범주**: 버그/정확성
- **문제**: `AActor::GetActorLabel()` 은 기본 인자가 `bCreateIfNone = true` 라, 라벨이 비어 있는 액터를 만나면 `const_cast` 로 `ActorLabel`(에디터 전용이지만 직렬화되는 프로퍼티)을 채워 넣고 `FCoreDelegates::OnActorLabelChanged` 를 브로드캐스트한다(엔진 `Engine/Source/Runtime/Engine/Private/ActorEditor.cpp:1263`~`1285`). 즉 이름상 순수 조회인 이 헬퍼가 지목 대상 액터의 상태를 바꾸고 아웃라이너 갱신까지 유발한다.
  호출 지점이 문제를 키운다. `Source/WxEditor/WxActorLocatorCustomization.cpp:143` 은 Slate 텍스트 게터라 디테일 패널에 로케이터 행이 떠 있는 동안 반복 호출되고, 나머지 6곳은 StateTree 노드의 `GetDescription` 이라 그래프가 그려질 때마다 돈다. 결과적으로 "액터를 로케이터로 지목한 뒤 디테일 패널을 열었을 뿐인데 그 액터에 라벨이 새로 붙어 있다"가 성립한다. 같은 경로에서 매번 도는 `Locator.SyncFind()`(`WxLocatorUtils.cpp:17`)도 프레임마다 `StaticFindObject` 를 태우지만, 그쪽은 로드는 하지 않으므로 비용 자체는 작다.
- **제안**: `Actor->GetActorLabel()` 을 `Actor->GetActorNameOrLabel()` 로 바꾼다(엔진 `GameFramework/Actor.h:1241`). 라벨이 있으면 라벨을, 없으면 `GetName()` 을 돌려주며 부작용이 없고 아웃라이너 표기와도 사실상 일치한다.
- **확신도**: 높음

### 2. 🟡 `Device.*` 15개는 코드가 한 번도 읽지 않는 WxWorld 저작 어휘다

- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:98`~`121`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:41`~`59`
- **범주**: 설계/구조
- **문제**: `Device.Elevator.1F` / `Device.Elevator.2F` / `Device.Piston.On` / `Device.TreasureChest.*` 등은 WxWorld 장치 StateTree 에셋이 상태값으로만 쓰는 순수 저작 어휘다. 저장소 전체를 훑어도 WxCore 밖 C++ 참조가 0건이고, 헤더 주석(`WxGameplayTags.h:101`)도 "코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다"라고 자인한다. "WxCore 는 공용 계약만" 방침과 맞지 않으며, 장치 종류가 늘 때마다 WxCore 헤더가 함께 자라고 그때마다 이 헤더에 의존하는 모든 모듈이 재빌드된다.
- **제안**: 다만 프로젝트에는 "모든 태그는 `WxGameplayTags.h/.cpp` 쌍으로만"이라는 상위 규칙이 있고 지금 구조는 그 규칙의 직접적 산물이므로, 태그 허브 자체를 흔드는 변경은 하지 않는다. 검토할 여지가 있는 선은 하나뿐이다 — **C++ 이 한 번도 참조하지 않는 순수 저작 태그(`Device.*`)만** 소비 플러그인(WxWorld)의 네이티브 태그 파일로 내려, 태그는 여전히 C++ 로 선언하되 어휘의 소유권을 도메인에 두는 것. 현행 유지를 택한다면 헤더에 "이 예외는 태그 허브 규칙 우선의 결과"임을 한 줄로 못박아 다음 리뷰에서 반복 제기되지 않게 하는 편이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `UniversalObjectLocator` 가 에디터 전용 코드용인데 Public 런타임 의존이다

- **위치**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs:17`
- **범주**: 설계/구조
- **문제**: WxCore 에서 UOL 을 쓰는 코드는 `FWxLocatorUtils` 하나뿐이고 선언·정의 전체가 `#if WITH_EDITOR` 로 묶여 있다(`Public/WxLocatorUtils.h:12`~`18`, `Private/WxLocatorUtils.cpp:5`~`55`). 헤더는 `FUniversalObjectLocator` 를 전방 선언만 하고, 소비 측(`WxWorld`·`WxQuest`·`WxUI`·`WxDialogue`·`WxEditor`)은 모두 자기 `.Build.cs` 에 UOL 을 직접 선언하고 있어 전이 의존에 기대는 곳도 없다. 결국 Shipping 런타임이 쓰지 않는 모듈을 링크한다.
- **제안**: `if (Target.bBuildEditor)` 로 감싸 `PrivateDependencyModuleNames` 로 내린다. Public 이어야 할 이유가 헤더에 없다.
- **확신도**: 중간

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **확인했으나 문제 없던 것(오탐 방지용 기록)**:
  - `ECC_WxAttack = ECC_GameTraceChannel1` 은 `Config/DefaultEngine.ini:39` 의 `Name="WxAttack"` 등록과 일치하며, 프로젝트 전체에서 raw `ECC_GameTraceChannel*` 을 직접 쓰는 코드는 이 상수 하나뿐이다. 헤더 주석의 "메시 Overlap / 캡슐 Ignore" 서술도 `Source/WxGame/Character/WxCharacterBase.cpp:25`,`:29` 실제 코드와 일치한다.
  - `WxCollisionChannels.h:15` 의 `inline constexpr` 은 함수가 아니라 변수이므로 "인라인 함수 정의 금지" 규칙 대상이 아니다.
  - `FWxCoreModule` 의 빈 `StartupModule`/`ShutdownModule` 과 Public 폴더 배치는 전 플러그인 공통 관례라 WxCore 단독 문제가 아니다.
  - `FWxLocatorUtils` 호출 7곳 전부 `#if WITH_EDITOR` 안에 있어 비에디터 타겟 컴파일이 깨지지 않는다.
  - `IWxInteractable`/`IWxSavable` 구현체 5개는 모두 액터이며(문서화된 "액터만 구현" 계약 준수), 소비 측 `Cast<>` 결과를 전부 널 체크한다.
  - `Locator.SyncFind()` 는 로드를 수행하지 않는 find 계열이라 동기 패키지 로드 히치 위험은 없다.
- **미검토 / 한계**: 네이티브 태그 102개가 실제로 BP/에셋(`.uasset`)에서 소비되는지는 3.9GB 콘텐츠 바이너리 스캔이 필요해 확인하지 않았다. 따라서 "죽은 태그" 판정은 하지 않았고, 발견 2도 어휘가 죽었다는 주장이 아니라 소유권 위치에 대한 지적이다.

---
*문서 기준 커밋 `8a9bb7e6` · 리뷰일 2026-08-26 · 소스 11파일 — `/module-review`로 갱신*
