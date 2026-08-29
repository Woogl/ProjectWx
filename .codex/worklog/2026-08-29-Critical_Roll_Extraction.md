# 치명타 판정 분리

## 계획

- `Execute_Implementation`에서 치명타 확률 계산과 `FMath::FRand()` 난수 판정으로 `bIsCritical`을 먼저 확정한다.
- `CalculateFinalDamage`는 확정된 치명타 여부와 치명타 피해율만 받아 최종 피해 배율을 적용하게 단순화한다.
- 기존 치명타 가능 조건, 스탯 캡처와 난수 호출 횟수를 유지하고 UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- `Execute_Implementation`에서 치명타 확률 계산과 `FMath::FRand()` 판정으로 `bIsCritical`을 먼저 확정했다.
- `CalculateFinalDamage`는 확정된 치명타 여부와 치명타 피해율만 받아 배율을 적용하도록 단순화했다.
- 기존 치명타 가능 조건, 스탯 캡처와 난수 호출 횟수를 유지했고 UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
