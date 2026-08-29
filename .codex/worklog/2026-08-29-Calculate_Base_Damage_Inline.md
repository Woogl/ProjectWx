# CalculateBaseDamage 인라인화

## 계획

- `CalculateBaseDamage`의 방어력 보정 상수와 피해 산식, 음수 피해 클램프를 유일한 호출부에 같은 순서로 인라인화한다.
- 사용처가 없는 정적 헬퍼만 제거하고, 대미지 계산·크리티컬·가드·출력 처리의 동작은 바꾸지 않는다.
- 변경 범위를 확인한 뒤 UE 5.8 `WxEditor(Development)` 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `CalculateBaseDamage`를 제거하고, ATK·DEF 캡처 직후에 기존 방어력 보정식과 음수 피해 클램프를 인라인화했다.
- 기존 대미지 계산, 크리티컬, 가드 및 출력 처리 순서는 유지했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
