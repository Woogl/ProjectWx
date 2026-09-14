# WxCombat — 코드 리뷰

> 발동 그룹·액션 페이즈·표 수치·서버 권위 적용이라는 축이 일관되고, 직전 리뷰의 가장 큰 결함(반응 없는 히트가 `UWxAbility_HitReact`를 발동시켜 넉다운·공격을 끊던 문제)은 판정을 순정 훅 `ShouldAbilityRespondToEvent`로 옮기면서 해소됐다. 남은 결함은 비권위 머신의 시간 배율·GE 제거·예측 키 처리, 참조 수 없이 나눠 쥔 상태(AI 두뇌 일시정지·AnimatingAbility), 그리고 이번 반응 게이트 변경 뒤 따라오지 못한 저작 계약 문구에 몰려 있다.
> 이번 리뷰는 187파일 전체를 기계 규칙 스캔하고, 직전 리뷰 이후 바뀐 11파일(HitReact·GuardReact·Passive·Hit 컴포넌트·SpawnProjectile 노티파이·대미지 행·투사체·투사체 서브시스템·무기·전투 라이브러리·uplugin)을 diff와 함께 깊게 읽었으며, 어빌리티 전부·ASC·어트리뷰트·대미지 실행·노티파이·태스크·타겟팅·소환 cpp를 설치된 UE 5.8 엔진 소스와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 6 |

## 결과

### 1. 🔴 컷신 태스크가 비권위 머신에서도 시퀀스를 요청 배율 기준 1000배속으로 재생해, 원격 소유 클라에서는 궁극기 컷신이 첫 틱에 끝난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117-120`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:58-64`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:54`
- **범주**: 버그/정확성
- **문제**: 전역 배율은 권위에서만 걸고(58-64행) 클라에는 `AWorldSettings` 복제로 늦게 도착하는데, 재생 속도 보정 `SetPlayRate(1 / GlobalTimeDilation)`은 머신 구분 없이 요청값 0.001(`WxAbility_Ultimate.cpp:54`) 기준으로 건다. 시퀀스는 월드 게임 시간 델타로 진행하므로(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequenceTickManager.cpp:328-343`), 배율이 아직 1인 소유 클라에서는 한 프레임(약 16ms)이 16초로 진행해 곧바로 `OnFinished`에 닿는다. 궁극기는 생성자에서 실행 정책을 바꾸지 않아 베이스 기본값 LocalPredicted(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:19`)이고 소유 클라가 서버보다 먼저 발동하므로 이 창은 거의 확정적으로 열린다. 그러면 `HandleCutsceneCompleted`(`WxAbility_Ultimate.cpp:64-77`)가 서버보다 컷신 길이만큼 먼저 `UltimateMontage`를 틀고, 뒤이어 도착한 0.001 배율이 그 몽타주를 세운다 — 원격 플레이어는 컷신을 보지 못하고 몽타주·노티파이 타이밍도 서버와 어긋난다. 스탠드얼론·리슨 서버 호스트는 배율이 같은 프레임에 걸려 드러나지 않는다.
- **제안**: 보정값을 요청값이 아니라 그 머신에 실제로 걸린 배율(`UGameplayStatics::GetGlobalTimeDilation`)에서 구해 배율이 바뀔 때 다시 맞추거나, 시퀀스를 월드 배율과 무관한 클럭으로 돌려 보정 자체를 없앤다.
- **확신도**: 중간(코드와 엔진 틱 경로는 소스로 확인, 네트워크 PIE 실측은 없다)

