# Projectile Ability Action

## 계획

- `UWxAbilityAction_SpawnProjectile`와 `Event.AbilityAction.SpawnProjectile`를 추가해 기존 `UWxAbilityBase::SpawnProjectile()`의 서버 권위 스폰 경로를 재사용한다.
- `AM_Pattern_4`, `AM_Skill_3_1`, `AM_Skill_3_2`, `AM_Skill_3_3`의 `UWxAnimNotify_SpawnProjectile`를 `UWxAnimNotify_ExecuteAbilityAction`으로 마이그레이션하고 인라인 Action 설정을 각 어빌리티에 옮긴다.
- 이전 전용 AnimNotify를 제거하고 WxEditor(Development) 빌드 및 에셋 참조 검증을 수행한다.

## 완료

- `UWxAbilityAction_SpawnProjectile`와 `Event.AbilityAction.SpawnProjectile`를 추가하고, 기존 `UWxAbilityBase::SpawnProjectile()`의 서버 권위 스폰 경로를 재사용했다.
- `GA_Pattern_4`, `GA_Skill_3`에 인라인 Action을 설정하고 `AM_Pattern_4`, `AM_Skill_3_1`, `AM_Skill_3_2`, `AM_Skill_3_3`의 전용 Projectile AnimNotify를 `UWxAnimNotify_ExecuteAbilityAction`으로 마이그레이션했다.
- `UWxAnimNotify_SpawnProjectile`를 제거했다.
- `WxEditor Win64 Development` 빌드에 성공했고, 마이그레이션 에셋에 새 참조가 존재하며 이전 전용 AN 참조가 없음을 확인했다.
