# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 런타임으로, 규칙 준수도와 주석 품질이 모두 높고 명백한 널 역참조·수명 버그는 발견되지 않았다. 이번 리뷰는 `Build.cs`/`.uplugin`과 전체 헤더를 훑은 뒤 어빌리티 베이스·ASC·어트리뷰트셋·데미지 ExecCalc·무기/투사체·락온·애님 노티파이·큐·타게팅 필터의 cpp를 실제 로직까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 예측으로 건 ActivationOwnedEffects를 낡은 핸들로 걷는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:209`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:106`
- **범주**: 버그/정확성
- **문제**: `ActivateAbility`가 `ApplyGameplayEffectToOwner`로 얻은 `FActiveGameplayEffectHandle`을 `ActivationOwnedEffectHandles`에 담아 두었다가 `EndAbility`에서 `RemoveActiveGameplayEffect(EffectHandle)`로 걷는다. 이 경로는 `UWxAbilityBase`의 기본값인 LocalPredicted에서 소유 클라도 함께 밟는데, 예측으로 만든 액티브 GE는 서버본이 복제되어 들어오면 다른 항목으로 대체되어 클라가 쥔 핸들이 무효해진다. 그러면 클라의 제거는 조용히 no-op이 되고 `Effect.Guard`(`WxAbility_Guard.cpp:15`)·`Effect.Invincible`(`WxAbility_Ultimate.cpp:20`)·속도 배율이 서버 복제가 도착할 때까지 클라에 남는다. `Effect.Guard`는 `UWxExecCalc_Damage`가 가드 배율 판정에 직접 읽는 태그라(`WxEffect_Damage.cpp:140`) 그 창 동안 클라 예측 대미지가 서버와 달라진다. 같은 함정을 이미 겪은 두 곳은 정의(클래스)로 지우는 쪽을 택했다 — `WxAbilityTask_PlaySkillCutscene.cpp:159`의 주석("예측으로 건 GE의 핸들은 서버본이 도착하면 무효해져 쓰지 못하므로 정의로 찾는다")과 `WxAnimNotifyState_ApplyGameplayEffect.cpp:30`. 즉 핸들 방식을 쓰는 베이스 클래스와 Sprint 쪽이 예외로 남아 있다.
- **제안**: `EndAbility`의 제거를 `RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, 1)`로 바꿔 이미 검증된 다른 두 경로와 맞춘다. Sprint의 `SpeedEffectHandle`·`DrainEffectHandle`도 같다.
- **확신도**: 중간

### 2. 🟡 크리 판정 난수를 서버와 클라가 각자 굴려 결과가 갈린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:170`
- **범주**: 설계/구조
- **문제**: `bIsCritical = FMath::FRand() < CritChance;`로 ExecCalc 안에서 크리를 굴린다. 대미지 GE는 무기 경로에서 양쪽 머신이 모두 적용하도록 되어 있고(`WxWeaponBase.cpp:254` "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다", `WxCombatLibrary.cpp:130`이 예측 키를 실어 `ApplyGameplayEffectSpecToTarget` 호출), 같은 시드가 아니므로 소유 클라의 예측 결과와 서버 결과가 서로 다른 크리로 갈릴 수 있다. 그 결과가 `FWxCombatEffectContext::SetCritical`(`WxEffect_Damage.cpp:192`)에 기록되고 `ProcessDamageTaken`이 데미지 플로터 큐에 실어 보내므로(`WxCombatAttributeSet.cpp:335`), 클라에는 크리 표시와 수치가 한 번 튀었다가 복제 HP로 정정되는 그림이 나온다. 참고로 투사체 경로는 `HasAuthority()`로 서버만 적용하고 있어(`WxProjectileBase.cpp:96`) 무기 경로와 권위 모델이 서로 다르다.
- **제안**: 크리 판정을 서버 권위 한 곳으로 모으거나(예: 예측 클라는 크리를 항상 false로 두고 서버가 컨텍스트로 정정), 히트 식별자를 시드로 삼는 결정적 난수를 쓴다. 싱글 위주로 유지할 생각이면 무기 경로도 투사체처럼 권위 게이트를 두는 쪽이 두 경로의 모델을 일치시킨다.
- **확신도**: 중간(현재 싱글 플레이 위주라 의도적으로 감수한 것일 수 있음)

### 3. 🟡 Attack·Skill·Pattern이 콤보 로직을 통째로 3중 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:25`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: 세 클래스가 `TArray<TObjectPtr<UAnimMontage>> ComboMontages` + `int32 ComboIndex = INDEX_NONE` 필드를 각자 선언하고(`WxAbility_Attack.h:34`, `WxAbility_Skill.h:37`, `WxAbility_Pattern.h:31`), `ActivateAbility`의 커밋→인덱스 전진→`PlayMontage` 3단, `EndAbility`의 `bWasCancelled` 시 인덱스 리셋을 글자 그대로 같게 반복한다. Attack과 Skill은 `HandleMontageCompleted`의 리셋까지 동일하다. 콤보 전진 규칙이 바뀌면 세 곳을 동시에 고쳐야 하며, 한 곳을 빠뜨려도 컴파일로는 드러나지 않는다.
- **제안**: 세 클래스의 실제 공통 의미는 "콤보 몽타주 열을 순서대로 재생하는 배타 액션"이므로, 그 공통분만 중간 베이스로 올리고 Pattern의 `HandleMontageBlendOut` 자동 전진 같은 차이는 각자 오버라이드로 남긴다. 배선 재사용을 위한 상속이 아니라 의미가 겹치는 구간만 올리는 선을 지킨다.
- **확신도**: 중간(중복 자체는 사실이나, 소폭 반복을 용인해 온 프로젝트 기조와 어느 선까지 묶을지는 판단이 필요함)

