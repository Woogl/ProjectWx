# 락온 대상 서버 검증

## 계획

- `ServerSetLockOnTarget`에서 `nullptr` 해제와 `CanBeLockedOn()`을 만족하는 락온 지점만 권위 상태에 반영한다.
- 서버 타기팅 프리셋 재실행, 활성 상태 보관, 거절 동기화 RPC는 제거한다.
- 기존 클라이언트 후보 선정과 복제 동작은 유지한다.
- `WxEditor` Development 빌드로 컴파일을 확인한다.

## 완료

- `nullptr` 해제는 허용하고, 대상 설정은 `UWxLockOnPointComponent`와 `CanBeLockedOn()` 조건을 만족할 때만 서버 권위 상태에 반영한다.
- 적대 관계 검증과 `WxCombatLibrary` 의존성을 제거했다.
- `WxEditor Win64 Development` 빌드 성공을 확인했다.
