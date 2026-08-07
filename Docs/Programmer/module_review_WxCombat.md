# WxCombat — 코드 리뷰

> GAS 순정 경로를 존중하며 잘 정리된 모듈이다. 태그 누수·콜백 레이스·콤보 재진입 같은 까다로운 실패 경로는 실패복구 코드와 주석으로 촘촘히 막혀 있고, 프로젝트 코딩·모듈 규칙 위반은 이번 리뷰 기준 0건이다(WxCore 외 Wx 플러그인 참조 0, 람다 0, `FORCEINLINE`/인라인 정의 0, `BlueprintCallable` 1건은 Blueprint Function Library, 델리게이트 콜백 32개 전부 `Handle` prefix, `Super::` 미호출 0). 남은 지적은 전부 네트워크 권위 경계·전역 상태 소유권·주석과 구현의 불일치에 몰려 있다. 이번 리뷰는 README 진입점(ASC·AbilityBase·AttributeSet·대미지 파이프라인)을 축으로 무기/투사체·락온·시간감속·AnimNotify·Effect/MMC·타게팅 필터까지 cpp 로직 레벨로 내려가 읽었고, 직전 리뷰(`1e9b745c`) 지적은 전부 현재 코드에 재대조해 해소된 항목(`HitActorsThisSwing`/`DamageInfo`/`ReticleWidgetClass`의 UPROPERTY 누락, Guard의 널 몽타주 성공 반환, `WaitInputActionTriggered`의 `Super::Activate` 누락, `ApplyDamage` 주석의 자기 피격 계약, 삭제된 `WxAnimNotify_AreaAttack` 관련 항목)은 뺐다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 8 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `PostAttributeChange`의 Max 어트리뷰트 비례 스케일이 클라이언트 복제 수신 경로에서도 실행된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:116-130`
- **범주**: 설계/구조
- **문제**: `PostAttributeChange`는 서버 전용 훅이 아니다. 엔진의 `FActiveGameplayEffectsContainer::SetBaseAttributeValueFromReplication` → `SetNumericAttribute_Internal` 경로가 이 훅을 호출하므로, 클라이언트가 MaxHP/MaxSP/MaxMP 복제를 수신할 때마다 `SetHP/SetSP/SetMP`로 서버 권위 어트리뷰트를 로컬 계산으로 덮어쓴다. 복제 경로는 "old 값으로 되감기 → new 적용" 2단이라 한 번의 복제에 두 번 불릴 수도 있다. 바로 아래 DP 분기(`:131-146`)는 정확히 같은 이유로 `ASC->IsOwnerActorAuthoritative()`를 명시 게이팅해 두었으므로 Max 3분기만 빠진 것으로 보인다.
- **제안**: 세 Max 분기도 DP 분기와 동일하게 권위 게이트를 건다.
- **확신도**: 중간

### 2. 🟡 `OnRep_LockOnTarget`이 `State.LockedOn` 태그 이관을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:56-60` (비교 대상 `:62-95`)
- **범주**: 버그/정확성
- **문제**: 대상 표시 태그의 부여/회수는 `ApplyLockOnTarget`(`:69-89`) 안에만 있는데, 복제 수신 경로는 네트 레이어가 `LockOnTarget`을 이미 대입한 뒤(=이전 값 유실) `OnRep_LockOnTarget`이 브로드캐스트만 한다. 소유 클라가 A를 예측했는데 서버가 B로 정정해 복제하면 A의 `State.LockedOn`이 영구히 남고 B에는 붙지 않는다. 로컬 예측과 복제 정합이 서로 다른 코드 경로를 타는 비대칭이다.
- **제안**: 태그 이관을 "이전 값 → 새 값" 함수로 분리해 `ApplyLockOnTarget`과 `OnRep_LockOnTarget`이 같은 경로를 타게 한다(OnRep용 직전 값 캐시 필요).
- **확신도**: 중간

