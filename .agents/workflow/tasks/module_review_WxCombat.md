# WxCombat — 코드 리뷰

> GAS 순정 경로(GE 컴포넌트·Immunity 통지·순정 쿨다운/코스트 API)를 잘 따르고 주석이 판단 근거를 충실히 남긴다. AGENTS.md 규칙 1·2 위반은 없다. 반면 여러 소유자가 참조 계수 없이 AI 브레인 일시정지를 제어하는 곳에서 실제 오동작 경로가 보인다.
> 커버리지: 피해 파이프라인, AttributeSet, ASC, 어빌리티 베이스와 주요 어빌리티 전부, 락온·타게팅, 무기·투사체, 피니셔, 미니언, 컷신, 히트스톱·입력 버퍼를 깊게 봤다. 나머지 GE·Cue·ANS·태스크 cpp는 대부분 훑었고, 헤더는 필요한 것만 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 AI 브레인 일시정지를 세 곳이 참조 계수 없이 따로 제어한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:181`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:67`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:187`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:191`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:55`
- **범주**: 설계/구조
- **문제**: `UBrainComponent`의 일시정지는 단일 플래그다. Reason 문자열은 로그용일 뿐이다(엔진 `BehaviorTreeComponent.cpp:173`·`:187`). 그런데 돌진 modifier·그로기·사망이 각자 `PauseLogic`/`ResumeLogic`/`StopLogic`을 부른다. 돌진 중에 GP가 차면 다음 순서가 된다.
  1. 그로기가 PreActivate에서 `Ability.*`를 취소하고(`WxAbility_Groggy.cpp:32`) 발동 즉시 정지를 건다(`:65`).
  2. 돌진 modifier는 그 뒤에 해제된다. 몽타주가 바뀐 다음 워핑 갱신에서 MarkedForRemoval되거나(엔진 `RootMotionModifier.cpp:287`), 몽타주 종료 시 ANS가 끝날 때다.
  3. 해제 시 `ReleaseState`는 `IsPaused()`만 보고 `ResumeLogic`을 부른다(`:181`). 결과적으로 그로기 도중 BT가 다시 돈다. 이동 태스크가 그로기 자세의 적을 끌고 다니거나 피니시 거리를 벗어나게 할 수 있다.

  그로기가 사망을 감지해 끝날 때(`WxAbility_Groggy.cpp:108`)도 `StopLogic("Death")` 뒤에 `ResumeLogic("Groggy")`를 부른다. AI 모듈(WxAI)에는 브레인 제어 코드가 하나도 없다. AI 실행 여부를 전투 모듈이 직접 정하는 구조다.
- **제안**: 브레인 제어의 주인을 하나로 둔다. 전투 쪽은 상태 태그(`Ability.Groggy`, `Ability.Death`, 돌진 중 태그)만 낸다. WxAI BT는 그 태그를 보는 데코레이터(관찰자 중단)로 분기를 멈춘다. 임시 대응으로 Rush 해제 시 `Ability.Groggy`·`Ability.Death`면 재개를 건너뛸 수는 있다. 다만 소유자가 늘면 같은 문제가 재발한다.
- **확신도**: 중간. 호출 순서는 엔진 코드로 확인했지만 플레이로 재현하지는 않았다.

### 2. 🟡 콤보 진행 코드가 Attack·Skill·Pattern에 세 벌 복제되어 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 필드(`WxAbility_Attack.h:30`, `WxAbility_Skill.h:36`, `WxAbility_Pattern.h:30`)와 인덱스 전진·취소 시 초기화 로직이 세 클래스에 그대로 있다. Attack과 Skill은 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted` 본문이 같고 생성자 태그·쿨다운만 다르다. Pattern은 블렌드아웃 체이닝(`WxAbility_Pattern.cpp:48`)만 다르다. 콤보 규칙을 바꾸면 세 곳을 함께 고쳐야 한다.
- **제안**: 콤보 진행(필드와 Activate/End/Completed 처리)을 한곳으로 모은다. 공통 콤보 베이스 하나를 두거나, `UWxAbilityBase`의 몽타주 헬퍼 옆에 선택적 콤보 진행을 둔다. Pattern의 자동 체이닝만 오버라이드로 남긴다.
- **확신도**: 높음

