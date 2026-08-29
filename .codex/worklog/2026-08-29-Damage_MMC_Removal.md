# 대미지 기본 피해 MMC 제거

## 계획

- `UWxMMC_DamageBase`를 제거하고 기본 피해식을 `WxEffect_Damage.cpp`의 순수 C++ 계산 함수로 전환한다.
- `DefenseConstant`은 같은 계산 함수의 명명된 상수로 유지한다.
- `UWxExecCalc_Damage`가 ATK·DEF를 캡처한 뒤 기본 피해식, 크리티컬, 가드·퍼펙트 가드, SP·GP·메타 어트리뷰트 출력을 기존 순서로 처리하게 한다.
- UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- `UWxMMC_DamageBase`와 수동 CDO 호출을 제거했다.
- `WxEffect_Damage.cpp` 내부의 `CalculateBaseDamage`가 ATK, DEF, 공격 계수로 기본 피해량을 계산하며 `DefenseConstant`(기본값 100)을 유지한다.
- ExecCalc가 ATK·DEF 캡처 뒤 기본 피해식, 크리티컬, 가드·퍼펙트 가드, SP·GP·메타 어트리뷰트 출력을 기존 순서로 처리하도록 유지했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