### 2. 🟡 돌진 modifier와 그로기가 참조 수 없는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 AI가 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:65`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:173-190`
- **범주**: 설계/구조
- **문제**: `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `Engine/Source/Runtime/AIModule/Private/BehaviorTree/BehaviorTreeComponent.cpp:173-176`, `BehaviorTreeComponent.cpp:187-194`). 돌진은 시작 때 자기가 멈춘 경우를 기억했다가 해제 때 `IsPaused()`면 무조건 재개한다. 돌진 중 GP가 차 그로기가 뜨면, 그로기의 `Ability` 취소 지목(`WxAbility_Groggy.cpp:32`)이 돌진 몽타주를 블렌드아웃시키고 곧이어 두뇌를 멈춘다(65행, 이미 멈춰 있어 사실상 무동작). 블렌드아웃이 끝나 몽타주 인스턴스가 종료되면 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부르고(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1743-1768`) `UWxAnimNotifyState_Rush::BranchingPointNotifyEnd`(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`) → `CancelRush` → `ReleaseState`가 두뇌를 재개한다. 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다. 콘텐츠 문자열 검색상 돌진 노티파이는 `AM_Minion_Skill_2`(AI 소환물)와 `AM_HGTest_Skill_2`에 있고, 두뇌가 있는 쪽은 소환물이다.
- **제안**: 재개를 기억한 플래그가 아니라 현재 상태에서 파생한다 — 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(`WxAbility_Groggy.cpp:76`)가 재개를 맡게 한다. 두 곳이 같은 "지금 멈춰야 하는가" 판정을 공유하면 새 일시정지 주체가 늘어도 같은 충돌이 생기지 않는다.
- **확신도**: 중간(엔진 경로는 소스로 확인, 돌진 몽타주 블렌드아웃이 0이면 해제가 그로기 일시정지보다 먼저 와 드러나지 않음, 소환물 MaxGP 저작에 달림)

### 3. 🟡 사망 어빌리티가 AnimatingAbility를 놓지 않아, 시체 메시의 본 갱신 강제가 파괴될 때까지 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:66-68`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:23-43`
- **범주**: 성능/안전
- **문제**: ASC는 몽타주 재생 시 메시를 `AlwaysTickPoseAndRefreshBones`로 올리고 마지막 AnimatingAbility가 해제될 때만 되돌린다(`WxAbilitySystemComponent.cpp:79-110`). 엔진이 해제하는 곳은 어빌리티 종료(엔진 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:1240-1244`)와, 인터럽트이거나 `bAllowInterruptAfterBlendOut=false`인 태스크 블렌드아웃(엔진 `Abilities/Tasks/AbilityTask_PlayMontageAndWait.cpp:32-37`)뿐인데, 베이스는 그 플래그를 true로 넘기고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:269-270`) 사망은 끝나지 않으며 완료 훅도 비어 있다. 래그돌로 떨어지는 인터럽트 경로만 해제되고, 사망 몽타주로 쓰러진 시체는 서버(플레이어면 소유 클라도)에서 렌더 여부와 무관하게 매 프레임 본을 갱신한다 — `ACharacter` 기본값 `AlwaysTickPose`(엔진 `Engine/Source/Runtime/Engine/Private/Character.cpp:125`)라면 건너뛸 비용이다. `PendingDestroyTime` 기본값 0은 "파괴하지 않음"이라(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Death.h:45-47`) 오픈월드에서 시체가 쌓일수록 서버 비용이 는다.
- **제안**: 사망 몽타주가 끝나는 지점(`UWxAbility_Death::HandleMontageCompleted`, 자세 유지형이라 완료가 오지 않으면 재생 길이 시점)에서 `ClearAnimatingAbility(this)`를 부른다 — 몽타주 자체는 멈추지 않는다.
- **확신도**: 중간(엔진 해제 경로는 소스로 확인, 실제 비용은 시체 수명 저작에 달림)

