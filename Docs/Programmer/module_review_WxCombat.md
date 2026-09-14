# WxCombat — 코드 리뷰

> 발동 그룹·액션 페이즈·표 수치·서버 권위 적용이라는 축이 일관되고, 직전 리뷰의 가장 큰 결함(타격 GE의 과거 예측 키 재사용)은 `ApplyDamage`를 권위 전용·무키 적용으로 바꾸면서 해소됐다. 남은 결함은 이벤트로 발동하는 반응 어빌리티의 부수효과, 비권위 머신의 시간 배율 보정, 여러 주체가 참조 수 없이 나눠 쥔 상태(AI 두뇌 일시정지·AnimatingAbility)에 몰려 있다.
> 이번 리뷰는 187파일 전체를 기계 규칙 스캔하고 어빌리티 15종·ASC·입력 버퍼·어트리뷰트·타격 Wrapper와 대미지 실행·노티파이·태스크·타겟팅·무기/투사체/소환 cpp를 읽었으며, 발동 순서·예측·몽타주·BT 판단은 설치된 UE 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 2 |
| 🟡 개선 | 2 |
| 🟢 사소 | 5 |

## 결과

### 1. 🔴 반응 없는 히트도 `UWxAbility_HitReact`를 발동시켜, 진행 중인 넉다운 등 반응 몽타주를 끊고 공격·스킬을 취소한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:21-22`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:33-38`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:77-81`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:134-147`
- **범주**: 버그/정확성
- **문제**:
  - 타격 Wrapper는 반응 태그가 비어도 `Event.Hit`을 보내고(`WxEffectComponent_Hit.cpp:134-147`), HitReact는 부모 `Event.Hit`에 트리거가 걸려 모든 공격 히트에서 발동을 시도한다. 반응 없음은 `ActivateAbility` 안에서야 걸러 즉시 종료한다(77-81행).
  - 그 시점엔 엔진이 이미 두 가지를 실행했다. ① `bRetriggerInstancedAbility`(33행)라 HitReact가 활성 중이면 기존 인스턴스를 `EndAbility`로 끝내고(엔진 `AbilitySystemComponent_Abilities.cpp:1836-1844`), 베이스 몽타주 태스크가 `bStopWhenAbilityEnds=true`(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:269-270`)라 재생 중인 넉다운·넉업 몽타주가 멈춘다. ② `PreActivate`가 `CancelAbilitiesWithTag`(21-22행)로 `Ability.Attack`·`Ability.Skill` 발동을 취소한다(엔진 `GameplayAbility.cpp:999`).
  - 결과적으로 넉다운 도중 들어온 무반응 후속타(다단 히트·광역 노티파이 등)가 쓰러짐 연출을 끊어 캐릭터가 곧바로 일어서고, 무반응 타가 공격·스킬 몽타주를 반응 연출 없이 끊는다. 헤더의 "반응 없는 히트는 이 어빌리티가 즉시 종료한다"(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_HitReact.h:14`)와 대미지 행의 "비워 두면 이 공격은 피격 반응을 일으키지 않는다"(`Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h:20-25`) 계약이 실제로는 지켜지지 않는다.
  - 가드 브레이크 조기 종료(50-55행)도 같은 구조다 — 지금은 `Ability.Guard` 차단과 겹쳐 드러나지 않을 뿐이다.
- **제안**: 무반응·가드 브레이크 판정을 순정 훅 `ShouldAbilityRespondToEvent` 오버라이드로 옮긴다 — 엔진이 `CanActivateAbility`·재발동 종료·`PreActivate`보다 먼저 부른다(엔진 `AbilitySystemComponent_Abilities.cpp:1800-1810`). `TargetTags`에 `HitReact.*`가 없고 이벤트가 `Event.Hit.Parry`도 아니면(패리는 `TargetTags` 없이 온다, `WxEffectComponent_Hit.cpp:187-193`) false, `Event.Hit.GuardBreak`면 false를 돌려준다. 무반응 타가 공격을 끊는 것이 의도라면 훅에서 "이미 활성 중 && 무반응"만 걸러 반응 몽타주 절단만 막는다.
- **확신도**: 높음(엔진 발동 순서를 소스로 확인) — 공격·스킬 취소 부분은 18-19행 주석으로 보아 의도일 수 있고, 발현 폭은 `HitReactTag`를 비운 대미지 행 저작에 달렸다.

