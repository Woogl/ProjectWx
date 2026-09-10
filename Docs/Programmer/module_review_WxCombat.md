# WxCombat — 코드 리뷰

> 건강한 모듈이다. 🔴은 없고, 직전 리뷰에서 지적한 데드 코드(`GetActiveMontage`/`ActiveMontage`, `IsActorLockedOn`, `OnLockedOnChanged`)와 낡은 README 한 줄은 이번 코드에서 모두 정리돼 있다. 남은 지적은 예측 경로의 난수 비결정성, 연출 계열의 수명·리소스 관리, 그리고 콤보 3종 중복·어트리뷰트 베이스 처리 같은 소소한 항목들이다. 프로젝트 규칙 위반은 이번에도 0건이다(모듈 의존이 `WxCore` 단일, 타입 접두사 전부 `Wx`, 델리게이트 콜백 36개 전부 `Handle` 접두사, `BlueprintCallable` 1건이 `UWxCombatLibrary`(Blueprint Function Library)의 `ApplyDamage`, `FORCEINLINE`·인라인 정의 0건, 람다 0건, 저작권 첫 줄 176파일 전부 충족). 이번 리뷰는 대미지 파이프라인(ExecCalc·`UWxEffectComponent_DamageResponse`·AttributeSet·CombatLibrary·DamageTableRow)·어빌리티 16종 전부·발동 그룹/캔슬 창·선입력 버퍼·무기/투사체 히트·락온·히트스톱·소환/투사체 서브시스템·GE 21종·AnimNotify 13종·GameplayCue 6종·Targeting 8종·어빌리티 태스크 5종을 헤더와 함께 읽었고, 직전 리뷰 이후 새로 들어온 코드(`UWxAnimNotify_AreaDamage`, `IWxMinion`, 다중 `AbilitySets`, 투사체 반사 이관)를 우선 확인했다. 판정이 갈리는 지점은 UE 5.8 엔진 소스까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:164`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드를 공유하지 않는다. `UWxAbilityBase` 생성자가 모든 어빌리티를 `LocalPredicted`로 깔고(`WxAbilityBase.cpp:18`), `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`) 이 실행 계산은 소유 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② 같은 파일 201행의 `TargetSP <= FinalDamage` 판정까지 갈려 `Damage.GuardBreak`(`:203`) 부여 여부가 어긋나며(이 태그가 곧 `Event.Hit.GuardBreak` 라우팅을 정한다) ③ 191행이 붙이는 `Damage.Critical`이 클라 플로터와 서버 값 사이에서 불일치한다.
- **제안**: 크리 판정을 권위에서만 굴려 결과를 실어 보내고 예측 측은 그 값을 읽게 하거나(비크리 낙관 예측), 예측 키·스펙 식별자를 시드로 삼는 결정적 난수로 바꾼다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 의도적으로 미룬 것일 수 있음)

### 2. 🟡 카메라 이동 ANS가 자기가 스폰한 카메라를 추적하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:69`, `:180`
- **범주**: 버그/정확성
- **문제**: 스폰한 임시 카메라를 어디에도 보관하지 않아 두 지점이 모두 추정으로 돈다. ① 69행의 안전망 수명 `TotalDuration + BlendOutTime + 1`에서 `TotalDuration`은 몽타주 PlayRate가 반영되지 않은 애니메이션 초다. ASPD 감소 등으로 PlayRate가 충분히 낮으면 구간이 끝나기 전에 뷰타겟 액터가 파괴되어 화면이 튄다 — 같은 함정을 `WxAnimNotifyState_SlowTime.cpp:24-25`는 `Montage_GetPlayRate`로 나눠 처리하고 있어 모듈 안에서 규칙이 갈린다. ② 180행의 `NotifyEnd`는 자기가 카메라를 실제로 세웠는지와 무관하게 무조건 `PC->GetPawn()`으로 뷰를 되돌린다. `NotifyBegin`이 스폰 실패(`:51`)로 빠져나갔거나 다른 연출이 뷰를 쥐고 있어도 그 뷰를 빼앗는다.
- **제안**: 스폰한 카메라를 노티파이가 아닌 액터 쪽(연출 컴포넌트나 태스크)에 맡겨 수명과 뷰 복귀를 그 소유자가 판단하게 한다. 최소 조치로는 수명 계산에 PlayRate를 반영하고, `NotifyEnd`에서 현재 뷰타겟이 자기가 세운 카메라일 때만 되돌린다.
- **확신도**: 중간(①은 PlayRate가 낮은 구성에서만, ②는 스폰 실패·연출 중첩에서만 드러난다)

