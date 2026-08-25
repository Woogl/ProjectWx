# WxCombat — 코드 리뷰

> 여전히 건강한 모듈이다. 직전 리뷰의 굵직한 항목들이 실제로 해소됐다 — `ActivationOwnedEffects` 해제가 정의 단위 전수 제거에서 핸들 단위로 바뀌었고(`WxAbilityBase.cpp:173-180`), 죽어 있던 ComboWindow 게이트가 `Exclusive_ComboWindow` 그룹 전이로 되살아났으며, 중복이던 Invincible/PerfectGuard ANS는 `WxAnimNotifyState_ApplyGameplayEffect` 하나로 합쳐졌다. 새로 눈에 띄는 것은 잔상 큐 파일(`WxCueNotify_GhostTrail`) 하나로, 이 파일만 모듈 평균에서 확연히 떨어지고 명백한 널 역참조를 안고 있다. 나머지는 직전 리뷰에서 그대로 남은 항목들이다. 이번 리뷰는 대미지 파이프라인(Library→TableRow→ExecCalc→AttributeSet)·`WxAbilityBase`와 13개 구체 어빌리티·ASC·락온·무기/투사체·TimeDilation·AnimNotify 8종·AbilityTask 4종을 cpp까지 읽었고, Effect·Cue·Targeting 필터는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 6 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 `AWxGhostTrail::BeginPlay`가 널 가드 직후 같은 포인터를 무조건 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:30-37`
- **범주**: 버그/정확성
- **문제**: 30행은 `OwnerCharacter ? OwnerCharacter->GetCapsuleComponent()->... : 0.f`로 캐스팅 실패를 명시적으로 대비해 놓고, 바로 다음 32·33·34·36·37행이 `OwnerCharacter`를 무조건 역참조한다. `Owner`가 `ACharacter`가 아닌 액터(Pawn 기반 적, `AWxDevice` 등 ASC 보유 비-Character)에서 잔상 큐가 발행되면 즉시 크래시다. 37행 `OwnerCharacter->GetMesh()`도 널 검사가 없어 메시가 없는 캐릭터에서도 같은 결과다. 30행의 삼항 자체가 작성자도 널 가능성을 인지했음을 보여준다.
- **제안**: 30행 이후를 `if (!OwnerCharacter) { return; }` 조기 반환으로 바꾸고, `GetMesh()` 결과를 지역 변수로 받아 널 검사 후 쓴다.
- **확신도**: 높음

### 2. 🟡 `UWxCueNotify_GhostTrail::HandleGameplayCue`가 EventType을 거르지 않고 스폰 결과도 검증하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:50-65`
- **범주**: 버그/정확성
- **문제**: 세 가지가 겹친다. (a) 형제 큐들(`WxCueNotify_Hit.cpp:20`, `WxCueNotify_DamageFloater.cpp:16`, `WxCueNotify_PerfectGuard.cpp:18`)은 모두 `EventType != EGameplayCueEvent::Executed`로 걸러 내는데 이 큐만 필터가 없다. 부모 `UGameplayCueNotify_Static::HandleGameplayCue`는 EventType으로 분기하는 디스패처이므로, 이 태그가 지속형 GE에 실리면 OnActive·WhileActive·Removed마다 잔상이 중복 스폰된다(WhileActive는 관련성 변화마다 재발화). (b) 61행이 `MyTarget`을 널 검사 없이 `->GetWorld()`한다 — 다른 큐들은 전부 `!MyTarget` 검사를 먼저 한다. (c) 62행이 `SpawnActor` 반환값을 검사하지 않고 바로 `SetLifeSpan`을 부른다. 덧붙여 `Super::HandleGameplayCue` 호출이 맨 뒤(64행)라 부모 분기보다 스폰이 앞선다.
- **제안**: 다른 Cue와 같은 형태로 정렬한다 — `Super`를 먼저 부르고, `EventType != Executed || !MyTarget`이면 조기 반환, `SpawnActor` 결과 검사 후 `SetLifeSpan`.
- **확신도**: 중간(순수 `ExecuteGameplayCue` 경로로만 발행된다면 중복 스폰은 드러나지 않는다)

