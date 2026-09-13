# WxUI — 코드 리뷰

> 뷰모델 계층의 수명주기·재진입·코얼레싱 처리는 여전히 성숙하다. 지난 리뷰의 🔴(삭제된 네임플레이트 클래스를 참조하는 에셋)은 `d1674fa`의 수동 주입 회귀로 해소됐고 — 클래스가 되살아났으며 `Config/DefaultEngine.ini`의 임시 ClassRedirect도 걷혔고, 함께 지워진 `WxViewModel_Nameplate`·`WxViewModelResolver_NameplateCharacter`를 참조하는 에셋은 `Content` 전체 바이너리 grep에서 하나도 남지 않았다 — 어트리뷰트 VM의 죽은 분기도 정리됐다. 이번 리뷰에서 심각(🔴) 등급은 없고, 남은 것은 매 프레임 비용과 공유 VM 획득 경로의 분산이다. 커버리지: 변경된 네임플레이트 컴포넌트·MVVM 6쌍·`WxUIManagerSubsystem`과 `WxAsyncAction_PushWidgetToLayer`·`WxViewModel_Ability`·`WxIndicator`를 cpp까지 정독했고, 위젯 베이스·팝업·StateTree 노드·자막은 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 네임플레이트가 거리·가시성과 무관하게 매 틱 바인딩을 재시도한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:31-39`, `:131-139`, `:58-98`
- **범주**: 성능/안전
- **문제**: `TickComponent`이 매 프레임 `UpdatePresentation`을 부르고, 그 첫 줄(`:133`)이 조건 없이 `BindViewModel()`을 돌린다. 거리 게이트(`:141`)나 `MaxVisibilityDistance <= 0`(`:135`) 판정보다 **앞**이라, 화면 밖·사거리 밖·표시를 끈 네임플레이트도 매 프레임 같은 비용을 낸다. 프레임당 네임플레이트 하나가 치르는 비용은 `UGameplayStatics::GetPlayerPawn(this, 0)`(`:35`, 컨트롤러 순회) + `GetExtension<UMVVMView>()` + `View->GetViewModel(BoundSourceName)` 이름 조회다. 적 N마리면 그대로 N배다. 더 나쁜 것은 실패 경로다 — 소유자에 ASC가 없거나(`:74`) WBP에 수동 주입 가능한 Character VM 소스가 없으면(`:82-98`) 매 프레임 `ReleaseViewModel()` + `GetSources()` 전체 순회를 반복하는데, 경고는 `bBindingErrorReported`로 1회만 나가므로 이 반복이 로그에 드러나지 않는다. 매 틱 재확인 자체는 위젯 재구성 시 자동 복구를 노린 의도로 읽히지만(뷰가 갈리면 `:66`이 불일치를 잡아낸다), 그 대가를 지금은 무조건 전액 지불한다.
- **제안**: `UpdatePresentation`의 순서를 뒤집어 거리·`MaxVisibilityDistance` 게이트를 먼저 통과한 뒤에만 `BindViewModel()`을 부른다. 뷰어 폰은 프레임당 한 번만 조회해(컴포넌트가 아니라 공용 업데이터나 캐시) 나눠 쓰고, 바인딩 재확인은 매 틱이 아니라 위젯 교체 신호(`SetWidget`/`InitWidget`)와 주기적(예: 0.5초) 확인으로 낮춘다.
- **확신도**: 중간(비용은 코드에서 그대로 읽히나, 적 수가 적은 현 단계에선 문제로 드러나지 않을 수 있다)

### 2. 🟡 공유 Character 뷰모델 획득이 세 갈래이고, 정작 팩토리는 데드 코드다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:8-21`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp:23-31`, `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp:101-106`
- **범주**: 중복/복잡도
- **문제**: 지난 리뷰의 같은 지적이 형태만 바뀌어 남았다. `UWxViewModel_Character::GetOrCreate`는 프로젝트 전체에 호출자가 하나도 없다(`grep`으로 확인 — 선언·정의뿐). 대신 플레이어 리졸버와 네임플레이트 컴포넌트가 각자 `FindSharedViewModel` + `NewObject`를 손으로 재현한다. 위험한 쪽은 규약이다 — "찾아진 것은 이미 초기화돼 있으니 `Initialize`하지 않는다"가 호출부에만 주석으로 적혀 있고(`WxNameplateComponent.cpp:100`), `Initialize`는 첫 줄이 `Deinitialize()`다(`WxViewModel_Character.cpp:25`). 새 호출부가 습관대로 `GetOrCreate` 뒤에 `Initialize`를 붙이면 남이 보고 있는 공유본의 표시가 한순간 비워지고 `AbilitySystem` 자식 참조까지 끊긴다. `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp:22`는 네 번째 갈래로, ASC가 아니라 자기 자신을 Outer로 잡아 공유 규약 밖에서 인스턴스를 하나 더 만든다.
- **제안**: "없으면 만들고, **이번에 만든 경우에만** 초기화한다"를 `UWxViewModel_Character`의 팩토리 하나로 올린다(표시 소스를 인자로 받는 `GetOrCreate(ASC, DisplaySource)` 형태). 두 호출부는 그것만 부르고, 쓰이지 않는 현재 `GetOrCreate`는 그 팩토리로 대체하거나 지운다.
- **확신도**: 높음

### 3. 🟡 이펙트 목록 최초 구성이 자기 Getter 안에서 이펙트마다 필드 변경을 브로드캐스트한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:44-64`, `:155-173`, `:218-219`
- **범주**: 성능/안전
- **문제**: `ActiveEffectViewModels`는 이번 변경으로 `Getter` 기반 FieldNotify 프로퍼티가 됐고, 그 Getter(`:44`)가 `InitializeActiveEffects` → `BuildActiveEffectViewModels`로 목록을 지연 구성한다. 구성은 활성 GE마다 `HandleActiveEffectAdded`를 태우는데(`:170`), 그 안에서 GE 하나당 `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ActiveEffectViewModels)`가 나간다(`:219`). 즉 **그 필드를 평가하는 중에 같은 필드의 변경 통지가 이펙트 수만큼 재진입**한다. `bActiveEffectsInitialized`(`:60`)가 구독·생성 중복은 막지만 브로드캐스트는 막지 않으므로, 버프 10개를 두른 대상의 목록을 처음 읽는 프레임에 바인딩(보통 `UListView` 채우기)이 아직 덜 채워진 배열로 10번 재실행된다. 증분 추가 경로에서는 브로드캐스트가 1건씩이라 맞지만, 최초 구성에서는 아니다.
- **제안**: 최초 구성과 증분 추가를 분리한다 — `BuildActiveEffectViewModels`는 로컬 배열에 다 담고 마지막에 한 번만 배열을 갈아끼우고 브로드캐스트하며, 이벤트 경로(`HandleActiveEffectAdded`)만 건당 브로드캐스트를 유지한다.
- **확신도**: 중간(재진입은 코드에서 확실하나, 체감 비용과 UMG 측 실제 영향은 에디터에서 확인 필요)

