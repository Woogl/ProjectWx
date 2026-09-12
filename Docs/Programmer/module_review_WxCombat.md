# WxCombat — 코드 리뷰

> GAS 위에 얹은 전투 규칙의 골격(어빌리티 점유·캔슬 창, 데미지 파이프라인, 어트리뷰트, 선입력, 히트스톱)은 권한 경계와 예측 롤백까지 일관되게 잡혀 있고 CLAUDE.md 코딩 규칙 위반은 0건이다. 결함은 대부분 주변부(카메라·컷신 연출, 투사체·무기, 소환·타게팅)에 몰려 있으며, 연출 태스크에 권위 게이트가 빠진 것과 실패 경로를 정상 종료로 처리하는 패턴이 반복된다. 이번 리뷰는 181개 소스 전부를 훑고 어빌리티 기반·ASC·ExecCalc·GE 컴포넌트·AnimNotify·Cue·AbilityTask·타게팅·무기·투사체·소환까지 핵심 cpp를 직접 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 10 |
| 🟢 사소 | 5 |

## 결과

### 1. 🟡 패턴 연쇄 몽타주 재생 실패를 정상 종료로 처리해 다음 발동 인덱스가 남는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:55`
- **범주**: 버그/정확성
- **문제**: `HandleMontageBlendOut`이 55행에서 인덱스를 다음 슬롯으로 올린 뒤 재생에 실패하면 58행에서 `bWasCancelled=false`로 종료한다. `EndAbility`(38~46행)는 취소일 때만 인덱스를 초기화하므로, 중간 슬롯이 비어 있거나 재생에 실패한 구성에서는 다음 발동이 29행에서 그다음 슬롯부터 시작해 앞 단이 통째로 건너뛰어진다. 최초 발동의 재생 실패는 34행에서 취소로 끝내므로 같은 실패가 두 갈래로 처리되는 셈이다.
- **제안**: 연쇄 재생 실패도 `bWasCancelled=true`로 종료해 기존 인덱스 초기화 경로를 태운다.
- **확신도**: 높음

### 2. 🟡 카메라 노티파이가 임시 카메라의 수명과 뷰 소유권을 추적하지 않는다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:69`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp:180`
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹친다. (가) 69행의 `SetLifeSpan(TotalDuration + BlendOutTime + 1.f)`에서 `TotalDuration`은 애니메이션 에셋 기준 초라 재생 속도가 반영되지 않는다. 180행 주석이 밝히듯 카메라 정리는 이 수명이 전담하므로, ASPD가 1 미만인 느린 재생에서는 구간이 끝나기 전에 뷰 타겟이 파괴된다. 같은 폴더의 `WxAnimNotifyState_SlowTime.cpp:62-64`는 같은 함정을 알고 `TotalDuration / PlayRate`로 보정하므로 이 파일만 어긋나 있다. (나) 180행은 생성한 카메라를 어디에도 보관하지 않아 "지금 뷰 타겟이 내 것인지"를 판단하지 못한 채 무조건 폰으로 되돌린다. NotifyState 인스턴스는 애님 에셋 소유라 같은 몽타주를 도는 액터들이 공유하므로, 두 구간이 겹치면 먼저 끝난 쪽이 뒤 구간(또는 그 사이 뷰를 가져간 컷신)의 카메라를 끊는다. 51행에서 스폰에 실패해 조기 반환한 경우에도 복귀는 그대로 실행된다.
- **제안**: 스폰한 카메라와 직전 뷰 타겟을 액터 단위로 기록하고, 자기가 현재 뷰를 쥐고 있을 때만 복구한다. 안전망 수명에도 몽타주 재생 속도를 반영한다.
- **확신도**: 높음

