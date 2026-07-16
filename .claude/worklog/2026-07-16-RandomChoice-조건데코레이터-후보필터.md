# RandomChoice 조건 Decorator 후보 필터

## 계획

### 목표
`UWxBTComposite_RandomChoice`가 가중치만 보고 블라인드로 자식을 골라, 뽑힌 자식의 조건 Decorator(`CompareAttributeRatio` 등)가 실패하면 폴백 없이 노드 전체가 실패하던 문제를 고친다. 후보 수집 단계에서 조건 Decorator가 실행을 막는 자식을 미리 제외해, "현재 유효한 후보들 중에서만 가중 추첨"이 되도록 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxBTComposite_RandomChoice.cpp` | `GetNextChildHandler` 후보 수집 루프에 조건 Decorator 통과 검사(`DoDecoratorsAllowExecution`)를 추가해 막힌 자식 제외 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxBTComposite_RandomChoice.h` | 클래스 doc 주석을 새 동작에 맞게 갱신 | 수정 |

### 접근 방식
- **후보 사전 필터**: 후보 수집 루프에서 `bAvoidRepeat` 회피 체크 직후, Weight 조회 전에 `DoDecoratorsAllowExecution(SearchData.OwnerComp, SearchData.OwnerComp.GetActiveInstanceIdx(), Index)`로 자식의 조건 Decorator를 평가해 false면 후보에서 제외한다. 이는 엔진의 `FindChildToExecute`가 자식 실행 직전에 호출하는 바로 그 검사라, 미리 걸러도 선택 결과가 엔진 판정과 어긋나지 않는다.
- **`RandomWeight`는 영향 없음**: 항상 true를 반환하므로 필터에 걸리지 않고, `CompareAttributeRatio` 같은 진짜 조건만 후보를 거른다.
- **전부 막히면 실패**: 통과 후보가 0개면 기존 "후보 0개 / TotalWeight ≤ 0" 경로로 `ReturnToParent`(실패)를 반환한다. 기존 시멘틱 유지.

### 범위 밖
- "폴백 없음" 시멘틱 자체는 유지 — 선택된 자식이 *런타임에* 실패하면 여전히 다른 자식으로 폴백하지 않는다. 이번 변경은 *실행 전* 조건 필터링에만 해당한다.
- 설계문서 `WX_BT_기능정리.md` 갱신, #2(에디터 시각 구분)·#3(FP 폴백)은 다루지 않음.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../Private/WxBTComposite_RandomChoice.cpp` | 후보 수집 루프에 `DoDecoratorsAllowExecution` 통과 검사 추가 — 조건 Decorator가 막는 자식을 후보에서 제외 | 수정 |
| `Plugins/WxAI/.../Public/WxBTComposite_RandomChoice.h` | 클래스 doc 주석을 "유효 후보 중에서만 가중 추첨 / 폴백 없음은 런타임 실패에만 적용"으로 갱신 | 수정 |

### 구현·결정과 그 이유
- **엔진 API `DoDecoratorsAllowExecution` 재사용**: 조건 평가 로직을 직접 재구현하지 않고 엔진이 `FindChildToExecute`에서 자식 실행 직전에 쓰는 바로 그 함수를 사전 필터로 호출했다. 인스턴스 인덱스도 엔진과 동일하게 `SearchData.OwnerComp.GetActiveInstanceIdx()`를 사용해, 사전 필터 판정과 엔진의 실행 직전 판정이 어긋날 여지를 없앴다.
- **회피 체크 직후·Weight 조회 전 위치**: `bAvoidRepeat`로 걸러진 자식은 조건 평가조차 불필요하므로 회피 다음에 두고, 통과한 자식만 가중치를 모으도록 했다.
- **전부 막힘 = 기존 실패 경로 재사용**: 통과 후보 0개는 `Candidates.Num() == 0`/`TotalWeight <= 0` 경로가 그대로 `ReturnToParent`(실패)로 처리 — 새 분기 추가 없음.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 에디터 BT 디버거로 실동작 검증(조건 실패 자식이 추첨에서 빠지는지, 전부 실패 시 노드 실패)은 미수행 — 컴파일까지 확인.
- 설계문서 `WX_BT_기능정리.md`의 RandomChoice 항목은 여전히 "균등 확률"로만 서술돼 있어(가중치·조건 필터 미기재) 갱신 필요. 기획서는 직접 수정하지 않으므로 기획자에게 반영 요청.