### 4. 🟡 사망 화면만 취소·재확인·회수 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269-280`
- **범주**: 버그/정확성
- **문제**: 지난 리뷰 지적이 그대로 남아 있다(코드 무변경). 바로 아래 대화 화면 경로(`:282-297`, `:307-323`, `:325-339`)는 진행 중인 푸시를 `PendingDialogueScreenPush`로 붙들고, 완료 시 태그가 아직 살아 있는지 재확인하고, 태그가 걷히면 취소·비활성화한다. 사망 화면은 셋 다 없다 — 푸시 액션을 기억하지 않고, 완료 시 `Ability.Death`를 재확인하지 않으며, 닫는 코드가 모듈 안에 없다. 닫는 주체는 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:79`) 하나뿐이고, 그것은 사망 화면 위젯 자신을 인자로 받고 `:21`에서 `IsActivated()`를, `:35`에서 `Ability.Death` 보유를 요구한다. 따라서 비동기 스트리밍 중에 사망 태그가 다른 경로로 걷히면 뒤늦게 뜬 사망 화면을 아무도 닫을 수 없다(부활 버튼도 `:35`에서 거부된다). Menu 레이어가 활성으로 굳으면 `IsMenuLayerActive()`가 true로 남아 인디케이터가 계속 숨고(`Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp:110-115`), 위젯의 `bPauseGame` 설정에 따라 정지까지 남는다. `WatchPawnTags`(`:240-267`)의 정리도 대화 화면만 걷는다.
- **제안**: 대화 화면과 같은 모양으로 맞춘다 — 진행 중인 푸시를 멤버로 붙들고, 완료 콜백에서 `WatchedAbilitySystem`의 `Ability.Death`를 재확인해 이미 걷혔으면 즉시 `DeactivateWidget`하며, `WatchPawnTags`의 정리 지점에서 사망 화면도 함께 닫는다.
- **확신도**: 중간(현 respawn 흐름만 보면 재현 창이 좁지만, 방어가 한쪽에만 있는 비대칭은 분명하다)

### 5. 🟢 `KillPopup` / `EWxPopupResult::Killed` 는 여전히 호출자가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:15`, `:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80-84`
- **범주**: 중복/복잡도
- **문제**: `KillPopup()`을 부르는 코드가 모듈·프로젝트 전체에 없다(재정의와 `Super` 호출뿐, `BlueprintCallable`도 아니라 BP에서도 부를 수 없다). 따라서 `EWxPopupResult::Killed`는 발생할 수 없는 결과값이고, 팝업을 밖에서 강제 종료하는 기능이 없다는 뜻이다. 있는 척하는 API가 오해를 만든다.
- **제안**: 강제 종료를 쓸 계획이 없으면 `KillPopup`과 `Killed`를 지운다. 쓸 계획이면 `UWxUIManagerSubsystem`에 호출 지점을 만들어 계약을 완성한다.
- **확신도**: 높음