### 3. 🟡 컷신 태스크가 권위 게이트 밖에서 PlayRate를 걸어, 클라이언트는 컷신을 건너뛴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:117`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: 58행의 `IsOwnerActorAuthoritative()` 게이트는 `SetGlobalTimeDilation`만 감싸는데, 117~120행의 `SetPlayRate(1.f / GlobalTimeDilation)`은 게이트 밖이다. `UWxAbility_Ultimate`는 기반 클래스 기본값대로 LocalPredicted라 소유 클라에서도 이 태스크가 돌고(`WxAbility_Ultimate.cpp:54`가 딜레이션 0.001로 생성), 클라에서는 월드 딜레이션이 WorldSettings 복제로 도착하기 전까지 시간이 정상 속도인 채 시퀀스만 PlayRate 1000으로 돌아 컷신이 즉시 끝난다. 부수적으로 119행은 62~63행이 "클램프된 실제값을 써야 한다"며 따로 저장해 둔 `AppliedDilation`이 아니라 요청값을 쓴다 — 프로젝트가 `Min/MaxGlobalTimeDilation`을 조정하면 배속과 딜레이션이 어긋난다(현재 Config에 재정의 없음).
- **제안**: PlayRate 계산을 권위 분기 안으로 넣고 `AppliedDilation`을 쓴다. 비권위 머신에서는 실제 딜레이션이 목표치에 닿은 뒤 `Play()`하도록 미룬다.
- **확신도**: 중간

### 4. 🟡 컷신 태스크의 무적 GE 제거에 권위 게이트가 없다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:24`
- **범주**: 설계/구조 (리플리케이션 권한)
- **문제**: `OnDestroy`가 머신 구분 없이 `RemoveActiveGameplayEffectBySourceEffect(UWxEffect_Invincible::StaticClass(), nullptr, 1)`을 부른다. 엔진의 활성 GE 제거 경로에는 권위 검사가 없어 클라에서도 그대로 지워지는데, 소유 클라에서 예측본 대신 서버 복제본이 먼저 매칭되면 서버 배열에는 항목이 남은 채 그 클라만 무적 태그를 잃는다. 같은 모듈의 `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_ApplyGameplayEffect.cpp:28-32`가 정확히 이 위험을 주석으로 명시하고 `IsOwnerActorAuthoritative()` 게이트를 두고 있어, 두 곳의 규약이 갈라져 있다.
- **제안**: 같은 게이트를 건다.
- **확신도**: 중간

### 5. 🟡 퍼펙트 가드 반사에 실패한 투사체가 반사도 파괴도 되지 않고 계속 난다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:179`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:60`
- **범주**: 버그/정확성
- **문제**: 179~186행은 `bReflecting`이면 `Reflect()`로 보내고 그 경우 `else if (!bEvaded) Destroy()`를 건너뛴다. 그런데 `Reflect`는 60~63행에서 `!Parrier || !Shooter`면 조용히 반환한다. 쏜 쪽이 비행 도중 파괴돼 `GetInstigator()`가 널이 되거나, 막아낸 대상이 Pawn이 아니어서 181행의 `Cast<APawn>`이 널이면 투사체가 원래 진영·원래 속도로 계속 날아가 뒤쪽 대상을 반복해 때린다.
- **제안**: `Reflect`가 성공 여부를 반환하게 하고, 실패 시 파괴 경로로 떨어뜨린다.
- **확신도**: 중간

### 6. 🟡 Rush 루트모션 modifier의 상태 저장·복원이 중첩에 안전하지 않다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:59`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp:167`
- **범주**: 설계/구조 (상태 관리)
- **문제**: 각 modifier 인스턴스가 `bSavedControllerYaw`·`bSavedPhysicsRotation`(59~63행)과 `SavedCollisionResponses`(73~83행)를 자기 필드에 담고 167~173행에서 되돌린다. 한 몽타주에 Rush 구간이 둘 겹치면 나중 것이 앞 것이 이미 꺼 놓은 값을 "원본"으로 저장하고, 앞 것이 먼저 진짜 원본을 복원한 뒤 나중 것이 꺼진 값을 덮어써 `bUseControllerRotationYaw`와 캡슐 응답(`ECR_Ignore`)이 영구히 고착된다. 캐릭터가 회전하지 않고 피격 판정이 빠진 채 남는다. 워프 타겟 이름도 `WxAnimNotifyState_Rush.cpp:86`에서 양쪽 다 `"Rush"`로 하드코딩돼 있어 177행의 제거가 아직 살아 있는 쪽 타겟까지 지운다. 88~91행의 `MatchesNotify`는 노티파이 인스턴스 동일성만 보므로 이 중첩을 막지 못한다.
- **제안**: 저장·복원을 아바타 단위 소유(참조 수)로 옮기거나, `InitializeRush`가 같은 아바타에 활성 Rush가 있으면 거절한다.
- **확신도**: 중간(중첩 구간을 저작해야 재현된다)

