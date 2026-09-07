# WxCombat — 코드 리뷰

> 건강한 모듈이다. 직전 리뷰의 🔴(소환물 스폰 좌표가 로컬 오프셋인 채 월드로 쓰이던 회귀)는 커밋 `95547202`에서 고쳐졌고, 이번 회차에 새로 뜬 심각 결함은 없다. 남은 지적은 멀티플레이 경로의 비대칭 두 건, 연출 계열의 수명·리소스 관리 두 건, 그리고 직전 리뷰에서 넘어온 중복·성능 항목이다. 프로젝트 규칙 위반은 이번에도 0건이다(모듈 의존이 `WxCore` 단일, `Wx` 접두사, 델리게이트 콜백 `Handle` 접두사 34개 전부, `BlueprintCallable` 1건이 `UWxCombatLibrary`(Blueprint Function Library), 인라인 정의·람다 0건, 저작권 첫 줄 173파일 전부 충족). 이번 리뷰는 대미지 파이프라인(ExecCalc·새 `UWxEffectComponent_DamageResponse`·AttributeSet·CombatLibrary·DamageTableRow)·어빌리티 16종·발동 그룹/캔슬 창·선입력 버퍼·무기/투사체 히트·락온·히트스톱·소환/투사체 서브시스템·GE 21종·AnimNotify 11종·GameplayCue 6종·Targeting 8종·어빌리티 태스크 5종을 헤더와 함께 읽었고, 판정이 필요한 지점은 UE 5.8 엔진 소스까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 6 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 잔상 PoseableMesh가 원시 세터로 메시를 갈아 끼워 포즈 복사가 조용히 무시된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:41`
- **범주**: 버그/정확성
- **문제**: `USkinnedMeshComponent::SetSkinnedAsset`은 포인터만 대입하는 원시 세터라 트랜스폼 버퍼 재할당(`AllocateTransformData`)도 렌더 스테이트 재생성도 하지 않는다. `UPoseableMeshComponent::AllocateTransformData`가 `RequiredBones`를 채우는 유일한 지점이고 그 호출은 컴포넌트 등록 시점의 메시로만 일어나므로, 잔상 BP가 `PoseableMesh`에 메시를 미리 지정해 두지 않았다면 등록 시점에 `RequiredBones`가 무효인 채 남는다. 그러면 바로 다음 줄 42행의 `CopyPoseFromSkeletalComponent`가 `if (RequiredBones.IsValid())` 게이트에서 아무 로그 없이 빠져나가 잔상이 포즈를 전혀 받지 못한다. BP가 다른 메시를 지정한 경우에는 렌더 스테이트가 갱신되지 않아 화면에 옛 메시가 남는다. 즉 이 코드는 "BP가 소유자와 같은 스켈레탈 메시를 이미 박아 뒀을 때만" 우연히 동작한다.
- **제안**: `SetSkinnedAssetAndUpdate()`로 바꾼다. 재할당과 렌더 스테이트 재생성을 함께 수행해 BP 저작 상태와 무관하게 성립한다.
- **확신도**: 높음(엔진 동작은 확인됨. 실제로 깨지는지는 잔상 BP의 메시 지정 여부에 달렸고 그 확인은 이번 범위 밖이다)

### 2. 🟡 무적 구간 ANS의 제거가 권위 게이트 없이 돌아 시뮬 프록시의 복제 GE를 지운다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:30`
- **범주**: 설계/구조
- **문제**: 부여 쪽(19행 → `WxCombatLibrary.cpp:166`)은 엔진의 `HasNetworkAuthorityToApplyGameplayEffect`가 예측 키 없는 머신을 자동으로 걸러 시뮬 프록시에서는 아무것도 걸리지 않는다. 반면 제거 쪽 30행 `RemoveActiveGameplayEffectBySourceEffect`에는 그런 검사가 없다 — 엔진의 `FActiveGameplayEffectsContainer::InternalRemoveActiveGameplayEffect`도 권위를 보지 않는다. 몽타주는 전 머신에 복제되므로 시뮬 프록시에서도 `NotifyEnd`가 돌고, 자기가 걸지 않은 **서버 복제본**을 로컬에서 걷어낸다. FastArray는 서버 쪽이 바뀌지 않는 한 그 항목을 다시 보내지 않으므로 `Effect.Invincible` 태그가 그 클라에서만 영구히 어긋난다. 구간의 주인이 노티파이라는 규약(08-24) 자체는 문제가 아니고, 두 반쪽의 게이트가 비대칭인 것이 문제다.
- **제안**: `NotifyEnd`에서도 부여와 같은 조건으로 게이트한다 — 소유자 권위이거나 부여를 실제로 수행한 머신일 때만 제거한다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 지금은 드러나지 않는다)

