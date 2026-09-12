# WxUI — ViewModel 코드 리뷰

> ViewModel의 공유 생성, 재초기화, FieldNotify 및 GAS 관찰 경로를 검토했다. 기존 초기화·종료 문제는 수정되었으며, 현재 전투 단계 변화가 발동 가능 표시로 전달되지 않는 문제 1개를 확인했다. 전체 WxUI 모듈 리뷰를 대체하지 않는다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 콤보 창·후딜 전환이 CanActivate에 반영되지 않는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:402`, `:417`, `:503`; `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:61`, `:86`, `:118`, `:134`
- **범주**: 버그/정확성
- **문제**: Ability VM은 태그 변경, 비용 Attribute 변경, 쿨다운 적용·충전 변화 때 발동 가능 여부를 다시 계산한다. 그러나 실제 판정은 `ActionPhase`도 읽는다. `OpenComboWindow`는 자기 콤보 재발동을 허용하고 `StartRecovery`는 다른 배타 어빌리티에 대한 점유를 해제하지만, 두 함수는 phase 대입과 입력 버퍼 재시도만 수행하며 태그나 VM 갱신을 발행하지 않는다. `WxAnimNotifyState_ComboWindow.cpp:26` 및 `WxAnimNotify_StartRecovery.cpp:26`의 호출부에도 별도 통지가 없다. 예를 들어 비용·쿨다운이 없고 추가 태그 변화가 없는 배타 공격을 발동한 뒤 VM의 예약 갱신이 실행되어 `CanActivate=false`가 된 상태에서, 입력 버퍼를 비운 채 콤보 창 또는 후딜에 진입하면 실제 `CanActivateAbility`는 true가 될 수 있지만 VM은 false를 유지한다. 반대로 창 안에서 다른 신호로 true가 갱신된 뒤 `CloseComboWindow`만 실행해도 false로 복귀하지 않는다. 이 필드를 바인딩한 발동 가능 표시는 다음 우연한 갱신까지 실제 판정과 어긋난다. 특정 WBP의 클릭 차단 여부는 확인하지 않았으며, 삭제된 AttackButton의 입력 문제로 판정하지 않는다.
- **제안**: 전투 단계 변경을 기존 게임 조립 경로 또는 WxCore 공용 계약으로 중계해 해당 ASC의 발동 가능 표시를 재평가한다. WxUI에서 WxCombat을 직접 참조하지 않는다. 단순히 Spec dirty 신호만 보내는 것으로 끝내면 부모는 `RefreshBoundAbility`만 호출하고 동일 CDO에서 조기 반환하므로, 발동 판정까지 실제로 갱신되는지 확인해야 한다. 추가 태그·자원 변화 없이 Blocking → ComboWindow → Blocking 및 Blocking → Recovery를 전환하는 회귀 사례가 적합하다.
- **확신도**: 높음

## 이전 리뷰 반영 및 유지된 사용 조건

- AbilitySystem의 Initialize는 private로 제한되었다. Ability·Attribute·Effect는 살아 있는 객체의 표시 초기화를 FieldNotify로 전달하고 Attribute의 Max 생략은 Current로 통일되었다. 이전 문서의 해결된 초기화 문제는 재지적하지 않는다.
- `.codex/worklog/2026-09-12-ViewModel-개선-범위-축소.md`에 승인된 대로 Ability의 Spec 재매칭과 Effect 제거는 부모 AbilitySystem이 관리한다. 독립 관찰, 공유본 재개, Resolver 파생 타입 지원 및 순정 ASC의 제거·클라이언트 복제 관찰 확장은 유지된 제약이며 이번 발견에 포함하지 않는다.
- 공유 AbilitySystem은 `GetOrCreate(ASC)`로 얻고 `Deinitialize`는 공유본 전체의 사용 종료에만 호출한다. 같은 ASC의 여러 화면이 공유하는 객체를 개별 위젯 종료 시 중단해서는 안 된다.

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Public/MVVM/`의 `WxViewModel.h`, `WxViewModel_Ability.h`, `WxViewModel_AbilitySystem.h`, `WxViewModel_Attribute.h`, `WxViewModel_Character.h`, `WxViewModel_Effect.h`, `WxViewModel_Indicator.h`, `WxViewModel_Interaction.h`, `WxViewModel_Item.h`, `WxViewModel_Subtitle.h` 및 `Plugins/WxUI/Source/WxUI/Private/MVVM/`의 대응 cpp 10개. Attribute·Subtitle Resolver와 `WxMVVMConversionLibrary.h/.cpp`도 확인했다.
- **깊게 본 소비·판정 경로**: `Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, `Plugins/WxUI/Source/WxUI/Private/Subtitle/WxStateTreeTask_PrintSubtitle.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, 같은 디렉터리의 `WxAbility_Guard.cpp`, `WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp`, 같은 디렉터리의 `WxAnimNotify_StartRecovery.cpp` 및 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp:107`.
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp`의 VM 주입·갱신 지점, `Source/WxGame/MVVM/WxViewModel_BossDisplay.cpp`의 Character 초기화 지점. UE 5.8 로컬 `AbilitySystemComponent_Abilities.cpp:1008`에서 Spec dirty 델리게이트의 발행 조건을 확인했다.
- **미검토 / 한계**: C++ 정적 리뷰이며 BP/WBP 내부 구조, 실제 UI 및 네트워크 PIE는 검증하지 않았다. 빌드·자동화 테스트는 새로 실행하지 않았다. 기존 worklog의 성공 기록은 과거 검증으로만 참고했다. 소스 56파일은 모듈 전체 h/cpp 수이며 모두 통독했다는 뜻은 아니다. 검토 도중 `WxIndicator.cpp`의 외부 미커밋 변경이 감지되어 표시 투영 구현 자체는 결론 범위에서 제외했다. 소스를 수정하지 않았고 `Docs/Programmer/module_review_WxUI.md`는 보존했다.

---
*문서 기준 커밋 `dfdad10a2` · 리뷰일 2026-09-12 · 소스 56파일 — `/module-review`로 갱신*
