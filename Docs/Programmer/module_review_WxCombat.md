# WxCombat — 코드 리뷰

> GAS 순정 경로를 존중하며 잘 정리된 모듈이다. 태그 누수·콜백 레이스·콤보 재진입 같은 까다로운 실패 경로는 실패복구 코드와 주석으로 촘촘히 막혀 있고, 프로젝트 코딩·모듈 규칙 위반은 사실상 없다(WxCore 외 Wx 플러그인 참조 0건, 람다·인라인 정의 0건, `BlueprintCallable` 오용 0건, 델리게이트 콜백 `Handle` prefix 준수). 남은 지적은 대부분 네트워크 권위 경계·전역 상태 소유권·객체 수명에 몰려 있다. 이번 리뷰는 README의 진입점(ASC·AbilityBase·AttributeSet·대미지 파이프라인)을 축으로 무기/투사체·락온·AnimNotify·Effect/MMC·타게팅 필터까지 cpp 로직 레벨로 내려가 읽었고, 직전 리뷰(`14a77aef`) 지적은 전부 현재 코드에 재대조해 해소된 항목(타이머 콜백 네이밍·죽은 멤버·주석 코드 일부·`AWxEffectZone`)은 뺐다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 9 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 `HitActorsThisSwing`가 UPROPERTY 없이 UObject 포인터를 들고 있어 GC에 추적되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:107` (사용처 `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:197-203`, `:257`)
- **범주**: 버그/정확성
- **문제**: `TSet<TObjectPtr<AActor>> HitActorsThisSwing`에 `UPROPERTY()`가 없다. 리플렉션에 등록되지 않은 `TObjectPtr`은 참조 그래프에 들어가지 않아 GC가 원소를 살려주지도, 파괴 시 null로 바꿔주지도 않는다. 스윙이 열려 있는 동안 피격된 적이 파괴되고 그 사이 GC가 돌면 `Tick`의 `Params.AddIgnoredActor(AlreadyHit.Get())`가 죽은 포인터를 만진다. 바로 위 `if (AlreadyHit)`는 null 검사일 뿐 유효성 검사가 아니다. 같은 클래스의 `GripPoint`/`Mesh`/`HitCollision`에는 모두 UPROPERTY가 붙어 있어 단순 누락으로 보인다. 같은 유형이 `Public/Weapon/WxWeaponBase.h:104`(`FWxDamageInfo DamageInfo` — 내부 `AdditionalEffects`의 클래스 참조가 추적 밖)와 `Public/AbilitySystem/Task/WxAbilityTask_LockOnTarget.h:69`(`TSubclassOf<UUserWidget> ReticleWidgetClass`)에도 있다.
- **제안**: 세 멤버에 `UPROPERTY()`를 붙인다. 수명 소유가 부담되면 `HitActorsThisSwing`은 `TSet<TWeakObjectPtr<AActor>>`도 대안이다.
- **확신도**: 높음

### 2. 🟡 `UWxAbility_Guard::PlayMontage`가 널 몽타주에도 성공을 반환한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:136-162`
- **범주**: 버그/정확성
- **문제**: 페이즈 몽타주(`GuardBreakMontage`/`GuardCounterMontage`/`PerfectGuardMontage`/`GuardKnockbackMontage`)는 전부 선택적 `UPROPERTY`인데 `PlayMontage`가 널을 거르지 않는다. 널이면 `ActiveMontage = nullptr`로 세팅한 뒤(`:154`) 태스크가 `OnCancelled`를 즉시 브로드캐스트해 어빌리티가 끝나는데도 함수는 `true`를 반환한다(`:161`). 그 결과 `ActivateAbility`(`:93`)와 `HandleGuardHitReact`(`:242`)의 실패 분기가 타지 않고, 이미 종료된 어빌리티 위에 `ListenForGuardHit`/`ListenForPerfectGuard`/`ListenForCounterInput`이 계속 등록된다. 형제 클래스 `UWxAbility_Dodge::PlayMontage`(`WxAbility_Dodge.cpp:251-254`)는 같은 자리에서 널을 검사해 `false`를 반환한다. 페이즈 판정이 전부 `ActiveMontage == <선택적 몽타주>` 포인터 비교라 양쪽이 널이면 오탐하는 구조라는 점도 같은 뿌리다.
- **제안**: `Dodge`와 동일하게 함수 앞단에 `if (!Montage) { return false; }`를 둔다.
- **확신도**: 높음

