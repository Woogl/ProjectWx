# LSP AttributeSet 초기화 순서 수정

## 계획

- `UWxCombatAttributeSet::OnSaveRestored`에서는 LSP가 복원한 GameplayAttribute base 값과 복원 프로퍼티를 먼저 보관한다.
- ASC가 이미 해당 AttributeSet을 등록한 상태라면 즉시 적용하고, 런타임 액터 deferred spawn처럼 ASC 초기화 전이면 적용을 미룬다.
- `AWxCharacterBase::InitAbilitySystem`에서 `GiveAbilitySet`으로 기본값을 구성한 직후 대기 중인 저장 base 값을 Max 계열부터 ASC API로 적용한다.
- `Effect.Savable` GameplayEffect 복원이 base 복원 뒤 실행되는 기존 순서를 유지한다.
- 런타임 적 저장·복원 경로에서 AttributeSet 미등록 ensure가 재발하지 않고 base 값과 GameplayEffect modifier가 각각 한 번만 반영되는지 정적으로 점검하고, UE 5.8 `WxEditor Development Win64` 빌드로 검증한다.

## 완료

- `UWxCombatAttributeSet::OnSaveRestored`가 LSP로 복원된 프로퍼티의 base 값을 `PendingRestoredBaseValues`에 먼저 보관하도록 변경했다.
- ASC가 아직 `UWxCombatAttributeSet`을 등록하지 않은 deferred spawn 구간에는 `SetNumericAttributeBase`를 호출하지 않고 대기 상태를 유지한다.
- ASC 등록이 끝난 경로는 즉시 적용하고, 런타임 캐릭터는 `AWxCharacterBase::InitAbilitySystem`에서 `GiveAbilitySet` 직후 다시 적용한다. Max 계열, 현재 자원, 전투 스탯 순서를 유지한다.
- 저장 base 적용 뒤 기존 `Effect.Savable` GameplayEffect의 레벨 복원 완료 콜백이 실행되는 순서를 유지했다.
- `git diff --check`를 통과했고, 소스보다 늦게 갱신된 `UnrealEditor-WxCombat.dll`·`UnrealEditor-WxGame.dll`을 확인했다.
- UE 5.8 `WxEditor Development Win64` 빌드 성공: `C:\Wx\.Codex\skills\build-doctor\logs\build_2026-08-31_235043.log`, `Result: Succeeded`.
