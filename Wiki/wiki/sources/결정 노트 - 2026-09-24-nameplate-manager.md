---
type: source
title: "결정 노트 - 2026-09-24-nameplate-manager"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "Nameplate"
  - "락온"
summary: "적마다 위젯을 만들던 Nameplate와 락온 태스크의 레티클 생성을 없애고 플레이어 컨트롤러의 NameplateManager가 로컬에서 붙이고 떼게 한 구조 기록"
source_type: decision-note
source_id: src-31e474b69a18800c3fa0
sha256: 9b281e65e4f5d167c014be26c4d3d77096e1f69a03aabc0e81f836cee5551bf1
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-nameplate-manager.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-nameplate-manager.md"
raw_copy: ".raw/captured/9b281e65e4f5d167c014be26c4d3d77096e1f69a03aabc0e81f836cee5551bf1.md"
claim_ids:
  - clm-f215e78b23-c1
  - clm-f215e78b23-c2
  - clm-f215e78b23-c3
  - clm-f215e78b23-c4
key_claims:
  - "커밋 aaf557a09는 락온 카메라 태스크의 State.LockedOn 루즈 태그와 레티클 생성을 없애고 플레이어 컨트롤러의 NameplateManager가 로컬에서 Nameplate·Reticle을 붙이고 떼게 했다."
  - "사용자는 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다고 결정했다."
  - "이 구조에서 새 Nameplate는 MaxVisibilityDistance 3000cm에서 히스테리시스 200cm를 뺀 거리 안에서만 붙고, LockOn 대상은 거리 제한을 받지 않는다."
  - "노트 시점의 WxUI 배치와 LockOnTargetQuery·NameplateSource 구조는 같은 날 WxGame 이동 작업으로 대체됐다."
---

# 결정 노트 - 2026-09-24-nameplate-manager

