# Ability Action 시스템 제거

## 계획

- `UWxAbilityBase`의 인라인 `AbilityActions` 배열과 공통 Action UObject 기반 실행 경로를 제거한다.
- 기존 몽타주가 보내는 `Event.AbilityAction.*` 이벤트는 유지하고, 피니셔·투사체·소환·아이템 사용 어빌리티가 각자 필요한 이벤트를 직접 대기해 기존 실행 시점과 동작을 보존한다.
- Action 클래스 및 더 이상 필요 없는 참조를 정리하고, WxEditor Development/DebugGame 빌드로 컴파일을 확인한다.

## 완료

- `AbilityActions` 인라인 UObject 배열과 공통·피니셔·투사체·소환·아이템 Action 클래스를 제거했다.
- 기존 GA 에셋 3개에서 투사체·소환 설정을 직접 프로퍼티로 자동 이전하고, 저장된 Action 참조를 비웠다.
- 피니셔와 아이템 사용은 각 어빌리티가 이벤트를 직접 수신하며, 투사체·소환은 공용 베이스의 직접 설정과 이벤트 처리로 실행한다.
- `WxEditor Win64 Development`, `WxEditor Win64 DebugGame` 빌드에 성공했다.
