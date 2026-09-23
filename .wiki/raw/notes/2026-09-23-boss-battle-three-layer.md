---
title: "보스 표시 세 층 구조와 전투 서브시스템"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, combat, game, lifecycle, architecture]
summary: "보스 표시를 모델(UWxBattleSubsystem)·연결(WxGame 리졸버)·VM(WxUI Character VM) 세 층으로 재구성한 사용자 결정과 코드 근거. 보스 식별은 IdentityTags 태그로 바꿨다. 엔진 확인 사실과 알려진 기존 문제, WBP 전환 도구도 기록한다. 인게임 표시는 미검증이다."
---

# 보스 표시 세 층 구조와 전투 서브시스템

2026-09-23 사용자 결정과 커밋 `ac723132d`·`4352e9100`의 정적 확인이다. 작업 기록은 [보스 표시 VM 단순화](../../../.agents/workflow/tasks/boss-display-simplification.md)에 있다. 이전 구조(`UWxViewModel_BossDisplay`가 정적 보스 교전 이벤트 구독·후보 목록·액터 순회·월드 필터를 떠안던 방식)를 대체한다.

## 사용자 확정 결정

- VM은 전부 WxUI에 모은다. 도메인 데이터가 필요한 표시는 세 층으로 나눈다.
  - 모델: WxGame·도메인. VM·MVVM을 모른다.
  - 연결: WxGame 리졸버. VM이 아니다.
  - VM: WxUI. 순수 표시만 담는다.
- 보스 식별은 bool 플래그·전용 클래스 대신 `AWxCharacterBase::IdentityTags`(`Character.*` GameplayTag)로 한다.
- 보스전 상황은 월드 서브시스템 `UWxBattleSubsystem`이 순수 모델로 소유한다.
  - UIManager는 보스 VM을 알지 않는다.
  - 전용 위젯 C++ 클래스를 만들지 않고 기존 `WBP_Nameplate_Boss`를 쓴다.
  - `UWxNameplateComponent`는 머리 위 표시용이라 HUD 게시 역할을 얹지 않는다.

## 구현 (정적 확인)

