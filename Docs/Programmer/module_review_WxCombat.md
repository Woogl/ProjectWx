# WxCombat — 코드 리뷰

> 건강한 모듈이다. 직전 리뷰의 🔴은 없었고 그때 지적된 두 건(잔상 `SetSkinnedAsset` 원시 세터, 무적 ANS 제거의 권위 게이트 누락)은 이번 코드에서 이미 고쳐져 있다. 남은 지적은 예측 경로의 난수 비결정성, 연출 계열의 수명·리소스 관리, 그리고 어트리뷰트·데드코드·중복의 소소한 항목들이다. 프로젝트 규칙 위반은 이번에도 0건이다(모듈 의존이 `WxCore` 단일, `Wx` 접두사, 델리게이트 콜백 34개 전부 `Handle` 접두사, `BlueprintCallable` 1건이 `UWxCombatLibrary`(Blueprint Function Library), 인라인 정의·람다 0건, 저작권 첫 줄 173파일 전부 충족). 이번 리뷰는 대미지 파이프라인(ExecCalc·`UWxEffectComponent_DamageResponse`·AttributeSet·CombatLibrary·DamageTableRow)·어빌리티 16종·발동 그룹/캔슬 창·선입력 버퍼·무기/투사체 히트·락온·히트스톱·소환/투사체 서브시스템·GE 21종·AnimNotify 12종·GameplayCue 6종·Targeting 8종·어빌리티 태스크 5종을 헤더와 함께 읽었고, 판정이 갈리는 지점은 UE 5.8 엔진 소스까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:164`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드를 공유하지 않는다. `UWxAbilityBase` 생성자가 모든 어빌리티를 `LocalPredicted`로 깔고(`WxAbilityBase.cpp:22`), `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`) 이 실행 계산은 소유 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② 같은 파일 201행의 `TargetSP <= FinalDamage` 판정까지 갈려 `Damage.GuardBreak` 부여 여부가 어긋나며(이 태그가 곧 `Event.Hit.GuardBreak` 라우팅을 정한다) ③ 191행이 붙이는 `Damage.Critical`이 클라 플로터와 서버 값 사이에서 불일치한다.
- **제안**: 크리 판정을 권위에서만 굴려 결과를 실어 보내고 예측 측은 그 값을 읽게 하거나(비크리 낙관 예측), 예측 키·스펙 식별자를 시드로 삼는 결정적 난수로 바꾼다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 의도적으로 미룬 것일 수 있음)

### 2. 🟡 카메라 이동 ANS가 자기가 스폰한 카메라를 추적하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:69`, `:180`
- **범주**: 버그/정확성
- **문제**: 스폰한 임시 카메라를 어디에도 보관하지 않아 두 지점이 모두 추정으로 돈다. ① 69행의 안전망 수명 `TotalDuration + BlendOutTime + 1`에서 `TotalDuration`은 몽타주 PlayRate가 반영되지 않은 애니메이션 초다. ASPD 감소 등으로 PlayRate가 충분히 낮으면 구간이 끝나기 전에 뷰타겟 액터가 파괴되어 화면이 튄다 — 같은 함정을 `WxAnimNotifyState_SlowTime.cpp:25`는 `Montage_GetPlayRate`로 나눠 처리하고 있어 모듈 안에서 규칙이 갈린다. ② 180행의 `NotifyEnd`는 자기가 카메라를 실제로 세웠는지와 무관하게 무조건 `PC->GetPawn()`으로 뷰를 되돌린다. `NotifyBegin`이 스폰 실패(`:51`)로 빠져나갔거나 다른 연출이 뷰를 쥐고 있어도 그 뷰를 빼앗는다.
- **제안**: 스폰한 카메라를 노티파이가 아닌 액터 쪽(연출 컴포넌트나 태스크)에 맡겨 수명과 뷰 복귀를 그 소유자가 판단하게 한다. 최소 조치로는 수명 계산에 PlayRate를 반영하고, `NotifyEnd`에서 현재 뷰타겟이 자기가 세운 카메라일 때만 되돌린다.
- **확신도**: 중간(①은 PlayRate가 낮은 구성에서만, ②는 스폰 실패·연출 중첩에서만 드러난다)

