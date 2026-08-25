# WxCombat — 코드 리뷰

> 직전 리뷰의 지적이 대부분 실제로 해소된 건강한 모듈이다 — 잔상 큐 널 역참조, 회피 TargetData 무검증 캐스트, 쿨다운 널 가드, 락온 회전 모드 원복, 낡은 타입명이 전부 고쳐졌고 CLAUDE.md 규칙 위반은 146파일 전수 검사에서 0건이다. 다만 이번엔 히트 판정 쿼리 방식이 프로젝트가 명시한 콜리전 계약을 우회하는 문제가 새로 드러났고, 컷신·타겟팅 필터·AbilitySet 부여에서 멀티플레이 관련 결함이 추가로 잡혔다. 이번 리뷰는 대미지 파이프라인(Library→TableRow→ExecCalc→AttributeSet)·13개 어빌리티·ASC·AbilityBase·AbilitySet·AnimNotify 9종·Cue 6종·AbilityTask 4종·Targeting 8종·무기/투사체/TimeDilation을 cpp까지 읽었고, 생성자만 있는 데이터성 GE는 훑는 수준으로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 8 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 무기 틱 Sweep이 오브젝트 타입 쿼리라 `ECC_WxAttack = Ignore`인 몸통 캡슐까지 잡는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:202-217`
- **범주**: 버그/정확성
- **문제**: 프로젝트의 피격 계약은 채널 응답으로 세워져 있다 — `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:12`가 "캐릭터 메시는 WxAttack에 Overlap으로, 캡슐은 Ignore로 명시 override하여 **메시에서만** 피격 판정이 일어난다"고 선언하고, `Source/WxGame/Character/WxCharacterBase.cpp:25`(메시 Overlap)·`:29`(캡슐 Ignore)가 이를 구현한다. 오버랩 이벤트 경로(`WxWeaponBase.cpp:166-169`)는 이 계약을 정확히 지킨다.

  그런데 터널링 보완용 틱 Sweep(`:212`)은 `SweepMultiByObjectType`에 `ECC_Pawn`만 넘긴다(`:203`). 오브젝트 타입 쿼리는 대상의 채널 응답을 보지 않는다 — 이 프로젝트 자신도 그 성질에 기대고 있다(`Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:254-257`의 판정 캡슐 주석). 그런데 몸통 캡슐(프로파일 `Pawn`)과 캐릭터 메시(프로파일 `WxCharacterMesh`, `Config/DefaultEngine.ini:41`에서 `ObjectTypeName="Pawn"`) **둘 다 오브젝트 타입이 `Pawn`**이라, Sweep은 캡슐도 함께 반환한다.

  구체적 실패: 캡슐은 메시를 감싸는 원기둥이라 스윕 경로상 대개 메시보다 **먼저** 히트한다. `:214-217`이 히트를 거리순으로 처리하고 `ProcessHit`이 액터 단위로 dedupe하므로(`:260`), 캡슐 히트가 이기고 메시 히트는 버려진다. 결과는 (a) 칼날이 몸에 닿기 전에 피격이 성립해 실효 히트박스가 전신 원기둥으로 확대되고, (b) `HitResult.ImpactPoint`가 캡슐 표면이라 임팩트 FX·큐 위치(`WxAbilitySystemGlobals`가 ImpactPoint를 Cue Location으로 채운다)가 실제 타격 부위와 어긋나며, (c) 앞으로 `ECC_WxAttack = Ignore`로 무적·페이즈를 구현하면 Sweep 경로로 그대로 뚫린다.
- **제안**: 오브젝트 쿼리 자체는 유지해야 한다 — 회피 판정 캡슐이 모든 채널 Ignore + `SetGenerateOverlapEvents(false)`라 이 Sweep으로만 잡히기 때문이다(`WxAbility_Dodge.cpp:256-258`). 대신 판정 캡슐에 `ECC_WxAttack = ECR_Overlap`을 주고, Sweep 결과를 `Hit.Component->GetCollisionResponseToChannel(ECC_WxAttack) == ECR_Overlap`으로 후처리 필터링해 두 경로가 같은 계약을 쓰게 한다.
- **확신도**: 높음

### 2. 🟡 궁극기 컷신의 PlayRate 보정이 원격 클라에서 컷신을 첫 프레임에 소모한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:49`, `:88-91`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: 태스크는 글로벌 시간을 0.001로 죽이고(`:49`) 시퀀스 재생속도를 `1/GlobalTimeDilation` = 1000배로 올려(`:90`) 상쇄한다. 그런데 `SetGlobalTimeDilationAuthoritative`는 서버 권위에서만 실제로 적용되고(`Private/Time/WxTimeDilationComponent.cpp:38-41`), 클라에는 복제로 늦게 도착한다 — 헤더 `Public/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h:18`이 그 사실을 이미 적어 두었다. 반면 PlayRate 보정은 머신 구분 없이 즉시 걸린다.

  `UWxAbility_Ultimate`은 베이스의 `LocalPredicted`를 그대로 쓰므로(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:21`) 소유 클라도 이 태스크를 만든다. 원격 클라에서는 딜레이션이 도착하기까지 half-RTT 동안 월드가 1.0배인 채 시퀀스만 1000배로 돈다 — 60fps에서 세 프레임이면 시퀀스 시간 약 50초로, 대부분의 컷신이 그 자리에서 끝나 `OnCompleted`(`:107`)가 즉시 발화한다. 그 뒤 딜레이션이 도착해 궁극기 몽타주가 0.001배로 재생된다. 리슨 서버 호스트에서는 권위가 있어 재현되지 않아 테스트에서 놓치기 쉽다.
- **제안**: 보정 배율을 요청값이 아니라 그 머신에 실제로 적용된 값(`UGameplayStatics::GetGlobalTimeDilation`)에서 뽑거나, 시퀀스 액터에 `CustomTimeDilation`을 걸어 글로벌 딜레이션의 영향을 아예 받지 않게 하고 PlayRate 보정을 없앤다.
- **확신도**: 중간(메커니즘은 코드로 확정, 실측은 원격 클라 환경 필요)

### 3. 🟡 컷신 태스크가 자기가 걸지 않은 Invincible을 걷어낼 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:23`
- **범주**: 버그/정확성
- **문제**: `OnDestroy`가 조건 없이 `RemoveEffect(UWxEffect_Invincible)`을 부른다. 그런데 `Activate()`에는 GE를 거는 `:94`에 **닿기 전에** 끝나는 경로가 둘 있다 — `:36-41`(World/LevelSequence 널)과 `:65-72`(SequencePlayer 생성 실패). 두 경로 모두 `EndTask()` → `OnDestroy` → `RemoveEffect`로 흘러간다.

  `UWxCombatLibrary::RemoveEffect`는 정의(클래스) 기준으로 아무 인스턴스나 1개를 지운다(`Private/WxCombatLibrary.cpp:107`). 즉 컷신이 시작조차 못 한 프레임에, 처형이 `ActivationOwnedEffects`로 걸어 둔 무적(`Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:31`)이나 i-frame ANS가 건 무적을 대신 벗길 수 있다.
- **제안**: 적용 성공 여부를 bool 멤버로 남기고 `OnDestroy`에서 그때만 제거한다.
- **확신도**: 높음(로직) / 중간(현재 유일 호출부인 `UWxAbility_Ultimate`은 SuperArmor를 쓰므로 오늘 당장은 잠재적)

### 4. 🟡 InputDirection 필터가 로컬 전용 입력을 읽어 SnapToTarget 워프가 서버/클라로 갈린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_InputDirection.cpp:23`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: `SourcePawn->GetLastMovementInputVector()`는 `AddMovementInput` 호출로만 채워진다. 서버는 원격 클라 폰에 대해 그 함수를 부르지 않고 ServerMove의 Acceleration을 직접 적용하므로, **서버에서 이 값은 항상 0**이고 필터는 아무것도 거르지 않는다(`:26-29`).

  문제가 되는 건 이 필터가 든 프리셋을 서버도 실행하기 때문이다. `Content/AbilitySystem/Targeting/TP_Attack_{250,1000,10000}.uasset` 셋 다 이 필터를 포함하고, 그 프리셋은 `WxAnimNotifyState_SnapToTarget`이 modifier에 주입해(`Private/AnimNotify/WxAnimNotifyState_SnapToTarget.cpp:60`, `:77`) 몽타주가 재생되는 **모든 머신**에서 돈다.

  구체적 실패: 락온 대상 A가 있고 스틱 입력이 다른 적 B를 가리키는 상황. 클라는 `bKeepAllWhenNoMatch`(기본 true) 경로로 A를 제외하므로 `WxRootMotionModifier_SnapToTarget.cpp:76`의 `bTargetInSnapRange`가 false → `:83`에서 `bWarpTranslation = false`. 서버는 입력이 0이라 A가 남아 true. 같은 몽타주가 클라는 순정 루트모션, 서버는 워프된 루트모션으로 재생돼 위치가 벌어지고 CMC 보정이 발생한다. 헤더 `Public/Targeting/WxRootMotionModifier_SnapToTarget.h:16`이 "플레이어 폰의 위치 워프만 복제되는 락온 대상이 있을 때로 제한해 멀티플레이 디싱크를 막고"라고 선언한 불변식이 이 필터로 깨진다.
- **제안**: SnapToTarget 전용으로 입력 필터가 빠진 프리셋을 쓰거나, 필터가 머신 간 일관된 값(`CharacterMovement->GetCurrentAcceleration()` — ServerMove로 복원되고 시뮬 프록시에도 복제된다)을 읽도록 바꾼다.
- **확신도**: 중간(코드·에셋 참조는 확정, 체감 빈도는 실측 필요)

### 5. 🟡 AbilitySet 부여에 취소 경로가 없어 재빙의 시 중복 부여된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:9-34`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:17-25`
- **범주**: 설계/구조 (상태 관리) *(직전 리뷰 🟢에서 승격)*
- **문제**: `FWxAbilitySetGrantedHandles::RemoveFromAbilitySystem`은 저장소 전체에 호출부가 0개다(선언 `Public/AbilitySystem/WxAbilitySet.h:22`, 정의 `WxAbilitySet.cpp:9` 두 줄이 전부). 그 결과 `AbilitySetGrantedHandles`(`Public/AbilitySystem/WxAbilitySystemComponent.h:70`)는 `GiveAbilitySet()`이 채우기만 하는 쓰기 전용 멤버가 되고, `GiveAbilitySet()` 자체에도 재진입 가드가 없다.

  구체적 실패: `AWxCharacterBase::PossessedBy`가 `InitAbilitySystem()`을 부르고(`Source/WxGame/Character/WxCharacterBase.cpp:104-108`), 그 안에서 권위일 때 `GiveAbilitySet()`을 부른다(`:211-214`). 재빙의(unpossess→repossess, AI 컨트롤러 교체·탈것 등)가 한 번이라도 일어나면 어빌리티와 GE가 통째로 중복 부여되고, `GiveToAbilitySystem`이 매번 `SetNumericAttributeBase`로 HP/SP를 초기값으로 되돌린다(`WxAbilitySet.cpp:43-59`) — 전투 중 재빙의는 곧 풀피 회복이다. 핸들 배열은 계속 누적되기만 한다.
- **제안**: `GiveAbilitySet()` 진입부에서 기존 핸들이 있으면 `RemoveFromAbilitySystem`을 먼저 부르거나(원래 의도로 보인다), 재부여 시나리오가 없다고 확정한다면 함수와 핸들 멤버를 함께 지우고 조기 반환 가드만 남긴다.
- **확신도**: 높음(데드코드·가드 부재는 확정) / 중간(재빙의가 실제 로드맵에 있는지)

### 6. 🟡 Hit 큐의 카메라 셰이크가 리슨 서버에서 두 번 걸린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_Hit.cpp:58`, `:71`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: `APlayerController::ClientStartCameraShake`는 client RPC다. GameplayCue는 데디케이티드 서버에서만 억제되고 **리슨 서버에서는 로컬 실행**되는데, 리슨 서버에는 원격 플레이어의 PlayerController 인스턴스가 존재한다. 따라서 호스트가 큐를 처리하며 원격 클라에 셰이크 RPC를 쏘고, 그 클라는 복제로 도착한 같은 큐를 자기 쪽에서도 실행해 로컬 셰이크를 한 번 더 재생한다 → 강도 2배 + RPC 지연분만큼 어긋난 두 번째 셰이크.
- **제안**: `PlayerController->IsLocalController()` 게이트를 추가하거나, 로컬 컨트롤러에서 `PlayerCameraManager->StartCameraShake`를 직접 부른다.
- **확신도**: 중간(리슨 서버를 쓰지 않는다면 영향 없음)

### 7. 🟡 CameraMove의 NotifyEnd가 뷰를 가져갔는지와 무관하게 뷰타겟을 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:180`
- **범주**: 버그/정확성 (상태 관리)
- **문제**: `NotifyBegin`은 PC 미획득(`:36-40`)이나 SpawnActor 실패(`:51-54`)로 조기 반환할 수 있는데, `NotifyEnd`는 그런 사실을 모른 채 항상 `SetViewTargetWithBlend(PC->GetPawn(), ...)`를 부른다. 실패 경로에서 남의 카메라(다른 연출·시퀀서)를 뺏어 폰으로 되돌리고, 같은 몽타주를 재생하는 액터가 둘이거나 구간이 겹치면 먼저 끝난 쪽이 아직 살아 있는 카메라를 회수한다. 사망 몽타주에서 폰이 이미 언포제스됐다면 `PC->GetPawn()`이 널이라 뷰타겟이 PlayerController 자신으로 넘어간다.
- **제안**: 노티파이 오브젝트는 애셋 단위 공유라 bool 멤버를 둘 수 없으므로, `PC->GetViewTarget()`이 `MeshComp->GetOwner()`를 오너로 갖는 `ACameraActor`일 때만 되돌리는 무상태 검사를 쓴다.
- **확신도**: 중간(코드 경로는 확실, 발생 빈도는 연출 배치에 달렸다)

### 8. 🟡 홀드 입력이 매 프레임 어빌리티 전수 스캔과 활성 GE 전수 조회를 유발한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:38-66`, `:159-191`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:401-443`
- **범주**: 성능/안전
- **문제**: `AbilityInputActionTriggered`는 `ETriggerEvent::Triggered`에 물려 있어(`Source/WxGame/Character/WxPlayerCharacter.cpp:112`) 홀드형 입력(가드·질주)이 눌린 동안 매 프레임 호출된다. 한 프레임마다 (a) `GetActivatableAbilities()` 전수 순회 + 스펙마다 `Cast<UWxAbilityBase>`, (b) `TryActivateAbility` → `CanActivateAbility`(`WxAbilityBase.cpp:85`) → `FindActivationGroupBlockers()`가 다시 전수 순회하며 스펙마다 `Spec.GetAbilityInstances()`(값 반환 = 힙 할당)를 부르고 결과 `TArray`도 값 반환, (c) `CheckCooldown` → `QueryActiveCooldowns` → `ASC.GetActiveEffects(Query)`(`:419`)가 활성 GE를 전수 스캔하며 또 `TArray`를 할당한다. 전투 중 활성 GE가 많을수록 회당 비용이 커진다. "차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다"는 재시도 의미론은 의도된 설계이므로(헤더 `WxAbilitySystemComponent.h:28-30`) 호출 빈도가 아니라 회당 비용을 낮추는 쪽이 맞다.
- **제안**: `UWxAbilitySet` 부여 시점에 `InputAction → FGameplayAbilitySpecHandle` 맵을 캐시해 전수 순회와 `Cast`를 없애고, `FindActivationGroupBlockers`는 출력 배열을 인자로 받아 재사용하며, `QueryActiveCooldowns`는 `GetActiveEffects`의 배열 반환 대신 어빌리티별 충전 카운트 캐시로 대체한다.
- **확신도**: 높음

### 9. 🟡 Attack·Skill·Pattern의 콤보 진행 코드가 3중 복제다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:21-56`, `.../WxAbility_Pattern.cpp:19-46`
- **범주**: 중복/복잡도
- **문제**: Attack과 Skill의 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted`는 클래스 이름을 치환하면 **바이트 단위로 동일**하다(diff 확인). Pattern도 두 함수가 같고 `HandleMontageCompleted`만 자동 진행이다. 헤더의 `ComboMontages`/`ComboIndex`도 셋에 각각 선언돼 있으며 에디터 카테고리가 이미 갈렸다 — `WxAbility_Attack.h:33`은 `"Wx"`, `WxAbility_Skill.h:36`은 `"Wx|Ability"`, `WxAbility_Pattern.h:29`는 `"Wx"`.

  복제가 이미 실제 결함을 만들었다. Pattern은 `bRetriggerInstancedAbility`를 켜지 않는데 `ActivateAbility:29`의 "다음 단으로 넘기거나 0으로 되돌린다" 줄을 그대로 복사했다. 정상 종료 시 `ComboIndex`가 마지막 인덱스라 항상 0이 되어 평소엔 무의미하지만, 배열 중간에 빈 슬롯이 있어 `HandleMontageCompleted:59-62`가 재생 실패로 `bWasCancelled = false` 종료하면 `ComboIndex`가 중간 값 k로 남고, 다음 발동이 0이 아니라 k+1부터 시작해 앞 단계를 조용히 건너뛴다. 헤더 주석(`WxAbility_Pattern.h:12-13`)도 "단일 몽타주를 재생한다"로 남아 있어 자동 체인 동작과 어긋난다.
- **제안**: `ComboMontages`/`ComboIndex`와 진행·리셋 규칙을 중간 베이스(`UWxAbility_ComboBase` 등)로 올리고 Pattern만 `HandleMontageCompleted`를 오버라이드해 자동 진행을 얹는다. Pattern에서는 활성화 시 항상 0에서 시작하도록 정리하고 헤더 주석도 실제 동작에 맞춘다.
- **확신도**: 높음

### 10. 🟢 GhostTrail이 메시 트랜스폼 대신 ACharacter 기본 오프셋을 재구성한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:40-44`
- **범주**: 버그/정확성
- **문제**: 포즈는 `OwnerMesh`에서 컴포넌트 스페이스로 복사하면서(`:46-47`), 액터 배치는 액터 트랜스폼 + `-CapsuleHalfHeight` + `Yaw -90°`라는 ACharacter 기본 메시 오프셋을 손으로 재구성해 쓴다. 메시 상대 트랜스폼이 기본값이 아닌 캐릭터(커스텀 회전이 들어간 보스, 캡슐 크기가 다른 몹, 메시 상대 스케일을 쓰는 캐릭터)에서는 잔상이 본체와 어긋난 위치·각도로 뜬다. `SetActorScale3D`(`:44`)도 액터 스케일만 반영해 메시 상대 스케일을 무시한다. 덧붙여 `:40`의 `GetCapsuleComponent()`는 바로 위에서 메시를 옵셔널 서브오브젝트라며 방어해 놓고(`:32-38`) 널 검사 없이 역참조한다.
- **제안**: `SetActorTransform(OwnerMesh->GetComponentTransform())` 한 줄로 대체하면 포즈 출처와 배치 기준이 정의상 일치하고 캡슐 역참조도 사라진다. 함께 `LifeSpan`(`Public/AbilitySystem/Cue/WxCueNotify_GhostTrail.h:42`)에 `ClampMin`을 걸거나 `:85`를 `LifeSpan > 0.f` 가드로 감싼다 — `SetLifeSpan(0)`은 UE에서 "수명 없음"이라 기획자가 0을 넣으면 대시마다 PoseableMesh 액터가 영구 누적된다.
- **확신도**: 높음(기본 셋업에서만 등가)

### 11. 🟢 히트스톱 복원 배속이 어빌리티의 PlayRate 오버라이드를 무시한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:223`
- **범주**: 버그/정확성
- **문제**: 얼렸던 몽타주를 ASC의 ASPD 기반 `GetMontagePlayRate()`로 복원하는데, 그 몽타주는 `UWxAbilityBase::PlayMontage`(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:200`)가 **어빌리티의** `GetMontagePlayRate()`로 재생한 것이다. Dodge·Guard·HitReact·Finisher·Death는 이 함수를 `1.f`로 오버라이드하므로, ASPD가 1이 아닌 캐릭터에서 그런 몽타주 중 히트스톱이 걸리면 복원 후 재생 속도가 원래와 달라진다. `ApplyHitStop`이 이미 `SourceAbility`를 받고 있어 고치기는 쉽다.
- **제안**: 프리즈 시점에 `SourceAbility->GetMontagePlayRate()`를 캡처해 타이머 델리게이트로 함께 넘기고 그 값으로 복원한다.
- **확신도**: 중간(PlayRate를 1로 고정한 어빌리티 몽타주에 히트스톱 유발 노티파이가 실제로 배치돼야 드러난다)

### 12. 🟢 AbilityTask들의 방어 가드가 파일마다 들쭉날쭉하다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:21`, `:41`
- **범주**: 버그/정확성
- **문제**: `UGameplayTask::GetWorld()`는 TasksComponent 약참조가 풀리면 nullptr을 반환하는데, 두 곳 모두 `GetWorld()->GetRealTimeSeconds()`로 무가드 역참조한다. 같은 폴더의 형제 태스크는 전부 방어한다 — `WxAbilityTask_PlaySkillCutscene.cpp:35-41`은 World 널 검사, `WxAbilityTask_WaitMoving.cpp:39-43`은 아바타 널 검사, `WxAbilityTask_LockOnTarget.cpp:31-51`도 전 단계 널 검사. 아바타·ASC가 파괴되는 프레임에 태스크가 살아 있으면 크래시다. 같은 맥락으로 `ShouldBroadcastAbilityTaskDelegates()` 가드도 `WxAbilityTask_PlaySkillCutscene.cpp:105`만 걸려 있고 같은 파일 `:38`·`:70`을 포함해 나머지 태스크는 전부 무가드다.
- **제안**: SlowTime의 두 지점에 World 널 가드를 넣고, 델리게이트 브로드캐스트 지점의 `ShouldBroadcastAbilityTaskDelegates()` 사용 여부를 태스크 전체에서 하나로 통일한다.
- **확신도**: 중간(엔진 계약상 널은 가능하나 실제 발현 창이 좁다)

### 13. 🟢 `UWxEffect_Guard::DamageReductionRate`가 튜닝 가능해 보이지만 실제로는 고정값이다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Guard.h:28`
- **범주**: 설계/구조
- **문제**: `EditDefaultsOnly, BlueprintReadOnly, ClampMin/ClampMax` 메타를 달았지만 양쪽 방향 모두 막혀 있다. 네이티브 `UGameplayEffect` 클래스라 별도 .uasset이 없어 CDO를 에디터에서 편집할 수 없고, BP 자식을 만들어 값을 바꿔도 부여는 `ActivationOwnedEffects.Add(UWxEffect_Guard::StaticClass())`(`Private/AbilitySystem/Ability/WxAbility_Guard.cpp:17`)로 네이티브 클래스가 고정이며 읽기도 `GetDefault<UWxEffect_Guard>()`(`Private/AbilitySystem/Effect/WxEffect_Guard.cpp:27`)라 자식 값이 절대 닿지 않는다. 소비처는 `Private/AbilitySystem/Effect/WxEffect_Damage.cpp:128`.
- **제안**: 진짜 밸런싱 값이면 데이터테이블이나 DeveloperSettings로 빼고, 아니면 `UPROPERTY`를 떼고 `static constexpr`로 의도를 드러낸다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `.../Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../Private/Damage/WxDamageTableRow.cpp`, `.../Private/Damage/WxCombatEffectContext.cpp`, `.../Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `.../Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../Private/AbilitySystem/WxAbilitySet.cpp`, 13개 `WxAbility_*.cpp` 전량, `.../Private/AnimNotify/` 9파일 전량, `.../Private/AbilitySystem/Cue/` 6파일 전량, `.../Private/AbilitySystem/Task/` 4파일 전량, `.../Private/Targeting/` 8파일 전량, `.../Private/Weapon/WxWeaponBase.cpp`, `.../Private/Weapon/WxProjectileBase.cpp`, `.../Private/Time/WxTimeDilationComponent.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_{Cost,Guard,Exhaust,AddDP,DrainDP,DrainSP,RegenSP,Exceed,MoveSpeedScale,HealPercent,Cooldown}.cpp`, 대응 Public 헤더 전량
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `.../Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `.../Private/WxCombatModule.cpp`, 생성자만 있는 데이터성 GE(`WxEffect_{Invincible,FullHP,Kill,SuperArmor,NoCooldown,InfiniteMP,PerfectGuard,ResetDP}`), 교차 검증용 `Source/WxGame/Character/WxCharacterBase.cpp`·`WxPlayerCharacter.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Config/DefaultEngine.ini`의 콜리전 프로파일
- **확인했고 문제 없던 항목**: CLAUDE.md 규칙 위반 0건 — `WxCore` 외 Wx 플러그인 참조 없음(`.uplugin`·`Build.cs`·인클루드 전수 확인, 외부 Wx 헤더는 `WxGameplayTags.h`·`WxCollisionChannels.h` 둘뿐), `Wx` prefix 전수 일치, `FORCEINLINE`·인라인 정의 0건, 람다 0건, 델리게이트 바인딩 25건 전부 `Handle` prefix, `BlueprintCallable`은 `Public/WxCombatLibrary.h:34`(BP Function Library) 한 곳뿐, 저작권 첫 줄 146파일 전부 통과(일부 파일에 UTF-8 BOM이 앞서지만 문구는 정상). `Super::` 미호출 오버라이드는 전수 확인 결과 전부 의도적 완전 대체다(`GetMontagePlayRate` 고정값, `HandleMontage*`의 동작 치환, `ShouldFilterTarget`·`GetScriptStruct`·`AllocGameplayEffectContext` 등 순수 가상). 직전 리뷰의 🔴 1건과 🟡 3건(GhostTrail 널 역참조·중복 스폰, Dodge TargetData 무검증 캐스트, 락온 회전 모드 하드코딩 원복)은 실제로 수정됐고, 락온 Server RPC 무검증은 `Public/Targeting/WxLockOnManagerComponent.h:52`에 "PvE 코옵 전제라 서버에서 재검증하지 않는다"로 신뢰 모델이 명시돼 결정 사항으로 처리했다.
- **미검토 / 한계**:
  - 무기 히트 판정이 시뮬 프록시에서도 그대로 도는 것(`WxWeaponBase.cpp:257` 주석이 "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다"고 명시)은 이번에도 의도된 설계로 보고 세지 않았다. 다만 그 경로로 `Event_DodgeSuccess`(`WxCombatLibrary.cpp:47`)가 비권위 머신에서도 발송돼 피격자 클라가 서버엔 없던 극한 회피를 로컬로 예측할 수 있다 — 재검토가 필요하면 이 지점부터 본다. (`ApplyHitStop`은 시뮬 프록시에서 `GetAnimatingAbility()`가 널이라 `WxAbilitySystemComponent.cpp:132`에서 조기 반환하므로 해당 없음.)
  - `WxTimeDilationComponent`가 서버에는 즉시, 클라에는 복제 도착 후 적용되는 half-RTT 창은 복제의 본질적 한계로 보고 별도 항목으로 세지 않았다(헤더 `WxTimeDilationComponent.h:12`가 세운 "모든 머신이 같은 값" 불변식은 그 창에서 성립하지 않는다). 2번 항목이 그 창에서 터지는 구체 사례다.
  - `WxTargetingFilterTask_ScreenBounds.cpp:26-29`의 조기 반환 때문에 `:34`의 뷰포트 0 가드가 사실상 도달 불가이고, 비로컬 PlayerController를 소스로 쓰면 모든 후보가 제외된다. 현재 유일 호출부가 `IsLocallyControlled()` 뒤라 라이브 버그가 아니어서 항목으로 세지 않았다.
  - `WxAnimNotifyState_CameraMove.cpp`의 `#if WITH_EDITOR` 프리뷰 경로(78-133, 183-193행)는 에디터 전용이라 정합성만 훑었다.
  - BP/WBP 에셋 내부(콤보 몽타주 배치, ANS 구간, `AbilityDataRow`/`DamageTableRow` 실제 값, AbilitySet 에셋)는 범위 밖이라 데이터 저작 실수로만 드러나는 결함은 잡지 못했다. 4번은 예외적으로 `TP_Attack_*` 프리셋의 필터 구성만 바이너리 검색으로 확인했다.
  - 멀티플레이 실측(2번의 실제 RTT 창, 4번의 러버밴딩 체감, 8번의 프레임 비용)은 전부 정적 분석이다.

---
*문서 기준 커밋 `f0cd3293` · 리뷰일 2026-08-25 · 소스 146파일 — `/module-review`로 갱신*