### 7. 🟡 소환물 초기화 중 로스터가 늘어나면 보관한 배열 참조가 무효화될 수 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:56`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:88`
- **범주**: 버그/정확성 (객체 수명)
- **문제**: 56행에서 `Rosters.FindOrAdd(&Master)`의 값 참조를 보관한 채 88행 `FinishSpawning`으로 컨스트럭션 스크립트와 BeginPlay를 동기 실행하고, 90행에서 그 참조에 추가한다. 초기화나 앞선 `Destroy()`의 EndPlay 연쇄에서 다른 주인의 소환이 들어와 TMap에 **새 키**가 추가되면 원소 저장 공간이 재할당되어 56행의 참조가 무효해지고, 90행이 해제된 메모리에 쓴다. (63~74행의 키 제거만으로는 축소가 일어나지 않아 안전하다.)
- **제안**: 56행의 장기 참조를 버리고, 90행 직전에 `Rosters.FindOrAdd(&Master).Add(Minion)`으로 다시 조회한다.
- **확신도**: 중간(재할당 메커니즘은 확실하나 중첩 소환의 실제 에셋 구성은 확인하지 않았다)

### 8. 🟡 ScreenBounds 필터가 비로컬 PlayerController에서 모든 후보를 제외한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_ScreenBounds.cpp:19`
- **범주**: 버그/정확성
- **문제**: 19행 가드가 `!PlayerController`만 본다. 서버에서 원격 클라의 폰을 소스로 이 프리셋을 돌리면 PC는 있지만 `ULocalPlayer`가 없어 26행의 `ProjectWorldLocationToScreen`이 무조건 false를 반환하고, 28행이 모든 타겟을 제외한다. "화면 밖"과 "판정 불가"가 같은 값으로 합쳐져 있어 헤더가 선언한 계약(플레이어 컨트롤러가 없으면 아무것도 제외하지 않는다)과도 어긋난다. 34~37행의 뷰포트 크기 안전망은 그 실패 경로보다 뒤라 사실상 도달하지 않는다. 현재 소비처(`GA_Shared_LockOn`)가 로컬 실행 전용이라 노출은 제한적이지만 재사용하면 바로 성립한다.
- **제안**: 19행을 `!PlayerController || !PlayerController->IsLocalController()`로 넓히고, 26행의 실패를 "제외"가 아니라 "판정 불가(통과)"로 가른다.
- **확신도**: 중간

### 9. 🟡 Attack·Skill·Pattern의 콤보 인덱스 로직이 세 벌로 복제돼 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:19`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:24`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:19`
- **범주**: 중복/복잡도
- **문제**: Attack의 19~53행과 Skill의 24~58행은 클래스 이름만 빼면 완전히 동일하고(`ActivateAbility`·`EndAbility`·`HandleMontageCompleted` 전부), Pattern의 19~46행도 앞 둘과 같다. `ComboMontages`·`ComboIndex` 선언도 세 헤더에 각각 있다. 항목 1의 인덱스 초기화 결함이 Pattern에만 남아 있는 것이 이 복제가 이미 갈라지고 있다는 증거다.
- **제안**: 콤보 커서 진행(`ComboMontages`, `ComboIndex`, 진행·초기화 규칙)만 `UWxAbilityBase` 아래 공통분으로 올리고, 각 클래스는 태그·쿨다운·연쇄 방식 차이만 남긴다. 배선 재사용 목적의 중간 클래스는 만들지 않는다.
- **확신도**: 중간(공통분을 어디까지 올릴지는 판단이 필요하다)

### 10. 🟡 비권위 머신의 Rush 소환물 탐색이 월드 전체 폰을 순회한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp:53`
- **범주**: 성능/안전
- **문제**: `TargetSource`가 소환물이고 권위가 아니면 53~63행이 `TActorIterator<APawn>`으로 로드된 폰을 전부 훑고, 폰마다 `GetMaster()`·ASC 조회·태그 검사를 더한다. 이 함수는 `bIsNativeBranchingPoint`(18행) 노티파이의 Begin에서 불려 애니메이션 평가 도중 게임 스레드에서 돈다. 오픈월드에서 로드된 폰이 수백을 넘으면 돌진마다 히치가 난다. 권위 경로(46~50행)는 이미 `UWxMinionSubsystem` 로스터로 O(1)에 가깝게 푼다.
- **제안**: 소환 관계를 복제되는 목록이나 서브시스템의 클라 미러 로스터로 노출해 비권위 조회도 로스터를 타게 한다.
- **확신도**: 중간