### 3. 🟡 대미지 플로터가 피격 1회마다 액터+위젯을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고(34행) `InitWidget()`으로 UMG 위젯을 새로 구성한다(60행). 액터의 `InitialLifeSpan`이 5초라(52행) 다타·다수 적 상황에서 화면당 수십 개가 동시에 살아 있게 되며, 각각이 Screen-space `UWidgetComponent`를 들고 매 프레임 그려진다. 액터 스폰과 위젯 생성은 전투 핫패스에서 가장 비싼 축이다.
- **제안**: 플로터 액터/위젯을 풀링해 재사용하거나, 수명을 애니메이션 길이에 맞춰 줄이고 동시 표시 개수 상한을 둔다.
- **확신도**: 중간(현 규모에서는 문제가 안 될 수 있음)

### 4. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:163`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다(169행). 지금 HP/SP/GP/MP/UP를 건드리는 GE는 Instant이거나 주기형(`WxEffect_RegenSP`·`DrainSP`·`DrainGP`·`InfiniteMP`)뿐이라 전부 베이스를 직접 바꾸고, 지속형 aggregator 모디파이어가 하나도 없어 베이스=현재값이다(지속형 모디파이어를 거는 `WxEffect_Exceed`·`WxEffect_MoveSpeedScale`·`WxEffect_GuardReduction`은 ATK/ASPD/SPD/GuardReductionScale만 건드려 이 쌍 목록에 없다). 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 5. 🟢 호출자 없는 선언이 셋 남아 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:159`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnPointComponent.h:42`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnPointComponent.h:44`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체(`Plugins/`·`Source/`)에 호출자가 없고 셋 다 `UFUNCTION`이 아니라 BP에서도 닿지 않는다. ① `UWxAbilityBase::GetActiveMontage()`(정의는 `WxAbilityBase.cpp:252`)와 그것만을 위해 존재하는 `ActiveMontage` 멤버(`WxAbilityBase.h:180`), ② `UWxLockOnPointComponent::IsActorLockedOn()`(정의는 `WxLockOnPointComponent.cpp:86`), ③ 델리게이트 `OnLockedOnChanged` — `WxLockOnPointComponent.cpp:38`이 브로드캐스트하지만 구독자가 하나도 없다. ②를 지우면 `IsLockedOn()`의 유일한 호출자도 함께 사라져, 지점 컴포넌트의 `bLockedOn`은 `WxAbilityTask_LockOnCamera`가 쓰기만 하고 아무도 읽지 않는 상태가 된다 — 표시 용도가 BP에도 없다면 그 플래그까지 함께 정리 대상이다.
- **제안**: 셋 다 지운다. `ActiveMontage`는 `GetActiveMontage`와 함께 걷히고, `bLockedOn`은 실제 소비처(에디터 표시 등)가 있는지 확인한 뒤 판단한다.
- **확신도**: 높음

### 6. 🟢 콤보 어빌리티 3종이 상태와 진행 로직을 그대로 복제하고, 그중 Pattern만 실패 경로에서 인덱스가 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-53`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24-58`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:56`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:30,34` / `WxAbility_Skill.h:36,40` / `WxAbility_Pattern.h:30,33`), Attack과 Skill은 `ActivateAbility`·`EndAbility`·`HandleMontageCompleted` 세 함수가 한 글자 차이 없이 같다(차이는 생성자의 애셋 태그와 쿨다운 GE 지정뿐). Pattern은 진행 방식만 다른데(재발동 대신 블렌드아웃 연쇄) 복제한 `ActivateAbility`의 인덱스 전진은 그대로 가져왔고, 정상 종료 시 리셋(`HandleMontageCompleted` 오버라이드)만 빠져 있다. 그 결과 연쇄 도중 `PlayMontage`가 실패하면(배열 중간에 빈 슬롯이 있는 경우) 56-59행이 `bWasCancelled=false`로 끝내 `ComboIndex`가 중간 단에 남고, 다음 발동이 첫 단이 아니라 그 다음 단부터 시작한다.
- **제안**: 최소 조치로 Pattern의 실패 종료를 `bWasCancelled=true`로 바꾼다(그러면 `EndAbility`의 기존 리셋이 걸린다). 구조 조치로는 "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고 재발동 방식만 파생에 남긴다.
- **확신도**: 중간(인덱스 잔류는 확실. 공통분 추출은 배선 재사용 목적의 상속이 과거에 거부된 방향이라 합의가 먼저다)

