# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 규칙이 일관된 축(발동 그룹 · 액션 페이즈 · 데이터 테이블 수치 · 태그 발행)으로 정리돼 있고, 권위/예측 경계와 수명주기 해제가 제자리에 있는 건강한 모듈이다. 직전 리뷰의 지적 중 BOM(🟢)과 크리 예측(🟡)은 해소됐고, 남은 것은 저작 실수를 유도하는 기본값과 세 어빌리티의 콤보 중복이다. 이번 리뷰는 직전 리뷰 이후 변경분(Minion 로스터를 월드 전역 스폰 통지 기반으로 재설계, 투사체 되돌림 참조 승격, 무기 슬롯 탐색, 락온 표시 루즈 태그화, Rush 타겟 탐색 단일화)을 먼저 깊게 보고, 이어서 어빌리티 기반 클래스·ASC·선입력·어트리뷰트·데미지 파이프라인·히트스톱·시간 지연·락온·루트모션 수정자·노티파이·큐까지 핵심 로직 cpp를 내려가며 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1로 고정돼 슬롯 간 쿨다운이 섞일 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: `UWxAbilityBase` 생성자는 "엔진이 스택을 GE 클래스 단위로 병합해서, 여기에 공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"는 이유로 쿨다운 GE 기본값을 일부러 비워 둔다(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:20-22`). `UWxAbility_Skill`은 그 자리에 `UWxEffect_Cooldown_Skill_1`을 기본값으로 깔아 두므로, 슬롯 2~4 BP가 애셋 태그만 바꾸고 `CooldownGameplayEffectClass`를 갈아 끼우는 것을 잊으면 두 스킬이 `Cooldown.Skill.1` 하나를 공유해 서로의 쿨다운에 막힌다. 애셋 태그 기본값(같은 파일 10-15행)은 "빠뜨려도 지목·잠금에서 빠지지 않게"라는 안전 방향이지만, 쿨다운 기본값은 반대로 위험한 방향이다. `UWxAbilityBase::OnGiveAbility`의 진단(`WxAbilityBase.cpp:280-285`)은 "쿨다운 태그가 아예 없는 경우"만 잡아 이 실수를 통과시킨다.
- **제안**: `UWxAbility_Skill`에서 `CooldownGameplayEffectClass` 대입을 걷어 기존 "쿨다운 태그 없음" Error가 저작 누락을 잡게 하거나, 그대로 두려면 `OnGiveAbility` 진단을 "쿨다운 GE가 부여하는 태그가 이 어빌리티의 슬롯 애셋 태그와 짝이 맞는가"까지 보도록 넓힌다.
- **확신도**: 중간

### 2. 🟡 Attack · Skill · Pattern이 콤보 상태 기계를 세 벌로 복제하고, Pattern에서 이미 어긋나 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-53`, `Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`, `Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19-60`
- **범주**: 중복/복잡도
- **문제**: 세 클래스가 각자 `ComboMontages`/`ComboIndex`를 선언하고(`WxAbility_Attack.h:29-34` / `WxAbility_Skill.h:35-40` / `WxAbility_Pattern.h:29-33`), `ActivateAbility`의 커밋→인덱스 전진→몽타주 재생과 `EndAbility`의 취소 시 `INDEX_NONE` 복귀를 문자 그대로 같은 코드로 반복한다. 갈리는 것은 진행 신호뿐이다 — Attack/Skill은 엔진 재발동(`bRetriggerInstancedAbility`), Pattern은 `HandleMontageBlendOut`에서 스스로 다음 단을 건다.
  이미 어긋난 지점이 있다: Attack(`WxAbility_Attack.cpp:48-53`)과 Skill(`WxAbility_Skill.cpp:53-58`)은 `HandleMontageCompleted`를 오버라이드해 정상 완료에서도 `ComboIndex`를 `INDEX_NONE`으로 되돌리는데, Pattern은 그 오버라이드가 없고(`WxAbility_Pattern.h`에 선언 없음) `EndAbility`는 `bWasCancelled`일 때만 되돌린다(`WxAbility_Pattern.cpp:38-46`). 터미널 단에서 끝나면 다음 발동이 `IsValidIndex(ComboIndex+1)` 실패로 0으로 감싸 들어가 증상이 감춰지지만, `HandleMontageBlendOut`의 `PlayMontage` 실패 경로(`WxAbility_Pattern.cpp:56-59`, `ComboMontages`에 빈 슬롯이 있으면 성립)는 `bWasCancelled=false`로 종료하므로 `ComboIndex`가 배열 중간에 남는다. 그 뒤 발동은 앞 단을 건너뛰고 중간부터 시작한다.
- **제안**: 공통분(콤보 배열·인덱스 전진·종료 시 리셋)만 중간 베이스로 올리고 진행 신호는 각자 남긴다. 한 번에 묶지 않겠다면 최소한 Pattern의 리셋 규칙을 Attack/Skill과 맞춘다(정상 완료·중간 실패에서도 `ComboIndex` 초기화).
- **확신도**: 높음(중복), 중간(Pattern 리셋 누락의 실제 발현은 저작 값에 달려 있음)

