# Ability Action 소환수 스폰

## 계획

- `UWxAbilityBase`가 BP에서 인라인으로 저작하는 `UWxAbilityAction` 목록을 활성화 시 시작하도록 추가한다.
- 모든 Action이 `TriggerEventTag`를 `UAbilityTask_WaitGameplayEvent`로 수신해 실행하도록 하고, 첫 기본값으로 `Event.AbilityAction.SummonActor`를 등록한다.
- 첫 Action으로 엔진 기본 `UAbilityTask_SpawnActor`로 전투 소환수를 서버에서 생성하는 `UWxAbilityAction_SummonActor`를 구현한다. 생성한 Pawn은 소환 슬롯으로 관리한다.
- 플레이어의 `UWxSummonComponent`가 슬롯별 소환수의 교체·사망·소유자 종료 정리를 담당하도록 하고, 소환수는 소환자의 Owner·Instigator·팀을 이어받는다.
- WxEditor(Development)를 빌드해 컴파일을 검증한다.

## 완료

- `UWxAbilityBase`에 인라인 `AbilityActions` 목록을 추가하고, 각 Action이 `TriggerEventTag`를 `UAbilityTask_WaitGameplayEvent`로 수신하도록 공통화했다.
- `UWxAbilityAction_SummonActor`는 기본 `Event.AbilityAction.SummonActor`를 수신해 엔진 기본 `UAbilityTask_SpawnActor`로 전투 소환수를 서버에서 생성한다. 생성한 Pawn은 플레이어의 `UWxSummonComponent`가 교체·사망·소유자 종료까지 관리한다.
- 전용 Notify 대신 기존 `UWxAnimNotify_SendGameplayEvent`를 사용한다. Action은 기본적으로 활성화마다 첫 이벤트 한 번만 실행하며, BP에서 반복 수신으로 변경할 수 있다.
- `WxEditor Win64 Development` 빌드 성공을 확인했다.