### 11. 🟢 콤보 창에서 진입 조건뿐 아니라 차단 조건도 함께 면제된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:172`
- **범주**: 설계/구조
- **문제**: 활성 상태의 ComboWindow이면 부모에 넘기지 않고 즉시 true를 반환한다. `DoesAbilitySatisfyTagRequirements`는 필수 진입 태그만이 아니라 `ActivationBlockedTags`와 다른 어빌리티의 `BlockAbilitiesWithTag`까지 보는 자리라, 헤더 131행이 설명하는 "진입 태그 조건 면제"보다 넓다. 사망·그로기·피격은 공격을 먼저 취소하므로 이 면제에 닿지 않지만, 공격 BP가 지정한 차단 태그(탈진 등)는 콤보 재발동 중 검사되지 않는다. 후딜(`StartRecovery`)의 설명은 차단 태그를 유지한다고 명시하고 있어 저작자가 두 창의 차이를 놓치기 쉽다.
- **제안**: 면제 범위를 필수 진입 태그로 좁히거나, 차단 태그까지 면제하는 것이 의도라면 헤더에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 12. 🟢 무기 조회 경로가 둘로 갈려 있다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:27`, `Source/WxGame/Character/WxCharacterBase.cpp:142`
- **범주**: 중복/복잡도
- **문제**: `AWxWeaponBase::FindWeapon`은 `GetAttachedActors()`를 훑어 **첫 번째** `AWxWeaponBase`를 집고, 캐릭터의 `GetEquippedWeapon`은 ChildActorComponent에서 집는다. 노티파이 경로(`WxAnimNotifyState_WeaponAttack.cpp:22`, `:38`)와 Exceed 큐(`WxCueNotify_Exceed.cpp:31`)는 전자를, 사망 처리(`WxCharacterBase.cpp:272`)는 후자를 쓴다. 캐릭터에 무기 파생 액터가 하나라도 더 붙으면 둘이 다른 액터를 가리켜 사망 시 `CancelAttack`이 엉뚱한 무기에 걸린다.
- **제안**: 조회를 한 경로로 통일한다(ChildActorComponent 기준 권장).
- **확신도**: 중간

### 13. 🟢 호출자 없는 무기 부착 함수와 읽는 곳 없는 락온 플래그
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:117`, `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnPointComponent.h:44`
- **범주**: 중복/복잡도 (데드 코드)
- **문제**: (가) `AttachToCharacter`는 저장소 전체에 호출자가 없고 UFUNCTION도 아니라 BP에서도 못 부른다. 무기는 실제로 `WxCharacterBase.cpp:144`의 ChildActorComponent로 만들어진다. 짝인 `DetachFromCharacter`는 자기 `EndPlay`(215행)에서만 불리는데, 거기서 정말 필요한 것은 `CancelAttack()`뿐이고 소멸 중인 액터에 `DetachFromActor` + `SetOwner(nullptr)`까지 한다. (나) `UWxLockOnPointComponent::bLockedOn`은 `WxAbilityTask_LockOnCamera.cpp:176`, `:193`에서 쓰이기만 하고 읽는 코드가 없다. private + AllowPrivateAccess 없는 `VisibleInstanceOnly`라 BP도 읽지 못하며, 실제 피대상 표시는 같은 자리에서 다는 `State.LockedOn` 루즈 태그가 담당한다.
- **제안**: 둘 다 제거한다. `EndPlay`는 `CancelAttack()`만 남긴다.
- **확신도**: 높음