### 3. 🟡 `PostAttributeChange`의 Max 어트리뷰트 비례 스케일이 클라이언트 복제 수신 경로에서도 실행된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:96-110`
- **범주**: 설계/구조
- **문제**: `PostAttributeChange`는 서버 전용 훅이 아니다. 엔진의 `FActiveGameplayEffectsContainer::SetBaseAttributeValueFromReplication` → `SetNumericAttribute_Internal` → `FGameplayAttribute::SetNumericValueChecked` 경로가 이 훅을 호출하므로, 클라이언트가 MaxHP/MaxSP/MaxMP 복제를 수신할 때마다 `SetHP/SetSP/SetMP`(내부적으로 `SetNumericAttributeBase`)로 서버 권위 어트리뷰트를 로컬 계산으로 덮어쓴다. 복제 경로는 "old 값으로 되감기 → new 적용" 2단이라 한 번의 복제에 두 번 불릴 수도 있다. 바로 아래 DP 분기(`:111-126`)는 정확히 같은 이유로 `ASC->IsOwnerActorAuthoritative()`를 명시 게이팅해 두었으므로 Max 분기만 빠진 것으로 보인다.
- **제안**: 세 Max 분기도 DP 분기와 동일하게 권위 게이트를 건다.
- **확신도**: 중간

### 4. 🟡 `OnRep_LockOnTarget`이 `State.LockedOn` 태그 이관을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:56-60` (비교 대상 `:62-95`)
- **범주**: 버그/정확성
- **문제**: 대상 표시 태그의 부여/회수는 `ApplyLockOnTarget` 안에만 있는데, 복제 수신 경로는 네트 레이어가 `LockOnTarget`을 이미 대입한 뒤(=이전 값 유실) `OnRep_LockOnTarget`이 브로드캐스트만 한다. 소유 클라가 A를 예측했는데 서버가 B로 정정해 복제하면 A의 `State.LockedOn`이 영구히 남고 B에는 붙지 않는다. 로컬 예측과 복제 정합이 서로 다른 코드 경로를 타는 비대칭이다.
- **제안**: 태그 이관을 "이전 값 → 새 값" 함수로 분리해 `ApplyLockOnTarget`과 `OnRep_LockOnTarget`이 같은 경로를 타게 한다(OnRep용 직전 값 캐시 필요).
- **확신도**: 중간

### 5. 🟡 무기·투사체 히트에 팀 판정이 전혀 없다 (헤더 주석은 팀 체크를 약속한다)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:97`, `Private/Weapon/WxWeaponBase.cpp:251-266`, `Private/Weapon/WxProjectileBase.cpp:93-144`
- **범주**: 버그/정확성
- **문제**: `ProcessHit`의 선언 주석은 "히트 검증/팀 체크/GE 적용/HitStop을 수행"이라고 적었지만 구현에 팀 판정이 없다. `HitCollision`은 `ECC_Pawn` 전체에 Overlap이고(`WxWeaponBase.cpp:31`), 대미지 GE인 `UWxEffect_Damage`도 `State.Dead` IgnoreTags만 둔다(`Private/AbilitySystem/Effect/WxEffect_Damage.cpp:17-19`). 투사체도 소유자/인스티게이터만 제외한다(`WxProjectileBase.cpp:95`). 결과적으로 AI의 광역 스윙이 다른 AI를, 멀티에서 플레이어 공격이 아군을 그대로 때린다. 모듈 안에 `UWxTargetingFilterTask_Team`이 이미 있는데 이 경로만 쓰지 않아 판정 모델이 갈린다.
- **제안**: `ProcessHit`/투사체 히트에서 `IGenericTeamAgentInterface::GetTeamAttitudeTowards`로 거르거나, 대미지 GE에 팀 조건을 얹는다. 프리 포 올이 의도라면 헤더 주석을 정정한다.
- **확신도**: 중간(의도된 설계일 수 있으나 주석과 어긋난다)

