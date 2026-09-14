# WxUI — 코드 리뷰

> 푸시 경로의 취소·재진입 가드와 뷰모델 계층의 수명·코얼레싱 처리는 여전히 탄탄하다. 남은 문제는 버튼이 아닌 경로로 끝나는 화면(확인 팝업·사망 화면)의 뒷정리, 이펙트 목록 최초 구성의 통지 폭주, 상시로 도는 비용 어트리뷰트 재판정이다. 지난 리뷰의 `TryActivateAbility` 명명 건은 현행 `CLAUDE.md`에 근거 규칙이 없어 뺐다. 커버리지: `WxUIManagerSubsystem`·`WxAsyncAction_PushWidgetToLayer`·팝업·MVVM 핵심 계층·네임플레이트·레이아웃 컴포넌트·인디케이터·StateTree 노드 2종을 cpp까지 정독했고, 나머지 소형 VM·위젯·설정은 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 확인 팝업이 버튼 외 경로로 끝나면 결과 콜백이 영영 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80-94`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:14-15`, `:70`
- **범주**: 버그/정확성
- **문제**: 결과 콜백은 `HandleResultChosen`에서만 실행되고, 그 진입점은 버튼 클릭 셋(`WxConfirmationPopup.cpp:12-23`)과 C++·에셋 어디에서도 호출되지 않는 `KillPopup`(`:80-84`)뿐이다. 그래서 두 갈래의 종료가 콜백 없이 끝난다.
  - 푸시 실패: `ShowConfirmation`은 완료 콜백을 걸지 않는다. 레이아웃이 없거나(`WxAsyncAction_PushWidgetToLayer.cpp:33-37`) 클래스 로드 실패·로드 중 레이아웃 교체·위젯 생성 실패(`:84-118`)면 `Finish(nullptr)`로 조용히 끝난다.
  - 활성화 후 버튼이 아닌 종료: 뒤로 가기 처리(`bIsBackHandler`), `UWxUILibrary::DeactivateOwningActivatable`(`WxUILibrary.cpp:50-65`), `DeactivateWidgetsInLayer`의 `ClearWidgets`(`:81`), PC 교체에 따른 레이아웃 제거(`WxUIManagerSubsystem.cpp:214-218`)는 모두 버튼을 거치지 않아 `OnResultCallback`이 바인딩된 채 버려진다.

  `EWxPopupResult::Killed`는 "사용자 입력 없이 강제로 종료됐다"로 정의돼 있지만 이를 내는 경로가 실제로는 없다. BP `ShowConfirmationPopup`으로 결과를 기다리는 흐름은 Esc 한 번(뒤로 가기 처리를 켠 팝업일 때)으로 끊긴다.
- **제안**: `UWxConfirmationPopup`에 `NativeOnDeactivated`를 재정의해 콜백이 아직 바인딩돼 있으면 `Killed`로 실행한다(`HandleResultChosen`이 먼저 언바인딩하므로 버튼 경로와 중복되지 않는다). `ShowConfirmation`은 완료 콜백을 걸어 `Widget == nullptr`이면 `ResultCallback(Killed)`를 부른다. 그 뒤 `KillPopup`은 지운다.
- **확신도**: 높음

