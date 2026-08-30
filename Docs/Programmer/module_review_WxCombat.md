# WxCombat — 코드 리뷰

> 전투의 핵심 경로에는 권위·예측 의도가 비교적 잘 드러나지만, 상태 효과와 락온처럼 소유권이 겹치는 지점에는 실제 멀티플레이 실패 경로가 남아 있다. README와 빌드 경계를 먼저 확인한 뒤 ASC·어빌리티·대미지·무기/투사체·락온·타임딜레이션·소환·AnimNotify/AbilityTask의 고위험 C++ 경로를 깊게 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 구간 종료가 동일 클래스의 상태 효과를 전부 해제할 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:30`
- **범주**: 버그/정확성
- **문제**: `RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1)`의 `1`은 일치하는 효과 인스턴스 수 제한이 아니라 각 인스턴스에서 제거할 스택 수이다. `nullptr`은 소스 필터도 해제하므로 같은 ASC에 같은 클래스의 Infinite GE가 여러 소유자에게서 걸려 있으면 모두 한 스택씩 제거된다. 회피 무적 ANS가 늦게 끝날 때 처형 무적까지 벗길 수 있으며, 컷신 태스크도 `WxAbilityTask_PlaySkillCutscene.cpp:27`에서 같은 방식으로 `UWxEffect_Invincible`을 해제한다.
- **제안**: 적용 주체를 구분하는 SourceObject/전용 GE 클래스를 컨텍스트에 싣고 그 소유자만 매칭해 제거한다. 서버에서는 적용 시 받은 `FActiveGameplayEffectHandle`을 보관해 정확한 인스턴스를 제거하고, 예측 효과는 예측 키 정합 뒤에도 소유 식별자가 유지되는 별도 경로로 처리한다.
- **확신도**: 높음

### 2. 🔴 서버 락온 RPC가 클라이언트가 고른 임의 타겟을 충분히 검증하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:48`
- **범주**: 설계/구조
- **문제**: 서버는 전달된 컴포넌트가 `UWxLockOnPointComponent`이고 현재 태그 요건을 만족하는지만 확인한다. 클라이언트 선택에 사용한 `TargetingPreset`의 거리·팀·시야 필터를 재검증하지 않으므로, 소유 클라이언트가 임의의 복제 컴포넌트를 보내면 정상 후보 밖의 대상도 서버 `LockOnTarget`이 된다. 이 값은 서버 투사체 호밍(`WxProjectileBase.cpp:58`)과 모션 워핑(`WxRootMotionModifier_SnapToTarget.cpp:31`)에 직접 소비된다. 반대로 서버가 요청을 거절해도 클라이언트가 32행에서 먼저 적용한 예측값을 명시적으로 되돌리지 않아, 서버 값이 원래 값에서 변하지 않은 경우 복제 정정도 발생하지 않는다.
- **제안**: 서버에서 대상 소유 액터에 대해 적대 관계·최대 거리·시야와 락온 지점 유효성을 다시 검사한다. 거절 시에는 권위 타겟과 revision을 명시적으로 반환하거나 강제 정정 RPC를 보내 로컬 예측을 즉시 롤백한다.
- **확신도**: 높음

### 3. 🟡 투사체가 `None` 판정을 유효 충돌처럼 연출하고 파괴한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:90`
- **범주**: 버그/정확성
- **문제**: `CheckDamage` 결과를 `Evaded`인지 여부로만 축약해 `None`도 `bEvaded == false`가 된다. 그 결과 적대 팀이지만 ASC가 없는 액터나 이미 사망한 대상은 대미지가 성립하지 않아도 임팩트 FX가 재생되고, 권위에서는 124행 분기로 투사체까지 파괴된다. 클래스 계약의 "판정이 성립하지 않는 Pawn은 통과"와 실제 동작이 어긋난다.
- **제안**: `EWxDamageCheck`를 그대로 분기해 `Damaged`일 때만 임팩트와 파괴를 수행하고, `Evaded`는 회피 이벤트만 발생시키며, `None`은 충돌을 무시한다. 실제 적용 실패까지 통과시킬지 여부는 `ApplyDamage` 반환값으로 별도 결정한다.
- **확신도**: 높음

### 4. 🟡 카메라 노티파이가 몽타주 소유자와 무관하게 첫 로컬 플레이어의 시점을 바꾼다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:28`
- **범주**: 버그/정확성
- **문제**: 원격 플레이어 폰만 제외하고 AI·비 Pawn 액터는 통과시킨 뒤 36행에서 항상 `GetFirstLocalPlayerController`를 고른다. 따라서 해당 노티파이가 들어간 AI 몽타주가 복제 재생되면 각 클라이언트의 첫 플레이어 카메라가 AI 쪽으로 전환될 수 있고, 분할 화면에서는 두 번째 로컬 폰의 몽타주도 첫 플레이어 시점을 바꾼다.
- **제안**: 런타임에서는 몽타주 소유 폰이 `IsLocallyControlled()`인 경우만 허용하고 그 폰의 Controller를 사용한다. AI 연출이 필요하다면 수신 PlayerController를 명시하는 별도 게임 로직으로 분리한다.
- **확신도**: 중간

### 5. 🟢 GP 드레인이 즉시 틱 때문에 몽타주보다 한 주기 먼저 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_DrainGP.cpp:16`
- **범주**: 버그/정확성
- **문제**: 적용 즉시 주기 효과를 한 번 실행하면서 틱당 차감량은 54행에서 `MaxGP / Duration * Period`로 계산한다. 첫 차감이 `t=0`에 일어나므로 GP는 의도한 Duration보다 `DrainPeriod`(약 33ms) 먼저 0에 도달하고, GP 변경 델리게이트가 그로기 어빌리티를 몽타주 끝보다 일찍 종료한다.
- **제안**: `bExecutePeriodicEffectOnApplication`을 끄거나, 즉시 실행을 유지해야 한다면 실제 실행 횟수를 기준으로 틱당 차감량을 계산한다.
- **확신도**: 높음

### 6. 🟢 이미 사망한 소환수 등록을 취소하고도 성공을 반환한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Summon/WxSummonComponent.cpp:40`
- **범주**: 버그/정확성
- **문제**: 등록 직후 `Ability.Death`가 있으면 목록과 델리게이트를 제거하지만 46행에서 그대로 `true`를 반환한다. 호출자 `UWxAbilityBase::SpawnSummon`은 성공으로 판단해 액터를 파괴하지 않으므로, 소유자 종료 때도 정리되지 않는 미추적 소환수가 남는다.
- **제안**: 사망 태그가 이미 있으면 등록 전에 `false`를 반환하거나, 현재 분기에서 제거 후 `false`를 반환해 호출자가 스폰 액터를 파괴하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Summon/WxSummonComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Public/` 전체 헤더, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 주요 파생 어빌리티, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 주요 큐, 대응 Damage·Task 소스
- **미검토 / 한계**: BP/WBP와 몽타주 노티파이 실제 배치, DataTable 행 값, TargetingPreset 에셋 내부 필터 구성은 범위 밖이다. 카메라 문제는 해당 노티파이를 AI·공유 몽타주에 배치하는 콘텐츠 구성에서 드러나므로 확신도를 중간으로 두었다. 런타임 네트워크·성능 프로파일은 수행하지 않았다.

---
*문서 기준 커밋 `66c0f6fd` · 리뷰일 2026-08-30 · 소스 152파일 — `/module-review`로 갱신*
