# 퍼펙트 가드 부가효과 차단

## 계획

- `UWxCombatLibrary::ApplyDamage`의 피해 스펙 적용 경로에서 퍼펙트 가드 판정을 확인한다.
- 퍼펙트 가드 성공 시 `UWxEffect_Damage`만 적용하고, `FWxDamageTableRow::AdditionalEffects`로 생성된 스펙은 적용하지 않는다.
- 일반 피해·일반 가드·회피 경로는 유지한다.
- `WxEditor` Development 빌드로 컴파일을 확인한다.

## 완료

- `UWxCombatLibrary::ApplyDamage`에서 `Damage_CanGuard` 공격이 `Effect.PerfectGuard` 상태에 적중하면 Damage GE만 적용하도록 변경했다.
- `FWxDamageTableRow::AdditionalEffects`로 만든 상태이상·디버프 스펙은 퍼펙트 가드 성공 시 적용하지 않는다.
- 일반 피해와 일반 가드, 회피 경로의 스펙 적용은 변경하지 않았다.
- `WxEditor Win64 Development` 빌드 성공을 확인했다.
