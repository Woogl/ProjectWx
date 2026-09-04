# WxCombat — 코드 리뷰

> 전반적으로 건강한 모듈이다. 모듈 경계(`WxCore` 외 Wx 의존 없음)·`Wx` 접두사·`Handle` 콜백 접두사·`BlueprintCallable` 사용처·인라인 정의 금지·저작권 첫 줄 등 프로젝트 규칙 위반은 한 건도 없었고, 예측·권위 분리와 GAS 계약을 아는 주석이 위험 지점마다 붙어 있다. 이번 리뷰는 대미지 파이프라인(ExecCalc·AttributeSet·CombatLibrary), 어빌리티 기반 클래스와 발동 그룹/캔슬 창, ASC 입력 라우팅과 선입력 버퍼, 무기·투사체 히트, 락온(어빌리티·컴포넌트·태스크), 소환/처형/히트스톱, AnimNotify·GameplayCue·Targeting 태스크·GE 정의 전반을 cpp까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:167`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance` 는 시드를 공유하지 않는다. 공격 어빌리티는 `LocalPredicted`이고 `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`) 실행 계산이 소유 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② `GP`·`Damage.GuardBreak` 판정(같은 함수 205행)까지 갈려 가드 브레이크 여부가 어긋나며 ③ `FWxCombatEffectContext::SetCritical`이 실은 크리 표식이 클라 화면의 플로터와 서버 값 사이에서 불일치한다. 로컬 플레이만 하는 동안에는 드러나지 않는다.
- **제안**: 크리 판정을 권위에서만 굴려 컨텍스트에 싣고 예측 측은 그 값을 읽게 하거나(비크리 낙관 예측), 예측 키·스펙 식별자를 시드로 삼는 결정적 난수로 바꾼다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 의도적으로 미룬 것일 수 있음)

### 2. 🟡 같은 InputAction을 공유하는 뒤쪽 어빌리티는 `InputPressed`를 못 받는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:104`
- **범주**: 버그/정확성
- **문제**: `AbilityInputActionTriggered`는 매칭 스펙을 순회하며 `Spec.InputPressed = true`를 세운 뒤 `TryActivateAbility`가 성공하면 즉시 `return true`한다. 한 IA에 어빌리티가 둘 이상 걸려 있고 앞 스펙이 성립하면, 뒤 스펙의 키 상태는 눌린 적 없는 채로 남는다. 반면 `AbilityInputActionReleased`는 매칭 스펙 전부를 내리므로 세우기/내리기가 비대칭이다. 이 값을 발동 조건으로 읽는 `UWxAbility_Guard::CanActivateAbility`(`WxAbility_Guard.cpp:27`)와 `IsInputHeld`(같은 파일 119행)는 그런 배치에서 버퍼 재시도를 영구히 거부한다. `UWxAbilitySet::GetInputActions`가 `AddUnique`를 쓰는 것(`WxAbilitySet.cpp:73`)과 `UWxInputBufferComponent::InputActionTriggered`가 매칭 스펙 전부를 훑는 것(`WxInputBufferComponent.cpp:47`)은 IA 공유를 전제로 읽힌다.
- **제안**: 키 상태 세우기를 발동 시도와 분리해 매칭 스펙 전부에 먼저 대입한 뒤, 두 번째 순회에서 발동을 시도한다.
- **확신도**: 중간(실제로 IA를 공유하는 배치가 없다면 드러나지 않음)

### 3. 🟡 공격 구간이 겹치면 스윙 피격 기록과 대미지 행이 뒤섞인다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:56`
- **범주**: 버그/정확성
- **문제**: `BeginAttack`은 `ActiveAttackCount`로 구간을 참조 계수하면서도(61·74행) `HitActorsThisSwing.Empty()`와 `DamageInfo` 대입은 계수와 무관하게 매번 수행한다. 즉 겹친 두 구간이 기록 하나·대미지 행 하나를 공유하는데, 뒤 구간의 시작이 앞 구간의 기록을 지운다. 앞 구간의 콜리전이 아직 켜져 있으므로(89행은 계수가 0일 때만 끔) 이미 맞은 대상이 같은 스윙 안에서 한 번 더 성립하고, 앞 구간의 남은 틱 Sweep은 뒤 구간의 `DamageInfo`로 대미지를 준다. `bIsNativeBranchingPoint`로 순서가 보장되는 콤보 전환에서 앞 ANS의 End보다 뒤 ANS의 Begin이 이른 배치가 실제 트리거 조건이다.
- **제안**: 계수가 0에서 올라갈 때만 기록을 비우고, 대미지 행은 구간별로 들고 다니거나 최소한 계수가 0일 때만 교체한다.
- **확신도**: 중간(구간을 겹쳐 저작하지 않는다면 드러나지 않으며, 겹침 허용 자체는 의도로 보임)

