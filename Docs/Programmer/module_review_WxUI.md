# WxUI — 코드 리뷰

> 뷰모델 계층은 수명주기·재진입·코얼레싱 처리가 여전히 성숙하다. 지난 리뷰의 네임플레이트 매 틱 재바인딩, 공유 Character VM 획득 경로 분산, `ApplyLoadedImage` 필드명 무시, GC 수명 주석 오류는 모두 고쳐졌다. 남은 것은 버튼이 아닌 경로로 끝나는 화면(확인 팝업·사망 화면)의 뒷정리와 이펙트 목록 최초 구성의 통지 폭주다. 커버리지: `WxUIManagerSubsystem`·`WxAsyncAction_PushWidgetToLayer`·네임플레이트 컴포넌트·MVVM 전 계층·팝업·인디케이터를 cpp까지 정독했고, StateTree 노드·HUD·버튼·설정은 훑었다.

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
- **문제**: 결과 콜백은 `HandleResultChosen`에서만 실행되고, 그 진입점은 버튼 클릭 셋(`WxConfirmationPopup.cpp:12-23`)과 호출자가 하나도 없는 `KillPopup`(`:80-84`)뿐이다. 그래서 두 갈래의 종료가 콜백 없이 끝난다.
  - 푸시 실패: `ShowConfirmation`은 완료 콜백을 걸지 않는다. 레이아웃이 없거나(`WxAsyncAction_PushWidgetToLayer.cpp:33-37`) 클래스 로드 실패·로드 중 레이아웃 교체(`:85-104`)면 `Finish(nullptr)`로 조용히 끝난다.
  - 활성화 후 버튼이 아닌 비활성화: 뒤로 가기 처리(`bIsBackHandler`), `UWxUILibrary::DeactivateWidgetsInLayer`의 `ClearWidgets`(`WxUILibrary.cpp:81`), PC 교체에 따른 레이아웃 제거(`WxUIManagerSubsystem.cpp:214-218`)는 전부 `DeactivateWidget` 경로라 `OnResultCallback`이 바인딩된 채 버려진다.
  
  `EWxPopupResult::Killed`는 "사용자 입력 없이 강제로 종료됐다"로 정의돼 있지만 이를 내는 코드가 없다. BP `ShowConfirmationPopup`으로 결과를 기다리는 쪽은 Esc 한 번(뒤로 가기 처리를 켠 팝업일 때)으로 흐름이 끊긴다.
- **제안**: `UWxConfirmationPopup`에 `NativeOnDeactivated`를 재정의해 콜백이 아직 바인딩돼 있으면 `Killed`로 실행한다(`HandleResultChosen`이 먼저 언바인딩하므로 버튼 경로와 중복되지 않는다). `ShowConfirmation`은 완료 콜백을 걸어 `Widget == nullptr`이면 `ResultCallback(Killed)`를 부른다. 그 뒤 `KillPopup`은 지운다.
- **확신도**: 높음

### 2. 🟡 이펙트 목록 최초 구성이 자기 Getter 안에서 같은 필드를 이펙트 수만큼 통지한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:44-64`, `:155-173`, `:218-219`
- **범주**: 성능/안전
- **문제**: `ActiveEffectViewModels`의 Getter(`:44`)가 `InitializeActiveEffects` → `BuildActiveEffectViewModels`로 목록을 지연 구성하고, 구성은 활성 GE마다 `HandleActiveEffectAdded`를 태워(`:170`) GE 하나당 `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels)`를 낸다(`:219`). 즉 그 필드를 읽는 도중에 같은 필드의 변경 통지가 아이콘 GE 수만큼 재진입한다. `bActiveEffectsInitialized`(`:60`)는 구독·생성 중복만 막고 통지는 막지 않는다. 엔진 MVVM 뷰는 바인딩 초기화 중 들어온 통지를 실행하지 않고 경고만 남기며(엔진 `ModelViewViewModel/.../View/MVVMView.cpp:754-759`), 같은 바인딩 실행 중에 들어오면 재귀로 보고 `ensureAlways`를 띄운다(`:816-827`). 어느 쪽이든 이미 버프가 걸린 대상에 목록 위젯이 처음 붙는 순간 헛 통지가 N번 나간다. 증분 추가 경로의 건당 통지는 맞다.
- **제안**: 최초 구성은 로컬 배열에 모두 담은 뒤 한 번만 대입·통지하고, 건당 통지는 이벤트 경로(`HandleActiveEffectAdded`)에만 남긴다. 구성이 통지를 내지 않으면 재진입 가드도 필요 없어지므로, 초기화 여부는 `OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this)`로 파생해 `bActiveEffectsInitialized` 플래그를 걷을 수 있다.
- **확신도**: 중간(재진입은 코드상 확실하나, 경고로 끝날지 ensure가 뜰지는 위젯의 바인딩 경로에 달렸다)

