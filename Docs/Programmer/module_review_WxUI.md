# WxUI — 코드 리뷰

> 뷰모델 계층의 수명주기·재진입·코얼레싱 처리가 전반적으로 성숙하다. 구독 해제, 파괴 중 브로드캐스트 억제, 티커 핸들 정리 같은 함정이 대부분 의식적으로 다뤄져 있고 그 이유가 주석으로 남아 있다. 다만 이번 작업 트리에서 `UWxNameplateComponent`를 지우고 그 역할을 MVVM 리졸버+WBP 바인딩으로 옮기는 전환이 마무리되지 않았고, 새 네임플레이트 경로가 모듈에서 가장 약한 지점이다. 커버리지: 최근 변경된 MVVM 6쌍과 `WxUIManagerSubsystem`·`WxAsyncAction_PushWidgetToLayer`·`WxIndicator`를 cpp까지 정독했고, 위젯 베이스·팝업·StateTree 노드·자막은 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 삭제된 `UWxNameplateComponent`를 아직 참조하는 에셋 7개
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Component/`(클래스 삭제됨) / 참조 잔존: `Content/Character/Minion/BP_Minion.uasset`, `Content/Character/Soldier/BP_Soldier.uasset`, `Content/Character/Sandbag/BP_Sandbag.uasset`, `Content/Character/Template/TemplateEnemy/BP_TemplateEnemy.uasset`, `Content/__ExternalActors__/Maps/LV_DevCombat/2/RU/NRCPT86OI84K9H87YKQM54.uasset`, `Content/__ExternalActors__/Maps/LV_OpenWorld/4/WD/DO20GLFT73YO7QI7GSTFRB.uasset`, `Content/__ExternalActors__/Maps/LV_OpenWorld/8/MP/2X2L249SMQSC8KSBTKX9V2.uasset`
- **범주**: 버그/정확성
- **문제**: C++ 쪽 정리는 끝나 있다 — `Source/WxGame/Character/WxEnemyCharacter.h:59`는 이제 `TObjectPtr<UWidgetComponent> NameplateComponent`이고 `WxEnemyCharacter.cpp:31`도 `CreateDefaultSubobject<UWidgetComponent>`로 바뀌었으며, WxUI 안에 끊긴 include·전방선언·데드 코드는 하나도 남아 있지 않다. 그런데 위 7개 에셋에는 여전히 `/Script/WxUI` + `WxNameplateComponent` 임포트 문자열이 그대로 들어 있다(바이너리 grep으로 확인). 즉 BP 4종은 상속 컴포넌트 오버라이드(InheritableComponentHandler) 레코드가, 배치 액터 3종은 인스턴스별 프로퍼티 오버라이드가 사라진 네이티브 클래스를 가리킨다. 다음 에디터 로드에서 해당 레코드는 "클래스를 찾을 수 없음"으로 버려지고, 저작해 둔 `MaxVisibilityDistance`·`VisibilityRequirements`·`ReferenceDistance` 같은 값이 조용히 유실된다. 메모리에 기록된 `renamed-component-cdo-ghost`·`wp-actordesc-stale-native-class`가 정확히 이 상황이다.
- **제안**: 커밋 전에 정리한다. 기록된 절차대로 스텁 클래스를 잠시 되살려 위 BP·외부 액터를 로드→재저장해 참조를 떨군 뒤 클래스를 최종 삭제하거나, 최소한 로드 로그에서 유실 범위를 확인하고 네임플레이트 튜닝값을 새 WBP 바인딩 쪽으로 옮겨 두고 나서 지운다.
- **확신도**: 높음(참조 잔존은 확인됨. 로드 시 실제 유실 범위는 에디터에서 확인 필요)

### 2. 🟡 네임플레이트 리졸버가 위젯→컴포넌트 역조회에 전역 오브젝트 순회를 쓴다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_NameplateCharacter.cpp:32-40`
- **범주**: 성능/안전
- **문제**: `TObjectIterator<UWidgetComponent>`로 메모리에 있는 모든 `UWidgetComponent`를 훑어 `GetWidget() == RootWidget`인 것을 찾는다. `TObjectIterator` 생성마다 해당 클래스의 전체 오브젝트 배열이 새로 만들어지고, WBP가 뷰모델 소스를 둘(`UWxViewModel_Nameplate` + `UWxViewModel_Character`) 선언하면 네임플레이트 하나당 이 순회가 두 번 돈다. 인디케이터 액터도 `UWidgetComponent`를 쓰므로 같은 풀에 섞인다. 결과적으로 적 N마리가 스트리밍-인 할 때 게임 스레드에서 O(N²) 탐색과 2N번의 배열 할당이 발생한다 — 오픈월드에서 히치로 드러나기 좋은 모양이다.
- **제안**: 역조회를 없앤다. 컴포넌트가 위젯을 만든 직후 자기 자신을 위젯에 밀어 넣고(예: 위젯이 구현하는 좁은 인터페이스나 `UUserWidget` 파생 베이스의 세터), 리졸버는 그 값을 읽기만 하게 한다. 역조회를 유지해야 한다면 최소한 결과를 한 번만 구해 두 소스가 나눠 쓰도록 묶는다.
- **확신도**: 중간(의도된 타협일 수 있으나, 순회 비용 자체는 코드에서 그대로 읽힌다)

