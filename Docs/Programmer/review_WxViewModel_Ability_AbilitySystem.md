# WxUI — Ability / AbilitySystem 코드 리뷰

> 슬롯 재매칭, 약한 ASC 참조, 구독 해제와 쿨다운 티커 정리는 명확하다. 다만 실제 발동 조건과 어빌리티 목록의 변경을 놓치는 갱신 경로가 있다.
> 이번 검토는 `UWxViewModel_Ability`, `UWxViewModel_AbilitySystem`의 헤더·구현 4파일과 필요한 호출부·GAS 구현에 한정한다. 기존 전체 모듈 리뷰는 보존한다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 후딜·콤보 창 전환 뒤 CanActivate가 실제 판정과 달라진다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:404`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:469`
- **범주**: 버그/정확성
- **문제**: 발동 가능 여부는 태그·비용 어트리뷰트·쿨다운 충전 수 변화에만 재평가된다. 그러나 `UWxAbilityBase::OpenComboWindow`, `CloseComboWindow`, `StartRecovery`는 `ActionPhase`를 변경하면서 해당 이벤트를 발생시키지 않는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:61`, `:78`, `:86`). 같은 파일 `:118` 및 `:133`에서 이 값은 실제 배타 점유·재발동 판정을 바꾼다. 예를 들어 공격 중 다른 배타 스킬이 `CanActivate=false`가 된 뒤 공격이 후딜에 진입하고 추가 입력이 없으면, 자원·태그·충전 수가 그대로인 동안 실제로는 발동 가능해도 UI는 계속 false이다. 콤보 창 개폐에서도 반대 방향의 오표시가 가능하다. 쿨다운 티커가 돌아도 충전 수가 같으면 재평가하지 않아 해결되지 않는다.
- **제안**: 액션 단계 변경을 UI가 구독할 수 있는 공용 상태 변경 신호로 전달하고 `RefreshActivationState`를 예약한다. `WxUI`가 `WxCombat`을 직접 참조하지 않도록 `WxCore` 계약이나 ASC의 공용 이벤트를 사용한다. 태그·자원·충전 수를 고정한 상태에서 후딜 진입 및 콤보 창 개폐 전후를 검증한다.
- **확신도**: 높음

### 2. 🟡 AbilitySpecDirtied만 구독하면 클라이언트 부여와 단독 제거를 놓친다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:41`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:267`
- **범주**: 버그/정확성
- **문제**: 이미 만든 슬롯의 목록 변경 감지를 `AbilitySpecDirtiedCallbacks`에 맡기지만, UE 5.8의 방송은 `MarkAbilitySpecDirty`의 authority 분기 안에만 있다. 클라이언트의 어빌리티 복제 수신은 이 콜백을 방송하지 않는다. 또한 서버에서도 비활성 어빌리티의 `ClearAbility`는 `OnRemoveAbility → RemoveAtSwap → MarkArrayDirty`로 끝나 해당 콜백이 없다. 따라서 빈 슬롯 VM을 먼저 만든 뒤 클라이언트에 어빌리티가 복제되거나, 표시 중인 비활성 어빌리티만 제거하면 관련 태그 변경 등 우연한 재매칭 신호가 올 때까지 빈 슬롯 또는 제거 전 제목·아이콘이 남는다. 비용 통지는 `RefreshActivationState`만 실행하므로 빈 슬롯을 새 어빌리티에 연결하지 못한다. `:271`의 후속 부여 신호 가정은 교체가 없는 제거 및 클라이언트 부여를 포괄하지 못한다.
- **제안**: 서버 부여·제거와 클라이언트 복제 수신을 모두 포괄하는 목록 변경 신호를 공용 계약으로 전달해 슬롯 재매칭을 예약한다. VM 생성 이후의 늦은 복제 부여와, 후속 부여·태그 변경 없는 비활성 어빌리티 단독 제거를 각각 검증한다.
- **확신도**: 높음

### 3. 🟡 이펙트 억제 해제 시 같은 핸들의 VM이 중복되고 제거 후에도 남는다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:205`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:228`
- **범주**: 버그/정확성
- **문제**: `HandleActiveEffectAdded`는 기존 핸들을 확인하지 않고 VM을 추가한다. UE 5.8에서는 이 이벤트가 최초 추가뿐 아니라 기존 GE의 inhibition 해제 시에도 같은 핸들로 발생한다. 아이콘이 있는 `IWxUIData` GE가 표시된 상태에서 ongoing 태그 요건을 잃었다가 되찾으면 같은 효과의 VM이 두 개가 된다. 이후 실제 제거 이벤트는 첫 항목만 지우고 `break`하므로 나머지는 배열에 남는다. 프로젝트에도 ongoing 태그 요건을 쓰는 `WxEffect_DrainSP`, `WxEffect_RegenSP`가 있지만, 이 두 C++ 클래스 자체가 아이콘 표시 대상이라는 근거는 없으므로 현재 에셋에서 발생한다고 단정하지 않는다. 일반 스택 추가는 엔진이 다른 경로로 처리하므로 이 문제의 재현 조건이 아니다.
- **제안**: `HandleActiveEffectAdded`에서 유효 핸들별 VM을 하나만 유지하도록 기존 항목을 조회한다. 아이콘이 있는 ongoing GE로 활성 → 억제 → 재활성 → 제거를 수행했을 때 항목 수가 중복되지 않고 마지막에 0이 되는지 검증한다.
- **확신도**: 중간

### 4. 🟡 Getter의 최초 목록 구성이 같은 필드의 변경 통지를 재발생시킨다

- **위치**: `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:49`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:166`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:215`
- **범주**: 설계/구조
- **문제**: `ActiveEffectViewModels` Getter의 최초 평가가 `InitializeActiveEffects → BuildActiveEffectViewModels → HandleActiveEffectAdded`를 호출하며, 아이콘이 있는 기존 GE마다 바로 같은 필드의 FieldNotify를 방송한다. 엔진 MVVM은 바인딩 초기화 중 이 통지를 받으면 경고하고 무시하며, 같은 Immediate 바인딩 실행 중 재진입하면 재귀 감지 ensure를 발생시킨다. `bActiveEffectsInitialized`를 먼저 설정하는 것은 VM 재생성을 막지만 통지 자체를 막지 못한다. 실제로 보이는 WBP의 실행 모드는 확인하지 않았으므로 모든 화면에서 ensure가 발생한다고 단정하지 않는다.
- **제안**: 최초 스냅샷 구성은 외부 통지 없이 완료하고 Getter가 완성된 배열을 반환하게 한다. 이후 실제 GE 추가·제거만 변경 통지를 보내도록 최초 구성과 이벤트 처리의 통지 책임을 구분한다. 기존 표시 GE가 있는 상태에서 최초 MVVM 바인딩을 실행해 초기화 경고와 Immediate 재귀가 없는지 검증한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp`
- **훑은 파일**: `Plugins/WxUI/README.md`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`, `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainSP.cpp`
- **엔진 근거**: 로컬 UE 5.8의 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:495`, `:1008`, `:1492`에서 삭제·dirty·복제 수신 경로를 확인했다. 같은 디렉터리의 `AbilitySystemComponent.cpp:362` 및 `GameplayEffect.cpp:4740`은 억제 해제 시 added 재통지 근거이며, `GameplayEffect.cpp:4549`는 스택 추가 중복 가설을 배제하는 근거이다. 엔진 경로 기준점은 `C:/Program Files/Epic Games/UE_5.8`이다.
- **MVVM 엔진 근거**: UE 5.8 `Engine/Plugins/Runtime/ModelViewViewModel/Source/ModelViewViewModel/Private/View/MVVMView.cpp:754`에서 초기화 중 FieldNotify 경고·무시, `:818`에서 Immediate 동일 바인딩 재귀 검사를 확인했다.
- **미검토 / 한계**: C++ 정적 검토이며 PIE·멀티플레이 재현 및 빌드는 수행하지 않았다. BP/WBP 바인딩·이벤트 그래프·DataTable 실제 설정은 검토하지 않았다. 2번은 API가 약속한 늦은 부여·제거에 대한 조건부 실패이며, 프로젝트 C++에서 직접 `ClearAbility`를 호출하는 사용처는 검색되지 않았다. 현재 플레이가 클라이언트 복제 경로를 사용하는지도 검증하지 않았다. 3번의 UIData·아이콘·ongoing 요건 조합이 현재 콘텐츠에 있는지는 확인하지 않았다. provenance의 58파일은 WxUI 전체 `.h`/`.cpp` 수이며 전체 파일을 통독했다는 의미가 아니다. 소스는 수정하지 않았다.

---
*문서 기준 커밋 `985aabe9c` · 리뷰일 2026-09-15 · 소스 58파일 — `/module-review`로 갱신*