### 3. 🟡 클라이언트가 보낸 TargetData를 타입 검사 없이 다운캐스트한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:282`
- **범주**: 성능/안전
- **문제**: `static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0))`로 네트워크 수신 데이터를 무검증 변환한다. `CallServerSetReplicatedTargetData` 경로는 등록된 어떤 `FGameplayAbilityTargetData` 파생 타입도 실어 보낼 수 있으므로, 변조 클라이언트가 다른 타입을 보내면 서버가 남의 레이아웃에서 `Direction`을 읽어 쓰레기 벡터로 몽타주 섹션과 캐릭터 회전(`WxAbility_Dodge.cpp:172`의 `AddActorWorldRotation`)을 정한다. 같은 모듈의 `WxEffect_Damage.cpp:83-85`가 EffectContext에 대해 `GetScriptStruct() == StaticStruct()` 비교로 정확히 이 상황을 안전하게 처리하고 있어, 쓸 패턴이 이미 코드베이스 안에 있다.
- **제안**: `DataHandle.Get(0)->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct()` 확인 후 캐스트하고, 불일치면 영벡터로 폴백한다.
- **확신도**: 높음

### 4. 🟡 락온 대상 Server RPC가 클라이언트 지정값을 무검증 수용한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp:39-42`, 선언은 `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h:52-53`
- **범주**: 설계/구조
- **문제**: `ServerSetLockOnTarget_Implementation`이 받은 `USceneComponent*`를 그대로 `ApplyLockOnTarget`에 넘긴다. 거리(`MaxDistance`)·팀·`UWxLockOnPointComponent::CanBeLockedOn` 판정은 전부 클라이언트 측 `UWxAbility_LockOn`에만 있다. 그런데 이 복제값을 서버 권위 소비처가 읽는다 — `AWxProjectileBase::BeginPlay`의 호밍 타겟(`WxProjectileBase.cpp:58-69`)과 `UWxRootMotionModifier_SnapToTarget`의 스냅 타겟(`WxRootMotionModifier_SnapToTarget.cpp:31-37`). 즉 클라이언트가 서버 스폰 투사체의 추적 대상과 몽타주 스냅 지점을 임의로 지정할 수 있다. 헤더가 표방하는 "서버 권위"는 복제 권위일 뿐 값 검증 권위가 아니다.
- **제안**: RPC 구현부에서 최소한 `UWxLockOnPointComponent`인지·`CanBeLockedOn()`인지·소유 액터가 사거리 안인지를 재검사하고, 실패 시 이전 값을 유지한다. 코옵 전제로 검증을 생략하기로 했다면 그 결정을 헤더 주석에 명시해 둔다.
- **확신도**: 높음(사실 관계는 확실하나, PvE 코옵만 상정한 의도된 신뢰 모델일 수 있음)

### 5. 🟡 홀드 입력이 매 프레임 어빌리티 전수 스캔과 활성 GE 전수 조회를 유발한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:38-66`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:401-436`
- **범주**: 성능/안전
- **문제**: `AbilityInputActionTriggered`는 `ETriggerEvent::Triggered`에 물려 있어(`Source/WxGame/Character/WxPlayerCharacter.cpp:112`) 홀드형 입력(가드·질주)이 눌린 동안 매 프레임 호출된다. 한 프레임마다 (a) `GetActivatableAbilities()` 전수 순회 + 스펙마다 `Cast<UWxAbilityBase>`, (b) 매칭 어빌리티에 `TryActivateAbility` → `CheckCooldown` → `QueryActiveCooldowns` → `ASC.GetActiveEffects(Query)`가 활성 GE를 전수 스캔하며 `TArray`를 힙 할당, (c) 활성 스펙에는 `Spec.GetAbilityInstances()`(값 반환 = 또 한 번 할당) + 인스턴스마다 `InvokeReplicatedEvent`가 반복된다. 전투 중 활성 GE가 많을수록 비용이 커진다. "차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다"는 재시도 의미론 자체는 의도된 설계이므로(헤더 주석 28-30행) 호출 빈도를 줄이는 대신 회당 비용을 낮추는 쪽이 맞다.
- **제안**: `UWxAbilitySet` 부여 시점에 `InputAction → FGameplayAbilitySpecHandle` 맵을 캐시해 전수 순회와 `Cast`를 없애고, `QueryActiveCooldowns`는 `GetActiveEffects`의 배열 반환 대신 `ForEachActiveEffect` 계열 순회나 어빌리티별 충전 카운트 캐시로 대체한다.
- **확신도**: 높음

