---
title: "Nameplate·락온 Reticle을 로컬 NameplateManager가 붙이고 떼는 구조"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, ui, combat, architecture]
summary: "적마다 위젯을 만들던 UWxNameplateComponent와 락온 태스크의 State.LockedOn·레티클 생성을 없애고, 플레이어 컨트롤러의 WxUI UWxNameplateManagerComponent가 로컬에서만 Nameplate·Reticle을 붙이고 떼게 했다. 적은 등록만 하는 UWxNameplateSourceComponent를 가진다. 빌드·BP 컴파일은 확인했고 인게임은 미검증."
---

# Nameplate·락온 Reticle을 로컬 NameplateManager가 붙이고 떼는 구조

커밋 `aaf557a09`(2026-09-24). 작업 기록: [Nameplate 작업 자료](../../../.agents/workflow/tasks/nameplate-manager.md). 기획 근거: [Nameplate 기획서](../../../Docs/CombatDesign/Nameplate_System.md) 5장(기본 숨김, 인식 시·카메라 락온 시 표시, 추적이 끝나면 즉시 숨김).

## 바뀌기 전

- 락온 카메라 태스크(`UWxAbilityTask_LockOnCamera`)가 대상 적 ASC에 `State.LockedOn` 루즈 태그를 붙이고 레티클 `UWidgetComponent`를 직접 만들었다. 리슨 호스트와 싱글에서는 이 태그가 적의 권위 ASC에 올라갔다.
- `UWxNameplateComponent`(WidgetComponent)가 모든 적에 네이티브로 붙어 BeginPlay 때 위젯을 만들고 매 틱 거리를 쟀다.
- `WBP_Nameplate_Enemy`가 자기 가시성을 `ANY(State.LockedOn, State.Engaged)`·사망 제외 태그 조건으로 바인딩했다.

## 사용자 결정 (2026-09-23~24)

1. 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다.
2. Nameplate는 동적으로 붙이고 뗀다. NameplateManager는 WxUI에 둔다(Lyra의 `NameplateSource`·`NameplateManagerComponent` 구조).
3. 태그 조건은 NameplateManager에 하나만 둔다.
4. 레티클도 NameplateManager로 옮긴다.
5. 높이는 캐릭터 메시 기준 머리 위 30cm로 통일한다. 기본 포즈 메시 높이로 고정하고(애니메이션 바운드·`head` 본 방식 기각) 캡슐(RootComponent)에 붙인다.
6. 위치·위젯이 없어진 컴포넌트는 `UWxNameplateSourceComponent`로 이름을 바꾼다.
7. 코드 리뷰 후속: LockOn 대상에는 표시 거리 제한을 두지 않는다(락온 가능 거리는 락온 쪽이 정한다). 표시 거리 경계에 여유(`VisibilityDistanceHysteresis`)를 둔다. 질의 이름은 일반화한 `FocusQuery`가 아니라 `LockOnTargetQuery`다("Focus 대상이 아니라, LockOn 대상 아닌가요?").

## 현재 구조

- **WxUI `UWxNameplateSourceComponent`**(ActorComponent): 적(`AWxEnemyCharacter`)에 네이티브로 붙는다. BeginPlay·EndPlay에서 정적 목록에 등록·해제만 한다. PIE에서는 여러 월드의 것이 섞이므로 쓰는 쪽이 월드를 거른다.
- **WxUI `UWxNameplateManagerComponent`**: `AWxPlayerController`에 네이티브로 붙고, 로컬 컨트롤러에서만 틱을 켠다(리슨 호스트 포함 소유 클라).
  - 매 틱 `LockOnTargetQuery`로 LockOn 대상 지점을 받아 Reticle을 그 지점에 붙인다. 대상 지점이 바뀌면 떼고 다시 붙인다.
  - 등록된 NameplateSource를 훑어 보일 대상에만 Nameplate 위젯 컴포넌트를 대상 루트에 붙이고, 조건을 벗어나면 뗀다.
  - 판정: 거리 안이고 `VisibilityRequirements`(C++ 기본값 Require `State.Engaged`, Ignore `Ability.Death`)를 만족하면 보인다. LockOn 대상의 주인은 거리와 Require를 건너뛰고 Ignore만 따른다.
  - 거리: 새로 붙이려면 `MaxVisibilityDistance`(3000cm) − `VisibilityDistanceHysteresis`(200cm) 안쪽이어야 하고, 이미 붙은 것은 `MaxVisibilityDistance`까지 유지한다.
  - 높이: Nameplate를 만들 때 한 번, `ACharacter::GetMesh()`의 `GetImportedBounds()`를 메시→루트 상대 변환한 상자의 `Max.Z`에 `HeadClearance`(30cm)를 더한다. 애니메이션 바운드를 따르면 모션마다 흔들리기 때문이다.
  - 위젯 컴포넌트는 대상 액터 소유(`NewObject<UWidgetComponent>(TargetActor)`)로 만든다. 대상이 파괴되면 함께 사라지고, NameplateManager의 EndPlay에서도 직접 뗀다.
  - Character VM은 `UWxViewModel_Character::GetOrCreate(TargetASC, Target)` 공유본을 `SetViewModelByClass`로 넣는다. 공유본 수명은 MVVM View가 유지한다.
  - 위젯 클래스·거리·스케일·높이 설정은 NameplateManager가 가진다. `BP_PlayerController`에 `NameplateWidgetClass`=`WBP_Nameplate_Enemy`, `ReticleWidgetClass`=`WBP_LockOnReticle`을 지정했다.
