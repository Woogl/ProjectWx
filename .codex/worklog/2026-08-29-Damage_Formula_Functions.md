# 대미지 산식 함수 분리

## 계획

- 방어 보정, 기본 피해, 치명타 배율을 각각 `CalculateDefenseMultiplier`, `CalculateBaseDamage`, `CalculateCriticalMultiplier`로 분리한다.
- `CalculateFinalDamage`는 세 계산 결과를 조합하고, 기존의 방어 보정 뒤 0 클램프와 치명타 배율 적용 순서를 유지한다.
- UE 5.8 `WxEditor(Development)` 타겟을 빌드해 컴파일을 검증한다.

## 완료

- 방어 보정, 기본 피해, 치명타 배율을 각각 `CalculateDefenseMultiplier`, `CalculateBaseDamage`, `CalculateCriticalMultiplier`로 분리했다.
- `CalculateFinalDamage`가 세 계산 결과를 조합하게 했고, 기존의 방어 보정 뒤 0 클램프와 치명타 배율 적용 순서를 유지했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
