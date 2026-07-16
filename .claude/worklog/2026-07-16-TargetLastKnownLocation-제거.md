# TargetLastKnownLocation 완전 제거

> 앞선 「청각 감지 시 소리 발생원을 TargetActor로」의 후속 2단계.

## 계획

### 목표
더 이상 소비처가 없어진 Blackboard 키 `TargetLastKnownLocation`을 코드에서 완전히 제거한다. 에셋 쪽(BB_Enemy 키 + BT_Enemy 참조 노드)은 이미 에디터에서 제거되어 있어(작업 트리에 반영됨), 남은 코드 참조만 정리한다.

### 배경 (제거해도 안전한 근거)
- `TargetLastKnownLocation`은 코드에서 **읽는(Get) 곳이 전혀 없다** — 쓰기(Set/Clear)만 있었다.
- 유일한 소비처였던 BT 노드는 이미 제거됨(HEAD엔 있으나 작업 트리 BB_Enemy·BT_Enemy엔 참조 0).
- TargetActor가 비워지는 시점(억제/사망)에는 LastKnown도 항상 함께 비워져, "타겟 없음 + LastKnown 있음" 상태 자체가 발생하지 않았다 → 조사형 경로는 이미 도달 불가(vestigial).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp` | 손실 시 LastKnown 갱신하던 `else if` 분기 제거(+불필요해진 BB 지역변수/가드 정리), 억제·사망의 `ClearTargetLastKnownLocation` 호출 제거, 관련 주석 정리 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` | 억제 doc의 `LastKnown` 표기 제거 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` | `TargetLastKnownLocation` FName 선언·Set/Clear accessor 선언·Vector Clear 예시 주석 제거 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp` | 위 FName 정의·Set/Clear accessor 정의 제거 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp` | 스테일 주석의 `(TargetLastKnownLocation)` 표기 제거 | 수정 |

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../WxAIPerceptionComponent.cpp` | 손실 `else if` 분기·억제/사망 Clear 호출 제거, `HandleTargetPerceptionUpdated`에서 불필요해진 BB 지역변수/널가드 제거, 주석 정리 | 수정 |
| `Plugins/WxAI/.../WxAIPerceptionComponent.h` | 억제 doc의 `LastKnown` 표기 제거 | 수정 |
| `Plugins/WxAI/.../WxBlackboardKeys.h` | FName 선언·Set/Clear accessor 선언·Vector Clear 예시 주석 제거 | 수정 |
| `Plugins/WxAI/.../WxBlackboardKeys.cpp` | FName 정의·Set/Clear accessor 정의 제거 | 수정 |
| `Plugins/WxCombat/.../WxCombatAttributeSet.cpp` | 스테일 주석의 키 표기 제거 | 수정 |

### 구현·결정과 그 이유
- **손실 분기 통째 제거**: 그 분기의 유일한 동작이 LastKnown 쓰기였고 TargetActor는 손대지 않았다. 제거해도 "감지 실패 시 TargetActor 유지"는 그대로다 — 실패 경로에서 아무것도 안 하므로 자연히 보존된다. 이 의도를 주석으로 명시해 남겼다.
- **BB 지역변수/널가드 제거**: else if 삭제로 `HandleTargetPerceptionUpdated`의 `BB` 지역변수가 미사용이 됐다. `SetTargetActor`·`UpdateRecognition`이 각자 내부에서 BB를 다시 얻고 널가드하므로 앞단 조기 가드를 없애도 동작·안전성이 동일하다(방어적 선언 지양).
- **Vector Clear 예시 주석 제거**: 남은 Vector 키(Home/Patrol)는 Set 전용이라 Vector용 Clear 설명이 무의미해졌다. Float(TargetDistance) Clear 설명은 유지.

### 계획 대비 달라진 점
- 계획대로. (당초 1단계 worklog엔 "에디터에서 BB/BT 편집 후 코드 정리"로 적었는데, 실제로는 사용자가 에셋 편집을 이미 완료해 둔 상태여서 코드 정리만 수행)

### 후속 과제
- 없음. (에셋·코드 양쪽에서 `TargetLastKnownLocation` 참조 0 확인, WxEditor(Development) 빌드 성공)
