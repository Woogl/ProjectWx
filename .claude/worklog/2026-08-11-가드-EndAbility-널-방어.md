# 가드 EndAbility 널 방어

## 계획

### 목표
`UWxAbility_Guard::EndAbility`가 `ActorInfo`를 널 검사 없이 역참조한다. 엔진이 널을 전제로 방어하는 경로가 있고 같은 모듈의 형제 어빌리티 6종은 모두 막고 있어, Guard에도 동일한 방어를 넣는다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp` | `EndAbility`의 ASC 획득을 `ActorInfo ? ... : nullptr` 삼항으로 변경 | 수정 |

### 접근 방식
- **Sprint의 삼항 관용구를 따른다**: Guard의 두 태그 정리 블록은 이미 `ASC &&`로 널을 막고 있어, ASC를 얻는 첫 줄만 널 안전하게 바꾸면 나머지는 손댈 필요가 없다. Dodge·LockOn의 래핑 형태는 블록 두 개를 다시 감싸야 해 변경 폭만 커진다.
- 몽타주 상태 리셋과 `Super::EndAbility` 호출은 `ActorInfo`와 무관하므로 그대로 둔다 — 널이어도 수행되어야 한다.
- 주석은 달지 않는다. 형제 6종 어느 것도 이 가드에 주석을 붙이지 않았고 코드에서 의도가 읽힌다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp` | `EndAbility`의 ASC 획득을 널 안전한 삼항으로 변경 (1줄) | 수정 |

### 구현·결정과 그 이유
- **최소 변경**: 뒤따르는 두 태그 정리 블록이 이미 `ASC &&`로 널을 막고 있었으므로, ASC를 얻는 한 줄만 고치면 방어가 완성된다. 블록을 다시 감싸는 형태는 얻는 것 없이 diff만 키운다.
- **형제와 같은 관용구**: 모듈에 두 가지 방어 형태가 공존하는데(삼항 / `IsValid()` 래핑), Guard의 기존 구조에 삼항이 그대로 맞아떨어져 Sprint 쪽을 따랐다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 널 경로는 의도적으로 재현하기 어려워 정상 경로 회귀만 확인 대상이다 — 가드 진입·해제 시 `State.Guard` 부착·해제, 가드브레이크 후 종료.
