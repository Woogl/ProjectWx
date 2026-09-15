# WxCombat — 코드 리뷰

> 직전 리뷰의 심각 3건(그로기 GP 0 종료, 히트스톱 서버 사본 배율, 컷신 1000배속)은 모두 해소됐다. 어빌리티 베이스·대미지 파이프라인·입력 버퍼 같은 전투 코어도 견고하다. 남은 위험은 새로 들어온 GameState 공용 컷신 컴포넌트에 몰려 있다. 종료가 로컬 시퀀스 완료 한 경로에만 기대는 점과, 전역 배율을 저장값으로 되돌리는 점이다.
> 이번 리뷰는 직전 리뷰(`e0106372a`) 이후 바뀐 소스 11파일(신규 컷신 컴포넌트 포함)을 전부 읽고 설치된 UE 5.8 엔진 소스와 대조했다. 코어 어빌리티·이펙트·타겟팅·무기·소환 파일을 다시 읽어 직전 발견을 재검증했고, 189파일 규칙 스캔을 돌렸다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 컷신 시계가 시퀀스 종료 임계에 닿지 못하면 Playing 단계를 빠져나올 길이 없어, 스탠드얼론·리슨 서버에서는 월드가 0.001배로 고정된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:88-95`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:205-209`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:438`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:364-374`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:381-393`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:482-492`
- **범주**: 버그/정확성
- **문제**:
  - 로컬 재생은 엔진 플레이어의 `OnFinished`로만 끝난다(482-492행). 스탠드얼론·리슨 서버에서는 이 콜백이 곧 서버 `Finish`다. 시간 초과는 준비 단계에만 있고(381-393행), 시퀀스 길이 기반 종료는 전용 서버에만 있다(364-374행).
  - 시계는 `Player->GetStartTime()`(438행)에 경과 시간을 더해 돌려준다. 경과 시간은 서버가 잰 초 단위 길이로 클램프한다(205-209행). 엔진의 `GetStartTime()`은 재생 범위 시작을 표시 프레임으로 내림한 값이다(엔진 `Engine/Source/Runtime/MovieScene/Private/MovieSceneSequencePlayer.cpp:987-990`, `Engine/Source/Runtime/MovieScene/Public/MovieSceneSequencePlayer.h:267`). 종료는 위치가 `시작 프레임 + 길이 프레임 + 서브프레임` 이상일 때만 판정한다(`MovieSceneSequencePlayer.cpp:825`). 이 임계는 원래 범위 끝 E와 같다.
  - 반면 시계 상한은 `내림한 시작 + (E - 원래 시작)`, 즉 `E - 시작의 소수부`다. 재생 범위 시작이 표시 프레임 경계에 있지 않으면 시계가 임계에 영영 닿지 않는다. 예를 들어 24fps에서 시작을 10프레임으로 잡은 뒤 표시 속도를 30fps로 바꾸면 시작이 12.5프레임이 된다.
  - 이때 `LocalPlayback`가 Playing에 머문다. 스탠드얼론·리슨에서는 `IsBusy()`가 계속 참이고 전역 배율 0.001이 복원되지 않는다. 전용 서버 구성에서는 클라이언트가 시네마틱 모드와 컷신 카메라에 갇힌다. 뒤따르는 세션도 전부 이 세션 뒤에 줄을 선다. 소유 클라의 궁극기는 서버 정상 종료를 보류한 채(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:96-100`) 끝나지 않아 배타 액션이 계속 막힌다.
  - 현재 `LS_Template_Ultimate`·`LS_HGTest_Ultimate`는 uasset 바이너리 휴리스틱상 범위 하한이 0으로 보여 드러나지 않는다(잠복).
- **제안**:
  - 시계 상한을 서버의 초 단위 길이 대신 플레이어 자신의 범위로 잡거나 클램프를 없앤다. 엔진은 끝에서 위치를 스스로 클램프한다(`MovieSceneSequencePlayer.cpp:1352`).
  - 권위 쪽의 길이 기반 종료(364-374행)를 넷 모드와 무관한 안전망으로 둔다. 그러면 로컬 재생이 어떤 이유로 끝나지 않아도 월드가 고정되지 않는다.
- **확신도**: 중간(엔진 판정식은 소스로 확인했다. 현 에셋의 범위는 바이너리 휴리스틱이다)

### 2. 🟡 컷신 종료가 시작 때 저장한 전역 배율을 무조건 되돌려, 컷신 도중 끊긴 슬로우 타임의 배율을 되살린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:471-476`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:364-369`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:535-539`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp:35-43`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:214-221`
- **범주**: 버그/정확성
- **문제**:
  - 전역 배율을 쓰는 주체가 둘이다. 슬로우 타임 태스크는 현재 값이 자기가 건 값일 때만 1로 되돌린다(35-43행). 컷신은 시작 때 현재 값을 저장했다가 끝에 그 값을 그대로 쓴다(535-539행). 교체 전 컷신 태스크(`e0106372a`의 `ClearTimeDilation`)는 슬로우 타임과 같은 규칙이었다.
  - 실패 순서는 이렇다. 플레이어 B가 극한 회피로 월드를 0.2배로 만든다(`AM_Shared_DodgeSuccess`에 슬로우 타임과 `StartRecovery` 노티파이가 함께 있다). 그 동안 A가 궁극기를 쓰면 컷신이 0.2를 저장하고 0.001을 건다.
  - 시네마틱 모드는 이동·시선 입력만 막는다(엔진 `Engine/Source/Runtime/LevelSequence/Private/LevelSequencePlayer.cpp:395`). 그래서 B는 컷신 중 공격을 누를 수 있고, 그 발동이 후딜의 회피를 끊는다(`WxAbilityBase.cpp:214-221`). 서버의 슬로우 타임 태스크는 현재 값 0.001이 자기 값과 달라 복원을 건너뛴다.
  - 컷신이 끝나며 0.2를 되돌리면 이를 1로 돌릴 주체가 없다. 월드가 모든 플레이어에게 계속 0.2배로 흐른다.
  - 끊기지 않은 슬로우 타임은 월드 시간 기준이라(`WxAbilityTask_SlowTime.cpp:29-32`) 컷신 뒤에 스스로 풀린다. 스탠드얼론·리슨의 시전자 자신은 발동 시 후딜 취소와 `StopAnimMontage`가 저장보다 먼저 와 해당하지 않는다. 결국 문제는 멀티에서 남의 슬로우 타임이 컷신 도중 끊기는 경우다.
- **제안**: 컷신도 슬로우 타임과 같은 규칙을 따른다. 실제로 박힌 값을 기억해 두고, 종료 때 현재 값이 그대로일 때만 1로 되돌린다(저장값 복원 제거). 배율 요청 주체가 더 늘면 전역 배율을 한 곳에서 우선순위로 합성하게 모은다.
- **확신도**: 중간(코드 경로는 확인했다. 멀티 실측은 없다)

### 3. 🟡 돌진 modifier와 그로기가 사유를 세지 않는 BT 일시정지를 나눠 쥐어, 돌진 중 그로기에 빠진 소환물이 그로기 도중 두뇌를 재개한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:65-72`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:179-183`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:64`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp:172-190`
- **범주**: 설계/구조
- **문제**:
  - `UBehaviorTreeComponent::PauseLogic`/`ResumeLogic`은 bool 하나를 세우고 내릴 뿐 사유를 세지 않는다(엔진 `Engine/Source/Runtime/AIModule/Private/BehaviorTree/BehaviorTreeComponent.cpp:173-176`, `:187-194`). 돌진은 자기가 멈춘 두뇌를 기억했다가, 해제 때 `IsPaused()`면 무조건 재개한다(179-183행).
  - 돌진 중 GP가 차서 그로기가 뜨면, 그로기의 `Ability` 취소 지목(`WxAbility_Groggy.cpp:31`)이 돌진 스킬을 끝내 몽타주를 블렌드아웃시킨다. 그로기가 두뇌를 멈추려 해도(64행) 이미 멈춰 있어 아무 일도 없다.
  - 블렌드아웃이 끝나 몽타주 인스턴스가 종료되면, 엔진이 분기점 노티파이 끝을 `bReachedEnd=false`로 부른다(엔진 `Engine/Source/Runtime/Engine/Private/Animation/AnimMontage.cpp:1759-1768`). 이 호출이 `UWxAnimNotifyState_Rush::BranchingPointNotifyEnd`(`Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:23-30`) → `CancelRush` → `ReleaseState`로 이어져, 그로기가 기대던 정지까지 푼다.
  - 그 뒤 그로기 창 내내 BT가 이동·회전·발동 요청을 다시 돌린다. 콘텐츠 문자열 검색상 돌진 노티파이는 `AM_Minion_Skill_2`에 있고, `BP_Minion`은 그로기를 담은 `ABS_Shared_Enemy`를 받는다.
- **제안**: 재개 여부를 기억한 포인터가 아니라 현재 상태로 판정한다. 돌진 해제는 소유자 ASC에 `Ability.Groggy`·`Ability.Death`가 있으면 재개하지 않고, 그로기 종료(`WxAbility_Groggy.cpp:75`)가 재개를 맡는다. 두 곳이 "지금 멈춰 있어야 하는가" 판정 하나를 공유하면 일시정지 주체가 늘어도 같은 충돌이 생기지 않는다.
- **확신도**: 중간(엔진 경로는 소스로 확인했다. 발현은 돌진 몽타주 블렌드아웃과 소환물 MaxGP 저작에 달렸다)

### 4. 🟢 관전 머신은 궁극기 시퀀스를 미리 로드하지 않아, 서버가 월드를 재개한 뒤에도 로드 시간만큼 시네마틱 입력 잠금이 이어진다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:26-34`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:403-410`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:418-421`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:533-551`
- **범주**: 설계/구조
- **문제**:
  - 선로드는 `OnGiveAbility`에서만 한다. 어빌리티 스펙은 소유 클라에만 복제되므로(GAS `Private/GameplayAbilityTypes.cpp:295`) 서버와 소유 클라만 시퀀스를 미리 들고 있다. 다른 클라는 시작 RPC를 받고서야 비동기 로드를 건다(403-410행).
  - 각 머신이 로드 후 처음부터 재생하고, 서버가 자기 기준으로 배율·무적을 복원하는 것은 승인된 방침이다(`.codex/worklog/2026-09-16-컷신-개별-재생.md`). 다만 관전 클라가 늦는 폭은 RTT/2가 아니라 RTT/2에 로드 시간을 더한 값이 된다.
  - 이 꼬리 동안 관전 머신의 로컬 컨트롤러는 이동·시선 입력이 막혀 있다(418-421행, 엔진 `LevelSequencePlayer.cpp:381-399`). 그 사이 서버 월드는 이미 1배로 돌아 적이 움직이고, 관전자에게는 무적도 없다.
  - 에디터 PIE는 에셋이 메모리에 있어 로드 시간이 0이라 드러나지 않는다.
- **제안**: 방침과 충돌하지 않는 쪽은 관전 머신의 로드 시간을 없애는 것이다. 모든 머신에 존재하는 캐릭터 BP나 GameState에 시퀀스 소프트 참조를 두고 미리 로드한다. 쿡 빌드 멀티에서 꼬리 길이를 한 번 재고 판단한다.
- **확신도**: 낮음(의도된 트레이드오프의 연장일 수 있다. 쿡 빌드 로드 시간은 측정하지 않았다)

### 5. 🟢 비권위 머신의 종료 경로가 GE 제거를 호출해 매번 경고만 남긴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:225-232`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:242-250`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:105-118`
- **범주**: 버그/정확성
- **문제**:
  - UE 5.8은 `AbilitySystem.Fix.AllowPredictiveGEFlags` 기본값이 0이다(GAS `Private/AbilitySystemPrivate.h:21`, 프로젝트 ini 재정의 없음). 그래서 비권위 머신의 `RemoveActiveGameplayEffect`는 핸들과 무관하게 Warning을 찍고 false를 돌려준다(GAS `Private/AbilitySystemComponent.cpp:1249-1260`).
  - `ActivationOwnedEffectHandles`는 적용에 실패한 빈 핸들까지 담는다(230행, GAS `Private/Abilities/GameplayAbility.cpp:2041-2054`). 종료 때는 머신 구분 없이 제거를 부른다.
  - 결과적으로 소유 클라에서 궁극기(슈퍼아머)와 처형(무적, ServerInitiated라 핸들이 늘 비어 있음)이 끝날 때마다 경고가 난다. 질주의 속도·드레인 제거도 같다.
  - 실제 정리는 예측 키 확인과 서버 복제가 맡으므로 동작은 맞다. 다만 242행 주석("효과가 새지 않는다")은 클라 제거가 동작하는 것처럼 읽힌다.
  - 직전 리뷰에서 함께 짚은 컷신 태스크의 권위 우회 제거는, 새 컴포넌트가 서버에서 핸들로만 걷도록 바뀌어(`Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:540-543`) 해소됐다.
- **제안**: 제거 호출은 권위에서만 하고, 비권위에서는 핸들만 비운다(빈 핸들은 애초에 담지 않는다). 242행 주석은 "예측본은 키 확인이, 서버본은 복제가 걷는다"로 좁힌다.
- **확신도**: 높음

### 6. 🟢 무기 판정이 소유 클라·시뮬 프록시에서도 콜리전과 매 틱 스윕을 켜지만, 그 결과를 쓰는 곳이 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:50-78`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:140-189`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_WeaponAttack.cpp:12-26`
- **범주**: 성능/안전
- **문제**:
  - 공격 노티파이는 몽타주가 도는 모든 머신에서 `BeginAttack`을 부른다. `BeginAttack`은 권위를 가리지 않고 형상 콜리전을 켜고 액터 틱을 연다(64-75행). 이후 매 틱 형상별 `SweepMultiByChannel`(180행)과 오버랩 이벤트가 `ProcessHit`로 모인다.
  - 비권위 머신에서는 `ApplyDamage`가 권위 검사에서 곧바로 false를 돌려주고(`Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp:45`), 히트스톱도 권위에서만 건다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp:26`).
  - 즉 클라의 스윕·오버랩은 소비처 없는 비용이다. 관련성 범위 안에서 동시에 공격하는 적 수만큼 모든 클라에 곱해진다.
- **제안**: `BeginAttack` 첫머리에서 소유 액터가 권위가 아니면 반환한다. 그러면 `ActiveAttackCount`가 0으로 남아, `EndAttack`과 사망 시 `CancelAttack`(`Source/WxGame/Character/WxCharacterBase.cpp:264`)은 그대로 아무 일도 하지 않는다.
- **확신도**: 중간(소비처가 없다는 것은 코드로 확인했다. 실제 비용은 동시 교전 규모에 달렸다)

### 7. 🟢 `UWxAbility_Skill`의 기본 쿨다운 GE가 슬롯 1이라, 새 슬롯 BP가 갈아 끼우지 않으면 쿨다운이 조용히 공유된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`
- **범주**: 설계/구조
- **문제**:
  - 베이스는 "공용 기본값을 두면 어빌리티끼리 쿨다운이 섞인다"며 쿨다운 GE 기본값을 일부러 비운다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:21-22`). 그런데 `UWxAbility_Skill`만 `UWxEffect_Cooldown_Skill_1`을 깔아 둔다.
  - `OnGiveAbility` 진단(`WxAbilityBase.cpp:304-309`)은 "쿨다운 태그 없음"만 잡으므로 이 누락을 통과시킨다.
  - 현재 `GA_HGTest_Skill_2`·`GA_Minion_Skill_2`는 `CooldownGameplayEffectClass`를 재지정해 두어(uasset 문자열 검색) 드러나지 않는다. 새로 만드는 슬롯 BP가 이 값을 놓치면 `Cooldown.Skill.1` 하나로 서로 막히는 잠복 함정이다.
- **제안**: 21행 대입을 걷어 베이스와 같은 규칙으로 되돌리면 기존 Error 진단이 누락을 잡는다. 이때 기본값에 기대는 슬롯 1 BP(`GA_Template_Skill`·`GA_HGTest_Skill_1`·`GA_Minion_Skill_1`, 재지정 문자열 없음)에는 `UWxEffect_Cooldown_Skill_1`을 명시 지정해야 쿨다운이 사라지지 않는다.
- **확신도**: 중간(발현은 이후 슬롯 BP 저작에 달렸다)

### 8. 🟢 호출자 없는 선언과 도달하지 않는 분기
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:19-26`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:311-317`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h:49`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp:52-55`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:28-31`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:175`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:475`
- **범주**: 중복/복잡도
- **문제**:
  - `EWxAbilityActivationPolicy::OnGiven`은 C++ 어디서도 고르지 않고, 콘텐츠 uasset 문자열 검색에도 저장된 값이 없다. 그런데 `OnGiveAbility`는 스펙 복제가 도착한 소유 클라에서도 불리므로(GAS `Private/GameplayAbilityTypes.cpp:295`), 현재 구현은 서버와 소유 클라가 각자 `TryActivateAbility`를 부른다. 권위 게이트 없는 미사용 선택지가 처음 쓰는 사람에게 함정으로 남아 있다.
  - `UWxAbility_Finisher::IsBackstab()`은 저장소 전체에 호출자가 없다. `bBackstab`도 `ActivateAbility` 안에서 변형을 고르는 데만 쓰여(`WxAbility_Finisher.cpp:69-70`) 멤버로 들고 있을 이유가 없다.
  - 컷신 태스크 서버 분기의 `if (!IsBusy()) Reserve(...)`(28-31행)는 타지 않는다. 유일한 생성자인 궁극기가 권위에서 이미 예약한 뒤 태스크를 만들기 때문이다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp:57-66`, `CreateTask`는 UFUNCTION이 아니다).
  - 클라 분기의 `IsForAvatar(...) ? 0 : 1`(24행)도 0 쪽에 닿지 않는다. `IsForAvatar`의 조건(`Phase != Idle`)은 `IsBusy()`에 포함되고, 궁극기 `CanActivateAbility`가 그때 발동을 막는다(`WxAbility_Ultimate.cpp:38-42`). 이 분기가 `IsForAvatar`의 유일한 사용처다.
  - `Start`의 준비 마감 대입(`WxSkillCutsceneComponent.cpp:175`)은 로컬 세션 진입(351행)이 곧바로 덮는다. 리슨 서버의 `ServerExecution.StartTime` 대입(475행)은 전용 서버 분기(364·370행)에서만 읽힌다.
- **제안**: 모두 걷는다. `OnGiven`을 남긴다면 권위에서만 발동하도록 게이트를 둔다.
- **확신도**: 높음(미사용·도달 불가), 중간(`OnGiven` 경합 양상)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Public/Cutscene/WxSkillCutsceneComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Ultimate.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Groggy.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxHitStopComponent.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_SlowTime.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Dodge.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbility_Finisher.h`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 의 Attack·Skill·Pattern·Passive·PlayMontageOnce·Death·HitReact·Guard·GuardReact, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 의 Cooldown·Cost·DrainGP·AddAttribute·EffectComponent_Table·EffectComponent_DamageResponse·Exhaust·HitStop·Invincible·SuperArmor·Exceed·RegenSP·DrainSP, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 의 Hit·DamageFloater·Exceed·AttackTelegraph·GhostTrail, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 의 ApplyGameplayEffect·AreaDamage·SpawnProjectile·SnapToTarget·WeaponAttack·SpawnMinion·CommandMinion·SendGameplayEvent·ComboWindow·StartRecovery·FinisherDamage·CameraMove, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 의 필터·소터 태스크·`WxLockOnComponent.cpp`·`WxLockOnPointComponent.cpp`·`WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxHitEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_WaitMoving.cpp`·`WxAbilityTask_RotateToTarget.cpp`, 교차 확인용 `Source/WxGame/Framework/WxGameState.cpp`·`Source/WxGame/Character/WxCharacterBase.cpp`
- **미검토 / 한계**:
  - 수치만 넣는 소형 이펙트(FullHP·GuardReduction·HealPercent·Hit·InfiniteMP·MoveSpeedScale·NoCooldown·PerfectGuard·ResetGP), `WxCueNotify_PerfectGuard`, `WxTargetingPreview.cpp`, 대부분의 Public 헤더는 통독하지 않았다.
  - 멀티플레이·쿡 빌드 실측이 없다. 1·2·3·4·5번은 설치 엔진(`C:\Program Files\Epic Games\UE_5.8\Engine`) 소스 경로로 확인한 결론이다. 문서의 엔진 경로는 이 엔진 루트 기준이고, `GAS/`는 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/`를 줄여 쓴 것이다. 런타임 `DuplicateObject`로 복제한 LevelSequence가 쿡 빌드에서 재컴파일돼 도는지도 확인하지 않았다(에디터 전용 가드가 없는 컴파일 경로만 확인).
  - BP·DataTable·LevelSequence 저작 값은 범위 밖이다. 콘텐츠 `.uasset` 문자열·바이너리 검색은 보조 근거로만 썼다(슬로우 타임·후딜 노티파이 배치, 시퀀스 범위 하한, 돌진 노티파이·그로기 배치, 슬롯 BP 쿨다운 재지정, `OnGiven` 저장값 없음).
  - 규칙 스캔(189파일 기계 검사): 첫 줄 Copyright 누락 0건, `FORCEINLINE`·`inline`·람다 0건. Wx 참조는 `WxCombat.Build.cs`·`WxCombat.uplugin`·`#include`(`WxGameplayTags.h`·`WxUIData.h`·`WxCollisionChannels.h`·`Minion/WxMinion.h`) 모두 `WxCore`뿐이다. `BlueprintCallable`은 함수 라이브러리(`WxCombatLibrary.h:33`) 한 곳이다.
  - 직전 리뷰(`e0106372a`) 대비 해소 확인:
    - **1번(그로기 종료)**: fa9d6de86이 GP 변경 구독을 되살렸다(`WxAbility_Groggy.cpp:94-100`, `:144-145`). 처형의 `UWxEffect_ResetGP`가 다시 그로기를 푼다. 관련 주석(`WxCombatAttributeSet.cpp:126`, `WxEffect_ResetGP.h:9`)도 실제 흐름과 맞다.
    - **2번(히트스톱 서버 사본)**: be11d136c가 캐릭터 메시의 `bOnlyAllowAutonomousTickPose`로 서버 사본을 제외했다(`WxHitStopComponent.cpp:64-66`).
    - **3번(컷신 1000배속)**: 컷신이 GameState 컴포넌트로 재설계되며 월드 배율과 무관한 플랫폼 시계로 재생한다(`WxSkillCutsceneComponent.cpp:69-95`). 역수 보정 자체가 사라졌다.
    - **5번(그로기 몽타주 자동 블렌드아웃 창)**: `AM_Shared_Groggy`가 섹션 자기 루프로 저작돼 창이 생기지 않는다는 기록(사용자 메모리 `groggy-duration-from-montage`)에 따라 올리지 않았다. 섹션 루프 없이 저작하면 창이 다시 생긴다.
    - **6번(비권위 GE 제거)**: 컷신 부분만 해소돼 이번 5번으로 좁혔다.
  - 올리지 않은 항목:
    - 무적 ANS 종료의 클래스 단위 스택 제거(`WxAnimNotifyState_ApplyGameplayEffect.cpp:35`)가 시전 직후 컷신 무적 GE까지 걷을 수 있다. 다만 그 창은 월드가 0.001배로 멈춘 동안이라 실효가 거의 없다.
    - 새 컷신 코드의 cpp 정의 순서가 헤더 선언 순서와 크게 다르고 분기용 멤버 플래그(`bLocalPresentationPending`)가 있다. 이는 `CLAUDE.md`가 아닌 사용자 메모리 규칙이라 발견에서 뺐다.
    - 직전 리뷰의 "올리지 않은 항목"은 같은 근거로 유지한다. 콤보 창의 태그 요건 면제, Attack·Skill·Pattern 콤보 코드 중복, `WxAnimNotifyState_CameraMove`의 적 몽타주 로컬 뷰 전환, 히트스톱·타격 큐의 비예측 방침, `WxAnimNotify_AreaDamage`의 비권위 타겟팅 실행, `AWxProjectileBase::Reflect` 역참조, 사망 해제 대기 오차가 해당한다.

---
*문서 기준 커밋 `4096004a4` · 리뷰일 2026-09-16 · 소스 189파일 — `/module-review`로 갱신*
