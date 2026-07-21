# Input.* 태그 제거 → InputAction 라우팅 전환

## 계획

### 목표
플레이어 입력→어빌리티 라우팅의 중간 키인 `Input.*` GameplayTag 13개를 제거하고 `UInputAction`을 직접 라우팅 키로 쓴다. 태그 남발·3중 동기화(태그 정의+InputConfig 맵+AbilitySet InputTag)·다중 정체성을 없앤다. 중앙 ASC 중재와 `SetupPlayerInputComponent` 바인딩 위치는 유지한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGameplayTags.h/.cpp` | `Input_*` 13개 선언·정의 삭제 | 삭제 |
| `WxAbilityBase.h/.cpp` | 단일 `InputAction` 멤버 + `IsActivationInput`/`IsObservedInput` 가상 함수 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | 라우터 태그→IA rekeying, `LastPressedInputAction`(+RPC) | 수정 |
| `WxAbilityTask_WaitInputTagPressed.h/.cpp` | `...WaitInputActionPressed`로 개명, IA 필터 | 개명·수정 |
| `WxAbility_Attack.h/.cpp` | `HeavyInputAction` + `IsActivationInput` override + L/H 판별 교체 | 수정 |
| `WxAbility_Guard.h/.cpp` `WxAbility_Dodge.h/.cpp` | `CounterInputAction` + `IsObservedInput` override + 태그 차용 블록 삭제 | 수정 |
| `WxAbilitySet.h/.cpp` | `InputTag` 필드·grant 소스태그 블록 제거 | 수정 |
| `WxInputConfig.h` | `AbilityInputBindings` 맵 제거, `AbilityInputActions` 목록 추가 | 수정 |
| `WxPlayerCharacter.h/.cpp` | 핸들러·바인딩 IA화 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | InputTag 라우팅 서술 갱신 | 수정 |

### 접근 방식
- **IA는 어빌리티 CDO에 배치**: CDO 디폴트는 복제 없이 서버·클라 양쪽에 존재하므로 클라 입력 라우팅이 `Spec.Ability`를 직접 읽는다(현재 소스태그 복제와 동일 효과). AbilitySet 항목 배치는 per-spec 복제가 필요해 부적합.
- **기본 단일 `InputAction` + 특수 케이스 명명 필드**: 배열·런타임 집합을 쓰지 않는다. Attack은 `HeavyInputAction`, Guard/Dodge는 `CounterInputAction`(약공격 단일). 발동/차용 판정은 `IsActivationInput`/`IsObservedInput` 가상 함수로 확장한다. ASC는 활성 spec일 때만 `IsObservedInput`을 검사하므로, 가드/회피가 하던 소스태그 add/remove 차용 블록이 선언적 필드로 대체돼 사라진다.
- **중앙 라우터 유지**: `AbilityInputTagPressed`의 활성/재전달 중재는 유지, 매칭 키만 태그→IA로 교체.
- **제외**: Finisher·Interact은 페이로드가 필요한 GameplayEvent 트리거라 입력 라우팅 대상이 아니다. F는 `IA_Interact` 직접 바인딩 하나로 유지한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGameplayTags.h/.cpp` | `Input.*` 13개 + 섹션 주석 삭제 | 삭제 |
| `WxAbilityBase.h/.cpp` | 단일 `InputAction` 멤버 + `IsActivationInput`/`IsObservedInput` 가상 함수 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | 라우터 IA rekeying, `LastPressedInputAction`(+Server RPC) | 수정 |
| `WxAbilityTask_WaitInputActionPressed.h/.cpp` | `WaitInputTagPressed`에서 개명, IA 정확 매칭 필터 | 개명 |
| `WxAbility_Attack.h/.cpp` | `HeavyInputAction` + `HandlesInput` override + L/H 판별 교체 | 수정 |
| `WxAbility_Guard.h/.cpp` `WxAbility_Dodge.h/.cpp` | `CounterInputAction` + `HandlesInput` override + 태그 차용 블록 삭제 | 수정 |
| `WxAbilitySet.h/.cpp` | `InputTag` 필드·grant 소스태그 블록 제거 | 수정 |
| `WxInputConfig.h` | `AbilityInputActions`(const 목록) | 수정 |
| `WxPlayerCharacter.h/.cpp` | 핸들러·바인딩 IA화(액션 payload) | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 서술 갱신 | 수정 |

### 구현·결정과 그 이유
- **IA는 어빌리티 CDO에**: CDO 디폴트는 복제 없이 서버·클라 양쪽에 존재해 클라 입력 라우팅이 `Spec.Ability`를 직접 읽는다. AbilitySet 항목에 두면 라우팅 키를 per-spec으로 복제해야 해 부적합(복제 가능 UObject 필드는 단일 `SourceObject`뿐).
- **발동/관찰을 단일목적 가상 둘로**: `IsActivationInput`(발동 입력) + `IsObservedInput`(활성 중 관찰 입력, base=false). ASC가 비활성 spec은 전자만, 활성 spec은 후자까지 검사한다 — 활성 게이트가 호출부(ASC)에 있어 각 함수는 한 가지만 답한다. 관찰이 활성에만 걸리므로 비활성 가드/회피가 공격 입력을 발동으로 오인하는 것을 막는다.
- **태그 차용 제거**: 가드/회피가 활성 시 소스태그를 add/remove하던 코드가 선언적 `CounterInputAction` + ASC의 `bActive` 게이트로 대체돼 `MarkAbilitySpecDirty` 복제·정리가 사라졌다.
- **반격 카디널리티는 약공격 단일**: 기존 부모태그 `Input.Attack`(약/강 모두)에서 단일 `CounterInputAction`으로 축소(사용자 결정). 회피/가드 중 강공격은 더 이상 반격을 유발하지 않는다.

### 계획 대비 달라진 점
- 매칭 가상은 최종 `IsActivationInput`/`IsObservedInput` 2개. 중간에 사용자 요청으로 단일 `HandlesInput(Action, bActive)`로 합쳤다가, bool 파라미터가 두 질문을 뭉뚱그려 헷갈린다는 지적에 다시 분리했다. (파라미터명 `bIsActive`는 `UGameplayAbility::bIsActive`와 shadow 충돌=C4458 주의.)
- 활성화 정책 enum `EWxAbilityActivationPolicy::OnInputTriggered` → `OnTriggered` 개명(입력 외 이벤트·AI 대기 어빌리티도 쓰므로). CoreRedirect는 기본값=index 0 유지라 불필요 판단.
- `AbilityInputActions`는 `TObjectPtr<const UInputAction>`(const) — `EnhancedInputComponent::BindAction` payload 타입을 핸들러(`const UInputAction*`)와 일치시키기 위함.

### 후속 과제
- **에셋 재저작 필수**(코드만으론 런타임 입력 무동작): InputConfig DA의 `AbilityInputActions`, 어빌리티 BP별 `InputAction`(+Attack `HeavyInputAction`, Guard/Dodge `CounterInputAction`). AbilitySet의 `InputTag`는 필드 삭제로 자동 소실.
- 런타임 PIE 검증: 약/강 콤보, 가드·회피 반격, 스킬 1–4·궁극기·락온·아이템·스프린트 발동. 리슨서버에서 L/H `LastPressedInputAction` RPC 복제 확인.