### 6. 🟡 무기 스윕이 비권위 머신에서도 매 틱 돌지만 결과는 전부 버려진다 (주석이 사실과 다름)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:170-217`, `:253-254`, `Private/WxCombatLibrary.cpp:33-34`
- **범주**: 성능/안전
- **문제**: `ANS_WeaponAttack`은 몽타주가 재생되는 모든 머신에서 실행되므로 클라이언트에서도 `BeginAttack` → 틱 활성 → 매 프레임 `SweepMultiByObjectType` + 히트마다 `MakeSpecs`가 돈다. 그런데 `UWxCombatLibrary::ApplyDamage`가 부르는 `Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target)`은 예측키 인자를 넘기지 않아 엔진 기본값 `FPredictionKey()`(무효)를 쓰고, `HasNetworkAuthorityToApplyGameplayEffect`는 `IsOwnerActorAuthoritative() || PredictionKey.IsValidForMorePrediction()`이므로 비권위 머신에서는 **예측이 아니라 무조건 no-op**이다. 그럼에도 `ApplyDamage`는 `bAppliedAny = true`를 반환하고 `HitActorsThisSwing`에는 대상이 기록된다. `WxWeaponBase.cpp:253-254`의 주석("클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며 … GAS가 자동으로 롤백한다")은 사실과 다르다 — 애님 노티파이는 어빌리티 활성화 스코프 밖이라 그 시점 `ScopedPredictionKey`가 유효하지 않다. 반면 `WxAnimNotify_AreaAttack.cpp:20`과 `WxProjectileBase.cpp:103`은 권위 게이팅이 돼 있어 모듈 안에서 모델이 갈린다.
- **제안**: 예측을 실제로 도입하든(유효 예측 키를 `ApplyDamage`까지 전달), 클라 판정을 포기하고 `BeginAttack`/`Tick`을 권위 게이팅하든 하나로 정한다. 어느 쪽이든 주석과 `bAppliedAny` 반환 의미를 실제 동작에 맞춘다.
- **확신도**: 높음

### 7. 🟡 전역 타임 딜레이션이 "마지막 요청자 승"이라 진행 중인 컷신의 시간축이 깨진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:50-55`, `Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:96-99`
- **범주**: 설계/구조
- **문제**: `SetDilationFrom`은 값이 같아도 소유권을 새 요청자에게 넘기고, `ClearDilationFrom`은 소유자가 다르면 무시한다 — 겹친 요청이 스택이 아니라 덮어쓰기다. 그런데 컷신 태스크는 진입 시점 배율로 `SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation)`(기본 0.001 → 1000배)를 한 번만 고정한다. 컷신이 도는 동안 다른 액터의 슬로우 요청(퍼펙트 가드 `WxAbility_Guard.cpp:275`, 극한 회피 `WxAbility_Dodge.cpp:292`)이 소유권을 가져가 월드 배율을 0.4로 올리면 시퀀스가 400배속으로 순식간에 끝난다. 컴포넌트가 GameState에 붙은 전역 상태라 요청자가 서로 다른 폰이어도 같은 값을 다툰다.
- **제안**: 요청을 참조 카운트/스택으로 쌓아 해제 시 이전 요청으로 되돌리거나, 컷신 재생 속도를 고정값이 아니라 현재 배율을 매 프레임 반영하도록 바꾼다.
- **확신도**: 중간(겹침 순서에 의존하지만 경로는 코드로 확인됨)

### 8. 🟡 `OnGranted` 자동 활성화가 클라이언트에서도 조건 없이 돌고 무검사 역참조를 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:146-154`
- **범주**: 설계/구조
- **문제**: `OnGiveAbility`는 서버의 `GiveAbility`뿐 아니라 클라이언트가 어빌리티 스펙을 복제받을 때도 호출된다. 여기서 조건 없이 `ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)`를 부르므로, 베이스 기본값인 `LocalPredicted` 패시브는 클라이언트에서 예측 활성화 → 서버 RPC → 서버는 이미 활성이라 거부 → 롤백을 밟고, 그 사이 `ActivationOwnedTags`가 붙었다 떨어지는 깜빡임이 생긴다. `Spec.IsActive()` 검사도, `ActorInfo`/`ActorInfo->AbilitySystemComponent`(약참조) 유효성 검사도 없다.
- **제안**: 조건을 `ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && !Spec.IsActive()`로 좁히고, 예측 활성화가 불필요하면 권위 게이트를 추가한다.
- **확신도**: 중간

