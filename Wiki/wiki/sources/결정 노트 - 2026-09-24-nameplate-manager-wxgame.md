---
type: source
title: "결정 노트 - 2026-09-24-nameplate-manager-wxgame"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "Nameplate"
  - "모듈"
summary: "NameplateManager를 WxUI에서 WxGame 컨트롤러로 옮겨 적과 락온 대상을 직접 읽게 하고 LockOnTargetQuery·마커 컴포넌트·옛 리다이렉트를 지운 기록"
source_type: decision-note
source_id: src-0b2c75e17de191705e7f
sha256: df5e4463b4bfe0308075828c9cf94eefec6fa155651cf035af336138da271bdd
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-nameplate-manager-wxgame.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-nameplate-manager-wxgame.md"
raw_copy: ".raw/captured/df5e4463b4bfe0308075828c9cf94eefec6fa155651cf035af336138da271bdd.md"
claim_ids:
  - clm-b8e2ee8151-c1
  - clm-b8e2ee8151-c2
  - clm-b8e2ee8151-c3
  - clm-b8e2ee8151-c4
key_claims:
  - "2026-09-24 NameplateManager는 WxUI에서 WxGame의 AWxPlayerController 서브오브젝트로 옮겨져 락온 대상을 LockOnComponent에서 직접 읽는다."
  - "이 작업에서 LockOnTargetQuery 델리게이트, UWxNameplateSourceComponent, VisibilityRequirements가 제거되고 State.Engaged 태그는 유지됐다."
  - "Nameplate 높이는 사용자 지시에 따라 SkeletalMesh가 아니라 캡슐 윗면에 HeadClearance(기본 90cm)를 더한 위치다."
  - "이 작업은 빌드·BP 컴파일·레벨 재로드만 확인했고 인게임 표시와 리슨 서버·원격 클라이언트 동작은 미검증이다."
---

# 결정 노트 - 2026-09-24-nameplate-manager-wxgame

