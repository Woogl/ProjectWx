# 가드 피해 배율 계산 통합

## 계획

- 일반 가드 여부에 따른 `CalculateGuardMultiplier`를 추가하고, `CalculateFinalDamage`가 방어 보정·기본 피해·치명타·가드 배율을 조합하게 한다.
- 퍼펙트 가드는 반사 피해에 일반 가드 감쇄가 적용되지 않도록 일반 가드 조건에서 제외한다.
- 기존 일반 가드의 SP 차감과 가드 브레이크 처리 순서를 보존하고 UE 5.8 `WxEditor(Development)` 타겟을 빌드해 검증한다.

## 완료

- `CalculateGuardMultiplier`를 추가하고 `CalculateFinalDamage`가 방어 보정·기본 피해·치명타·가드 배율을 조합하게 했다.
- 퍼펙트 가드는 일반 가드 판정에서 제외해 반사 피해에 가드 감쇄가 적용되지 않도록 기존 동작을 보존했다.
- 기존 일반 가드의 SP 차감과 가드 브레이크 처리 순서를 유지했고 UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
