# 죽은 LastReleasedInputTag + WaitInputTagReleased 태스크 제거

## 계획

### 목표
`UWxAbilitySystemComponent::LastReleasedInputTag`와 그 유일한 소비자 `UWxAbilityTask_WaitInputTagReleased`는 이 태스크를 생성하는 어빌리티가 C++·BP 어디에도 없어(팩토리도 non-BlueprintCallable) 사실상 죽은 코드다. 통째로 제거한다. (`LastPressedInputTag`는 Attack·Dodge·Guard가 실사용하므로 유지.)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbilitySystemComponent.h` | `GetLastReleasedInputTag`·`SetLastReleasedInputTag`·`ServerSetLastReleasedInputTag` 선언, `LastReleasedInputTag` 멤버 제거 | 수정 |
| `WxAbilitySystemComponent.cpp` | 위 함수들 정의 제거. `AbilityInputTagReleased`에서 `SetLastReleasedInputTag(InputTag)` 한 줄만 제거(함수 본체·`InvokeReplicatedEvent(InputReleased)`는 Guard/Sprint 엔진 InputReleased용으로 유지) | 수정 |
| `WxAbilityTask_WaitInputTagReleased.h` / `.cpp` | 파일 삭제(죽은 태스크 클래스) | 삭제 |

### 접근 방식
- **`AbilityInputTagReleased` 존치**: 이 함수는 활성 스펙에 `AbilitySpecInputReleased` + `InvokeReplicatedEvent(InputReleased)`를 발행해 Guard/Sprint의 엔진 `InputReleased` 오버라이드를 구동한다. 죽은 것은 `LastReleasedInputTag` 사이드카뿐이므로 그 캐싱만 걷어낸다.
- **대칭 유지**: `LastPressedInputTag` 쪽(Get/Set/Server/멤버)은 실사용이라 그대로 둔다. 제거는 Released 계열에 한정.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbilitySystemComponent.h` | `GetLastReleasedInputTag`·`SetLastReleasedInputTag`·`ServerSetLastReleasedInputTag` 선언과 `LastReleasedInputTag` 멤버 제거 | 수정 |
| `WxAbilitySystemComponent.cpp` | 위 함수 정의 3종 제거, `AbilityInputTagReleased`에서 `SetLastReleasedInputTag` 호출 한 줄 제거 | 수정 |
| `WxAbilityTask_WaitInputTagReleased.h` / `.cpp` | 파일 삭제 | 삭제 |

### 구현·결정과 그 이유
- **죽은 코드 확정 후 제거**: 소스·BP 스냅샷 전수 검색으로 `WxAbilityTask_WaitInputTagReleased`를 생성하는 곳이 없고 팩토리도 non-BlueprintCallable임을 확인. 그 태스크가 유일 소비자인 `LastReleasedInputTag` 사이드카까지 안전하게 제거.
- **`AbilityInputTagReleased` 존치**: 활성 스펙에 `AbilitySpecInputReleased`+`InvokeReplicatedEvent(InputReleased)`를 발행해 Guard/Sprint의 엔진 `InputReleased`를 구동하므로 함수는 남기고, `LastReleasedInputTag` 캐싱 한 줄만 걷어냈다.
- **`LastPressedInputTag`는 유지**: Attack(L/H)·Dodge·Guard(카운터)가 실사용하는 load-bearing 상태라 대칭 제거하지 않았다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