### 6. 🟡 Attack·Skill·Pattern의 콤보 진행 코드가 3중 복제다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:21-56`, `.../WxAbility_Pattern.cpp:19-46`
- **범주**: 중복/복잡도
- **문제**: `UWxAbility_Attack`과 `UWxAbility_Skill`의 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted`는 클래스 이름 세 줄을 빼면 **바이트 단위로 동일**하다(diff로 확인). `UWxAbility_Pattern`도 `ActivateAbility`/`EndAbility`가 같고 `HandleMontageCompleted`만 자동 진행으로 다르다. 헤더의 `ComboMontages`/`ComboIndex` 필드도 셋에 각각 선언돼 있고 에디터 카테고리가 이미 갈렸다 — `WxAbility_Attack.h:33`은 `"Wx"`, `WxAbility_Skill.h:36`은 `"Wx|Ability"`, `WxAbility_Pattern.h:29`는 `"Wx"`. 콤보 규칙을 고칠 때 세 곳을 동시에 손대야 하고, 한 곳을 빠뜨리면 조용히 어긋난다.
- **제안**: `ComboMontages`/`ComboIndex`와 진행·리셋 규칙을 중간 베이스(`UWxAbility_ComboBase` 등)로 올리고 Pattern만 `HandleMontageCompleted`를 오버라이드해 자동 진행을 얹는다. 구조 추출을 미룬다면 최소한 카테고리·주석 드리프트만이라도 되돌린다.
- **확신도**: 높음

### 7. 🟡 락온 종료가 `bOrientRotationToMovement`를 저장값 복원이 아니라 `true`로 하드코딩한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:48`(끄기), `:104`(복원)
- **범주**: 설계/구조
- **문제**: 활성화 시 이전 값을 저장하지 않고 `false`로 내린 뒤 종료에서 무조건 `true`로 되돌린다. 이 플래그는 이 모듈 밖에서도 토글되므로, 다른 소유자가 꺼 둔 상태에서 락온이 끝나면 그 의도를 덮어써 CMC 회전이 튄다. 어빌리티가 캔슬로 끊기는 경로도 같은 EndAbility를 지나므로 동일하다.
- **제안**: 활성화 시점의 값을 멤버에 기억해 종료 시 그 값으로 되돌리거나, 회전 정책을 플래그 직접 조작이 아닌 이동 컴포넌트의 상태 API로 옮겨 소유자를 하나로 만든다.
- **확신도**: 중간

### 8. 🟢 `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`이 데드 코드다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-34`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h:70`
- **범주**: 중복/복잡도
- **문제**: 저장소 전수 검색 결과 `RemoveFromAbilitySystem` 호출부가 없다. `AbilitySetGrantedHandles`는 `GiveAbilitySet()`(`WxAbilitySystemComponent.cpp:24`)에서 채워지기만 하고 소비되지 않아, 부여 취소 경로가 실제로는 존재하지 않는다.
- **제안**: 회수 시나리오(장비 교체·Experience 전환)가 계획에 없다면 핸들 수집과 함수를 함께 지우고, 있다면 호출부를 붙인다.
- **확신도**: 높음

### 9. 🟢 쿨다운 오버라이드 중 `CheckCooldown`만 `ActorInfo` 널 가드가 빠져 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:331`, 같은 파일 `:407`
- **범주**: 버그/정확성
- **문제**: 같은 파일의 형제 오버라이드들은 모두 `ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr`로 방어하는데(309·369·386행) 331행만 `ActorInfo->AbilitySystemComponent.Get()`로 무조건 역참조한다. 407행의 `ASC.GetWorld()->GetTimeSeconds()`도 월드 널 검사가 없다.
- **제안**: 331행을 형제들과 같은 삼항 가드로 맞추고, 407행은 월드를 지역 변수로 받아 널이면 0을 반환한다.
- **확신도**: 중간(엔진이 실제로 널 `ActorInfo`를 넘기는 경로는 드물어, 크래시보다 일관성 결함에 가깝다)

