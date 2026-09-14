# WxUI — 코드 리뷰

> 푸시 경로의 취소·재진입 가드와 뷰모델 계층의 수명·예약 병합은 탄탄하고, 지난 리뷰의 이펙트 목록 통지 폭주·미사용 선언·가득 찬 자원의 비용 헛 판정은 해소됐다. 남은 실질 문제는 버튼이 아닌 경로로 끝나는 두 화면(확인 팝업·사망 화면)의 뒷정리이고, 나머지는 사소한 관례 어긋남이다. 커버리지: 소스 58파일을 모두 읽었고, 서브시스템·푸시 액션·팝업·MVVM 핵심 VM·네임플레이트·레이아웃 컴포넌트·인디케이터·StateTree 노드 2종은 cpp까지 정독했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 확인 팝업이 버튼 외 경로로 끝나면 결과 콜백이 오지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:66-79`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80-94`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:14-15`, `:70`
- **범주**: 버그/정확성
- **문제**: 결과 콜백은 `HandleResultChosen`에서만 실행되고, 그 진입점은 버튼 클릭 셋(`WxConfirmationPopup.cpp:12-23`)과 호출자가 없는 `KillPopup`(`:80-84`, UFUNCTION이 아니라 BP에서도 못 부른다)뿐이다. 그래서 두 갈래의 종료가 콜백 없이 끝난다.
  - 푸시 실패: `ShowConfirmation`은 완료 콜백을 걸지 않는다. 팝업 클래스 미지정·레이아웃 없음(`Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:24-37`), 클래스 로드 실패·로드 중 레이아웃 교체·위젯 생성 실패(`:84-118`)면 `Finish(nullptr)`로 조용히 끝난다.
  - 활성화 후 버튼이 아닌 종료: 엔진의 뒤로 가기 기본 처리(`bIsBackHandler`면 무조건 `DeactivateWidget`), `UWxUILibrary::DeactivateOwningActivatable`(`Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:48-63`), PC 교체에 따른 레이아웃 제거(`WxUIManagerSubsystem.cpp:214-218`)는 버튼을 거치지 않아 `OnResultCallback`이 바인딩된 채 버려진다.

  `EWxPopupResult::Killed`는 "사용자 입력 없이 강제로 종료됐다"로 정의돼 있지만 이를 내는 경로가 실제로는 없다. BP `ShowConfirmationPopup`으로 결과를 기다리는 흐름은 뒤로 가기 한 번(뒤로 가기 처리를 켠 팝업일 때)에 끊긴다.
- **제안**: `UWxConfirmationPopup`에 `NativeOnDeactivated`를 재정의해 콜백이 아직 바인딩돼 있으면 `Killed`로 실행한다(`HandleResultChosen`이 먼저 언바인딩하므로 버튼 경로와 겹치지 않는다). `ShowConfirmation`은 완료 콜백을 걸어 `Widget == nullptr`이면 `ResultCallback`에 `Killed`를 보낸다. 그 뒤 호출자 없는 `KillPopup`은 지운다.
- **확신도**: 높음

### 2. 🟡 사망 화면만 취소·재확인·회수 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269-280`, `:240-253`
- **범주**: 버그/정확성
- **문제**: 대화 창은 진행 중인 푸시를 붙들고(`:292-296`), 완료 시 태그를 재확인하며(`:307-323`), 관찰을 놓을 때 닫는다(`:251-252`, `:325-339`). 사망 화면은 셋 다 없이 띄우고 잊는다. 닫는 주체는 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:79`)뿐이고, 이 함수는 PC가 빙의 중인 폰이 유효하고 `Ability.Death`를 가질 때만 진행한다(`:32-39`). 따라서 사망한 폰이 부활 외 경로로 빙의 해제·파괴되면(사망 직후 월드 밖으로 떨어져 파괴되는 경우 등) `WatchPawnTags`는 대화 창만 닫고 Menu 레이어의 사망 화면은 남으며, 부활 버튼도 거부돼 복구할 수 없다. Menu 레이어가 활성으로 굳어 인디케이터도 계속 숨는다(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:110-115`). 로드 중에 관찰 대상이 바뀌어도 푸시가 취소되지 않아 옛 폰의 사망 화면이 뒤늦게 뜬다.
- **제안**: 대화 창과 같은 모양으로 맞추되, 닫기만 하지 말고 현재 상태에 맞춘다. 진행 중인 푸시와 띄운 화면을 멤버로 붙들고, 관찰 대상이 바뀌거나 푸시가 끝날 때 관찰 중인 ASC에 `Ability.Death`가 없으면 닫고(푸시는 취소), 있는데 화면이 없으면 띄운다. 이렇게 하면 `RequestRespawn`이 부활 실패 시 죽은 폰을 다시 빙의하는 경로(`WxRespawnLibrary.cpp:57-63`)에서도 `UnPossess`로 닫힌 화면이 다시 뜬다.
- **확신도**: 중간(현재 부활 흐름만 보면 재현 창이 좁다)

