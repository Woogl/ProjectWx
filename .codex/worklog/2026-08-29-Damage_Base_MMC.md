# 기본 피해량 MMC 분리

## 계획

- `UWxMMC_DamageBase`를 `UWxEffect_Damage`와 같은 파일에 추가해 ATK, DEF, `SetByCaller.CoeffATK`로 기본 피해량을 계산한다.
- 방어 보정식의 상수 100을 기본값 100의 `DefenseConstant`으로 명명해 이후 밸런스 조정 지점을 명확히 한다.
- `UWxExecCalc_Damage`는 MMC 결과에 크리티컬, 가드·퍼펙트 가드, SP·GP·메타 어트리뷰트 출력을 적용하도록 유지한다.
- 기존 피해 공식과 출력 순서는 바꾸지 않고, UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- `UWxMMC_DamageBase`를 추가해 ATK, DEF, `SetByCaller.CoeffATK` 기반 기본 피해량 계산을 이관했다.
- 방어 보정식의 기존 상수 100을 `UWxMMC_DamageBase::DefenseConstant`(기본값 100)으로 명명했다.
- `UWxExecCalc_Damage::CalcDamage`를 제거하고, ExecCalc에는 MMC 결과에 대한 크리티컬·가드·퍼펙트 가드·출력 처리만 남겼다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
