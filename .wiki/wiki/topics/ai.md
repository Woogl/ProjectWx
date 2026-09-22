---
title: "WxAI — AI 인지와 행동"
category: topic
sources:
  - "raw/notes/2026-09-22-current-ai.md"
  - "raw/notes/2026-09-22-current-foundation.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, ai]
aliases: ["WxAI"]
confidence: medium
volatility: warm
verified: 2026-09-22
summary: "WxAI는 인지 결과를 Blackboard로 전달하고 Behavior Tree 노드로 이동·전투 행동을 구성한다."
---

# WxAI — AI 인지와 행동

WxAI는 인지 결과를 Blackboard로 전달하고 Behavior Tree 노드로 이동·전투 행동을 구성한다.

## 책임과 조립 경계

WxAI는 AIBehavior 설정·Blackboard 키·BT 서비스/태스크/데코레이터를 제공한다. 실제 AIController는 WxGame에 있다. WxAI의 Build.cs는 GAS와 WxCore에 의존하며 WxCombat을 직접 참조하지 않는다. 전투 속성이나 이동 효과를 요구하는 BT 노드에는 에셋에서 적절한 데이터가 지정되어야 한다.

## 인지에서 행동까지

`UWxAIBehaviorComponent::InitializeComponent`는 빙의가 Pawn Owner를 컨트롤러로 바꾸기 전에 스폰 주체에서 순찰 컴포넌트를 확보한다. BeginPlay에서는 현재 컨트롤러와 이후 교체에 감각 설정을 적용한다. 피해 이벤트는 양수 피해와 적대 관계를 확인해 Damage 센스에 보고한다.

`UpdateTargetActor`는 현재 타겟이 유효하고 사망·IgnoreAggro 상태가 아니면 유지한다. 교체가 필요하면 이전 타겟의 인지 기록을 지운 뒤 감지 목록에서 유효 후보를 찾는다. 이 코드는 가장 가까운 적을 점수화하는 선정기가 아니며, 반환된 감지 목록의 첫 허용 후보를 사용한다.

Blackboard의 SelfActor·HomeLocation·Master와 TargetActor·TargetDistance·PatrolTargetLocation은 공용 키 계약이다. 타겟 부재의 거리를 0으로 초기화하면 근접 조건을 통과할 수 있어 `NoTargetDistance`를 사용한다. 키 이름·타입은 실제 Blackboard 에셋과 맞아야 한다.

## 락온 수명

`UWxBTService_LockOn`은 활성 진입과 틱에 Gameplay 우선순위 포커스를 맞추고 폰의 회전 모드를 설정한다. 분기 이탈 시 포커스를 해제하고 폰의 Movement 아키타입 기본값으로 복구한다. 빙의 해제 뒤에도 이전 폰을 정리할 수 있도록 노드 메모리에 폰을 보관한다. 검색 중 임시 활성화가 아니라 `OnBecomeRelevant`에서 적용하는 이유는 정리 콜백과 수명을 맞추기 위해서다.

## 확장과 확인 범위

[Blackboard 계약](../../../Plugins/WxAI/Source/WxAI/Public/WxBlackboardKeys.h), [타겟 서비스](../../../Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp), [AIController](../../../Source/WxGame/Controller/WxAIController.cpp) 순서로 추적한다. 순찰·배회·어빌리티 발동·소환수 추종 노드도 제공하지만, 개별 BT 조합과 소환수 미러링 전체 실행은 이번 핵심 경로 조사로 보증하지 않는다.

## 관련 문서

- [[combat-abilities|전투 어빌리티와 이펙트]] ([전투 어빌리티와 이펙트](../concepts/combat-abilities.md))
- [[combat-groggy|그로기]] ([그로기](../concepts/combat-groggy.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-ai.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
