# 쿨다운 GE 통합

- 요청: BP 커스텀·오버라이드로 AI가 구조를 헷갈리지 않게 한다. `Skill.1`·`Ultimate.1` 같은 번호 태그가 없어지길 원함.
- 사용자 결정: `Ability.Pattern.N`은 BT 재정비에 필요할 수 있어 유지. `Ability.Skill.N` 제거는 BT 재정비 때. 스킬·쿨다운 파생 클래스 최소화. "쿨다운 태그는 따로 두더라도, 쿨다운 클래스는 통합" 진행 승인. 필드는 `FGameplayTagContainer CooldownTags` 유지(단일 `FGameplayTag CooldownGroup`안 검토 후 "그대로 계속 진행"). `SharesCooldownGroup`은 사용자 지적으로 추가하지 않음.
- 구현: 쿨다운 그룹별 `UWxEffect_Cooldown` 파생 클래스를 지우고 공용 `UWxEffect_Cooldown` 하나를 `UWxAbilityBase`의 `CooldownGameplayEffectClass` 기본값으로 둠. 어빌리티 `CooldownTags`를 `ApplyCooldown`이 스펙 `DynamicGrantedTags`에 붙임. 쌓지 않고 충전 하나당 GE 하나, 지속시간 MMC가 같은 태그의 최대 남은 시간 + `CooldownTime`으로 차례 회복을 유지. 슬롯 VM은 `DynamicGrantedTags`로 세고 주기는 `IWxUIData::GetCooldownTime()`. `Cooldown.Skill.4` 태그 제거.
- 순정 규칙 누락과 보정: 설계 때 엔진의 쿨다운 GE 태그 규칙(`AbilitySystem.WarnCooldownEffectWithoutTags`: GE가 태그를 부여하지 않으면 GA_ 검증 오류, 빈 쿨다운 태그면 발동 검사 경고)을 놓쳐 첫 검증에서 GA_ 7개가 오류. CVar를 끄지 않고 공용 GE가 `Cooldown` 부모 태그를 부여하고, 빈 `CooldownTags`면 `CheckCooldown`이 먼저 통과시키도록 보정.
- 에셋: 헤드리스 Python으로 GA_ 9개 이관(`CooldownTags` 채움, `CooldownGameplayEffectClass` 저장값 제거). 쿨다운 0인 HGTest Skill_2·Minion Skill_1은 태그 없음.
- 검증: WxEditor Development 빌드 성공. DataValidation 커맨드릿 GameplayAbilityBlueprint 40개·WxAbilitySet 9개 오류 0(경고 1건은 MCP EULA 안내). 어빌리티 목록 생성기에 `CooldownTags` 칸 반영.
- 미확인: 인게임 회피 2충전 차례 회복, 쿨다운 UI 표시, 소환물 쿨다운 무시, 네트워크 복제. 사용자 에디터의 DebugGame 바이너리는 다시 빌드해야 함. 사람 리뷰·제출 전.
- Wiki: raw/notes/2026-09-25-cooldown-single-ge.md와 combat-abilities 기사 반영.