### 9. 🟡 데미지 ExecCalc가 순수 계산이 아니라 부수효과 허브다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:52-178`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어 산출 외에 (a) 다른 ASC에 GE 적용(`:137`), (b) 소스 ASC 어트리뷰트 직접 쓰기(`:210` `SetNumericAttributeBase` — 스펙·컨텍스트·면역·복제 경로를 우회하면서 결과는 "그로기 유발"이라는 큰 판정으로 이어진다), (c) 대상 어빌리티 취소(`:330`), (d) GameplayEvent 동기 송출(`:93`/`:101`/`:175`/`:351`)과 Cue 실행(`:156`/`:160`)을 모두 수행한다. 전부 대상 ASC의 GE 실행 스코프 안에서 벌어져 재진입 적용이 일어난다. 실제로 `UWxAbility_Guard::HandleGuardHitReact`(`WxAbility_Guard.cpp:231-233`)는 "이벤트 수신 시점의 `GetSP()`는 차감 적용 전 값"이라는 이 실행 순서에 명시적으로 의존하고 있어, 순서가 바뀌면 가드 브레이크 판정이 조용히 틀어진다. 같은 함수에서 회복은 GE 경로, 반사는 직접 쓰기로 방식이 엇갈리는 점, 크리 판정에 `FMath::FRand()`(`:262`)를 써서 비결정적인 점도 함께 걸린다.
- **제안**: 최소한 반사 DP를 회복과 동일하게 Instant GE로 통일한다. 이벤트/Cue/GE 적용을 GE 적용 확정 이후 훅(`PostGameplayEffectExecute` 등)으로 밀어내는 방향도 함께 검토한다.
- **확신도**: 낮음(의도된 단축일 수 있음 — 현재 가드/패링 타이밍이 이 순서에 맞춰져 있다)

### 10. 🟢 `UWxCombatLibrary::ApplyDamage`의 문서가 약속한 자기 피격 방지가 구현에 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h:27` vs `Private/WxCombatLibrary.cpp:10-13`
- **범주**: 버그/정확성
- **문제**: 주석은 "Source/Target ASC 가 nullptr 이거나 **둘이 동일하면** false 반환"이라고 명시하지만 구현은 `if (!Source || !Target)`만 검사한다. 이 함수는 무기/투사체 밖 경로의 단일 진입점이라, TargetingPreset 결과를 그대로 흘리는 `WxAnimNotify_AreaAttack`(`Private/AnimNotify/WxAnimNotify_AreaAttack.cpp:90`) 같은 호출부에서 프리셋이 시전자를 제외하지 않으면 자기 자신에게 대미지가 들어간다.
- **제안**: `Source == Target` 조기 반환을 추가하거나(문서대로), 해당 문장을 주석에서 지워 계약을 실제와 맞춘다.
- **확신도**: 높음(불일치 자체), 중간(실제 노출 여부는 프리셋 데이터에 달림)

