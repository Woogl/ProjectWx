# WxCombat — 코드 리뷰

> GAS 위에 올린 전투 코어치고 권위·예측 경계, 상태 태그 수명, 어빌리티 배타 모델이 일관되게 정리되어 있고 까다로운 지점마다 근거 주석이 붙어 있어 건강하다. 직전 리뷰의 🔴 1건과 🟡·🟢 3건은 모두 수정됐다. 이번 리뷰는 README·`WxCombat.Build.cs`·`uplugin`·전체 public 헤더를 훑은 뒤 `c486a5c7..HEAD` 변경 파일 전부와 대미지 파이프라인(ExecCalc·AttributeSet·CombatLibrary)·ASC/어빌리티 베이스·무기/투사체·락온·선입력·히트스톱·AnimNotify·어빌리티 태스크의 cpp를 깊게 읽었고, 애매한 GAS 동작(Instant GE 핸들 반환, 비권위 GE 적용 거부, `RemoveActiveGameplayEffectBySourceEffect`)은 UE 5.8 엔진 소스로 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `WxAnimNotifyState_CameraMove`의 NotifyEnd가 자기 카메라인지 보지 않고 뷰 타겟을 폰으로 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:159-180` (스폰·설정은 `:50`, `:69`, `:71`)
- **범주**: 버그/정확성
- **문제**: `NotifyEnd`는 게이트만 통과하면 무조건 `PC->SetViewTargetWithBlend(PC->GetPawn(), ...)`를 부른다. 지금 뷰 타겟이 자기가 스폰한 카메라인지, 애초에 `NotifyBegin`이 카메라를 세우는 데 성공했는지 확인하지 않는다. 구체적 실패 경로 둘:
  - 이 노티파이는 `OwnerPawn->IsPlayerControlled() && !IsLocallyControlled()`만 걸러내므로 **모든 AI 몽타주가 로컬 플레이어의 뷰 타겟을 가져간다**(`:31-34`, `:167-170`, 의도된 설계). 적 A의 구간이 [t0,t2], 적 B의 구간이 [t1,t3]로 겹치면 t2에서 A의 `NotifyEnd`가 B의 연출 중인 카메라를 끊고 뷰를 폰으로 되돌린다. B의 카메라는 lifespan(`:69`)까지 고아로 남는다.
  - `PC->GetPawn()`이 널이면(사망·언포제스 직후) `APlayerController::SetViewTarget`이 뷰 타겟을 컨트롤러 자신으로 대체한다. 사망 몽타주에 이 노티파이가 얹히면 카메라가 엉뚱한 곳으로 튄다.
  - 부수적으로 `SetLifeSpan(TotalDuration + ...)`은 애니메이션 시간 기준이라 재생 속도가 1이 아니면(ASPD·히트스톱) 실사용 중에 카메라가 먼저 사라진다.
- **제안**: ANS 오브젝트는 인스턴스 간 공유라 상태를 못 들고 있으므로, 스폰한 카메라를 소유 액터 쪽(컴포넌트나 태그)에 걸어 두고 `NotifyEnd`에서 `PC->GetViewTarget()`이 그 카메라일 때만 복원한다. 복원 대상도 `GetPawn()`이 널이면 건너뛴다. 헤더의 `// TODO: 게임 로직 이관 필요`(`Public/AnimNotify/WxAnimNotifyState_CameraMove.h:17`)를 처리할 때 함께 정리할 지점이다.
- **확신도**: 중간 (겹침 시나리오는 데이터 배치에 달렸으나, 널 폰 경로와 소유권 미검사는 코드상 확정)

### 2. 🟡 투사체가 비권위 머신에서 멈추지 않아 RTT 동안 계속 날며 임팩트 연출을 더 낸다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:104-107`, `:135-138`, `:150-155`
- **범주**: 설계/구조
- **문제**: 히트 연출(`PlayImpactFX`)을 각 머신이 로컬로 내는 것은 헤더에 명시된 의도지만, 콜리전·이동을 끄는 처리는 없고 `Destroy()`만 `HasAuthority()` 뒤에 있다. 비권위 클라이언트의 투사체는 서버의 파괴가 복제되어 올 때까지 계속 날아가며, 그 사이 지나치는 다른 적대 액터마다 `HandleHitCollisionOverlap`이 다시 발화해 임팩트 FX가 엉뚱한 자리에서 반복된다(대미지는 권위 게이트와 GAS의 비권위 적용 거부에 이중으로 막혀 안전하다). 같은 이유로 `HandleHitCollisionHit`(지형 블록)에서도 클라 화면에서는 투사체가 벽에 부딪힌 뒤 그대로 통과해 지나간다.
- **제안**: 히트가 성립해 파괴가 확정된 시점에 각 머신이 로컬로 `SetActorEnableCollision(false)` + 이동 비활성 + 메시 숨김까지 하고, 실제 정리는 지금처럼 권위의 `Destroy()` 복제에 맡긴다.
- **확신도**: 중간