- 원본: `.wiki/raw/notes/2026-09-24-nameplate-manager-wxgame.md`
- 원자료 사본: `.raw/captured/df5e4463b4bfe0308075828c9cf94eefec6fa155651cf035af336138da271bdd.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-24. 2026-09-24 제출 기록(사용자 "제출해주세요").
- 작업 기록은 [[작업 - nameplate-manager]]의 "NameplateSource 정적 목록 제거"·"NameplateManager WxGame 이동"·"LockOnTargetQuery 단순화 검토" 절이다. 앞 구조는 [[결정 노트 - 2026-09-24-nameplate-manager]].
- 빌드·BP 컴파일·레벨 재로드는 확인했고 인게임 표시는 미검증이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 사람의 판단 원문

> 사용자 2026-09-24: "LockOnTargetQuery 이걸 사용하는 방식이 좀 복잡해보이는데요", "코드 품질 개선과 단순화가 목적이긴 해요", "아까 얘기하던 4번 진행하죠".

> 사용자 2026-09-24: "WxCore에 추가하는 것은 원치 않아요" (WxCore에 락온 계약 인터페이스를 두는 안에 대해)

> 사용자 2026-09-24: "SkeletalMesh 윗면 쓰지 말고 캡슐(Root)의 윗면을 쓰세요"

> 사용자: "HeadClearance = 90은 제가 의도한 것이에요"

> 사용자: "오래된 리디렉터 제거 진행합시다."

## 이동 이유와 결정

- 복잡함의 원인: NameplateManager는 UI(위젯·`UWxViewModel_Character`)와 Combat(락온)을 함께 아는 연결 코드인데 WxUI에 있어 델리게이트·마커·태그 비대칭 조건 같은 우회가 필요했다. 의존 팬아웃 원칙에 따라 WxGame으로 옮겼다.
- `State.Engaged`는 유지한다. `AWxEnemyCharacter::RefreshEngagement`가 교전 규칙(`IsAlive() && 자기 락온 대상 != nullptr`)을 한 곳에서 계산하므로, 없애면 소비처(NameplateManager·뒤잡 판정)가 규칙을 각자 다시 계산하게 된다.
- Reticle은 `UWxLockOnComponent`에 두지 않는다(AI·시뮬 프록시에도 붙는 복제 모델). 락온 표식을 태그로 두는 안도 권하지 않았다(부위 컴포넌트 질의가 남고, 보는 사람마다 다른 상태가 리슨 호스트의 권위 ASC에 섞임).

## 구현 관찰(노트 시점 구조)

- `Source/WxGame/Controller/WxNameplateManagerComponent.h/.cpp`: `AWxPlayerController`의 네이티브 서브오브젝트, 로컬 컨트롤러에서만 틱.
- 락온 대상은 `GetPawn<AWxCharacterBase>()->GetLockOnComponent()->GetLockOnTarget()`으로 직접 읽는다. 대상은 `TActorIterator<AWxEnemyCharacter>`로 찾고 `HasActorBegunPlay()`가 아닌 적은 건너뛴다.
- 표시 조건: `Viewer && IsAlive() && (bLockedOn || (bInRange && bEngaged))`, `bEngaged`는 `State.Engaged` 태그.
- 높이: 캡슐에 붙이고 `GetUnscaledCapsuleHalfHeight() + HeadClearance`, 기본값 90cm.
- 지운 것: `LockOnTargetQuery`, `AWxPlayerController::BeginPlay`·`GetLockOnTarget()`, `UWxNameplateSourceComponent`, `VisibilityRequirements`(사망 판정은 `Ability.Death` 대신 HP 0).

## 오래된 CoreRedirects 제거

- WxGame→WxUI ClassRedirect 4개(`WxViewModelResolver_Ability`·`WxViewModel_Quest`·`WxViewModel_QuestObjective`·`WxViewModel_Dialogue`)는 이미 빠져 있었다. 참조 WBP 6개 헤드리스 로드 시 누락 클래스 경고 0, `CompileAllBlueprints` 오류·경고 0.
- `BP_PlayerController`를 MCP로 다시 저장한 뒤(저장 전 `HeadClearance` 오버라이드가 생기지 않음을 대조) Nameplate 리다이렉트도 지워 `[CoreRedirects]` 섹션이 없어졌다.

## 검증 범위

- 확인: WxEditor Development 빌드 성공, 헤드리스 Python으로 `BP_PlayerController` 컴포넌트 클래스·위젯 클래스 값 유지, 적 BP 5종과 `LV_DevCombat` 배치 액터 재저장, 새 프로세스에서 LogLinker·Nameplate 경고 0(전체 컴파일 실패는 무관한 `EUB_SnapToActor` 하나), 리다이렉트 0개 상태에서 관련 BP 7개 컴파일 오류 0.
- 미검증: 인게임 표시, 리슨 서버·원격 클라이언트.

## 관련 주제

- [[UI 표시 구조]]
- [[모듈 구조와 코드 정리]]
- [[기획서 - Nameplate_System]]

## 핵심 주장

- 2026-09-24 NameplateManager는 WxUI에서 WxGame의 AWxPlayerController 서브오브젝트로 옮겨져 락온 대상을 LockOnComponent에서 직접 읽는다. ^c1
- 이 작업에서 LockOnTargetQuery 델리게이트, UWxNameplateSourceComponent, VisibilityRequirements가 제거되고 State.Engaged 태그는 유지됐다. ^c2
- Nameplate 높이는 사용자 지시에 따라 SkeletalMesh가 아니라 캡슐 윗면에 HeadClearance(기본 90cm)를 더한 위치다. ^c3
- 이 작업은 빌드·BP 컴파일·레벨 재로드만 확인했고 인게임 표시와 리슨 서버·원격 클라이언트 동작은 미검증이다. ^c4
