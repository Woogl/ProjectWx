# 코스트 GE 동적 사용 제거 — MMC가 AbilityDataRow를 직접 조회하는 정적 GE로 전환

## 계획

### 목표
`UWxAbilityBase`가 `WxEffect_Cost`를 런타임에 동적으로 다루던 것(`NewObject` 인스턴스 + 매 호출 Modifiers 재구성 + `ApplyCost` 우회 오버라이드)을 없앤다. 코스트 값을 MMC가 계산 시점에 소스 어빌리티의 `AbilityDataRow`에서 직접 조회하게 해, GE Modifiers를 CDO에 정적 선언하고 코스트 경로를 엔진 순정으로 완전 복귀시킨다(어빌리티 오버라이드 0).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxMMC_Cost.h`·`.cpp` | abstract 베이스 `UWxMMC_Cost`(컨텍스트→어빌리티→Row 조회 헬퍼) + 파생 `UWxMMC_MPCost`/`UWxMMC_UPCost` | 신규 |
| `WxCombat/.../Effect/WxEffect_Cost.cpp`·`.h` | 생성자에 MP·UP 정적 모디파이어 2개(Op=Additive, Magnitude=CustomCalc→각 MMC), 헤더 클래스 주석 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.h`·`.cpp` | `GetCostGameplayEffect`·`ApplyCost` 오버라이드 제거, `mutable CostEffect` 멤버 제거, 클래스 독 코멘트 코스트 서술 갱신 | 수정 |

### 접근 방식
- **MMC가 Row 조회**: `CalculateBaseMagnitude_Implementation(Spec)`에서 `Spec.GetContext().GetAbility()`→`UWxAbilityBase` CDO→공개 `AbilityDataRow`→`-MPCost`/`-UPCost`. `AbilityDataRow`는 `EditDefaultsOnly`라 CDO에 존재하고 정적 설계 데이터라 서버/클라 동일(예측 안전).
- **정적 CDO + 순정 경로**: CDO에 MP/UP 두 모디파이어를 정적 선언하면, 순정 `CheckCost`(`CanApplyAttributeModifiers`가 CDO 스펙 생성→`CalculateModifierMagnitudes`에서 MMC 실행)·`ApplyCost`·`GetCostGameplayEffect`가 그대로 동작 → 어빌리티 오버라이드 불필요.
- **`ModifierOp = Additive`**: 순정 `CanApplyAttributeModifiers`는 `Additive` 모디파이어만 자원 부족 판정하므로 명시해 일치 보장(형제 `WxEffect_DrainDP`와 동일). 배선 패턴도 `WxEffect_DrainDP.cpp`를 따른다.
- **MP/UP 두 클래스**: MMC API가 평가 중 모디파이어 인덱스를 안 주므로 어트리뷰트별로 클래스를 분리(선례 `WxMMC_LinearDrain`의 동일 제약).
- **범위 밖**: 쿨다운 커스텀 경로(다중 충전 직렬 회복은 커스텀 불가피), `CostGameplayEffectClass` 기본값·`CanEditChange`/`PostEditChangeProperty` 편집 UX는 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Effect/WxMMC_Cost.h`·`.cpp` | abstract `UWxMMC_Cost`(컨텍스트→어빌리티→Row 조회 헬퍼 `GetCostRow`) + `UWxMMC_MPCost`/`UWxMMC_UPCost`(각 `-MPCost`/`-UPCost` 반환) | 신규 |
| `WxCombat/.../Effect/WxEffect_Cost.cpp`·`.h` | 생성자에 MP·UP 정적 모디파이어 2개(Op=Additive, Magnitude=CustomCalc→각 MMC) 추가, 헤더 클래스 주석을 MMC 방식으로 갱신 | 수정 |
| `WxCombat/.../Ability/WxAbilityBase.h`·`.cpp` | `GetCostGameplayEffect`·`ApplyCost` 오버라이드 삭제, `mutable CostEffect` 멤버·미사용 전방선언 삭제, 클래스 독 코멘트 코스트 서술 갱신 | 수정 |

### 구현·결정과 그 이유
- **MMC가 Row 직접 조회**: 코스트 값을 어빌리티가 런타임에 인스턴스에 채우는 대신, MMC가 `Spec.GetEffectContext().GetAbility()`로 소스 어빌리티 CDO를 얻어 공개 `AbilityDataRow`를 조회한다. CDO의 `AbilityDataRow`는 정적 설계 데이터라 서버/클라 동일 — 예측 안전하고 별도 캐시가 필요 없다. (`.GetAbility()` 사용은 `WxAbilityBase.cpp`의 기존 쿨다운 쿼리와 동일 관례.)
- **정적 CDO로 순정 경로 복귀**: CDO에 MP/UP 모디파이어가 완전히 정의되므로 엔진 순정 `CheckCost`(`CanApplyAttributeModifiers`가 CDO 스펙 생성→`CalculateModifierMagnitudes`에서 MMC 실행)·`ApplyCost`·`GetCostGameplayEffect`가 그대로 동작한다. 어빌리티의 코스트 오버라이드 2개·mutable 멤버·Modifiers 재구성 코드가 모두 사라졌다.
- **`ModifierOp = Additive`**: 순정 `CanApplyAttributeModifiers`가 `Additive` 모디파이어만 자원 부족을 판정하므로 명시해 일치를 보장. 기존 코드는 `AddBase`였는데, 인스턴스 방식에선 `GetCostGameplayEffect`가 반환한 그 인스턴스를 검사해 통과했으나, 순정 판정 대상과의 일치성 측면에서 `Additive`가 명확하다(형제 `WxEffect_DrainDP`와 동일).
- **MP/UP 두 MMC 클래스**: MMC API가 평가 중 모디파이어 인덱스를 주지 않아 어트리뷰트별 분리가 불가피(선례 `WxMMC_LinearDrain` 동일 제약). 공통 Row 조회는 abstract 베이스 `UWxMMC_Cost::GetCostRow`로 한 곳에 뒀다.

### 계획 대비 달라진 점
- 계획에선 "`class UWxEffect_Cost;` 전방선언 유지"라 했으나, `CostEffect` 멤버 삭제 후 헤더에서 해당 타입 참조가 사라져 **미사용이 되어 함께 삭제**했다(생성자·`PostEditChangeProperty`의 `UWxEffect_Cost` 사용은 전부 `.cpp`이고 그쪽은 헤더를 직접 include). 그 외 계획대로.

### 후속 과제
- **런타임 검증(미완)**: 에디터 플레이에서 (1) MP/UP 코스트 스킬 발동 시 자원 정확 감소, (2) 자원 부족 시 `CheckCost` 차단으로 발동 실패 + HUD 발동가능 표시 갱신, (3) 무코스트 어빌리티 정상 발동. 컴파일은 WxEditor(Development) 빌드 성공(Result: Succeeded)으로 확인 완료.
- **문서 정합성(선택)**: `Docs/Programmer/Ability_Activation_Flow.md`의 코스트 경로 서술(인스턴스 Def 스펙/ApplyCost 우회 등)이 이번 변경으로 구식이 됐다. 범위 밖이라 미수정 — 필요 시 별도 갱신.