- **WxCore**: `Character.Boss` 태그. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`.
- **`Source/WxGame/Character/WxCharacterBase.h/.cpp`**
  - `IdentityTags`: EditDefaultsOnly, Categories=Character.
  - `PostInitializeComponents`에서 태그마다 `SetLooseGameplayTagCount(Tag, 1)`로 전 머신의 ASC에 loose 태그로 올린다. 재실행돼도 결과가 같다.
- **`Source/WxGame/Character/WxEnemyCharacter.cpp`**
  - `RefreshEngagement`·`EndPlay`가 `State.Engaged`를 갱신한 뒤 `UWorld::GetSubsystem<UWxBattleSubsystem>(GetWorld())`의 `NotifyEngagementChanged`를 호출한다.
  - `bIsBoss`·`IsBoss`·정적 `OnAnyBossEngagementChanged`는 제거했다.
- **`Source/WxGame/Battle/WxBattleSubsystem.h/.cpp`**
  - `Character.Boss`를 가진 캐릭터만 교전 순서 목록에 모은다. 보스 판정은 추가할 때만 한다.
  - `GetCurrentBoss()`는 맨 앞 보스를 돌려준다. `OnCurrentBossChanged`는 현재 보스가 바뀔 때만 발행한다.
  - 먼저 교전한 보스를 유지하고, 그 보스가 빠지면 다음 순서로 넘어간다.
  - 교전 상태는 머신마다 계산되므로 복제 없이 각 머신이 모은다.
- **`Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.h/.cpp`**
  - `CreateInstance`: 위젯을 Outer로 WxUI `UWxViewModel_Character`를 만든다. `FDelegate::CreateWeakLambda(VM, ...)`로 서브시스템을 구독하고, 현재 보스를 즉시 한 번 반영한다.
  - `DestroyInstance`: `RemoveAll(ViewModel)`로 그 VM의 구독만 끊는다.
  - 리졸버는 위젯 클래스가 공유하는 const 객체라 상태를 들지 않는다. 08c73f513의 `RemoveAll(this)` 버그를 되풀이하지 않기 위해서다.
- **삭제**: `Source/WxGame/MVVM/WxViewModel_BossDisplay`.
- **`WBP_Nameplate_Boss`**
  - VM `VM_BossCharacter`를 `UWxViewModel_Character`·Resolver 생성으로 전환했다.
  - 5개 바인딩(효과 목록·이름·가시성 변환 인자·HP·GP 변환 인자)에서 `Character` 단계를 제거했다.
  - 부모는 `UserWidget`을 유지한다.
- **검증**
  - WxEditor Development 빌드 성공.
  - 새 프로세스에서 WBP 로드와, 경고를 오류로 취급한 컴파일을 통과했다(`Saved/Logs/BossNameplateVerify.log`).
  - 인게임 표시는 미검증이다. 보스 콘텐츠가 없고, `BP_Boss`는 e0e3ecc51에서 삭제되었다.

## 엔진 확인 사실 (로컬 UE 5.8 소스)

- 다른 서브시스템의 `Initialize` 안에서 `GetSubsystem`을 부르면 요청한 서브시스템을 그 자리에서 초기화한다(`Engine/Private/Subsystems/SubsystemCollection.cpp` GetSubsystemInternal, 97-106). `InitializeDependency`는 불필요하다.
- 게임 월드는 GameInstance를 받은 뒤 `InitWorld`로 월드 서브시스템을 초기화한다. LoadMap은 `UnrealEngine.cpp` 16504→16530, PIE는 `GameInstance.cpp` 352→368이다.
- 스트리밍 레벨이 숨겨졌다 다시 보이면 `RouteEndPlay(RemovedFromWorld)`가 `bActorInitialized=false`로 만든다(`Actor.cpp` 3238). 재추가 시 `PostInitializeComponents`가 다시 실행된다(`Level.cpp` 3876).
- MVVM 리졸버는 위젯 클래스가 공유하고, 뷰마다 `CreateInstance`(`MVVMViewClass.cpp` 150)와 `DestroyInstance`(203)를 부른다.
- `UObject::GetWorld`는 Outer를 따라간다(`Obj.cpp` 1209).

## 알려진 기존 문제 (미수정)

`AWxCharacterBase::PostInitializeComponents`의 사망·래그돌 `RegisterGameplayTagEvent(...).AddUObject` 구독은 레벨이 다시 보일 때 중복된다.
- 사망 시 `OnDeath`가 두 번 발행된다.
- 그러면 `AWxEnemyCharacter::HandleOwnerDeath`가 두 번 돌고 보상이 두 번 지급될 수 있다.

별도 작업 대상이다.

## 도구 (커밋 ac723132d)

- **`WxMVVMToolset.SetBindingSourcePath(WidgetBlueprint, BindingId, ArgumentName, SourcePath)`**
  - ArgumentName이 비면 바인딩 소스 경로를 바꾼다. 기존 변환 함수는 엔진처럼 제거된다.
  - ArgumentName을 지정하면 Source→Destination 변환 함수 인자 하나의 경로만 바꾸고, 나머지 인자의 경로·기본값은 유지한다.
  - 경로 형식은 "뷰모델이름.필드[.필드...]" 또는 "Self.필드"다.
  - BlueprintCallable이라 Python(`unreal.WxMVVMToolset.set_binding_source_path`)에서도 호출할 수 있다.
  - D→S 방향은 지원하지 않는다.
- **WBP 전환 함정**
  - VM 클래스를 먼저 삭제한 채 WBP를 로드하면 로드 시 컴파일이 실패해 스켈레톤에 VM 프로퍼티가 없다.
  - 그 상태에서 변환 인자 경로를 설정하면 `MVVMConversionFunctionHelper` ensure("Could not find root property")가 난다.
  - 저장 데이터는 올바르게 남는다. 다만 VM을 재지정(`MVVMEditorSubsystem.ReparentViewModel`)한 뒤 한 번 컴파일하고 경로를 바꾸는 편이 깨끗하다.
