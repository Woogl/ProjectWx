# WxCombat — 코드 리뷰

> GAS 파이프라인이 엔진 순정 경로를 존중하며 잘 정리돼 있고, 태그 누수·콜백 레이스·콤보 재진입 같은 까다로운 실패 경로는 실패복구 코드와 주석으로 촘촘히 막아 놓았다. 프로젝트 코딩·모듈 규칙 위반은 사실상 없다(WxCore 외 참조 0건, 인라인 정의·불필요 람다·BlueprintCallable 오용 0건). 남은 지적은 대부분 네트워크 권위 경계·전역 상태 소유권·객체 수명 처리에 몰려 있다. 이번 리뷰는 Build.cs/uplugin과 Public 헤더 전량을 훑고, 어빌리티 베이스·주요 파생 어빌리티·데미지 파이프라인·무기 히트·락온·타임딜레이션은 cpp 로직까지 내려가 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 9 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 `HitActorsThisSwing`가 UPROPERTY 없이 강참조를 들고 있어 GC에 추적되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:107` (사용처 `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:198`)
- **범주**: 버그/정확성
- **문제**: `TSet<TObjectPtr<AActor>> HitActorsThisSwing`에 `UPROPERTY()`가 없다. `TObjectPtr`이라도 리플렉션에 등록되지 않으면 참조 그래프에 들어가지 않으므로, GC가 원소를 살려주지도 파괴 시 null로 바꿔주지도 않는다. 스윙 중 피격된 적이 파괴되고 그 사이 GC가 돌면 `Tick`의 `Params.AddIgnoredActor(AlreadyHit.Get())`가 죽은 포인터를 역참조한다. 바로 위 `if (AlreadyHit)`는 null 검사일 뿐 유효성 검사가 아니라 걸러주지 못한다. 같은 클래스의 다른 UObject 멤버(`GripPoint`/`Mesh`/`HitCollision`)에는 모두 UPROPERTY가 붙어 있어 단순 누락으로 보인다.
- **제안**: `UPROPERTY()`를 붙이거나, 수명 소유가 부담되면 `TSet<TWeakObjectPtr<AActor>>`로 바꾼다(`AWxEffectZone::AppliedTargets`가 이미 그 방식이다).
- **확신도**: 높음

### 2. 🟡 전역 타임 딜레이션이 "마지막 요청자 승"이라 진행 중인 컷신의 시간축이 깨진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:50`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:98`
- **범주**: 설계/구조
- **문제**: `SetDilationFrom`은 값이 같아도 소유권을 새 요청자에게 넘기고, `ClearDilationFrom`은 소유자가 다르면 무시한다 — 즉 겹친 요청은 스택이 아니라 덮어쓰기다. 그런데 컷신 태스크는 진입 시점 배율을 기준으로 `SequencePlayer->SetPlayRate(1.f / GlobalTimeDilation)`(기본 0.001 → 1000배)를 한 번만 고정한다. 컷신이 도는 동안 다른 액터의 슬로우 요청(퍼펙트 가드 `WxAbility_Guard.cpp:275`, 극한 회피 `WxAbility_Dodge.cpp:292`)이 소유권을 가져가 월드 배율을 0.4로 올리면, 시퀀스는 400배속으로 순식간에 끝난다. 컴포넌트가 GameState에 붙은 전역 상태라 요청자가 서로 다른 폰이어도 같은 값을 다툰다. 잔존 슬로우로 월드가 갇히는 예전 결함은 소유권 도입으로 해소됐으나, 겹침 시 연출이 서로를 끊는 성질은 남아 있다.
- **제안**: 요청을 참조 카운트/스택으로 쌓아 가장 강한(또는 최신) 요청이 해제될 때 이전 요청으로 되돌리거나, 컷신 재생 속도를 고정값이 아니라 현재 배율을 매 프레임 반영하는 방식으로 바꾼다.
- **확신도**: 중간(겹침 순서에 의존하지만 경로는 코드로 확인됨)

### 3. 🟡 `OnGranted` 자동 활성화가 클라이언트에서도 돌고 중복 활성 가드가 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:146`
- **범주**: 설계/구조
- **문제**: `OnGiveAbility`는 서버의 `GiveAbility`뿐 아니라 클라이언트가 어빌리티 스펙을 복제받을 때도 호출된다. 여기서 조건 없이 `TryActivateAbility`를 부르므로, 베이스 기본값인 `LocalPredicted` 패시브는 클라이언트에서 예측 활성화 → 서버 RPC → 서버는 이미 활성이라 거부 → 클라이언트 롤백을 밟는다. 그 사이 `ActivationOwnedTags`가 잠깐 붙었다 떨어지는 깜빡임이 생긴다. `Spec.IsActive()` 검사도, `ActorInfo`/`ActorInfo->AbilitySystemComponent` 유효성 검사도 없다(약참조를 `->`로 바로 역참조한다).
- **제안**: 활성화 조건을 `ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && !Spec.IsActive()`와 "예측 중이 아님"으로 좁힌다. 부여 시점보다 아바타 세팅 시점이 자연스러운 훅이다.
- **확신도**: 중간

