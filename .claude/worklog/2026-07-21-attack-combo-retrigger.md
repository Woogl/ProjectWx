# Attack 콤보를 엔진 재발동(retrigger) 구조로 전환

## 계획

### 목표
Skill 콤보와 동일하게([[2026-07-21-skill-combo-retrigger]]) Attack 콤보를 엔진 순정 재발동(`bRetriggerInstancedAbility`) 구조로 통일한다. 버그 수정은 아니고(Attack은 UI 진입점이 없어 기존 `WaitInputPress`로 정상 동작) 구조 일관화·단순화가 목적이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxAbility_Attack.h` | `WaitInputPress` 계열(태스크·전방선언·`WaitForComboInput`·`HandleComboInputPressed`·`Reactivate`)과 `NextComboPath` 제거. `CanActivateAbility` 오버라이드 선언. 헬퍼 `ResolveNextComboPath()` 추가 | 수정 |
| `WxAbility_Attack.cpp` | 생성자 `bRetriggerInstancedAbility=true`. `CanActivateAbility`(재발동은 콤보 윈도우 + 유효 입력 + 비용/쿨다운/사망, 자기 차단 우회; 신규는 `Super`). `ActivateAbility`는 `ResolveNextComboPath()`로 경로 결정. `EndAbility`는 몽타주 태스크만 정리(경로 리셋 안 함). 몽타주 종료 핸들러가 `CurrentPath` 비움. `WaitInputPress` include 제거 | 수정 |

ASC·VM 추가 변경 없음(기존 라우팅이 retriggerable 되면 자동으로 재발동 경로로 감).

### 접근 방식
- **Skill과 다른 핵심**: Attack은 L/H 분기 트리라 현재 노드에서 그 입력이 이어질 경로가 없으면 **입력을 무시하고 현재 몽타주를 유지**해야 한다. 재발동은 `CanActivateAbility` 통과 → EndAbility(몽타주 끊김) 순이라, 이 "무시" 판정을 `CanActivateAbility`로 올린다. 엔진은 `CanActivateAbility`를 재발동 종료보다 먼저 부르므로, 무효 입력은 재발동 거부 → 몽타주 유지가 된다.
- **경로 해석 단일화**: `ResolveNextComboPath()` const 헬퍼가 `LastPressedInputTag`(L/H)+`CurrentPath`로 "다음 경로 or 빈 문자열(무시)"을 계산한다. `CanActivateAbility`(게이트: 빈 문자열이면 거부)와 `ActivateAbility`(적용)가 공유해 로직 중복과 `NextComboPath` stash를 제거한다.
- **상태 단일 원천**: `CurrentPath`가 원천. 재발동의 EndAbility는 보존하고(다음 ActivateAbility가 이어감), 콤보 자연 종료는 몽타주 핸들러가 비운다. 재발동 EndAbility가 몽타주 태스크 종료 콜백으로 경로를 되돌리지 않도록 `EndTask`로 콜백을 먼저 해제한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAbility_Attack.h` | `WaitInputPress` 계열·`Reactivate`·`NextComboPath` 제거. `CanActivateAbility` 오버라이드 선언. `ResolveNextComboPath()` 헬퍼 추가. `CurrentPath` 주석에 재발동 보존/자연종료 리셋 명시 | 수정 |
| `WxAbility_Attack.cpp` | 생성자 `bRetriggerInstancedAbility=true`. `CanActivateAbility`(재발동=콤보 윈도우+`ResolveNextComboPath` 유효+`CheckCost`/`CheckCooldown`+`ActivationBlockedTags`, 자기 차단 우회; 신규는 `Super`). `ActivateAbility`는 `ResolveNextComboPath()`로 경로 결정. `EndAbility`는 몽타주 태스크만 `EndTask` 정리(경로 리셋 안 함). 종료 핸들러 3종이 `CurrentPath` 비움. `WaitInputPress` include 제거 | 수정 |

### 구현·결정과 그 이유
- **분기 유효성 판정을 `CanActivateAbility`로**: Skill과 달리 Attack은 L/H 트리라 현재 노드를 잇지 못하는 입력은 무시하고 몽타주를 유지해야 한다. 재발동은 CanActivate 통과 후 EndAbility(끊김)라, 유효성 판정을 CanActivate로 올려 무효 입력은 재발동 자체를 거부(=몽타주 유지)했다. 엔진이 CanActivate를 재발동 종료보다 먼저 부르는 순서를 그대로 활용.
- **`ResolveNextComboPath()` 단일화**: `LastPressedInputTag`(L/H)+`CurrentPath`로 다음 경로(또는 무시=빈 문자열)를 한 곳에서 계산하고 CanActivate(게이트)·ActivateAbility(적용)가 공유. 기존의 `NextComboPath` stash와 `Reactivate` 왕복이 사라져 Skill과 대칭적이고 더 단순해졌다.
- **서버 L/H 정합**: 하드웨어 입력이 `AbilityInputTagPressed`에서 `SetLastPressedInputTag`(서버 복제)를 재발동 RPC보다 먼저 보내므로, 서버 `ActivateAbility`도 같은 L/H로 같은 경로를 고른다.

### 계획 대비 달라진 점
- 계획대로. (ASC·VM 추가 변경 없이, 기존 라우팅이 retriggerable 어빌리티를 자동으로 재발동 경로로 보냄)

### 후속 과제
- 에디터 PIE에서 약/강공격 콤보 진행, 분기 노드에서 반대 입력 무시(몽타주 유지), 터미널 첫타 재시작 확인(사용자 진행).
