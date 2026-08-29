# 대미지 캡처 정적 데이터 분리

## 계획

- ATK·DEF 캡처를 `FWxDamageBaseStatics`로 옮겨 기본 피해 MMC가 소유하게 한다.
- 크리티컬, SP, GP, `IncomingDamage`, `IncomingReflect`를 `FWxDamageExecutionStatics`로 분리해 ExecCalc가 소유하게 한다.
- ExecCalc가 MMC의 ATK·DEF 캡처를 스펙에 등록하는 현재 동작과, 피해 공식·출력 순서를 보존한다.
- UE 5.8 `WxEditor(Development)` 빌드로 검증한다.

## 완료

- ATK·DEF 캡처를 `FWxDamageBaseStatics`로 분리해 기본 피해 MMC가 사용하도록 했다.
- 크리티컬, SP, GP, `IncomingDamage`, `IncomingReflect`를 `FWxDamageExecutionStatics`로 분리해 ExecCalc가 사용하도록 했다.
- ExecCalc가 MMC에 필요한 ATK·DEF 캡처를 스펙에 등록하는 기존 동작과 피해 공식·출력 순서를 유지했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다.