### 3. 🟡 SnapToTarget의 AI 포커스 대상이 서버에만 있어 머신마다 다른 적을 향해 워프한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp:44-51`, `:70-74`, `:92-95`
- **범주**: 설계/구조
- **문제**: `AIController::GetFocusActor()`로 대상을 집는 새 경로는 컨트롤러가 있는 서버에서만 성립하고, 클라이언트의 시뮬레이티드 프록시는 `TargetingResults[0]` 폴백을 탄다(코드 주석이 인정하는 사실). 그런데 위치 워프 제한(`bRequireLockOnForTranslation`)은 `IsPlayerControlled()`일 때만 걸리므로 **AI는 위치 워프가 그대로 켜진 채** 서버와 클라가 서로 다른 액터를 향해 루트 모션을 보정한다. 변경 전에는 양쪽 다 프리셋 결과를 써서 대체로 일치했으므로, 이번 변경이 만든 새 갈라짐이다. 헤더 주석(`Public/Targeting/WxRootMotionModifier_SnapToTarget.h:16`)은 여전히 "플레이어 폰의 위치 워프만 제한해 멀티플레이 디싱크를 막는다"고 적혀 있어 현재 동작과 어긋난다.
- **제안**: 워프 대상이 서버 전용 정보(`GetFocusActor`)에서 왔을 때는 회전만 허용하고 위치 워프를 끄거나, AI 대상도 복제되는 값(락온 컴포넌트 등)으로 통일한다. 어느 쪽도 아니라면 최소한 헤더 주석을 현재 동작에 맞게 고친다.
- **확신도**: 중간 (시뮬 프록시 위치는 이동 복제가 결국 보정하므로 영향은 시각적 튐에 한정될 수 있다)

### 4. 🟡 콤보 어빌리티 3종이 같은 상태 관리 코드를 그대로 복제하고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:24-58`, `.../WxAbility_Pattern.cpp:19-46` (필드도 각각 `WxAbility_Attack.h:34,38`, `WxAbility_Skill.h:37,41`, `WxAbility_Pattern.h:31,34`에 따로 선언)
- **범주**: 중복/복잡도
- **문제**: `ActivateAbility`(커밋 → `ComboIndex` 순환 → `PlayMontage` → 실패 시 종료)와 `EndAbility`(취소 시 `ComboIndex = INDEX_NONE`)가 클래스명만 다른 동일 코드로 세 번 존재하고, `ComboMontages`/`ComboIndex` 선언까지 각자 들고 있다. 콤보 진행 규칙을 한 곳만 고치면 나머지 둘이 조용히 어긋난다. 이미 리셋 방식이 갈라져 있다 — Attack·Skill은 `HandleMontageCompleted`에서 명시적으로 `INDEX_NONE`으로 되돌리는데 Pattern은 그 오버라이드가 없어 `ActivateAbility`의 순환(`IsValidIndex(ComboIndex + 1)` 실패 시 0)에 기대고 있다. 지금은 결과가 같지만, `ComboMontages` 중간에 널 항목이 있어 `WxAbility_Pattern.cpp:58`의 `bWasCancelled=false` 종료로 빠지면 Pattern만 중간 인덱스를 물고 다음 발동이 첫 단부터 시작하지 않는다.
- **제안**: `UWxAbilityBase`와 세 클래스 사이에 `ComboMontages`/`ComboIndex`와 순환·리셋 규칙만 담은 중간 베이스(예: `UWxAbility_ComboMontage`)를 두고, Pattern은 블렌드아웃 체이닝(`HandleMontageBlendOut`)만 오버라이드한다.
- **확신도**: 높음

### 5. 🟢 락온 종료 시 `bOrientRotationToMovement` 복원이 무관한 ASC 유효성 조건 안에 갇혀 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:105-125` (저장은 `:45-51`)
- **범주**: 설계/구조
- **문제**: CMC의 `bOrientRotationToMovement` 저장·비활성화는 `ActivateAbility`에서 `IsLocallyControlled` 게이트보다 **앞**에서 무조건 수행되는데(`:45-51`), `EndAbility`의 복원만 `if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())` 블록 안에 들어 있다. 이동 컴포넌트 복원은 ASC 유효성과 아무 관련이 없어 조건이 비대칭이고, 조건이 깨지는 경로가 생기면 캐릭터가 영구히 이동 방향으로 회전하지 않는 상태로 남는다.
- **제안**: `SavedOrientRotationToMovement` 복원 블록을 ASC 조건 밖으로 빼서 저장 지점과 같은 조건(=`ActorInfo`만)으로 맞춘다.
- **확신도**: 중간 (현재 ASC는 캐릭터 서브오브젝트라 실제로 조건이 깨지기는 어렵다)

