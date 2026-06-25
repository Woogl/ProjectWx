# RandomChoice 자식별 가중치 부여

## 계획

### 목표
`UWxBTComposite_RandomChoice`가 자식을 균등 확률로만 추첨하는 것을, 자식마다 가중치를 줘서 빈도를 차등화할 수 있게 한다. (보스 공격 패턴처럼 "약공격 자주, 강공격 가끔") BT Composite는 자식별 메타데이터를 못 들고 있으므로, 가중치를 자식에 붙는 Decorator로 운반한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_RandomChoiceWeight.h` | 가중치 데이터용 Decorator 선언 (`UBTDecorator` 베이스, `float Weight`) | 신규 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_RandomChoiceWeight.cpp` | 생성자/`CalculateRawConditionValue`(항상 true)/`GetStaticDescription` 구현 | 신규 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp` | `GetNextChildHandler` 추첨부를 누적 가중치 룰렛으로 교체, include 추가 | 수정 |

### 접근 방식
- **Weight Decorator**: 가중치 값만 담는 순수 데이터용 Decorator를 자식 노드에 붙인다. `CalculateRawConditionValue`는 항상 true(실행을 막지 않는 데이터 운반용). 가중치가 자식을 따라다니므로 재배치/추가/삭제에 안전하고 그래프에서 직접 보인다.
- **핸들러 룰렛**: 후보 수집 루프(첫 진입 체크, `bAvoidRepeat` 회피)는 유지. 각 후보의 `Children[Index].Decorators`에서 `UWxBTDecorator_RandomChoiceWeight`를 찾아 `Weight`(없으면 1.0) 사용. `TotalWeight`를 누적해 `FRandRange(0, Total)` 룰렛으로 선택. 가중치 0은 사실상 제외, 전부 0이면 `ReturnToParent`.

### 범위 밖
- 다른 Decorator 조건(AttributeRatio 등)은 가중치 추첨에 반영하지 않음 (선택된 자식 막히면 폴백 없이 실패 — 기존 시멘틱 유지).
- 한 자식에 Weight Decorator 여러 개면 첫 번째만 사용.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../Public/WxBTDecorator_RandomChoiceWeight.h` | `float Weight`(기본 1.0, ClampMin 0) + `GetWeight()` 접근자 선언 | 신규 |
| `Plugins/WxAI/.../Private/WxBTDecorator_RandomChoiceWeight.cpp` | 생성자, `CalculateRawConditionValue`(항상 true), `GetStaticDescription` 구현 | 신규 |
| `Plugins/WxAI/.../Private/WxBTComposite_RandomChoice.cpp` | `GetNextChildHandler` 추첨부를 누적 가중치 룰렛으로 교체, include·설명 문구 추가 | 수정 |

### 구현·결정과 그 이유
- **가중치를 Decorator로 운반**: BT Composite는 자식별 데이터를 못 들고 있어, 가중치를 자식에 붙는 Decorator에 담았다. 자식을 따라다녀 재배치/추가/삭제에 안전하고 그래프에 직접 보인다.
- **`CalculateRawConditionValue`는 항상 true**: 이 Decorator는 조건이 아니라 데이터 운반용이므로 자식 실행을 절대 막지 않아야 한다.
- **가중치 조회를 핸들러에 인라인**: 호출부 1곳뿐이라 별도 헬퍼/익명 namespace 없이 후보 루프 안에서 처리.
- **`GetWeight()` 접근자 노출**: `Weight`를 protected로 두고 RandomChoice가 읽도록 const 게터 제공.
- **전부 가중치 0 → `ReturnToParent`**: 실행할 후보가 없는 상태로 보고 기존 "후보 0개" 경로와 동일하게 실패 반환.

### 계획 대비 달라진 점
- `Weight`가 protected라 RandomChoice에서 직접 못 읽어 `GetWeight()` const 게터를 추가했다(계획엔 명시 안 됨). 그 외 계획대로.

### 후속 과제
- 에디터 동작 검증(가중치 차등 분포·0 제외·`bAvoidRepeat` 결합)은 미수행 — 컴파일까지 확인. 다음 에디터 실행 시 BT 디버거로 확인 필요.
