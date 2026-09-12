# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 규칙이 일관된 축(발동 그룹 · 액션 페이즈 · 데이터 테이블 수치 · 태그 발행)으로 정리돼 있고, 권위/예측 경계와 수명주기 해제가 대체로 제자리에 있는 건강한 모듈이다. 이번 리뷰는 커밋되지 않은 변경분(Minion 로스터 재설계, 투사체 되돌림, 무기 탐색, 락온 표시)을 우선 깊게 보고, 이어서 어빌리티 기반 클래스·ASC·어트리뷰트·데미지 파이프라인·히트스톱·선입력·락온·루트모션 수정자까지 핵심 로직 cpp를 내려가며 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 크리 판정을 예측 실행 경로에서 `FMath::FRand()`로 굴려 클라와 서버 결과가 갈린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:164`
- **범주**: 버그/정확성
- **문제**: 무기 히트는 클라와 서버가 같은 대미지 GE를 각자 적용한다(`Private/Weapon/WxWeaponBase.cpp:224` 주석, `WxCombatLibrary.cpp:128`이 예측 키를 실어 보냄). Instant GE는 유효한 예측 키가 있으면 클라에서도 실제로 실행되므로, 이 `FRand()`가 머신마다 따로 굴려진다. HP·GP 같은 어트리뷰트는 복제로 정정되지만, 같은 실행 안에서 스펙에 붙는 `Damage.Critical` 태그(`WxEffect_Damage.cpp:191`)는 정정되지 않는다. 이 태그를 그대로 읽어 플로터 갈래를 고르는 `UWxEffectComponent_DamageResponse::ProcessDamageTaken`(`Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp:87-89`) 때문에, 예측한 클라만 크리가 아닌 히트에 "크리티컬" 플로터를 보거나 그 반대가 된다. `Damage.GuardBreak` 판정도 같은 실행의 SP 캡처를 쓰므로 함께 어긋날 수 있다.
- **제안**: 굴림을 권위 전용으로 좁힌다 — `ExecutionParams.GetTargetAbilitySystemComponent()->IsOwnerActorAuthoritative()`가 아니면 `bIsCritical = false`로 두어 클라는 항상 비크리를 예측하게 하고(과대 예측 대신 과소 예측), 서버 결과가 복제로 덮게 한다. 크리 연출까지 클라에서 즉시 보여야 한다면 굴림 결과를 어빌리티 활성화 예측 키에서 파생한 결정적 시드로 만들어야 한다.
- **확신도**: 중간

### 2. 🟡 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1로 고정돼 슬롯 간 쿨다운이 섞일 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: `UWxAbilityBase` 생성자는 "엔진이 스택을 GE 클래스 단위로 병합해서, 여기에 공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"는 이유로 쿨다운 GE 기본값을 일부러 비워 둔다(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:20-22`). `UWxAbility_Skill`은 그 자리에 `UWxEffect_Cooldown_Skill_1`을 기본값으로 깔아 두므로, 슬롯 2~4 BP가 애셋 태그만 바꾸고 `CooldownGameplayEffectClass`를 갈아 끼우는 것을 잊으면 두 스킬이 `Cooldown.Skill.1` 하나를 공유해 서로의 쿨다운에 막힌다. 애셋 태그 기본값(같은 파일 10-15행)은 "빠뜨려도 지목·잠금에서 빠지지 않게"라는 안전 방향이지만, 쿨다운 기본값은 반대로 위험한 방향이다. 게다가 `UWxAbilityBase::OnGiveAbility`의 진단(`WxAbilityBase.cpp:280-285`)은 "쿨다운 태그가 아예 없는 경우"만 잡아 이 실수를 통과시킨다.
- **제안**: `UWxAbility_Skill`에서 `CooldownGameplayEffectClass` 대입을 걷어 기존 "쿨다운 태그 없음" Error가 저작 누락을 잡게 하거나, 그대로 두려면 `OnGiveAbility` 진단을 "쿨다운 GE가 부여하는 태그가 이 어빌리티의 슬롯 애셋 태그와 짝이 맞는가"까지 보도록 넓힌다.
- **확신도**: 중간

### 3. 🟡 Attack · Skill · Pattern이 콤보 상태 기계를 세 벌로 복제하고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-53`, `Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`, `Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19-46`
- **범주**: 중복/복잡도
- **문제**: 세 클래스가 각자 `ComboMontages`/`ComboIndex`를 선언하고(각 헤더 29-34행 / 35-40행 / 29-33행), `ActivateAbility`의 커밋→인덱스 전진→몽타주 재생과 `EndAbility`의 취소 시 `INDEX_NONE` 복귀를 문자 그대로 같은 코드로 반복한다. Attack과 Skill은 `HandleMontageCompleted`까지 동일해 사실상 세 본문 전체가 같다. 갈리는 것은 진행 신호뿐이다 — Attack/Skill은 엔진 재발동, Pattern은 `HandleMontageBlendOut`에서 스스로 다음 단을 건다. 지금은 무해하지만 콤보 규칙이 한 군데만 바뀌면 세 곳이 조용히 어긋난다.
- **제안**: 공통분(콤보 배열·인덱스 전진·취소 시 리셋)만 중간 베이스로 올리고 진행 신호는 각자 남긴다. 프로젝트가 "최소 인플레이스"를 선호하므로 한 번에 묶지 않는다면, 최소한 셋 중 한 곳에 "나머지 둘과 같은 규칙"임을 명시해 동기화 지점을 표시한다.
- **확신도**: 중간

