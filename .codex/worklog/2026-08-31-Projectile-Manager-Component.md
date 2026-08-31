# Projectile Manager Component

## 계획

- `UWxProjectileManagerComponent`를 `WxCombat`에 추가하고 `AWxCharacterBase`에 기본 컴포넌트로 부착한다.
- 컴포넌트가 `Event.AbilityAction.SpawnProjectile` GameplayEvent를 구독하고, 서버 권위에서 기존과 동일한 위치·회전·Owner·Instigator 설정으로 투사체를 스폰하도록 한다.
- `UWxAnimNotify_SpawnProjectile`를 데이터 전송 전용 Notify로 추가하고 `ProjectileClass`, `SpawnSocketName`을 명시적 이벤트 페이로드로 전달한다.
- `UWxAbilityBase`에서 투사체 설정·이벤트 태스크·콜백·`SpawnProjectile()` 구현을 제거하고 소환 처리 경로는 유지한다.
- 기존 투사체 몽타주 4종의 범용 Notify를 새 Notify로 교체하고 `GA_Pattern_4`, `GA_Skill_3`의 기존 값을 몽타주 Notify로 이전한다.
- UE 5.8 `WxEditor Win64 Development` 빌드와 에셋 참조 검증을 수행한다.

## 완료

- `UWxProjectileManagerComponent`를 추가해 `Event.AbilityAction.SpawnProjectile`를 구독하고 서버 권위에서 기존과 동일한 위치·회전·Owner·Instigator·AlwaysSpawn 설정으로 투사체를 생성하도록 이관했다.
- `UWxAnimNotify_SpawnProjectile`가 자신과 발화 메시를 GameplayEvent의 `OptionalObject`·`OptionalObject2` 페이로드로 전달하도록 구현했다.
- `AWxCharacterBase`에 컴포넌트를 기본 부착하고, `UWxAbilityBase`의 투사체 설정·이벤트 태스크·콜백·스폰 구현을 제거했다.
- `AM_Pattern_4`, `AM_Skill_3_1`, `AM_Skill_3_2`, `AM_Skill_3_3`을 전용 Notify로 마이그레이션하고 `BP_Projectile`·`hand_r` 설정을 옮겼으며, `GA_Pattern_4`와 `GA_Skill_3`의 투사체 설정을 제거했다.
- 네 몽타주가 기존과 동일한 트랙 1·발화 시각 0.3128534초를 유지하는지 확인했고, 두 GA 블루프린트를 warnings-as-errors로 컴파일했다.
- PIE에서 플레이어와 적 모두 `ProjectileManagerComponent`를 생성하는지 확인했다.
- UE 5.8 `WxEditor Win64 Development` 빌드에 성공했다. 최종 로그는 `.codex/skills/build-doctor/logs/build_2026-08-31_205409.log`에 저장했다.