### 3. 🟡 무기·투사체 히트에 팀 판정이 전혀 없다 (헤더 주석은 팀 체크를 약속한다)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:96`, `Private/Weapon/WxWeaponBase.cpp:238-253`, `Private/Weapon/WxProjectileBase.cpp:93-144`
- **범주**: 버그/정확성
- **문제**: `ProcessHit`의 선언 주석은 "히트 검증/팀 체크/GE 적용/HitStop을 수행"이라고 적었지만 구현에 팀 판정이 없다. `HitCollision`은 `ECC_Pawn` 전체에 Overlap이고(`WxWeaponBase.cpp:31`), 대미지 GE인 `UWxEffect_Damage`도 `State.Dead` IgnoreTags만 둔다(`Private/AbilitySystem/Effect/WxEffect_Damage.cpp:16-19`). 투사체는 소유자/인스티게이터만 제외한다(`WxProjectileBase.cpp:95`). 결과적으로 AI의 광역 스윙이 다른 AI를, 멀티에서 플레이어 공격이 아군을 그대로 때린다. 모듈 안에 `UWxTargetingFilterTask_Team`(`Private/Targeting/WxTargetingFilterTask_Team.cpp`)이 이미 있는데 이 경로만 쓰지 않아 판정 모델이 갈린다.
- **제안**: `ProcessHit`/투사체 히트에서 `IGenericTeamAgentInterface::GetTeamAttitudeTowards`로 거르거나, 대미지 GE에 팀 조건을 얹는다. 프리 포 올이 의도라면 헤더 주석을 정정한다.
- **확신도**: 중간(의도된 설계일 수 있으나 주석과 어긋난다)

