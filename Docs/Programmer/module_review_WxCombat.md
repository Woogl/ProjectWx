# WxCombat — 코드 리뷰

> GAS 순정 경로를 존중하며 잘 정리된 모듈이다. 태그 누수·콜백 레이스·콤보 재진입 같은 까다로운 실패 경로는 실패복구 코드와 주석으로 촘촘히 막혀 있고, 프로젝트 코딩·모듈 규칙 위반은 이번 리뷰 기준 0건이다(143개 소스 전부 Copyright 첫 줄, WxCore 외 Wx 플러그인 참조 0, 람다 0, `FORCEINLINE`/인라인 정의 0, `BlueprintCallable` 1건은 Blueprint Function Library, 델리게이트 콜백 56개 전부 `Handle` prefix). 남은 지적은 전부 네트워크 권위 경계·예측 키 수명·전역 상태 소유권에 몰려 있다. 이번 리뷰는 README 진입점(ASC·AbilityBase·AttributeSet·대미지 파이프라인)을 축으로 무기/투사체·락온·시간감속·AnimNotify·Effect/MMC/Cue·타게팅 필터까지 cpp 로직 레벨로 내려가 읽었고, 직전 리뷰(`95a57ef3`) 지적은 전부 현재 코드에 재대조해 해소된 항목(`OnRep_LockOnTarget`의 `State.LockedOn` 이관 → 락온 태스크로 이동, 무기 히트의 예측 키 미전달, 팀 판정 관련 헤더 주석 불일치 → 자유 피격이 의도임을 명시, Finisher·Skill의 낡은 주석·죽은 코드)은 뺐다. 대신 예측 키 도입(`ad9187b6`)이 만든 새 실패 모드를 UE 5.8 엔진 소스로 대조해 추가했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟡 개선 | 6 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 무기 히트가 이미 서버 ack된 활성화 예측 키로 GE를 예측 적용해, 롤백이 다음 스테일 스윕까지 밀린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:24-31`·`:45`, `Private/Weapon/WxWeaponBase.cpp:240-242`
- **범주**: 버그/정확성
- **문제**: `ApplyDamage`가 `AnimatingAbility->GetCurrentActivationInfo().GetActivationPredictionKey()`를 예측 키로 쓴다. 그런데 이 키는 **어빌리티 활성화 시점**에 만들어져 서버가 즉시 ack하는 키다 — 클라는 활성화 후 약 1 RTT 만에 `FReplicatedPredictionKeyItem::OnRep` → `FPredictionKeyDelegates::CatchUpTo(K)`로 이 키를 이미 소진한다(엔진 `GameplayPrediction.cpp:614`). 반면 `ANS_WeaponAttack`의 히트는 그보다 수백 ms 뒤에 발생한다. UE 5.8의 `FPredictionKey::IsValidForMorePrediction()`은 `IsLocalClientKey()`와 동일해 stale 판정이 없으므로(엔진 `GameplayPrediction.h:342-357`), 이미 소진된 키로도 예측 적용이 그대로 통과한다. 그 결과 `ApplyGameplayEffectSpec`이 등록하는 롤백 델리게이트가 **이미 처리된 키 슬롯에 새로 만들어지고**(엔진 `GameplayPrediction.cpp:340-355`의 `CatchUpTo`는 정확히 그 키만 찾아 처리하고 끝난다), 서버 확정본이 도착해도 예측본이 걷히지 않는다. 실제 정리는 그 키가 ±32 키 창 밖으로 밀려날 때의 스테일 스윕에서야 일어나며, 그때 엔진이 `LogPredictionKey: Warning: UnAck'd PredictionKey ... in DelegateMap` 경고를 남긴다(엔진 `GameplayPrediction.cpp:634-682`). 구체적 증상 둘: (a) Instant 대미지 GE는 클라에서 `bTreatAsInfiniteDuration`으로 무한 GE가 되어(엔진 `AbilitySystemComponent.cpp:1066`) 대상 ASC에 계속 쌓인다, (b) `FWxDamageInfo::AdditionalEffects`의 지속형 GE(예: `UWxEffect_Burn`, `GameplayCues` 보유·무제한 스택)는 예측본과 서버 복제본이 그 창 동안 **동시에** 살아 있어 큐/스택이 이중으로 보인다. `WxCombatLibrary.h:28-31`은 "서버 확정본이 도착하면 GAS가 예측본을 정리한다"고 적었지만, 그 정리는 키가 아직 in-flight일 때만 성립한다.
- **제안**: 히트 시점에 유효한 예측 키를 새로 열든(`FScopedPredictionWindow`로 히트 단위 키 발급 + 서버 전달), 무기 히트의 클라 예측을 포기하고 `BeginAttack`/`Tick`을 권위 게이팅하든 하나로 정한다. 어느 쪽이든 헤더 주석의 "정리된다" 문구를 실제 수명에 맞춘다.
- **확신도**: 중간(엔진 경로는 소스로 확인했으나 데디케이티드 세션 재현은 하지 않음)

