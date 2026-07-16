# AttributeRatio 데코 비교 enum을 엔진 EArithmeticKeyOperation으로 교체

## 계획

### 목표
`UWxBTDecorator_AttributeRatio`의 자체 비교 enum `EWxAttributeRatioComparison`을 걷어내고, 엔진 표준 `EArithmeticKeyOperation`(AIModule)으로 통일한다. 표준 Blackboard 데코와 동일한 드롭다운을 쓰고 `NotEqual`도 얻는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h` | `EWxAttributeRatioComparison` 제거, `BlackboardKeyEnums.h` 포함, 프로퍼티를 `TEnumAsByte<EArithmeticKeyOperation::Type>`로 교체 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp` | 기본값·두 switch 한정자 교체, `NotEqual` 케이스 추가 | 수정 |

### 접근 방식
- **엔진 표준 enum 채택**: 엔진 `UBTDecorator_Blackboard`와 동일하게 `TEnumAsByte<EArithmeticKeyOperation::Type>` 프로퍼티 사용. 마이그레이션은 미고려(사용자 지시, 사용처 BT_Boss는 기본값).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_AttributeRatio.h` | 자체 enum 제거, `BlackboardKeyEnums.h` 포함, `Comparison`을 `TEnumAsByte<EArithmeticKeyOperation::Type>`로 교체 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_AttributeRatio.cpp` | 기본값 `EArithmeticKeyOperation::LessOrEqual`, 두 switch 한정자 교체, `NotEqual`(`!=`, `!IsNearlyEqual`) 케이스 추가 | 수정 |

### 구현·결정과 그 이유
- **엔진 표준으로 통일**: 목적이 동일한 enum이 엔진에 이미 있으므로 자체 정의를 없애 중복을 제거하고 표준 데코와 UI 일관성을 확보.
- **NotEqual 처리 추가**: 엔진 enum엔 있고 우리엔 없던 값이라, 에디터 선택 시 무동작(false)이 되지 않도록 두 switch에 반드시 추가. 판정은 기존 `Equal`의 `IsNearlyEqual`과 대칭으로 `!IsNearlyEqual`.
- **프로퍼티명 `Comparison` 유지**: 리네임은 요청 밖이라 스코프 최소화.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