### 3. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:164`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드를 공유하지 않는다. 공격 어빌리티가 `LocalPredicted`이고 `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`) 실행 계산이 소유 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② 같은 파일 201행의 `TargetSP <= FinalDamage` 판정까지 갈려 `Damage.GuardBreak` 부여 여부가 어긋나며(이 태그가 곧 `Event.Hit.GuardBreak` 라우팅을 정한다) ③ 191행이 붙이는 `Damage.Critical`이 클라 플로터와 서버 값 사이에서 불일치한다. 직전 리뷰 이후 크리 전달 경로가 커스텀 컨텍스트에서 스펙 동적 태그로 바뀌었을 뿐 난수 자체는 그대로다.
- **제안**: 크리 판정을 권위에서만 굴려 결과를 실어 보내고 예측 측은 그 값을 읽게 하거나(비크리 낙관 예측), 예측 키·스펙 식별자를 시드로 삼는 결정적 난수로 바꾼다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 의도적으로 미룬 것일 수 있음)

### 4. 🟡 카메라 이동 ANS가 자기가 스폰한 카메라를 추적하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:69`, `:180`
- **범주**: 버그/정확성
- **문제**: 스폰한 임시 카메라를 어디에도 보관하지 않아 두 지점이 모두 추정으로 돈다. ① 69행의 안전망 수명 `TotalDuration + BlendOutTime + 1`에서 `TotalDuration`은 몽타주 PlayRate가 반영되지 않은 애니메이션 초다. ASPD 감소 등으로 PlayRate가 충분히 낮으면 구간이 끝나기 전에 뷰타겟 액터가 파괴되어 화면이 튄다 — 같은 함정을 `WxAnimNotifyState_SlowTime.cpp:25`는 PlayRate로 나눠 처리하고 있어 모듈 안에서 규칙이 갈린다. ② 180행의 `NotifyEnd`는 자기가 카메라를 실제로 세웠는지와 무관하게 무조건 `PC->GetPawn()`으로 뷰를 되돌린다. `NotifyBegin`이 스폰 실패로 빠져나갔거나 다른 연출이 뷰를 쥐고 있어도 그 뷰를 빼앗는다. 헤더 17행의 `// TODO: 게임 로직 이관 필요`도 같은 지점을 가리킨다.
- **제안**: 스폰한 카메라를 노티파이가 아닌 액터 쪽(연출 컴포넌트나 태스크)에 맡겨 수명과 뷰 복귀를 그 소유자가 판단하게 한다. 최소 조치로는 수명 계산에 PlayRate를 반영하고, `NotifyEnd`에서 현재 뷰타겟이 자기가 세운 카메라일 때만 되돌린다.
- **확신도**: 중간(①은 PlayRate가 0.6 아래로 떨어지는 구성에서만, ②는 스폰 실패·연출 중첩에서만 드러난다)

### 5. 🟡 대미지 플로터가 피격 1회마다 액터+위젯을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고(34행) `InitWidget()`으로 UMG 위젯을 새로 구성한다(60행). 액터의 `InitialLifeSpan`이 5초라(52행) 다타·다수 적 상황에서 화면당 수십 개가 동시에 살아 있게 되며, 각각이 Screen-space `UWidgetComponent`를 들고 매 프레임 그려진다. 액터 스폰과 위젯 생성은 전투 핫패스에서 가장 비싼 축이다.
- **제안**: 플로터 액터/위젯을 풀링해 재사용하거나, 수명을 애니메이션 길이에 맞춰 줄이고 동시 표시 개수 상한을 둔다.
- **확신도**: 중간(현 규모에서는 문제가 안 될 수 있음)

