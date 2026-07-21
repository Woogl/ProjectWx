# IsObservedInput 제거 → 반격 입력 감지를 AbilityTask로 일원화

## 계획

### 목표
헷갈리는 `UWxAbilityBase::IsObservedInput`(감지 함수가 아니라 ASC 라우팅 게이트)을 삭제하고, 이미 감지를 담당하던 `WaitInputActionPressed` 태스크가 ASC의 단일 입력 브로드캐스트를 구독해 자기완결하도록 일원화한다. 태스크는 명칭을 `WaitInputActionTriggered`로 통일한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbilitySystemComponent.h/.cpp` | `OnInputActionTriggered` 멀티캐스트 추가·브로드캐스트, `IsObservedInput` 분기(Triggered/Released) 삭제 | 수정 |
| `WxAbilityTask_WaitInputActionPressed.{h,cpp}` → `...Triggered.{h,cpp}` | 파일·클래스·델리게이트·핸들러 개명 + 글로벌 브로드캐스트 구독 전환 | 개명·수정 |
| `WxAbilityBase.h/.cpp` | `IsObservedInput` 가상 삭제 | 삭제 |
| `WxAbility_Guard.h/.cpp` `WxAbility_Dodge.h/.cpp` | `IsObservedInput` override 삭제, 개명 태스크 참조·콜백(`HandleCounterInputTriggered`) 갱신 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | 태스크 서술·트리거표·역할표 갱신 | 수정 |

### 접근 방식
- **감지 신호 단일화**: ASC가 입력 트리거마다 `OnInputActionTriggered(Action)` 논다이나믹 멀티캐스트를 fire. 태스크가 이를 구독해 액션 필터 후 `OnTriggered` 방송. per-spec 리플리케이티드 이벤트 + prediction key + LastPressedInputAction 필터 플러밍 제거.
- **라우팅 게이트 제거**: ASC의 활성 spec 매칭에서 `IsObservedInput` 분기 삭제 → `IsActivationInput`만(자기 발동 입력). 반격 입력은 브로드캐스트 경로로 이동.
- **바인딩 계층 불변**: `GetInputActions`(반격 입력을 EnhancedInput에 바인딩)와 카운터 리스닝 로직은 이름만 바꾸고 유지.
- **부수 정리**: 반격 입력 릴리즈가 어빌리티로 안 감(가드 풀림 잠복버그 해소), 반격 누름이 `Guard::InputPressed`를 안 깨움(퍼펙트가드 경합 해소).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbilitySystemComponent.h/.cpp` | `OnInputActionTriggered` 멀티캐스트+접근자 추가, Triggered에서 방송, Triggered/Released의 `IsObservedInput` 분기 삭제(IsActivationInput만) | 수정 |
| `WxAbilityTask_WaitInputActionTriggered.{h,cpp}` | 개명 신규(구 `...Pressed` 삭제), `OnInputActionTriggered` 구독+액션 필터, 출력 `OnTriggered`, 핸들러 `HandleInputTriggered` | 개명·수정 |
| `WxAbilityBase.h/.cpp` | `IsObservedInput` 가상 삭제 | 삭제 |
| `WxAbility_Guard.h/.cpp` `WxAbility_Dodge.h/.cpp` | `IsObservedInput` override 삭제, 태스크 참조·바인딩·콜백(`HandleCounterInputTriggered`) 개명 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | ① 섹션·트리거표·역할표 갱신 | 수정 |

### 구현·결정과 그 이유
- **게이트 삭제, 방송으로 대체**: `IsObservedInput`은 감지가 아니라 ASC 라우팅 게이트였다. ASC가 입력 트리거마다 `OnInputActionTriggered(Action)`를 방송하고, 감지를 이미 하던 태스크가 이를 구독해 자기완결한다. 어빌리티에서 라우팅 판정이 사라져 개념이 한 곳(태스크)에 모였다.
- **태스크 플러밍 축소**: per-spec 리플리케이티드 이벤트 + prediction key + `GetLastPressedInputAction` 필터를, 방송 인자로 오는 액션 한 번 비교로 단순화. 태스크 수명이 어빌리티 활성 구간에 한정되므로 액션 필터만으로 충분.
- **명칭 통일**: ASC `AbilityInputActionTriggered`·`OnInputActionTriggered`에 맞춰 태스크를 `WaitInputActionTriggered`, 출력 `OnTriggered`, 어빌리티 콜백 `HandleCounterInputTriggered`로 개명.
- **바인딩 계층 불변**: `GetInputActions`(반격 입력을 EnhancedInput에 바인딩)와 `LastPressedInputAction` RPC(Attack L/H)는 그대로.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- PIE 런타임 미검증(빌드만 확인). 특히 의도된 동작 변화 2건 확인 필요: (a) 콤보 윈도우 밖 약공격 톡 → 가드 유지(안 풀림), (b) 퍼펙트가드 중 약공격 → 반격만(가드 복귀 경합 없음), 가드 버튼 재입력 → 가드 복귀. 그 외 가드/회피 반격, Attack 약/강 콤보 정상 여부.