### 4. 🟡 Max 어트리뷰트 비례 스케일이 클라이언트 복제 수신 경로에서도 어트리뷰트를 쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:96`
- **범주**: 설계/구조
- **문제**: `PostAttributeChange`는 복제 수신(`GAMEPLAYATTRIBUTE_REPNOTIFY` → `SetBaseAttributeValueFromReplication`) 경로에서도 호출된다. MaxHP/MaxSP/MaxMP 분기는 그 경로에서도 `SetHP/SetSP/SetMP`로 서버 권위 어트리뷰트를 클라이언트가 직접 덮어쓴다. 바로 아래 DP 분기(`:111`)는 정확히 같은 이유로 `IsOwnerActorAuthoritative()`를 명시적으로 걸어두었으므로 Max 분기만 게이트가 빠진 것으로 보인다. 증상은 Max 값이 바뀌는 순간 클라이언트에서 현재값이 한 프레임 튀었다가 다음 복제로 교정되는 형태다.
- **제안**: Max 분기도 DP 분기와 동일하게 권위 게이트를 건다.
- **확신도**: 중간

### 5. 🟡 `OnRep_LockOnTarget`이 `State.LockedOn` 태그 이관을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:56`
- **범주**: 버그/정확성
- **문제**: 대상 표시 태그(`State.LockedOn`)의 부여/회수는 `ApplyLockOnTarget`(`:62`)에만 있는데, 복제 수신 경로는 네트 레이어가 `LockOnTarget`을 직접 대입한 뒤 `OnRep_LockOnTarget`이 브로드캐스트만 한다. 소유 클라가 A를 예측했는데 서버가 B로 정정해 복제하면 A에 붙은 `State.LockedOn`이 영구히 남고 B에는 붙지 않는다. 로컬 예측과 복제 정합이 서로 다른 코드 경로를 타는 비대칭이다.
- **제안**: 태그 이관 로직을 별도 함수로 빼서 `OnRep`에서도 이전 값 대비로 호출한다(직전 값 캐시 필요).
- **확신도**: 중간

### 6. 🟡 데미지 ExecCalc가 순수 계산이 아니라 부수효과 허브다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:52`
- **범주**: 설계/구조
- **문제**: `Execute_Implementation`이 출력 모디파이어 산출 외에 (a) 다른 ASC에 GE 적용(`:137` `UWxEffect_RecoverResource::ApplyTo`), (b) 소스 ASC 어트리뷰트 직접 쓰기(`:210` `SetNumericAttributeBase` — 스펙·컨텍스트·면역·복제/예측 경로를 통째로 우회하며, 그 결과가 그로기 유발이라는 큰 게임플레이 판정으로 이어진다), (c) 대상 어빌리티 취소(`:330` `CancelAbilities` → Guard의 `EndAbility`가 루스 태그와 태스크를 건드린다), (d) 이벤트 송출·Cue 실행을 모두 수행한다. 전부 대상 ASC의 GE 실행 스코프 안에서 벌어지므로 공격자와 피격자가 같은 ASC인 경우(자해·환경 대미지) 같은 컨테이너에 재진입 적용이 일어난다. 같은 함수 안에서 회복은 GE 경로, 반사는 직접 쓰기로 방식이 엇갈리는 점도 일관성 문제다.
- **제안**: 최소한 반사 DP는 회복과 동일하게 Instant GE로 통일한다. GE 적용·어빌리티 취소를 ExecCalc 밖(적용 후 훅)으로 밀어내는 방향도 함께 검토한다.
- **확신도**: 낮음(의도된 단축일 수 있음 — 현재 경로에서 관측된 오작동은 없다)

### 7. 🟡 락온 종료가 `bOrientRotationToMovement`를 저장값이 아니라 `true`로 하드코딩 복원한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:43`, `:101`
- **범주**: 설계/구조
- **문제**: 활성화 시 `false`로 끄고 종료 시 무조건 `true`로 되돌린다. 원래 값을 기억하지 않으므로 기본값이 `false`인 폰(스트레이프 이동)이나 같은 플래그를 끄고 있는 다른 연출과 겹치면 그쪽 상태가 조용히 덮어써진다. 락온 해제는 대상 소실·거리 초과·사망 등 임의 타이밍에 일어나 겹칠 여지가 넓다. 부수적으로 `GetCharacterMovement()` 널 검사도 없다.
- **제안**: 진입 시 이전 값을 캐시해 복원하거나, 회전 모드를 소유한 쪽(캐릭터/AI)에 요청을 넘긴다.
- **확신도**: 중간

