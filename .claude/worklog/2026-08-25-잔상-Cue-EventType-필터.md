# 잔상 Cue의 EventType 필터·스폰 결과 검증

## 계획

### 목표
`UWxCueNotify_GhostTrail::HandleGameplayCue`가 형제 Cue들과 형태가 어긋나 (a) EventType을 거르지 않고 (b) `MyTarget`을 널 검사 없이 역참조하며 (c) `SpawnActor` 결과를 검사하지 않는다. 형제 Cue와 같은 순서로 정렬해 셋을 함께 없앤다. (`module_review_WxCombat.md` 이슈 2)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp` | `HandleGameplayCue` 본문을 `Super` 우선 → `Executed`·`MyTarget`·`GhostTrailClass` 조기 반환 → 월드 검사 → 스폰 결과 검사 순으로 재작성 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Cue/WxCueNotify_GhostTrail.h` | 쓰이지 않게 된 `SpawnedGhostTrail` 필드 제거 | 수정 |

### 접근 방식
- **형제 Cue 형태로 정렬**: `WxCueNotify_Hit`·`WxCueNotify_PerfectGuard`·`WxCueNotify_DamageFloater`가 모두 쓰는 `Super` → `EventType != Executed || !MyTarget` 조기 반환 → 월드 지역 변수 순서를 그대로 따른다. 부모 `UGameplayCueNotify_Static::HandleGameplayCue`가 EventType 디스패처라, 필터가 없으면 이 태그가 지속형 GE에 실릴 때 OnActive·WhileActive·Removed마다 잔상이 중복 스폰된다.
- **`SpawnedGhostTrail` 멤버 제거**: 스폰 결과를 지역 변수로 받으면 이 필드는 참조처가 사라진다. 부모 `UGameplayCueNotify_Burst`는 `UGameplayCueNotify_Static` 파생이라 인스턴스 없이 CDO에서 실행되므로, 그 필드는 모든 잔상·모든 월드가 공유하는 CDO 상태였다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp` | `HandleGameplayCue`를 `Super` 우선 → `Executed`·`MyTarget`·`GhostTrailClass` 조기 반환 → 월드 검사 → 스폰 결과 검사 순으로 재작성 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Cue/WxCueNotify_GhostTrail.h` | `SpawnedGhostTrail` 필드 제거 | 수정 |

### 구현·결정과 그 이유
- **형제 Cue와 같은 형태**: `WxCueNotify_Hit`·`PerfectGuard`·`DamageFloater`가 쓰는 순서를 그대로 따랐다. 부모가 EventType 디스패처라 필터가 없으면 이 태그가 지속형 GE에 실릴 때 OnActive·WhileActive·Removed마다 잔상이 중복 스폰된다.
- **`GhostTrailClass` 검사를 같은 조기 반환에 합류**: 조건이 셋 다 "스폰할 이유가 없다"는 같은 층위라 분기를 나누지 않았다.
- **`SpawnedGhostTrail` 제거**: 부모 `UGameplayCueNotify_Burst`는 `UGameplayCueNotify_Static` 파생이라 인스턴스 없이 CDO에서 실행된다. 그 필드는 모든 잔상·모든 월드(PIE 포함)가 공유하는 CDO 상태였고 매 스폰마다 덮어써지며 마지막 하나에 하드 레퍼런스를 남겼다. 스폰 결과를 지역 변수로 받으면 참조처가 사라지므로 함께 지웠다. 수명은 그대로 `SetLifeSpan`이 책임진다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
