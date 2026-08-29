# 퍼펙트 가드 헬퍼 제거

## 계획

- 사용처가 둘뿐인 `UWxExecCalc_Damage::IsPerfectGuardApplied` 선언·정의를 제거한다.
- `UWxCombatLibrary::ApplyDamage`와 `UWxExecCalc_Damage::Execute_Implementation`에 퍼펙트 가드 조건을 직접 작성한다.
- 가드 가능 공격과 `Effect.PerfectGuard` 태그의 기존 판정 의미를 보존하고, UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- `UWxExecCalc_Damage::IsPerfectGuardApplied` 선언·정의를 제거했다.
- `UWxCombatLibrary::ApplyDamage`와 `UWxExecCalc_Damage::Execute_Implementation`에 가드 가능 여부와 `Effect.PerfectGuard` 태그를 직접 검사하는 기존 조건을 배치했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
