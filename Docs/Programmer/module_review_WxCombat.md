# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 런타임으로, 어빌리티·어트리뷰트·대미지 파이프라인·락온·무기/투사체·히트스톱을 폭넓게 다룬다. 전반적으로 네트워크 권위 모델과 몽타주/태스크 수명주기를 세심하게 처리한 건강한 코드베이스이며, 코딩·모듈 규칙 위반은 발견되지 않았다. 대미지 계산(WxExecCalc_Damage), 어트리뷰트(PostGameplayEffectExecute), ASC 입력 라우팅, 어빌리티(Attack/Dodge/HitReact/Death), 무기/투사체 히트 경로, 락온·시간감속을 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 투사체 히트 처리가 권위 게이트 없이 클라에서도 Destroy를 호출
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:137`, `:147`
- **범주**: 설계/구조 (리플리케이션 권위 모델)
- **문제**: 투사체는 `bReplicates = true`로 서버가 스폰하고(`WxAnimNotify_SpawnProjectile` → `WxAbilityBase::SpawnProjectile`는 `HasAuthority()` 게이트로 서버 전용) 클라엔 시뮬프록시로 복제된다. 그런데 `HandleHitCollisionOverlap`/`HandleHitCollisionHit`에는 권위 체크가 없어 클라이언트의 로컬 오버랩/히트 타이밍에서도 그대로 실행되고 마지막에 무조건 `Destroy()`를 호출한다. 시뮬프록시에서 복제 액터를 클라가 로컬로 파괴하면 서버 권위와 무관하게 투사체가 조기·불일치 소멸하고, `Destroyed()`가 클라 로컬 임팩트 위치에 `ImpactFX`를 스폰해 서버 권위 소멸 지점과 어긋난다. 같은 대미지 트리거인 `WxAnimNotify_AreaAttack`은 `Owner->HasAuthority()`로 게이트하고 있어 처리 방식이 일관되지 않다. (대미지 GE 자체는 비권위·프리딕션키 부재 시 GAS가 instant 실행을 건너뛰어 무해할 가능성이 높으나, `Destroy()`는 조건 없이 실행된다.)
- **제안**: 히트 처리를 `HasAuthority()`로 게이트해 서버에서만 대미지 적용·`Destroy()`를 수행하고, 클라 소멸은 복제로 전파되게 한다. 클라 예측 소멸이 목적이라면 그 의도를 명시적으로 설계·주석화한다.
- **확신도**: 중간

### 2. 🟡 SetLastPressedInputAction의 GetOwnerActor() 널 미검사 역참조
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:119`
- **범주**: 버그/정확성 (널 역참조)
- **문제**: `if (!GetOwnerActor()->HasAuthority())`에서 `GetOwnerActor()` 반환을 널 검사 없이 역참조한다. 이 모듈의 다른 권위 판정 지점(`WxAbility_Death::EnableRagdoll`의 `if (!Avatar || !Avatar->HasAuthority())` 등)은 모두 액터를 먼저 가드하는데, 여기만 무방비다. ActorInfo 초기화 이전이나 소유 액터 해제 시점에 호출되면 크래시로 이어진다.
- **제안**: `const AActor* Owner = GetOwnerActor(); if (Owner && !Owner->HasAuthority()) { ServerSetLastPressedInputAction(Action); }` 형태로 가드한다.
- **확신도**: 낮음 (입력이 도착하는 시점엔 소유 액터가 유효한 것이 일반적이라 실제 발생 빈도는 낮을 수 있음)

### 3. 🟢 무기 히트 시 "클라 예측" 주석과 실제 프리딕션 부재
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:270`
- **범주**: 중복/복잡도 (오해 소지 있는 주석·불변식)
- **문제**: `ProcessHit`의 주석은 "클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며 … 롤백된다"고 설명하지만, `ProcessHit`는 `Tick`(스윕)과 오버랩 콜백에서 호출되어 어빌리티의 `ActivateAbility` 스코프 밖이다. 즉 활성 ScopedPredictionKey가 없어 클라의 `ApplyDamage`는 실제로 예측되지 않으며, instant GE는 비권위·키 부재 시 실행되지 않아 사실상 무동작에 가깝다(결과적으로 서버 권위만으로 정상 동작). 기능상 문제는 아니나 주석이 실제 동작과 어긋나 유지보수 시 오해를 부른다.
- **제안**: 주석을 실제 경로(예측 키가 없어 클라 적용은 무동작이며 서버 권위로 확정)에 맞게 정정하거나, 예측이 필요하면 어빌리티 스코프 내에서 대미지를 적용하도록 경로를 재설계한다.
- **확신도**: 낮음 (GAS 내부 동작 추정에 의존)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Private/WxCombatLibrary.cpp`, `Private/WxDamageInfo.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Private/Targeting/WxLockOnManagerComponent.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/WxEffectZone.cpp`, `Private/AbilitySystem/WxAbilitySet.cpp`, `Private/AnimNotify/WxAnimNotify_AreaAttack.cpp`, `Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp`
- **훑은 파일**: `WxCombat.Build.cs`, `README.md`, Public 헤더 전반, 나머지 Effect/MMC/ExecCalc·Cue·Task·TargetingFilter·AnimNotify 계열 cpp(패턴 반복 확인 수준)
- **미검토 / 한계**: 데이터 주도 수치(DataTable Row·`UWxAbilitySet` 에셋 디폴트값)와 BP 파생 어빌리티/이펙트의 실제 설정값은 코드만으로 검증 불가. 리플리케이션 관련 지적은 GAS 내부 동작(비권위 instant GE 실행 여부, 시뮬프록시 오버랩 발생 여부)에 대한 정적 추론에 기반하며 실런타임·네트워크 프로파일(리슨 서버 vs 데디케이티드) 검증은 수행하지 않았다. Skill/Ultimate/Guard/Finisher/Groggy/Pattern/LockOn/Sprint 등 나머지 어빌리티 cpp는 통독하지 않았다.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 149파일 — `/module-review`로 갱신*