### 11. 🟢 `WxAnimNotify_AreaAttack`의 무의미한 삼항 분기
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaAttack.cpp:76`
- **범주**: 중복/복잡도
- **문제**: `Context.AddInstigator(OwnerPawn ? static_cast<AActor*>(OwnerPawn) : Owner, Owner)` — `OwnerPawn`은 `Cast<APawn>(Owner)`(`:57`)이므로 캐스트가 성공하면 `OwnerPawn == Owner`, 실패하면 폴백도 `Owner`다. 두 분기 결과가 항상 같아 `OwnerPawn` 지역변수 자체가 무의미하다.
- **제안**: `Context.AddInstigator(Owner, Owner)`로 줄이고 `OwnerPawn`을 제거한다.
- **확신도**: 높음

### 12. 🟢 규칙 위반: `UWxAbilityTask_WaitInputActionTriggered::Activate()`가 `Super::Activate()`를 호출하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitInputActionTriggered.cpp:23-33`
- **범주**: 규칙 위반
- **문제**: 프로젝트 규칙(override에서 `Super::` 호출)에 어긋난다. 같은 폴더의 `WxAbilityTask_SlowTime.cpp:37`, `WxAbilityTask_PlaySkillCutscene.cpp:31`, `WxAbilityTask_LockOnTarget.cpp:137`은 모두 호출하고 있어 이 파일만 예외다. 현재 `UAbilityTask::Activate()` 기본 구현이 비어 있어 동작 영향은 없지만 엔진 업데이트에 취약하다.
- **제안**: 함수 첫 줄(조기 `EndTask()` 경로보다 앞)에 `Super::Activate();`를 추가한다.
- **확신도**: 높음

### 13. 🟢 널 검사·정리 순서·상태 복원의 국지적 불일치
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:137`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp:117`, `Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:54-62`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:45`·`:103`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:95`, `Private/AnimNotify/WxAnimNotifyState_{ComboWindow,Invincible,PerfectGuard,WeaponAttack}.cpp`
- **범주**: 버그/정확성
- **문제**:
  (a) `SetLastPressedInputAction`이 `GetOwnerActor()->HasAuthority()`로 널 검사 없이 역참조한다.
  (b) `UWxAbility_Guard::EndAbility`가 `ActorInfo->`를 검사 없이 역참조하는데, 같은 위치에서 `Dodge`(`WxAbility_Dodge.cpp:112`)·`HitReact`(`WxAbility_HitReact.cpp:134`)·`Finisher`(`WxAbility_Finisher.cpp:131`)·`Groggy`는 모두 `ActorInfo`를 먼저 확인한다.
  (c) `UWxAbility_Sprint::EndAbility`만 `Super::EndAbility`를 먼저 부르고 그 뒤에 GE를 회수한다(모듈의 다른 모든 오버라이드는 "정리 → Super"). Super 안에서 태스크 종료·`OnGameplayAbilityEnded` 브로드캐스트가 먼저 돌아 다른 어빌리티가 곧바로 활성화될 수 있다.
  (d) `WxAbility_LockOn`은 `GetCharacterMovement()` 널 검사 없이 `bOrientRotationToMovement`를 쓰고, 종료 시 저장값이 아니라 `true`로 하드코딩 복원한다(현재 `AWxCharacterBase`의 기본값이 `true`라 문제가 드러나지 않지만, `WxAIPerceptionComponent`처럼 같은 플래그를 끄고 쓰는 코드와 겹치면 조용히 덮어쓴다). `WxAbility_HitReact.cpp:95`의 `GetCharacterMovement()->JumpZVelocity`도 같은 종류의 무검사 역참조다.
  (e) AnimNotify 4종이 `MeshComp->GetOwner()`를 널 검사 없이 부른다(`ComboWindow`/`Invincible`/`PerfectGuard`는 `:12`·`:25`, `WeaponAttack`은 `:16`·`:32`). 같은 폴더의 나머지 6개 노티파이는 전부 `MeshComp` 널을 먼저 막는다. 특히 앞 셋은 루즈 태그를 Begin/End 쌍으로 붙였다 떼는 구조라 한쪽만 조기 이탈하면 태그가 누수된다.
- **제안**: 형제 코드에 맞춰 검사·순서·복원 방식을 통일한다.
- **확신도**: 중간