### 8. 🟡 무기 스윕이 비권위 머신에서도 매 틱 돌지만 결과는 전부 버려진다(주석이 사실과 다름)
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:171`, `:253`
- **범주**: 성능/안전
- **문제**: `ANS_WeaponAttack`은 몽타주가 재생되는 모든 머신에서 실행되므로 클라이언트에서도 `BeginAttack` → 틱 활성 → 매 프레임 `SweepMultiByObjectType` + 히트마다 `MakeSpecs`가 돈다. 그런데 최종적으로 부르는 `ApplyGameplayEffectSpecToTarget`은 예측 키 없는 비권위 호출을 엔진(`HasNetworkAuthorityToApplyGameplayEffect`)이 걸러내므로 클라이언트에서는 아무 효과도 만들지 못한다. 교전 중인 캐릭터 수만큼 곱해지는 순수 낭비 쿼리다. `:255`의 주석("클라이언트의 GE 적용은 어빌리티의 ScopedPredictionKey로 예측 처리되며 … GAS가 자동으로 롤백한다")도 사실과 다르다 — 애님 노티파이는 어빌리티 활성화 스코프 밖이라 그 시점 `ScopedPredictionKey`는 유효하지 않다. 반대로 `WxAnimNotify_AreaAttack.cpp:20`과 `WxProjectileBase.cpp:103`은 권위 게이팅이 돼 있어 모듈 내에서 모델이 갈린다.
- **제안**: 예측을 실제로 도입하든(어빌리티 활성화 스코프에서 유효 예측 키를 실어 보냄), 클라 판정을 포기하고 `BeginAttack`/`Tick`을 권위 게이팅하든 하나로 정한다. 어느 쪽이든 주석을 실제 동작에 맞춘다.
- **확신도**: 높음

### 9. 🟡 `UWxAbility_Guard::PlayMontage`가 널 몽타주에도 성공을 반환한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:136`
- **범주**: 버그/정확성
- **문제**: 페이즈 몽타주(`GuardBreakMontage`/`GuardCounterMontage`/`PerfectGuardMontage` 등)는 전부 선택적 `UPROPERTY`인데 `PlayMontage`가 널을 거르지 않는다. 널이면 `ActiveMontage = nullptr`로 세팅한 뒤 태스크가 `OnCancelled`를 즉시 브로드캐스트해 어빌리티가 끝나는데도 함수는 `true`를 반환한다. 그 결과 `ActivateAbility`(`:93`)는 실패 분기를 타지 않고 이미 종료된 어빌리티 위에 `ListenForGuardHit`/`ListenForPerfectGuard`/`ListenForCounterInput` 태스크를 계속 등록한다. 형제 클래스 `UWxAbility_Dodge::PlayMontage`(`WxAbility_Dodge.cpp:251`)는 같은 자리에서 널을 검사해 `false`를 반환한다. 이 클래스의 페이즈 판정이 전부 `ActiveMontage == <선택적 몽타주>` 포인터 비교라, 양쪽이 널이면 오탐하는 구조라는 점도 같은 뿌리다.
- **제안**: `Dodge`와 동일하게 `if (!Montage) return false;`를 앞단에 둔다.
- **확신도**: 높음

### 10. 🟢 타이머 콜백 두 곳에 `Handle` prefix가 빠져 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:355`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:69`
- **범주**: 규칙 위반
- **문제**: `SetTimer`로 델리게이트에 바인딩되는 `ResumeFromHitStop`, `TickPlayMontage`에 `Handle` prefix가 없다. 같은 모듈의 다른 타이머 콜백 `HandleGroggySafetyTimeout`(`WxAbility_Groggy.cpp:73`), `HandleRagdollDelayElapsed`(`WxAbility_Death.cpp:83`)는 규칙을 지키고 있어 모듈 안에서도 어긋난다.
- **제안**: `HandleHitStopResumed`, `HandleMontagePollTick` 등으로 통일한다.
- **확신도**: 높음

