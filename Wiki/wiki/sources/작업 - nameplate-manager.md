---
type: source
title: "작업 - nameplate-manager"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "Nameplate"
  - "락온"
summary: "적 Nameplate와 락온 레티클을 로컬 플레이어 컨트롤러의 NameplateManager가 붙이고 떼는 구조로 바꾸고 WxGame으로 옮긴 완료 작업 기록"
source_type: task-record
source_id: src-adaaf5f0488283bcc1c8
sha256: b6020103ecffc7507a7ae78cb774df9d1287a01763acc6ca9fd2a9394ac2ce64
authority: primary
independence_key: ".agents/workflow/tasks/nameplate-manager.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/nameplate-manager.md"
raw_copy: ".raw/captured/b6020103ecffc7507a7ae78cb774df9d1287a01763acc6ca9fd2a9394ac2ce64.md"
claim_ids:
  - clm-a387287572-c1
  - clm-a387287572-c2
  - clm-a387287572-c3
  - clm-a387287572-c4
key_claims:
  - "적 Nameplate와 락온 레티클은 WxGame의 UWxNameplateManagerComponent가 로컬 플레이어 컨트롤러에서 붙이고 떼며, 표시 조건은 IsAlive() && (락온 대상 || (거리 안 && State.Engaged))이다."
  - "Nameplate 작업은 State.LockedOn 태그, UWxNameplateSourceComponent, LockOnTargetQuery 델리게이트를 제거했고 State.Engaged 태그는 유지했다."
  - "Nameplate 높이는 캡슐 반높이에 HeadClearance를 더한 값이며 헤더 기본값 90은 사용자가 의도한 값이다."
  - "Nameplate 작업의 인게임 6개 항목(리슨 서버·원격 클라이언트 구분 포함)은 2026-09-25 이우성이 모두 통과로 확인했다."
---

# 작업 - nameplate-manager

