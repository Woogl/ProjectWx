# WxCombat — 코드 리뷰

> 발동 그룹·액션 페이즈·표 수치·태그 발행이라는 축이 일관되고 모듈 경계와 수명주기 해제도 대부분 제자리에 있어 구조적으로는 건강한 모듈이다. 남은 결함은 거의 전부 멀티플레이 경로(타격 시점에 빌려 쓰는 과거 활성화 예측 키, 비권위 머신의 시간 배율 보정)에 몰려 있다.
> 이번 리뷰는 181파일 전체를 기계 규칙 스캔하고 어빌리티·ASC·어트리뷰트·대미지 파이프라인·노티파이·태스크·타겟팅·무기/투사체/소환 cpp를 읽었으며, 예측·몽타주·시퀀서 판단은 설치된 UE 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `UWxCombatLibrary::ApplyDamage`가 타격 시점 AnimatingAbility의 활성화 예측 키를 빌려 써, 소유 클라에 예측 GE가 영구히 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:79-91`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:128`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:140`
- **범주**: 버그/정확성
- **문제**:
  - 무기 스윙(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:22-25` → `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:239`)과 광역 노티파이(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp:39`)는 권위 게이트 없이 소유 클라에서도 `ApplyDamage`를 부르고, 여기서 `Source->GetAnimatingAbility()`의 활성화 키를 그대로 GE 적용에 넘긴다.
  - 스윙 판정 프레임은 보통 서버가 그 키를 이미 확인한 뒤다. 엔진은 클라에서 Instant GE를 무한 지속으로 바꿔 대상 ASC에 넣고(엔진 `AbilitySystemComponent.cpp:1066`) 그 키에 정리 델리게이트를 새로 건다(엔진 `GameplayEffect.cpp:4516-4533`).
  - `CatchUpTo`는 그 키가 확인되는 순간에 등록돼 있던 것만 발화하고(엔진 `GameplayPrediction.cpp:340-355`), 뒤늦게 등록된 것은 기본 `AbilitySystem.PredictionKey.StaleKeyBehavior=2`가 실행 없이 버린다(엔진 `GameplayPrediction.cpp:27`, `GameplayPrediction.cpp:680`).
  - 그래서 소유 클라가 적중할 때마다 `UWxEffect_Damage` 항목이 클라 쪽 대상 ASC에 쌓이고, 대미지 행 `AdditionalEffects`에 모디파이어·부여 태그가 있으면 그 값이 클라에서만 영구히 틀어진다. `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:224` 주석의 "큐·히트스톱만 앞당긴다"와 달리 GE 항목이 남는다.
  - 서버 전용 경로도 같은 뿌리를 탄다. 투사체(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:137-139`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:171`)가 맞을 때 쏜 쪽이 LocalPredicted 어빌리티를 재생 중이면 서버가 그 클라의 키를 실어 큐를 멀티캐스트하고, 쏜 클라는 예측했다고 보고 `GameplayCue.Hit`를 건너뛴다(엔진 `AbilitySystemComponent.cpp:1654`).
  - 컨텍스트 어빌리티(83행)도 같은 값이라, `UWxAbility_Passive`의 발동당 1회 지급 판정(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Passive.cpp:25-27`)이 투사체 적중을 그때 재생 중인 다른 공격 발동에 묶는다.
  - `UWxEffect_HitStop::Apply`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:32-37`)도 같은 키를 쓰지만 지속시간 GE라 만료로 걷혀 누수는 없다.
- **제안**: 노티파이 GE 수정(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:14-18`)과 같은 방향으로, `ApplyDamage`의 GE 적용은 권위에서만 하고 예측 키를 넘기지 않는다(소유 클라의 히트 큐·히트스톱은 복제를 따라 RTT만큼 늦어진다). 컨텍스트 어빌리티는 적중을 낸 발동을 명시로 넘기는 편이 정확하다(무기는 `BeginAttack`, 투사체는 스폰 시점에 잡아 두기).
- **확신도**: 높음(엔진 경로를 소스로 확인) — 누적 규모와 `AdditionalEffects` 영향은 저작에 달려 있고 네트워크 PIE 실측은 없다.

