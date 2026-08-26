# WxCombat — 코드 리뷰

> 직전 리뷰의 🔴(무기 틱 Sweep이 오브젝트 타입 쿼리라 몸통 캡슐까지 잡는 문제)는 채널 Sweep 전환으로 해소됐지만, 그 전환이 극한 회피 판정 캡슐을 어느 히트 경로에도 잡히지 않게 만들었다 — 대미지 파이프라인·어빌리티 라우팅·상태 전이의 뼈대 자체는 여전히 정합적이고 CLAUDE.md 규칙 위반은 148파일 전수 검사에서 0건이다. 이번 리뷰는 직전 리뷰 이후 바뀐 파일(무기·투사체·대미지 Row·어트리뷰트셋·회피·신규 `UWxAbility_Passive`)을 새로 읽고, 대미지 진입점·ExecCalc·어트리뷰트셋·ASC·AbilityBase·14개 어빌리티·AnimNotify 9종·Cue 6종·AbilityTask 4종·Targeting 8종·GE 20종을 cpp까지 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 5 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 극한 회피 판정 캡슐이 무기 Sweep에 잡히지 않아 사실상 죽은 코드다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:264-266`
- **범주**: 버그/정확성
- **문제**: 직전 리뷰의 🔴을 고치면서 무기 틱 Sweep이 `SweepMultiByObjectType(ECC_Pawn)`에서 `SweepMultiByChannel(ECC_WxAttack, ResponseParams=형상 자신의 응답)`으로 바뀌었고(`Private/Weapon/WxWeaponBase.cpp:212`), 같은 커밋에서 판정 캡슐의 오브젝트 타입도 `ECC_Pawn` → `ECC_WxAttack`으로 바뀌었다. 그런데 캡슐은 여전히 `SetCollisionResponseToAllChannels(ECR_Ignore)`이고 `SetGenerateOverlapEvents(false)`다.

  채널 쿼리는 양방향 응답의 최솟값을 취한다(`UE_5.8/.../Chaos/Private/Chaos/CollisionFilterData.cpp:467` `ChannelTypeNarrowFilter`). 두 방향 모두 Ignore다 — (a) 캡슐의 트레이스 채널 `ECC_WxAttack`에 대한 응답이 Ignore, (b) 쿼리가 넘긴 응답 컨테이너는 무기 히트박스의 것인데 `ECC_Pawn`만 Overlap이라(`WxWeaponBase.cpp:166-168`) 캡슐의 오브젝트 타입 `ECC_WxAttack`에 대해서도 Ignore다. 결과적으로 Sweep은 캡슐을 절대 반환하지 않고, 오버랩 이벤트 경로는 캡슐이 이벤트 생성을 꺼 둬서 애초에 닿지 않는다. 투사체(`WxProjectile` 프로파일도 `WxAttack=Ignore`)도 마찬가지다.

  구체적 실패: 헤더가 선언한 설계(`Public/AbilitySystem/Ability/WxAbility_Dodge.h:33-34` — "판정 캡슐이 '피하지 않았다면 맞았을 자리'를 추가로 덮는다")가 통째로 무효가 된다. 무적 구간에 몸을 실제로 빼낸 회피는 아무것도 맞지 않으므로 `Event.DodgeSuccess`가 나가지 않고(`Private/WxCombatLibrary.cpp:50-58`), 극한 회피 몽타주·슬로우가 뜨지 않는다. 남는 성립 경로는 "무적 중에 몸통 메시가 그대로 맞은" 경우뿐이라, 잘 피할수록 보상을 못 받는 역전이 생긴다. 태그·이벤트 배선은 정상이라 로그로는 드러나지 않는다.
- **제안**: 두 방향을 모두 열어야 한다. 가장 작은 수정은 캡슐을 `ECC_Pawn` 오브젝트 타입으로 되돌리고(무기 응답 컨테이너가 이미 Pawn을 Overlap한다) `SetCollisionResponseToChannel(ECC_WxAttack, ECR_Overlap)`을 명시로 추가하는 것이다 — 캐릭터 메시가 피격에 참여하는 방식과 정확히 같아지고, 다른 채널은 여전히 Ignore라 이동·시야·카메라 트레이스에는 걸리지 않는다. 캡슐을 `ECC_WxAttack`으로 유지하려면 무기 히트박스 쪽에도 `ECC_WxAttack = ECR_Overlap`을 더해야 하는데, 그러면 무기끼리도 서로 잡히므로 권하지 않는다.
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

  `UWxCombatLibrary::RemoveEffect`는 정의(클래스) 기준으로 아무 인스턴스나 1개를 지운다(`Private/WxCombatLibrary.cpp:123`). 즉 컷신이 시작조차 못 한 프레임에, 처형이 `ActivationOwnedEffects`로 걸어 둔 무적(`Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:31`)이나 i-frame ANS가 건 무적을 대신 벗길 수 있다.
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
- **문제**: `NotifyBegin`은 SpawnActor 실패(`:51-54`)로 조기 반환할 수 있는데, `NotifyEnd`는 그런 사실을 모른 채 항상 `SetViewTargetWithBlend(PC->GetPawn(), ...)`를 부른다. 더 자주 드러나는 쪽은 소유권 검사가 없다는 점이다 — 같은 몽타주를 재생하는 액터가 둘이거나 다른 연출(궁극기 시퀀서 등)과 구간이 겹치면, 먼저 끝난 쪽이 아직 살아 있는 남의 카메라를 폰으로 회수한다. 사망 몽타주에서 폰이 이미 언포제스됐다면 `PC->GetPawn()`이 널이라 뷰타겟이 PlayerController 자신으로 넘어간다.
- **제안**: 노티파이 오브젝트는 애셋 단위 공유라 bool 멤버를 둘 수 없으므로, `PC->GetViewTarget()`이 `MeshComp->GetOwner()`를 오너로 갖는 `ACameraActor`일 때만 되돌리는 무상태 검사를 쓴다.
- **확신도**: 중간(코드 경로는 확실, 발생 빈도는 연출 배치에 달렸다)

### 6. 🟡 Attack·Skill·Pattern의 콤보 진행 코드가 3중 복제다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19-54`, `.../WxAbility_Skill.cpp:21-56`, `.../WxAbility_Pattern.cpp:19-63`
- **범주**: 중복/복잡도
- **문제**: Attack과 Skill의 `ActivateAbility`/`EndAbility`/`HandleMontageCompleted`는 클래스 이름을 치환하면 **바이트 단위로 동일**하다(차이는 생성자의 태그와 주석뿐). Pattern도 앞 두 함수가 같고 `HandleMontageCompleted`만 자동 진행이다. 헤더의 `ComboMontages`/`ComboIndex`도 셋에 각각 선언돼 있으며 에디터 카테고리가 이미 갈렸다 — `WxAbility_Attack.h:33`은 `"Wx"`, `WxAbility_Skill.h:36`은 `"Wx|Ability"`, `WxAbility_Pattern.h:29`는 `"Wx"`.

  복제가 이미 실제 결함을 만들었다. Pattern은 `bRetriggerInstancedAbility`를 켜지 않는데(생성자 `:6-17`) `ActivateAbility:29`의 인덱스 진행 줄을 그대로 복사했다. 정상 종료 시 `ComboIndex`가 마지막 인덱스라 다음 발동은 0이 되어 평소엔 무해하지만, `ComboMontages` 중간에 빈 슬롯이 있어 `HandleMontageCompleted:59-62`가 재생 실패로 `bWasCancelled = false` 종료하면 `ComboIndex`가 중간 값 k로 남고, 다음 발동이 0이 아니라 k+1부터 시작해 앞 단계를 조용히 건너뛴다. 헤더 주석(`WxAbility_Pattern.h:12`)도 "단일 몽타주를 재생한다"로 남아 있어 자동 체인 동작과 어긋난다.
