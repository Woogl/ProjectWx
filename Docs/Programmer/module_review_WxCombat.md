# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 도메인치고는 건강한 편이다. 배타 발동 그룹·입력 버퍼·히트스톱·대미지 ExecCalc 같은 위험 지점마다 예측/권위 경계와 함정이 주석으로 남아 있고, 규칙(접두사·Copyright·Handle 콜백·BlueprintCallable·람다·인라인 정의)과 모듈 경계(`WxCore` 외 Wx 참조 0건)는 거의 그대로 지켜진다. 이번 리뷰는 `WxCombat.Build.cs`/`.uplugin`부터 ASC·어빌리티 베이스·어트리뷰트·ExecCalc·무기/투사체·락온·소환·타임딜레이션·AnimNotify·AbilityTask를 cpp 수준까지 봤고, 대미지 산식 자체는 직전 리뷰가 다뤘으므로 호출 경계와 수명주기 위주로 재검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `ApplyAttributeChange`만 널 가드가 없고, 호출마다 같은 이름의 transient GE를 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:181`
- **범주**: 버그/정확성 · 성능/안전
- **문제**: 같은 파일의 `ApplyDamage`(60행)·`ApplyEffect`(161행)는 모두 진입에서 널을 거르는데 `ApplyAttributeChange`만 검사 없이 194행에서 `TargetASC`를 역참조한다. 헤더에 공개된 static API(`WxCombatLibrary.h:54`)이므로, 현재 유일한 호출자(`WxCombatAttributeSet.cpp:362`, 패리 시 공격자 GP 반사)가 널을 미리 걸러 주고 있을 뿐이고 호출처가 하나만 늘어도 크래시가 된다. 더해서 183행 `NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ApplyAttributeChange")))` 는 매번 같은 Outer에 같은 이름을 요구한다 — 이름이 충돌하면 UE는 기존 오브젝트를 파괴하고 그 슬롯을 재사용하므로, 앞선 GE를 아직 참조하는 곳이 있으면 위험하고 없더라도 패리마다 UObject 할당이 쌓인다.
- **제안**: 진입에 `if (!TargetASC || !Attribute.IsValid()) return;` 을 넣는다. GE는 `SetByCaller` 배율을 쓰는 전용 `UWxEffect_*` 클래스로 대체하는 편이 낫고, 최소한 이름 인자를 빼서(`NewObject<UGameplayEffect>(GetTransientPackage())`) 유니크 이름을 받게 한다.
- **확신도**: 높음 (널 가드), 중간 (이름 재사용)

### 2. 🟡 Attack·Skill·Pattern이 콤보 진행 로직을 셋으로 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: 세 클래스가 `ComboMontages`/`ComboIndex` 필드와 `ActivateAbility`(커밋 → 인덱스 전진 → `PlayMontage` 실패 시 종료), `EndAbility`(취소 시 `INDEX_NONE`) 를 사실상 같은 문장으로 갖는다. `UWxAbility_Attack`과 `UWxAbility_Skill`은 태그·`bRetriggerInstancedAbility` 선언을 빼면 본문이 동일하다. 콤보 규칙(전진 조건·리셋 시점)을 바꾸면 세 곳을 함께 고쳐야 하고, `UWxAbility_Pattern`만 `HandleMontageCompleted`에서 인덱스를 리셋하지 않고 대신 `HandleMontageBlendOut`(48행)에서 자동 연결하는 차이가 있는데 이것이 의도인지 누락인지 코드만으로는 구분되지 않는다.
- **제안**: `UWxAbilityBase`와 세 파생 사이에 콤보 전용 중간 베이스(예: `UWxAbility_ComboBase`)를 두어 `ComboMontages`·인덱스 전진·리셋을 한 곳에 모으고, 각 파생은 애셋 태그·`ActivationGroup`·자동 연결 여부만 선언하게 한다.
- **확신도**: 높음

