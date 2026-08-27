# `UWxCombatLibrary::RemoveEffect` 제거

## 계획

### 목표

`RemoveEffect` 는 널 검사 두 줄과 엔진 호출 한 줄이 전부인 래퍼이고, 그 널 검사의 절반은 엔진이 이미 하고 있다 — `RemoveActiveGameplayEffectBySourceEffect` 가 `if (GameplayEffect)` 로 시작한다. 짝인 `ApplyEffect` 는 스펙 구성과 예측 키 산출이라는 실제 일을 하지만 이쪽은 이름만 한 겹 덧씌운 셈이라 걷어낸다. 호출부는 두 곳뿐이고 `UFUNCTION` 이 아니어서 블루프린트 참조도 없다.

함수가 사라지면 헤더 주석에 적힌 설계 근거도 같이 사라진다. `nullptr, 1` 인자는 취향이 아니라 2026-08-24 에 확정된 결정이고 08-14 에 Begin/End 짝이 기각됐던 근거를 막는 부분이라, 이 근거를 호출부로 옮기는 것이 이 작업의 실질적인 내용이다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Public/WxCombatLibrary.h` | `RemoveEffect` 선언과 doc 주석 삭제 | 수정 |
| `Plugins/WxCombat/Private/WxCombatLibrary.cpp` | `RemoveEffect` 정의 삭제 | 수정 |
| `Plugins/WxCombat/.../AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp` | `NotifyEnd` 의 호출을 엔진 호출로 풀어쓰고 근거 주석 부여 | 수정 |
| `Plugins/WxCombat/.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `OnDestroy` 의 호출을 엔진 호출로 풀어쓰고 근거 주석 부여 | 수정 |

인클루드는 둘 다 그대로 둔다 — 두 파일 모두 `AbilitySystemComponent.h` 를 이미 포함하고 있고, `WxCombatLibrary.h` 는 각각 `ApplyEffect` 때문에 계속 필요하다.

### 접근 방식

- **`NotifyEnd`**: ASC 널 검사는 기존 `if` 가 이미 하고 `EffectClass` 널은 엔진이 거른다. 호출 한 줄로 대체한다.
- **`OnDestroy`**: 태스크의 ASC 가 약참조라 `.Get()` 결과를 `if` 초기화문으로 받아 감싼다. 래퍼가 대신 해 주던 널 검사가 여기서만 실제로 필요하다.
- **근거 주석은 나누어 싣는다**: 같은 두 문장을 양쪽에 복붙하면 한쪽만 고쳐지며 어긋난다. 각 호출부에서 실제로 걸리는 쪽만 한 줄씩 남긴다 — 노티파이에는 수량 1 의 근거(늦게 도착한 `NotifyEnd` 가 이미 걸린 처형 무적을 벗기는 경로), 컷신 태스크에는 정의 조회의 근거(예측으로 건 GE 의 핸들은 서버본이 오면 무효해진다).
- **`ApplyEffect` 는 남긴다**: 풀어쓰면 호출부마다 대여섯 줄이 붙는다. 노티파이의 Begin/End 가 "헬퍼 : 엔진 호출" 로 비대칭이 되지만 두 쪽의 무게가 애초에 다르다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Public/WxCombatLibrary.h` | `RemoveEffect` 선언과 doc 주석 삭제 | 수정 |
| `Plugins/WxCombat/Private/WxCombatLibrary.cpp` | `RemoveEffect` 정의 삭제 | 수정 |
| `Plugins/WxCombat/.../AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp` | `NotifyEnd` 를 엔진 호출로 풀어쓰고 수량 1 의 근거를 주석으로 | 수정 |
| `Plugins/WxCombat/.../Task/WxAbilityTask_PlaySkillCutscene.cpp` | `OnDestroy` 를 널 검사로 감싼 엔진 호출로 풀어쓰고 정의 조회의 근거를 주석으로 | 수정 |

### 구현·결정과 그 이유

- **널 검사를 옮기지 않고 필요한 쪽에만 남김**: 래퍼가 하던 두 검사 중 `EffectClass` 쪽은 엔진이 같은 조건으로 이미 거르고 있어 되살리지 않았다. ASC 쪽은 노티파이가 이미 `if` 로 받고 있었고, 컷신 태스크만 약참조라 실제로 필요해 그쪽에만 붙였다.

- **설계 근거를 호출부로 쪼개 옮김**: `nullptr, 1` 은 08-14 에 기각됐던 Begin/End 짝을 다시 성립시킨 결정이라 근거가 사라지면 다음 사람이 되돌린다. 같은 문장을 양쪽에 복붙하면 한쪽만 고쳐지며 어긋나므로, 각 호출부에서 실제로 걸리는 쪽만 남겼다 — 노티파이에는 수량 1(늦게 도착한 `NotifyEnd` 가 처형 무적을 벗기는 경로), 컷신에는 정의 조회(예측 핸들이 서버본 도착 시 무효).

- **`ApplyEffect` 는 남김**: 스펙 구성과 예측 키 산출이 실제 일이라 풀어쓰면 호출부마다 대여섯 줄이 붙는다. 노티파이의 Begin/End 가 헬퍼와 엔진 호출로 비대칭이 됐지만 두 쪽의 무게가 애초에 다르다.

### 계획 대비 달라진 점
계획대로.

### 후속 과제
- 없음. 빌드 확인까지 마쳤고 인자·호출 대상이 그대로라 런타임 동작은 달라지지 않는다.