### 4. 🟡 콤보 창에서 태그 요건 전체를 면제해, 앞으로 추가될 차단 태그가 조용히 무시된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:169-178`
- **범주**: 설계/구조
- **문제**: `DoesAbilitySatisfyTagRequirements`가 콤보 창에서 무조건 `true`를 돌려주므로 `ActivationBlockedTags`·`ActivationRequiredTags`·`Source/TargetTags`·`AreAbilityTagsBlocked`가 통째로 건너뛰어진다. 주석은 피격·그로기·사망만 근거로 들고, 실제로 그 셋은 콤보를 먼저 취소하므로 지금은 이 경로에 닿지 않는다(확인함). 문제는 면제 폭이 그 셋보다 훨씬 넓다는 점이다 — 콤보를 가진 Exclusive 어빌리티에 나중에 `ActivationBlockedTags`(예: 탈진·침묵 계열)를 붙이면 첫 단에서만 걸리고 콤보 중에는 아무 표시 없이 통과한다.
- **제안**: 면제를 의도한 범위로 좁힌다 — 조건 없는 `true` 대신 "재발동을 막는 것이 자기 자신의 `ActivationOwnedTags`인 경우"만 면제하거나(자기 태그를 요건 검사에서 제외한 뒤 Super 호출), 최소한 면제되는 검사 목록을 주석에 못 박아 이후 저작이 오해하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 5. 🟢 Dodge·GuardReact 4개 파일만 UTF-8 BOM으로 저장돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:1`, `Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp:1`, `Public/AbilitySystem/Ability/WxAbility_Dodge.h:1`, `Public/AbilitySystem/Ability/WxAbility_GuardReact.h:1`
- **범주**: 규칙 위반
- **문제**: 이 네 파일은 첫 줄 앞에 BOM(`EF BB BF`) 3바이트가 붙어 있어, "모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다"를 엄밀히는 만족하지 않는다. 모듈의 나머지 177개 파일은 BOM이 없다. 컴파일에는 지장이 없지만 diff·grep·첫 줄 검사 도구에서 이 두 클래스만 다르게 잡힌다.
- **제안**: 네 파일을 BOM 없는 UTF-8로 다시 저장한다.
- **확신도**: 높음

### 6. 🟢 락온 종료에서 이동 회전 플래그 복원이 무관한 ASC 유효성 검사에 묶여 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:104-124`
- **범주**: 설계/구조
- **문제**: `SavedOrientRotationToMovement` 복원은 캐릭터 이동 컴포넌트만 건드리는데 `ActorInfo->AbilitySystemComponent.IsValid()` 게이트 안에 들어 있다. 그 게이트가 필요한 것은 바로 위의 락온 컴포넌트 해제뿐이다. 실제로 ASC가 무효한 종료는 거의 없어 현재 버그로 드러나지는 않지만, 조건과 대상이 어긋나 있어 "왜 여기 있는지"를 읽는 사람이 매번 다시 추적해야 한다.
- **제안**: 복원 블록을 게이트 밖으로 빼고 `SavedOrientRotationToMovement.IsSet()`과 아바타 유효성만 보게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`(`WxCore` 외 Wx 참조 없음 확인), `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE·MMC 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, 대응 헤더 전체
- **미검토 / 한계**:
  - BP·DataTable 저작 값(GA_* / GE_* / DT_Damage·DT_Effect·DT_Ability 행, 몽타주 노티파이 배치)은 범위 밖이다. 이 모듈의 규약 상당수 — 스킬 슬롯별 쿨다운 GE 지정, 콤보 창·후딜 노티파이 배치, `WxAbility_Skill`/`_Pattern`의 슬롯 애셋 태그 — 가 저작 쪽에 걸려 있어 C++만으로는 실제 구성이 규약을 지키는지 확인하지 못했다. 2번 지적은 "저작이 어긋날 수 있다"까지만 검증했고 실제로 어긋난 에셋이 있는지는 확인하지 않았다.
  - 멀티플레이 실측이 없다. 예측·권위 관련 지적(1번)은 엔진 소스(`ApplyGameplayEffectSpecToSelf`의 Instant 경로, `HasNetworkAuthorityToApplyGameplayEffect`)를 따라간 코드 경로 추론이며, 단일 플레이·리슨 서버 호스트에서는 증상이 드러나지 않는다.
  - 커밋되지 않은 변경분은 모두 읽었고 결함을 찾지 못했다. Minion 로스터를 주인별 맵에서 월드 전역 배열 + 질의 시점 `GetMaster` 파생으로 바꾼 재설계는 스냅샷 순회로 파괴 중 재진입을 피하고 있고(`WxMinionSubsystem.cpp:44-54`, `177-185`), 복제 스폰 시점에 `Instigator`가 비어 있다는 전제도 엔진 동작(`OnActorSpawned`가 `FinishSpawning`에서 발화)과 일치한다. 투사체 되돌림의 `Reflect(APawn&)` 참조 승격도 오버랩 핸들러의 `IsHostile(GetInstigator(), ...)`가 널 인스티게이터를 걸러 주므로 안전하다. 무기 탐색을 `UChildActorComponent` 기준으로 바꾼 것은 `AWxCharacterBase`의 `WeaponActor` 슬롯(`Source/WxGame/Character/WxCharacterBase.cpp:41`)과 맞고, 제거된 `SetLockedOn`을 대신하는 `State.LockedOn` 루즈 태그는 `WBP_Nameplate_Enemy`가 소비 중임을 확인했다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 181파일 — `/module-review`로 갱신*
