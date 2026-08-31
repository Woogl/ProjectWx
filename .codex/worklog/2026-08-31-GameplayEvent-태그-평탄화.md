# GameplayEvent 태그 평탄화

## 계획

- `Event.AbilityAction.SummonActor`, `Event.AbilityAction.UseItem`, `Event.AbilityAction.ApplyFinisherDamage`, `Event.AbilityAction.SpawnProjectile`를 각각 `Event.SummonActor`, `Event.UseItem`, `Event.ApplyFinisherDamage`, `Event.SpawnProjectile`로 변경한다.
- 네이티브 GameplayTag 식별자와 모든 C++ 송신·수신 참조를 새 태그명에 맞춘다.
- `Event.AbilityAction` 루트 태그를 제거하고, 공용 Ability Action AnimNotify는 허용된 평탄화 태그만 발행하도록 유지한다.
- 사용자 요청에 따라 에셋은 수정하지 않는다.
- WxEditor(Development) 타겟을 빌드해 컴파일을 검증한다.

## 완료

- `Event.AbilityAction` 루트 네이티브 태그를 제거하고 `Event.SummonActor`, `Event.UseItem`, `Event.ApplyFinisherDamage`, `Event.SpawnProjectile` 네이티브 태그로 평탄화했다.
- 소환·아이템 사용·처형 피해·투사체의 모든 C++ 송신·수신 참조와 관련 소스 주석을 새 태그명으로 변경했다.
- 공용 `UWxAnimNotify_ExecuteAbilityAction`은 새 Action 이벤트 4개만 허용하도록 명시적으로 검증한다.
- 사용자 요청에 따라 몽타주 등 에셋은 수정하지 않았다.
- 소스 검색에서 `Event.AbilityAction` 및 `Event_AbilityAction` C++ 참조가 남지 않았고 `git diff --check`를 통과했다.
- UE 5.8에서 `WxCore`, `WxCombat` 모듈 빌드와 `WxAbility_UseItem.cpp` 단일 파일 컴파일이 성공했다.
- 전체 `WxEditor Win64 Development` 빌드는 이번 변경과 무관한 `WxSave`의 기존 컴파일 오류(`UWxSaveGameSubsystem::TryGetPlayerTransform`, `ApplySavedPlayerStats` 선언 누락)에서 중단됐다.