### 11. 🟢 죽은 상태와 주석 처리된 코드
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h:111`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:75`
- **범주**: 중복/복잡도
- **문제**: `PrevCapsuleRotation`은 `WxWeaponBase.cpp:76`과 `:218`에서 쓰기만 하고 읽는 곳이 없다(스윕은 `CurrRotation`을 쓴다). `WxAbility_HitReact.cpp:75~85`에는 "패턴 중 Normal 피격 억제" 로직이 통째로 주석 처리돼 있고 `:20~21`에도 주석 처리된 태그 추가가 남아 있다.
- **제안**: 미사용 멤버는 삭제하고, 주석 코드는 되살릴 계획이 없다면 제거한다(필요하면 의도만 한 줄로 남긴다).
- **확신도**: 높음

### 12. 🟢 구현과 어긋난 낡은 주석
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:212`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:11`
- **범주**: 중복/복잡도
- **문제**: Finisher의 `ApplyFinisherDamage` 주석은 "앞잡 그로기 해제(DP 0)는 피해자의 짝 피격 몽타주 종료 시 `WxAbility_HitReact`가 처리한다"고 적었지만, 실제로는 같은 파일 `HandleFinisherMontageCompleted`(`:148`)에서 공격자가 `UWxEffect_ResetDP`로 처리한다. Skill 생성자 주석은 `AssetTags` 직접 대입 형태(현재 API는 `SetAssetTags`)와 `Input.Skill.1~4` 태그·`AbilitySet 항목의 InputTag`를 언급하는데, 입력 라우팅은 이미 `UInputAction` 직접 매칭으로 옮겨져 그런 개념이 없다.
- **제안**: 두 주석을 현재 구현에 맞춰 갱신하거나 삭제한다.
- **확신도**: 높음

### 13. 🟢 `AWxEffectZone`이 GE 레벨 0으로 스펙을 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxEffectZone.cpp:48`
- **범주**: 버그/정확성
- **문제**: `MakeOutgoingSpec(EffectClass, 0, Context)` — 모듈의 다른 모든 호출부(`WxDamageInfo.cpp:38`, `WxAbilitySet.cpp:74`, `WxEffect_RecoverResource.cpp:39`)는 레벨 1을 쓴다. 레벨 인덱스 커브를 쓰는 GE를 이 존에 붙이면 의도와 다른 구간을 읽는다.
- **제안**: 레벨 1로 맞추거나, 0이 의도라면 이유를 주석으로 남긴다.
- **확신도**: 중간

### 14. 🟢 널 검사·정리 순서의 국지적 불일치
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:137`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:117`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:56`
- **범주**: 버그/정확성
- **문제**: (a) `SetLastPressedInputAction`이 `GetOwnerActor()->HasAuthority()`로 널 검사 없이 역참조한다. (b) `UWxAbility_Guard::EndAbility`가 `ActorInfo->`를 검사 없이 역참조하는데, 같은 위치에서 `Dodge`(`WxAbility_Dodge.cpp:112`)·`HitReact`(`:149`)·`Finisher`(`:131`)·`Groggy`는 모두 `ActorInfo`를 먼저 확인한다. (c) `UWxAbility_Sprint::EndAbility`만 `Super::EndAbility`를 먼저 부르고 그 뒤에 GE를 정리한다(다른 어빌리티는 전부 정리 후 Super). 셋 다 현재 호출 경로에서 문제가 관측되지는 않지만 모듈 내 규약이 갈린다. `WxAbility_HitReact.cpp:110`의 `GetCharacterMovement()->JumpZVelocity`도 같은 종류의 무검사 역참조다.
- **제안**: 형제 코드에 맞춰 검사와 순서를 통일한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxDamageInfo.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체(GE·MMC·`WxExecCalc_Burn`), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`·`WxAbilityTask_WaitInputActionTriggered.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxEffectZone.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지(Skill·Ultimate·Pattern·Death·Sprint), Public 헤더 전량(선언·UPROPERTY 스캔)
- **미검토 / 한계**: 데이터 자산은 보지 않았다 — `FWxAbilityTableRow`/`FWxDamageTableRow`/`FWxCombatAttributeInitTableRow` DataTable 실제 값, GE·어빌리티 BP 서브클래스 디폴트, TargetingPreset 구성은 범위 밖이라 "어떤 몽타주·행이 실제로 비어 있는가" 같은 데이터 기인 결함은 확인하지 못했다. 리플리케이션 관련 지적(3·4·5·8)은 정적 분석 기반이며 데디케이티드 서버 세션으로 재현 검증하지는 않았다. `UWxAnimNotifyState_CameraMove`의 에디터 프리뷰 경로와 `UWxAbilityTask_PlaySkillCutscene`의 시퀀스 바인딩·`SetBindingByTag`는 코드만 읽고 동작 확인은 하지 않았다.

---
*문서 기준 커밋 `14a77aef` · 리뷰일 2026-08-03 · 소스 147파일 — `/module-review`로 갱신*