### 3. 🟡 대미지 플로터가 피격 1회마다 액터+위젯을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고(34행) `InitWidget()`으로 UMG 위젯을 새로 구성한다(60행). 액터의 `InitialLifeSpan`이 5초라(52행) 다타·다수 적 상황에서 화면당 수십 개가 동시에 살아 있게 되며, 각각이 Screen-space `UWidgetComponent`(47행)를 들고 매 프레임 그려진다. 액터 스폰과 위젯 생성은 전투 핫패스에서 가장 비싼 축이다.
- **제안**: 플로터 액터/위젯을 풀링해 재사용하거나, 수명을 애니메이션 길이에 맞춰 줄이고 동시 표시 개수 상한을 둔다.
- **확신도**: 중간(현 규모에서는 문제가 안 될 수 있음)

### 4. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:163`, `:169`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다. 지금 쌍 목록(`:11-18`)의 HP/SP/GP/MP/UP를 건드리는 GE는 Instant이거나 주기형(`WxEffect_RegenSP`·`DrainSP`·`DrainGP`·`InfiniteMP`)뿐이라 전부 베이스를 직접 바꾸고, 지속형 aggregator 모디파이어가 하나도 없어 베이스=현재값이다(지속형 모디파이어를 거는 GE는 `WxEffect_Exceed.cpp:20-29`의 ATK/ASPD, `WxEffect_MoveSpeedScale.cpp:15`의 SPD, `WxEffect_GuardReduction.cpp:30`의 GuardReductionScale뿐이고 이 쌍 목록에 없다). 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 5. 🟢 콤보 어빌리티 3종이 상태와 진행 로직을 그대로 복제하고, 그중 Pattern만 실패 경로에서 인덱스가 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:50-59`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-53`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:30,34` / `WxAbility_Skill.h:36,40` / `WxAbility_Pattern.h:30,33`), Attack과 Skill은 `ActivateAbility`·`EndAbility`·`HandleMontageCompleted` 세 함수가 한 글자 차이 없이 같다(차이는 생성자의 애셋 태그와 쿨다운 GE 지정뿐). Pattern은 진행 방식만 다른데(재발동 대신 블렌드아웃 연쇄) 복제한 `ActivateAbility`의 인덱스 전진(`:29`)은 그대로 가져왔고, 정상 종료 시 리셋(`HandleMontageCompleted` 오버라이드)만 빠져 있다. 그 결과 연쇄 도중 `PlayMontage`가 실패하면(배열 중간에 빈 슬롯이 있는 경우) 56-59행이 `bWasCancelled=false`로 끝내 `ComboIndex`가 중간 단에 남고, 다음 발동이 첫 단이 아니라 그 다음 단부터 시작한다. 마지막 단까지 정상 완주한 경우는 다음 발동의 `IsValidIndex(ComboIndex + 1)`이 실패해 0으로 수렴하므로 드러나지 않는다.
- **제안**: 최소 조치로 Pattern의 실패 종료를 `bWasCancelled=true`로 바꾼다(그러면 `EndAbility`의 기존 리셋이 걸린다). 구조 조치로는 "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고 재발동 방식만 파생에 남긴다.
- **확신도**: 중간(인덱스 잔류는 확실. 공통분 추출은 배선 재사용 목적의 상속이 과거에 거부된 방향이라 합의가 먼저다)

### 6. 🟢 소환 로스터의 배열 참조를 소환물 BeginPlay 너머까지 들고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:41`, `:71`, `:73`
- **범주**: 버그/정확성
- **문제**: 41행이 `Rosters.FindOrAdd(&Master)`로 얻은 **TMap 값의 참조**를 잡고, 71행 `Minion->FinishSpawning()`(소환물의 `BeginPlay`·GAS 초기화·`OnGiven` 어빌리티가 여기서 동기로 돈다)을 지난 뒤 73행에서 그 참조에 `Add`한다. 그 사이에 `Rosters`에 **새 키가 하나라도 들어가면**(소환물이 자기 소환물을 부르는 구성) TMap이 재해싱되어 41행의 참조가 해제된 메모리를 가리킨다. 같은 함수 55행의 `ReleaseMinion`은 순회만 하므로 무해하고, 위험은 신규 키 삽입 경로 하나뿐이다.
- **제안**: 참조를 들고 있지 말고 스폰이 끝난 뒤 `Rosters.FindOrAdd(&Master).Add(Minion)`으로 다시 찾는다(상한 정리 구간에서만 참조를 쓰거나 인덱스로 대체).
- **확신도**: 낮음(소환물이 소환하는 구성이 아직 없다면 도달하지 않는다)