### 6. 🟢 `AdjustCurrentAttributeForMaxChange`가 현재값을 읽어 베이스값에 쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:188-194`
- **범주**: 버그/정확성
- **문제**: `ASC->GetNumericAttribute(Pair->Attribute)`는 모디파이어가 반영된 **현재값**을 주는데, 그 값을 비례 스케일한 뒤 `SetNumericAttributeBase`로 **베이스**에 기록한다. HP·SP 등에 Infinite GE 모디파이어가 붙은 상태에서 Max가 한 번이라도 바뀌면 그 모디파이어분이 베이스에 그대로 눌러앉고, 애그리게이터는 다시 그 위에 모디파이어를 얹어 값이 이중 적용된다. 지금은 현재값 계열 어트리뷰트에 모디파이어를 얹는 GE가 없어 드러나지 않는다(모두 `SetHP`/`SetNumericAttributeBase` 경로로만 바뀐다).
- **제안**: `GetNumericAttributeBase`로 읽어 베이스끼리 비례 스케일한다.
- **확신도**: 중간 (현재 데이터에서는 재현되지 않는 잠재 결함)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `.../Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_AddAttribute.cpp`, `.../Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `.../Private/AbilitySystem/WxAbilitySet.cpp`, `.../Private/AbilitySystem/WxInputBufferComponent.cpp`, `.../Private/AbilitySystem/WxHitStopComponent.cpp`, `.../Private/Weapon/WxWeaponBase.cpp`, `.../Private/Weapon/WxProjectileBase.cpp`, `.../Private/Weapon/WxProjectileManagerComponent.cpp`, `.../Private/Targeting/WxLockOnComponent.cpp`, `.../Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `.../Private/Targeting/WxLockOnPointComponent.cpp`, `.../Private/Minion/WxMinionManagerComponent.cpp`, `.../Private/Finisher/WxFinisherDamageComponent.cpp`, `.../Private/AbilitySystem/Ability/` 전체(Attack·Skill·Pattern·Dodge·Guard·GuardReact·HitReact·Groggy·Death·Finisher·Sprint·Ultimate·Passive·PlayMontageOnce·LockOn), `.../Private/AbilitySystem/Task/` 전체, `.../Private/AnimNotify/` 전체, `.../Private/Damage/`, `.../Private/AbilitySystem/Effect/` 전체
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Public/` 전체 헤더, `.../Private/AbilitySystem/Cue/` 전체, `.../Private/Targeting/WxTargetingFilterTask_*`·`WxTargetingSorterTask_*`, `Source/WxGame/Character/WxCharacterBase.cpp`(`GiveAbilitySet` 권위 게이트 확인), `Source/WxGame/Cheat/WxCheatManager.cpp`(`UWxEffect_AddIncomingDamage` 호출부 확인)
- **미검토 / 한계**:
  - BP/WBP 자산(어빌리티·GE 파생 BP, 몽타주 노티파이 배치, `FWxDamageTableRow`/`FWxAbilityTableRow`/`FWxEffectTableRow` 실제 데이터 행)은 범위 밖이라 데이터 오설정으로만 드러나는 결함은 잡히지 않는다.
  - `WxAnimNotifyState_CameraMove`의 `WITH_EDITOR` 프리뷰 경로는 에디터 전용이라 얕게만 봤다.
  - 규칙 스캔은 전 파일 자동 검사로 돌렸고(카피라이트 첫 줄·`Wx` prefix·`FORCEINLINE`/인라인 정의·람다·`BlueprintCallable`·델리게이트 콜백 `Handle` prefix·override의 `Super::` 누락) **CLAUDE.md 위반은 하나도 나오지 않았다**. 유일한 `BlueprintCallable`(`Public/WxCombatLibrary.h:45`)은 Blueprint Function Library 소속이라 허용 범위이고, `Super::`를 부르지 않는 override는 전부 베이스가 비어 있거나(순수 getter·MMC·Targeting Task·`IModuleInterface`) 의도적으로 종료를 막는 지점(`WxAbility_Death`·`WxAbility_Guard`의 몽타주 핸들러)이다. `Build.cs`·`uplugin` 의존도 `WxCore` 외 다른 Wx 플러그인을 참조하지 않는다.
  - 대미지 GE의 크리 판정이 ExecCalc 안의 `FMath::FRand()`(`WxEffect_Damage.cpp:168`)라 예측하는 소유 클라와 서버의 굴림이 갈리지만, "예측 장치를 새로 만들지 않는다"는 기존 결정에 따른 것으로 보아 발견으로 올리지 않았다.

---
*문서 기준 커밋 `3d9e73c0` · 리뷰일 2026-09-04 · 소스 169파일 — `/module-review`로 갱신*