- **LockOnTargetQuery**: NameplateManager의 네이티브 단일 델리게이트(`USceneComponent*` 반환)다. 락온은 WxCombat 소유이고 WxUI는 WxCombat을 include하지 않으므로, WxGame `AWxPlayerController::BeginPlay`가 빙의 캐릭터의 `UWxLockOnComponent::GetLockOnTarget`에 바인딩한다.
- **WxCombat**: 락온 카메라 태스크에서 루즈 태그와 레티클 생성·파괴, `ReticleWidgetClass` 인자를 걷었다. `UWxAbility_LockOn`의 `ReticleWidgetClass` 속성을 지웠다(`GA_Shared_LockOn`의 옛 값은 로드 때 버려진다).
- **WxCore**: `State.LockedOn` 태그를 지웠다.
- `WBP_Nameplate_Enemy`의 가시성 태그 바인딩을 지웠다. 붙어 있으면 보이고, 사망 시 제거는 NameplateManager가 한다.
- `State.Engaged`는 적이 복제된 자기 락온 대상으로 스스로 붙인다(`AWxEnemyCharacter::RefreshEngagement`). 이 부분은 바뀌지 않았다.

## 에셋·리다이렉트

- 클래스 리다이렉트는 넣지 않았다. 넣으면 배치 액터에 저장된 옛 서브오브젝트가 새 클래스로 로드되어 한 액터에 NameplateSource가 둘이 될 수 있다. 리다이렉트가 없으면 옛 데이터는 "클래스 없음" 경고와 함께 버려진다.
- 적 BP 5종, `LV_DevCombat` 배치 액터 1, `LV_OpenWorld` 스포너 2를 옛 컴포넌트 데이터를 없애려 다시 저장했다. 스포너 2개는 저장되지 않는 옛 미리보기 데이터가 함께 빠져 크기가 줄었다.

## 검증

- WxEditor Development 빌드 성공. `CompileAllBlueprints`(적 BP 5종, `BP_PlayerController`, `GA_Shared_LockOn`, `WBP_Nameplate_Enemy`, `WBP_LockOnReticle`) 오류 0.
- `/code-review high` 후보 9건을 코드·에셋으로 검증해 낡은 주석 1건만 결함으로 고쳤다. 매 틱 전체 순회는 예전에 적마다 같은 계산을 하던 것을 모은 것이라 퇴행이 아니다.
- 미검증: 인게임 표시(교전 전 숨김, 락온 시 표시, 대상 교체 시 레티클 이동, 사망 시 제거, 거리 스케일·높이)와 리슨 서버·원격 클라이언트에서 각자 자기 락온만 보이는지. 확인 항목은 작업 자료의 "인게임 확인 항목"에 있다.

근거: [NameplateManager](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxNameplateManagerComponent.cpp), [NameplateSource](../../../Plugins/WxUI/Source/WxUI/Public/Component/WxNameplateSourceComponent.h), [PlayerController](../../../Source/WxGame/Controller/WxPlayerController.cpp), [EnemyCharacter](../../../Source/WxGame/Character/WxEnemyCharacter.cpp), [LockOnCamera 태스크](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp). HEAD `ca84c9aac`에서 코드를 다시 읽어 대조했다.