### 4. 🟡 무기 스윕이 비권위 머신에서도 매 틱 돌지만 결과는 전부 버려진다 (주석이 사실과 다름)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:157-204`, `:240-241`, `Private/WxCombatLibrary.cpp:33-34`
- **범주**: 성능/안전
- **문제**: `ANS_WeaponAttack`은 몽타주가 재생되는 모든 머신에서 실행되므로 클라이언트에서도 `BeginAttack` → 틱 활성 → 매 프레임 `SweepMultiByObjectType` + 히트마다 `MakeSpecs`가 돈다. 그런데 `UWxCombatLibrary::ApplyDamage`가 부르는 `Source->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), Target)`은 예측키 인자를 넘기지 않아 엔진 기본값 `FPredictionKey()`(무효)를 쓰고, `HasNetworkAuthorityToApplyGameplayEffect`는 `IsOwnerActorAuthoritative() || PredictionKey.IsValidForMorePrediction()`이므로 비권위 머신에서는 **예측이 아니라 무조건 no-op**이다. 그럼에도 `ApplyDamage`는 `bAppliedAny = true`를 반환하고 `HitActorsThisSwing`에는 대상이 기록된다. `WxWeaponBase.cpp:240-241`의 주석("클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며 … GAS가 자동으로 롤백한다")은 사실과 다르다 — 애님 노티파이는 어빌리티 활성화 스코프 밖이라 그 시점 `ScopedPredictionKey`가 유효하지 않다. 반면 투사체(`WxProjectileBase.cpp:102-106`)와 투사체 스폰(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:86-90`)은 권위 게이팅이 돼 있어 모듈 안에서 모델이 갈린다.
- **제안**: 예측을 실제로 도입하든(유효 예측 키를 `ApplyDamage`까지 전달), 클라 판정을 포기하고 `BeginAttack`/`Tick`을 권위 게이팅하든 하나로 정한다. 어느 쪽이든 주석과 `bAppliedAny` 반환 의미를 실제 동작에 맞춘다.
- **확신도**: 높음

### 5. 🟡 컷신 PlayRate가 진입 시점 배율로 고정돼, 원격 클라이언트에서 궁극기 컷신이 한두 프레임에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:51`·`:95-99`, `Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:55`
- **범주**: 버그/정확성
- **문제**: 태스크는 `SetGlobalTimeDilationAuthoritative`로 월드를 느리게 만든 뒤 `SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation)`(궁극기 기본값 0.001 → **1000배**)를 한 번만 고정한다. 그런데 `SetDilationFrom`은 권위 게이트가 걸려 있어(`Private/Time/WxTimeDilationComponent.cpp:37-41`) 원격 클라에서는 즉시 no-op이고, 실제 감속은 `ReplicatedTimeDilation` 복제가 도착한 뒤 `OnRep`(`:130-133`)에서야 적용된다. `UWxAbility_Ultimate`는 베이스 기본값인 `LocalPredicted`(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:23`)라 이 태스크가 소유 클라에서도 돌므로, 복제가 오기 전 프레임 동안 시퀀스만 1000배속으로 진행된다. 60fps 기준 한 프레임에 약 16초 분량이 흘러가 컷신이 사실상 통째로 스킵된다. 같은 고정 배율은 리슨 서버에서도 문제인데, 컷신이 도는 중 다른 액터의 슬로우 요청(퍼펙트 가드 `Private/AbilitySystem/Ability/WxAbility_Guard.cpp:281`, 극한 회피 `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:292`)이 월드 배율을 0.4로 올리면 시퀀스가 400배속이 된다.
- **제안**: PlayRate를 진입 시점 고정값이 아니라 현재 월드 배율을 매 프레임 반영하도록 바꾸거나(태스크를 틱시켜 `GetGlobalTimeDilation` 역수 갱신), 시퀀스를 월드 딜레이션에서 분리해(언디레이트 틱) 배율 보정 자체를 없앤다.
- **확신도**: 중간(정적 분석 기반, 데디케이티드/리슨 세션 재현은 하지 않음)

### 6. 🟡 전역 타임 딜레이션 요청이 스택이 아니라 "마지막 요청자 승" 덮어쓰기다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:50-56`, `:66-77`
- **범주**: 설계/구조
- **문제**: `SetDilationFrom`은 값이 같아도 소유권을 새 요청자에게 넘기고(`:50-52`), `ClearDilationFrom`은 소유자가 다르면 무시한다(`:68-71`). 겹친 요청이 스택이 아니라 덮어쓰기이므로, 긴 슬로우 A가 진행 중일 때 짧은 슬로우 B가 소유권을 가져가면 B가 끝나는 순간 월드가 1.0으로 복귀하고 A의 남은 구간은 통째로 사라진다(A의 뒤늦은 `ClearDilationFrom`도 소유자가 이미 비어 있어 그냥 1.0을 재설정한다). 컴포넌트가 GameState에 붙은 전역 상태라 요청자가 서로 다른 폰이어도 같은 값을 다툰다 — 멀티에서 두 플레이어가 각각 극한 회피/퍼펙트 가드를 겹쳐 쓰면 바로 재현되는 구조다.
- **제안**: 요청을 참조 카운트/스택으로 쌓아 해제 시 남아 있는 요청 중 가장 강한(또는 가장 최근) 값으로 되돌린다.
- **확신도**: 중간(겹침 순서에 의존하지만 경로는 코드로 확인됨)

### 7. 🟡 `OnGranted` 자동 활성화가 클라이언트에서도 조건 없이 돌고 무검사 역참조를 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:146-154`
- **범주**: 설계/구조
- **문제**: `OnGiveAbility`는 서버의 `GiveAbility`뿐 아니라 클라이언트가 어빌리티 스펙을 복제받을 때도 호출된다. 여기서 조건 없이 `ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)`를 부르므로, 베이스 기본값인 `LocalPredicted`(`:23`) 패시브는 클라이언트에서 예측 활성화 → 서버 RPC → 서버는 이미 활성이라 거부 → 롤백을 밟고, 그 사이 `ActivationOwnedTags`가 붙었다 떨어지는 깜빡임이 생긴다. `Spec.IsActive()` 검사도, `ActorInfo`/`ActorInfo->AbilitySystemComponent`(약참조) 유효성 검사도 없다.
- **제안**: 조건을 `ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && !Spec.IsActive()`로 좁히고, 예측 활성화가 불필요하면 권위 게이트를 추가한다.
- **확신도**: 중간

### 8. 🟡 데미지 ExecCalc가 순수 계산이 아니라 부수효과 허브다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:52-178`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어 산출 외에 (a) 다른 ASC에 GE 적용(`:137`), (b) 소스 ASC 어트리뷰트 직접 쓰기(`:210` `SetNumericAttributeBase` — 스펙·컨텍스트·면역·복제 경로를 우회하면서 결과는 "그로기 유발"이라는 큰 판정으로 이어진다), (c) 대상 어빌리티 취소(`:330`), (d) GameplayEvent 동기 송출(`:93`/`:101`/`:175`/`:351`)과 Cue 실행(`:156`/`:160`)을 모두 수행한다. 전부 대상 ASC의 GE 실행 스코프 안에서 벌어져 재진입 적용이 일어난다. 실제로 `UWxAbility_Guard::HandleGuardHitReact`(`Private/AbilitySystem/Ability/WxAbility_Guard.cpp:237-239`)는 "이벤트 수신 시점의 `GetSP()`는 차감 적용 전 값"이라는 이 실행 순서에 명시적으로 의존하고 있어, 순서가 바뀌면 가드 브레이크 판정이 조용히 틀어진다. 같은 함수에서 회복은 GE 경로, 반사는 직접 쓰기로 방식이 엇갈리는 점도 함께 걸린다.
- **제안**: 최소한 반사 DP를 회복과 동일하게 Instant GE로 통일한다. 이벤트/Cue/GE 적용을 GE 적용 확정 이후 훅(`PostGameplayEffectExecute` 등)으로 밀어내는 방향도 함께 검토한다.
- **확신도**: 낮음(의도된 단축일 수 있음 — 현재 가드/패링 타이밍이 이 순서에 맞춰져 있다)

### 9. 🟢 AbilitySet 부여에 재진입 방어가 없고, 유일한 해제 경로가 데드 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-34`·`:44-60`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp:16-24`, `Public/AbilitySystem/WxAbilitySystemComponent.h:65`
- **범주**: 중복/복잡도
- **문제**: `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`은 저장소 전체에서 호출부가 없다. 그 핸들을 담으려고 존재하는 멤버 `AbilitySetGrantedHandles`(`WxAbilitySystemComponent.h:65`)도 채워지기만 하고 읽히지 않으므로, 부여는 있고 회수는 없는 편도 API다. 함께 걸리는 것이 `GiveToAbilitySystem`의 어트리뷰트 초기화 순서로, HP를 MaxHP보다 먼저 세팅한다(`:46-47`, SP/MP/DP/UP도 동일). 첫 부여에서는 MaxHP가 0이라 `PreAttributeChange`의 클램프도 `PostAttributeChange`의 비례 스케일(`Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:116-130`)도 건너뛰어 문제가 드러나지 않지만, 재소유(재부여)가 한 번이라도 일어나면 HP가 이전 MaxHP로 클램프된 뒤 MaxHP 변경이 비례 스케일을 태워 Row가 지정한 값과 다른 HP가 남는다. 중복 부여 자체를 막는 가드도 없다.
- **제안**: 해제 경로를 실제로 쓰거나(재부여 전 `RemoveFromAbilitySystem` 호출) 쓰지 않을 거면 구조체·멤버를 제거한다. 남긴다면 Max를 먼저 세팅하도록 순서를 뒤집고, 이미 부여된 상태면 조기 반환하는 가드를 둔다.
- **확신도**: 높음(데드 코드), 중간(재부여 시 어트리뷰트 오차 — 현재 호출부는 `PossessedBy` 1회뿐이라 노출되지 않는다)

### 10. 🟢 널 검사·정리 순서·상태 복원의 국지적 불일치
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:137`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp:117`, `Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:50-56`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:45`·`:103`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:95`·`:172`·`:202`, `Private/AnimNotify/WxAnimNotifyState_{ComboWindow,Invincible,PerfectGuard,WeaponAttack}.cpp`
- **범주**: 버그/정확성
- **문제**:
  (a) `SetLastPressedInputAction`이 `GetOwnerActor()->HasAuthority()`로 널 검사 없이 역참조한다.
  (b) `UWxAbility_Guard::EndAbility`가 `ActorInfo->`를 검사 없이 역참조하는데, 같은 위치에서 `Dodge`(`WxAbility_Dodge.cpp:112`)·`HitReact`(`WxAbility_HitReact.cpp:134`)·`Finisher`(`WxAbility_Finisher.cpp:131`)·`Groggy`(`WxAbility_Groggy.cpp:92`)는 모두 `ActorInfo`를 먼저 확인한다.
  (c) `UWxAbility_Sprint::EndAbility`만 `Super::EndAbility`를 먼저 부르고 그 뒤에 GE를 회수한다(모듈의 다른 모든 오버라이드는 "정리 → Super"). Super 안에서 태스크 종료·`OnGameplayAbilityEnded` 브로드캐스트가 먼저 돌아 다른 어빌리티가 이동속도 GE가 아직 붙은 상태에서 활성화될 수 있다.
  (d) `WxAbility_LockOn`은 `GetCharacterMovement()` 널 검사 없이 `bOrientRotationToMovement`를 쓰고, 종료 시 저장값이 아니라 `true`로 하드코딩 복원한다(현재 `AWxCharacterBase`의 기본값이 `true`라 문제가 드러나지 않지만, 같은 플래그를 끄고 쓰는 코드와 겹치면 조용히 덮어쓴다). `WxAbility_HitReact`의 `GetCharacterMovement()->JumpZVelocity`(`:95`)와 `CurrentActorInfo->` 역참조(`:172`, `:202`)도 같은 종류다.
  (e) AnimNotify 4종이 `MeshComp->GetOwner()`를 널 검사 없이 부른다(`ComboWindow`/`Invincible`/`PerfectGuard`는 `:12`·`:25`, `WeaponAttack`은 `:16`·`:32`). 같은 폴더의 나머지 5개 노티파이는 전부 `MeshComp` 널을 먼저 막는다. 특히 앞 셋은 루즈 태그를 Begin/End 쌍으로 붙였다 떼는 구조라 한쪽만 조기 이탈하면 태그가 누수된다.
- **제안**: 형제 코드에 맞춰 검사·순서·복원 방식을 통일한다.
- **확신도**: 중간

### 11. 🟢 구현과 어긋난 낡은 주석·주석 처리된 코드
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:213`, `Private/AbilitySystem/Ability/WxAbility_Skill.cpp:10-11`·`:21`
- **범주**: 중복/복잡도
- **문제**: Finisher의 `ApplyFinisherDamage` 주석은 "앞잡 그로기 해제(DP 0)는 피해자의 앞잡 짝 피격 몽타주 종료 시 `WxAbility_HitReact`가 처리한다"고 적었지만, 실제로는 같은 파일 `HandleFinisherMontageCompleted`(`:148-168`)에서 공격자가 `UWxEffect_ResetDP`로 처리한다. Skill 생성자에는 주석 처리된 코드(`//AssetTags.AddTag(WxGameplayTags::Ability_Skill_@);`)가 남아 있고, `:21`의 "입력 태그(Input.Skill.1~4)는 AbilitySet 항목의 InputTag로 지정한다"는 현재 구조와 맞지 않는다 — `UWxAbilitySet`에 InputTag 개념이 없고(`Public/AbilitySystem/WxAbilitySet.h:49-59`) 입력 라우팅 키는 `UWxAbilityBase::ActivationInputAction`이 보유한다.
- **제안**: 주석 코드는 제거하고 두 주석을 현재 구현에 맞춰 갱신한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Private/AbilitySystem/WxAbilitySet.cpp`, `Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/Targeting/WxLockOnManagerComponent.cpp`, `Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Private/WxCombatLibrary.cpp`, `Private/WxDamageInfo.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체(10개), `Private/AbilitySystem/Effect/` 전체(GE 15개·MMC 3개·`WxExecCalc_Burn`), `Private/AbilitySystem/Cue/` 전체(5개), `Private/Targeting/` 나머지(필터 태스크 5종·`WxLockOnPointComponent`), `Private/AbilitySystem/Task/` 나머지(`SlowTime`·`WaitInputActionTriggered`), `Private/AbilitySystem/Ability/` 나머지(Skill·Ultimate·Pattern·Death·Sprint), `Public/` 헤더 전량(선언·UPROPERTY 스캔), `WxCombat.Build.cs`, `WxCombat.uplugin`, 호출부 확인용 `Source/WxGame/Character/WxCharacterBase.cpp`·`WxPlayerCharacter.cpp`, `Plugins/WxUI/.../WxViewModel_Ability.cpp`
- **규칙 준수 확인(위반 없음)**: 143개 소스 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.`, `Wx` prefix 준수, 람다 0건, `FORCEINLINE`/인라인 정의 0건, 델리게이트 바인딩 콜백 32종 전부 `Handle` prefix, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 하나(Blueprint Function Library), Build.cs·uplugin 의존은 Wx 플러그인 중 `WxCore`뿐. override의 `Super::` 미호출은 값 반환형 순수가상 계열(`Execute_Implementation`·`CalculateBaseMagnitude_Implementation`·`ShouldFilterTarget` 등)뿐으로 실질 위반 0건.
- **미검토 / 한계**:
  - 데이터 자산은 보지 않았다 — `FWxAbilityTableRow`/`FWxDamageTableRow`/`FWxCombatAttributeInitTableRow` DataTable 실제 값, GE·어빌리티 BP 서브클래스 디폴트(예: 스킬 BP가 `Ability.Skill` 애셋 태그를 실제로 지정하는지 — 미지정 시 `WxAbility_HitReact`/`WxAbility_Finisher`의 `CancelAbilitiesWithTag`가 동작하지 않는다), TargetingPreset 구성은 범위 밖이다.
  - 리플리케이션 관련 지적(1·2·3·4·5·6·7)은 정적 분석 기반이며 데디케이티드 서버 세션으로 재현 검증하지 않았다.
  - 직전 리뷰가 남긴 `UWxAbilityBase::GetCooldownGameplayEffect`(`WxAbilityBase.cpp:156-183`)의 런타임 `NewObject` 우려는 이번에 재확인해 발견에서 제외했다 — 엔진 `ApplyGameplayEffectToOwner`가 `GameplayEffect->GetClass()`로 스펙을 만들어 실제 `Spec.Def`는 CDO가 되고, 이 인스턴스는 `UWxViewModel_Ability`(`Plugins/WxUI/.../WxViewModel_Ability.cpp:23-26`)의 `StackLimitCount` 조회 전용이다.
  - `AWxProjectileBase`의 `CachedEffectContext`/`CachedSpecHandles`(`Public/Weapon/WxProjectileBase.h:80-81`)에 UPROPERTY가 없으나, 내부 강참조가 GE CDO뿐이고 액터 참조는 전부 약참조라 실제 GC/댕글링 위험이 없다고 판단해 발견으로 올리지 않았다.
  - `UWxAbility_Groggy`의 0.1초 몽타주 폴링(`WxAbility_Groggy.cpp:69`, `:174-193`)은 의도된 설계로 알고 있어 지적하지 않았다.
  - `UWxAnimNotifyState_CameraMove`의 에디터 프리뷰 경로, `UWxAbilityTask_PlaySkillCutscene`의 시퀀스 바인딩(`SetBindingByTag`), GameplayCue의 데디케이티드 서버 실행 여부는 코드만 읽고 동작 확인은 하지 않았다.

---
*문서 기준 커밋 `95a57ef3` · 리뷰일 2026-08-07 · 소스 143파일 — `/module-review`로 갱신*