### 4. 🟡 CameraMove 노티파이가 플레이어 뷰타깃 전환까지 직접 쥔다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotifyState_CameraMove.h:17`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:36`
- **범주**: 설계/구조
- **문제**: 헤더가 이미 `// TODO: 게임 로직 이관 필요`로 표시해 둔 부채다. 이 노티파이는 `GEngine->GetFirstLocalPlayerController`로 로컬 컨트롤러를 집어(`:36`) `ACameraActor`를 스폰하고(`:50`) `SetViewTargetWithBlend`로 뷰를 뺏었다가(`:71`) 종료에서 폰으로 되돌린다(`:180`). 카메라·컨트롤러 조립은 README가 소비 측(WxGame·GameFeature)에 위임한다고 선언한 영역이고, `GetFirstLocalPlayerController`는 아바타의 실제 소유 컨트롤러가 아니라 첫 로컬 컨트롤러를 집으므로 분할 화면·다중 로컬 플레이어에서는 근거가 없다.
- **제안**: 노티파이는 "카메라 연출 구간 시작/종료" 이벤트만 발행하고, 카메라 액터 수명과 뷰타깃 전환은 WxGame 쪽 컴포넌트가 받아 처리하도록 옮긴다. 최소 조치로는 컨트롤러 조회를 아바타 폰의 소유 컨트롤러 기준으로 바꾼다.
- **확신도**: 중간

### 5. 🟢 최대치 변경 시 현재값을 읽어 베이스값에 쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:186`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`가 `ASC->GetNumericAttribute`(모디파이어가 반영된 현재값)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(베이스값)에 쓴다. 지금은 HP/SP/GP/MP/UP에 지속시간 모디파이어가 붙지 않아 두 값이 같으므로 증상이 없지만, 나중에 "HP +20 지속 버프" 같은 것이 생기면 MaxHP가 바뀌는 순간 그 +20이 베이스에 구워지고 버프가 만료될 때 그만큼 깎여 나간다.
- **제안**: `GetNumericAttributeBase`로 읽어 베이스끼리 스케일하도록 맞춘다.
- **확신도**: 낮음(현재 데이터 구성에서는 재현되지 않음)

### 6. 🟢 `PreviousMontageTickOption`이 초기화되지 않은 채 선언돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:91`
- **범주**: 성능/안전
- **문제**: `EVisibilityBasedAnimTickOption PreviousMontageTickOption;`에 초기값이 없다. 실제로는 `EnableAnimatingMontageMeshTick`이 `MontageTickMesh`를 세우면서 반드시 먼저 대입하고 `RestoreAnimatingMontageMeshTick`은 그 메시가 있을 때만 읽으므로 현재 흐름에서 미초기화 값을 쓰지는 않지만, 같은 헤더의 다른 멤버들과 달리 이것만 기본값이 없어 정적 분석·향후 경로 추가에 취약하다.
- **제안**: `= EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` 등 명시적 기본값을 준다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Public/**` 전체 헤더, 나머지 `WxAbility_*`·`WxEffect_*`·`WxCueNotify_*`·`WxAnimNotify*`·`WxTargetingFilterTask_*`·`WxAbilityTask_*` cpp
- **미검토 / 한계**:
  - 규칙 준수는 기계적으로 확인했고 위반이 없다 — 플러그인 의존은 `WxCore`뿐(`Build.cs`), 람다 0건, 델리게이트 콜백 전원 `Handle` 접두사, `FORCEINLINE`·인라인 정의 0건, `BlueprintCallable`은 `WxCombatLibrary.h:45` 한 곳뿐(BP Function Library로 허용 범위), 저작권 첫 줄은 161개 파일 전부 존재(일부 파일은 UTF-8 BOM이 앞서지만 규칙 위반은 아님).
  - GE·어빌리티·데이터테이블 **에셋의 실제 값**(`DT_Ability`/`DT_Damage`/`DT_Effect` 행, BP 서브클래스의 몽타주·태그 지정)은 확인하지 않았다. 위 지적 중 데이터 구성에 따라 증상이 달라지는 것은 5번이다.
  - 멀티플레이 실측 없이 코드만으로 판단했다. 1·2번은 리슨 서버 호스트/싱글에서는 드러나지 않고 리모트 클라에서만 문제가 되는 성질이라, 확정하려면 2인 PIE 검증이 필요하다.
  - 전투가 실제로 어떤 순서로 도는지(콤보 창 ↔ 입력 버퍼 ↔ 히트스톱의 프레임 단위 상호작용)는 정적 독해 범위를 넘어 검증하지 못했다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 161파일 — `/module-review`로 갱신*
