# ActivationGroup 축 분리 — 선언(기획) / 런타임 창(노티파이)

## 계획

### 목표
`EWxAbilityActivationGroup` 하나가 기획자 선언(Independent·Exclusive·Reaction)과 발동 한 번 동안의 런타임 창 전이(Blocking→ComboWindow→Recovery)를 겸하고 있다. 두 축을 분리해 선언은 불변으로 두고, 창은 별도 런타임 멤버가 받게 한다. 동작은 그대로인 순수 리팩토링이며, 기획자 드롭다운에 고를 수 없는 값이 섞이지 않게 하는 것이 목적이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbilityBase.h` | `EWxAbilityActivationGroup`을 값 셋으로 축소, `EWxAbilityActionPhase` 신설, `ActionPhase` 멤버 추가 | 수정 |
| `WxAbilityBase.cpp` | 전이 함수 셋·`CanActivateAbility`·`ActivateAbility`의 판정을 두 축으로 분리, CDO 되읽기 삭제 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | 점유 판정을 두 축으로, `CancelActivationGroupAbilities` → `CancelRecoveringAbilities` 개명 | 수정 |
| 어빌리티 8종 생성자 | `Exclusive_Blocking` → `Exclusive` | 수정 |
| `WxCharacterBase.cpp` | 개명된 취소 함수 호출 | 수정 |
| `Plugins/WxCombat/README.md` 및 관련 주석 | 옛 값 이름 서술 정정 | 수정 |

### 접근 방식
- **선언 축**: `ActivationGroup`은 `EditDefaultsOnly`로 남되 런타임에 절대 바뀌지 않는다. 값은 `Independent`/`Exclusive`/`Reaction` 셋뿐이라 드롭다운 자체가 안전한 선언이 된다.
- **런타임 축**: `EWxAbilityActionPhase ActionPhase`가 노티파이 전이를 받는다. 직렬화·복제되지 않는 순수 인스턴스 상태라 `UENUM`/`UPROPERTY`를 붙이지 않고, 매 활성화 도입부에서 `Blocking`으로 초기화한다(선언값을 CDO에서 되찾아오던 코드가 사라진다).
- **판정**: 점유 여부는 `Reaction || (Exclusive && Phase != Recovery)`, 후딜 취소는 `Exclusive && Phase == Recovery`. 전이 가드가 값 나열에서 "배타인가 + 어느 창인가" 두 물음으로 갈린다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbilityBase.h` | 그룹 enum을 Independent·Exclusive·Reaction 셋으로 축소, `EWxAbilityActionPhase` 신설, `ActionPhase` 멤버 추가 | 수정 |
| `WxAbilityBase.cpp` | 전이 함수 셋과 자기 점유 갈래를 창 기준으로, 활성화 도입부의 CDO 되읽기를 창 초기화로 교체 | 수정 |
| `WxAbilitySystemComponent.h/.cpp` | 점유 판정을 두 축 조합으로, `CancelActivationGroupAbilities` → `CancelRecoveringAbilities` | 수정 |
| 어빌리티 8종 생성자 | 배타 선언을 `Exclusive`로 | 수정 |
| `WxCharacterBase.cpp` | 개명된 취소 함수 호출, 쓰이지 않게 된 어빌리티 헤더 포함 제거 | 수정 |
| `Plugins/WxCombat/README.md` | 그룹 값 목록 정정 | 수정 |

### 구현·결정과 그 이유
- **선언과 창을 멤버 둘로**: 선언은 이제 어디서도 덮어쓰이지 않으므로, 직전 활성화가 남긴 값을 CDO에서 되찾아오던 복원 코드가 필요 없어졌다. 재사용 인스턴스에서 초기화할 것은 창 하나뿐이다.
- **창은 반영에 올리지 않음**: 저장도 복제도 되지 않는 인스턴스 상태라 `UPROPERTY`를 붙이지 않았다. 창이 ASC 전역이 아니라 어빌리티 인스턴스에 속한다는 기존 결론은 그대로다.
- **전이 가드가 두 물음으로 갈림**: 예전엔 "어떤 값에서 들어왔나"를 나열해 Independent 승격과 Reaction의 캔슬 면역 상실을 함께 막았는데, 이제 배타 여부는 그룹이, 창 순서는 phase가 각각 답한다.
- **취소 함수 개명**: 호출부 둘 다 후딜만 넘기고 있었고 분리 후엔 그룹이 아니라 창을 묻는 함수가 되어, 인자를 없애고 이름에 뜻을 담았다.

### 계획 대비 달라진 점
- `WxCharacterBase.cpp`에서 enum이 사라지며 `WxAbilityBase.h` 포함이 미사용이 되어 함께 제거했다(계획에 없던 한 줄).

### 후속 개명 — Reaction → Override
축 분리 뒤 셋째 값 `Reaction`만 콘텐츠 부류 이름이라 축과 어긋난 것이 드러나, 같은 날 `Override`로 개명했다. 어긋남의 증거는 `WxAbility_Finisher`였다 — 반응이 아니라 공격자 어빌리티인데 "상호작용 점유에 막히지 않으려고" 이 값을 빌려 쓰고 있었고 주석에도 그렇게 적혀 있었다.

이름 후보 중 `Forced`는 바로 위에 붙는 `ActivationPolicy`(OnTriggered/OnGiven) 탓에 "강제 발동"이라는 트리거 얘기로 오독될 수 있어 뺐고, `Unblockable`은 가드 시스템이 있어 "가드 불가 공격"으로 읽혀 뺐다. `Interrupting`은 아예 틀렸다 — 이 값은 앞 액션을 끊지 않고 공존한다.

Lyra 어휘를 빌리는 안도 검토했으나 대응물이 없어 접었다. Lyra의 `Exclusive_Replaceable`은 남을 막지 않고 밀리는 값이라 우리 `Exclusive`가 아니며, 우리 `Exclusive`가 이미 Lyra의 `Exclusive_Blocking`이 하는 일을 한다. 실제 대응은 그룹이 아니라 창 축에 있다 — 우리 `Blocking` 창이 Lyra의 `Exclusive_Blocking`, `Recovery` 창이 `Exclusive_Replaceable`이고, Lyra가 값 둘로 나눈 것을 우리는 한 어빌리티가 시간에 따라 밟는 두 상태로 접은 셈이다. 배타를 관통하면서 캔슬도 안 되는 칸은 Lyra에 아예 없다. 남은 `Override`는 애니메이션·상태 시스템의 표준 어휘라 설명이 필요 없고 다른 두 값과 길이·결이 맞는다.

### 후속 과제
- PIE 회귀 확인 미실시 — 콤보 이어짐, `AM_DodgeSuccess` 후딜 캔슬, 후딜 중 점프 끊기, HitReact의 배타 관통.
