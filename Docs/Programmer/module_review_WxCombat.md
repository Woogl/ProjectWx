# WxCombat — 코드 리뷰

> 직전 리뷰의 지적이 대부분 실제로 해소된 건강한 모듈이다 — InputDirection 필터의 서버/클라 불일치, AbilitySet 재빙의 중복 부여, Hit 큐의 리슨 서버 이중 셰이크, GhostTrail 배치·LifeSpan, SlowTime의 World 널 가드, 가드 배율 상수화가 전부 커밋으로 정리됐고 CLAUDE.md 규칙 위반은 146파일 전수 검사에서 0건이다. 남은 것은 무기 히트 판정이 콜리전 계약을 우회하는 문제와 컷신·글로벌 시간감속을 둘러싼 권위/상태 결함이며, 이번 리뷰는 최신 커밋의 `Event.Hit` 태그 통합 경로(AttributeSet→HitReact→Guard→Finisher)를 새로 훑고 대미지 파이프라인·13개 어빌리티·ASC·AbilityBase·AbilitySet·AnimNotify 9종·Cue 6종·AbilityTask 4종·Targeting 8종·무기/투사체/TimeDilation을 cpp까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 5 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 무기 틱 Sweep이 오브젝트 타입 쿼리라 `ECC_WxAttack = Ignore`인 몸통 캡슐까지 잡는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:202-217`
- **범주**: 버그/정확성
- **문제**: 프로젝트의 피격 계약은 채널 응답으로 세워져 있다 — `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:12`가 "캐릭터 메시는 WxAttack에 Overlap으로, 캡슐은 Ignore로 명시 override하여 **메시에서만** 피격 판정이 일어난다"고 선언하고, `Source/WxGame/Character/WxCharacterBase.cpp:25`(메시 Overlap)·`:29`(캡슐 Ignore)가 이를 구현한다. 오버랩 이벤트 경로(`WxWeaponBase.cpp:166-169`)는 이 계약을 정확히 지킨다.

  그런데 터널링 보완용 틱 Sweep(`:212`)은 `SweepMultiByObjectType`에 `ECC_Pawn`만 넘긴다(`:202-203`). 오브젝트 타입 쿼리는 대상의 채널 응답을 보지 않는다 — 이 프로젝트 자신도 그 성질에 기대고 있다(`Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:254-258`의 판정 캡슐 주석). 그런데 몸통 캡슐(프로파일 `Pawn`)과 캐릭터 메시(프로파일 `WxCharacterMesh`, `Config/DefaultEngine.ini:41`에서 `ObjectTypeName="Pawn"`) **둘 다 오브젝트 타입이 `Pawn`**이라, Sweep은 캡슐도 함께 반환한다.

  구체적 실패: 캡슐은 메시를 감싸는 원기둥이라 스윕 경로상 대개 메시보다 **먼저** 히트한다. `:214-217`이 히트를 거리순으로 처리하고 `ProcessHit`이 액터 단위로 dedupe하므로(`:260`), 캡슐 히트가 이기고 메시 히트는 버려진다. 결과는 (a) 칼날이 몸에 닿기 전에 피격이 성립해 실효 히트박스가 전신 원기둥으로 확대되고, (b) `HitResult.ImpactPoint`가 캡슐 표면이라 임팩트 FX·큐 위치(`Private/AbilitySystem/WxAbilitySystemGlobals.cpp:23`이 ImpactPoint를 Cue Location으로 채운다)가 실제 타격 부위와 어긋나며, (c) 앞으로 `ECC_WxAttack = Ignore`로 무적·페이즈를 구현하면 Sweep 경로로 그대로 뚫린다. 사망 처리가 메시의 `ECC_WxAttack`을 Ignore로 내리는 것(`Private/AbilitySystem/Ability/WxAbility_Death.cpp:49`)도 Sweep 경로에서는 무력하다.
- **제안**: 오브젝트 쿼리 자체는 유지해야 한다 — 회피 판정 캡슐이 모든 채널 Ignore + `SetGenerateOverlapEvents(false)`라 이 Sweep으로만 잡히기 때문이다(`WxAbility_Dodge.cpp:256-258`). 대신 판정 캡슐에 `ECC_WxAttack = ECR_Overlap`을 주고, Sweep 결과를 `Hit.Component->GetCollisionResponseToChannel(ECC_WxAttack) == ECR_Overlap`으로 후처리 필터링해 두 경로가 같은 계약을 쓰게 한다.
- **확신도**: 높음

### 2. 🟡 궁극기 컷신의 PlayRate 보정이 비권위 소유 클라에서 컷신을 첫 프레임에 소모한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:53`, `:97-99`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: 태스크는 글로벌 시간을 0.001로 죽이고(`:53`, 요청값은 `Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:51`) 시퀀스 재생속도를 `1/GlobalTimeDilation` = 1000배로 올려(`:99`) 상쇄한다. 그런데 `SetGlobalTimeDilationAuthoritative`는 서버 권위에서만 실제로 적용되고(`Private/Time/WxTimeDilationComponent.cpp:37-41`), 클라에는 복제로 늦게 도착한다 — 헤더 `Public/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h:18`이 그 사실을 이미 적어 두었다. 반면 PlayRate 보정은 머신 구분 없이 즉시 걸린다.

  `UWxAbility_Ultimate`은 베이스의 `LocalPredicted`를 그대로 쓰므로(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:21`) 궁극기를 누른 소유 클라도 이 태스크를 만든다. 그 머신에서는 딜레이션이 복제로 도착하기까지 half-RTT 동안 월드가 1.0배인 채 시퀀스만 1000배로 돈다 — 60fps에서 세 프레임이면 시퀀스 시간 약 50초로, 대부분의 컷신이 그 자리에서 끝나 `OnCompleted`(`:116`)가 즉시 발화한다. 그 뒤 딜레이션이 도착해 궁극기 몽타주가 0.001배로 재생된다. 리슨 서버 호스트에서는 권위가 있어 재현되지 않아 테스트에서 놓치기 쉽다.
- **제안**: 보정 배율을 요청값이 아니라 그 머신에 실제로 적용된 값(`UGameplayStatics::GetGlobalTimeDilation`)에서 뽑거나, 시퀀스 액터에 `CustomTimeDilation`을 걸어 글로벌 딜레이션의 영향을 아예 받지 않게 하고 PlayRate 보정을 없앤다.
- **확신도**: 중간(메커니즘은 코드로 확정, 실측은 데디케이티드/클라 환경 필요)

### 3. 🟡 컷신 태스크가 자기가 걸지 않은 Invincible을 걷어낼 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:23`
- **범주**: 버그/정확성
- **문제**: `OnDestroy`가 조건 없이 `RemoveEffect(UWxEffect_Invincible)`을 부른다. 그런데 `Activate()`에는 GE를 거는 `:103`에 **닿기 전에** 끝나는 경로가 둘 있다 — `:36-45`(World/LevelSequence 널)과 `:69-81`(SequencePlayer 생성 실패). 두 경로 모두 `EndTask()` → `OnDestroy` → `RemoveEffect`로 흘러간다.

  `UWxCombatLibrary::RemoveEffect`는 정의(클래스) 기준으로 아무 인스턴스나 1개를 지운다(`Private/WxCombatLibrary.cpp:107`). 즉 컷신이 시작조차 못 한 프레임에, 처형이 `ActivationOwnedEffects`로 걸어 둔 무적(`Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:31`)이나 i-frame ANS가 건 무적을 대신 벗길 수 있다.
- **제안**: 적용 성공 여부를 bool 멤버로 남기고 `OnDestroy`에서 그때만 제거한다.
- **확신도**: 높음(로직) / 중간(현재 유일 호출부인 `UWxAbility_Ultimate`은 SuperArmor를 쓰므로 오늘 당장은 잠재적)

### 4. 🟡 글로벌 시간감속이 단일 소유자 슬롯이라 협동 플레이에서 서로의 연출을 잘라낸다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp:51`, `:68-76`
- **범주**: 설계/구조 (상태 관리)
- **문제**: `SetDilationFrom`이 값과 소유권을 무조건 마지막 요청자에게 넘기고(`:51`), `ClearDilationFrom`은 소유자일 때 배율을 **1.0으로** 되돌린다(`:73-76`). 헤더 `Public/Time/WxTimeDilationComponent.h:26`이 "나중 요청이 앞선 요청을 밀어낸다"고 선언하지만, 앞선 요청의 **남은 시간이 사라진다**는 결과까지는 다루지 않는다. 컴포넌트는 GameState에 하나뿐이라(`:87-93`) 슬롯은 파티 전체가 공유한다.

  구체적 실패: 플레이어 A가 궁극기 컷신을 재생하는 동안(딜레이션 0.001, `WxAbilityTask_PlaySkillCutscene.cpp:53`) 플레이어 B가 퍼펙트 가드에 성공하면 `UWxAbilityTask_SlowTime`이 소유권을 뺏어 0.4로 올린다(`Private/AbilitySystem/Ability/WxAbility_Guard.cpp:155`, `Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:58`, 기본값 `Public/AbilitySystem/Ability/WxAbility_Guard.h:66,69`). 컷신 시퀀스의 PlayRate는 여전히 1000배라 이 순간 400배로 폭주하고, 0.4초 뒤 B의 태스크가 배율을 1.0으로 되돌리면(`SlowTime.cpp:41`) 컷신은 그 프레임에 소모된다. A의 태스크는 이미 소유자가 아니라 자기 Clear가 no-op이 되어 복구도 못 한다. 반대로 A가 나중이면 B의 슬로우가 통째로 끊긴다.
- **제안**: 요청자별 스택(또는 우선순위·최소값 규칙)으로 바꿔 소유자가 사라질 때 남아 있는 요청 중 가장 강한 값으로 복원하거나, 컷신처럼 절대 뺏기면 안 되는 연출에는 글로벌 딜레이션 대신 액터 `CustomTimeDilation`을 쓴다(2번과 같은 해법이다).
- **확신도**: 중간(단일 소유 모델 자체는 헤더에 선언돼 있으나, 4인 코옵을 전제한 모듈에서 위 시나리오는 실제 결함이다)

### 5. 🟡 CameraMove의 NotifyEnd가 뷰를 가져갔는지와 무관하게 뷰타겟을 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:180`
- **범주**: 버그/정확성 (상태 관리)
- **문제**: `NotifyBegin`은 PC 미획득(`:36-40`)이나 SpawnActor 실패(`:51-54`)로 조기 반환할 수 있는데, `NotifyEnd`는 그런 사실을 모른 채 항상 `SetViewTargetWithBlend(PC->GetPawn(), ...)`를 부른다. 실패 경로에서 남의 카메라(다른 연출·시퀀서)를 뺏어 폰으로 되돌리고, 같은 몽타주를 재생하는 액터가 둘이거나 구간이 겹치면 먼저 끝난 쪽이 아직 살아 있는 카메라를 회수한다. 사망 몽타주에서 폰이 이미 언포제스됐다면 `PC->GetPawn()`이 널이라 뷰타겟이 PlayerController 자신으로 넘어간다.
- **제안**: 노티파이 오브젝트는 애셋 단위 공유라 bool 멤버를 둘 수 없으므로, `PC->GetViewTarget()`이 `MeshComp->GetOwner()`를 오너로 갖는 `ACameraActor`일 때만 되돌리는 무상태 검사를 쓴다.
- **확신도**: 중간(코드 경로는 확실, 발생 빈도는 연출 배치에 달렸다)

### 6. 🟡 Attack·Skill·Pattern의 콤보 진행 코드가 3중 복제다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:21-56`, `.../WxAbility_Pattern.cpp:19-63`
- **범주**: 중복/복잡도
- **문제**: Attack과 Skill의 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted`는 클래스 이름을 치환하면 **바이트 단위로 동일**하다(diff로 확인 — 차이는 생성자의 태그와 주석뿐). Pattern도 앞 두 함수가 같고 `HandleMontageCompleted`만 자동 진행이다. 헤더의 `ComboMontages`/`ComboIndex`도 셋에 각각 선언돼 있으며 에디터 카테고리가 이미 갈렸다 — `WxAbility_Attack.h:33`은 `"Wx"`, `WxAbility_Skill.h:36`은 `"Wx|Ability"`, `WxAbility_Pattern.h:29`는 `"Wx"`.

  복제가 이미 실제 결함을 만들었다. Pattern은 `bRetriggerInstancedAbility`를 켜지 않는데 `ActivateAbility:29`의 "다음 단으로 넘기거나 0으로 되돌린다" 줄을 그대로 복사했다. 정상 종료 시 `ComboIndex`가 마지막 인덱스라 항상 0이 되어 평소엔 무의미하지만, `ComboMontages` 중간에 빈 슬롯이 있어 `HandleMontageCompleted:59-62`가 재생 실패로 `bWasCancelled = false` 종료하면 `ComboIndex`가 중간 값 k로 남고, 다음 발동이 0이 아니라 k+1부터 시작해 앞 단계를 조용히 건너뛴다. 헤더 주석(`WxAbility_Pattern.h:12`)도 "단일 몽타주를 재생한다"로 남아 있어 자동 체인 동작과 어긋난다.
- **제안**: `ComboMontages`/`ComboIndex`와 진행·리셋 규칙을 중간 베이스(`UWxAbility_ComboBase` 등)로 올리고 Pattern만 `HandleMontageCompleted`를 오버라이드해 자동 진행을 얹는다. Pattern에서는 활성화 시 항상 0에서 시작하도록 정리하고 헤더 주석도 실제 동작에 맞춘다.
- **확신도**: 높음

### 7. 🟢 히트스톱 복원 배속이 어빌리티의 PlayRate 오버라이드를 무시한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:252`
- **범주**: 버그/정확성
- **문제**: 얼렸던 몽타주를 ASC의 ASPD 기반 `GetMontagePlayRate()`로 복원하는데, 그 몽타주는 `UWxAbilityBase::PlayMontage`(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:200`)가 **어빌리티의** `GetMontagePlayRate()`로 재생한 것이다. Dodge·Guard·HitReact·Finisher·Death는 이 함수를 `1.f`로 오버라이드하므로(예: `WxAbility_HitReact.cpp:54-57`), ASPD가 1이 아닌 캐릭터에서 그런 몽타주 중 히트스톱이 걸리면 복원 후 재생 속도가 원래와 달라진다. `ApplyHitStop`이 이미 `SourceAbility`를 받고 있어(`:139`) 고치기는 쉽다.
- **제안**: 프리즈 시점에 `SourceAbility->GetMontagePlayRate()`를 캡처해 타이머 델리게이트로 함께 넘기고 그 값으로 복원한다.
- **확신도**: 중간(PlayRate를 1로 고정한 어빌리티 몽타주에 히트스톱 유발 노티파이가 실제로 배치돼야 드러난다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `.../Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../Private/Damage/WxDamageTableRow.cpp`, `.../Private/Damage/WxCombatEffectContext.cpp`, `.../Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `.../Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../Private/AbilitySystem/WxAbilitySet.cpp`, 13개 `WxAbility_*.cpp` 전량(특히 최신 커밋이 손댄 `WxAbility_HitReact.cpp`·`WxAbility_Guard.cpp`·`WxAbility_Finisher.cpp`), `.../Private/AnimNotify/` 9파일 전량, `.../Private/AbilitySystem/Cue/` 6파일 전량, `.../Private/AbilitySystem/Task/` 4파일 전량, `.../Private/Targeting/` 8파일 전량, `.../Private/Weapon/WxWeaponBase.cpp`, `.../Private/Weapon/WxProjectileBase.cpp`, `.../Private/Time/WxTimeDilationComponent.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_{Cost,Guard,Exhaust,AddDP,DrainDP,Cooldown}.cpp`, 대응 Public 헤더 전량
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `.../WxCombat.Build.cs`, `.../Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `.../Private/WxCombatModule.cpp`, 생성자만 있는 데이터성 GE(`WxEffect_{Invincible,FullHP,Kill,SuperArmor,NoCooldown,InfiniteMP,PerfectGuard,ResetDP,DrainSP,RegenSP,Exceed,MoveSpeedScale,HealPercent}`), 교차 검증용 `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`·`WxCollisionChannels.h`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`, `Config/DefaultEngine.ini`의 콜리전 프로파일
- **확인했고 문제 없던 항목**:
  - CLAUDE.md 규칙 위반 0건 — `WxCore` 외 Wx 플러그인 참조 없음(`.uplugin`·`Build.cs`·인클루드 전수 확인, 외부 Wx 헤더는 `WxGameplayTags.h`·`WxCollisionChannels.h` 둘뿐), `Wx` prefix 전수 일치, `FORCEINLINE`·인라인 정의 0건, 람다 0건, 델리게이트 바인딩 25건 전부 `Handle` prefix, `BlueprintCallable`은 `Public/WxCombatLibrary.h:34`(BP Function Library) 한 곳뿐, 저작권 첫 줄 146파일 전부 통과(일부 파일에 UTF-8 BOM이 앞서지만 문구는 정상).
  - 최신 커밋의 `Event.Hit` 태그 통합은 정합적이다 — `FWxDamageTableRow::HitReactTag`가 스펙 동적 태그로 실려(`WxDamageTableRow.cpp:24-27`) `ProcessDamageTaken`이 `Filter(Event.Hit).First()`로 뽑고(`WxCombatAttributeSet.cpp:279`), HitReact는 자식 태그만 트리거로 등록해 부모 평타에 반응하지 않으며(`WxAbility_HitReact.cpp:35-51`), Guard는 `OnlyMatchExact=false`로 부모에서 받는다(`WxAbility_Guard.cpp:89`). 가드 불가 히트가 이벤트보다 먼저 가드를 끊는 순서(`WxCombatAttributeSet.cpp:281-286`)도 맞다.
  - `UWxAbilityBase::GetCooldownGameplayEffect`가 다중 충전에서만 만드는 GE 인스턴스는 죽은 코드가 아니다 — 적용 스펙은 CDO로 만들어지지만 `StackLimitCount`는 `Plugins/WxUI/.../WxViewModel_Ability.cpp:21-24`가 최대 충전 수 표시에 읽는다(헤더 주석대로).
  - 직전 리뷰의 🟡/🟢 중 InputDirection 서버 불일치(`WxTargetingFilterTask_InputDirection.cpp:32`가 `GetCurrentAcceleration`으로 교체), AbilitySet 재빙의 중복 부여(`WxAbilitySystemComponent.cpp:21-26` 가드 추가), Hit 큐 리슨 서버 이중 셰이크(`WxCueNotify_Hit.cpp:55,67` 로컬 컨트롤러 게이트), GhostTrail 배치·LifeSpan(`WxCueNotify_GhostTrail.cpp:40`·헤더 `:42` ClampMin), SlowTime World 널 가드(`WxAbilityTask_SlowTime.cpp:21-26,51-56`), 가드 배율 상수화(`WxEffect_Guard.h:24`)는 전부 실제로 수정됐다.
  - 홀드 입력 경로의 회당 힙 할당은 해소됐다(`WxAbilitySystemComponent.cpp:66-74,186-200`의 두 배열 직접 순회, `WxAbilityBase.cpp:418`의 활성 GE 직접 순회). 남은 것은 스펙 선형 스캔뿐이라 별도 항목으로 세지 않았다.
- **미검토 / 한계**:
  - 무기 히트 판정이 시뮬 프록시에서도 그대로 도는 것(`WxWeaponBase.cpp:257` 주석이 "클라와 서버가 같은 히트 판정과 GE 적용을 수행한다"고 명시)은 이번에도 의도된 설계로 보고 세지 않았다. 다만 그 경로로 `Event_DodgeSuccess`(`WxCombatLibrary.cpp:47`)가 비권위 머신에서도 발송돼 피격자 클라가 서버엔 없던 극한 회피를 로컬로 예측할 수 있다 — 재검토가 필요하면 이 지점부터 본다.
  - `UWxCombatLibrary::ApplyDamage`는 `Event_DodgeSuccess`를 보낸 뒤(`:47`)에야 DamageRow 유효성을 검사하므로(`:50-54`), Row가 비어 대미지를 하나도 못 넣는 공격도 극한 회피 보상을 유발한다. 저작 실수에서만 드러나 항목으로 세지 않았다.
  - `WxTargetingFilterTask_ScreenBounds.cpp:26-29`의 조기 반환 때문에 `:34`의 뷰포트 0 가드가 사실상 도달 불가이고, 비로컬 PlayerController를 소스로 쓰면 모든 후보가 제외된다. 현재 유일 호출부가 `IsLocallyControlled()` 뒤라 라이브 버그가 아니어서 항목으로 세지 않았다.
  - `WxAnimNotifyState_CameraMove.cpp`의 `#if WITH_EDITOR` 프리뷰 경로(78-133, 183-193행)는 에디터 전용이라 정합성만 훑었다.
  - 락온 대상 Server RPC 무검증은 `Public/Targeting/WxLockOnManagerComponent.h`에 "PvE 코옵 전제라 서버에서 재검증하지 않는다"로 신뢰 모델이 명시돼 결정 사항으로 처리했다.
  - BP/WBP 에셋 내부(콤보 몽타주 배치, ANS 구간, `AbilityDataRow`/`DamageTableRow` 실제 값, AbilitySet 에셋, `TP_Attack_*` 타겟팅 프리셋)는 범위 밖이라 데이터 저작 실수로만 드러나는 결함은 잡지 못했다.
  - 멀티플레이 실측(2번의 실제 RTT 창, 4번의 코옵 충돌 빈도)은 전부 정적 분석이다.

---
*문서 기준 커밋 `cf3a7a0` · 리뷰일 2026-08-25 · 소스 146파일 — `/module-review`로 갱신*