### 3. 🟢 `RegisterSummon`이 등록을 취소하고도 `true`를 반환해 추적되지 않는 소환수가 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Summon/WxSummonComponent.cpp:40`
- **범주**: 버그/정확성
- **문제**: 등록 직후 대상 ASC에 이미 `Ability.Death`가 있으면 `RemoveSummon(Summon, false)`로 목록에서 걷어내지만 그대로 46행에서 `true`를 돌려준다. 호출자 `UWxAbilityBase::SpawnSummon`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:207`)은 이 반환값만 보고 실패를 판단하므로 `Destroy()`를 건너뛴다. 결과적으로 어느 컴포넌트도 추적하지 않고 소유자가 사라져도 정리되지 않는 폰이 월드에 남는다.
- **제안**: 그 분기에서 `false`를 반환하거나, 사망 태그가 이미 있으면 델리게이트를 걸기 전에 등록 자체를 거부한다.
- **확신도**: 중간(의도된 설계일 수 있음)

### 4. 🟢 소환 스폰 실패가 조용히 삼켜지고 `UAbilityTask_SpawnActor`가 정리되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:187`
- **범주**: 버그/정확성(미처리 실패 경로)
- **문제**: `UAbilityTask_SpawnActor::SpawnActor`로 태스크를 만든 뒤 `ReadyForActivation()` 없이 `BeginSpawningActor`/`FinishSpawningActor`만 직접 호출한다. 189행에서 `BeginSpawningActor`가 false를 반환하면 로그 한 줄 없이 반환하고 태스크는 `EndTask()`도 없이 버려지므로(성공 경로에서만 `FinishSpawningActor`가 끝낸다), 소환이 안 되는 원인을 런타임에서 추적할 수 없다. 이 모듈은 다른 실패 지점에서 `LogWxCombat` 경고를 남기고 있어(`WxTimeDilationComponent.cpp:97`, `WxCueNotify_GhostTrail.cpp:32`) 이 경로만 침묵한다.
- **제안**: 실패 시 `LogWxCombat` 경고를 남기고 `SpawnTask->EndTask()`로 정리한다. 태스크의 델리게이트를 쓰지 않을 것이면 `World->SpawnActorDeferred` + `FinishSpawning`을 직접 쓰는 편이 의도가 더 드러난다.
- **확신도**: 중간

### 5. 🟢 내부 헬퍼 구조체 두 개에 `Wx` 접두사가 빠졌다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Summon/WxSummonComponent.h:31`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h:173`
- **범주**: 규칙 위반
- **문제**: `FActiveSummon`과 `FMaxAttributePair`가 `F` 접두사만 있고 `Wx`가 없다. `CLAUDE.md` 코딩 규칙 1은 예외를 두지 않으며, 같은 모듈의 파일 지역 헬퍼조차 `FWxDamageBaseStatics`·`FWxDamageExecutionStatics`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:29,46`)로 접두사를 지키고 있어 이 둘만 어긋난다.
- **제안**: `FWxActiveSummon`, `FWxMaxAttributePair`로 개명한다. 둘 다 private 중첩 타입이라 파급 범위가 각 클래스 안으로 한정된다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Summon/WxSummonComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지 어빌리티(Attack·Skill·Pattern·HitReact·GuardReact·Death·BeingFinished·Ultimate·Passive), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`·`WxAbilityTask_WaitMoving.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, 대응 공개 헤더
- **미검토 / 한계**: 무기·투사체가 클라와 서버 양쪽에서 GE를 적용하는 설계(`WxWeaponBase.cpp:254` 주석의 명시적 결정)가 실제 네트워크에서 어떻게 수렴하는지는 엔진 GAS의 예측 키 처리에 달려 있어 코드만으로는 판정하지 않았다. 리모트 클라의 회피 TargetData(`WxAbility_Dodge.cpp:77`)가 도착하지 않는 경우의 타임아웃이 없으나, 신뢰 RPC라 접속 종료 외에는 재현 경로를 찾지 못해 발견으로 올리지 않았다. BP/WBP·몽타주 노티파이 배치·DataTable 행 구성, `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록 여부, 실제 런타임 프로파일은 확인하지 않았다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 150파일 — `/module-review`로 갱신*