### 2. 🔴 컷신 태스크가 비권위 머신에서도 시퀀스를 요청 배율 기준 1000배속으로 재생해, 원격 소유 클라에서는 궁극기 컷신이 첫 틱에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117-120`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:58-64`
- **범주**: 버그/정확성
- **문제**: 전역 배율은 권위에서만 걸고(58-64행) 클라에는 `AWorldSettings` 복제로 늦게 도착하는데, 재생 속도 보정 `SetPlayRate(1 / GlobalTimeDilation)`은 머신 구분 없이 요청값 0.001(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:54`) 기준으로 건다. 시퀀스는 월드 게임 시간 델타로 진행하므로(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:328-343`), 배율이 아직 1인 소유 클라에서는 한 프레임(약 16ms)이 16초로 진행해 곧바로 `OnFinished`에 닿는다. 궁극기는 베이스 기본값 LocalPredicted(`WxAbilityBase.cpp:19`)라 소유 클라가 서버보다 먼저 발동하므로 이 창은 거의 확정적으로 열린다. 그러면 `HandleCutsceneCompleted`(`WxAbility_Ultimate.cpp:64-77`)가 서버보다 컷신 길이만큼 먼저 `UltimateMontage`를 틀고, 뒤이어 도착한 0.001 배율이 그 몽타주를 세운다 — 원격 플레이어는 컷신을 보지 못하고 몽타주·노티파이 타이밍도 서버와 어긋난다. 스탠드얼론·리슨 서버 호스트는 배율이 같은 프레임에 걸려 드러나지 않는다.
- **제안**: 보정값을 요청값이 아니라 그 머신에 실제로 걸린 배율에서 구해 배율이 바뀔 때 다시 맞추거나, 시퀀스를 월드 배율과 무관한 클럭으로 돌려 보정 자체를 없앤다.
- **확신도**: 중간(엔진 틱 경로는 소스로 확인, 네트워크 PIE 실측은 없다)

