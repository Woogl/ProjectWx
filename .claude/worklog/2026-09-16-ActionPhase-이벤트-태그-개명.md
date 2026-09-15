# ActionPhase 이벤트 태그 개명

## 계획

### 목표
어빌리티 단계 전이 이벤트의 태그 이름과 문구가 "발동 조건 변경" 전반을 알리는 것처럼 읽히는 문제를 고친다(WxCore 리뷰 3번). 실제로는 단계 전이만 싣는다. 이름만 보고 새 소비자가 태그·쿨다운·코스트 변화까지 이 이벤트로 받으려 하면 갱신이 빠진다. 모델이 발행하는 사실 그대로 `Event.Ability.ActionPhaseChanged`로 이름을 바꾸고 문구를 좁힌다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 태그 선언 개명, 발행자·싣지 않는 것·트리거 금지로 문구 교체 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | 태그 정의 개명 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp` | 발행부 태그 참조 교체 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` | 태그 이름을 따른 핸들러·구독 핸들 개명 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | 구독·해제·핸들러의 태그 참조와 식별자 교체 | 수정 |

### 접근 방식
- **이름만 바꾸고 동작은 그대로 둔다**: 발행 시점·구독 방식·재평가 예약은 건드리지 않는다. 위치도 `Event.Ability` 하위에 둔다.
- **에셋 이관 절차는 생략한다**: 옛 태그 문자열이 에셋·ini 어디에도 없어, 리다이렉트와 리세이브가 필요 없다.
- **VM 식별자는 태그를 따른 것만 맞춘다**: 핸들러와 구독 핸들은 태그 이름에서 왔으므로 함께 바꾼다. VM이 스스로 부르는 "발동 상태 재평가" 계열 이름은 VM 개념이라 그대로 둔다.
- **과거 기록은 두고 간다**: 이전 worklog와 리뷰 문서의 옛 이름은 당시 기록이라 고치지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 태그 선언을 개명하고, 문구를 발행자·싣지 않는 변화·트리거 금지 세 줄로 바꿨다 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | 태그 정의 개명 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp` | 단계 전이 발행부의 태그 참조 교체 | 수정 |
| `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` | 핸들러·구독 핸들 개명 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp` | 구독·해제·핸들러의 태그 참조와 식별자 교체 | 수정 |

### 구현·결정과 그 이유
- **이름은 발행자가 아는 사실로 붙였다**: 어빌리티는 자기 단계가 바뀐 것만 알고, 그것이 발동 가능 여부에 어떤 의미인지는 VM이 판단한다. 소비자 관점 이름이 발행 범위를 넘겨 읽히던 원인이라, 발행 원천의 사실로 이름을 맞췄다.
- **문구에 싣지 않는 것을 적었다**: 새 소비자가 이 이벤트 하나로 발동 가능 여부 전체를 받으려는 오해를 막는 것이 이번 수정의 목적이라서다.
- **리다이렉트 없이 바꿨다**: 옛 태그 문자열이 에셋과 ini에 없음을 확인했으므로, 문자열이 사라져도 조용히 빠지는 에셋이 없다.
- **검증**: `Source/`·`Plugins/` 코드에서 옛 이름 0건. UE 5.8.2 WxEditor Win64 Development 빌드 성공(`Result: Succeeded`).

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 동작 변경이 없는 개명이라 PIE 실측은 하지 않았다.
- 리뷰 문서(WxCore 3번)는 다음 `/module-review`에서 해소를 확인한다.