### 6. 🟡 콤보 어빌리티 3종이 상태와 진행 로직을 그대로 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:29`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:34`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:29`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:30,34` / `WxAbility_Skill.h:36,40` / `WxAbility_Pattern.h:31,34`), `ActivateAbility`의 인덱스 전진·`PlayMontage`·`EndAbility`의 `bWasCancelled` 리셋이 세 cpp에서 같은 문장이다. Attack과 Skill은 애셋 태그와 쿨다운 GE 지정을 빼면 구현이 완전히 같다. Pattern은 진행 방식만 다른데(재발동 대신 블렌드아웃 연쇄, `WxAbility_Pattern.cpp:48`) 복제한 `ActivateAbility`의 인덱스 전진은 그 방식에서 뜻이 없고 — Pattern은 `bRetriggerInstancedAbility`가 없어 활성 중 재발동이 배타 판정에 막힌다 — 사실상 항상 0으로 떨어진다. 남은 로직이 실제 차이를 낳는 지점도 있다: 연쇄 도중 `PlayMontage`가 실패하면(배열 중간에 빈 슬롯이 있는 경우) `bWasCancelled=false`로 끝나(`WxAbility_Pattern.cpp:56-59`) `ComboIndex`가 중간 단에 남아, 다음 발동이 첫 단이 아니라 그 다음 단부터 시작한다.
- **제안**: "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고, 재발동 방식(입력 재발동 vs 블렌드아웃 자동 전진)만 파생에 남긴다. 최소 조치로는 Pattern에서 뜻 없는 인덱스 전진을 걷어내고 실패 종료를 `bWasCancelled=true`로 바꾼다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 배선 재사용 목적의 상속은 과거에 거부된 방향이므로, 공통분이 정말 의미 단위인지 먼저 합의가 필요하다)

### 7. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:163`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다(169행). 지금 HP/SP/GP/MP/UP를 건드리는 GE는 Instant이거나 주기형(`WxEffect_RegenSP`·`DrainSP`·`DrainGP`·`InfiniteMP`)뿐이라 전부 베이스를 직접 바꾸고, 지속형 aggregator 모디파이어가 하나도 없어 베이스=현재값이다(지속형 모디파이어를 거는 `WxEffect_Exceed`·`WxEffect_MoveSpeedScale`·`WxEffect_GuardReduction`은 ATK/ASPD/SPD/GuardReductionScale만 건드려 이 쌍 목록에 없다). 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 8. 🟢 호출자 없는 선언이 셋 남아 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:159`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnPointComponent.h:42`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnPointComponent.h:44`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체(`Plugins/`·`Source/`)에 호출자가 없고 셋 다 `UFUNCTION`이 아니라 BP에서도 닿지 않는다. ① `UWxAbilityBase::GetActiveMontage()`(정의는 `WxAbilityBase.cpp:252`)와 그것만을 위해 존재하는 `ActiveMontage` 멤버, ② `UWxLockOnPointComponent::IsActorLockedOn()`(정의는 `WxLockOnPointComponent.cpp:86`), ③ 델리게이트 `OnLockedOnChanged` — `WxLockOnPointComponent.cpp:38`이 브로드캐스트하지만 구독자가 하나도 없다.
- **제안**: 셋 다 지운다. `ActiveMontage`는 `GetActiveMontage`와 함께 걷힌다.
- **확신도**: 높음

### 9. 🟢 태그 하나를 Target/Asset 양쪽에 부여하는 GE 생성자가 같은 문장을 다섯 번 되풀이한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp:13-23`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_PerfectGuard.cpp:13-23`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_SuperArmor.cpp:12-22`
- **범주**: 중복/복잡도
- **문제**: `UTargetTagsGameplayEffectComponent` + `UAssetTagsGameplayEffectComponent`를 만들어 같은 태그를 양쪽에 넣는 11줄이 태그 이름만 바꾼 채 그대로 반복된다. Invincible·PerfectGuard·SuperArmor 셋은 한 글자 차이 없이 동일하고, `WxEffect_Exhaust.cpp:14-24`·`WxEffect_GuardReduction.cpp:14-24`도 같은 블록에 다른 설정을 덧붙인 형태다. 같은 모듈의 `UWxEffect_Cooldown::GrantCooldownTag`(`WxEffect_Cooldown.cpp:27-34`)가 이미 이 모양을 보호 헬퍼로 접는 선례다.
- **제안**: `GrantCooldownTag`와 같은 자리(공통 베이스의 보호 헬퍼)에 "태그 하나를 Target·Asset 양쪽에 부여" 한 건을 두고 각 GE는 태그만 넘긴다.
- **확신도**: 낮음(반복을 용인하고 인플레이스를 우선하는 것이 이 프로젝트의 기존 방향이므로, 헬퍼가 정말 의미 단위인지 합의가 먼저다)