### 3. 🟡 돌진 modifier와 그로기가 참조 수 없는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 AI가 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:65`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:173-190`
- **범주**: 설계/구조
- **문제**: `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `BehaviorTreeComponent.cpp:173-176`, `BehaviorTreeComponent.cpp:187-194`). 돌진은 시작 때 자기가 멈춘 경우를 기억했다가 해제 때 `IsPaused()`면 무조건 재개한다. 돌진 중 GP가 차 그로기가 뜨면, 그로기의 `Ability` 취소(`WxAbility_Groggy.cpp:32`)가 돌진 몽타주를 블렌드아웃시키고 곧이어 두뇌를 멈춘다(65행, 이미 멈춰 있어 사실상 무동작). 블렌드아웃이 끝나 몽타주가 종료되면 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부르고(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1743-1768`) `UWxAnimNotifyState_Rush::BranchingPointNotifyEnd`(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`) → `CancelRush` → `ReleaseState`가 두뇌를 재개한다. 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다. 콘텐츠 검색상 돌진 노티파이는 소환물 스킬 몽타주(`AM_Minion_Skill_2`)에 있어 AI 소환물이 대상이다.
- **제안**: 재개를 기억한 플래그가 아니라 현재 상태에서 파생한다 — 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(76행)가 재개를 맡게 한다. 두 곳이 같은 "지금 멈춰야 하는가" 판정을 공유하면 새 일시정지 주체가 늘어도 같은 충돌이 생기지 않는다.
- **확신도**: 중간(엔진 경로는 소스로 확인, 돌진 몽타주 블렌드아웃이 0이면 해제가 그로기 일시정지보다 먼저 와 드러나지 않음, 소환물 MaxGP 저작에 달림)

### 4. 🟡 사망 어빌리티가 AnimatingAbility를 놓지 않아, 시체 메시의 본 갱신 강제가 파괴될 때까지 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:66-68`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23-43`
- **범주**: 성능/안전
- **문제**: ASC는 몽타주 재생 시 메시를 `AlwaysTickPoseAndRefreshBones`로 올리고 마지막 AnimatingAbility가 해제될 때만 되돌린다(`WxAbilitySystemComponent.cpp:79-110`). 엔진이 해제하는 곳은 어빌리티 종료(엔진 `AbilitySystemComponent_Abilities.cpp:1241-1243`)와, 인터럽트이거나 `bAllowInterruptAfterBlendOut=false`인 태스크 블렌드아웃(엔진 `AbilityTask_PlayMontageAndWait.cpp:32-37`)뿐인데, 베이스는 그 플래그를 true로 넘기고(`WxAbilityBase.cpp:269-270`) 사망은 끝나지 않으며 완료 훅도 비어 있다. 래그돌로 떨어지는 인터럽트 경로만 해제되고, 사망 몽타주로 쓰러진 시체는 서버(플레이어면 소유 클라도)에서 렌더 여부와 무관하게 매 프레임 본을 갱신한다 — `ACharacter` 기본값 `AlwaysTickPose`(엔진 `Engine/Source/Runtime/Engine/Private/Character.cpp:125`)라면 건너뛸 비용이다. `PendingDestroyTime` 기본값 0은 "파괴하지 않음"이라(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Death.h:45-47`) 오픈월드에서 시체가 쌓일수록 서버 비용이 는다.
- **제안**: 사망 몽타주가 끝나는 지점(`UWxAbility_Death::HandleMontageCompleted`, 자세 유지형이라 완료가 오지 않으면 재생 길이 시점)에서 `ClearAnimatingAbility(this)`를 부른다 — 몽타주 자체는 멈추지 않는다.
- **확신도**: 중간(엔진 해제 경로는 소스로 확인, 실제 비용은 시체 수명 저작에 달림)

### 5. 🟢 비권위 머신의 종료 경로 GE 제거는 권위 게이트에 막혀 경고만 남기고, 컷신 태스크만 게이트를 우회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:225-232`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:242-250`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본 0이라 비권위 `RemoveActiveGameplayEffect`가 핸들 유효성과 무관하게 Warning을 찍고 false를 돌려준다(엔진 `AbilitySystemComponent.cpp:1249-1260`). `ActivationOwnedEffectHandles`는 적용에 실패한 무효 핸들까지 담고(230행, 비권위·무키면 엔진 `GameplayAbility.cpp:2040-2053`이 빈 핸들을 준다) 종료에서 머신 구분 없이 제거하므로, 소유 클라에서 `ActivationOwnedEffects`를 쓰는 어빌리티(C++ 기본값으로는 궁극기·처형, 처형은 ServerInitiated라 클라 핸들이 늘 무효)가 끝날 때마다 효과 수만큼 경고가 난다. 질주의 속도 배율·드레인 제거도 같다. 실제 정리는 예측본은 키 확인이, 서버본은 복제가 맡아 동작은 맞지만 242행 주석("효과가 새지 않는다")은 클라 제거가 동작한다고 읽힌다. 반대로 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 권위 검사 없이 컨테이너를 직접 부르므로(엔진 `AbilitySystemComponent.cpp:1292-1318`, 게이트가 있는 `RemoveActiveEffects`는 `AbilitySystemComponent.cpp:1832-1840`) 소유 클라가 복제된 무적 GE를 서버보다 먼저 로컬에서 걷는다 — 2번과 겹치면 컷신 길이만큼 이르다.
- **제안**: 제거 호출은 권위에서만 하고 비권위는 핸들만 비운다(무효 핸들은 애초에 담지 않는다). 컷신 태스크의 정의 기반 제거도 권위로 한정하고, `WxAbilityBase.cpp` 242행과 `WxAbilityTask_PlaySkillCutscene.cpp` 26행 주석을 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 6. 🟢 호출자 없는 선언 2건 — 그중 `EWxAbilityActivationPolicy::OnGiven`은 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:311-317`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h:49`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:52-55`
- **범주**: 중복/복잡도
- **문제**:
  - `OnGiven`은 C++ 어디서도 고르지 않고 콘텐츠 에셋 문자열 검색에서도 저장된 값이 없다. `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(엔진 `GameplayAbilityTypes.cpp:295`) 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다 — LocalPredicted 기본값에서는 클라 예측 발동과 서버발 활성 통지가 겹친다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
  - `UWxAbility_Finisher::IsBackstab()`은 public이지만 UFUNCTION이 아니고 호출자가 없다 — 내부는 `bBackstab`을 직접 읽는다.
