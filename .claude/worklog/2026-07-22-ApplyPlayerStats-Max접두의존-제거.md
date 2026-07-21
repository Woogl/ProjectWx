# ApplyPlayerStats — "Max" 문자열 접두 의존 제거

## 계획

### 목표
`UWxSaveWorldSubsystem::ApplyPlayerStats`의 어트리뷰트 복원 순서 판정을 `GetName().StartsWith("Max")` 이름 heuristic에서 떼어낸다. WxSave가 구체 AttributeSet 명명 규칙에 결합돼 복원이 조용히 깨질 수 있는 문제(ModuleReview/WxSave.md 🟡)를 없애는 것이 목적이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp` | `ApplyPlayerStats`의 2패스 블록(223~256)을 이름 없는 멱등 2패스로 교체 | 수정 |

### 접근 방식
- **이름 없는 2패스(전량 적용 → 미복원분 재적용)**: 명명 규칙 대신 멱등 재적용으로 순서 의존을 흡수한다.
  - 1패스: 저장 집합의 모든 어트리뷰트를 `SetNumericAttributeBase`로 적용. 이 패스 후 모든 Max base가 저장 값으로 확정된다(Max는 다른 어트리뷰트에 의해 재조정되지 않으므로). current 일부는 잘못된 Max로 클램프/재조정돼 틀어질 수 있다.
  - 2패스: 현재 base가 저장 값과 다른 것만 재세팅. Max는 이미 정확 → skip(재조정 드리프트 방지), 틀어진 current만 올바른 Max로 클램프되어 정확히 안착.
- **근거**: 캡은 독립적이고 current는 캡에 의존해 클램프된다는 GAS 바이탈 어트리뷰트 보편 구조라 1회 교정 패스로 충분. `StartsWith("Max")` 분기 제거. 시그니처·호출부·헤더 변경 없음.
- **대안(도메인 캡 계약, WxCore 인터페이스) 기각**: 플러그인 경계상 배선 비용이 크고 "목록 갱신 누락" 회귀가 잔존. 사용자와 논의 후 이 방식으로 확정.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp` | `ApplyPlayerStats`의 `StartsWith("Max")` 이름 분기 제거, 멱등 2패스로 교체 | 수정 |

### 구현·결정과 그 이유
- **이름 판정 제거**: 1패스는 전량 무조건 적용, 2패스는 `GetNumericAttributeBase == 저장값`이면 skip. 1패스 후 모든 Max가 저장 값으로 확정되므로(Max는 다른 어트리뷰트가 재조정하지 않음) 2패스에서 틀어진 current만 정확한 Max로 클램프되어 복원된다. 캡을 이름·계약으로 식별할 필요가 없어 WxSave의 AttributeSet 독립성 유지.
- **skip 조건(`== 저장값`)을 둔 이유**: 이미 정확한 Max를 2패스에서 재세팅하면 `PostAttributeChange` 비율 재조정이 다시 돌아 float 미세 드리프트가 생길 수 있어 이를 원천 차단. 동시에 불필요한 쓰기도 없앰.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- (권장) 런타임 스모크: 로드 직전 기본 MaxHP가 저장 MaxHP보다 작은 상태에서 로드 시 HP가 잘리지 않고 저장 값으로 복원되는지 확인. 코드 근거상 정확하나 실기 미검증.
