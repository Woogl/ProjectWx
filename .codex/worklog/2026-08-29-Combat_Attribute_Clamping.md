# 전투 어트리뷰트 클램핑 중앙화

## 계획

- `UWxCombatAttributeSet`에 어트리뷰트별 최소값과 대응 Max 관계를 한 곳에 정의한다.
- HP/SP/GP/MP/UP는 대응 Max를 포함해 클램프하고, Max가 0이면 현재값도 0으로 제한한다.
- ASPD의 최소값을 0.001로 정하고, CritRate에는 상한을 두지 않는다.
- `PreAttributeChange`, `PreAttributeBaseChange`, 피해 처리의 중복 클램핑을 공통 규칙으로 통합한다.
- Max 변경 비율 보정에 UP를 포함하고, WxEditor Development 타겟 빌드로 검증한다.
- 현재값–Max 관계만 담은 표로 규칙 표현을 단순화하고, 함수 포인터 기반 구현은 제거한다.
- Max 변경 보정은 `SetNumericAttributeBase`를 직접 호출해 `SetCurrentAttributeValue` 중계 함수를 제거한다.
- ASPD 최소값 예외는 `ClampAttributeValue`에 직접 두어 별도 최소값 조회 함수를 제거한다.

## 완료

- `FWxAttributeClampRule` 표에 모든 전투·메타 어트리뷰트의 최소값과 현재값-최대값 관계를 모았다.
- HP/SP/GP/MP/UP는 Max가 0이어도 현재값을 0으로 제한하고, Max 변경 시 동일 표를 통해 비율 보정 후 다시 클램프한다.
- 누락되어 있던 UP의 Max 변경 비율 보정을 추가했으며, ASPD 최소값은 0.001로 적용했다.
- CritRate는 최소값 0만 적용하고 상한은 두지 않았다.
- 두 사전 변경 훅과 피해 처리의 중복 클램프를 공통 규칙으로 통합했다.
- `WxEditor Win64 Development` 빌드 성공을 확인했다.
- 클램핑 규칙 표와 조회 함수는 익명 네임스페이스 대신 `UWxCombatAttributeSet`의 private 멤버로 두었다.
- 함수 포인터 기반 규칙 표를 현재값–Max 쌍 5개로 축소해 읽기 쉽게 정리했고, 재빌드 성공을 확인했다.
