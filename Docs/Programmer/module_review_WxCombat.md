# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 코어치고 상태 관리·리플리케이션 권위·예측 경계가 일관되게 정리되어 있고, 까다로운 지점마다 근거 주석이 붙어 있어 전반적으로 건강하다. 이번 리뷰는 README·`WxCombat.Build.cs`·전체 public 헤더를 훑은 뒤 어빌리티 베이스/ASC/대미지 파이프라인(ExecCalc·AttributeSet)/무기·투사체/락온/선입력·히트스톱/AnimNotify·태스크의 cpp를 깊게 읽었고, 애매한 GAS 동작은 UE 5.8 엔진 소스로 직접 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `ApplyAttributeChange`가 매 호출마다 고정 이름으로 런타임 `UGameplayEffect`를 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:175-186`
- **범주**: 성능/안전 (객체 수명주기)
- **문제**: `NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("ApplyAttributeChange")))` 는 이름을 명시했으므로, 두 번째 호출부터 `StaticAllocateObject`의 "같은 이름 객체 교체" 경로로 들어간다(UE 5.8 `UObjectGlobals.cpp:3568` "Replace an existing object without affecting the original's address or index"). 즉 직전 호출이 만든 GE를 **같은 주소에서 소멸자까지 돌려 재구축**한다. 이 GE는 어디서도 참조되지 않으므로 GC가 곧 Unreachable로 표시하는데, 표시 시점과 실제 purge 사이에 두 번째 호출이 겹치면 그 경로의 `check(!Obj->IsUnreachable())`에 걸려 Development 빌드가 죽고, Shipping에서는 회수 중인 UObject를 덮어쓴다. 유일한 호출부가 `WxCombatAttributeSet.cpp:366`(퍼펙트가드 패리 GP 환급)이라 평범한 전투 중 반복 호출되는 경로다. 부수적으로 지역 변수 이름 `InfoXP`가 다른 시스템에서 복사된 잔재로 남아 있다.
- **제안**: 최소한 이름을 `NAME_None`으로 바꿔 매번 유니크 이름을 받게 한다. 더 나은 방향은 이 모듈이 이미 쓰는 패턴 — `UWxEffect_ResetGP`·`UWxEffect_HitStop`처럼 SetByCaller 크기를 받는 CDO 기반 GE 하나를 추가하고 `ApplyAttributeChange`를 그쪽으로 돌리는 것이다(런타임 GE 생성 자체가 UE 5.x에서 권장되지 않는다).
- **확신도**: 중간 (교체 경로 자체는 엔진 소스로 확인했으나, 실제 크래시는 GC 타이밍이 겹쳐야 한다)

### 2. 🟡 `MaxMinionCount`가 0이면 소환 시 배열 범위를 넘어선다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp:131-140` (필드: `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionComponent.h:34`)
- **범주**: 버그/정확성
- **문제**: `MinionCountToRemove = FMath::Max(ActiveMinions.Num() - MaxMinionCount + 1, 0)` 는 `MaxMinionCount >= 1`을 암묵 전제한다. 기획자가 BP에서 0(또는 음수)을 넣으면 소환수가 하나도 없는 상태에서도 제거 횟수가 1 이상이 되어 `ActiveMinions[0]`이 빈 배열을 인덱싱한다. `MaxMinionCount`에는 `ClampMin` 메타가 없어 디테일 패널에서 그대로 입력된다.
- **제안**: `UPROPERTY`에 `meta = (ClampMin = "1")`을 붙이거나, 루프 상한을 `FMath::Min(MinionCountToRemove, ActiveMinions.Num())`으로 잘라 데이터에 관계없이 안전하게 만든다.
- **확신도**: 높음

