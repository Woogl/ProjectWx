# WxUI — 코드 리뷰

> 레이어·팝업·일시정지, 비동기 push 취소, 폰 교체 때 태그 관찰 교체, VM 공유와 파괴 경로 해제가 촘촘히 방어돼 있어 건강하다. AGENTS.md 3조항 위반은 없고, Build.cs·uplugin도 WxCore 외 Wx 모듈에 의존하지 않는다. 남은 발견은 원격 클라이언트에서만 드러나는 슬롯 재매칭 누락 1건이다. 자막 VM이 엔진 글로벌 컬렉션에 스스로 등록하는 구조는 사용자 결정(2026-09-24, `UWxUIManagerSubsystem`은 Subtitle을 모른다)으로 유지한다. 커버리지: WxUI 소스 61파일 중 서브시스템·레이아웃·비동기 push·VM 전부·StateTree 태스크 2종은 cpp까지 읽었다. GAS·CommonUI·GameMode 동작은 UE 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 원격 클라이언트에서는 어빌리티 부여·회수가 슬롯 재매칭을 부르지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:41`
- **범주**: 버그/정확성
- **문제**: 슬롯 재매칭(`FlushAbilityRebind`, `:283`)을 부르는 통로는 `AbilitySpecDirtiedCallbacks` 구독(`:41`, `HandleAbilitySpecDirtied` `:263`)뿐이다. 그런데 엔진은 이 델리게이트를 권위 머신에서만 방송한다(UE 5.8 `AbilitySystemComponent_Abilities.cpp:1010`~`1017`, 비권위는 `:1019`~`1022`에서 `MarkArrayDirty`만 한다). 클라이언트 복제 경로(`GameplayAbilityTypes.cpp:253` `OnRemoveAbility`, `:295` `OnGiveAbility`)도 이 델리게이트를 방송하지 않는다. 그래서 리슨 서버의 원격 클라이언트에서는 헤더 `WxViewModel_AbilitySystem.h:25`의 "부여가 바뀌면 재매칭" 계약이 성립하지 않는다. 재매칭은 슬롯 VM이 아무 태그 변화에나 거는 `FlushActivationRefresh` → `RefreshBoundAbility`(`WxViewModel_Ability.cpp:36`, `:525`)에만 기댄다. 실패 시나리오는 두 가지다. 첫째, 폰 복제 직후 HUD가 생길 때(`WxViewModel_Ability.cpp:41`) 스펙이 아직 복제되지 않았으면 다음 태그 변화까지 슬롯이 비어 있다. 둘째, 태그 변화 없이 서버가 어빌리티를 바꾸면(무기 교체 등) 옛 스킬이 계속 표시된다. 전투 중에는 태그가 자주 바뀌어 문제가 가려지고, 리슨 호스트와 스탠드얼론에서는 드러나지 않는다.
- **제안**: WxUI는 고치지 않고 신호를 내는 쪽에 통로를 둔다. WxCombat `UWxAbilitySystemComponent`에서 클라이언트 복제 경로에서도 불리는 `OnGiveAbility`·`OnRemoveAbility`(UE 5.8 `AbilitySystemComponent.h:1695`, `:1698`)를 오버라이드한다. `IsOwnerActorAuthoritative()`가 거짓일 때만 같은 `AbilitySpecDirtiedCallbacks`를 방송한다. 새 의존은 생기지 않는다. 재매칭은 다음 틱에 돌므로 제거 직전에 방송해도 된다. 다만 엔진이 권위에서만 쓰는 델리게이트를 클라에서도 방송하므로 순정 의미를 넓히는 셈이다. 채택 전까지는 헤더 `:25` 주석에 권위 머신 한정이라고 적는다.
- **확신도**: 중간 — 엔진 방송 조건은 소스로 확인했다. 원격 클라이언트 인게임 재현은 하지 않았다.

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxConfirmationPopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxGamePopup.cpp`, `Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxStateTreeTask_MarkIndicator.cpp` 및 각 헤더
- **훑은 파일**: WxUI의 나머지 소스(`WxActivatableWidget`, `WxButtonBase`, `WxMVVMConversionLibrary`, `WxViewModelResolver_PlayerCharacter`, `WxViewModel_Item`·`Quest`·`QuestObjective`·`Dialogue`·`Indicator`·`Interaction`, `WxIndicatorWidget.h`, `WxSubtitleTableRow.h`, `WxUIDeveloperSettings`, `WxUIModule`), `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, 호출부 확인용 `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`·`Source/WxGame/MVVM/WxViewModelResolver_Quest.cpp`, UE 5.8 엔진 소스(`Controller.cpp`, `LocalPlayer.cpp`, `GameModeBase.cpp`, `AbilitySystemComponent_Abilities.cpp`, `AbilitySystemComponent.h`, `GameplayAbilityTypes.cpp`, `GameplayEffect.cpp`, `CommonInputModeTypes.h`)
- **미검토 / 한계**: WBP·BP 내부(MVVM 바인딩, 위젯 계층)는 범위 밖이며, 에셋 사용 여부는 Content 바이너리 문자열 검색으로만 확인했다. CommonUI 스택 전이 순서(비활성 방송과 다음 위젯 활성 사이의 일시적 해제·재정지)는 최종 상태가 수렴한다고 보고 엔진 파일까지 대조하지 않았다. Lyra 원본 파일은 대조하지 않았다. PIE·리슨 서버·원격 클라이언트 실행 검증은 하지 않았다.

---
*문서 기준 커밋 `4c9ee67d3` · 리뷰일 2026-09-24 · 소스 61파일 — `/module-review`로 갱신*
