# AnimNotify 카테고리 색상 설정

## 계획

- `UWxCombatDeveloperSettings`에 전투·이동·연출·기타 AnimNotify 색상 설정을 추가하고 기본값을 각각 빨강·파랑·초록·노랑으로 지정한다.
- 각 `UWxAnimNotify*` 및 `UWxAnimNotifyState*` 클래스가 `GetEditorColor()`에서 담당 카테고리의 설정값을 직접 읽도록 구현한다.
- 전투에는 `AreaDamage`, `FinisherDamage`, `SpawnProjectile`, `SpawnMinion`, `DespawnMinion`, `StartRecovery`, `WeaponAttack`, `ComboWindow`, `ApplyGameplayEffect`를 분류한다.
- 이동에는 `Rush`, `SnapToTarget`을 분류한다.
- 연출에는 `CameraMove`, `SlowTime`을 분류한다.
- 기타에는 `SendGameplayEvent`를 분류한다.
- Notify 실행 로직과 애셋 데이터는 변경하지 않는다.
- UE 5.8의 `WxEditor Win64 Development` 빌드로 컴파일을 검증하고, 실패하면 build-doctor로 진단한다.

## 완료

- `UWxCombatDeveloperSettings`에 전투·이동·연출·기타 AnimNotify 색상 설정을 추가하고 기본값을 빨강·파랑·초록·노랑으로 지정했다.
- WxCombat의 단발 Notify 7개와 NotifyState 7개가 `GetEditorColor()`에서 담당 카테고리 설정을 직접 읽도록 연결했다.
- `AreaDamage`와 `SnapToTarget`의 에디터 타겟팅 프리뷰도 각각 전투·이동 설정 색상을 사용하도록 맞췄다.
- Notify 실행 로직과 애셋 데이터는 변경하지 않았다.
- Build Doctor로 `WxEditor Win64 Development` 빌드를 실행했으며 성공했다.