### 4. 🟢 권위 전용 GE 노티파이가 발동의 활성화 예측 키를 그대로 실어, 헤더의 "예측 미사용" 계약과 어긋나고 소유 클라의 Cue가 빠질 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:14-20`, `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.h:17-18`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:97-108`
- **범주**: 버그/정확성
- **문제**: 노티파이는 권위에서만 적용하고 14행 주석과 헤더(17-18행)가 "활성화 예측 키를 재사용하지 않는다", "클라이언트 예측은 사용하지 않는다"고 선언하지만, `ApplyEffect`에 `ASC->GetAnimatingAbility()`를 넘기므로 `ApplyEffect`가 그 발동의 활성화 예측 키로 적용한다(`WxCombatLibrary.cpp:102-108`). 원격 플레이어의 LocalPredicted 발동(회피 무적 구간 등)이면 이 키는 소유 클라가 만든 키라 서버 GE에 박혀 소유 연결에만 유효하게 복제되고(엔진 `GameplayPrediction.cpp:129`), 소유 클라의 `PostReplicatedAdd`는 같은 키를 가진 예측 GE(같은 발동의 코스트·쿨다운 예측본)가 아직 남아 있으면 이 GE의 Cue 이벤트를 건너뛴다(엔진 `GameplayEffect.cpp:2825-2835`, `GameplayEffect.cpp:5757-5768`). 발동 직후 열리는 무적 구간이 그 창에 가장 가깝다. `ApplyDamage`가 바로 이 이유("과거 활성화 키를 실으면 예측본 잔류나 소유 클라의 Cue 생략이 발생한다", `WxCombatLibrary.cpp:84`)로 무키 적용으로 바꾼 것과도 어긋난다.
- **제안**: 노티파이 경로는 레벨만 어빌리티에서 받고 `FPredictionKey()`로 적용한다. 실제 예측 적용이 필요한 호출자는 컷신 태스크(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:123`)뿐이므로 `ApplyEffect`의 키 사용을 그쪽 경로로 한정한다.
- **확신도**: 높음(주석·헤더와 코드 불일치), 낮음(Cue 생략 발현은 `EffectClass`에 Cue를 저작했는지와 복제 순서에 달림)

### 5. 🟢 반응 게이트 변경 뒤 `FWxDamageTableRow`의 저작 안내가 낡아, `HitReactTag`를 비운 행에서 패리·가드 브레이크·퍼펙트 가드 연출이 조용히 사라진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h:20-25`, `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h:34-36`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:41-47`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp:40-46`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp:134-139`
- **범주**: 설계/구조
- **문제**: 이번 변경으로 HitReact·GuardReact가 `TargetTags`에 `HitReact.*`가 없는 이벤트를 모두 거부하고, 퍼펙트 가드·패리 이벤트도 원래 공격의 반응 태그만 싣는다(`WxEffectComponent_Hit.cpp:176`, `WxEffectComponent_Hit.cpp:198`). 그래서 `HitReactTag`를 비운 행은 공격자 패리 역경직, 가드 브레이크 몽타주(가드만 해제, 134-139행), 퍼펙트 가드 몽타주까지 없다. 이는 작업 기록이 계획한 동작이지만(`.codex/worklog/2026-09-15-무반응-피격-활성화-차단.md:6`, `:15-16`), 에디터 툴팁으로 노출되는 행 주석은 여전히 "패리·가드 브레이크는 전투 시스템이 별도 이벤트로 생성하므로 저작하지 않는다"(22행), "false이면 퍼펙트 가드로 막아도 공격자가 역경직에 걸리지 않는다"(34행)라고 두 설정이 독립인 것처럼 안내한다. `bCanParry=true`에 반응 태그를 비운 행은 저작자 기대와 달리 역경직이 나지 않는다.
- **제안**: 22행과 34행 주석을 현재 계약으로 고친다 — "비우면 패리·가드 브레이크·퍼펙트 가드 연출도 일어나지 않는다", "`bCanParry`는 `HitReactTag`가 있을 때만 의미가 있다".
- **확신도**: 높음(주석과 동작 불일치 — 동작 자체는 의도)

### 6. 🟢 비권위 머신의 종료 경로 GE 제거는 권위 게이트에 막혀 경고만 남기고, 컷신 태스크만 게이트를 우회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:225-232`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:242-250`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24-28`
- **범주**: 버그/정확성
- **문제**: UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본 0이라 비권위 `RemoveActiveGameplayEffect`가 핸들 유효성과 무관하게 Warning을 찍고 false를 돌려준다(엔진 `AbilitySystemComponent.cpp:1249-1260`). `ActivationOwnedEffectHandles`는 적용에 실패한 무효 핸들까지 담고(230행, 비권위·무키면 엔진 `GameplayAbility.cpp:2040-2053`이 빈 핸들을 준다) 종료에서 머신 구분 없이 제거하므로, 소유 클라에서 `ActivationOwnedEffects`를 쓰는 어빌리티(C++ 기본값으로는 궁극기·처형, 처형은 ServerInitiated라 클라 핸들이 늘 무효)가 끝날 때마다 효과 수만큼 경고가 난다. 질주의 속도 배율·드레인 제거도 같다. 실제 정리는 예측본은 키 확인이, 서버본은 복제가 맡아 동작은 맞지만 242행 주석("효과가 새지 않는다")은 클라 제거가 동작한다고 읽힌다. 반대로 컷신 태스크의 `RemoveActiveGameplayEffectBySourceEffect`는 권위 검사 없이 컨테이너를 직접 부르므로(엔진 `AbilitySystemComponent.cpp:1292-1318`, 게이트가 있는 `RemoveActiveEffects`는 `AbilitySystemComponent.cpp:1832-1840`) 소유 클라가 복제된 무적 GE를 서버보다 먼저 로컬에서 걷는다 — 1번과 겹치면 컷신 길이만큼 이르다.
- **제안**: 제거 호출은 권위에서만 하고 비권위는 핸들만 비운다(무효 핸들은 애초에 담지 않는다). 컷신 태스크의 정의 기반 제거도 권위로 한정하고, `WxAbilityBase.cpp` 242행과 `WxAbilityTask_PlaySkillCutscene.cpp` 26행 주석을 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 7. 🟢 무기 판정이 소유 클라·시뮬 프록시에서도 콜리전과 매 틱 스윕을 켜지만 그 결과를 쓰는 곳이 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:50-78`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:140-189`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12-26`
- **범주**: 성능/안전
- **문제**: 공격 노티파이는 몽타주가 복제돼 도는 모든 머신에서 `BeginAttack`을 부르고, `BeginAttack`은 권위 구분 없이 형상 콜리전을 켜고 액터 틱을 연다(64-75행). 그 뒤 매 틱 형상별 `SweepMultiByChannel`(180행)과 오버랩 이벤트가 `ProcessHit`로 모이지만, 비권위 머신에서는 `ApplyDamage`가 권위 검사에서 곧바로 false를 돌려주고(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:45`) 히트스톱·타격 큐도 서버 판정만 따른다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`, 225행 주석). 즉 클라이언트의 스윕·오버랩은 소비처가 없는 비용이며, 관련성 범위 안에서 동시에 공격하는 적 수만큼 모든 클라에 곱해진다.
- **제안**: `BeginAttack` 첫머리에서 소유 액터가 권위가 아니면 반환한다 — `ActiveAttackCount`가 0으로 남아 `EndAttack`·`CancelAttack`은 그대로 무동작이다.
- **확신도**: 중간(소비처 없음은 코드로 확인, 실제 비용은 동시 교전 규모에 달림)

### 8. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**: 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21-22`). `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 두어, 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나에 서로 막힌다. `OnGiveAbility` 진단(`WxAbilityBase.cpp:304-309`)은 "쿨다운 태그 없음"만 잡으므로 이 실수를 통과시킨다. 슬롯 2~4 전용 GE를 추가한 작업도 BP 지정을 후속으로 남기며 "지정 전까지는 넷 모두 슬롯 1 GE를 물려받아 쿨다운을 공유한다"고 적었다(`.claude/worklog/2026-09-12-스킬-2-3-4-쿨다운-GE-추가.md:43`). 같은 생성자의 애셋 태그 기본값(10-15행)은 빠뜨려도 안전한 방향이지만 쿨다운 기본값은 반대 방향이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌린다 — 그러면 기존 Error 진단이 슬롯 BP의 지정 누락을 잡는다.
- **확신도**: 중간(발현은 새로 만드는 슬롯 BP의 지정 누락에 달림)

### 9. 🟢 호출자 없는 선언 2건 — 그중 `EWxAbilityActivationPolicy::OnGiven`은 쓰면 서버와 소유 클라가 각자 발동한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:311-317`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h:49`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:52-55`
- **범주**: 중복/복잡도
- **문제**:
  - `OnGiven`은 C++ 어디서도 고르지 않고 콘텐츠 에셋 문자열 검색에서도 저장된 값이 없다. `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(엔진 `GameplayAbilityTypes.cpp:295`) 현재 구현은 서버와 소유 클라가 각각 `TryActivateAbility`를 부른다 — LocalPredicted 기본값에서는 클라 예측 발동과 서버발 활성 통지가 겹친다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
  - `UWxAbility_Finisher::IsBackstab()`은 public이지만 UFUNCTION이 아니고 저장소 전체에 호출자가 없다 — 내부는 `bBackstab`을 직접 읽는다.
- **제안**: 둘 다 걷는다. `OnGiven`을 남긴다면 NetExecutionPolicy에 맞는 한쪽(보통 권위)에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용), 중간(`OnGiven` 경합 양상)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Passive.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_SpawnProjectile.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cost.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 파생 어빌리티 15개 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, 대응 Public 헤더
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 나머지 GE·`WxEffectComponent_Table.cpp`·`WxEffect_AddAttribute.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지(CameraMove·SnapToTarget·SlowTime·ComboWindow·StartRecovery·SpawnMinion·CommandMinion·SendGameplayEvent·FinisherDamage), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지(SlowTime·WaitMoving·RotateToTarget), `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 필터·소터·프리뷰·락온 지점, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/TargetData/WxAbilityTargetData_Direction.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatModule.cpp`
- **미검토 / 한계**:
  - 규칙 스캔(187파일 기계 검사): 첫 줄 Copyright 누락 0건, `FORCEINLINE`·`inline`·람다 0건, 헤더 안 함수 본문 0건(GAS 표준 `ATTRIBUTE_ACCESSORS` 매크로 전개 제외), 타입 prefix `Wx` 누락 0건(`WxTargetingPreview`는 타입이 아닌 네임스페이스). Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include` 모두 `WxCore`뿐이다.
  - 멀티플레이 실측이 없다. 1·2·3·4·6·9번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이며 PIE 재현은 하지 않았다.
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열 검색은 보조 근거로만 썼다(`OnGiven` 저장값 없음, 돌진·투사체 노티파이 배치). 2·3·4·5·8번의 실제 발현 폭은 저작에 달려 있다.
  - 직전 리뷰(`28fe02f12`) 대비 해소 확인: 1번(무반응 히트의 HitReact 발동)은 `ShouldAbilityRespondToEvent`·`CanActivateAbility` 게이트(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:41-54`)가 엔진의 재발동 종료·`PreActivate`보다 먼저 돈다는 것을 엔진 `AbilitySystemComponent_Abilities.cpp:1800-1852`로 확인해 뺐다. 8번(투사체 적중의 컨텍스트 어빌리티)은 투사체면 어빌리티를 비우도록 바뀌어(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:50-69`) 뺐고, 9번(Niagara 선언 누락)은 `Plugins/WxCombat/WxCombat.uplugin:36-39`로 해소돼 뺐다. 나머지(컷신 배속·돌진 일시정지·사망 AnimatingAbility·클라 GE 제거·`OnGiven`·스킬 쿨다운 기본값)는 현재 코드로 재검증해 유효한 것만 옮겼다.
  - 올리지 않은 항목: 투사체 적중이 항상 독립 지급이라 한 발동에 여러 발을 쏘는 플레이어 투사체 스킬이 생기면 패시브가 발마다 지급되지만, 사용자 승인 설계(`.codex/worklog/2026-09-15-ApplyDamage-입력-자동화.md`)이고 현재 `WxAnimNotify_SpawnProjectile`은 적 패턴 몽타주(`AM_Template_Pattern_2`)에만 있다. 반사 투사체가 쏜 쪽 발사 레벨을 유지하는 것은 헤더(`Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxProjectileBase.h:36`)에 명시된 의도다. `WxProjectileBase`의 `friend class UWxProjectileSubsystem`은 생성자↔생성물 관례 1건 수준이다. 콤보 창의 태그 요건 전체 면제, Attack·Skill·Pattern 콤보 코드 중복, `WxAnimNotifyState_CameraMove`의 적 몽타주 로컬 뷰 전환, 히트스톱·타격 큐의 비예측 방침, `WxAnimNotify_AreaDamage`의 비권위 타겟팅 실행, `WxAnimNotifyState_ApplyGameplayEffect` 종료의 클래스 단위 스택 제거는 직전 리뷰와 같은 근거(결정 기록·주석 명시·결과 폐기)로 올리지 않았다.

---
*문서 기준 커밋 `993d2a031` · 리뷰일 2026-09-15 · 소스 187파일 — `/module-review`로 갱신*
