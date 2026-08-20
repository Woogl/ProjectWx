# Trait.Exclusive 차단 규칙 정리 (Interact·Pattern·Sprint)

## 계획

### 목표

`Trait.Exclusive` 마커를 단 어빌리티 중 "막는 쪽"(`BlockAbilitiesWithTag`)이 비어 있던 셋의 정책을 명시적으로 정리한다. Interact·Pattern 은 활성 동안 다른 액션을 막게 하고, Sprint 는 막지 않는 쪽으로 확정한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp` | 생성자에 `BlockAbilitiesWithTag.AddTag(Trait_Exclusive)` 추가 | 수정 |
| `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp` | 생성자에 `BlockAbilitiesWithTag.AddTag(Trait_Exclusive)` 추가 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp` | 변경 없음 (이미 차단을 걸지 않음, 확인 완료) | 없음 |

### 접근 방식

- **Pattern**: 몽타주 길이만큼 활성이 유지되므로 차단이 실효를 갖는다. 패턴 도중 다른 패턴·공격·회피·가드가 겹쳐 들어오는 것을 막는다. 반응형 어빌리티(HitReact·Groggy·Death·Finisher)는 마커를 달지 않으므로 여전히 패턴을 끊을 수 있다. `CanJumpInternal` 이 같은 마커의 차단 여부로 점프를 게이트하므로 패턴 중 점프도 함께 막히지만, 패턴은 AI 전용이라 실질 영향이 없다.

- **Interact**: 요청대로 차단 태그를 단다. 다만 이 어빌리티는 활성화 함수 안에서 곧바로 종료하므로 차단이 걸렸다 풀리는 구간이 같은 호출 안이고, 실측 효과는 없다 — 의도 선언에 가깝다. 상호작용 연출 중의 실제 차단은 대상 StateTree 의 이동 태스크가 ASC 에 직접 걸고 있다.

- **Sprint**: 차단 목록이 비어 있고 스프린트가 거는 GE 에도 어빌리티 차단이 없으므로 요청 상태가 이미 성립한다. 마커(애셋 태그)는 그대로 둬서 다른 액션이 스프린트를 막고 끊는 방향은 유지한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp` | 생성자에 `BlockAbilitiesWithTag` 마커 추가 | 수정 |
| `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp` | 생성자에 `BlockAbilitiesWithTag` 마커 추가 | 수정 |

### 구현·결정과 그 이유

- **Sprint 는 손대지 않았다**: 차단 목록이 비어 있고 스프린트가 거는 두 GE 에도 어빌리티 차단 설정이 없어, 요구하는 상태가 코드 변경 없이 이미 성립한다. 마커(애셋 태그)는 유지해 다른 액션이 스프린트를 막고 끊는 반대 방향은 그대로 둔다.
- **패턴 차단이 피격 반응을 죽이지 않는 근거**: 반응형 어빌리티는 마커를 애셋 태그로 달지 않고 거는 쪽으로만 쓴다. 차단은 애셋 태그 보유자에게만 걸리므로 패턴은 계속 끊긴다.
- **Interact 는 실효보다 규약 정렬**: 활성화 함수 안에서 곧바로 종료해 차단 구간이 같은 호출 안에 갇힌다. 실제 연출 중 차단은 대상 StateTree 의 이동 태스크가 ASC 에 직접 걸고 있어 이번 변경과 무관하게 동작한다. 상호작용 순간에 진행 중이던 액션을 끊고 싶다면 필요한 건 차단이 아니라 취소 태그이며, 이번 요청 범위 밖이라 넣지 않았다.

### 계획 대비 달라진 점

계획대로.

### 후속 과제

- PIE 실측 미수행: 패턴이 겹치지 않는지, 패턴 중 피격 반응·그로기·사망이 여전히 끊는지 확인 필요. (빌드 및 패턴 BP 4종이 차단 목록을 오버라이드하지 않아 부모 값을 상속함은 확인)
