# WxCombat — 코드 리뷰

> GAS 위에 얹은 도메인치고 드물게 정돈된 모듈이다 — 프로젝트 코딩 규칙(prefix·Copyright·`Handle` 콜백·인라인 금지·`BlueprintCallable` 제한) 위반이 하나도 없고, 리플리케이션 권위와 예측 경계도 대체로 코드와 주석이 일치한다. 이번 리뷰는 `WxCombat.Build.cs`/`.uplugin`, ASC·어빌리티 베이스·어트리뷰트셋·대미지 ExecCalc·무기/투사체·락온·시간 조작 등 위험도가 높은 cpp를 깊게 보고, 이펙트·큐·타게팅 필터·애님 노티파이는 전수 훑는 방식으로 진행했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `GetCooldownGameplayEffect()`가 어빌리티 CDO에 런타임 UObject를 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:285`
- **범주**: 설계/구조
- **문제**: `const` 게터가 `NewObject<UWxEffect_Cooldown>(const_cast<UWxAbilityBase*>(this), TEXT("CooldownEffect"))`로 `this`를 Outer 삼아 GE를 만든다. 이 함수의 유일한 외부 호출자인 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:21`은 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_AbilitySystem.cpp:128`에서 `Spec.Ability`(= 어빌리티 **CDO**)를 넘겨 부르므로, UI가 열리는 순간 CDO에 이름이 고정된(`"CooldownEffect"`) 트랜지언트 서브오브젝트가 생긴다. `CooldownEffect`는 `Instanced`가 아닌 평범한 `TObjectPtr`(`WxAbilityBase.h:149`)이라, 그 뒤에 만들어지는 모든 어빌리티 인스턴스는 CDO에서 이 포인터를 그대로 복사받아 한 객체를 공유하게 된다. 값이 클래스마다 동일해 현재는 증상이 없지만, 런타임에 CDO 상태를 바꾸는 구조라 PIE 반복·핫리로드·쿠킹 가정과 충돌할 여지가 있다.
- **제안**: `StackLimitCount`가 순수히 ViewModel 전달용이라면(주석 `WxAbilityBase.h:145-147`) GE 인스턴스를 만들지 말고, ViewModel이 `AbilityDataRow`의 `MaxRecharges`를 직접 읽게 하거나 어빌리티에 `GetMaxRecharges()` 같은 조회 함수를 두는 편이 안전하다. 인스턴스가 꼭 필요하면 Outer를 CDO가 아닌 ASC/어빌리티 인스턴스로 한정하고, CDO 경로에서는 CDO를 그대로 돌려주도록 갈라야 한다.
- **확신도**: 중간

### 2. 🟡 가드 브레이크 판정이 ExecCalc의 모디파이어 나열 순서에 암묵적으로 묶여 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:296`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:136`
- **범주**: 설계/구조
- **문제**: `bGuardBroken`은 `GetSP() <= 0.f`로 판정하는데, 이 값이 "차감 후"인 것은 오직 `Execute_Implementation`이 `SP` 출력 모디파이어를 `IncomingDamage`보다 앞에 실었기 때문이다(`WxEffect_Damage.cpp:138` vs `:146`). 이 결합은 주석으로만 지켜지고 있어(`WxCombatAttributeSet.cpp:294-295`) 두 `AddOutputModifier` 호출의 위치를 바꾸는 무심한 편집 한 번으로 "가드가 영영 깨지지 않는" 무증상 회귀가 난다. 컴파일·런타임 어느 쪽도 이를 잡아 주지 않는다.
- **제안**: 판정을 순서 의존에서 떼어낸다 — ExecCalc가 이미 `bGuardHit`과 차감량을 알고 있으므로 가드 브레이크 여부를 `FWxCombatEffectContext`(크리 플래그와 같은 통로)나 스펙의 동적 애셋 태그에 실어 보내고, `ProcessDamageTaken`은 그것을 읽기만 하게 한다.
- **확신도**: 중간

### 3. 🟡 컷신 태스크가 `ACharacter::GetMesh()`를 검사 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:88`
- **범주**: 버그/정확성
- **문제**: `InstanceData->TransformOrigin = AvatarCharacter ? AvatarCharacter->GetMesh()->GetComponentTransform() : ...` — `AvatarCharacter` 널 여부만 보고 `GetMesh()` 결과는 그대로 역참조한다. 이 코드베이스 스스로 "ACharacter::Mesh는 Optional 서브오브젝트라 널일 수 있다"고 명시하고 방어하는 곳이 있어(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp:31-37`) 전제가 어긋난다. 메시 없는 캐릭터가 궁극기를 쓰면 크래시다.
- **제안**: `USkeletalMeshComponent* AvatarMesh = AvatarCharacter ? AvatarCharacter->GetMesh() : nullptr;`로 받아 널이면 `AvatarActor->GetActorTransform()` 폴백으로 떨어뜨린다.
- **확신도**: 높음

### 4. 🟡 Attack·Skill·Pattern의 콤보 몽타주 로직이 3중 중복
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: 세 클래스의 `ActivateAbility`(커밋 → `ComboIndex` 전진 → `PlayMontage` → 실패 시 종료)와 `EndAbility`(`bWasCancelled`면 `ComboIndex = INDEX_NONE`)가 글자 단위로 같다. `WxAbility_Attack`과 `WxAbility_Skill`은 `HandleMontageCompleted`까지 동일하고, `WxAbility_Pattern`만 자동 다음 단 재생으로 갈린다. 콤보 인덱스 규칙(재발동 시 유지, 완주 시 리셋)이 미묘한 상태 관리라 세 곳이 어긋나기 쉽다.
- **제안**: `ComboMontages` + `ComboIndex` + 전진/리셋 규칙을 쥔 중간 베이스(예: `UWxAbility_ComboMontageBase`)를 두고, 세 클래스는 태그·활성화 그룹·`HandleMontageCompleted` 정책만 남긴다.
- **확신도**: 높음

### 5. 🟡 태그만 부여하는 GE 4종의 생성자가 완전 중복
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Guard.cpp:8`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Invincible.cpp:9`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_SuperArmor.cpp:8`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_PerfectGuard.cpp:9`
- **범주**: 중복/복잡도
- **문제**: 네 생성자가 `DurationPolicy = Infinite` + `UTargetTagsGameplayEffectComponent` + `UAssetTagsGameplayEffectComponent`를 같은 형태로 반복하고 태그 하나만 다르다. 서브오브젝트 이름(`"TargetTags"`, `"AssetTags"`)까지 같아 실질적으로 복붙 4벌이며, 이 패턴이 늘어날수록 규약(두 컴포넌트를 항상 짝으로 붙여야 태그가 Owned/Asset 양쪽에 잡힌다) 누락 위험이 커진다.
- **제안**: 태그 하나를 받아 두 컴포넌트를 구성하는 공용 베이스(예: `UWxEffect_TagOnly`)를 만들고 파생은 생성자에서 태그만 지정하게 한다.
- **확신도**: 높음

### 6. 🟢 락온 태스크만 생성 결과를 검사하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp:84`
- **범주**: 성능/안전
- **문제**: `LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(...)` 직후 널 검사 없이 델리게이트를 바인딩한다. 같은 모듈의 다른 태스크 생성부는 모두 `if (Task)`로 감싸고 있어(`WxAbility_Dodge.cpp:205`, `WxAbility_GuardReact.cpp:94`, `WxAbility_Sprint.cpp:81`, `WxAbility_Ultimate.cpp:52`) 관례가 어긋난다. `NewAbilityTask`가 실제로 널을 내는 경우는 사실상 없어 즉시 터질 코드는 아니다.
- **제안**: 다른 호출부와 동일하게 널 가드를 씌운다.
- **확신도**: 높음

### 7. 🟢 커스텀 EffectContext의 NetSerialize가 부모의 실패를 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp:26`
- **범주**: 버그/정확성
- **문제**: `FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess)`의 결과를 무시하고 마지막에 `bOutSuccess = true;`를 무조건 대입한다. 부모가 인스티게이터·이펙트 코저 등 오브젝트 레퍼런스 매핑에 실패해 `false`를 냈어도 호출자에게는 성공으로 보고되어, 컨텍스트가 부분적으로 비어 있는 채로 큐 파라미터에 실려 나간다.
- **제안**: 부모 반환값/`bOutSuccess`를 보존하고, 자체 필드 직렬화 실패만 추가로 반영한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Time/WxTimeDilationComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`
- **훑은 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/` 나머지 전부(Attack·Skill·Pattern·Death·Passive·Guard·GuardReact·HitReact·Finisher·Sprint·Ultimate), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전부, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전부, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 전부, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_*.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemGlobals.cpp`, 대응 `Public/` 헤더 전반, `Config/DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록 확인
- **미검토 / 한계**:
  - 규칙 준수 여부(prefix·Copyright 첫 줄·`Handle` 콜백 prefix·`FORCEINLINE`/인라인 정의·람다·`BlueprintCallable`)는 모듈 전수 grep으로 확인했고 위반이 0건이었다. 모듈 의존성도 `WxCombat.Build.cs`가 `WxCore` 외 Wx 플러그인을 참조하지 않아 규칙에 맞는다.
  - 입력 버퍼/발동 그룹 전이(`AbilityInputActionTriggered` → `FlushBufferedInputs` → `OpenComboWindow`/`StartRecovery`)는 애님 노티파이 실행 중에 어빌리티 목록을 건드리는 재진입 경로다. 정적 읽기로는 `ABILITYLIST_SCOPE_LOCK` 적용이 타당해 보이나, 실제 재진입 안전성은 런타임 검증이 필요해 판단을 보류했다.
  - `UWxExecCalc_Damage`의 밸런스 공식(방어 계수 `100/(100+DEF)`, 크리 배율)과 데이터테이블 실제 수치의 타당성은 기획 영역이라 보지 않았다.
  - BP/WBP 자산 내부 구조(몽타주 노티파이 배치, 어빌리티 BP 파생의 오버라이드 여부)는 이 리뷰 범위 밖이다 — 발동 그룹·콤보 창 규약이 실제 몽타주에 제대로 깔렸는지는 확인하지 못했다.
  - **철회된 지적(2026-08-28)**: 이전 판에 `UWxAbility_Groggy`의 타이머 해제가 아바타 파괴 시 누락된다는 항목이 있었으나 사실이 아니다. ASC는 캐릭터의 서브오브젝트라 소유자와 아바타가 같은 액터이고(PlayerState 소유 구성이 아니다), 액터 파괴는 컴포넌트 해제 → 가비지 표시 순서라 ASC가 어빌리티를 취소하는 시점의 아바타 약참조는 아직 유효하다. 남은 실익이 표현 중복뿐이어서 해당 코드는 어빌리티 자신의 `GetWorld()`를 쓰도록 정리했고 항목은 삭제했다. 같은 지적을 다시 올리지 말 것.

---
*문서 기준 커밋 `49cc6a81` · 리뷰일 2026-08-27 · 소스 150파일 — `/module-review`로 갱신*