### 6. 🟢 티커 델리게이트 콜백 일부에 `Handle` 접두가 빠져 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:246`(`FlushOwnedTagsRefresh`), `:259`(`FlushAbilityRebind`), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:41`(`UpdateCooldownState`), `:411`(`FlushActivationRefresh`), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:68`(`UpdateEffectState`)
- **범주**: 규칙 위반
- **문제**: 델리게이트 콜백에 `Handle` 접두를 붙이는 규칙인데 위 다섯은 붙어 있지 않다. 지난 리뷰에서 같은 지적이 있었고 그때 비교 대상이던 `WxViewModel_Nameplate`는 사라졌지만, 규칙 자체는 그대로다. 다만 `UpdateCooldownState`는 `WxViewModel_Ability.cpp:239`에서 직접 호출도 되어 순수 콜백은 아니다.
- **제안**: 델리게이트로만 쓰이는 `FlushOwnedTagsRefresh`·`FlushAbilityRebind`·`FlushActivationRefresh`·`UpdateEffectState`에 `Handle` 접두를 붙인다.
- **확신도**: 중간(직접 호출을 겸하는 함수는 예외로 볼 여지가 있다)

### 7. 🟢 `ApplyLoadedImage` 파생 구현 셋이 `FieldName`을 무시한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:41-44`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:383-386`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:235-238`
- **범주**: 버그/정확성
- **문제**: 베이스가 `FieldName`을 넘기는 이유는 슬롯이 여럿일 때 파생이 분기하라는 것이고(`Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h:49-59`), `UWxViewModel_Item::ApplyLoadedImage`(`WxViewModel_Item.cpp:44-49`)는 실제로 `FieldName == TEXT("Icon")`을 확인한다. 반면 위 셋은 인자를 버리고 자기 단일 필드에 무조건 대입한다. 지금은 슬롯이 하나뿐이라 결과가 같지만, 이 VM에 두 번째 이미지 슬롯(예: 캐릭터에 배경·프레임)이 붙는 순간 두 요청이 같은 필드를 덮어써 조용히 어긋난다.
- **제안**: 파생 셋도 `Item`처럼 `FieldName`을 확인하고 일치하지 않으면 무시한다.
- **확신도**: 높음(현재 동작은 정상, 확장 시 잠재 결함)

