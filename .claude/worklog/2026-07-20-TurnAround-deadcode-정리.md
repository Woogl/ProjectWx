# TurnAround dead code 정리

## 계획

### 목표
미실행 dead code `UWxAbilityTask_TurnAround`(상호작용 응시용이었으나 2026-07-19 호출부 제거)를 삭제하고, 공격/스킬 헤더의 낡은 주석("타겟 방향 회전은 ANS_TurnAround이 담당")을 실제 시스템(`ANS_SnapToTarget`)에 맞게 정정한다. 타겟 회전은 이미 `WxAnimNotifyState_SnapToTarget` + `WxRootMotionModifier_SnapToTarget`(루트모션)으로 이관돼 있어 런타임 동작 변화는 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Task/WxAbilityTask_TurnAround.h` | 파일 삭제 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_TurnAround.cpp` | 파일 삭제 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Attack.h` | :26 주석을 "타겟 방향 회전은 ANS_SnapToTarget이 담당."으로 정정 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Skill.h` | :26 주석 동일 정정 | 수정 |

### 접근 방식
- **삭제 안전성**: `grep WxAbilityTask_TurnAround`(Intermediate 제외) 참조처는 자기 헤더/구현과 worklog뿐. 다른 소스의 include·호출 0 → 삭제해도 컴파일·링크 무영향.
- **Intermediate 잔재 미조치**: 이미 소스가 없는 `WxAnimNotifyState_TurnAround.*`/`WxAnimNotifyState_MotionWarpTurnAround.*` 및 이번 삭제분의 `.obj/.old`는 UBT 디렉터리 스캔으로 재빌드 시 링크에서 자연 제외.
- **주석만 갱신, 로직 불변**: 콤보 흐름 등 나머지 주석은 유지하고 어긋난 한 줄만 `ANS_` 접두 표기(기존 `ANS_ComboWindow`와 일관)로 정정.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Task/WxAbilityTask_TurnAround.h` | 파일 삭제 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_TurnAround.cpp` | 파일 삭제 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Attack.h` | :26 주석 "ANS_TurnAround" → "ANS_SnapToTarget" 정정 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Skill.h` | :26 주석 동일 정정 | 수정 |

### 구현·결정과 그 이유
- **dead code 삭제**: `UWxAbilityTask_TurnAround`는 상호작용 응시 전용이었고 2026-07-19 그 사용처가 제거되며 호출부·include가 전부 사라졌다. `CreateTask`가 `UFUNCTION`이 아니라 BP 노출도 없어 실행 경로가 0이므로, 남겨두면 "회전은 TurnAround가 한다"는 오해만 유발한다. 실제 회전 주체(`ANS_SnapToTarget` + `WxRootMotionModifier_SnapToTarget`)만 코드에 남긴다.
- **주석만 정정, 로직 불변**: 두 헤더의 콤보 흐름 설명 등은 유지하고 어긋난 한 줄만 실제 시스템명으로 고쳤다. 런타임 동작은 손대지 않았다.
- **Intermediate 잔재 미조치**: 이번 빌드에서 UBT가 `source file removed`로 makefile을 무효화하고 재링크했으며, 삭제 파일 참조로 인한 링크 에러 없이 통과했다. `.obj/.old` 잔재는 별도 정리 불필요.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 `Result: Succeeded` (약 22초). WxCombat·WxGame 재컴파일·재링크 정상. 삭제 대상이 미실행 코드라 인게임 동작 검증은 불필요.

### 후속 과제
- (선택) 메모리 `feedback_turnaround_rootmotion.md`는 `SetActorRotation` 방식 시절 교훈이라 현재 RootMotionModifier 이관 사실과 어긋난다. 갱신 여부는 사용자 판단.
