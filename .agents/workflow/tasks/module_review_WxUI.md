# WxUI — 코드 리뷰

> 복제 경로 빙의에서도 플레이어 Character VM이 새 ASC로 다시 채워지고 폰 교체로 걷힌 HUD의 메뉴 요청도 취소된다. 남은 틈은 원격 클라이언트의 슬롯 재매칭 신호와 버려진 AbilitySystem VM 트리의 정리, 두 가지다.
> `0260099b1` 위의 미커밋 변경(Character·AbilitySystem VM, HUD 레이아웃, WxGame `SetPawn`)과 그 호출자, 비동기 push 취소 경로, UE 5.8 엔진 경로를 깊게 봤다. 변경이 없는 나머지 영역은 직전 리뷰 결과를 이어받았다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 원격 클라이언트에서 어빌리티 복제 완료가 슬롯 재매칭을 알리지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:16`
- **범주**: 버그/정확성
- **문제**: 부여 목록이 바뀌면 `AbilitySpecDirtiedCallbacks` 구독(`:16`)이 다음 틱의 `FlushAbilityRebind`(`:207`, `:227`)를 불러 슬롯을 다시 매칭한다. 그런데 UE 5.8 `AbilitySystemComponent_Abilities.cpp:1010`·`:1017`은 이 델리게이트를 권위 머신에서만 방송한다. 클라이언트의 복제 제거·추가 경로(`GameplayAbilityTypes.cpp:253`·`:295`)는 `OnRemoveAbility`·`OnGiveAbility`만 부르고, 프로젝트에는 이를 재정의해 보완하는 코드가 없다. 원격 클라이언트에서 슬롯 VM이 스펙 복제보다 먼저 만들어지거나 슬롯 어빌리티가 런타임에 부여·회수되는 경우가 문제다. 그러면 슬롯은 이전 매칭(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:111`)에 머물고, 그 ASC에서 태그 변경이나 ActionPhaseChanged 이벤트(`WxViewModel_Ability.cpp:220`·`:225`)가 와야 `:296`에서 다시 고른다. 그런 신호 없이 조용한 상태가 이어지면 입력 가능한 스킬이 빈 칸이나 옛 스킬로 보인다. 사용자는 2026-09-30 이 수정을 보류했다([작업 기록](wxui-review-possession-menu-fix.md)). 지금은 슬롯 어빌리티를 런타임에 부여·회수하는 경로가 없어 드러나지 않는다는 판단이다.
- **제안**: 스킬 교체·장비 부여를 도입하거나 멀티플레이를 정식 지원할 때 다시 본다. 그때는 클라이언트에서 부여·회수가 끝난 것도 권한과 무관하게 알리는 신호를 슬롯 재매칭에 연결한다(예: 프로젝트 ASC가 `OnGiveAbility`·`OnRemoveAbility`를 재정의해 방송).
- **확신도**: 높음 — 구독 경로를 UE 5.8 방송 조건과 대조했다. 원격 클라이언트에서 실행해 재현하지는 않았다.

### 2. 🟡 Character VM이 놓거나 버린 AbilitySystem VM 트리가 ASC 구독과 매 틱 타이머를 GC 때까지 유지한다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp:39`
- **범주**: 성능/안전
- **문제**: `Initialize`는 ASC가 바뀌면 새 AbilitySystem VM을 만들고(`:39`), `Deinitialize`는 참조만 비운다(`:47`). 놓인 AbilitySystem VM과 그 자식에는 해제 경로가 없다. 그래서 GC 전까지 이런 일이 계속된다.
  - ASC 구독을 유지한다(`WxViewModel_AbilitySystem.cpp:15`·`:16`·`:43`·`:44`, `WxViewModel_Ability.cpp:25`~`:32`, `WxViewModel_Attribute.cpp:26`·`:28`).
  - 태그가 바뀔 때마다 다음 틱 타이머를 건다(`WxViewModel_AbilitySystem.cpp:204`, `WxViewModel_Ability.cpp:245`).
  - 효과 남은 시간 타이머(`WxViewModel_Effect.cpp:102`)와 쿨다운 타이머(`WxViewModel_Ability.cpp:51`)를 매 프레임 다시 건다.
  - 새 효과가 붙으면 효과 VM을 만들고 아이콘을 스트리밍한다.

  같은 ASC로 다시 부르면 이제 트리를 유지하므로(`:26`), 조건 없이 도는 플레이어 `SetPawn` 갱신은 트리를 늘리지 않는다. 반면 살아 있는 적 ASC를 대상으로 한 교체는 그대로 트리를 버린다. 보스가 바뀌거나 빠질 때(`Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp:24`·`:28`)가 그렇다. 네임플레이트도 붙일 때마다 새 Character VM을 만들고(`Source/WxGame/Controller/WxNameplateManagerComponent.cpp:127`), 파괴할 때(`:106`) `Deinitialize` 없이 위젯과 함께 버린다. 보스 바 위젯을 파괴할 때도 `DestroyInstance`는 보스 구독만 끊는다(`WxViewModelResolver_BossCharacter.cpp:42`). 그래서 교전 중에는 쓸모없는 구독과 타이머가 GC 주기 동안 쌓인다.
