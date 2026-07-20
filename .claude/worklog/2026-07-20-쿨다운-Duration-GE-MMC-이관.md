# 쿨다운 Duration을 GE/MMC 소유로 이관 + 쿨다운 인스턴스 최소화

## 계획

### 목표
`UWxAbilityBase`의 쿨다운 Duration 계산(CooldownTime + 직렬 회복분)을 `UWxEffect_Cooldown`(+MMC)이 소유하도록 옮긴다. 그 결과 `ApplyCooldown` 오버라이드가 순정과 같아져 삭제되고, 활성 쿨다운 쿼리는 쿨다운 GE 도메인으로 이동한다. MaxRecharges 표시용 per-ability 인스턴스는 (WxUI 경계상 완전 제거 불가) 남기되 다중 충전 어빌리티에만 생성해 최소화한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxMMC_CooldownDuration.h`·`.cpp` | `UWxMMC_CooldownDuration`: 컨텍스트→어빌리티→AbilityDataRow.CooldownTime + 직렬 LongestRemaining(ASC 쿼리) 반환 | 신규 |
| `WxCombat/.../Effect/WxEffect_Cooldown.h`·`.cpp` | `DurationMagnitude`를 SetByCaller→CustomCalc(MMC)로 교체, static `QueryActiveCharges` 추가(어빌리티 쿼리 본문 이관), 주석 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.h`·`.cpp` | `ApplyCooldown` 삭제, `CheckCooldown`/`GetCooldownTimeRemaining[AndDuration]`을 GE static 호출로 교체, private `QueryActiveCooldowns` 삭제, `GetCooldownGameplayEffect` 인스턴스 최소화, 독 코멘트 갱신 | 수정 |

### 접근 방식
- **Duration MMC**: `Spec.GetEffectContext().GetAbility()`로 소스 어빌리티 CDO를 얻어 `AbilityDataRow.CooldownTime`을 읽고, 시전 ASC의 활성 `UWxEffect_Cooldown` GE 중 이 어빌리티 것의 최장 잔여시간을 더해 반환(직렬 회복). 신규 GE는 duration 계산 시점에 미적용이라 기존 것만 집계 — 현 `ApplyCooldown`과 동일 타이밍.
- **순정 복귀**: `DurationMagnitude`가 MMC가 되면 엔진 순정 `ApplyCooldown`이 올바른 duration으로 적용 → 어빌리티 `ApplyCooldown` 오버라이드 삭제. no-cooldown 게이팅은 `GetCooldownGameplayEffect()` nullptr로 유지.
- **쿼리 응집**: 활성 쿨다운 집계를 `UWxEffect_Cooldown::QueryActiveCharges(ASC, SourceAbilityCDO, ...)` static으로 옮겨 어빌리티 3개 메서드 + MMC가 공유(중복 방지). ViewModel의 제네릭 쿼리는 별개 유지.
- **인스턴스 최소화**: 단일 충전(MaxRecharges ≤ 1)은 `Super::GetCooldownGameplayEffect()`(공유 CDO, ViewModel이 Max(1,StackLimitCount)=1로 읽음), 다중 충전만 `CooldownEffect` 인스턴스 생성.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxMMC_CooldownDuration.h`·`.cpp` | `UWxMMC_CooldownDuration`: 컨텍스트→어빌리티→`AbilityDataRow.CooldownTime` + 시전 ASC 활성 GE 쿼리로 `LongestRemaining` 더해 반환 | 신규 |
| `WxCombat/.../Effect/WxEffect_Cooldown.h`·`.cpp` | `DurationMagnitude`를 SetByCaller→`FCustomCalculationBasedFloat(MMC)`로 교체, static `QueryActiveCharges` 추가, 클래스 주석 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.h`·`.cpp` | `ApplyCooldown` 오버라이드 삭제, `CheckCooldown`/`GetCooldownTimeRemaining[AndDuration]`을 `UWxEffect_Cooldown::QueryActiveCharges` 호출로 교체, private `QueryActiveCooldowns` 삭제, `GetCooldownGameplayEffect` 인스턴스 최소화, 독 코멘트 갱신 | 수정 |

