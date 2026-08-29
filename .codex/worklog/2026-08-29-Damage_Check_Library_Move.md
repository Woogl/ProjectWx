# 대미지 선판정 라이브러리 이관

## 계획

- `EWxDamageCheck`과 `CheckDamage`를 `UWxExecCalc_Damage`에서 `UWxCombatLibrary`로 옮긴다.
- 사망·팀·무적의 판정 조건과 `ApplyDamage`, 투사체의 기존 호출 흐름을 보존한다.
- 투사체의 불필요한 Damage Effect 헤더 의존을 제거한다.
- UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- `EWxDamageCheck`과 사망·팀·무적 선판정 구현을 `UWxCombatLibrary`로 이관했다.
- `ApplyDamage`와 투사체가 `UWxCombatLibrary::CheckDamage`를 사용하도록 갱신하고, 투사체의 Damage Effect 헤더 의존을 제거했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