### 4. 🟡 대미지 플로터가 피격 1회마다 액터+위젯을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고(34행) `InitWidget()`으로 UMG 위젯을 새로 구성한다(59-60행). 액터의 `InitialLifeSpan`이 5초라(52행) 다타·다수 적 상황에서 화면당 수십 개가 동시에 살아 있게 되며, 각각이 Screen-space `UWidgetComponent`를 들고 매 프레임 그려진다. 액터 스폰과 위젯 생성은 전투 핫패스에서 가장 비싼 축이다.
- **제안**: 플로터 액터/위젯을 풀링해 재사용하거나, 수명을 애니메이션 길이에 맞춰 줄이고 동시 표시 개수 상한을 둔다.
- **확신도**: 중간(현 규모에서는 문제가 안 될 수 있음)

### 5. 🟡 콤보 어빌리티 3종이 상태와 진행 로직을 그대로 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:33,37` / `WxAbility_Skill.h:37,41` / `WxAbility_Pattern.h:31,34`), 인덱스 전진·`PlayMontage`·`EndAbility`의 `bWasCancelled` 리셋까지 세 cpp가 사실상 같은 문장이다. Attack과 Skill은 쿨다운 GE 지정과 애셋 태그를 빼면 구현이 동일하다. 콤보 단계 규약이 바뀌면 세 곳을 함께 고쳐야 하고, 실제로 Pattern만 `HandleMontageCompleted`에서 인덱스를 되돌리지 않는 차이가 이미 생겼다.
- **제안**: "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고, 재발동 방식(입력 재발동 vs 블렌드아웃 자동 전진)만 파생에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 배선 재사용 목적의 상속은 과거에 거부된 방향이므로, 공통분이 정말 의미 단위인지 먼저 합의가 필요하다)

### 6. 🟢 파일명과 클래스명이 어긋난다 — `WxAbilityTask_LockOnCamera` vs `UWxAbilityTask_LockOnTarget`
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Task/WxAbilityTask_LockOnCamera.h:19`
- **범주**: 중복/복잡도
- **문제**: 파일은 `WxAbilityTask_LockOnCamera.h/.cpp`인데 정의된 유일한 클래스는 `UWxAbilityTask_LockOnTarget`이다. 이 모듈의 다른 파일은 전부 파일명=클래스명 규약을 지키고 있어, 클래스명으로 파일을 찾는 흐름이 여기서만 끊긴다.
- **제안**: 파일을 `WxAbilityTask_LockOnTarget.h/.cpp`로 개명한다(클래스명 변경이면 `CoreRedirects`가 필요하므로 파일 쪽을 맞추는 편이 안전하다).
- **확신도**: 높음

### 7. 🟢 `AWxGhostTrail::BeginPlay`가 캐릭터 메시를 널 검사 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:31`
- **범주**: 성능/안전
- **문제**: 바로 위에서 `OwnerCharacter`는 정성껏 검사해 로그까지 남기는데(24-29행), 이어지는 `OwnerMesh`는 검사 없이 `GetComponentTransform()`·`GetSkeletalMeshAsset()`·`CopyPoseFromSkeletalComponent`에 들어간다. 기본 서브오브젝트라 대개 유효하지만, 메시를 만들지 않은 파생 캐릭터에서는 그대로 크래시다.
- **제안**: `OwnerMesh` 널 검사를 추가하고 캐릭터 검사와 같은 방식으로 조기 반환한다.
- **확신도**: 높음

### 8. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:188`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다. 지금은 HP/SP/GP/MP/UP에 지속형 aggregator 모디파이어를 거는 GE가 없어 베이스=현재값이라 드러나지 않지만, 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 9. 🟢 쓰이지 않는 모듈 의존
- **위치**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs:22`
- **범주**: 중복/복잡도
- **문제**: `NavigationSystem`(22행)과 `InputCore`(31행)는 모듈 전체에서 참조가 하나도 없다. `NetCore`(23행)도 직접 쓰는 곳이 없고 `GameplayAbilities`를 통해 전이로 딸려 온다. 불필요한 Public 의존은 이 모듈을 참조하는 쪽까지 전파된다.
- **제안**: 세 항목을 제거하고 빌드가 통과하는지 확인한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`
- **훑은 파일**: 나머지 어빌리티(`WxAbility_Attack/Skill/Pattern/Passive/Ultimate/Sprint/Death/Guard/Finisher/PlayMontageOnce`), GE 정의 21종(`Private/AbilitySystem/Effect/*`), AnimNotify 11종(`Private/AnimNotify/*`), GameplayCue 6종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`), 어빌리티 태스크(`WxAbilityTask_SlowTime/PlaySkillCutscene/RotateToTarget/WaitMoving`), `WxAbilitySet.cpp`, `WxAbilitySystemGlobals.cpp`, `WxCombatEffectContext.cpp`, `WxDamageTableRow.cpp`, `WxEffectComponent_Table.cpp`, `WxFinisherDamageComponent.cpp`, `WxProjectileManagerComponent.cpp`, `WxLockOnPointComponent.cpp`, `WxCombat.Build.cs`, `WxCombat.uplugin`
- **미검토 / 한계**: 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE의 BP 파생, 몽타주 노티파이 배치)은 범위 밖이라, 발견 3·5처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해로만 판단했고 실행 검증은 하지 않았다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 169파일 — `/module-review`로 갱신*