### 구현·결정과 그 이유
- **Duration을 MMC가 소유**: `Spec.GetEffectContext().GetAbility()`로 소스 어빌리티 CDO를 얻어 `AbilityDataRow.CooldownTime`을 읽고, 직렬 회복을 위해 시전 ASC(`GetInstigatorAbilitySystemComponent()`)의 활성 쿨다운을 쿼리해 `LongestRemaining + CooldownTime`을 반환. 신규 GE는 duration 계산 시점에 미적용이라 기존 것만 집계 — 기존 `ApplyCooldown`과 동일 타이밍/결과.
- **ApplyCooldown 오버라이드 삭제**: MMC가 duration을 채우므로 엔진 순정 `ApplyCooldown`(→`GetCooldownGameplayEffect()` 적용)이 그대로 올바르게 동작. no-cooldown은 `GetCooldownGameplayEffect()` nullptr로 게이팅 유지. 코스트에서 `ApplyCost`를 걷어낸 것과 같은 결.
- **쿼리 응집**: 활성 쿨다운 집계를 `UWxEffect_Cooldown::QueryActiveCharges` static으로 이관해 어빌리티 3개 메서드 + MMC가 공유(WxCombat 내 중복 방지). ViewModel(WxUI)의 제네릭 쿼리는 별개 유지.
- **인스턴스 최소화**: 단일 충전(MaxRecharges ≤ 1)은 `Super::GetCooldownGameplayEffect()`(공유 CDO, ViewModel이 `Max(1, StackLimitCount)`=1로 읽음)로 NewObject 생략. 다중 충전에서만 per-ability 인스턴스 생성. `mutable CooldownEffect` 멤버는 다중 충전 캐시용으로 유지.
- **`ModifierOp`/`SetByCaller_Duration`**: `SetByCaller_Duration` 태그는 NoCooldown/InfiniteMP/DrainDP/Groggy가 계속 사용하므로 정의 유지 — 쿨다운 쪽 사용처 2곳만 사라짐.

### 계획 대비 달라진 점
- 계획대로. (헤더의 `class UAbilitySystemComponent;` 전방선언은 `QueryActiveCooldowns` 제거로 미사용이 됐을 수 있으나, 무해·범용 선언이라 이번엔 제거하지 않고 유지.)

### 후속 과제
- **런타임 검증(미완)**: 에디터 플레이에서 (1) 단일 충전 스킬 쿨다운 정상 적용·회복, (2) 다중 충전 스킬 연속 소모 시 직렬 회복 + HUD 충전 pip 수 정확, (3) no-cooldown 무영향, (4) 커스텀 CooldownGE 어빌리티 순정 경로 정상. 컴파일은 WxEditor(Development) 빌드 성공(Result: Succeeded)으로 확인.
- **문서 정합성(선택)**: `Docs/Programmer/Ability_Activation_Flow.md`의 쿨다운 서술(ApplyCooldown SetByCaller 등)이 구식이 됐을 수 있음 — 범위 밖, 필요 시 별도 갱신.

### 후속 리팩터 (동일 세션)
- 사용자 요청으로, 위에서 `UWxEffect_Cooldown`에 static으로 뒀던 활성 쿨다운 쿼리를 **어빌리티의 public 비정적 멤버 `QueryActiveCooldowns`로 환원**했다. 그 쿼리는 "이 어빌리티(소스 CDO)의 활성 쿨다운"이라 어빌리티가 자연스러운 소유자(`this` CDO로 매칭)이고, static 유틸로의 재배치는 과했다.
- MMC(`UWxMMC_CooldownDuration`)는 컨텍스트에서 얻은 어빌리티 CDO에 대해 `WxAbility->QueryActiveCooldowns(...)`를 호출한다(private→public로만 변경). `UWxEffect_Cooldown`은 생성자(Duration=MMC)만 남고 static 쿼리·관련 include/전방선언이 제거됐다. 동작 불변, WxEditor(Development) 빌드 성공.
