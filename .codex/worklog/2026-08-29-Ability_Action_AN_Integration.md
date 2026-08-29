# Ability Action AN 통합

## 계획

- `UWxAnimNotify_ExecuteAbilityAction`이 `Event.AbilityAction.*` GameplayEvent만 발행하도록 구현하고, 포션·처형 대미지의 전용 AnimNotify를 제거한다.
- `UWxAbilityAction_UseItem`(WxGame)과 `UWxAbilityAction_ApplyFinisherDamage`(WxCombat)를 추가해 기존 어빌리티가 보유한 사용 가능 검사·대상 처리·대미지 적용 경로를 재사용한다.
- `GA_UseItem`, `GA_Finisher`, `GA_Backstab`의 인라인 Action 및 `AM_UseItem`, `AM_Finisher`, `AM_BackstabFinisher`의 AnimNotify를 새 규칙으로 마이그레이션한다.
- WxEditor(Development) 빌드와 에셋 참조 검증 후 작업 기록을 완료한다.

## 완료

- `UWxAnimNotify_ExecuteAbilityAction`을 추가해 `Event.AbilityAction.*` GameplayEvent만 발행하도록 통합했다.
- `UWxAbilityAction_UseItem`과 `UWxAbilityAction_ApplyFinisherDamage`를 추가하고, 기존 아이템 사용·앞잡·뒤잡의 효과 적용 경로를 재사용했다.
- `GA_UseItem`, `GA_Finisher`, `GA_Backstab`의 인라인 Action 및 `AM_UseItem`, `AM_Finisher`, `AM_BackstabFinisher`의 전용 AnimNotify를 새 구조로 마이그레이션하고 기존 전용 AN과 `Event.UseItem` 태그를 제거했다.
- `WxEditor Win64 Development` 빌드에 성공했고, 마이그레이션 에셋에 새 Action·Notify·이벤트 태그가 존재하며 이전 전용 참조가 없음을 확인했다.