### 2. 🟡 이펙트 목록 최초 구성이 자기 Getter 안에서 같은 필드를 이펙트 수만큼 통지한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:44-64`, `:155-173`, `:218-219`
- **범주**: 성능/안전
- **문제**: `ActiveEffectViewModels`의 Getter(`:44-49`)가 `InitializeActiveEffects` → `BuildActiveEffectViewModels`로 목록을 지연 구성하고, 구성은 활성 GE마다 `HandleActiveEffectAdded`를 태워(`:170`) GE 하나당 `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels)`를 낸다(`:219`). 즉 그 필드를 읽는 도중에 같은 필드의 변경 통지가 아이콘 GE 수만큼 재진입한다. `bActiveEffectsInitialized`(`:60`)는 구독·생성 중복만 막고 통지는 막지 않는다. 엔진 MVVM 뷰는 바인딩 초기화 중 들어온 통지를 실행하지 않고 경고만 남기며(엔진 `Engine/Plugins/Runtime/ModelViewViewModel/Source/ModelViewViewModel/Private/View/MVVMView.cpp:754-759`), 같은 바인딩 실행 중에 들어오면 재귀로 보고 `ensureAlways`를 띄운다(`:818-827`). 어느 쪽이든 이미 버프가 걸린 대상에 목록 위젯이 처음 붙는 순간 헛 통지가 N번 나간다. 증분 추가 경로의 건당 통지는 맞다.
- **제안**: 최초 구성은 로컬 배열에 모두 담은 뒤 한 번만 대입·통지하고, 건당 통지는 이벤트 경로(`HandleActiveEffectAdded`)에만 남긴다. 구성이 통지를 내지 않으면 재진입 가드도 필요 없어지므로, 초기화 여부는 `OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this)`로 파생해 `bActiveEffectsInitialized` 플래그를 걷을 수 있다.
- **확신도**: 중간(재진입은 코드상 확실하나, 경고로 끝날지 ensure가 뜰지는 위젯의 바인딩 경로에 달렸다)

### 3. 🟡 사망 화면만 취소·재확인·회수 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269-280`, `:240-253`
- **범주**: 버그/정확성
- **문제**: 대화 화면은 진행 중인 푸시를 붙들고(`:292-296`), 완료 시 태그를 재확인하며(`:307-323`), 관찰을 놓을 때 닫는다(`:251-252`, `:325-339`). 사망 화면은 셋 다 없다. 닫는 주체는 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:79`)뿐이고, 그것은 PC가 빙의 중인 폰이 `Ability.Death`를 가질 때만 진행한다(`:32-38`). 따라서 사망 중 폰이 부활 외 경로로 빙의 해제·파괴되면(사망 후 월드 밖 낙하 등) `WatchPawnTags(nullptr)`는 대화 창만 닫고, Menu 레이어의 사망 화면은 남고, 부활 버튼도 거부돼 복구할 수 없다. Menu 레이어가 활성으로 굳어 인디케이터도 계속 숨는다(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:110-115`). 스트리밍 도중 관찰 대상이 바뀌면 옛 폰의 사망 화면이 뒤늦게 뜨는 것도 같은 원인이다.
- **제안**: 대화 화면과 같은 모양으로 맞춘다. 진행 중인 푸시와 띄운 화면을 멤버로 붙들고, 완료 콜백에서 `WatchedAbilitySystem`의 `Ability.Death`를 재확인해 이미 없으면 즉시 `DeactivateWidget`하며, `WatchPawnTags`의 정리 지점에서 사망 화면도 함께 닫는다.
- **확신도**: 중간(현재 부활 흐름만 보면 재현 창이 좁다)

### 4. 🟢 대상·호출자가 없는 선언 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h:39`, `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h:41-42`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:67-82`
- **범주**: 중복/복잡도
- **문제**: `friend class FWxNameplatePresentationTest;`가 가리키는 클래스는 저장소 어디에도 없다. 실제 테스트 `FWxNameplateViewModelTest`(`Source/WxGame/Tests/WxNameplateViewModelTest.cpp:15`)는 공개 API만 쓴다. `UWxUILibrary::DeactivateWidgetsInLayer`는 C++ 호출자가 없고 `Content`·`Plugins`의 `.uasset` 바이너리 문자열 검색에서도 참조가 나오지 않는다.
- **제안**: 둘 다 지운다.
- **확신도**: 높음