### 7. 🟢 README가 삭제된 타입을 아직 진입점 규약으로 안내한다
- **위치**: `Plugins/WxCombat/README.md:36`
- **범주**: 설계/구조
- **문제**: "`UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`으로 등록해야 `FWxCombatEffectContext`가 만들어진다 — 누락 시 대미지 결과가 실리지 못한다"고 적혀 있으나, `FWxCombatEffectContext`는 저장소 어디에도 없다(이 README 한 줄이 유일한 언급이다). 지금 `UWxAbilitySystemGlobals`가 하는 일은 큐 파라미터의 `Location`을 `ImpactPoint`로 채우는 것뿐이며(`WxAbilitySystemGlobals.cpp:17`), ini 등록이 빠졌을 때의 증상도 "대미지 결과 누락"이 아니라 "임팩트 연출이 월드 원점에서 터짐"이다. 크리 판정은 스펙 동적 애셋 태그(`WxEffect_Damage.cpp:191`)로 옮겨져 있다.
- **제안**: 해당 줄을 현재 역할로 고친다. `/readme-writer` 갱신 대상이다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`
- **훑은 파일**: 나머지 어빌리티(`WxAbility_Passive`/`WxAbility_PlayMontageOnce`), GE 정의 21종(`Private/AbilitySystem/Effect/*` — 각 `DurationPolicy`·`Period`·`StackingType`·모디파이어 대상과 MMC까지 확인), 나머지 AnimNotify 8종(`Private/AnimNotify/*`), 나머지 GameplayCue 4종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`), 나머지 어빌리티 태스크(`WxAbilityTask_SlowTime`/`RotateToTarget`/`WaitMoving`), `WxAbilitySet.cpp`, `WxAbilitySystemGlobals.cpp`, `WxProjectileSubsystem.cpp`, `WxAbilityTargetData_Direction.cpp`, `WxCombatModule.cpp`, 데이터 행 구조체 4종(`WxAbilityTableRow`/`WxEffectTableRow`/`WxCombatAttributeInitTableRow`/`WxDamageTableRow`), `WxCombat.Build.cs`, `WxCombat.uplugin`, 호출부 확인용 `Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**: 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE/플로터의 BP 파생, 몽타주 노티파이 배치)은 범위 밖이라, 발견 2·6처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해와 엔진 소스 대조로만 판단했고 PIE 실행 검증은 하지 않았다. 다음 항목들은 이번에도 발견으로 세우지 않았다 — ① `UWxLockOnComponent::ServerSetLockOnTarget`이 클라 값을 검증 없이 받는 것(헤더 22행이 "클라 신뢰"로 명시한 의도), ② `UWxAbilityTask_SlowTime`·`UWxAbilityTask_PlaySkillCutscene`이 전역 딜레이션 소유권을 "현재 값이 내가 건 값과 같은가"로만 판정해 겹치면 먼저 끝난 쪽이 풀어 버리는 것, ③ `UWxTargetingFilterTask_ScreenBounds`가 비로컬 컨트롤러에서 후보를 전부 걸러내는 것(현재 유일한 호출부인 `UWxAbility_LockOn`이 `IsLocallyControlled()` 뒤에서만 돌려 도달하지 않는다), ④ `UWxEffectComponent_DamageResponse`가 비권위 머신에서도 `Event.Hit`/`Event.DamageDealt`를 발행해 ServerInitiated·ServerOnly 어빌리티가 엔진 로그를 남기는 것(동작은 정상). 넷 다 멀티플레이 정책을 정할 때 다시 볼 지점이다. 검증 과정에서 확인한 사실 하나를 남긴다 — `FGameplayCueParameters::AggregatedSourceTags`는 UE 5.8의 `NetSerialize`에서 RepBits와 무관하게 항상 직렬화되므로, 플로터가 태그로 실어 보내는 크리 표시는 원격 머신에도 정상 도달한다.

---
*문서 기준 커밋 `262e4cca` · 리뷰일 2026-09-08 · 소스 173파일 — `/module-review`로 갱신*