### 2. 🟡 `PostAttributeChange`의 Max 어트리뷰트 비례 스케일이 클라이언트 복제 수신 경로에서도 실행된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:116-130`
- **범주**: 설계/구조
- **문제**: `PostAttributeChange`는 서버 전용 훅이 아니다. 엔진의 `SetBaseAttributeValueFromReplication` → `SetNumericAttribute_Internal` 경로가 이 훅을 호출하므로, 클라이언트가 MaxHP/MaxSP/MaxMP 복제를 수신할 때마다 `SetHP/SetSP/SetMP`로 서버 권위 어트리뷰트를 로컬 계산으로 덮어쓴다. 복제 경로는 "old 값으로 되감기 → new 적용" 2단이라 한 번의 복제에 두 번 불릴 수도 있다. 바로 아래 DP 분기(`:131-146`)는 정확히 같은 이유로 `ASC->IsOwnerActorAuthoritative()`를 명시 게이팅해 두었으므로 Max 3분기만 빠진 것으로 보인다.
- **제안**: 세 Max 분기도 DP 분기와 동일하게 권위 게이트를 건다.
- **확신도**: 중간

### 3. 🟡 컷신 PlayRate가 진입 시점 배율로 고정돼, 원격 클라이언트에서 궁극기 컷신이 한두 프레임에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:51`·`:96-99`, `Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:55`
- **범주**: 버그/정확성
- **문제**: 태스크는 `SetGlobalTimeDilationAuthoritative`로 월드를 느리게 만든 뒤 `SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation)`(궁극기 기본값 0.001 → **1000배**)를 한 번만 고정한다. 그런데 `SetDilationFrom`은 권위 게이트가 걸려 있어(`Private/Time/WxTimeDilationComponent.cpp:37-41`) 원격 클라에서는 즉시 no-op이고, 실제 감속은 `ReplicatedTimeDilation` 복제가 도착한 뒤 `OnRep`(`:130-133`)에서야 적용된다. `UWxAbility_Ultimate`는 베이스 기본값인 `LocalPredicted`(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:23`)라 이 태스크가 소유 클라에서도 돌므로, 복제가 오기 전 프레임 동안 시퀀스만 1000배속으로 진행된다. 60fps 기준 한 프레임에 약 16초 분량이 흘러가 컷신이 사실상 통째로 스킵된다. 같은 고정 배율은 리슨 서버에서도 문제인데, 컷신이 도는 중 다른 액터의 슬로우 요청(퍼펙트 가드 `Private/AbilitySystem/Ability/WxAbility_Guard.cpp:281`, 극한 회피 `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:292`)이 월드 배율을 올리면 시퀀스가 그만큼 빨라진다.
- **제안**: PlayRate를 진입 시점 고정값이 아니라 현재 월드 배율을 매 프레임 반영하도록 바꾸거나(태스크를 틱시켜 `GetGlobalTimeDilation` 역수 갱신), 시퀀스를 월드 딜레이션에서 분리해(언디레이트 틱) 배율 보정 자체를 없앤다.
- **확신도**: 중간(정적 분석 기반, 데디케이티드/리슨 세션 재현은 하지 않음)

### 4. 🟡 전역 타임 딜레이션 요청이 스택이 아니라 "마지막 요청자 승" 덮어쓰기다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:50-56`·`:66-77`
- **범주**: 설계/구조
- **문제**: `SetDilationFrom`은 값이 같아도 소유권을 새 요청자에게 넘기고(`:50-52`), `ClearDilationFrom`은 소유자가 다르면 무시한다(`:68-71`). 겹친 요청이 스택이 아니라 덮어쓰기이므로, 긴 슬로우 A가 진행 중일 때 짧은 슬로우 B가 소유권을 가져가면 B가 끝나는 순간 월드가 1.0으로 복귀하고 A의 남은 구간은 통째로 사라진다(A의 뒤늦은 `ClearDilationFrom`도 소유자가 이미 비어 있어 그냥 1.0을 재설정한다). 컴포넌트가 GameState에 붙은 전역 상태라 요청자가 서로 다른 폰이어도 같은 값을 다툰다 — 멀티에서 두 플레이어가 각각 극한 회피/퍼펙트 가드를 겹쳐 쓰거나, 궁극기 컷신(`WxAbilityTask_PlaySkillCutscene`) 중 다른 플레이어가 패링하면 바로 재현되는 구조다.
- **제안**: 요청을 참조 카운트/스택으로 쌓아 해제 시 남아 있는 요청 중 가장 강한(또는 가장 최근) 값으로 되돌린다.
- **확신도**: 중간(겹침 순서에 의존하지만 경로는 코드로 확인됨)

### 5. 🟡 `OnGranted` 자동 활성화가 클라이언트에서도 조건 없이 돌고 무검사 역참조를 한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:146-154`
- **범주**: 설계/구조
- **문제**: `OnGiveAbility`는 서버의 `GiveAbility`뿐 아니라 클라이언트가 어빌리티 스펙을 복제받을 때도 호출된다. 여기서 조건 없이 `ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)`를 부르므로, 베이스 기본값인 `LocalPredicted`(`:23`) 패시브는 클라이언트에서 예측 활성화 → 서버 RPC → 서버는 이미 활성이라 거부 → 롤백을 밟고, 그 사이 `ActivationOwnedTags`가 붙었다 떨어지는 깜빡임이 생긴다. `Spec.IsActive()` 검사도, `ActorInfo`/`ActorInfo->AbilitySystemComponent`(약참조) 유효성 검사도 없다.
- **제안**: 조건을 `ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && !Spec.IsActive()`로 좁히고, 예측 활성화가 불필요하면 권위 게이트를 추가한다.
- **확신도**: 중간

### 6. 🟡 데미지 ExecCalc가 순수 계산이 아니라 부수효과 허브다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:52-178`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어 산출 외에 (a) 다른 ASC에 GE 적용(`:137`), (b) 소스 ASC 어트리뷰트 직접 쓰기(`:210` `SetNumericAttributeBase` — 스펙·컨텍스트·면역·복제 경로를 우회하면서 결과는 "그로기 유발"이라는 큰 판정으로 이어진다), (c) 대상 어빌리티 취소(`:330`), (d) GameplayEvent 동기 송출(`:93`/`:101`/`:175`/`:351`)과 Cue 실행(`:156`/`:160`)을 모두 수행한다. 전부 대상 ASC의 GE 실행 스코프 안에서 벌어져 재진입 적용이 일어난다. 실제로 `UWxAbility_Guard::HandleGuardHitReact`(`Private/AbilitySystem/Ability/WxAbility_Guard.cpp:237-239`)는 "이벤트 수신 시점의 `GetSP()`는 차감 적용 전 값"이라는 이 실행 순서에 명시적으로 의존하고 있어, 순서가 바뀌면 가드 브레이크 판정이 조용히 틀어진다. 같은 함수에서 회복은 GE 경로(`UWxEffect_RecoverResource::ApplyTo`), 반사는 직접 쓰기로 방식이 엇갈리는 점도 함께 걸린다.
- **제안**: 최소한 반사 DP를 회복과 동일하게 Instant GE로 통일한다. 이벤트/Cue/GE 적용을 GE 적용 확정 이후 훅(`PostGameplayEffectExecute` 등)으로 밀어내는 방향도 함께 검토한다.
- **확신도**: 낮음(의도된 단축일 수 있음 — 현재 가드/패링 타이밍이 이 순서에 맞춰져 있다)

### 7. 🟢 AbilitySet 부여에 재진입 방어가 없고, 유일한 해제 경로가 데드 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-34`·`:44-60`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp:16-24`, `Public/AbilitySystem/WxAbilitySystemComponent.h:65`
- **범주**: 중복/복잡도
- **문제**: `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`은 저장소 전체에서 호출부가 없다. 그 핸들을 담으려고 존재하는 멤버 `AbilitySetGrantedHandles`(`WxAbilitySystemComponent.h:65`)도 채워지기만 하고 읽히지 않으므로, 부여는 있고 회수는 없는 편도 API다. 함께 걸리는 것이 `GiveToAbilitySystem`의 어트리뷰트 초기화 순서로, HP를 MaxHP보다 먼저 세팅한다(`:46-47`, SP/MP/DP/UP도 동일). 첫 부여에서는 MaxHP가 0이라 `PreAttributeChange`의 클램프도 `PostAttributeChange`의 비례 스케일(`Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:116-130`)도 건너뛰어 문제가 드러나지 않지만, 재부여가 한 번이라도 일어나면 HP가 이전 MaxHP로 클램프된 뒤 MaxHP 변경이 비례 스케일을 태워 Row가 지정한 값과 다른 HP가 남는다. 유일한 호출부가 `AWxCharacterBase::PossessedBy`(`Source/WxGame/Character/WxCharacterBase.cpp:123`→`:215`)라 같은 ASC로 재빙의되면 그대로 노출되며, 중복 부여를 막는 가드도 없다.
- **제안**: 해제 경로를 실제로 쓰거나(재부여 전 `RemoveFromAbilitySystem` 호출) 쓰지 않을 거면 구조체·멤버를 제거한다. 남긴다면 Max를 먼저 세팅하도록 순서를 뒤집고, 이미 부여된 상태면 조기 반환하는 가드를 둔다.
- **확신도**: 높음(데드 코드), 중간(재부여 시 어트리뷰트 오차)

### 8. 🟢 널 검사·정리 순서·타이머 해제의 국지적 불일치
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:137`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp:117`, `Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:48-56`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:45`·`:103`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:95`·`:172`·`:202`, `Private/AbilitySystem/Ability/WxAbility_Death.cpp:86`, `Private/AnimNotify/WxAnimNotifyState_{ComboWindow,Invincible,PerfectGuard,WeaponAttack}.cpp`
- **범주**: 버그/정확성
- **문제**:
  (a) `SetLastPressedInputAction`이 `GetOwnerActor()->HasAuthority()`로 널 검사 없이 역참조한다.
  (b) `UWxAbility_Guard::EndAbility`가 `ActorInfo->`를 검사 없이 역참조하는데, 같은 위치에서 `Dodge`(`WxAbility_Dodge.cpp:112`)·`HitReact`(`WxAbility_HitReact.cpp:134`)·`Finisher`(`WxAbility_Finisher.cpp:131`)·`Groggy`(`WxAbility_Groggy.cpp:92`)는 모두 `ActorInfo`를 먼저 확인한다.
  (c) `UWxAbility_Sprint::EndAbility`만 `Super::EndAbility`를 먼저 부르고 그 뒤에 GE를 회수한다(모듈의 다른 모든 오버라이드는 "정리 → Super"). Super 안에서 태스크 종료·`OnGameplayAbilityEnded` 브로드캐스트가 먼저 돌아 다른 어빌리티가 이동속도 GE가 아직 붙은 상태에서 활성화될 수 있다.
  (d) `WxAbility_LockOn`은 `GetCharacterMovement()` 널 검사 없이 `bOrientRotationToMovement`를 쓰고, 종료 시 저장값이 아니라 `true`로 하드코딩 복원한다(현재 `AWxCharacterBase`의 기본값이 `true`라 문제가 드러나지 않지만, 같은 플래그를 끄고 쓰는 코드와 겹치면 조용히 덮어쓴다). `WxAbility_HitReact`의 `GetCharacterMovement()->JumpZVelocity`(`:95`)와 `CurrentActorInfo->` 역참조(`:172`, `:202`)도 같은 종류다.
  (e) `UWxAbility_Death`는 `RagdollDelayTimerHandle`(`:86`)을 예약만 하고 `EndAbility` 오버라이드가 없어 정리하지 않는다. 어빌리티가 지연 0.15초 안에 취소되면 타이머가 뒤늦게 `RagdollAndEnd`를 불러 이미 끝난 어빌리티에 래그돌·`EndAbility`를 덧씌운다. 같은 클래스 계층의 `UWxAbilityBase::EndAbility`(`WxAbilityBase.cpp:255-265`)는 정확히 이 이유로 히트스톱 타이머를 정리한다.
  (f) AnimNotify 4종이 `MeshComp->GetOwner()`를 널 검사 없이 부른다(`ComboWindow`/`Invincible`/`PerfectGuard`는 `:12`·`:25`, `WeaponAttack`은 `:16`·`:32`). 같은 폴더의 나머지 5개 노티파이는 전부 `MeshComp` 널을 먼저 막는다. 특히 앞 셋은 루즈 태그를 Begin/End 쌍으로 붙였다 떼는 구조라 한쪽만 조기 이탈하면 태그가 누수된다.
- **제안**: 형제 코드에 맞춰 검사·순서·복원·타이머 정리를 통일한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Private/WxDamageInfo.cpp`, `Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Private/AbilitySystem/WxAbilitySet.cpp`, `Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/Targeting/WxLockOnManagerComponent.cpp`, `Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Private/AnimNotify/` 전체(10개). 예측 키 관련 판정은 UE 5.8 엔진 소스(`GameplayAbilities/Private/AbilitySystemComponent.cpp`, `Private/GameplayPrediction.cpp`, `Public/GameplayPrediction.h`, `Public/GameplayAbilitiesDeveloperSettings.h`)와 직접 대조했다.
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체(GE 15개·MMC 3개·`WxExecCalc_Burn`), `Private/AbilitySystem/Cue/` 전체(5개), `Private/Targeting/` 나머지(필터 태스크 5종·`WxLockOnPointComponent`), `Private/AbilitySystem/Task/` 나머지(`SlowTime`·`WaitInputActionTriggered`), `Private/AbilitySystem/Ability/` 나머지(Skill·Ultimate·Pattern·Death·Sprint), `Private/WxCombatModule.cpp`, `Public/` 헤더 전량(선언·UPROPERTY 스캔), `WxCombat.Build.cs`, `WxCombat.uplugin`, 호출부 확인용 `Source/WxGame/Character/WxCharacterBase.cpp`·`WxPlayerCharacter.cpp`
- **규칙 준수 확인(위반 없음)**: 143개 소스 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.`, `Wx` prefix 준수, 람다 0건, `FORCEINLINE`/인라인 정의 0건, 델리게이트 바인딩 콜백 56종 전부 `Handle` prefix, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 하나(`UBlueprintFunctionLibrary` 파생), Build.cs·uplugin 의존은 Wx 플러그인 중 `WxCore`뿐. override의 `Super::` 미호출은 값 반환형 순수가상 계열(`Execute_Implementation`·`CalculateBaseMagnitude_Implementation`·`ShouldFilterTarget` 등)뿐으로 실질 위반 0건.
- **미검토 / 한계**:
  - 데이터 자산은 보지 않았다 — `FWxAbilityTableRow`/`FWxDamageTableRow`/`FWxCombatAttributeInitTableRow` DataTable 실제 값, GE·어빌리티 BP 서브클래스 디폴트(특히 발견 1의 이중 큐 증상은 `AdditionalEffects`에 지속형 GE가 실제로 들어 있는지에 달려 있다), TargetingPreset 구성은 범위 밖이다.
  - 리플리케이션 관련 지적(1·2·3·4·5)은 정적 분석 기반이며 데디케이티드 서버 세션으로 재현 검증하지 않았다.
  - 무기·투사체의 팀 판정 부재(`Private/Weapon/WxWeaponBase.cpp:238-253`, `Private/Weapon/WxProjectileBase.cpp:93-144`)는 이번에 발견에서 제외했다 — `Public/Weapon/WxWeaponBase.h:96-100`이 "팀 판정은 하지 않는다 — 맞은 대상은 아군·중립 여부와 무관하게 피해를 받는다"고 명시해 의도된 설계임이 확인됐다. 다만 투사체 헤더에는 같은 명시가 없어 규약이 한쪽에만 적혀 있다.
  - `UWxAbilityBase::GetCooldownGameplayEffect`(`WxAbilityBase.cpp:156-183`)의 런타임 `NewObject`는 재확인해 제외했다 — 엔진이 `GameplayEffect->GetClass()`로 스펙을 만들어 실제 `Spec.Def`는 CDO가 되고, 이 인스턴스는 `StackLimitCount` 조회 전용이다.
  - `AWxProjectileBase`의 `CachedEffectContext`/`CachedSpecHandles`(`Public/Weapon/WxProjectileBase.h:80-81`)에 UPROPERTY가 없으나, 내부 강참조가 GE CDO뿐이고 액터 참조는 전부 약참조라 실제 GC/댕글링 위험이 없다고 판단해 발견으로 올리지 않았다.
  - `UWxAbility_Groggy`의 0.1초 몽타주 폴링(`WxAbility_Groggy.cpp:69`, `:174-193`)은 의도된 설계로 알고 있어 지적하지 않았다.
  - `UWxAnimNotifyState_CameraMove`의 에디터 프리뷰 경로, `UWxAbilityTask_PlaySkillCutscene`의 시퀀스 바인딩(`SetBindingByTag`), GameplayCue의 데디케이티드 서버 실행 여부(`UWxCueNotify_Damage`가 서버에서도 플로터 액터를 스폰하는지)는 코드만 읽고 동작 확인은 하지 않았다.

---
*문서 기준 커밋 `18f580a2` · 리뷰일 2026-08-07 · 소스 143파일 — `/module-review`로 갱신*
