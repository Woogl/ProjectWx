# GameplayAttribute base LSP 복원

## 계획

- LSP가 직접 복원한 `FGameplayAttributeData`에서 모든 base 값을 먼저 스냅샷하고, Max 계열을 우선한 뒤 나머지 스탯을 `UAbilitySystemComponent::SetNumericAttributeBase`로 복원한다.
- 저장된 base 복원이 끝난 뒤 기존 `Effect.Savable` GameplayEffect 복원 경로가 실행되는 순서를 유지해 modifier를 정확히 한 번 반영한다.
- 엔진이 새로 생성하는 플레이어 폰은 `AWxWorldSettings::PlayerPersistenceState`를 운반책으로 유지하고, 기존 `GetNumericAttributeBase`·`SetNumericAttributeBase` 경로를 사용한다.
- 구 `PlayerStats`, `bHasPlayerStats`, `ApplyLegacyPlayerStats` 호환 경로를 제거하고 저장 포맷을 5로 올린다. 구 슬롯 변환은 구현하지 않는다.
- WxSave 문서를 갱신하고, 관련 정적 검사와 UE 5.8 `WxEditor Development Win64` 빌드로 검증한다.

## 완료

- `UWxCombatAttributeSet::OnSaveRestored`가 LSP로 기록된 16개 `FGameplayAttributeData`의 base 값을 먼저 모두 스냅샷하도록 변경했다.
- MaxHP·MaxSP·MaxGP·MaxMP·MaxUP를 먼저 `SetNumericAttributeBase`로 적용하고, 저장된 HP·SP·GP·MP·UP와 전투 스탯 base를 뒤이어 적용한다. 이후 기존 `Effect.Savable` 복원이 modifier를 한 번만 더한다.
- 플레이어 상태 운반은 `AWxWorldSettings::PlayerPersistenceState`에 유지했으며, 기존 `GetNumericAttributeBase` 캡처와 `SetNumericAttributeBase` 복원 경로를 확인했다.
- `UWxSaveGame`의 `bHasPlayerStats`·`PlayerStats`와 `AWxWorldSettings::ApplyLegacyPlayerStats`를 제거했다.
- 저장 포맷을 5로 올리고 README에 base·GE 복원 경계를 반영했다. 이전 포맷은 변환하지 않는다.
- LSP 설정에 영속 스탯 16개가 한 번씩 등록되고 `IncomingDamage`·`IncomingReflect`가 제외된 것을 확인했다. 구 호환 심볼 검색과 `git diff --check`도 통과했다.
- UE 5.8 `WxEditor Development Win64` 전체 빌드 성공: 15/15 액션, `Result: Succeeded`.
- `BP_Boss`, `BP_Enemy`, `BP_Player`, `BP_Sandbag` 허용 목록 컴파일 성공: 0 오류, 0 컴파일 경고. 기존 누락 모듈·컴포넌트에 대한 로드 경고(`WxSound`, `WxBGMSourceComponent`, `WxRewardComponent`)는 별도 이슈로 남아 있다.
