# WxCombat — 코드 리뷰

> GAS 위에 올린 전투 도메인으로, 발동 배타 그룹·대미지 단일 진입점·판정/연출 분리 같은 굵은 계약이 코드와 주석에 일관되게 지켜지고 있어 전반적으로 건강하다. 다만 서버 권위 경로에서 나온 결과가 소유 클라이언트까지 닿지 않는 지점이 몇 군데 남아 있다. 이번 리뷰는 `*.Build.cs`·`*.uplugin`과 전 헤더를 훑고, ASC·어빌리티 베이스와 파생 전부·어트리뷰트셋·ExecCalc·무기/투사체·락온·타임딜레이션·어빌리티 태스크·애님 노티파이·타게팅 필터 cpp를 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 히트스톱 복원이 몽타주 복제 경로를 우회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:214`, `:329`
- **범주**: 설계/구조
- **문제**: 얼릴 때는 `CurrentMontageSetPlayRate(0.001f)`를 쓴다. 이 엔진 함수는 `AnimInstance`뿐 아니라 `RepAnimMontageInfo.PlayRate`까지 갱신해 시뮬레이션 프록시에 복제한다. 반면 복원(`HandleHitStopElapsed`)은 `AnimInstance->Montage_SetPlayRate(HitStopMontage, ...)`를 직접 불러 복제 정보를 손대지 않는다. 서버에서 히트스톱이 걸리면 다른 클라이언트는 PlayRate 0.001만 받고 복원값은 영영 못 받아, 다음 몽타주 재생이 `RepAnimMontageInfo`를 덮어쓸 때까지 그 공격자가 정지한 채로 보인다. 로컬 복원은 정상이므로 공격자 본인과 서버 화면에서는 드러나지 않는다.
- **제안**: 복원 시 `HitStopMontage`가 여전히 `GetCurrentMontage()`면 `CurrentMontageSetPlayRate(HitStopResumePlayRate)`를 쓰고, 주인이 바뀐 경우에만 지금처럼 직접 복원한다.
- **확신도**: 중간

### 2. 🟡 가드 페이즈 전환이 서버 로컬 이벤트에만 의존해 소유 클라이언트에 반영되지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp:106`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:298`, `:351`
- **범주**: 설계/구조
- **문제**: 대미지 GE는 Instant+Execution이라 클라 예측에서 execution이 스킵되므로(모듈 스스로 `Public/WxCombatLibrary.h:38`에 명시) `ProcessDamageTaken`·`ProcessPerfectGuard`는 서버에서만 돈다. 거기서 나가는 `ASC->HandleGameplayEvent(Event.Hit)`·`SendGameplayEventToActor(Event.PerfectGuard)`는 복제되지 않는 로컬 발행이고, 이를 받는 `UWxAbility_Guard`의 `UAbilityTask_WaitGameplayEvent`는 서버 인스턴스에서만 발화한다. 결과적으로 GuardHitReact·GuardKnockback·GuardBreak·PerfectGuard 몽타주와 SP 고갈 시의 `RemoveActiveEffectsWithTags` 후속 연출이 서버에서만 재생되는데, 엔진은 locally-controlled 아바타에 `RepAnimMontageInfo`를 적용하지 않으므로 **정작 가드한 플레이어 본인만 그 연출을 못 본다**. Effect.Guard 해제는 GE 복제로 도착하니 클라는 태그 없이 가드 루프만 계속 도는 상태가 된다. 같은 문제를 HitReact·Groggy·Death·Finisher는 `ServerInitiated` 정책으로 풀었지만 Guard는 LocalPredicted라 그 통로가 없다.
- **제안**: 가드 페이즈 전환을 어빌리티 트리거(ServerInitiated 파생 어빌리티)나 복제되는 신호(GE 태그·GameplayCue)로 옮겨 소유 클라에도 같은 전이가 도달하게 한다.
- **확신도**: 중간(스탠드얼론·리슨서버 호스트에서는 드러나지 않아 의도된 유예일 수 있음)

### 3. 🟡 `GetCooldownGameplayEffect()` const 접근자가 CDO에 런타임 서브오브젝트를 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:286`
- **범주**: 설계/구조
- **문제**: 다중 충전 어빌리티에서 `NewObject<UWxEffect_Cooldown>(const_cast<UWxAbilityBase*>(this), TEXT("CooldownEffect"))`로 GE 인스턴스를 만들어 mutable 필드에 캐시하고 `StackLimitCount`만 실어 보낸다. 그런데 이 접근자는 인스턴스가 아니라 **CDO에도 호출된다** — `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:128`이 `AbilityVM->Initialize(ASC, AbilityCDO)`로 CDO를 넘기고 `WxViewModel_Ability.cpp:21`이 그 위에서 부른다. 즉 const 접근자가 클래스 공유 상태(CDO)를 런타임에 변형하고, 그 서브오브젝트는 PIE 세션이 끝나도 CDO에 남는다. 게다가 이 인스턴스는 실제 적용에는 쓰이지 않는다 — `ApplyCooldown`(`:311`)이 `CooldownGE->GetClass()`를 넘겨 스펙을 만들므로 적용되는 Def는 언제나 CDO다. 결국 GE 오브젝트 하나를 "MaxRecharges 전달용 값 상자"로만 쓰는 셈이다.
- **제안**: 최대 충전 수를 `FWxAbilityTableRow::MaxRecharges`를 읽는 부작용 없는 별도 접근자로 노출하고(ViewModel이 그것을 읽게), `GetCooldownGameplayEffect()`는 순수 접근자로 되돌려 `CooldownEffect` 캐시를 없앤다.
- **확신도**: 중간

