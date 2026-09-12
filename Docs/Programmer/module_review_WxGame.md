# WxGame — 코드 리뷰

> 조립 모듈로서 경계가 잘 지켜져 있고, 수명·권위·정리 경로 대부분이 의도적으로 설계되어 있다. 이번에도 치명적 결함은 확인하지 못했으며, 남은 항목은 직전 네임플레이트/`IWxUIData` 재구성이 남긴 잔재와 이전 리뷰에서 아직 손대지 않은 건들이다. 프레임워크 골격(GameMode·Controller·Character)·프론트엔드 흐름·MVVM 뷰모델 전부·어빌리티/아이템사용/MetaHuman 조립 경로의 cpp까지 내려가 읽었고, 기계적 규칙(Copyright·인라인·람다·`Handle` 접두사·`BlueprintCallable`)은 모듈 전체를 일괄 검사했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 `IWxUIData` 이관 후 남은 캐릭터 표시 접근자 두 개가 호출자 없는 데드 코드다

- **위치**: `Source/WxGame/Character/WxCharacterBase.h:60`, `Source/WxGame/Character/WxCharacterBase.h:61`, `Source/WxGame/Character/WxCharacterBase.cpp:147`, `Source/WxGame/Character/WxCharacterBase.cpp:152`
- **범주**: 중복/복잡도
- **문제**: 네임플레이트 표시 데이터를 `IWxUIData` 기반 뷰모델로 옮기면서 `GetTitle()`/`GetIcon()`(`WxCharacterBase.cpp:157`·`:167`)이 `CharacterName`·`Portrait`를 읽는 정식 통로가 됐는데, 옛 통로였던 `GetCharacterName()`·`GetPortrait()`가 그대로 남아 같은 두 필드를 두 번째 이름으로 노출한다. 저장소 전체에서 이 둘을 부르는 코드는 없고(선언·정의가 전부), `UFUNCTION`이 아니라 BP에서도 부를 수 없다. 같은 데이터에 접근 경로가 둘이면 다음 사람이 어느 쪽이 계약인지 판단할 근거가 사라진다.
- **제안**: 두 접근자를 선언·정의에서 함께 지운다. 삭제된 `UWxViewModelResolver_PlayerCharacter`가 남긴 끊긴 참조는 이번에 확인한 결과 없다(`Config/DefaultEngine.ini:117`의 `ClassRedirects`로 WxUI 쪽 새 클래스에 연결돼 있고, `Source/WxGame/README.md`의 안내 링크도 새 경로를 가리킨다).
- **확신도**: 높음

### 2. 🟡 프론트엔드 선택지가 C++ 버튼 이름·고정 인덱스에 묶여 있다

- **위치**: `Source/WxGame/FrontEnd/WxFrontEndWidget.cpp:20`, `Source/WxGame/FrontEnd/WxFrontEndWidget.cpp:21`, `Source/WxGame/FrontEnd/WxFrontEndWidget.h:38`, `Source/WxGame/FrontEnd/WxFrontEndWidget.h:40`
- **범주**: 설계/구조
- **문제**: 클래스 주석(`WxFrontEndWidget.h:15`)은 "선택 데이터와 디자인은 WBP에 두고"라고 선언하지만, 실제로는 버튼 하나하나가 `BindWidget` 필수 프로퍼티로 C++에 박혀 있고 `CharacterOptions`/`DestinationOptions` 배열의 인덱스 `0`/`1`이 바인딩 시점에 리터럴로 고정된다. 캐릭터나 목적지를 하나 늘리려면 WBP만이 아니라 C++ 헤더·생성자 양쪽을 고쳐야 하므로 주석이 말하는 데이터 주도 구조가 성립하지 않는다. 더구나 `BP_HGTest`라는 테스트 에셋 이름이 필수 바인딩으로 남아 있어, 그 이름의 버튼이 없는 WBP는 바인딩 실패로 막힌다. `CharacterOptions`에 항목이 두 개 미만이면 `HandleSelectCharacter`의 인덱스 검사에 걸려 버튼이 조용히 무동작이 된다.
- **제안**: 버튼 목록을 `UListView`(또는 동적 생성 엔트리)로 돌려 옵션 배열이 버튼 수를 정하게 하거나, 최소한 `BP_HGTest`를 `BindWidgetOptional`로 낮추고 인덱스 리터럴을 엔트리 위젯이 들고 오는 값으로 바꾼다.
- **확신도**: 높음