### 2. 🔴 컷신 태스크가 비권위 머신에서도 시퀀스를 요청 배율 기준으로 1000배속 재생해, 원격 소유 클라에서는 궁극기 컷신이 첫 틱에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117-120`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:58-64`
- **범주**: 버그/정확성
- **문제**: 전역 배율은 권위에서만 걸고(58-64행) 클라에는 `AWorldSettings` 복제로 늦게 도착하는데, 재생 속도 보정 `SetPlayRate(1 / GlobalTimeDilation)`은 머신 구분 없이 요청값 0.001(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:54`) 기준으로 건다. 시퀀스 틱은 월드 게임 시간 델타로 진행하므로(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:303`, `MovieSceneSequenceTickManager.cpp:343`), 배율이 아직 1인 소유 클라에서는 한 프레임(약 16ms)이 16초로 진행해 컷신이 곧바로 `OnFinished`에 닿는다. 그러면 `HandleCutsceneCompleted`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:64-77`)가 서버보다 컷신 길이만큼 먼저 `UltimateMontage`를 틀고, 뒤이어 도착한 0.001 배율이 그 몽타주를 멈춘다 — 원격 플레이어는 컷신을 보지 못하고 몽타주·노티파이 타이밍도 서버와 어긋난다. `.claude/worklog/2026-07-31-타임딜레이션-소유권-일원화.md:66`은 이를 "순간적으로 시퀀스가 빠르게 보일 수 있다"로 적었지만 실제 폭은 컷신 전체다. 스탠드얼론·리슨 서버 호스트는 배율이 같은 프레임에 걸려 드러나지 않는다.
- **제안**: 보정값을 요청값이 아니라 그 머신에 실제로 걸린 배율에서 구해 배율이 바뀔 때 다시 맞추거나, 시퀀스를 월드 배율과 무관한 클럭으로 돌려 보정 자체를 없앤다.
- **확신도**: 중간(시퀀스 클럭이 기본 Tick이라는 전제는 스탠드얼론에서 보정이 맞게 동작한다는 점으로 추정했고, 네트워크 PIE 실측은 없다)

### 3. 🟡 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:20-21`). `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 두어, 슬롯 2~4 BP가 애셋 태그만 바꾸고 이 값을 놓치면 두 스킬이 `Cooldown.Skill.1` 하나에 서로 막힌다. `OnGiveAbility` 진단(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:280-285`)은 "쿨다운 태그 없음"만 잡으므로 이 실수를 통과시킨다. 같은 생성자의 애셋 태그 기본값(10-15행)은 빠뜨려도 안전한 방향이지만 쿨다운 기본값은 반대 방향이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌린다 — 그러면 기존 Error 진단이 슬롯 BP의 지정 누락을 잡는다.
- **확신도**: 중간(슬롯 BP의 실제 지정 여부는 범위 밖)

