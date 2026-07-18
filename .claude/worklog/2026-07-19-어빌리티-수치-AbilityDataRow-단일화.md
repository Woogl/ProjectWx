# 어빌리티 쿨다운/코스트 수치 소스 단일화 (직접입력 필드 제거 → AbilityDataRow only)

## 계획

### 목표
`UWxAbilityBase`가 쿨다운/코스트 수치를 직접입력 필드 4개(`CooldownTime`·`MaxRecharges`·`MPCost`·`UPCost`)와 `AbilityDataRow` 두 곳에서 받는 이중 진실 구조를 없앤다. 직접입력 필드를 제거하고 값은 오직 `AbilityDataRow`에서 온디맨드로 읽어 단일 소스로 만든다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Ability/WxAbilityBase.h` | 4개 UPROPERTY 삭제, `ApplyAbilityTableRow` 선언 삭제, private `GetDataRow()` 추가, 독 코멘트 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.cpp` | `GetDataRow()` 추가, `OnGiveAbility`/`CanEditChange` 정리, 쿨다운·코스트 함수 row 온디맨드 전환, `ApplyAbilityTableRow` 삭제 | 수정 |
| `WxCombat/.../Effect/WxEffect_Cost.h` | MPCost/UPCost 언급 주석을 AbilityDataRow로 정정 | 수정 |
| `WxCombat/.../Ability/WxAbility_Pattern.h`, `_Skill.h`, `_Attack.h` | 쿨다운/충전을 CooldownTime·MaxRecharges로 설정한다는 주석을 AbilityDataRow로 정정 | 수정 |

### 접근 방식
- **온디맨드 읽기**: 값이 필요한 시점(`GetCooldownGameplayEffect`·`CheckCooldown`·`ApplyCooldown`·`GetCostGameplayEffect`)마다 `GetDataRow()`로 `FWxAbilityTableRow`를 해석해 읽는다. `AbilityDataRow`는 `EditDefaultsOnly`라 CDO·인스턴스 양쪽에 handle이 있으므로 CDO를 따로 populate할 필요가 없고, 이에 따라 `ApplyAbilityTableRow`와 `OnGiveAbility`의 CDO populate 로직을 함께 제거한다.
- **스톡 GE 경로 유지**: `CooldownGameplayEffectClass`/`CostGameplayEffectClass` 분기와 row와의 상호배타 편집 처리(`CanEditChange`/`PostEditChangeProperty`)는 그대로 둔다. 요청 범위 밖의 직교 escape hatch.
- **ViewModel 무영향**: `WxViewModel_Ability`는 필드를 직접 안 읽고 `GetCooldownGameplayEffect()`/`GetCostGameplayEffect()`를 통해서만 읽으므로 getter가 row를 읽도록 바뀌면 그대로 동작한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Ability/WxAbilityBase.h` | 4개 UPROPERTY 삭제, `ApplyAbilityTableRow` 선언 삭제, private `GetDataRow()` 추가, 독/멤버 주석 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.cpp` | `GetDataRow()` 추가, `OnGiveAbility`·`CanEditChange` 정리, 쿨다운·코스트 4함수를 row 온디맨드로 전환, `ApplyAbilityTableRow` 삭제 | 수정 |
| `WxCombat/.../Effect/WxEffect_Cost.h` | MPCost/UPCost 출처 주석을 AbilityDataRow로 정정 | 수정 |
| `WxCombat/.../Ability/WxAbility_Pattern.h`, `_Skill.h`, `_Attack.h` | 쿨다운/충전 설정처 주석을 AbilityDataRow로 정정 | 수정 |
| `Docs/Programmer/Ability_Activation_Flow.md` | OnGiveAbility·커밋·프로퍼티 표 서술을 온디맨드/Row-only로 정정 | 수정 |

### 구현·결정과 그 이유
- **온디맨드 읽기(캐시 필드 없음)**: `AbilityDataRow`가 `EditDefaultsOnly`라 CDO·인스턴스 모두 handle을 가지므로, 필요 시점에 `GetDataRow()`로 Row를 해석하면 충분하다. 별도 캐시 멤버가 사라져 이중 진실이 원천적으로 제거되고, `OnGiveAbility`의 CDO populate 훅과 `ApplyAbilityTableRow`도 함께 불필요해졌다. grant 타이밍과 무관해져 오히려 더 견고.
- **clamp 위치 이동**: 기존 `ApplyAbilityTableRow`가 하던 `Max(1, MaxRecharges)` 클램프는 소비 지점(`GetCooldownGameplayEffect`/`CheckCooldown`)에서 `FMath::Max(1, Row->MaxRecharges)`로 인라인 처리.
- **스톡 GE 경로 유지**: `CooldownGameplayEffectClass`/`CostGameplayEffectClass` 분기와 row 상호배타(`CanEditChange`/`PostEditChangeProperty`)는 그대로. 요청 범위 밖의 직교 escape hatch라 손대지 않음(사용자 확인).
- **ViewModel 무영향 확인**: `WxViewModel_Ability`는 getter(`GetCooldownGameplayEffect`/`GetCostGameplayEffect`) 결과의 `StackLimitCount`/`Modifiers`만 읽어 디커플링돼 있음.

### 계획 대비 달라진 점
- 문서 `Ability_Activation_Flow.md` 정정을 추가(계획엔 코드/헤더 주석만 명시). 제거된 프로퍼티를 서술하던 부분이라 오해 방지 차원에서 함께 갱신.

### 후속 과제
- 쿨다운/코스트가 필요한 어빌리티 BP는 `FWxAbilityTableRow` DataTable Row를 만들고 `AbilityDataRow`로 지정해야 한다(직접입력 필드가 사라졌으므로). 기존에 필드를 직접 세팅했던 BP가 있다면 그 값은 로드 시 소실 — 에디터에서 Row로 이관 필요.
- 검증: WxEditor(Development) 빌드 성공(Result: Succeeded)으로 컴파일 확인. 런타임 동작(실제 쿨다운/코스트 적용·HUD 표시)은 에디터 플레이로 별도 확인 권장.
