# 사망 이벤트 가해자 전달

## 계획

- `UWxCombatAttributeSet`의 `Event_Death` 페이로드에서 `Instigator`가 피해자 자신으로 설정되는 경로를 수정한다.
- Gameplay Effect Context에 기록된 실제 가해자를 `Instigator`로 전달하고, `Target`은 사망한 소유 액터로 유지한다.
- 사망 판정, 어빌리티 발동 및 피격 처리의 순서는 변경하지 않는다.
- `WxEditor` Development 빌드로 컴파일을 확인한다.

## 완료

- `Event_Death`의 `Instigator`를 `Data.EffectSpec`의 Gameplay Effect Context에 기록된 실제 가해자로 교체했다.
- `Target`은 사망한 어트리뷰트 소유 액터로 명시해, 가해자와 피해자 의미를 분리했다.
- 사망 판정과 사망 어빌리티 발동 순서는 변경하지 않았다.
- `WxEditor Win64 Development` 빌드에 성공했다.