### 4. 🟡 Attack·Skill·Pattern의 콤보 진행 로직이 3중 복제
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:29`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:31`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:29` (필드는 각각 `Public/AbilitySystem/Ability/WxAbility_Attack.h:34`, `WxAbility_Skill.h:37`, `WxAbility_Pattern.h:30`)
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`/`ComboIndex` 필드와 `ActivateAbility`의 인덱스 전진·몽타주 선택, `EndAbility`의 취소 시 `INDEX_NONE` 리셋이 세 클래스에 그대로 복사돼 있다. Attack과 Skill은 생성자의 태그를 빼면 cpp 본문이 사실상 동일하다. 콤보 규칙(터미널 단 롤오버, 취소 시 초기화 등)을 바꾸면 세 곳을 함께 고쳐야 하고, Pattern만 `HandleMontageCompleted`가 다음 단을 자동 재생하는데 그 차이가 의도인지 누락인지 코드만 봐서는 구분되지 않는다.
- **제안**: 콤보 몽타주 진행을 공통 베이스(예: `UWxAbilityBase` 또는 그 아래 중간 클래스)로 올리고, Pattern의 자동 연속 재생만 `HandleMontageCompleted` 오버라이드로 남긴다.
- **확신도**: 높음

### 5. 🟡 `WxAnimNotifyState_CameraMove::NotifyEnd`가 뷰 타겟을 무조건 폰으로 되돌린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:180`
- **범주**: 버그/정확성
- **문제**: NotifyBegin이 카메라 스폰에 실패했거나(`:51`) 애초에 뷰를 가로채지 않았어도, NotifyEnd는 조건 없이 `PC->SetViewTargetWithBlend(PC->GetPawn(), ...)`을 호출한다. 이 노티파이는 로컬 플레이어 폰뿐 아니라 AI 몽타주에서도 실행되므로(`:28-34` 주석대로 의도된 동작), 다른 연출이 뷰를 쥐고 있는 사이 — 예를 들어 `UWxAbilityTask_PlaySkillCutscene`의 레벨 시퀀스 카메라나 스펙테이트 중 — AI 몽타주의 CameraMove 구간이 끝나면 그 카메라를 폰으로 끊어버린다. 두 CameraMove 구간이 겹칠 때도 먼저 끝난 쪽이 나중 카메라를 걷어낸다.
- **제안**: 자기가 세운 카메라 액터가 여전히 `PC->GetViewTarget()`일 때만 복원한다. 노티파이는 에셋 공유 인스턴스라 재생별 상태를 둘 수 없으므로, 스폰한 임시 카메라 액터 쪽에 복원 책임을 두는 방법이 안전하다.
- **확신도**: 중간

### 6. 🟢 `FWxCombatEffectContext::NetSerialize`가 부모의 실패를 덮는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp:26`, `:37`
- **범주**: 버그/정확성
- **문제**: `FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess)`가 채운 `bOutSuccess`를 확인하지 않고 마지막에 `bOutSuccess = true;`로 무조건 덮는다. 부모가 미매핑 오브젝트 등으로 직렬화에 실패해도 호출자에게는 성공으로 보고된다.
- **제안**: 부모 호출의 `bOutSuccess`를 보존하고 자기 필드 직렬화 결과와 AND로 합친다.
- **확신도**: 높음

### 7. 🟢 `AWxGhostTrail::EndPlay`가 Super만 호출하는 빈 오버라이드
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:46`
- **범주**: 중복/복잡도
- **문제**: 본문이 `Super::EndPlay(EndPlayReason);` 한 줄뿐이라 아무 것도 하지 않는 데드 오버라이드다.
- **제안**: 선언과 정의를 함께 제거한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/Public/` 전 헤더, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Damage/`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`
- **규칙 점검 결과**: `CLAUDE.md`의 코딩·모듈 규칙 위반은 발견되지 않았다. `Wx` prefix, 전 파일 첫 줄 저작권(BOM 뒤 동일 문자열), 델리게이트 콜백 `Handle` prefix(24개 바인딩 전부), 람다 0건, `FORCEINLINE`·인라인 정의 0건, `BlueprintCallable`은 `UWxCombatLibrary::ApplyDamage` 한 곳뿐이며 해당 클래스는 `UBlueprintFunctionLibrary`라 허용 범위다. `WxCombat.Build.cs`의 Wx 의존은 `WxCore` 하나뿐이다.
- **미검토 / 한계**: (1) 리플리케이션 관련 발견 1·2는 정적 분석 결과이며 데디케이티드 서버 실측으로 확인하지 않았다. (2) BP 파생 어빌리티·무기·투사체의 저작 값(콤보 몽타주 구성, 히트박스 형상, TargetingPreset 내용)은 범위 밖이라 데이터 오류로 생기는 문제는 잡지 못한다. (3) `WxUI`·`WxGame` 쪽 호출부는 발견 3의 근거 확인 목적으로만 최소한 열람했고 리뷰하지 않았다. (4) GameplayCue 에셋 등록·쿠킹 경로는 코드 밖이라 확인하지 않았다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 148파일 — `/module-review`로 갱신*