- 원본: `.agents/workflow/tasks/nameplate-manager.md`
- 원자료 사본: `.raw/captured/b6020103ecffc7507a7ae78cb774df9d1287a01763acc6ca9fd2a9394ac2ce64.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

락온 표시가 대상 ASC 루즈 태그(`State.LockedOn`)와 락온 카메라 태스크가 만든 레티클 위젯으로 흩어져 있던 문제(WxCombat 모듈 리뷰 지적)를 정리한 작업이다. 적마다 위젯을 만들던 `UWxNameplateComponent` 대신 로컬 컨트롤러의 `UWxNameplateManagerComponent`가 Nameplate와 레티클을 동적으로 붙이고 뗀다. 상태는 완료이며 체크리스트 6/6이 사람 확인으로 통과했다.

기획 근거는 [[기획서 - Nameplate_System]] 5장이다(요구사항): 기본은 숨김, 인식 시와 카메라 락온 시 표시, 추적이 끝나면 즉시 숨김.

## 조사 (구현 관찰)

- 락온 카메라 태스크(`UWxAbilityTask_LockOnCamera`)가 대상 적 ASC에 `State.LockedOn`을 붙였고, 이 태그를 쓰는 곳은 `WBP_Nameplate_Enemy`의 가시성 조건 하나뿐이었다. 같은 태스크가 레티클 `UWidgetComponent`도 직접 만들었다.
- `State.Engaged`는 적이 복제된 자기 락온 대상으로 스스로 붙인다(`AWxEnemyCharacter::RefreshEngagement`).
- Lyra는 폰의 `NameplateSource`가 등록만 하고 로컬 컨트롤러의 `NameplateManagerComponent`가 인디케이터를 붙인다.

## 사용자 결정 (2026-09-23)

1. 교전하지 않은 적도 락온하면 Nameplate가 떠야 한다.
2. Nameplate는 동적으로 붙이고 뗀다. NameplateManager는 WxUI에 둔다(→ 2026-09-24 WxGame 이동으로 뒤집힘).
3. 태그 조건은 NameplateManager에 하나만 둔다.
4. 레티클도 NameplateManager로 옮긴다.

## 구현 흐름

- 1차(09-23): `UWxNameplateComponent`를 위젯·틱 없는 등록용 SceneComponent로 바꾸고, WxUI에 `UWxNameplateManagerComponent`를 신설했다. 로컬 컨트롤러에서만 틱하며 레티클을 락온 대상 지점에 붙인다. 락온 태스크의 루즈 태그·레티클 생성, `UWxAbility_LockOn`의 `ReticleWidgetClass`, WxCore의 `State.LockedOn` 태그를 제거했다. `WBP_Nameplate_Enemy`의 자체 가시성 바인딩도 삭제했다.
- 높이·이름(09-24): 등록 컴포넌트를 `UWxNameplateSourceComponent`(ActorComponent)로 바꿨다. 클래스 리다이렉트는 한 액터에 NameplateSource가 둘 생길 수 있어 넣지 않았다.
- 정적 목록 제거(09-24, 사용자 "네. 진행해주세요"): `TObjectIterator`와 `HasBegunPlay()` 조회로 바꿨다. 구 오버랩 탐색은 콜리전 채널 의존·비용 때문에 기각했다.
- WxGame 이동(09-24): 아래 "LockOnTargetQuery 검토"의 4번 안을 채택했다. `UWxNameplateManagerComponent`를 WxGame `Controller/`로 옮기고, 빙의 캐릭터의 `GetLockOnComponent()->GetLockOnTarget()`을 직접 읽는다. `LockOnTargetQuery` 델리게이트, `UWxNameplateSourceComponent`, `VisibilityRequirements`를 지웠다. 대상 순회는 `TActorIterator<AWxEnemyCharacter>` + `HasActorBegunPlay()`다.
- 최종 표시 조건: `IsAlive() && (bLockedOn || (bInRange && bEngaged))`. `bEngaged`는 `State.Engaged` 태그로 읽는다. 사망 판정은 `Ability.Death` 태그에서 HP 0(`IsAlive`)으로 바뀌었다.
- 최종 높이: 캡슐에 붙이고 `GetUnscaledCapsuleHalfHeight() + HeadClearance`. 헤더 기본값 `HeadClearance = 90`은 사용자 의도값이다.
- 코드 리뷰 후속: 락온 대상에는 `MaxVisibilityDistance`를 적용하지 않는다(락온 가능 거리는 `GA_Shared_LockOn` 2000cm가 정함). `VisibilityDistanceHysteresis`(기본 200cm)를 추가해 새로 붙일 때는 `MaxVisibilityDistance - 200` 안쪽이어야 한다.
- 같은 리뷰 후속으로 구간 GE ANS(`UWxAnimNotifyState_ApplyGameplayEffect`)를 B안(자기 핸들만 제거, 몽타주 인스턴스 ID별 핸들 보관)으로 바꿨다(사용자 "네 B로 합시다"). 임시 자동화 `Wx.Combat.ApplyEffectNotify.OwnHandleOnly` 통과 후 테스트 파일은 삭제했다.
- 오래된 CoreRedirects 제거(사용자 "오래된 리디렉터 제거 진행합시다. 에디터 켜져있어요."): `BP_PlayerController`를 다시 저장한 뒤 `[CoreRedirects]` 섹션을 없앴다.

## 사람의 판단 원문

> 사용자 2026-09-24: "코드 품질 개선과 단순화가 목적이긴 해요", "아까 얘기하던 4번 진행하죠".

> 사용자 2026-09-24: "SkeletalMesh 윗면 쓰지 말고 캡슐(Root)의 윗면을 쓰세요"

> 사용자 2026-09-24: "Focus 대상이 아니라, LockOn 대상 아닌가요?" (`FocusQuery` → `LockOnTargetQuery` 이름 변경)

> 사용자 2026-09-24: "HeadClearance = 90은 제가 의도한 것이에요"

- 에이전트 정정(사용자 동의): `State.Engaged`는 유지한다. `RefreshEngagement`가 교전 규칙을 한 곳에서 계산하므로 없애면 NameplateManager와 뒤잡 판정이 규칙을 각자 계산하게 된다.
- 레티클 위치: `UWxLockOnComponent`는 AI·시뮬 프록시에도 있는 복제 모델이라 로컬 표시를 두지 않고 NameplateManager에 둔다.

## LockOnTargetQuery 검토 (4 > 2 > 1 > 3)

후보는 1 현상 유지, 2 WxCore 계약 인터페이스(사용자 처음에 "WxCore에 추가하는 것은 원치 않아요"), 3 WxUI 인터페이스, 4 WxGame 이동이었다. 판정 규칙이 게임 규칙이라 4번이 채택됐다. 상호작용 스캐너가 락온 대상을 읽게 되면 그 소비처를 위해 2번을 더한다는 판단을 남겼다. `UWxAbility_Interact` 이전은 사용자가 "이 부분도 나중에 잘 고민해볼게요"로 보류했다.

## 검증 범위

- AI 빌드: 단계마다 WxEditor Win64 Development 빌드 성공.
- AI BP 컴파일·재로드: 적 BP 5종, `BP_PlayerController`, `WBP_Nameplate_Enemy`, `WBP_LockOnReticle` 등 오류 0. 리다이렉트 0개 상태에서 7개 BP 오류 0·경고 0. 재저장 뒤 `Content`에서 `WxNameplateSourceComponent` 문자열 0건.
- 에셋 재저장 과정에서 맵을 열 때 무관한 9개 패키지(PCG·Landmass·도로 스플라인)가 함께 저장돼 사용자 승인("네")으로 HEAD로 되돌렸다.
- 사람(이우성, 2026-09-25) 인게임 6개 통과: 교전 표시, 락온 표시, 락온 대상 전환, 사망 표시, 크기·위치(3000cm 밖 숨김, 캡슐 윗면 약 90cm 위), 리슨 서버 호스트·원격 클라이언트 구분.

## 미결정·충돌

- 상호작용 대상 락온과 `UWxAbility_Interact` 이전은 사용자가 보류했다. 이전 이유는 아직 듣지 못했다.

## 관련 주제

- [[UI 표시 구조]]
- [[적 AI와 몬스터]]
- [[모듈 구조와 코드 정리]]
- [[결정 노트 - 2026-09-24-nameplate-manager]]
- [[결정 노트 - 2026-09-24-nameplate-manager-wxgame]]
- [[결정 노트 - 2026-09-26-nameplate-play-acceptance]]

## 핵심 주장

- 적 Nameplate와 락온 레티클은 WxGame의 UWxNameplateManagerComponent가 로컬 플레이어 컨트롤러에서 붙이고 떼며, 표시 조건은 IsAlive() && (락온 대상 || (거리 안 && State.Engaged))이다. ^c1
- Nameplate 작업은 State.LockedOn 태그, UWxNameplateSourceComponent, LockOnTargetQuery 델리게이트를 제거했고 State.Engaged 태그는 유지했다. ^c2
- Nameplate 높이는 캡슐 반높이에 HeadClearance를 더한 값이며 헤더 기본값 90은 사용자가 의도한 값이다. ^c3
- Nameplate 작업의 인게임 6개 항목(리슨 서버·원격 클라이언트 구분 포함)은 2026-09-25 이우성이 모두 통과로 확인했다. ^c4