### 3. 🟡 InventoryItem 뷰모델의 진입점 세 개가 같은 구독 코드를 각각 복제한다

- **위치**: `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:12`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:23`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp:34`
- **범주**: 중복/복잡도
- **문제**: `StartObserving`·`Initialize(Instance)`·`Initialize(ItemDef)` 세 함수가 `Deinitialize` → 타깃 설정 → `ApplyStaticDataFromDef` → `OnAnyInventoryReady`/`OnAnyInventoryEnded` 구독 → `BindSource` 순서를 글자 그대로 세 번 반복한다. 특히 구독 두 줄(`:18`–`:19`, `:29`–`:30`, `:39`–`:40`)은 완전히 동일하다. 전역 신호가 하나 늘거나 구독 조건이 바뀔 때 세 곳을 모두 고쳐야 하고 한 곳을 빠뜨려도 컴파일은 통과하므로, 특정 초기화 경로만 신호를 놓치는 형태로 조용히 깨진다.
- **제안**: 구조 분리 없이 인플레이스로, 같은 클래스 안의 private 헬퍼 하나(관찰 시작 + `BindSource`)에 세 진입점이 모두 들어가게 모은다. `WxViewModel_Inventory.cpp:12`에 있는 같은 패턴의 한 벌은 클래스가 달라 이번 범위 밖으로 둔다.
- **확신도**: 높음

### 4. 🟢 플레이어 입력 콜백 다섯 개가 `Handle` 접두사 규칙을 어긴다

- **위치**: `Source/WxGame/Character/WxPlayerCharacter.h:53`–`:58`, `Source/WxGame/Character/WxPlayerCharacter.cpp:110`, `Source/WxGame/Character/WxPlayerCharacter.cpp:114`, `Source/WxGame/Character/WxPlayerCharacter.cpp:123`, `Source/WxGame/Character/WxPlayerCharacter.cpp:128`, `Source/WxGame/Character/WxPlayerCharacter.cpp:129`
- **범주**: 규칙 위반
- **문제**: `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased`는 `BindAction`으로 델리게이트에 물리는 자체 콜백인데 `Handle` 접두사 규칙을 따르지 않는다. 모듈 전체를 훑어본 결과 델리게이트 콜백 중 규칙을 벗어난 것은 이 다섯뿐이다(`HandleRagdollTagChanged`·`HandleOwnerDeath`·`HandleFrontEndChanged`·`HandleUseItemEvent` 등은 모두 준수). 엔진 오버라이드인 `Jump`와 엔진 함수 `ACharacter::StopJumping`은 대상이 아니다.
- **제안**: 선언·정의·바인딩 이름을 `Handle` 접두사로 맞춘다.
- **확신도**: 높음

### 5. 🟢 MetaHuman 해제가 리더 메시의 틱 옵션을 되돌리지 않는다

- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:131`
- **범주**: 성능/안전
- **문제**: 조립 시 리더 메시에 두 가지를 건다 — 표시 끄기(`:54`)와 `AlwaysTickPoseAndRefreshBones`(`:56`). 해제에서는 표시만 되돌리고(`:131`) 틱 옵션은 그대로 둔다. 또 되돌림이 "원래 값 복원"이 아니라 무조건 `true`라, 이 컴포넌트가 손대기 전부터 숨겨져 있던 리더는 해제 후 보이게 된다. 실제 창은 좁다 — 액터 파괴에서는 리더도 함께 사라지고, 레벨 스트리밍 재등록에서는 조립이 다시 돌며 두 설정을 모두 다시 걸기 때문이다. 짝지어 건 설정 중 하나만 되돌리는 비대칭 자체가 남는 문제다.
- **제안**: 등록 시 리더의 표시·틱 옵션 원래 값을 보관하고 해제에서 그대로 복원한다.
- **확신도**: 높음

### 6. 🟢 `PostInitializeComponents`의 즉시 태그 확인 두 곳은 주석이 말하는 상황을 못 본다

- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:61`, `Source/WxGame/Character/WxCharacterBase.cpp:71`
- **범주**: 중복/복잡도
- **문제**: 두 블록의 주석은 "late join 시 구독보다 먼저 초기 복제로 태그가 실려 왔을 수 있어"를 근거로 든다. 그런데 권위 측에서는 이 시점의 ASC가 갓 만들어진 상태라 `Ability.Death`·`State.Ragdoll`이 서 있을 수 없고, 뒤늦게 태그가 오면 바로 위에서 건 `RegisterGameplayTagEvent` 콜백이 그 변경을 잡는다. 더 결정적인 건 여기서 `HandleDeath()`가 불려도 `OnDeath` 구독자가 아직 하나도 없다는 점이다 — `AWxEnemyCharacter`는 `BeginPlay`(`WxEnemyCharacter.cpp:49`)에서, `AWxAIController`는 `OnPossess`(`WxAIController.cpp:36`)에서 붙는다. 그래서 이 확인이 참이 되더라도 방송은 허공에 나가고, 실제로 필요한 안전망은 `AWxEnemyCharacter::BeginPlay:51`–`:56`의 재확인이 이미 들고 있다.
- **제안**: 두 즉시 확인을 지우거나, 정말 초기 복제 이후를 보려면 `BeginPlay`로 옮긴다. 옮기지 않는다면 주석의 근거를 실제 대상 상황으로 고친다.
- **확신도**: 중간

### 7. 🟢 처형 프롬프트가 코드에 박힌 비번역 문자열이다

- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:137`
- **범주**: 설계/구조
- **문제**: 플레이어에게 보이는 상호작용 프롬프트를 `FText::FromString(TEXT("Finisher"))`로 만든다. 수집용 namespace/key가 없어 일반 텍스트 수집 경로로 번역되지 않고, 문구를 바꾸려면 코드 수정과 재빌드가 필요하다. 같은 계약을 구현하는 `AWxDialogueActor::GetInteractionPrompt`는 데이터(대화 컴포넌트)에서 문구를 가져오므로 이 하나만 어긋나 있다.
- **제안**: `NSLOCTEXT`로 정의하거나, BP 디폴트에서 저작 가능한 `FText` 프로퍼티로 노출한다.
- **확신도**: 높음

### 8. 🟢 적 ASC의 GE 복제 모드 설정은 엔진 기본값을 다시 쓰는 무동작 호출이다

- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:46`
- **범주**: 중복/복잡도
- **문제**: `BeginPlay`에서 `SetReplicationMode(EGameplayEffectReplicationMode::Full)`을 부르는데, `UAbilitySystemComponent` 생성자가 이미 `Full`로 초기화한다(UE 5.8 `AbilitySystemComponent.cpp:78`). 즉 상태를 바꾸지 않는 줄이 런타임 초기화 경로에 남아 있다. 게다가 짝이 되는 플레이어 쪽(`WxPlayerCharacter.cpp:56`)은 같은 설정을 생성자에서 하므로, 같은 성격의 설정이 두 파생에서 서로 다른 시점에 놓여 있다. 부여 순서도 어긋나 있다 — `AutoPossessAI`로 `PossessedBy`→`InitAbilitySystem`→`GiveAbilitySets`가 `BeginPlay`보다 먼저 끝나므로, 이 호출은 어빌리티셋 부여 뒤에 도착한다.
- **제안**: 줄을 지우거나(순정 기본값 유지), 의도를 남기려면 플레이어와 같이 생성자로 올린다.
- **확신도**: 높음

### 9. 🟢 Quest·Dialogue 뷰모델은 늦게 오는 소스에 다시 붙을 경로가 없는데 주석은 있다고 말한다

