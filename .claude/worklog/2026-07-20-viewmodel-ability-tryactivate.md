# UWxViewModel_Ability::TryActivateAbility 추가

## 계획

### 목표
`WBP_Ability` 버튼의 `On Clicked` 이벤트를 MVVM View Binding으로 어빌리티 발동에 연결하기 위해, 바인딩된 어빌리티를 발동하는 command 함수 `TryActivateAbility`를 ViewModel에 `BlueprintCallable`로 추가한다. MVVM 이벤트 바인딩 대상 함수는 `BlueprintCallable`이어야 바인딩 패널에 노출되기 때문이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` | `Deinitialize()` 뒤에 `BlueprintCallable bool TryActivateAbility()` 선언 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | 헤더 순서에 맞춰 `Deinitialize()` 뒤에 정의 | 수정 |

### 접근 방식
- **기존 스펙 탐색 재사용**: VM이 이미 캐시한 `CachedASC`·`CachedAbility`(CDO)로 `ASC->GetActivatableAbilities()`에서 CDO와 일치하는 `FGameplayAbilitySpec`을 찾는다. 탐색 루프는 `RefreshActivationState`와 동일한 인라인 스타일(헬퍼 추출 없이 약간의 반복 용인).
- **엔진 위임**: 발동 판정(비용/쿨다운/태그)은 `ASC->TryActivateAbility(Spec.Handle)`에 위임하고 그 결과(bool)를 반환한다.
- **규칙 7 예외**: MVVM ViewModel command 함수를 규칙 7의 정당한 예외로 취급(코드베이스에 비라이브러리 `BlueprintCallable` 선례 존재: `UWxButtonBase`, `UWxTabListWidgetBase`). CLAUDE.md 문구는 수정하지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` | `Deinitialize()` 뒤에 `BlueprintCallable bool TryActivateAbility()` 선언 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | 헤더 순서에 맞춰 `Deinitialize()` 뒤에 정의(캐시된 CDO로 스펙 탐색 후 `ASC->TryActivateAbility`) | 수정 |

### 구현·결정과 그 이유
- **VM에 command 함수를 둠**: 발동에 필요한 ASC·AbilitySpec이 VM 내부에 캐시돼 있어 위젯보다 VM이 자연스러운 소유자다. 위젯 이벤트 그래프에서 처리하면 이 상태를 노출하거나 탐색을 중복 구현해야 해 응집도가 떨어진다.
- **`BlueprintCallable` 채택(규칙 7 예외)**: MVVM 이벤트 바인딩 대상 함수는 `BlueprintCallable`이어야 바인딩 패널에 노출된다. 대안이 없어 MVVM command를 규칙 7의 정당한 예외로 취급했다(코드베이스에 비라이브러리 선례 존재).
- **엔진 위임**: 비용/쿨다운/태그 판정은 `ASC->TryActivateAbility`에 맡기고 결과 bool을 그대로 반환. VM의 `CanActivate` 재평가와 판정 기준이 일치한다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 에디터에서 `WBP_Ability`의 `On Clicked` → `Try Activate Ability` View Binding 연결 및 PIE 발동 확인(사용자 진행).