### 7. 🟢 몽타주 구간에 메시 본 갱신 정책을 저장·복원하는데 소유권 검사가 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:92`, `:108`
- **범주**: 설계/구조
- **문제**: `EnableAnimatingMontageMeshTick`이 `Mesh->VisibilityBasedAnimTickOption`을 `PreviousMontageTickOption`(`WxAbilitySystemComponent.h:66`)에 담아 두고, 마지막 AnimatingAbility가 풀릴 때 그 값을 되쓴다. 그런데 이 필드는 저장소에 또 다른 쓰기 주체가 있다 — `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`이 리더 메시를 숨기면서 `AlwaysTickPoseAndRefreshBones`를 **영구 설정**으로 박고, 그 리더 메시는 `AWxCharacterBase`의 `GetMesh()`(`Source/WxGame/Character/WxCharacterBase.cpp:45`)로 ASC가 만지는 것과 같은 컴포넌트다. 몽타주가 도는 동안 MetaHuman 조립이 다시 일어나면(컴포넌트 재등록으로 `OnRegister`가 재실행되는 경로) 그 설정이 몽타주 종료 시 낡은 저장값으로 조용히 덮이고, 숨긴 리더가 본 갱신을 멈춰 부착 메시 전체가 굳는다. 같은 모듈의 `UWxAbilityTask_SlowTime`·`UWxAbilityTask_PlaySkillCutscene`은 이런 공유 값에 "지금 값이 내가 건 값인가"를 확인하고 되돌리는데(`WxAbilityTask_SlowTime.cpp:37`), 여기는 그 검사가 없다.
- **제안**: 복원 전에 현재 값이 자기가 세운 `AlwaysTickPoseAndRefreshBones`인지 확인하고, 아니면 남의 설정으로 보아 손대지 않는다.
- **확신도**: 낮음(정상 순서에서는 MetaHuman 조립이 `OnRegister`로 몽타주보다 먼저 끝나 저장값이 이미 `AlwaysTickPoseAndRefreshBones`라 무해하다)

### 8. 🟢 락온 종료의 회전 설정 복원이 ASC 유효성에 묶여 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:104`, `:114-123`
- **범주**: 버그/정확성
- **문제**: `ActivateAbility`가 `bOrientRotationToMovement`를 끄고(`:49-51`) 그 값을 `SavedOrientRotationToMovement`에 담는데, 되돌리는 코드는 `if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())` 블록 안에 들어 있다. 이동 컴포넌트 설정을 되돌리는 일은 ASC와 무관한데 ASC 유효성에 게이트가 걸려 있어, ASC가 먼저 무효해지는 종료 경로에서는 아바타가 이동 방향으로 다시는 돌아서지 못하는 상태로 남는다.
- **제안**: 복원 블록을 ASC 게이트 밖으로 꺼내 `ActorInfo`(또는 아바타)만 보고 되돌린다.
- **확신도**: 중간(현재 도달 경로는 아바타 파괴와 겹쳐 CMC도 함께 사라지는 경우로 보이나, 게이트 자체가 근거 없는 결합이다)