### 3. 🟡 피해 판정 쿼리가 권위 없는 머신에서도 돈 뒤 `ApplyDamage`에서 버려진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:48`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:178`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp:32`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:49`
- **범주**: 성능/안전
- **문제**: WeaponAttack ANS와 AreaDamage 노티파이는 몽타주를 재생하는 모든 머신(서버, 소유 클라, 모든 시뮬 프록시)에서 실행된다. 무기는 `BeginAttack`에서 판정 형상을 켜고 틱을 돌려 형상마다 매 틱 `SweepMultiByChannel`을 한다. AreaDamage는 TargetingSystem 쿼리를 실행한다. 그러나 결과 소비처는 `ApplyDamage`(`WxWeaponBase.cpp:238`, `WxAnimNotify_AreaDamage.cpp:45`)뿐이고, 권위 검사(`WxCombatLibrary.cpp:49`)에서 전부 버려진다. 적이 많은 오픈월드에서는 클라마다 모든 적 공격의 스윕 비용을 낸다.
- **제안**: `AWxWeaponBase::BeginAttack`/`Tick`과 `UWxAnimNotify_AreaDamage::Notify` 초입에 권위 게이트를 둔다. 사망 시 `CancelAttack`은 모든 머신에서 불려도 무해하다.
- **확신도**: 높음

### 4. 🟢 락온 어빌리티에 도달하지 않는 폴백과 조회 가능한 값의 저장이 남아 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:88`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:49`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_LockOn.h:93`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp:132`
- **범주**: 중복/복잡도
- **문제**:
  - `UWxLockOnComponent`는 `AWxCharacterBase`에 네이티브로 부착된다(`Source/WxGame/Character/WxCharacterBase.cpp:38`). 그래서 컴포넌트가 없을 때의 폴백(어빌리티 `:88`, 카메라 태스크의 초기 타겟 폴백 `:132`)은 현재 어떤 아바타에서도 타지 않는다.
  - 컴포넌트를 약참조로 캐시(`WxAbility_LockOn.h:91`)하면서도 두 핸들러는 다시 `FindComponentByClass`한다(`WxAbility_LockOn.cpp:193`, `:247`).
  - `bOrientRotationToMovement`는 직전 값을 `TOptional`에 저장해 복원한다(`:49`, `:113`). 같은 플래그를 WxAI는 무브먼트 아키타입 기본값으로 복원한다(`Plugins/WxAI/Source/WxAI/Private/WxBTService_LockOn.cpp:147`). 복원 규약이 둘로 갈려 있다.
- **제안**: 폴백 분기를 걷고 캐시를 하나로 쓴다. 복원은 아키타입 기본값에서 읽어 저장 필드를 없앤다.
- **확신도**: 중간

### 5. 🟢 AttributeSet 접근자 매크로가 헤더에 인라인 정의 76개를 만든다(규칙 3)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h:10`
- **범주**: 규칙 위반
- **문제**: `ATTRIBUTE_ACCESSORS`가 GAS 매크로를 묶어 19개 어트리뷰트마다 클래스 본문 인라인 함수 4개를 정의한다(엔진 `AttributeSet.h:428`~`:455`). AGENTS.md 규칙 3의 예외는 템플릿 함수와 `GetInstanceDataType()`뿐이다. 예외라면 해당 지점에 사유 주석이 있어야 하는데 없다.
- **제안**: GAS 관용이므로 제거하지 않는다. 매크로 정의 지점에 예외 사유 주석을 달거나 AGENTS.md 예외 목록에 GAS 어트리뷰트 접근자 매크로를 추가한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**:
  - 진입점·피해 파이프라인: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageReaction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_PerfectGuard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_AdditionalEffects.cpp`
  - GAS 기반: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`
  - 주요 어빌리티: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`
  - 피니셔·컷신·락온: `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`
  - 무기·투사체·모션 워핑: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`
  - 보조 시스템: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지(Attack·Skill·Pattern·Passive·PlayMontageOnce·Sprint), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE·MMC, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 타게팅 태스크·프리뷰, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`. 규칙 1·2·3은 전 파일을 Grep으로 확인했다.
- **미검토 / 한계**:
  - 빌드·실행·멀티플레이 재현은 하지 않았다. BP/DataTable/몽타주 내부값(노티파이 시각 등)은 uasset 문자열 검색으로 클래스 참조만 확인했다.
  - Iris 환경에서 커스텀 `FWxDamageEffectContext`의 직렬화는 확인하지 않았다.
  - 피해 파이프라인의 다음 항목은 [`.agents/workflow/tasks/damage-pipeline-structure-review.md`](damage-pipeline-structure-review.md) 작업 범위라 여기서 다루지 않았다.
    - AttributeSet 모디파이어 순서에 따른 사망·그로기 발행 순서 정리
    - 가드 방향과 `_DamageReaction`의 가드 취소 조건
    - 커스텀 Context 할당 경로(`AllocGameplayEffectContext`)
    - `ExecCalc` 계산 헬퍼 분리
  - 그 작업 문서의 "현재 흐름" 절(51~55행)은 삭제된 Hit GE·Hit 컴포넌트·DamageResponse 경로를 서술해 현재 코드(`ApplyDamage` → `UWxEffect_Damage` → Damage GE 컴포넌트 4종)와 어긋난다.

---
*문서 기준 커밋 `e72c9179f` · 리뷰일 2026-09-23 · 소스 203파일 — `/module-review`로 갱신*