- **제안**: 둘 다 걷는다. `OnGiven`을 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(`OnGiven` 경합 양상)

### 7. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`WxAbilityBase.cpp:21-22`). `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 두어, 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나에 서로 막힌다. `OnGiveAbility` 진단(`WxAbilityBase.cpp:304-309`)은 "쿨다운 태그 없음"만 잡으므로 이 실수를 통과시킨다. 슬롯 2~4 전용 GE를 추가한 작업도 BP 지정을 후속으로 남기며 "지정 전까지는 넷 모두 슬롯 1 GE를 물려받아 쿨다운을 공유한다"고 적었다(`.claude/worklog/2026-09-12-스킬-2-3-4-쿨다운-GE-추가.md:43`). 같은 생성자의 애셋 태그 기본값(10-15행)은 빠뜨려도 안전한 방향이지만 쿨다운 기본값은 반대 방향이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌린다 — 그러면 기존 Error 진단이 슬롯 BP의 지정 누락을 잡는다.
- **확신도**: 중간(현재 슬롯 2 BP들은 에셋 문자열상 이 값을 덮어쓰고 있어 당장의 발현은 새 슬롯 BP에 달림)

### 8. 🟢 `ApplyDamage`가 컨텍스트 어빌리티를 타격 시점의 AnimatingAbility로 채워, 되돌린 투사체의 적중이 그 순간 재생 중인 다른 발동에 묶인다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:49-53`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Passive.cpp:23-33`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:57-63`
- **범주**: 버그/정확성
- **문제**: 패시브의 "공격 발동당 1회"는 `Event.DamageDealt` 컨텍스트에 실린 어빌리티 인스턴스의 현재 활성화 키로 판정한다(`WxAbility_Passive.cpp:25-31`). 무기·광역처럼 적중이 그 발동의 몽타주 노티파이에서 나오면 맞지만, 투사체는 날아간 뒤 맞으므로 그 순간 재생 중인 다른 발동이 실린다. 퍼펙트 가드로 되돌린 투사체는 출처가 패리한 플레이어로 바뀌므로(62-63행), 그 플레이어가 공격 콤보 중일 때 적중하면 그 단의 활성화 키로 묶여 같은 단의 근접 적중과 합쳐 한 번만 지급된다(패시브 지급 누락). 몽타주가 끝난 뒤의 투사체만 따로 친다는 `WxAbility_Passive.cpp:24` 주석의 전제가 이 경우엔 빠져 있다.
- **제안**: 원인 액터가 공격자 자신도 아니고 공격자에게 부착돼 있지도 않으면(투사체) 컨텍스트 어빌리티를 비운다 — 부착 여부는 `UWxAbility_GuardReact`가 원인 액터를 가르는 기준(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp:76`)과 같고, 광역·처형 노티파이는 원인이 공격자 자신이라 영향이 없다. 그러면 투사체 적중은 무효 키로 들어가 패시브 주석대로 개별 지급된다.
- **확신도**: 중간(C++상 플레이어 쪽 투사체 경로는 현재 되돌림뿐이고, 발현은 패시브 BP 저작에 달림)

### 9. 🟢 `WxCombat.uplugin`이 모듈이 의존하는 Niagara 플러그인을 선언하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs:31`, `Plugins/WxCombat/WxCombat.uplugin:15-36`
- **범주**: 설계/구조
- **문제**: 모듈은 `Niagara`를 비공개 의존으로 두고 투사체·큐가 Niagara 타입을 쓰지만(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:13-14`), 서술자의 `Plugins` 목록에는 없다. UBT는 이 조합을 플러그인 검증에서 "does not list plugin" 경고로 낸다(엔진 `Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:1574-1577`). 같은 저장소의 `Plugins/WxWorld/WxWorld.uplugin:26`은 Niagara를 선언해 두었다. Niagara가 엔진 기본 활성이라 당장 로드가 깨지지는 않는다.
- **제안**: `WxCombat.uplugin`의 `Plugins`에 Niagara 항목을 더한다.
- **확신도**: 높음(선언 누락), 중간(경고 노출 — 로컬 UBT 로그에서는 확인하지 못함)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 파생 어빌리티 15개 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 태스크 5개 전부, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ComboWindow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_StartRecovery.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_FinisherDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, 대응 Public 헤더
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지(CameraMove·SnapToTarget·SlowTime·SpawnProjectile·SpawnMinion·CommandMinion·SendGameplayEvent), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰·락온 지점, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatModule.cpp`
- **미검토 / 한계**:
  - 규칙 스캔(187파일 기계 검사): 첫 줄 Copyright 누락 0건, `FORCEINLINE`·`inline`·람다 0건, 헤더 안 함수 본문 0건(GAS 표준 `ATTRIBUTE_ACCESSORS` 매크로 전개 제외), 타입 prefix `Wx` 누락 0건, `BlueprintCallable`은 BP 함수 라이브러리(`UWxCombatLibrary::ApplyDamage`)뿐이다. Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include` 모두 `WxCore`뿐이다.
  - 멀티플레이 실측이 없다. 1·2·3·4·5·6번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이며 PIE 재현은 하지 않았다.
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열 검색은 보조 근거로만 썼다(`OnGiven` 저장값 없음, 돌진 노티파이 배치, 슬롯 2 스킬 BP의 쿨다운 GE 덮어쓰기). 1·3·4·7·8번의 실제 발현 폭은 저작에 달려 있다.
  - 직전 리뷰(`9d8cb2dd`) 대비: 1번(타격 GE의 과거 활성화 예측 키 재사용)은 `ApplyDamage`가 권위 전용·`FPredictionKey()` 적용으로 바뀌어(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:44`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:69`) 해소를 확인해 뺐고, 그중 컨텍스트 어빌리티 부분만 8번으로 남겼다. 나머지(컷신 배속·사망 AnimatingAbility·클라 GE 제거·`OnGiven`·스킬 쿨다운 기본값·Niagara 선언)는 현재 코드로 재검증해 유효한 것만 옮겼다.
  - 올리지 않은 항목: 콤보 창의 태그 요건 전체 면제는 `.claude/worklog/2026-09-10-콤보-창-재발동-태그-면제.md`가 폭을 알고 택한 결정이다. Attack·Skill·Pattern 콤보 코드 중복은 확정된 감수 사항이다. `WxAnimNotifyState_CameraMove`가 적·AI 몽타주에서 각 클라의 로컬 뷰를 바꾸는 것은 주석에 명시된 의도다. 히트스톱·타격 큐가 예측 없이 서버 복제를 따르는 것은 현재 헤더(`WxEffect_HitStop.h`, `WxCombatLibrary.h`)가 명시한 방침이다. `WxAnimNotify_AreaDamage`가 비권위 머신에서도 타겟팅 요청을 실행하는 것은 결과가 버려질 뿐 동작 결함이 아니라 올리지 않았다. `WxAnimNotifyState_ApplyGameplayEffect` 종료가 같은 GE 클래스의 모든 인스턴스에서 스택 하나씩 걷는 것은 주석에 명시돼 있고, 실제 겹침 시나리오를 코드로 확인하지 못했다.

---
*문서 기준 커밋 `28fe02f12` · 리뷰일 2026-09-15 · 소스 187파일 — `/module-review`로 갱신*
