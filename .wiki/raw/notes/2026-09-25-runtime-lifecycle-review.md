---
title: "모듈 리뷰로 확인한 캐릭터·화면 수명 설명의 범위"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, game, ui, static-review]
summary: "39f3629a4의 캐릭터 재등록 시 어빌리티 소실과 중복 구독 조건, 새 게임 선택 검사, 폰 교체 때 화면 요청 정리 범위를 구분한다."
---

# 모듈 리뷰로 확인한 캐릭터·화면 수명 설명의 범위

사용자가 지정한 기준은 `39f3629a4`이다. 모듈별 리뷰 담당 서브에이전트가 프로젝트 핵심 cpp와 필요한 로컬 UE 5.8 소스를 읽어 대조한 정적 관찰이다. 검토 중 다른 작업에서 발생한 미커밋 소스 변경은 제외한다. 빌드·플레이·에셋 배치 재현을 수행하지 않았다. 수정 판단과 전체 지적은 [WxGame 리뷰](../../../.agents/workflow/tasks/module_review_WxGame.md)와 [WxUI 리뷰](../../../.agents/workflow/tasks/module_review_WxUI.md)에 유지한다.

## 동일 캐릭터의 재등록

- 범위는 레벨을 로드된 채 유지하고 가시성만 껐다 켜 같은 캐릭터·ASC를 재사용하는 경우다. 언로드·GC 후 새 객체를 만드는 경로와 구분한다. UE 5.8 `LevelStreaming.cpp:744`·`:1138`의 LoadedNotVisible 경로가 전제다.
- [WxCharacterBase](../../../Source/WxGame/Character/WxCharacterBase.cpp) 210행의 초기화는 [WxCombat ASC](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp) 49행의 `bAbilitySetsGranted` 가드로 재부여를 생략한다. 하지만 엔진 ASC의 `OnUnregister` → `DestroyActiveState`는 권위 측 스펙을 `ClearAllAbilities`로 제거한다. 근거는 UE 5.8 `AbilitySystemComponent.cpp:236`, `AbilitySystemComponent_Abilities.cpp:109`·`:1386`이다.
- 캐릭터의 `PostInitializeComponents` 58·63행은 사망·래그돌 태그 콜백을 추가한다. UE 5.8 `Actor.cpp:3238`·`Level.cpp:3876`의 재초기화에서 다시 등록되며 기존 태그 델리게이트는 남는다.
- 따라서 구독 누적 자체는 확인되지만, 일반 공격 후 보상 중복이 곧바로 드러난다고 보장하지 않는다. 사망 어빌리티도 먼저 소실되므로 태그가 나중에 다시 들어오거나 스펙 복원 후 해당 콜백이 호출되는 조건이 필요하다.

## 새 게임의 선택 검사

[WxGameFlowSubsystem](../../../Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp) 43·50행의 캐릭터 선택 검사는 소프트 경로의 null 여부다. 체크포인트 삭제 전에 클래스 로드·생성 가능 여부를 검증한 것으로 해석하지 않는다. 실제 클래스 로드는 79행의 목적지 조회에서 수행한다. 현재 설정 에셋이 잘못되었다는 관찰은 아니다.

## 폰 교체의 비동기 화면 요청

[PlayerLayoutComponent](../../../Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp) 90·99·102행은 직접 소유한 HUD 생성 요청과 기존 HUD를 정리한다. 이 범위를 HUD의 모든 하위 요청으로 확대할 수 없다.

[HUDLayout](../../../Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp) 87·95행의 `PendingMenuPush`에는 제거 시 취소 경로가 없다. [AsyncAction](../../../Plugins/WxUI/Source/WxUI/Private/Widget/WxAsyncAction_PushWidgetToLayer.cpp) 18행에서 GameInstance에 등록된 요청이 이전 HUD를 보유하며, 같은 PC의 폰 교체는 100행의 PrimaryGameLayout 일치 검사를 통과할 수 있다. 실제 비동기 로드를 지연시킨 재현은 하지 않았다.