### 3. 🟡 락온 지점 선택이 `GetComponents`의 비결정 순서에 기대고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp:30-48`, `Private/Targeting/WxLockOnPointComponent.cpp:50-68`
- **범주**: 버그/정확성
- **문제**: `ResolveLockOnTarget`은 "락온 가능한 첫 지점을 반환한다"(`Public/Targeting/WxLockOnPointComponent.h:27`)고 계약하지만, 후보를 `Actor->GetComponents<UWxLockOnPointComponent>(Points)`로 모은다. `AActor::GetComponents`는 `OwnedComponents`(TSet)를 순회하므로 반환 순서가 정의되지 않는다 — 즉 "첫 지점"에 의미가 없다. 헤더가 "한 액터에 여러 개를 붙여 부위별 락온을 구성한다"(같은 파일 11행)고 명시한 다부위 구성에서, 최초 락온이 머리에 걸릴지 몸통에 걸릴지 인스턴스마다·빌드마다 달라질 수 있다. 소비처가 셋이라 영향 범위도 넓다 — `UWxAbility_LockOn::ActivateAbility`의 최초 지점 선택(`Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:64-72`), `HandleTargetLost`의 재탐색(`같은 파일:219`), AI·애님 노티파이가 쓰는 `GatherLockOnPoints`. (`HandleRetargetRequested`는 화면 정렬도로 다시 정렬하므로 영향을 받지 않는다.)
- **제안**: 우선순위를 저작 가능한 값으로 만든다 — `UWxLockOnPointComponent`에 `Priority`(또는 `SortOrder`) UPROPERTY를 두고 `ResolveLockOnTarget`/`GatherLockOnPoints`가 그 값으로 정렬한 뒤 고르게 한다. 최소 처방으로는 컴포넌트 이름 등 안정적인 키로 정렬해 적어도 결정적으로 만든다.
- **확신도**: 중간(다부위 락온을 실제로 저작한 에셋이 있는지는 확인하지 못했다. 지점이 하나뿐이면 현재도 무해하다)

