---
title: "NameplateManager를 WxGame으로 옮기고 마커·락온 질의를 제거"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, ui, game, architecture]
summary: "WxUI의 UWxNameplateManagerComponent를 WxGame Controller로 옮겨 적(AWxEnemyCharacter)과 락온 대상을 직접 읽게 했다. LockOnTargetQuery 델리게이트·PC 연결 코드·UWxNameplateSourceComponent·VisibilityRequirements를 지웠고, 높이는 캡슐 윗면 기준으로 바꿨다. State.Engaged는 유지한다. 빌드·BP 컴파일·레벨 재로드는 확인했고 인게임은 미검증."
---

# NameplateManager를 WxGame으로 옮기고 마커·락온 질의를 제거

2026-09-24 제출(사용자 "제출해주세요"). 작업 기록: [Nameplate 작업 자료](../../../.agents/workflow/tasks/nameplate-manager.md)의 "NameplateSource 정적 목록 제거"·"NameplateManager WxGame 이동"·"LockOnTargetQuery 단순화 검토" 절. 앞 구조는 [Nameplate·Reticle을 로컬 NameplateManager로](2026-09-24-nameplate-manager.md).

## 계기와 사용자 판단

- 사용자: "LockOnTargetQuery 이걸 사용하는 방식이 좀 복잡해보이는데요", "코드 품질 개선과 단순화가 목적이긴 해요", "아까 얘기하던 4번 진행하죠".
- WxCore에 락온 계약 인터페이스를 두는 안은 사용자가 원치 않았다("WxCore에 추가하는 것은 원치 않아요").
- 복잡함의 원인: NameplateManager는 UI(위젯·`UWxViewModel_Character`)와 Combat(락온)을 함께 알아야 하는 연결 코드인데 WxUI에 있었다. 그래서 델리게이트·마커·태그 비대칭 조건 같은 우회가 필요했다. 모듈 위치를 의존 팬아웃으로 정한다는 원칙에 따라 WxGame으로 옮겼다.
- `State.Engaged`는 유지한다. `AWxEnemyCharacter::RefreshEngagement`가 교전 규칙(`IsAlive() && 자기 락온 대상 != nullptr`)을 한 곳에서 계산해 태그로 알리므로, 없애면 소비처(NameplateManager·뒤잡 판정)가 규칙을 각자 다시 계산하게 된다.
- Reticle은 `UWxLockOnComponent`에 두지 않는다. LockOnComponent는 AI·시뮬 프록시에도 붙는 복제 모델이다. Reticle은 Nameplate와 입력(로컬 락온 대상)과 방식(대상 소유 월드 위젯)이 같다.
- 사용자 지시: "SkeletalMesh 윗면 쓰지 말고 캡슐(Root)의 윗면을 쓰세요".
- 락온 표식을 태그로 두는 안은 권하지 않았다. Reticle에는 부위 컴포넌트가 필요해 질의가 남는다. 또 보는 사람마다 다른 상태가 리슨 호스트의 권위 ASC에 섞인다.

## 현재 구조

- `Source/WxGame/Controller/WxNameplateManagerComponent.h/.cpp`(WxGame): `AWxPlayerController`의 네이티브 서브오브젝트다. 로컬 컨트롤러에서만 틱한다.
- 락온 대상은 `GetPawn<AWxCharacterBase>()->GetLockOnComponent()->GetLockOnTarget()`으로 직접 읽는다.
- 대상은 `TActorIterator<AWxEnemyCharacter>`로 찾고, `HasActorBegunPlay()`가 아닌 적은 건너뛴다. `TActorIterator`는 클래스 해시(`GetObjectsOfClass`)로 모은 뒤 월드를 거른다(`EngineUtils.h:191`). 기본 플래그는 `OnlyActiveLevels | SkipPendingKill`이다.
- 표시 조건은 `Viewer && IsAlive() && (bLockedOn || (bInRange && bEngaged))`이고, `bEngaged`는 `State.Engaged` 태그다.
- 높이는 캡슐에 붙인 뒤 `GetUnscaledCapsuleHalfHeight() + HeadClearance`다. 기본값은 이후 사용자가 90cm로 정했다("HeadClearance = 90은 제가 의도한 것이에요").
- 지운 것
  - `LockOnTargetQuery`
  - `AWxPlayerController::BeginPlay`·`GetLockOnTarget()`
  - `UWxNameplateSourceComponent`와 적의 `NameplateSourceComponent`
  - `VisibilityRequirements`. 사망 판정은 `Ability.Death` 태그 대신 HP 0이다.
- `Config/DefaultEngine.ini`: 이동 직후 `+ClassRedirects=(OldName="/Script/WxUI.WxNameplateManagerComponent",NewName="/Script/WxGame.WxNameplateManagerComponent")`를 두었다가, `BP_PlayerController`를 다시 저장한 뒤 제거했다(아래).

## 검증

- WxEditor Development 빌드 성공.
- 헤드리스 Python: `BP_PlayerController`의 컴포넌트 클래스가 WxGame 경로로 로드되고 `NameplateWidgetClass`·`ReticleWidgetClass` 값이 유지됐다.
- 적 BP 5종과 `LV_DevCombat` 배치 액터 1개를 다시 저장해 옛 마커 데이터를 없앴다. 외부 액터 패키지는 맵을 불러온 뒤 `save_packages`로 저장된다.
- 새 프로세스에서 확인했다. 전체 `CompileAllBlueprints`의 실패는 무관한 `EUB_SnapToActor` 하나다. `LV_DevCombat`과 관련 BP를 다시 불러왔을 때 LogLinker·Nameplate 경고는 0건이다.
- 미검증: 인게임 표시, 리슨 서버·원격 클라이언트.

## 오래된 CoreRedirects 제거

- 사용자: "오래된 리디렉터 제거 진행합시다."
- 옛 ClassRedirect 4개(WxGame→WxUI의 `WxViewModelResolver_Ability`·`WxViewModel_Quest`·`WxViewModel_QuestObjective`·`WxViewModel_Dialogue`)는 작업 사본에서 이미 빠져 있었다.
  - 참조 WBP 6개를 리다이렉트 없이 헤드리스로 로드했을 때 누락 클래스 경고가 0건이었다. `CompileAllBlueprints` 결과도 오류 0, 경고 0이었다.
- `BP_PlayerController`는 실행 중인 에디터에서 MCP로 다시 저장했다.
  - 저장 전에 네이티브 기본값과 BP 저장본을 대조해, `HeadClearance` 오버라이드가 생기지 않는 것을 확인했다.
  - 저장 후 Nameplate 리다이렉트도 지웠다. `[CoreRedirects]` 섹션은 없어졌다.
- 리다이렉트가 0개인 상태에서 `BP_PlayerController`의 컴포넌트 클래스와 위젯 클래스 값이 유지된다. 관련 BP 7개 컴파일 결과는 오류 0이다.
- 에셋 리다이렉터 패키지는 없다.