- **제안**: AbilitySystem VM에 정리 함수를 둔다. 이 함수는 ASC 델리게이트를 `RemoveAll`로 끊고 자식 VM의 타이머·구독도 정리한다(효과 VM은 기존 `Deinitialize` 재사용). Character VM은 트리를 놓을 때 이 함수를 부른다. 네임플레이트 파괴와 보스 바 `DestroyInstance`에서는 Character VM의 `Deinitialize`를 불러 같은 정리를 거치게 한다.
- **확신도**: 중간 — 구독과 타이머가 약참조라 GC 뒤에는 멈춘다. 그래서 영향은 GC 주기 동안의 낭비로 한정되고, 실제로 얼마나 쌓이는지는 측정하지 않았다.

## 검토 범위
- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Character.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`. 연동 확인으로 `Source/WxGame/Controller/WxPlayerController.cpp`도 봤다.
- **훑은 파일**:
  - WxUI: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`(스택 전환 시간 0).
  - 호출자: `Source/WxGame/MVVM/WxGameViewModelUtils.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/Controller/WxNameplateManagerComponent.cpp`, `Source/WxGame/Battle/WxBattleSubsystem.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`(ASC 소유 위치).
  - 엔진 근거(UE 5.8): `CommonActivatableWidgetContainer.cpp`, `SCommonAnimatedSwitcher.cpp`, `SObjectWidget.cpp`, `UserWidgetPool.cpp`, `StreamableManager.cpp`, `LaunchEngineLoop.cpp`, `GameEngine.cpp`, `EditorEngine.cpp`, `AbilitySystemComponent_Abilities.cpp`, `GameplayAbilityTypes.cpp`.
  - AGENTS.md 규칙: 63파일 전체에서 첫 줄 저작권 문구와 `FORCEINLINE`·`inline`을 검색했고, 변경 파일은 인라인 정의와 Wx 접두사까지 확인했다. 위반은 없다.
- **미검토 / 한계**:
  - 이번 리뷰는 커밋 `0260099b1` 위의 미커밋 작업 트리 변경(WxUI 6파일과 `Source/WxGame/Controller/WxPlayerController.cpp`)을 포함해 봤다.
  - 정적 리뷰다. 빌드·PIE·원격 클라이언트 실행은 직접 하지 않았고, 작업 기록의 검증 결과는 참고만 했다.
  - HUD 메뉴 요청 취소는 `NativeDestruct`에서 걸리는데, 이 함수는 Slate 위젯이 코어 티커에서 풀릴 때 불린다(`CommonActivatableWidgetContainer.cpp:257`·`:264`). 코어 티커는 틱 객체 단계보다 늦게 돈다. 그래서 스트리밍 완료 델리게이트가 같은 프레임의 틱 객체 단계에서 먼저 실행되는 한 프레임 창이 남는다. 지적할 수준은 아니라고 봤다.
  - 변경이 없는 나머지 영역(화면 레이어·일시정지·인디케이터·퀘스트·대화·자막 등)은 `e09ed19d1` 리뷰에서 봤고 다시 통독하지 않았다.
  - WBP·BP 내부는 범위 밖이다.

---
*문서 기준 커밋 `0260099b1` · 리뷰일 2026-09-30 · 소스 63파일 — `/module-review`로 갱신*