### 10. 🟢 히트스톱 복원 배속이 어빌리티의 PlayRate 오버라이드를 무시한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:223`
- **범주**: 버그/정확성
- **문제**: 얼렸던 몽타주를 ASC의 ASPD 기반 `GetMontagePlayRate()`로 복원하는데, 그 몽타주는 `UWxAbilityBase::PlayMontage`(`WxAbilityBase.cpp:200`)가 **어빌리티의** `GetMontagePlayRate()`로 재생한 것이다. Dodge·Guard·HitReact·Finisher·Death는 이 함수를 `1.f`로 오버라이드하므로, ASPD가 1이 아닌 캐릭터에서 그런 몽타주 중 히트스톱이 걸리면 복원 후 재생 속도가 원래와 달라진다. `ApplyHitStop`이 이미 `SourceAbility`를 받고 있어 고치기는 쉽다.
- **제안**: 프리즈 시점에 `SourceAbility->GetMontagePlayRate()`를 캡처해 타이머 델리게이트로 함께 넘기고 그 값으로 복원한다.
- **확신도**: 중간(PlayRate를 1로 고정한 어빌리티 몽타주에 히트스톱 유발 노티파이가 실제로 배치돼야 드러난다)

### 11. 🟢 삭제된 `FWxDamageInfo`를 가리키는 선언·주석이 남았다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h:13`, `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxCombatEffectContext.h:29`
- **범주**: 중복/복잡도
- **문제**: `FWxDamageInfo`는 `FWxDamageTableRow`로 통합되며 사라졌는데, `WxCombatLibrary.h:13`에 전방 선언이 남아 있고(정의 없는 타입이라 누군가 참조하면 링크 단계에서야 드러난다) `WxCombatEffectContext.h:29` 주석이 여전히 `FWxDamageInfo::MakeSpecs`를 가리킨다.
- **제안**: 전방 선언을 지우고 주석의 타입명을 `FWxDamageTableRow`로 고친다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/*.cpp`(8파일 전량), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 나머지 5파일, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_{SlowTime,WaitMoving,PlaySkillCutscene}.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`(5파일), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/`의 주요 파일(Guard·Cooldown·Cost·Exhaust·AddDP·DrainDP), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_{Attack,Skill,Pattern,Death,Ultimate}.cpp`, 대응 Public 헤더 다수
- **확인했고 문제 없던 항목**: CLAUDE.md 코딩·모듈 규칙 위반은 전수 확인 결과 0건이다 — `WxCore` 외 Wx 플러그인 참조 없음(`.uplugin`·`Build.cs` 확인), `Wx` prefix 전수 일치, `FORCEINLINE`/인라인 정의 0건, 람다 0건, 델리게이트 바인딩 24건 전부 `Handle` prefix, `BlueprintCallable`은 `WxCombatLibrary.h:35`(BP Function Library) 한 곳뿐, 저작권 첫 줄 146파일 전부 통과(일부 파일에 UTF-8 BOM이 앞서지만 문구 자체는 정상). `UWxAbility_Death::HandleMontageCompleted`가 `Super::`를 부르지 않는 것은 사망 몽타주 종료 후에도 어빌리티를 살려 `Ability.Death`를 유지하려는 의도라 위반으로 세지 않았다.
- **미검토 / 한계**:
  - 생성자만 있는 데이터 정의성 GE(`WxEffect_Invincible`·`WxEffect_FullHP`·`WxEffect_Kill`·`WxEffect_SuperArmor` 등 15개 남짓)는 헤더 수준만 확인했다.
  - `WxAnimNotifyState_CameraMove.cpp`의 `#if WITH_EDITOR` 프리뷰 경로(78-133, 140-157행)는 에디터 전용이라 정합성만 훑었다.
  - 직전 리뷰가 🟡로 올렸던 "무기 히트 판정이 시뮬 프록시에서도 그대로 돈다"(`WxWeaponBase.cpp:255-268`에 권위·로컬 게이트 없음)는 코드가 그대로지만, README가 예측/권위 모델을 명시적으로 문서화한 상태라 의도된 설계로 보고 이번에는 새 발견으로 세지 않았다. 재검토가 필요하면 그 항목부터 본다.
  - BP/WBP 에셋 내부(콤보 몽타주 배치, ANS 구간, `AbilityDataRow`/`DamageTableRow` 실제 값, AbilitySet 에셋)는 범위 밖이라 데이터 저작 실수로만 드러나는 결함은 잡지 못했다.
  - 멀티플레이 실측(예측 롤백, 5번의 실제 프레임 비용)은 정적 분석만 했다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 146파일 — `/module-review`로 갱신*
