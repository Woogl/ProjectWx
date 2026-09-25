# WxUI — 코드 리뷰
상태: 확인 대기 · 코드 리뷰 결과 판단 대기
다음 행동: 원격 클라이언트 슬롯 재매칭과 HUD 제거 시 메뉴 요청 취소의 수정 여부를 판단한다.

> 화면 레이어·비동기 push·공유 VM·일시정지의 핵심 C++를 검토했다. 원격 클라이언트의 슬롯 재매칭 신호와 HUD 교체 중 메뉴 요청 수명에 미해결 사항 2건이 있다.

## 요청

`module-review`로 현재 WxUI를 정적 검토하고 기존 미해결 지적을 현재 코드와 대조한다. 소스 수정은 포함하지 않는다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 어빌리티 복제 완료가 슬롯 재매칭을 알리지 않는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:39`
- **범주**: 버그/정확성
- **문제**: 목록 변경에 따른 재매칭은 `AbilitySpecDirtiedCallbacks`를 받아 다음 틱의 `FlushAbilityRebind`로 이어진다(`:258`, `:278`). UE 5.8 `AbilitySystemComponent_Abilities.cpp:1010`·`:1017`은 이 델리게이트를 권위 머신에서만 방송한다. `GameplayAbilityTypes.cpp:233`·`:277`의 클라이언트 제거·추가 복제 경로는 이를 방송하지 않고, 프로젝트 ASC에도 `OnGiveAbility`·`OnRemoveAbility`를 통한 보완 처리가 없다. 원격 클라이언트에서 HUD가 스펙 복제보다 먼저 생성되면 슬롯은 빈 채로 만들어지고, 스펙이 도착해도 재매칭하지 않는다. 이후 별도 태그 변경이나 ActionPhaseChanged 이벤트가 발생해야 `WxViewModel_Ability.cpp:497`을 통해 재매칭하므로 입력 가능한 스킬이 있어도 빈 표시가 유지될 수 있다.
- **제안**: 클라이언트의 부여·회수 완료까지 포함하는 변경 신호를 마련해 슬롯 재매칭에 연결한다. 기존 엔진 델리게이트를 재사용한다면 권위 전용이라는 기존 의미가 바뀌는 점을 명시한다.
- **확신도**: 높음 — 현재 구독 경로와 UE 5.8 방송 조건을 대조했다. 원격 클라이언트 실행 재현은 하지 않았다.

### 2. 🟡 HUD가 교체돼도 진행 중인 메뉴 로드가 살아남는다
- **위치**: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp:87`
- **범주**: 버그/정확성
- **문제**: HUD는 요청을 `PendingMenuPush`에 보관하고 완료 시 비우지만(`:95`), 비활성화·제거 때 취소하지 않는다. 폰 교체는 `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp:51`의 `ClearLayout`에서 기존 HUD를 비활성화하고 스택에서 제거하지만(`:99`, `:102`), 취소 대상은 HUD 자체를 만드는 `PendingLayoutPush`뿐이다(`:92`). 메뉴 요청은 `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp:18`에서 GameInstance에 등록되고 이전 HUD를 `WorldContextObject`로 강하게 보유한다(`Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h:49`). 같은 PC의 폰만 교체되면 PrimaryGameLayout이 같아 완료 시 `WxAsyncAction_PushWidgetToLayer.cpp:100` 검사도 통과한다. 메뉴 최초 로드 중 폰이 교체되면 이미 제거된 HUD의 요청이 뒤늦게 새 HUD 위에 메뉴를 띄운다.
- **제안**: HUD가 제거되거나 비활성화될 때 `PendingMenuPush->Cancel()`로 진행 중 요청을 정리한다. 완료 시 현재 HUD의 요청인지 확인하는 방법도 함께 검토한다.
- **확신도**: 높음 — 요청의 참조와 레이아웃 검사, 폰 교체 정리 범위를 C++에서 대조했다. 실제 로드 지연 실행 재현은 하지 않았다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 기존 지적 대조 | 현재 구독·비동기 요청 경로와 UE 5.8 엔진 소스를 대조한다 | AI | 통과 | 미해결 2건을 재확인하고 `WxViewModel_Ability.cpp:207`의 무효 참조 초기화 수정으로 해결된 지적은 제외했다 |
| 리뷰 결과 판단 | 위 2건의 실패 조건과 제안을 검토해 후속 수정 범위를 정한다 | 사람 | 대기 | |
| 원격 클라이언트 표시 | HUD를 먼저 만든 뒤 스펙을 늦게 복제해 슬롯 표시를 확인한다 | 사람 | 미실행 | 정적 리뷰만 수행했다 |
| HUD 교체 중 메뉴 요청 | 메뉴 로드를 지연시킨 상태에서 같은 PC의 폰을 교체하고 오래된 메뉴의 push를 확인한다 | 사람 | 미실행 | 정적 리뷰만 수행했다 |

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Attribute.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`.
- **훑은 파일**: `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/WxUI.uplugin`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxHUDLayout.h`, `Plugins/WxUI/Source/WxUI/Public/Widget/WxAsyncAction_PushWidgetToLayer.h`, `Plugins/WxUI/Source/WxUI/Private/Widget/WxActivatableWidget.cpp`. 연동 신호 확인을 위해 프로젝트 ASC의 부여·회수 관련 심볼과 UE 5.8 `AbilitySystemComponent_Abilities.cpp`·`GameplayAbilityTypes.cpp`의 관련 구간을 검색·대조했다.
- **미검토 / 한계**: 현재 작업 트리를 정적으로 확인했다. WBP·BP 내부와 에셋 설정, 자막·인디케이터·팝업·단순 표시 VM은 이번에 깊게 재검토하지 않았다. 빌드·자동화 테스트·PIE·리슨 서버·원격 클라이언트·로드 지연 실행 검증은 하지 않았다. CommonUI 내부 전체 전이 순서도 통독하지 않았다. 단일 로컬 플레이어 전제는 기존 계약으로 유지한다.

---
*문서 기준 커밋 `ad0db6de0` · 리뷰일 2026-09-26 · 소스 59파일 — `/module-review`로 갱신*