### 5. 🟢 비용 어트리뷰트 변경만 발동 판정을 모으지 않아 상시로 초당 30회 돈다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:416-420`, `:584-591`
- **범주**: 성능/안전
- **문제**: 태그 변경은 티커로 프레임당 한 번 판정하는데(`:403-414`), 비용 어트리뷰트와 그 최대치 변경은 올 때마다 즉시 `RefreshActivationState`(활성 스펙 순회 + `CanActivateAbility`, 막히면 `CheckCost`)를 돈다. 엔진은 값이 같아도 어트리뷰트 변경 델리게이트를 브로드캐스트한다(엔진 `GameplayAbilities/Private/GameplayEffect.cpp:3945-3981`, `GameplayEffectAggregator.cpp:438-445`에 동등 비교가 없다). 플레이어 공용 어빌리티 세트 에셋(`Content/Character/Template/Shared/ABS_Shared_Player.uasset`)이 참조하는 `UWxEffect_RegenSP`는 1/30초 주기의 무한 GE라(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_RegenSP.h:23`) 가드·질주·탈진이 아닌 한 SP가 가득 찬 상태에서도 초당 30회 통지가 오고, 질주 중에는 `WxEffect_DrainSP.h:23`이, 무한 MP 동안에는 `WxEffect_InfiniteMP.h:22`가 같은 주기로 이어받는다. 결국 SP·MP를 비용으로 쓰는 슬롯 VM마다 초당 30회의 `CanActivateAbility`(비용 판정의 스펙·컨텍스트 할당 포함)가 상시로 돈다. 헤더(`WxViewModel_Ability.h:26-27`)는 태그 변경만 모은다고 적어 두었지만, 실제로 더 자주 오는 쪽은 비용 어트리뷰트다.
- **제안**: `HandleCostAttributeChanged`에서 `Data.OldValue == Data.NewValue`면 조기 반환하고, 나머지는 `ActivationRefreshHandle` 티커를 예약해 `FlushActivationRefresh`를 재사용한다.
- **확신도**: 중간(절대 비용은 슬롯 수에 비례해 작다)

### 6. 🟢 호출부가 하나뿐인 헬퍼를 익명 namespace로 뺐다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:14-28`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:12-22`
- **범주**: 중복/복잡도
- **문제**: `MakeNativeResultDelegate`(호출부 `WxUILibrary.cpp:101`)와 `MakeSubtitleContext`(호출부 `WxViewModel_Subtitle.cpp:35`)는 각각 호출부가 하나뿐인데 익명 namespace 자유 함수로 분리돼 있다. 프로젝트 방침은 .cpp 내부 헬퍼를 호출부에 인라인하는 것이다. `MakeSubtitleContext`의 근거 주석("등록·조회가 같은 값을 써야 하므로 문맥을 한 곳에서 만든다")도, 실제로는 `GetOrCreate` 안에서 만든 `Context` 하나를 조회(`:36`)와 등록(`:43`)이 함께 쓰므로 헬퍼 없이 이미 성립한다.
- **제안**: 두 헬퍼를 각 호출부에 인라인하고 익명 namespace를 걷는다. 동적→네이티브 델리게이트 어댑터 람다(`WxUILibrary.cpp:23`)는 필요한 람다이므로 그대로 둔다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 피드백으로 정해진 방침이다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp` (각 대응 헤더 포함)
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_RegenSP.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_InfiniteMP.cpp`, 엔진 `CommonActivatableWidgetContainer.cpp`·`MVVMView.cpp`·`GameplayEffect.cpp`(판정 근거 확인용)
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩 행)는 보지 않았다. 발견 2의 경고/ensure 중 어느 쪽이 실제로 나는지, 발견 5에서 어느 HUD 슬롯이 SP·MP 비용 어빌리티를 무는지는 에디터 확인이 필요하다.
  - 에셋 참조 확인은 `.uasset` 바이너리 문자열 검색이라 리다이렉터 등 이름이 달라진 참조는 잡지 못한다.
  - 인디케이터가 대상의 월드 파티션 스트리밍 아웃 시점에 곧바로 부착 해제되는지(`HasTarget` 재시도의 전제)는 런타임으로 확인하지 않았다.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).
  - 모듈 규칙은 전 파일 확인했다. `WxUI.Build.cs`·`WxUI.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않고, 소스의 Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다. 소스 58파일 모두 첫 줄 저작권이 맞다. 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고 코딩 규칙 4를 가리키는 예외 사유 주석이 있다. 람다는 `WxUILibrary.cpp:23`의 동적→네이티브 델리게이트 어댑터 1건으로 정당하다.
  - 지난 리뷰의 "`TryActivateAbility`가 `Request~` 명명을 따르지 않는다"는 현행 `CLAUDE.md`에 `BlueprintCallable`·명명 규칙이 없어 뺐다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 58파일 — `/module-review`로 갱신*