### 3. 🟡 콤보 어빌리티 3종이 같은 상태 관리 코드를 그대로 복제하고 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:24-58`, `.../WxAbility_Pattern.cpp:19-46` (헤더도 각각 `ComboMontages`·`ComboIndex`를 따로 선언)
- **범주**: 중복/복잡도
- **문제**: `ActivateAbility`(커밋 → `ComboIndex` 순환 → `PlayMontage`), `EndAbility`(취소 시 `ComboIndex = INDEX_NONE`), `HandleMontageCompleted`(인덱스 리셋)이 세 파일에서 클래스명만 다른 동일 코드다. 콤보 진행 규칙을 하나만 고치면 나머지 둘이 조용히 어긋난다. 실제로 이미 갈라진 곳이 있다 — `UWxAbility_Pattern`은 `HandleMontageCompleted`를 재정의하지 않아, 마지막 단이 아닌 지점에서 `bWasCancelled=false`로 끝나면(`WxAbility_Pattern.cpp:58`) `ComboIndex`가 중간값으로 남아 다음 발동이 첫 단이 아닌 곳부터 이어진다.
- **제안**: `UWxAbilityBase`와 세 클래스 사이에 `ComboMontages`/`ComboIndex`와 순환·리셋 규칙만 담은 중간 베이스(예: `UWxAbility_ComboMontage`)를 두고, Pattern은 블렌드아웃 체이닝만 오버라이드한다.
- **확신도**: 높음

### 4. 🟡 투사체가 비권위 머신에서는 멈추지 않아 RTT 동안 계속 날며 임팩트 연출을 더 낸다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:104`, `:135-138`
- **범주**: 설계/구조
- **문제**: 히트 연출(`PlayImpactFX`)은 각 머신이 로컬로 내지만(의도된 설계), `Destroy()`는 `HasAuthority()` 게이트 뒤에만 있다. 비권위 클라이언트의 투사체는 서버의 파괴가 복제되어 올 때까지 계속 이동하며, 그 사이 지나치는 다른 액터마다 `HandleHitCollisionOverlap`이 다시 발화해 임팩트 FX가 엉뚱한 자리에서 한 번 더 난다(대미지는 권위 게이트에 막혀 안전하다). 같은 이유로 `HandleHitCollisionHit`(지형 충돌)도 클라에서 투사체가 벽을 통과해 지나가는 것처럼 보인다.
- **제안**: 히트가 성립해 파괴가 확정된 시점에 각 머신이 로컬로도 콜리전과 이동을 끄고 메시를 숨기고(권위만 `Destroy()`), 서버 파괴 복제가 실제 정리를 맡게 한다.
- **확신도**: 중간

### 5. 🟡 극한 회피·퍼펙트가드·궁극기 슬로모션이 월드 전역 시간을 늦춘다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:63-69`, `.../WxAbilityTask_PlaySkillCutscene.cpp:58-64` (호출부: `.../Ability/WxAbility_Dodge.cpp:205`, `.../Ability/WxAbility_GuardReact.cpp:113`)
- **범주**: 설계/구조 (리플리케이션 권위)
- **문제**: 두 태스크 모두 `IsOwnerActorAuthoritative()`일 때만 `UGameplayStatics::SetGlobalTimeDilation`을 건다. `AWorldSettings::TimeDilation`은 복제되므로, 한 플레이어의 극한 회피/퍼펙트가드/궁극기가 서버 월드 전체를 느리게 만들어 접속한 모든 플레이어에게 걸린다. 개인 연출로 의도했다면 아바타 단위 `CustomTimeDilation` 또는 로컬 전용 처리여야 한다.
- **제안**: 멀티 대응이 필요하면 발동 플레이어의 폰·카메라에만 걸리는 방식으로 좁힌다. 전역 슬로모션이 의도라면 그 사실을 태스크 주석에 못박아 두고, 소유 클라 한정으로 걸지 서버가 걸지도 함께 기록한다.
- **확신도**: 낮음 (의도된 설계일 수 있음 — `WxAbility_GuardReact.cpp:110`의 "복제된 딜레이션" 주석은 복제를 인지하고 있음을 보여준다)