### 14. 🟢 무기·투사체의 히트 결과 조립과 공격 종료 처리가 그대로 중복된다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:222`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:146`
- **범주**: 중복/복잡도
- **문제**: (가) 무기 222~240행과 투사체 146~164행의 "`bFromSweep` 분기 → `GetClosestPointOnCollision` 성공/실패 폴백" 19줄이 참조 컴포넌트만 빼고 동일하다. 한쪽만 손보면 근접 히트와 투사체 히트의 임팩트 지점 산출이 갈리는데, 이 값은 `UWxAbilitySystemGlobals`가 Cue 위치로 그대로 쓴다. 양쪽 모두 `bFromSweep==false && OtherComp==nullptr`이면 원점 `FHitResult`가 그대로 `ApplyDamage`로 흘러간다. (나) `WxWeaponBase.cpp:86-95`의 `EndAttack` 종료 처리와 `:106-114`의 `CancelAttack`이 3단계(콜리전 해제·틱 해제·스윙 기록 비우기) 그대로 두 벌이다.
- **제안**: (가)는 `UWxCombatLibrary`로 추출하고, (나)는 private 헬퍼 하나로 합친다.
- **확신도**: 높음

### 15. 🟢 AreaDamage 노티파이에 게임 월드 가드가 없어 에디터 프리뷰에서도 타게팅 쿼리가 돈다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaDamage.cpp:15`
- **범주**: 성능/안전
- **문제**: `UTargetingSubsystem`은 `EditorPreview` 월드에도 존재하므로 Persona에서 타임라인을 스크럽할 때마다 실제 콜리전 쿼리가 돌고 `ApplyDamage`까지 진입한다(프리뷰 액터에 ASC가 없어 `WxCombatLibrary.cpp:74`에서 걸리지만 쿼리 비용은 이미 발생). 같은 폴더의 `WxAnimNotifyState_Rush.cpp:73`은 `IsGameWorld()` 가드를 두고 있어 규약이 갈라져 있다.
- **제안**: `Owner->GetWorld()->IsGameWorld()` 가드를 추가한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_CameraMove.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_Rush.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_ScreenBounds.cpp`.
- **훑은 파일**: `Plugins/WxCombat/README.md`, `Plugins/WxCombat/WxCombat.uplugin`, `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/` 전체(21개 GE·MMC), `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/` 전체, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/` 나머지, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileSubsystem.cpp`, 그리고 대응하는 `Public/` 헤더 전부.
- **미검토 / 한계**: 빌드·PIE·네트워크 실행 검증은 하지 않았고, DT 행·GE BP·몽타주 노티파이 배치 등 에셋 쪽 구성은 리뷰 범위 밖이다. 규칙 점검은 기계적 전수 검사(Copyright 첫 줄, `Wx` prefix, 람다, `Handle` prefix, `BlueprintCallable`, `FORCEINLINE`·헤더 본문 정의, 플러그인 간 참조)로 돌렸고 위반 0건이다 — 외부 include는 `WxGameplayTags.h`·`WxCollisionChannels.h`·`WxUIData.h`뿐으로 전부 `WxCore` 소속이다. `DrawInEditor` 2건(`WxAnimNotifyState_SnapToTarget.cpp:90`, `WxAnimNotify_AreaDamage.cpp:55`)과 `WxAnimNotifyState_SlowTime.cpp:30`의 `GetNotifyName_Implementation`이 `Super::`를 부르지 않지만, CLAUDE.md에 Super 호출 규칙이 없고 해당 베이스 구현이 비어 있어 결함으로 싣지 않았다(`WxAnimNotifyState_Rush.cpp:21`, `:27`의 미호출은 엔진이 빈 `FAnimNotifyEventReference`를 넘기는 문제를 우회한 의도적 대체다). 지난 리뷰에서 제기됐던 ExecCalc의 크리 난수 서버/클라 불일치는 엔진 소스로 재확인한 결과 예측 클라에서는 Instant GE가 무한 지속으로 치환돼 실행 계산이 돌지 않으므로 이월하지 않았다. 데미지 플로터가 피격마다 액터+위젯을 스폰하고 5초를 사는 것(`WxCueNotify_DamageFloater.cpp:34`, `:52`)은 실측 자료가 없어 결함으로 확정하지 않았다. `WxAbilityBase::EndAbility`가 `ActivationOwnedEffectHandles`를 순회하며 GE를 제거하는 구간의 재진입 가능성은 구체적 도달 경로를 찾지 못해 제외했다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 181파일 — `/module-review`로 갱신*