### 3. 🟡 사망 화면만 취소·재확인·회수 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269-280`, `:240-253`
- **범주**: 버그/정확성
- **문제**: 대화 화면은 진행 중인 푸시를 붙들고(`:292-296`), 완료 시 태그를 재확인하며(`:307-323`), 관찰을 놓을 때 닫는다(`:251-252`, `:325-339`). 사망 화면은 셋 다 없다. 닫는 주체는 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:79`)뿐이고, 그것은 PC가 빙의 중인 폰이 `Ability.Death`를 가질 때만 진행한다(`:32-38`). 따라서 사망 중 폰이 부활 외 경로로 빙의 해제·파괴되면(월드 밖 낙하 등) `WatchPawnTags(nullptr)`는 대화 창만 닫고, Menu 레이어의 사망 화면은 남고, 부활 버튼도 거부돼 복구할 수 없다. Menu 레이어가 활성으로 굳어 인디케이터도 계속 숨는다(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:110-115`). 스트리밍 도중 관찰 대상이 바뀌면 옛 폰의 사망 화면이 뒤늦게 뜨는 것도 같은 원인이다.
- **제안**: 대화 화면과 같은 모양으로 맞춘다. 진행 중인 푸시와 띄운 화면을 멤버로 붙들고, 완료 콜백에서 `WatchedAbilitySystem`의 `Ability.Death`를 재확인해 이미 없으면 즉시 `DeactivateWidget`하며, `WatchPawnTags`의 정리 지점에서 사망 화면도 함께 닫는다.
- **확신도**: 중간(현재 부활 흐름만 보면 재현 창이 좁다)

### 4. 🟢 대상·호출자가 없는 선언 2건
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateComponent.h:39`, `Plugins/WxUI/Source/WxUI/Public/WxUILibrary.h:41-42`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:67-82`
- **범주**: 중복/복잡도
- **문제**: `friend class FWxNameplatePresentationTest;`가 가리키는 클래스는 저장소 어디에도 없다. 실제 테스트 `FWxNameplateViewModelTest`(`Source/WxGame/Tests/WxNameplateViewModelTest.cpp:15`)는 공개 API만 쓴다. `UWxUILibrary::DeactivateWidgetsInLayer`는 C++ 호출자가 없고 `Content` 에셋 바이너리 문자열 검색에서도 참조가 나오지 않는다.
- **제안**: 둘 다 지운다.
- **확신도**: 높음

### 5. 🟢 비용 어트리뷰트 변경만 발동 판정을 프레임 단위로 모으지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:418-422`, `:586-593`
- **범주**: 성능/안전
- **문제**: 태그 변경은 티커로 프레임당 한 번 판정하는데(`:405-416`), 비용 어트리뷰트와 그 최대치 변경은 올 때마다 즉시 `RefreshActivationState`(활성 스펙 순회 + `CanActivateAbility`, 막히면 `CheckCost`)를 돈다. SP 회복·소모 GE 주기가 1/30초(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_RegenSP.h:23`, `WxEffect_DrainSP.h:24`)라 SP를 쓰는 슬롯 VM 하나당 초당 30회 판정이 돈다. 헤더(`WxViewModel_Ability.h:26-27`)는 태그 변경만 모은다고 적어 두었지만, 주기 GE가 붙는 비용 어트리뷰트는 태그보다 더 자주 바뀐다.
- **제안**: `HandleCostAttributeChanged`도 `ActivationRefreshHandle` 티커를 예약하는 방식으로 바꿔 `FlushActivationRefresh`를 재사용한다.
- **확신도**: 중간(절대 비용은 슬롯 수에 비례해 작다)

### 6. 🟢 `TryActivateAbility`가 VM Command 예외의 `Request~` 명명을 따르지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h:41-42`
- **범주**: 규칙 위반
- **문제**: 뷰모델의 `BlueprintCallable`은 뷰→도메인 명령(`Request~`)에 한해 허용되는데, 이 함수는 WBP에서 부르는 명령이면서 이름이 `TryActivateAbility`다. 같은 성격의 WxGame VM 명령은 모두 `RequestInteract`·`RequestAdvance`·`RequestUseConsumable`처럼 `Request~`를 쓴다. 엔진 ASC의 같은 이름 함수와도 헷갈린다.
- **제안**: `RequestActivateAbility`로 개명하고, WBP 노드가 끊기지 않도록 `FunctionRedirects`를 함께 넣는다.
- **확신도**: 중간(명령 용도라는 예외의 본질은 충족하고 이름만 다르다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp` (각 대응 헤더 포함)
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Source/WxGame/Tests/WxNameplateViewModelTest.cpp`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩 행)는 보지 않았다. 발견 2의 경고/ensure 중 어느 쪽이 실제로 나는지는 네임플레이트 WBP의 바인딩 경로에 달려 있어 에디터 확인이 필요하다. 네임플레이트 수동 주입이 성립하는지(`CanBeSet` 소스)는 `WxNameplateViewModelTest`가 검증 대상으로 삼고 있으나 이번에 돌리지는 않았다.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).
  - 모듈 규칙은 전 파일 확인했다. `WxUI.Build.cs`·`WxUI.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않고, 소스의 Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다. 소스 58파일 모두 첫 줄 저작권이 맞고 BOM이 없다. 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고 예외 사유 주석이 있다(다만 주석이 가리키는 "코딩 규칙 6"은 현재 `CLAUDE.md`에서 4번이다). 람다는 `WxUILibrary.cpp:23`의 동적→네이티브 델리게이트 어댑터 1건으로 정당하다.
  - 지난 리뷰의 "델리게이트 콜백 `Handle` 접두 누락"은 현재 `CLAUDE.md`에 그 규칙이 없어 뺐다.

---
*문서 기준 커밋 `6bde8a033` · 리뷰일 2026-09-13 · 소스 58파일 — `/module-review`로 갱신*