### 8. 🟢 공유 VM 수명 설명이 GC 동작과 어긋난다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h:36`
- **범주**: 설계/구조
- **문제**: "GC 는 객체 → Outer 방향만 수집하므로 Outer 인 ASC 는 이 VM 을 살려 두지 않는다 — 수명은 이 VM 을 Outer 로 삼는 자식 VM 이 쥔다"고 적혀 있다. 앞 절은 맞지만 뒤 절은 오해를 부른다. 자식 VM은 이 VM의 `AttributeViewModels`·`AbilityViewModels`·`ActiveEffectViewModels`(`WxViewModel_AbilitySystem.h:76`, `:79`, `:91`)가 붙들고, 자식은 Outer로 이 VM을 붙드는 **순환 참조**다. 언리얼 GC는 루트에서 도달 불가한 순환은 통째로 수거하므로, 실제 수명을 정하는 것은 이 그래프 **밖**의 참조(MVVMView의 소스, `UWxViewModel_Character::AbilitySystem`, 네임플레이트 컴포넌트의 `CharacterViewModel`)다. 코드 동작은 이 전제로 정확하지만, 설명을 그대로 믿으면 "위젯이 놓아도 자식이 있으니 살아 있다"고 잘못 추론해 재초기화·해제 판단을 틀리게 한다.
- **제안**: 주석을 "외부(뷰·상위 VM·컴포넌트) 참조가 하나라도 남아 있는 동안만 살아 있고, 부모-자식은 순환이라 함께 수거된다"로 정정한다.
- **확신도**: 중간(코드가 아니라 근거 서술의 정확성 문제다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp` (각 대응 헤더 포함)
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIDeveloperSettings.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이라 보지 않았다. 특히 네임플레이트는 이제 WBP가 **수동 주입 가능한**(`CanBeSet`) `UWxViewModel_Character` 소스를 선언해야만 값이 들어오는데(`WxNameplateComponent.cpp:82-89`), 그 조건이 `WBP_Nameplate_Enemy`에서 실제로 충족되는지는 에디터에서만 확인된다. 발견 1·3의 체감 비용도 마찬가지다.
  - `AWxIndicator::UpdateProjection`/`ClampToScreenEdge`의 투영·역투영 수식은 읽고 논리적 모순만 확인했고, 레터박스·울트라와이드 뷰포트에서의 좌표 정확성은 런타임 검증하지 않았다.
  - 빌드·PIE는 돌리지 않았다(리뷰 전용). 모듈 규칙은 전 파일 확인했고 위반은 발견 6 외에 없다 — `WxUI.Build.cs`·`WxUI.uplugin`은 `WxCore` 외 Wx 플러그인을 참조하지 않고 소스 어디에도 다른 도메인 플러그인 include가 없으며, `FORCEINLINE`·인라인 정의는 0건(StateTree `GetInstanceDataType()`만 예외 사유 주석과 함께 남아 있다), 소스 58파일 전부 첫 줄 저작권이 맞고(이번 변경에서 `WxUIManagerSubsystem`의 UTF-8 BOM도 제거됐다), 람다는 `WxUILibrary.cpp:23`의 동적→네이티브 델리게이트 어댑터 1건으로 정당한 사용으로 봤다.

---
*문서 기준 커밋 `231068b` · 리뷰일 2026-09-13 · 소스 58파일 — `/module-review`로 갱신*
