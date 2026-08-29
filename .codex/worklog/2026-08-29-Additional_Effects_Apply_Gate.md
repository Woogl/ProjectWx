# AdditionalEffects 적용 게이트

## 계획

- `UWxCombatLibrary::ApplyDamage`에서 Damage GE를 먼저 적용하고, 성공한 경우에만 일반 경로의 `AdditionalEffects`를 적용한다.
- 퍼펙트 가드 판정을 `UWxExecCalc_Damage`의 공용 함수로 모아 적용 단계와 계산 단계가 같은 조건을 사용하게 한다.
- 퍼펙트 가드에서는 기존처럼 Damage GE만 적용하고 부가 효과는 적용하지 않음을 유지한다.
- WxEditor(Development) 타겟을 빌드해 컴파일을 확인한다.

## 완료

- Damage GE를 먼저 적용하고 성공한 일반 경로에서만 `AdditionalEffects`를 적용하도록 변경했다.
- 퍼펙트 가드 판정을 `UWxExecCalc_Damage::IsPerfectGuardApplied`로 모아 스펙 적용 단계와 계산 단계가 동일한 조건을 사용한다.
- 퍼펙트 가드에서는 Damage GE만 적용하고 부가 효과를 생략하는 기존 동작을 유지했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