- 원본: `.wiki/raw/notes/2026-09-24-nameplate-manager.md`
- 원자료 사본: `.raw/captured/9b281e65e4f5d167c014be26c4d3d77096e1f69a03aabc0e81f836cee5551bf1.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 옛 LLM Wiki 결정 노트. frontmatter: 출처 `MANUAL`, 수집일 2026-09-24. 커밋 `aaf557a09`(2026-09-24), HEAD `ca84c9aac`에서 코드를 다시 읽어 대조했다.
- 작업 기록은 [[작업 - nameplate-manager]], 기획 근거는 [[기획서 - Nameplate_System]] 5장(기본 숨김, 인식 시·카메라 락온 시 표시, 추적이 끝나면 즉시 숨김).
- 이 구조의 WxUI 배치·`LockOnTargetQuery`·`UWxNameplateSourceComponent`는 같은 날 뒤이은 작업에서 바뀌었다([[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]). 노트 날짜 기준이라 현재 코드와 다르다.

## 바뀌기 전(구현 관찰)

- 락온 카메라 태스크(`UWxAbilityTask_LockOnCamera`)가 대상 적 ASC에 `State.LockedOn` 루즈 태그를 붙이고 레티클 `UWidgetComponent`를 직접 만들었다. 리슨 호스트·싱글에서는 이 태그가 적의 권위 ASC에 올라갔다.
- `UWxNameplateComponent`가 모든 적에 붙어 BeginPlay 때 위젯을 만들고 매 틱 거리를 쟀다. `WBP_Nameplate_Enemy`가 `ANY(State.LockedOn, State.Engaged)`·사망 제외 태그로 가시성을 바인딩했다.

## 확정 결정(사용자, 2026-09-23~24)

1. 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다.
2. Nameplate는 동적으로 붙이고 뗀다. NameplateManager는 WxUI에 둔다(Lyra의 `NameplateSource`·`NameplateManagerComponent` 구조).
3. 태그 조건은 NameplateManager에 하나만 둔다.
4. 레티클도 NameplateManager로 옮긴다.
5. 높이는 캐릭터 메시 기준 머리 위 30cm로 통일하고 기본 포즈 메시 높이로 고정해 캡슐에 붙인다(애니메이션 바운드·`head` 본 방식 기각).
6. 위치·위젯이 없어진 컴포넌트는 `UWxNameplateSourceComponent`로 이름을 바꾼다.
7. 코드 리뷰 후속: LockOn 대상에는 표시 거리 제한이 없고, 표시 거리 경계에 `VisibilityDistanceHysteresis`를 둔다. 질의 이름은 `LockOnTargetQuery`다.

> 사용자: "Focus 대상이 아니라, LockOn 대상 아닌가요?"

(노트는 결정 1~6을 요약으로만 적었고 원문 인용은 위 한 문장뿐이다.)

## 구현 관찰(커밋 `aaf557a09` 시점)

- WxUI `UWxNameplateSourceComponent`: 적에 붙어 BeginPlay·EndPlay에서 정적 목록에 등록·해제만 한다.
- WxUI `UWxNameplateManagerComponent`: `AWxPlayerController`에 붙고 로컬 컨트롤러에서만 틱. 매 틱 `LockOnTargetQuery`로 Reticle 위치를 정하고, NameplateSource를 훑어 보일 대상에만 위젯을 붙인다.
- 판정: 거리 안이고 `VisibilityRequirements`(Require `State.Engaged`, Ignore `Ability.Death`)를 만족하면 보인다. LockOn 대상은 거리·Require를 건너뛰고 Ignore만 따른다.
- 거리: 새로 붙이려면 `MaxVisibilityDistance`(3000cm) − `VisibilityDistanceHysteresis`(200cm) 안, 붙은 것은 3000cm까지 유지.
- 높이: 생성 때 한 번 `GetImportedBounds()` 상자의 `Max.Z` + `HeadClearance`(30cm).
- 위젯 컴포넌트는 대상 액터 소유로 만들고, Character VM은 `UWxViewModel_Character::GetOrCreate` 공유본을 넣는다. `BP_PlayerController`에 `WBP_Nameplate_Enemy`·`WBP_LockOnReticle`을 지정했다.
- `LockOnTargetQuery`는 WxUI가 WxCombat을 include하지 않아 WxGame `AWxPlayerController::BeginPlay`가 `UWxLockOnComponent::GetLockOnTarget`에 바인딩했다.
- WxCombat 락온 태스크에서 루즈 태그·레티클 생성과 `ReticleWidgetClass`를 걷고, WxCore `State.LockedOn` 태그를 지웠다. `State.Engaged`(`AWxEnemyCharacter::RefreshEngagement`)는 그대로다.
- 클래스 리다이렉트는 넣지 않았다(배치 액터에 NameplateSource가 둘이 될 수 있어서). 적 BP 5종, `LV_DevCombat` 배치 액터 1, `LV_OpenWorld` 스포너 2를 재저장했다.

## 검증 범위

- 확인: WxEditor Development 빌드 성공, `CompileAllBlueprints`(관련 BP) 오류 0, `/code-review high` 후보 9건 중 낡은 주석 1건만 결함으로 수정.
- 미검증: 인게임 표시(교전 전 숨김, 락온 시 표시, 레티클 이동, 사망 시 제거, 거리 스케일·높이), 리슨 서버·원격 클라이언트에서 각자 자기 락온만 보이는지.

## 관련 주제

- [[UI 표시 구조]]
- [[플레이어 캐릭터와 조작]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 커밋 aaf557a09는 락온 카메라 태스크의 State.LockedOn 루즈 태그와 레티클 생성을 없애고 플레이어 컨트롤러의 NameplateManager가 로컬에서 Nameplate·Reticle을 붙이고 떼게 했다. ^c1
- 사용자는 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다고 결정했다. ^c2
- 이 구조에서 새 Nameplate는 MaxVisibilityDistance 3000cm에서 히스테리시스 200cm를 뺀 거리 안에서만 붙고, LockOn 대상은 거리 제한을 받지 않는다. ^c3
- 노트 시점의 WxUI 배치와 LockOnTargetQuery·NameplateSource 구조는 같은 날 WxGame 이동 작업으로 대체됐다. ^c4
