# Phase BB 값과 UWxAbility_Pattern_Phase 제거

## 계획

### 목표
저장식 페이즈 상태인 `Phase` Blackboard 값이 사실상 죽은 값(BB_Enemy에 키 미등록 → 기록이 no-op, 읽는 C++ 없음)이므로, 이 값과 그 유일한 기록자 `UWxAbility_Pattern_Phase` 클래스, 그리고 고아가 되는 `Ability.Pattern.Phase` 태그를 함께 제거한다. 페이즈 게이팅은 이미 HP 비율(`UWxBTDecorator_AttributeRatio`)로 파생 처리하는 방향이라 저장 상태는 원천과 중복이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h` | `Phase` 키 선언 삭제 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBlackboardKeys.cpp` | `Phase` 키 정의 삭제 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Pattern_Phase.h` | 클래스 헤더 삭제 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern_Phase.cpp` | 클래스 구현 삭제 | 삭제 |
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | `Ability_Pattern_Phase` 태그 선언 삭제 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | `Ability_Pattern_Phase` 태그 정의 삭제 | 수정 |

### 접근 방식
- **C++ 심볼 3종 제거**: `WxBlackboardKeys::Phase`, `UWxAbility_Pattern_Phase`, `WxGameplayTags::Ability_Pattern_Phase`. 세 심볼 모두 이 클래스 외 C++ 참조가 없음을 확인함. 헤더를 include 하는 다른 파일도 없음.
- **Content는 사용자가 에디터에서 정리**: BP 자식 `GA_Pattern_Phase2`, 이를 부여하는 `ABS_Boss`, 태그로 발동하던 `BT_Boss` 노드, `BT_Custer`의 잔여 Phase 참조. (부모/태그 제거로 댕글링이 되므로 필수 후속)

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/.../WxBlackboardKeys.h` | `Phase` 키 선언 제거 | 수정 |
| `Plugins/WxAI/.../WxBlackboardKeys.cpp` | `Phase` 키 정의 제거 | 수정 |
| `Plugins/WxCombat/.../WxAbility_Pattern_Phase.h` | 클래스 헤더 삭제 | 삭제 |
| `Plugins/WxCombat/.../WxAbility_Pattern_Phase.cpp` | 클래스 구현 삭제 | 삭제 |
| `Plugins/WxCore/.../WxGameplayTags.h` | `Ability_Pattern_Phase` 태그 선언 제거 | 수정 |
| `Plugins/WxCore/.../WxGameplayTags.cpp` | `Ability_Pattern_Phase` 태그 정의 제거 | 수정 |

### 구현·결정과 그 이유
- **심볼 3종 완전 제거**: BB 키·클래스·태그를 함께 지웠다. 저장 상태(Phase 정수)가 원천(HP 비율, `AttributeRatio`)과 중복이고, BB_Enemy에 키조차 없어 기록이 no-op였기 때문. 남기면 고아 심볼이 된다.
- **잔여 참조 0건 확인**: 제거 전후로 소스 트리(Intermediate 제외)에서 `WxAbility_Pattern_Phase`/`Ability_Pattern_Phase`/`BlackboardKeys::Phase` grep — 모두 무참조. 헤더를 include 하는 외부 파일도 없음.

### 계획 대비 달라진 점
- 코드 변경은 계획대로.
- **빌드 검증 보류**: WxEditor(Development) 빌드가 UHT 단계에서 `WxCharacterBase.h(82): UWxContextEffectsComponent 미발견`으로 실패. 이는 **병행 세션이 진행 중인 ContextEffects 제거 작업**의 미완 상태(컴포넌트 파일은 삭제됐으나 WxCharacterBase가 아직 참조) 탓으로, 본 작업과 무관. 해당 파일은 타 세션 소유라 손대지 않음.

### 후속 과제
- 병행 세션의 ContextEffects 정리가 정합 상태가 된 뒤 WxEditor 재빌드로 본 변경 컴파일 최종 확정.
- Content 정리(사용자): `GA_Pattern_Phase2` 삭제, `ABS_Boss` 부여 항목 제거, `BT_Boss`의 `Ability.Pattern.Phase` 발동 노드 제거, `BT_Custer` 잔여 Phase 참조 확인. (BB_Enemy의 Phase 키는 타 세션에서 이미 처리됨)