### 3. 🟢 `AWxIndicator::BeginPlay`가 베이스보다 넓은 public으로 재정의돼 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h:28-31`
- **범주**: 설계/구조
- **문제**: 직접 베이스 `AActor::BeginPlay`는 protected인데(엔진 `Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:2125`) 여기서는 public 절에 선언해, 파생 타입으로는 외부에서 `BeginPlay`를 직접 부를 수 있게 열린다. override는 직접 베이스의 접근 지정자를 유지한다는 프로젝트 관례와 어긋난다. 같은 절의 `Tick`은 베이스도 public이라 문제없고, 모듈의 다른 override(`UWxButtonBase`·`UWxNameplateComponent`·`UWxPlayerLayoutComponent`·리졸버 등)는 모두 베이스와 일치한다.
- **제안**: `BeginPlay` 선언을 protected 절로 옮기고, cpp 정의 순서도 헤더에 맞춘다.
- **확신도**: 높음

### 4. 🟢 호출자 없는 `FWxConfirmationPopupAction::operator==`
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:35`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:5-8`
- **범주**: 중복/복잡도
- **문제**: 저장소 C++ 어디에서도 이 구조체나 `ButtonActions` 배열을 비교하지 않고, `TStructOpsTypeTraits`(`WithIdenticalViaEquality`) 특수화도 없어 리플렉션 경로에서도 쓰이지 않는다. 호출자 없는 방어적 선언이다.
- **제안**: 선언과 정의를 지운다.
- **확신도**: 높음

### 5. 🟢 이펙트 목록 초기화 여부를 별도 bool로 들고 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:92`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:56`, `:62`, `:87`
- **범주**: 중복/복잡도
- **문제**: `bActiveEffectsInitialized`가 뜻하는 "추가·제거 이벤트를 구독하고 목록을 구성했다"는 사실은 `InitializeActiveEffects`의 구독(`:63-64`)과 `Deinitialize`의 해제(`:72-73`)가 이미 ASC 델리게이트에 남긴다. 최초 목록 구성이 더는 통지를 내지 않아(`:159-168`) 재진입 가드 역할도 없으므로, 같은 사실을 두 곳에 두는 분기용 멤버 플래그일 뿐이다. 기존 상태에서 파생하고 플래그를 두지 않는다는 프로젝트 방침과 어긋난다.
- **제안**: 가드를 `ASC->OnActiveGameplayEffectAddedDelegateToSelf.IsBoundToObject(this)`로 바꾸고, 플래그 선언과 `Deinitialize`의 리셋을 걷는다.
- **확신도**: 중간