- **제안**: `ComboMontages`/`ComboIndex`와 진행·리셋 규칙을 중간 베이스(`UWxAbility_ComboBase` 등)로 올리고 Pattern만 `HandleMontageCompleted`를 오버라이드해 자동 진행을 얹는다. Pattern에서는 활성화 시 항상 0에서 시작하도록 정리하고 헤더 주석도 실제 동작에 맞춘다.
- **확신도**: 높음

### 7. 🟢 히트스톱 복원 배속이 어빌리티의 PlayRate 오버라이드를 무시한다 *(직전 리뷰 미해소)*
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:252`
- **범주**: 버그/정확성
- **문제**: 얼렸던 몽타주를 ASC의 ASPD 기반 `GetMontagePlayRate()`로 복원하는데, 그 몽타주는 `UWxAbilityBase::PlayMontage`(`Private/AbilitySystem/Ability/WxAbilityBase.cpp:200`)가 **어빌리티의** `GetMontagePlayRate()`로 재생한 것이다. Dodge·Guard·HitReact·Finisher·Death는 이 함수를 `1.f`로 오버라이드하므로(예: `WxAbility_HitReact.cpp:54-57`), ASPD가 1이 아닌 캐릭터에서 그런 몽타주 중 히트스톱이 걸리면 복원 후 재생 속도가 원래와 달라진다. `ApplyHitStop`이 이미 `SourceAbility`를 받고 있어(`:139`) 고치기는 쉽다.
- **제안**: 프리즈 시점에 `SourceAbility->GetMontagePlayRate()`를 캡처해 타이머 델리게이트로 함께 넘기고 그 값으로 복원한다.
- **확신도**: 중간(PlayRate를 1로 고정한 어빌리티 몽타주에 히트스톱 유발 노티파이가 실제로 배치돼야 드러난다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `.../Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `.../Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `.../Private/Damage/WxDamageTableRow.cpp`, `.../Private/Damage/WxCombatEffectContext.cpp`, `.../Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `.../Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `.../Private/AbilitySystem/WxAbilitySet.cpp`, 14개 `WxAbility_*.cpp` 전량(특히 이번에 바뀐 `WxAbility_Dodge.cpp`과 신규 `WxAbility_Passive.cpp`), `.../Private/AnimNotify/` 9파일 전량, `.../Private/AbilitySystem/Cue/` 6파일 전량, `.../Private/AbilitySystem/Task/` 4파일 전량, `.../Private/Targeting/` 8파일 전량, `.../Private/Weapon/WxWeaponBase.cpp`, `.../Private/Weapon/WxProjectileBase.cpp`, `.../Private/Time/WxTimeDilationComponent.cpp`, `.../Private/AbilitySystem/Effect/` 20파일, 대응 Public 헤더
- **훑은 파일**: `Plugins/WxCombat/WxCombat.uplugin`, `.../WxCombat.Build.cs`, `.../Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `.../Private/WxCombatModule.cpp`, `.../Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, 교차 검증용 `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Config/DefaultEngine.ini`의 콜리전 프로파일, 엔진 `Chaos/CollisionFilterData.cpp`·`Enum.cpp`(1번 판정과 회피 섹션 이름 조회 검증용)
- **확인했고 문제 없던 항목**:
  - CLAUDE.md 규칙 위반 0건 — `WxCore` 외 Wx 플러그인 참조 없음(`.uplugin`·`Build.cs`·인클루드 전수 확인, 외부 Wx 헤더는 `WxGameplayTags.h`·`WxCollisionChannels.h` 둘뿐), `Wx` prefix 전수 일치, `FORCEINLINE`·인라인 정의 0건, 람다 0건, 델리게이트 바인딩 25종 전부 `Handle` prefix, `BlueprintCallable`은 `Public/WxCombatLibrary.h:44`(BP Function Library) 한 곳뿐, 저작권 첫 줄 148파일 전부 통과(일부 파일에 UTF-8 BOM이 앞서지만 문구는 정상).
  - 직전 리뷰 🔴은 실제로 해소됐다 — 틱 Sweep이 채널 쿼리로 바뀌어(`WxWeaponBase.cpp:212`) 몸통 캡슐(`WxAttack=Ignore`)은 더 이상 잡히지 않고, 메시(`WxAttack=Overlap`)에서만 성립한다. 지형이 Sweep을 자르는 문제도 형상 자신의 응답을 넘겨 막았다. 다만 그 부작용이 1번이다.
  - 아군 오적용 방어가 추가됐다 — `WxWeaponBase.cpp:265-269`가 `IsHostile` 실패 시 피격 기록 전에 반환해 `AdditionalEffects`가 아군에게 걸리지 않는다. 투사체도 `WxProjectileBase.cpp:98-101`에서 권위 검사 뒤에만 `ApplyDamage`를 부른다.
  - 신규 `UWxAbility_Passive`는 정합적이다 — ServerOnly로 트리거되고 발동당 1회 지급을 소스 어빌리티의 활성화 예측 키로 가른다(`WxAbility_Passive.cpp:25-33`). 애님 어빌리티가 없는 히트(투사체 등)는 키가 무효라 그 히트를 1회로 치는 것까지 주석과 코드가 일치한다.
  - 회피 8방향 섹션 로직은 정상이다 — `GetNameStringByValue`가 내는 짧은 이름을 `GetValueByName`이 다시 해석한다(엔진 `Enum.cpp:951` `GetIndexByNameString`이 짧은 이름에 enum 이름을 붙여 재조회). 8분면 양자화(`WxAbility_Dodge.cpp:110-112`)의 음수 각도 처리도 맞다.
  - `Event.Hit` 태그 통합 경로는 여전히 정합적이다 — 스펙 동적 태그 → `ProcessDamageTaken`의 `Filter(Event.Hit).First()`(`WxCombatAttributeSet.cpp:279`, 빈 컨테이너에서도 안전) → HitReact는 자식만 트리거 등록, Guard는 부모 매칭. 가드 불가 히트가 이벤트보다 먼저 가드를 끊는 순서도 맞다.
- **미검토 / 한계**:
  - 무기 히트 판정이 시뮬 프록시에서도 그대로 도는 것(`WxWeaponBase.cpp:257` 주석이 명시)은 이번에도 의도된 설계로 보고 세지 않았다. 그 경로로 `Event_DodgeSuccess`(`WxCombatLibrary.cpp:57`)가 비권위 머신에서도 발송돼 피격자 클라가 서버엔 없던 극한 회피를 로컬로 예측할 수 있다 — 재검토가 필요하면 이 지점부터 본다.
  - `UWxCombatLibrary::ApplyDamage`는 `Event_DodgeSuccess`를 보낸 뒤(`:57`)에야 DamageRow 유효성을 검사하므로(`:66-70`), Row가 비어 대미지를 하나도 못 넣는 공격도 극한 회피 보상을 유발한다. 저작 실수에서만 드러나 항목으로 세지 않았다.
  - 1번을 고쳐도 투사체는 판정 캡슐을 잡지 못한다(캡슐이 오버랩 이벤트를 꺼 두었고 투사체에는 틱 Sweep 경로가 없다). 이는 이번 변경 이전부터 그랬으므로 회귀가 아니라 원래 범위 밖으로 봤다.
  - `WxTargetingFilterTask_ScreenBounds.cpp:26-29`의 조기 반환 때문에 `:34`의 뷰포트 0 가드가 사실상 도달 불가이고, 비로컬 PlayerController를 소스로 쓰면 모든 후보가 제외된다. 현재 유일 호출부가 `IsLocallyControlled()` 뒤라 라이브 버그가 아니어서 항목으로 세지 않았다.
  - `UWxAbility_Sprint`·`UWxAbilityBase`가 활성 GE를 핸들로 제거하는 것(`WxAbility_Sprint.cpp:110-120`, `WxAbilityBase.cpp:175-178`)은 예측으로 건 GE의 클라 핸들이 서버본 도착 후 무효해져 로컬 제거가 no-op이 된다. 서버 종료가 복제로 따라와 수렴하므로 항목으로 세지 않았다.
  - `WxAnimNotifyState_CameraMove.cpp`의 `#if WITH_EDITOR` 프리뷰 경로(78-133, 140-157, 183-193행)는 에디터 전용이라 정합성만 훑었다.
  - 락온 대상 Server RPC 무검증은 `Public/Targeting/WxLockOnManagerComponent.h:52`에 "PvE 코옵 전제라 서버에서 재검증하지 않는다"로 신뢰 모델이 명시돼 결정 사항으로 처리했다.
  - BP/WBP 에셋 내부(콤보 몽타주 배치, ANS 구간, 회피 몽타주의 8방향 섹션 구성, `AbilityDataRow`/`DamageTableRow` 실제 값, AbilitySet 에셋, 타겟팅 프리셋)는 범위 밖이라 데이터 저작 실수로만 드러나는 결함은 잡지 못했다.
  - 멀티플레이 실측(2번의 실제 RTT 창, 4번의 코옵 충돌 빈도)과 1번의 인게임 재현은 전부 정적 분석·엔진 소스 대조다.

---
*문서 기준 커밋 `8a9bb7e6` · 리뷰일 2026-08-26 · 소스 148파일 — `/module-review`로 갱신*
