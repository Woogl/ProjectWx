---
title: "퀘스트 전환 예약과 실행 수명의 분리"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, quests, statetree, static-review]
summary: "다음 틱 퀘스트 활성화 예약은 이전 StateTree 실행이 끝나거나 새 수주가 시작되어도 자동 취소되지 않는다."
---

# 퀘스트 전환 예약과 실행 수명의 분리

사용자 지정 커밋 `39f3629a4`의 WxQuest 정적 리뷰 근거다. 담당 서브에이전트가 프로젝트 cpp와 UE 5.8의 StateTreeComponent·StateTreeExecutionContext·TimerManager 구현을 대조했다. 수정 판단은 [WxQuest 리뷰](../../../.agents/workflow/tasks/module_review_WxQuest.md)에 유지한다. 빌드·PIE·실제 레벨에서의 발생 빈도는 확인하지 않았다.

[WxQuestComponent](../../../Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp)의 `RequestActivateQuest`(47행)는 TimerManager의 다음 틱 콜백을 예약하며 핸들이나 예약 당시 퀘스트 실행 식별자를 보관하지 않는다. `ActivateQuest`(34행)는 러너를 교체하지만 그 예약을 취소하지 않는다. 콜백(132행)은 실행 당시 컴포넌트에서 새 퀘스트를 활성화한다.

따라서 A가 B로 넘어가도록 예약한 후 콜백 전에 C를 새로 수주하면, 남아 있던 예약이 C를 중단하고 B를 시작할 수 있다. 다음 틱 예약은 같은 StateTree 호출 스택 안의 교체를 피하지만, 이전 실행의 요청이 새 실행을 덮지 않도록 보장하는 장치는 아니다. 단일 러너·저널 미복제라는 기존 설계 전제와는 별개의 수명 조건이다.