### 4. 🟡 콤보 창에서 태그 요건 전체를 면제해, 앞으로 추가될 차단 태그가 조용히 무시된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:169-178`
- **범주**: 설계/구조
- **문제**: `DoesAbilitySatisfyTagRequirements`가 콤보 창에서 무조건 `true`를 돌려주므로 `ActivationBlockedTags`·`ActivationRequiredTags`·`Source/TargetTags`·`AreAbilityTagsBlocked`가 통째로 건너뛰어진다. 주석은 피격·그로기·사망만 근거로 들고, 실제로 그 셋은 콤보를 먼저 취소하므로 지금은 이 경로에 닿지 않는다(확인함). 문제는 면제 폭이 그 셋보다 훨씬 넓다는 점이다 — 콤보를 가진 Exclusive 어빌리티에 나중에 `ActivationBlockedTags`(예: 탈진·침묵 계열)를 붙이면 첫 단에서만 걸리고 콤보 중에는 아무 표시 없이 통과한다. 이 면제가 필요한 이유는 좁다: 엔진이 `bRetriggerInstancedAbility`로 재발동을 처리하기 전에 `CanActivateAbility`를 먼저 묻기 때문에, 자기 자신의 `ActivationOwnedTags`가 자기 재발동을 막는 것만 피하면 된다.
- **제안**: 면제를 그 좁은 범위로 줄인다 — 조건 없는 `true` 대신 자기 `ActivationOwnedTags`만 요건 검사에서 제외한 뒤 `Super`를 호출한다. 그대로 둘 거라면 면제되는 검사 목록을 주석에 못 박아 이후 저작이 오해하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 락온 종료에서 이동 회전 플래그 복원이 무관한 ASC 유효성 검사에 묶여 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:104-124`
- **범주**: 설계/구조
- **문제**: `SavedOrientRotationToMovement` 복원(114-123행)은 캐릭터 이동 컴포넌트만 건드리는데 `ActorInfo->AbilitySystemComponent.IsValid()` 게이트(104행) 안에 들어 있다. 그 게이트가 필요한 것은 바로 위의 락온 컴포넌트 해제뿐이다. 게다가 저장 쪽(`같은 파일:45-51`)은 "autonomous proxy의 회전 정합을 위해 서버에서도 꺼야 하므로 `IsLocallyControlled` 게이트 앞에서 처리한다"며 일부러 게이트 밖에 두었으니, 저장과 복원의 조건이 대칭이 아니다. ASC가 무효한 종료는 거의 없어 현재 버그로 드러나지는 않지만, 조건과 대상이 어긋나 있어 읽는 사람이 매번 다시 추적해야 한다.
- **제안**: 복원 블록을 게이트 밖으로 빼고 `SavedOrientRotationToMovement.IsSet()`과 아바타 유효성만 보게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 무기 히트의 예측 모델 주석이 같은 파이프라인의 다른 주석과 어긋난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:224`
- **범주**: 중복/복잡도
- **문제**: `ProcessHit`의 첫 줄은 "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다"고 적혀 있다. 반면 같은 경로 끝의 `UWxExecCalc_Damage`에는 "예측 클라는 Instant GE를 무한 지속으로 바꿔 실행을 건너뛴다"는 주석이 붙어 있다(`Private/AbilitySystem/Effect/WxEffect_Damage.cpp:164`). 둘 다 옳게 읽으면 "스펙 적용은 양쪽, 실행(수치·판정 태그)은 권위만"인데, 224행만 읽으면 클라가 대미지까지 예측한다고 오해하게 된다. 실제로 직전 리뷰가 이 주석을 근거로 크리 예측 불일치를 지적했다가 취소됐다. 이 모듈에서 가장 헷갈리는 지점이라 서술이 갈려 있으면 같은 오독이 반복된다.
- **제안**: 224행을 "히트 판정과 스펙 적용은 양쪽이 수행하지만, 실행 계산은 권위에서만 돈다 — 클라가 예측하는 것은 히트스톱과 큐다" 수준으로 좁힌다. 예측 규약의 원본은 `WxEffect_Damage.cpp:164` 한 곳으로 두고 여기서는 그쪽을 가리키게 한다.
- **확신도**: 중간(주석만의 문제이며 코드 동작은 정상이다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, 대응 Public 헤더 전체
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`·`Plugins/WxCombat/WxCombat.uplugin`(`WxCore` 외 Wx 참조 없음 재확인), `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰·SnapToTarget, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`
- **규칙 스캔(전 파일 기계 검사)**: `Wx` prefix, 첫 줄 `// Copyright Woogle. All Rights Reserved.`(BOM 포함) — 181파일 전부 통과. `FORCEINLINE`·헤더 인라인 정의·람다 — 0건. `BlueprintCallable` — `UWxCombatLibrary`(BP Function Library) 1건뿐으로 규칙 범위 안. 델리게이트 콜백 `Handle` prefix — 바인딩 37건 전부 준수.
- **미검토 / 한계**:
  - 직전 리뷰의 🟢 "Dodge·GuardReact 4파일 BOM"은 해소를 확인했다(전 파일 BOM 0건). 🟡 "크리를 예측 경로에서 굴린다"도 해소로 판단한다 — `WxEffect_Damage.cpp:164`의 새 주석대로 예측 클라는 Instant GE를 무한 지속으로 바꿔 실행 계산을 건너뛰므로 클라에서 굴림이 돌지 않는다. 다만 이 환경에는 엔진 소스가 없어 UE 5.8의 `ApplyGameplayEffectSpecToSelf` 실제 분기를 다시 읽어 확인하지는 못했다(알려진 GAS 동작과 일치한다는 수준까지만 검증).
  - BP·DataTable 저작 값(GA_* / GE_* / DT_Damage·DT_Effect·DT_Ability 행, 몽타주 노티파이 배치, 락온 지점 개수)은 범위 밖이다. 1·2·3번은 "저작이 어긋날 수 있다"까지만 검증했고 실제로 어긋난 에셋이 있는지는 확인하지 않았다.
  - 멀티플레이 실측이 없다. 특히 재설계된 Minion 로스터가 전 머신에서 동등해지려면 소환물의 `Ability.Death`가 비권위 머신에도 보여야 한다(`WxMinionSubsystem.cpp:165`의 태그 구독이 로스터에서 시체를 내리는 유일한 신호다). `UWxAbility_Death`는 이 태그를 `ActivationOwnedTags`로만 발행하고(`WxAbility_Death.cpp:21`) 어빌리티는 `ServerInitiated`이므로, UE 5.8이 `ActivationOwnedTags`를 복제하지 않는다면 클라 로스터는 죽은 소환물을 계속 들고 있어 `WxAnimNotifyState_Rush`의 예측 대상이 서버와 갈린다. 엔진 동작을 확인할 수 없고 같은 전제가 모듈 밖(`AWxCharacterBase`의 `Ability.Death` 구독)에서도 광범위하게 쓰이고 있어 발견으로 올리지 않았다 — 리슨 서버 호스트에서는 증상이 드러나지 않으므로 데디케이티드 환경에서 한 번 확인할 값어치가 있다.
  - 그 밖의 변경분은 모두 읽고 결함을 찾지 못했다. `Reflect(APawn&)`의 참조 승격과 `Shooter` 널 검사 제거는 호출부의 `IsHostile(GetInstigator(), ...)`가 널 인스티게이터를 걸러 주므로 안전하다(`WxProjectileBase.cpp:122`, `WxCombatLibrary.cpp:13-19`). 무기 탐색을 `UChildActorComponent` 기준으로 바꾼 것은 `AWxCharacterBase`의 단일 `WeaponActor` 슬롯(`Source/WxGame/Character/WxCharacterBase.cpp:41`)과 맞다. 시간 지연을 건드리는 두 태스크(`WxAbilityTask_SlowTime`·`WxAbilityTask_PlaySkillCutscene`)는 해제 전에 자기가 박은 값과 현재 값을 비교하므로 서로를 덮지 않는다.

---
*문서 기준 커밋 `231068b` · 리뷰일 2026-09-13 · 소스 181파일 — `/module-review`로 갱신*