### 3. 🟡 네임플레이트마다 코어 티커 하나가 매 프레임 태그 컨테이너를 다시 만든다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Nameplate.cpp:14-19`, `:47-81`
- **범주**: 성능/안전
- **문제**: 네임플레이트 VM마다 `FTSTicker` 항목을 하나씩 등록하고, 그 안에서 매 프레임 `UGameplayStatics::GetPlayerPawn` 조회 + 거리 계산 + `GetOwnedGameplayTags(NewTags)`로 소유 태그 컨테이너를 통째로 새로 만들어 비교한다(`:55-59`). 태그 컨테이너 재구성은 프레임당·적당 힙 할당이다. 게다가 `Distance` 변경 판정 임계값이 `UE_DOUBLE_KINDA_SMALL_NUMBER`라 플레이어가 움직이는 동안에는 사실상 매 프레임 `Distance` 브로드캐스트가 나가고, 그때마다 WBP의 가시성·스케일 컨버전 바인딩이 재실행된다. 코어 티커라 화면 밖이든 Collapsed든 정지 중이든 멈추지 않는다(`:16` 주석대로 의도된 선택이다).
- **제안**: 티커를 VM마다 두지 말고 등록된 네임플레이트를 한 번에 도는 공용 업데이터(서브시스템) 하나로 모아 플레이어 폰 조회를 프레임당 1회로 줄인다. 태그는 매 프레임 스냅샷 비교 대신 소유자 ASC의 태그 변경 이벤트로 갱신하고(다른 VM들이 이미 쓰는 방식), `Distance`는 표시에 의미 있는 단위(예: 0.1m)로 양자화해 브로드캐스트 빈도를 낮춘다.
- **확신도**: 중간(코스트는 확실하나, 적 수가 적은 현 단계에선 문제로 드러나지 않을 수 있다)

### 4. 🟡 사망 화면 푸시만 취소·재확인 경로가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:269-280`
- **범주**: 버그/정확성
- **문제**: 바로 아래 대화 화면 경로(`:282-297`, `:307-323`, `:325-339`)는 진행 중인 푸시를 `PendingDialogueScreenPush`로 붙들고, 완료 시 태그가 아직 살아 있는지 다시 확인하고, 태그가 걷히면 `CloseDialogueScreen()`으로 취소·비활성화한다. 사망 화면은 셋 다 없다 — 푸시 액션을 기억하지 않고, 완료 시 `Ability.Death`를 재확인하지 않으며, 사망 화면을 닫는 코드는 모듈 어디에도 없다. 닫는 주체가 `UWxRespawnLibrary::RequestRespawn`(`Source/WxGame/Framework/WxRespawnLibrary.cpp:79`) 하나뿐인데, 그건 사망 화면 위젯 자신을 인자로 받으므로 화면이 이미 떠 있어야만 동작한다. 따라서 최초 로드(비동기 스트리밍) 중에 사망 태그가 다른 경로로 걷히면, 뒤늦게 뜬 사망 화면을 아무도 닫지 못한다. Menu 레이어가 활성인 채 굳으면 `IsMenuLayerActive()`가 true로 남아 인디케이터가 계속 숨고(`WxIndicator.cpp:111`) `ShouldPauseGame` 설정에 따라 정지까지 남는다. 폰이 교체될 때도 `WatchPawnTags`(`:240-267`)는 대화 화면만 걷고 사망 화면은 놓아둔다.
- **제안**: 대화 화면과 같은 모양으로 맞춘다 — 진행 중인 푸시를 멤버로 붙들고, 완료 콜백에서 `WatchedAbilitySystem`의 `Ability.Death`를 재확인해 이미 걷혔으면 즉시 `DeactivateWidget`하며, `WatchPawnTags`의 정리 지점에서 사망 화면도 함께 닫는다.
- **확신도**: 중간(현 respawn 흐름만 보면 재현이 좁지만, 방어가 한쪽에만 있는 비대칭은 분명하다)

### 5. 🟡 공유 Character 뷰모델을 집는 방법이 세 가지로 갈려 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:8-21`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp:23-31`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_NameplateCharacter.cpp:53-60`
- **범주**: 중복/복잡도
- **문제**: 같은 일(ASC를 Outer로 하는 공유 Character VM을 찾고 없으면 만들어 초기화)을 세 군데가 다르게 한다. `GetOrCreate`는 find-then-create를 이미 품고 있는데, 네임플레이트 리졸버는 `FindSharedViewModel`을 한 번 더 부른 뒤 `GetOrCreate`를 호출해 조회가 이중으로 돌고, 플레이어 리졸버는 `GetOrCreate`를 건너뛰고 `NewObject`를 직접 부른다. "찾아진 것은 이미 초기화돼 있다"와 "새로 만든 것만 `Initialize`한다"는 규약이 호출부마다 손으로 재현되고 있어, 새 리졸버가 하나 붙을 때 공유본을 재초기화해 남의 바인딩을 끊는 사고(`WxViewModel_Character.cpp:25`가 `Deinitialize()`부터 부른다)가 나기 쉽다.
- **제안**: 규약을 `UWxViewModel_Character`로 끌어올린다. "없으면 만들고 이번에 만든 경우에만 초기화한다"를 한 팩토리 함수가 책임지게 하고(표시 소스는 인자로 받는다), 두 리졸버는 그것만 호출한다.
- **확신도**: 높음