### 4. 🟡 사망 어빌리티가 AnimatingAbility를 놓지 않아, 시체 메시의 본 갱신 강제가 파괴될 때까지 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:66-68`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23-43`
- **범주**: 성능/안전
- **문제**: ASC는 몽타주 재생 시 메시를 `AlwaysTickPoseAndRefreshBones`로 올리고 마지막 AnimatingAbility가 해제될 때만 되돌린다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:79-110`). 엔진이 해제하는 곳은 어빌리티 종료(엔진 `AbilitySystemComponent_Abilities.cpp:1241-1244`)와 인터럽트이거나 `bAllowInterruptAfterBlendOut=false`인 태스크 블렌드아웃(엔진 `AbilityTask_PlayMontageAndWait.cpp:32-37`)뿐인데, 베이스는 그 플래그를 true로 넘기고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:245-246`) 사망은 끝나지 않으며 완료 훅도 비어 있다. 래그돌로 떨어지는 인터럽트 경로만 해제되고, 사망 몽타주로 쓰러진 시체는 서버(플레이어면 소유 클라도)에서 렌더 여부와 무관하게 매 프레임 본을 갱신한다 — `ACharacter` 기본값 `AlwaysTickPose`(엔진 `Engine/Source/Runtime/Engine/Private/Character.cpp:125`)라면 건너뛸 비용이다. `PendingDestroyTime` 기본값 0은 "파괴하지 않음"이라(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Death.h:45-47`) 오픈월드에서 시체가 쌓일수록 서버 비용이 는다.
- **제안**: 사망 몽타주가 끝나는 지점(`UWxAbility_Death::HandleMontageCompleted`, 자세 유지형이라 완료가 오지 않으면 재생 길이 시점)에서 `ClearAnimatingAbility(this)`를 부른다 — 몽타주 자체는 멈추지 않는다.
- **확신도**: 중간(엔진 해제 경로는 소스로 확인, 실제 비용은 시체 수명 저작에 달림)

### 5. 🟢 비권위 머신의 종료 경로 GE 제거는 권위 게이트에 막혀 경고만 남기고, 컷신 태스크만 게이트를 우회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:201-208`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:218-226`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본 0이라 비권위 `RemoveActiveGameplayEffect`가 핸들 유효성과 무관하게 Warning을 찍고 false를 돌려준다(엔진 `AbilitySystemComponent.cpp:1249-1260`). `ActivationOwnedEffectHandles`는 적용에 실패한 무효 핸들까지 담고(206행) 종료에서 머신 구분 없이 제거하므로, 소유 클라에서 `ActivationOwnedEffects`를 쓰는 어빌리티(C++ 기본값으로는 궁극기·처형, 처형은 ServerInitiated라 클라 핸들이 늘 무효)가 끝날 때마다 효과 수만큼 경고가 난다. 질주의 속도 배율 제거도 같다. 실제 정리는 예측본은 키 확인이, 서버본은 복제가 맡아 동작은 맞지만 218행 주석("효과가 새지 않는다")은 클라 제거가 동작한다고 읽힌다. 반대로 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 권위 검사 없이 컨테이너를 직접 부르므로(엔진 `AbilitySystemComponent.cpp:1292-1318`, 게이트가 있는 `RemoveActiveEffects`는 `AbilitySystemComponent.cpp:1832-1840`) 소유 클라가 복제된 무적 GE를 서버보다 먼저 로컬에서 걷는다 — 직전 리뷰의 "클라에서 조용히 0을 돌려준다"는 5.8 소스와 맞지 않아 정정한다.
- **제안**: 제거 호출은 권위에서만 하고 비권위는 핸들만 비운다(무효 핸들은 애초에 담지 않는다). 컷신 태스크의 정의 기반 제거도 권위로 한정하고, `WxAbilityBase.cpp` 218행과 `WxAbilityTask_PlaySkillCutscene.cpp` 26행 주석을 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 6. 🟢 `EWxAbilityActivationPolicy::OnGiven`은 C++ 사용처가 없고, 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:287-293`
- **범주**: 중복/복잡도
- **문제**: C++ 어디에도 `OnGiven`을 고른 곳이 없다. `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(엔진 `GameplayAbilityTypes.cpp:295`) 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다 — LocalPredicted 기본값에서는 클라 예측 발동과 서버발 활성 통지가 겹친다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
- **제안**: 선택지를 걷는다. 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(경합 양상)

### 7. 🟢 `WxCombat.uplugin`이 모듈이 의존하는 Niagara 플러그인을 선언하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs:31`, `Plugins/WxCombat/WxCombat.uplugin:15-36`
- **범주**: 설계/구조
- **문제**: 모듈은 `Niagara`를 비공개 의존으로 두고 투사체·큐가 Niagara 타입을 쓰지만(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:13-14`), 서술자의 `Plugins` 목록에는 없다. UBT는 이 조합을 플러그인 검증에서 "does not list plugin" 경고로 낸다(엔진 `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:1564-1578`). 같은 저장소의 `Plugins/WxWorld/WxWorld.uplugin:26`은 Niagara를 선언해 두었다. Niagara가 엔진 기본 활성이라 당장 로드가 깨지지는 않는다.
- **제안**: `WxCombat.uplugin`의 `Plugins`에 Niagara 항목을 더한다.
- **확신도**: 높음(선언 누락), 중간(경고 노출 — 캐시된 Makefile 빌드에서는 검증이 돌지 않아 최근 UBT 로그로는 확인하지 못함)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 파생 어빌리티 15개 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, 대응 Public 헤더
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰·락온 지점, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatModule.cpp`
- **미검토 / 한계**:
  - 규칙 스캔(181파일 기계 검사): 첫 줄 Copyright 누락 0건, `FORCEINLINE`·`inline`·람다 0건, 타입 prefix `Wx` 누락 0건, `BlueprintCallable`은 BP 함수 라이브러리(`UWxCombatLibrary::ApplyDamage`)뿐이다. Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include` 모두 `WxCore`(`WxGameplayTags.h`·`WxUIData.h`·`WxCollisionChannels.h`·`Minion/WxMinion.h`)만이다. 헤더 안 함수 본문은 `WxCombatAttributeSet.h`의 GAS 표준 `ATTRIBUTE_ACCESSORS` 매크로 전개뿐이라 엔진 매크로로 보고 올리지 않았다.
  - 멀티플레이 실측이 없다. 1·2·4·5·6번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이며 네트워크 PIE 재현은 하지 않았다.
  - BP·DataTable·LevelSequence 저작 값(GA_*·GE_*·DT_* 행, 시퀀스 클럭 소스, 몽타주 노티파이 배치, 시체 수명)은 범위 밖이다. 1·2·3·4번의 실제 발현 폭은 저작에 달려 있다.
  - 직전 리뷰 대비: 노티파이 구간 GE의 과거 키 재사용(`61f9d245`)과 Pattern 콤보 인덱스 잔류(`f51c28c2`)는 현재 코드에서 수정을 확인해 뺐고, 나머지 커밋은 주석 정리뿐이다.
  - 올리지 않은 항목: 콤보 창의 태그 요건 전체 면제는 `.claude/worklog/2026-09-10-콤보-창-재발동-태그-면제.md`가 폭을 알고 택한 결정이다. Attack·Skill·Pattern 콤보 코드 중복은 확정된 감수 사항이다. `WxAnimNotifyState_CameraMove`가 적·AI 몽타주에서 각 클라의 로컬 뷰를 바꾸는 것은 주석에 명시된 의도다. 홀드 IA의 Triggered가 매 프레임 활성 스펙에 `InputPressed`를 흘리는 계약(`UWxAbility_LockOn::InputPressed`가 누름 트리거 IA를 전제)은 IA 저작에 달려 있어 올리지 않았다. `.cpp` 익명 namespace·static 헬퍼는 선호 방침일 뿐 CLAUDE.md 규칙이 아니다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 181파일 — `/module-review`로 갱신*
