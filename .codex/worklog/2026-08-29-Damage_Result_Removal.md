# 대미지 결과 구조체 제거

## 계획

- `UWxExecCalc_Damage::Execute_Implementation` 안에서만 쓰이는 `FWxDamageResult`를 제거한다.
- `FinalDamage`, `bIsCritical` 지역 변수로 치환해 크리티컬·가드·컨텍스트·SP·GP·메타 어트리뷰트 처리 순서를 보존한다.
- UE 5.8 `WxEditor(Development)` 빌드로 컴파일을 검증한다.

## 완료

- `FWxDamageResult`를 제거하고 `FinalDamage`, `bIsCritical` 지역 변수로 치환했다.
- 크리티컬·가드·퍼펙트 가드·컨텍스트·SP·GP·메타 어트리뷰트의 처리 순서와 피해 공식을 보존했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