### 6. 🟢 `KillPopup` / `EWxPopupResult::Killed` 는 호출자가 없다
- **위치**: `Plugins/WxUI/Source/WxUI/Public/Widget/WxGamePopup.h:15`, `:70`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp:91-93`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp:80-84`
- **범주**: 중복/복잡도
- **문제**: `KillPopup()`을 부르는 코드가 모듈·프로젝트 전체에 없다(재정의와 `Super` 호출뿐). 따라서 `EWxPopupResult::Killed`도 발생할 수 없는 결과값이고, 이를 처리하려고 분기해 둔 WBP 쪽 코드가 있다면 영영 죽은 가지다. 팝업을 밖에서 강제 종료하는 기능이 아직 없다는 뜻이라, 있는 척하는 API가 오히려 오해를 만든다.
- **제안**: 강제 종료 기능을 실제로 쓸 계획이 없으면 `KillPopup`과 `Killed`를 지운다. 쓸 계획이면 `UWxUIManagerSubsystem`에 호출 지점을 만들어 계약을 완성한다.
- **확신도**: 높음

### 7. 🟢 `UWxViewModel_Attribute::Initialize` 의 유효성 재검사가 죽은 분기다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp:14`, `:19`, `:22`, `:28`, `:33`
- **범주**: 중복/복잡도
- **문제**: `:14`에서 `!InAttribute.IsValid()`면 이미 반환하는데 `:22`가 `if (InAttribute.IsValid())`로 다시 감싼다. `:19`에서 `InMaxAttribute`를 유효한 값으로 확정한 뒤 `:28`·`:33`이 다시 `InMaxAttribute.IsValid()`를 묻는다 — 셋 다 항상 참이다. 읽는 사람에게 "여기선 없을 수도 있다"는 잘못된 신호를 준다.
- **제안**: 죽은 조건을 걷어내고 본문을 평평하게 편다.
- **확신도**: 높음

### 8. 🟢 티커 델리게이트 콜백 일부에 `Handle` 접두가 빠져 있다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:41`(`UpdateCooldownState`), `:411`(`FlushActivationRefresh`), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:246`(`FlushOwnedTagsRefresh`), `:259`(`FlushAbilityRebind`), `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp:68`(`UpdateEffectState`)
- **범주**: 규칙 위반
- **문제**: 델리게이트 콜백은 `Handle` 접두를 붙이는 규칙인데 위 다섯은 붙어 있지 않다. 같은 모듈의 `WxViewModel_Nameplate.cpp:18`은 같은 성격의 티커 콜백을 `HandleUpdatePresentation`으로 두고 있어 모듈 안에서도 갈린다. 다만 `UpdateCooldownState`는 `WxViewModel_Ability.cpp:239`에서 직접 호출되기도 해 순수 콜백은 아니다.
- **제안**: 델리게이트로만 쓰이는 `FlushOwnedTagsRefresh`·`FlushAbilityRebind`·`FlushActivationRefresh`·`UpdateEffectState`에 `Handle` 접두를 붙여 맞춘다.
- **확신도**: 중간(직접 호출을 겸하는 함수는 예외로 볼 여지가 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Nameplate.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_NameplateCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxMVVMConversionLibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp` (각 대응 헤더 포함)
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Indicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Interaction.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxButtonBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`
- **미검토 / 한계**:
  - WBP 내부(위젯 계층·MVVM 바인딩 행·이벤트 그래프)는 범위 밖이라 보지 않았다. 발견 2·3의 실제 체감 비용과 발견 1의 유실 범위는 에디터에서 확인해야 확정된다.
  - `AWxIndicator::UpdateProjection`/`ClampToScreenEdge`의 투영·역투영 수식은 읽고 논리적 모순만 확인했고, 레터박스·울트라와이드 같은 뷰포트 형상에서의 실제 좌표 정확성은 런타임 검증하지 않았다.
  - 빌드·PIE는 돌리지 않았다(리뷰 전용). 모듈 규칙(플러그인 참조 DAG, 첫 줄 저작권, 인라인 정의 금지, 람다 남용)은 전 파일 grep으로 확인했고 위반은 발견 8 외에 없다 — `WxUI.Build.cs`는 `WxCore` 외 Wx 플러그인을 참조하지 않고, `FORCEINLINE`/인라인 정의는 StateTree `GetInstanceDataType()` 예외(사유 주석 있음)뿐이며, 람다는 `WxUILibrary.cpp:23`의 동적→네이티브 델리게이트 어댑터 1건으로 정당한 사용으로 봤다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 60파일 — `/module-review`로 갱신*