- **위치**: `Source/WxGame/MVVM/WxViewModel_Quest.cpp:82`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp:72`
- **범주**: 설계/구조
- **문제**: 두 리졸버 모두 "소스가 늦게 준비되면 호출 측에서 이 인스턴스에 `Initialize` 로 주입한다"고 적어 두었지만, 저장소 전체에서 이 두 뷰모델의 `Initialize`를 밖에서 부르는 코드는 없다(두 cpp 내부 호출이 전부다). 같은 폴더의 Inventory·InteractionList는 `OnAnyInventoryReady`·`OnAnyScannerReady` 같은 준비 신호를 스스로 관찰해 나중에 붙는데, Quest·Dialogue만 생성 시점 스냅샷으로 끝난다. Quest 소스는 GameState에 있어 위젯보다 늦게 도착할 여지가 Dialogue(PC 기본 서브오브젝트)보다 크다. 스탠드얼론에서는 GameState가 항상 먼저 서므로 현재 증상으로 드러나지는 않는다.
- **제안**: 주석이 말하는 주입 경로를 실제로 만들지 않을 것이라면 주석을 지우고 "생성 시점 소스가 곧 계약"임을 명시한다. 재연결이 필요해지는 시점에 Inventory와 같은 준비 신호 관찰을 도입한다.
- **확신도**: 중간

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/FrontEnd/WxFrontEndWidget.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InventoryItem.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`.
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.h`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp` 및 대응 헤더 전부. 이번 변경의 경계를 확인하려고 `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Character.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Nameplate.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_NameplateCharacter.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, 엔진의 `AbilitySystemComponent.cpp`도 함께 읽었다.
- **일괄 검사**: `Copyright` 첫 줄(63파일 전부 통과), `FORCEINLINE`·헤더 인라인 정의(0건), 람다(0건), 델리게이트 콜백 `Handle` 접두사(발견 4 외 전부 통과), `BlueprintCallable`(라이브러리 2건과 승인된 VM Command 예외 5건뿐), `TODO`/`FIXME`(0건), 틱·타이머(0건).
- **재검증 결과 제외한 항목**: 삭제된 `UWxViewModelResolver_PlayerCharacter`로 인한 끊긴 참조는 없었다 — 클래스는 WxUI로 이관됐고 `Config/DefaultEngine.ini:117`의 `ClassRedirects`가 옛 `/Script/WxGame` 경로를 받아 준다. 뷰모델의 `BlueprintCallable`(`RequestAdvance`·`RequestInteract`·`RequestCycle`·`RequestUseConsumable`·`SetCurrentCategory`)은 VM Command에 대해 승인된 예외라 규칙 위반으로 세지 않았다. `LastAcquiredItem` 교체 시 이전 인스턴스를 종료하지 않는 건은 토스트 등 독립 표시 객체라는 판단이 이미 내려져 있고, 교체와 동시에 유일한 `UPROPERTY` 참조가 끊겨 GC 대상이 되므로 구독이 무한히 쌓이지 않음을 확인해 제외했다. `AWxEnemyCharacter::OnSpawnedBy`의 스포너 부착은 확정된 설계라 다루지 않았다. `UWxViewModel_InteractionList`가 첫 바인딩 이후 `OnAnyScannerReady` 구독을 놓는 건, 스캐너가 PC 기본 서브오브젝트여서 위젯보다 먼저 사라질 경로가 없음을 확인해 제외했다. `HandleOwnerDeath`의 보상 중복 지급 가능성은 `UWxAbility_Death`가 스스로 종료하지 않아 `Ability.Death`가 한 번만 서는 것을 확인해 제외했다. `InitAbilitySystem`이 SPD 0으로 `MaxWalkSpeed`를 한 번 0으로 만드는 창은 같은 함수 안의 `GiveAbilitySets`가 곧바로 어트리뷰트 변경 콜백으로 복구함을 확인해 제외했다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE·네트워크 실행은 하지 않았다. WBP/BP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)와 BP 디폴트 값(예: `UWxAbility_Interact::ScanRadius`와 스캐너 반경의 일치, `AWxEnemyCharacter::NameplateComponent`의 `WidgetClass`·기본 가시성)은 범위 밖이며 C++ 기본값의 정합만 확인했다. 멀티플레이 경로는 프로젝트 정책상 미결정이라 권위·복제 지적을 단정하지 않고 근거만 적었다. 모듈 아래 h/cpp 63개를 전부 통독한 결과는 아니다 — `Automation/`·`Commandlet/`·`Tests/` 폴더는 현재 비어 있다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 63파일 — `/module-review`로 갱신*