### 6. 🟢 락온 종료 시 `bOrientRotationToMovement` 복원이 무관한 ASC 유효성 조건 안에 갇혀 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:105-125`
- **범주**: 설계/구조
- **문제**: CMC의 `bOrientRotationToMovement` 저장·복원은 `ActivateAbility`에서는 `IsLocallyControlled` 게이트 **앞**(`:46-51`)에서 무조건 수행되는데, `EndAbility`의 복원만 `if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())` 블록 안에 들어 있다. 이동 컴포넌트 복원은 ASC 유효성과 아무 관련이 없어 조건이 대칭이 아니고, 조건이 깨지는 경로가 생기면 캐릭터가 영구히 이동 방향으로 회전하지 않는 상태로 남는다.
- **제안**: `SavedOrientRotationToMovement` 복원 블록을 ASC 조건 밖으로 빼서 저장 지점과 같은 조건으로 맞춘다.
- **확신도**: 중간 (현재 ASC는 캐릭터 서브오브젝트라 실제로 조건이 깨지기는 어렵다)

### 7. 🟢 `PreviousMontageTickOption`이 초기화되지 않은 채 선언되어 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:58`
- **범주**: 버그/정확성
- **문제**: `EVisibilityBasedAnimTickOption PreviousMontageTickOption;`에 초기값이 없다. 현재는 `MontageTickMesh`와 항상 한 쌍으로 대입되고 `RestoreAnimatingMontageMeshTick`이 메시 널 검사로 조기 반환하므로 읽히지 않지만, 그 불변식이 깨지면 메시 틱 정책에 쓰레기 값이 들어간다.
- **제안**: `= EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` 등 기본값을 명시한다.
- **확신도**: 높음

### 8. 🟢 `ApplyEffect`가 어빌리티 없이 호출되면 GE 스펙을 Level 0으로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:156`
- **범주**: 버그/정확성
- **문제**: `const float Level = PredictingAbility ? PredictingAbility->GetAbilityLevel() : 0.f;` — GAS 관례상 어빌리티 레벨은 1부터이고, 이 모듈의 다른 스펙 생성부는 모두 `1.f`를 넘긴다(`WxDamageTableRow.cpp:17`, `WxEffect_HitStop.cpp:42`, `WxEffect_Exhaust.cpp:44`). 지금 이 경로로 걸리는 GE(무적·슈퍼아머)는 레벨 스케일 수치가 없어 무해하지만, 나중에 `FScalableFloat` 커브를 쓰는 GE를 이 통로에 태우면 레벨 0 구간을 읽어 조용히 다른 값이 나온다.
- **제안**: 폴백을 `1.f`로 바꾼다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `.../Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../Private/Weapon/WxWeaponBase.cpp`, `.../Private/Weapon/WxProjectileBase.cpp`, `.../Private/Targeting/WxLockOnComponent.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `.../Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `.../Private/AbilitySystem/WxInputBufferComponent.cpp`, `.../Private/AbilitySystem/WxHitStopComponent.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `.../Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `.../Private/Minion/WxMinionComponent.cpp`, `.../Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `.../Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Public/` 전체 헤더, 나머지 `WxEffect_*`·`WxCueNotify_*`·`WxTargetingFilterTask_*`·`WxAnimNotify*` 구현부, `Source/WxGame/Character/WxCharacterBase.cpp`(`GiveAbilitySet` 호출부 권위 확인용)
- **미검토 / 한계**:
  - BP/WBP 자산(어빌리티·GE 파생 BP, 몽타주 노티파이 배치, `FWxDamageTableRow`/`FWxAbilityTableRow` 실제 데이터 행)은 범위 밖이라 데이터 오설정으로만 드러나는 결함은 잡히지 않는다.
  - `WxAnimNotifyState_CameraMove`의 `WITH_EDITOR` 프리뷰 경로는 에디터 전용이라 얕게만 봤다.
  - 규칙 스캔은 전 파일 자동 검사로 돌렸고(카피라이트 첫 줄·`Wx` prefix·`FORCEINLINE`/인라인 정의·람다·`BlueprintCallable`·델리게이트 콜백 `Handle` prefix·`Super::` 누락) **CLAUDE.md 위반은 하나도 나오지 않았다**. `Build.cs`·`uplugin` 의존도 `WxCore` 외 다른 Wx 플러그인을 참조하지 않는다. 일부 파일 첫 줄이 UTF-8 BOM으로 시작하지만 텍스트 자체는 규정대로다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 169파일 — `/module-review`로 갱신*