### 6. 🟢 호출부가 하나뿐인 헬퍼를 익명 namespace로 뺐다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp:12-26`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp:12-22`
- **범주**: 중복/복잡도
- **문제**: `MakeNativeResultDelegate`(호출부 `WxUILibrary.cpp:82`)와 `MakeSubtitleContext`(호출부 `WxViewModel_Subtitle.cpp:35`)는 각각 호출부가 하나뿐인데 파일 로컬 자유 함수로 분리돼 있다. 프로젝트 방침은 단일 호출부의 작은 헬퍼를 호출부에 풀어 쓰는 것이다. `MakeSubtitleContext`의 근거 주석("등록·조회가 같은 값을 써야 하므로 문맥을 한 곳에서 만든다")도, 실제로는 `GetOrCreate` 안에서 만든 `Context` 하나를 조회(`:36`)와 등록(`:43`)이 함께 쓰므로 헬퍼 없이 이미 성립한다.
- **제안**: 두 헬퍼를 각 호출부에 인라인하고 익명 namespace를 걷는다. 동적→네이티브 델리게이트 어댑터 람다(`WxUILibrary.cpp:21`)는 필요한 람다이므로 그대로 둔다.
- **확신도**: 중간(`CLAUDE.md` 명문 규칙이 아니라 사용자 피드백으로 정해진 방침이다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp` (각 대응 헤더 포함)
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUIModule.cpp`, `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicatorWidget.h`, `Plugins/WxUI/Source/WxUI/Public/Subtitle/WxSubtitleTableRow.h`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, 엔진 `LevelTick.cpp`·`CommonActivatableWidget.cpp`·`CommonActivatableWidgetContainer.cpp`·`LocalPlayer.cpp`·`Actor.h`(판정 근거 확인용)
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩·이벤트 그래프)는 보지 않았다. 에셋 사용 여부는 `.uasset` 바이너리 문자열 검색으로만 확인해 리다이렉터 등 이름이 달라진 참조는 잡지 못한다.
  - 게임→프론트엔드 `OpenLevel` 이동 시, GameInstance 서브시스템이 쥔 `PrimaryGameLayout`과 스택 위젯 풀·MVVM 소스(Outer가 옛 폰 ASC인 VM)가 옛 월드를 GC에서 붙잡는지는 코드만으로 결론 내지 못했다. 이동 후 월드 누수 경고 여부를 런타임으로 확인할 필요가 있다.
  - 모듈 규칙은 전 파일 확인했다. `WxUI.Build.cs`·`WxUI.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않고, Wx 헤더 include도 WxCore 공개 헤더(`WxUIData.h`·`WxGameplayTags.h`·`WxLocatorUtils.h`)뿐이다. 소스 58파일 모두 첫 줄 저작권이 맞다. 헤더 인라인 정의는 StateTree `GetInstanceDataType()` 2건뿐이고 코딩 규칙 4 예외 사유 주석이 있다. 람다는 `WxUILibrary.cpp:21`의 델리게이트 어댑터 1건으로 정당하다.
  - 의도된 결정이라 뺀 것: 월드 타이머 예약이 일시정지 중 돌지 않아 해제 후 반영되는 점과 비용 어트리뷰트 실제 변경의 즉시 판정은 `2026-09-14-코드리뷰-반영-월드타이머-전환` 워크로그에서 감수로 결정됐다. `UWxActivatableWidget::GetDesiredInputConfig`가 `All`을 미설정으로 돌리고 `Super`(BP `BP_GetDesiredInputConfig`)를 부르지 않는 점은 워크로그상 "관여하지 않음" 의미로 쓰이고 있고 해당 설정·오버라이드를 쓰는 에셋이 없어 뺐다.
  - `CLAUDE.md`에 근거 규칙이 없어 뺀 것: `UWxViewModel_Ability::TryActivateAbility`의 명명(`Request~` 관례와 다르지만 뷰모델 명령의 BlueprintCallable 예외 범주), 타이머·티커 콜백의 `Flush~`·`Update~` 명명.
  - 빌드·PIE·자동화 테스트는 돌리지 않았다(리뷰 전용).

---
*문서 기준 커밋 `28fe02f12` · 리뷰일 2026-09-15 · 소스 58파일 — `/module-review`로 갱신*