### 14. 🟢 구현과 어긋난 낡은 주석·주석 처리된 코드
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:212-213`, `Private/AbilitySystem/Ability/WxAbility_Skill.cpp:11`·`:21`
- **범주**: 중복/복잡도
- **문제**: Finisher의 `ApplyFinisherDamage` 주석은 "앞잡 그로기 해제(DP 0)는 피해자의 앞잡 짝 피격 몽타주 종료 시 `WxAbility_HitReact`가 처리한다"고 적었지만, 실제로는 같은 파일 `HandleFinisherMontageCompleted`(`:148-168`)에서 공격자가 `UWxEffect_ResetDP`로 처리한다. Skill 생성자에는 주석 처리된 코드(`//AssetTags.AddTag(WxGameplayTags::Ability_Skill_@);`)가 남아 있고, `:21`의 "입력 태그(Input.Skill.1~4)는 AbilitySet 항목의 InputTag로 지정한다"는 현재 구조와 맞지 않는다 — `UWxAbilitySet`에 InputTag 개념이 없고(`Public/AbilitySystem/WxAbilitySet.h:54-59`) 입력 라우팅 키는 `UWxAbilityBase::ActivationInputAction`이 보유한다.
- **제안**: 주석 코드는 제거하고 두 주석을 현재 구현에 맞춰 갱신한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/Targeting/WxLockOnManagerComponent.cpp`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/WxCombatLibrary.cpp`, `Private/WxDamageInfo.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Private/AbilitySystem/Effect/` 전체(GE·MMC·`WxExecCalc_Burn`), `Private/AbilitySystem/Cue/` 전체, `Private/Targeting/` 전체(필터 태스크·`WxRootMotionModifier_SnapToTarget`·`WxLockOnPointComponent`), `Private/AbilitySystem/Task/` 나머지, `Private/AbilitySystem/Ability/` 나머지(HitReact·Skill·Ultimate·Pattern·Death·Sprint), `Private/AbilitySystem/WxAbilitySet.cpp`, `Public/` 헤더 전량(선언·UPROPERTY 스캔), `WxCombat.Build.cs`, `WxCombat.uplugin`
- **규칙 준수 확인(위반 없음)**: 145개 소스 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.`, `Wx` prefix 준수, 람다 0건, `FORCEINLINE`/인라인 정의 0건, 델리게이트 바인딩 콜백 전부 `Handle` prefix(직전 리뷰 지적분 해소 확인), `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 하나(Blueprint Function Library), Build.cs·uplugin 의존은 Wx 플러그인 중 `WxCore`뿐. 유일한 `Super::` 미호출은 항목 12.
- **미검토 / 한계**:
  - 데이터 자산은 보지 않았다 — `FWxAbilityTableRow`/`FWxDamageTableRow`/`FWxCombatAttributeInitTableRow` DataTable 실제 값, GE·어빌리티 BP 서브클래스 디폴트(예: 스킬 BP가 `Ability.Skill` 애셋 태그를 실제로 지정하는지 — 미지정 시 `WxAbility_HitReact`/`WxAbility_Finisher`의 `CancelAbilitiesWithTag`가 동작하지 않는다), TargetingPreset 구성은 범위 밖이다.
  - 리플리케이션 관련 지적(3·4·5·6·8)은 정적 분석 기반이며 데디케이티드 서버 세션으로 재현 검증하지 않았다.
  - `UWxAbilityBase::GetCooldownGameplayEffect`(`WxAbilityBase.cpp:176-182`)가 다중 충전 어빌리티에서 런타임 `NewObject<UWxEffect_Cooldown>`을 GE Def로 반환한다. CDO가 아닌 객체는 네트워크 주소가 안정적이지 않아 복제되는 `FGameplayEffectSpec::Def`가 원격에서 해소되지 않을 소지가 있어 보였으나 재현하지 못해 발견으로 올리지 않았다 — 다중 충전 스킬을 멀티에서 검증할 때 함께 확인할 것.
  - `UWxAbility_Groggy`의 0.1초 몽타주 폴링(`WxAbility_Groggy.cpp:69`, `:174-193`)은 의도된 설계로 알고 있어 지적하지 않았다.
  - `UWxAnimNotifyState_CameraMove`의 에디터 프리뷰 경로, `UWxAbilityTask_PlaySkillCutscene`의 시퀀스 바인딩(`SetBindingByTag`), GameplayCue의 데디케이티드 서버 실행 여부는 코드만 읽고 동작 확인은 하지 않았다.

---
*문서 기준 커밋 `1e9b745c` · 리뷰일 2026-08-05 · 소스 145파일 — `/module-review`로 갱신*