### 9. 🟢 투사체 반사가 성립했는데 대상이 Pawn이 아니면 투사체가 멈추지도 되돌아가지도 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:179-186`
- **범주**: 버그/정확성
- **문제**: `bReflecting`은 `TargetASC`의 `Effect.PerfectGuard`만 보고 정해지는데(`:169-170`) 실제 되돌림은 `Reflect(Cast<APawn>(OtherActor))`로 넘긴다. 대상이 Pawn이 아니면 `Reflect`가 `!Parrier`로 즉시 돌아오고(`:57-63`), 179행의 `if (bReflecting)` 분기가 `else if (!bEvaded) Destroy()`를 건너뛰므로 투사체는 그대로 원래 방향으로 계속 날아 수명이 끝날 때까지 월드에 남는다.
- **제안**: `bReflecting` 판정에 `Cast<APawn>(OtherActor)` 성립까지 포함시켜, 되돌릴 수 없는 대상은 일반 히트로 떨어져 파괴되게 한다.
- **확신도**: 낮음(퍼펙트 가드 태그를 가진 비-Pawn 액터가 현재 구성에 있는지 확인하지 못했다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Passive.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_PlayMontageOnce.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`
- **훑은 파일**: GE 정의 21종(`Private/AbilitySystem/Effect/*` — 각 `DurationPolicy`·`Period`·`StackingType`·`ModifierOp`·모디파이어 대상과 MMC까지 확인), 나머지 AnimNotify 8종(`Private/AnimNotify/*`), 나머지 GameplayCue 4종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`), 나머지 어빌리티 태스크(`WxAbilityTask_RotateToTarget`/`WaitMoving`), `WxAbilitySystemGlobals.cpp`, `WxProjectileSubsystem.cpp`, `WxAbilityTargetData_Direction.cpp`, `WxCombatModule.cpp`, 데이터 행 구조체 4종(`WxAbilityTableRow`/`WxEffectTableRow`/`WxCombatAttributeInitTableRow`/`WxDamageTableRow`), `Public/Minion/WxMinion.h`, `WxCombat.Build.cs`, `WxCombat.uplugin`, 호출부 확인용 `Source/WxGame/Character/WxCharacterBase.cpp`·`Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`
- **미검토 / 한계**: 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE/플로터의 BP 파생, 몽타주 노티파이 배치, TargetingPreset 태스크 구성)은 범위 밖이라, 발견 2·5·9처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해와 엔진 소스 대조로만 판단했고 PIE 실행 검증은 하지 않았다. 다음 항목들은 이번에도 발견으로 세우지 않았다 — ① `UWxLockOnComponent::ServerSetLockOnTarget`이 클라 값을 검증 없이 받는 것(`WxLockOnComponent.cpp:39` 주석이 "클라 신뢰"로 명시한 의도), ② `UWxAbilityTask_SlowTime`·`UWxAbilityTask_PlaySkillCutscene`이 전역 딜레이션 소유권을 "현재 값이 내가 건 값과 같은가"로만 판정해 겹치면 먼저 끝난 쪽이 풀어 버리는 것, ③ `UWxTargetingFilterTask_ScreenBounds`가 비로컬 컨트롤러에서 후보를 전부 걸러내는 것(현재 유일한 호출부인 `UWxAbility_LockOn`이 `IsLocallyControlled()` 뒤에서만 돌려 도달하지 않는다), ④ ExecCalc가 가드 성립을 `Effect.GuardReduction`(`WxEffect_Damage.cpp:134`)로 보는데 반응 라우팅은 `Ability.Guard`(`WxEffectComponent_DamageResponse.cpp:47,55`)로 보는 이중 기준(DamageResponse 쪽 주석이 의도임을 밝히고 있고, 두 태그가 어긋나는 저작 구성이 실제로 있는지는 BP 데이터를 봐야 한다), ⑤ `UWxLockOnPointComponent::bLockedOn`(`WxLockOnPointComponent.h:44`)이 `SetLockedOn` 외에 읽는 곳이 없는 것(`VisibleInstanceOnly`라 디테일 패널 확인이 소비처로 성립한다), ⑥ 무기·범위 대미지 노티파이가 권위 게이트 없이 전 머신에서 `ApplyDamage`를 부르는 것(예측 설계상 의도). ②④⑥은 멀티플레이 정책을 정할 때 다시 볼 지점이다. 검증 과정에서 확인한 사실 하나를 남긴다 — UE 5.8의 `FActiveGameplayEffectHandle::WasSuccessfullyApplied()`는 `bPassedFiltersAndWasExecuted`를 보므로 Instant GE에서도 `true`가 나온다(`ActiveGameplayEffectHandle.h:40-60`). `UWxCombatLibrary::ApplyDamage:129`가 Instant 대미지 GE의 성공을 이 함수로 판정하는 것은 정상 동작이며, 히트스톱·`AdditionalEffects`가 조용히 죽는 문제는 없다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 176파일 — `/module-review`로 갱신*