### 10. 🟢 README가 삭제된 타입을 아직 진입점으로 안내한다
- **위치**: `Plugins/WxCombat/README.md:36`
- **범주**: 설계/구조
- **문제**: "`UWxAbilitySystemGlobals`를 `AbilitySystemGlobalsClassName`으로 등록해야 `FWxCombatEffectContext`가 만들어진다 — 누락 시 대미지 결과가 실리지 못한다"고 적혀 있으나, `FWxCombatEffectContext`는 「대미지 판정 태그 통일」 작업에서 삭제됐고 크리 판정은 스펙 동적 애셋 태그로 옮겨졌다. 지금 `UWxAbilitySystemGlobals`가 하는 일은 큐 파라미터의 `Location`을 `ImpactPoint`로 채우는 것뿐이며(`WxAbilitySystemGlobals.cpp:17`) ini 등록이 빠졌을 때의 증상도 "대미지 결과 누락"이 아니라 "임팩트 연출이 월드 원점에서 터짐"이다. 클래스 주석은 이미 갱신됐는데 README만 남았다.
- **제안**: 해당 줄을 현재 역할로 고친다. `/readme-writer` 갱신 대상이다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnMinion.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`
- **훑은 파일**: 나머지 어빌리티(`WxAbility_Passive`/`WxAbility_PlayMontageOnce`), GE 정의 21종(`Private/AbilitySystem/Effect/*` — 각 `DurationPolicy`·`Period`·`StackingType`·모디파이어 대상과 MMC까지 확인), 나머지 AnimNotify 9종(`Private/AnimNotify/*`), 나머지 GameplayCue 4종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`), 나머지 어빌리티 태스크(`WxAbilityTask_SlowTime`/`RotateToTarget`/`WaitMoving`/`PlaySkillCutscene`), `WxAbilitySet.cpp`, `WxProjectileSubsystem.cpp`, `WxAbilityTargetData_Direction.cpp`, `WxCombatModule.cpp`, 데이터 행 구조체 4종(`WxAbilityTableRow`/`WxEffectTableRow`/`WxCombatAttributeInitTableRow`/`WxDamageTableRow`), `WxCombat.Build.cs`, `WxCombat.uplugin`
- **미검토 / 한계**: 작업 트리에 커밋 `53bc7de6` 이후의 미커밋 변경이 있어(대미지 후처리의 `UWxEffectComponent_DamageResponse` 이관, 탈진 ASC 이관, 커스텀 이펙트 컨텍스트 삭제 등) 이 리뷰는 커밋이 아니라 **디스크 상태**를 본 것이다. 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE/잔상/플로터의 BP 파생, 몽타주 노티파이 배치)은 범위 밖이라, 발견 1·6처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해와 엔진 소스 대조로만 판단했고 PIE 실행 검증은 하지 않았다. 다음 항목들은 이번에도 발견으로 세우지 않았다 — ① `UWxLockOnComponent::ServerSetLockOnTarget`이 클라 값을 검증 없이 받는 것(README가 "대상 선택은 클라 신뢰"로 명시한 의도), ② `UWxAbilityTask_SlowTime`·`UWxAbilityTask_PlaySkillCutscene`이 전역 딜레이션 소유권을 "현재 값이 내가 건 값과 같은가"로만 판정해 겹치면 먼저 끝난 쪽이 풀어 버리는 것(직전 리뷰에서도 같은 판단), ③ `UWxTargetingFilterTask_ScreenBounds`가 비로컬 컨트롤러에서 후보를 전부 걸러내는 것(현재 유일한 호출부인 `UWxAbility_LockOn`이 `IsLocallyControlled()` 뒤에서만 돌려 도달하지 않는다). 셋 다 멀티플레이 정책을 정할 때 다시 볼 지점이다.

---
*문서 기준 커밋 `53bc7de6` · 리뷰일 2026-09-08 · 소스 173파일 — `/module-review`로 갱신*
