# NameplateComponent: ShowIfAny/HideIfAny를 FGameplayTagRequirements로 통합

## 계획

### 목표
네임플레이트 표시 조건을 두 컨테이너(ShowIfAny/HideIfAny)에서 엔진 표준 `FGameplayTagRequirements` 단일 프로퍼티로 합친다(LockOnPointComponent와 동일 방식). 현재 동작은 보존한다.

### 수정 범위
| 모듈 | 파일 | 수정할 내용 |
|---|---|---|
| `WxUI` | `Public/Component/WxNameplateComponent.h` | ShowIfAny/HideIfAny 제거, `FGameplayTagRequirements VisibilityRequirements` 추가, 인클루드 교체 |
| `WxUI` | `Private/Component/WxNameplateComponent.cpp` | 생성자 기본값·RefreshVisibility를 Requirements 기반으로 |
| `WxUI` | `README.md` | 35행 `(ShowIfAny/HideIfAny)` → `(VisibilityRequirements)` |

### 접근 방식
ShowIfAny는 OR(HasAny)이고 기본값이 태그 2개를 OR로 쓴다. RequireTags는 AND(HasAll)라 그대로 옮기면 동작이 깨진다. 따라서:
- 표시(OR) 조건 → `TagQuery = MakeQuery_MatchAnyTags({InCombat, LockedOn})`
- 숨김 조건 → `IgnoreTags = {Dead}` (HasAny면 숨김, 의미 일치)
- RefreshVisibility → `GetOwnedGameplayTags` 후 `RequirementsMet()`.
RequirementsMet = HasAll(Require=∅)=true && !HasAny(Ignore=Dead) && TagQuery(AnyOf) → 기존 식과 정확히 동일.

영향 확인: ShowIfAny/HideIfAny 오버라이드 BP 없음(스냅샷 grep). WxUI는 이미 GameplayAbilities/GameplayTags 의존.

### 트레이드오프
- 표시 조건 BP 오버라이드가 TagQuery 편집으로 바뀜(현재 오버라이드 BP 없어 즉시 영향 없음).
- RequirementsMet의 보유 태그 컨테이너 1회 복사(무시 가능).

---

## 완료

### 수정한 파일
| 모듈 | 파일 | 수정 내용 |
|---|---|---|
| `WxUI` | `Public/Component/WxNameplateComponent.h` | ShowIfAny/HideIfAny 제거 → `FGameplayTagRequirements VisibilityRequirements`, 인클루드 GameplayEffectTypes.h로 교체, 주석 갱신 |
| `WxUI` | `Private/Component/WxNameplateComponent.cpp` | 생성자 기본값(IgnoreTags=Dead, TagQuery=AnyOf{InCombat,LockedOn}), RefreshVisibility를 RequirementsMet로 |
| `WxUI` | `README.md` | 표시 조건 표기 `(ShowIfAny/HideIfAny)` → `(VisibilityRequirements)` |

### 구현·결정과 그 이유
- **OR 보존 위해 TagQuery 사용**: ShowIfAny는 HasAny(OR)이고 기본값이 태그 2개를 OR로 쓴다. RequireTags(HasAll)로 옮기면 "둘 다 보유해야 표시"로 깨지므로, 표시 조건을 `MakeQuery_MatchAnyTags`로 TagQuery에 넣어 기존 동작을 그대로 보존했다. 숨김은 IgnoreTags(HasAny면 숨김)와 의미가 일치해 직접 매핑.
- **RequirementsMet 단일 평가**: RefreshVisibility가 `GetOwnedGameplayTags` 후 `RequirementsMet` 한 번으로 평가. HasAll(∅)=true && !HasAny(Dead) && Query(AnyOf) = 기존 식과 정확히 등가.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. (표시 조건을 BP에서 오버라이드할 일이 생기면 OR 추가는 Query Must Match로 편집한다 — 현재 오버라이드 BP 없음.)
