# WxQuest — 코드 리뷰

> 단일 권위 러너와 목표 핸들의 책임은 명확하지만, 퀘스트 교체 시 이전 실행이 남긴 예약을 무효화하지 않아 새 퀘스트가 덮일 수 있다. `39f3629a4`의 러너 교체·예약 실행·목표 등록과 제거·저널 종료 통지 및 네 가지 StateTree 태스크를 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 이전 퀘스트의 활성화 예약이 새 수주를 덮는다

- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:47`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:34`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp:132`
- **범주**: 버그/정확성
- **문제**: `RequestActivateQuest`는 다음 퀘스트를 독립 타이머로 예약하고 핸들을 버린다. 예약을 만든 실행의 식별자도 저장하지 않는다. 퀘스트 A의 `StartNextQuest`가 B를 예약한 뒤, 타이머 실행 전에 트리거의 `StartQuest(C)`가 새 퀘스트 C를 시작하면 A의 예약이 남는다. 이후 콜백이 무조건 `ActivateQuest(B)`를 호출하여 C를 중단하고 B로 교체한다. 러너를 `StopLogic`으로 정지해도 컴포넌트에 바인딩된 TimerManager 예약은 취소되지 않는다. C의 진입 효과는 이미 실행됐는데 진행은 B로 넘어가는 불일치가 생길 수 있다.
- **제안**: 예약 타이머 핸들과 실행 세대를 보관하고, 다른 퀘스트의 명시적 활성화 때 이전 예약을 취소하거나 세대를 무효화한다. 지연 콜백도 예약 당시 세대가 유효한지 확인한다. 정상 완료 후 다음 퀘스트로 이어지는 예약은 유지해야 하므로 모든 종료 통지에서 일괄 취소하지 않는다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestComponent.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestComponent.h`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxQuestLibrary.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestObjective.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_SetQuestTitle.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_StartNextQuest.cpp`, `Plugins/WxQuest/Source/WxQuest/Private/Quest/WxStateTreeTask_WaitMoveToTarget.cpp`와 각 태스크 공개 헤더.
- **훑은 파일**: `Plugins/WxQuest/Source/WxQuest/Public/Quest/WxQuestLibrary.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`. 저널 소비자는 같은 기준 커밋의 `Source/WxGame/MVVM/WxViewModelResolver_Quest.cpp`와 공개 헤더로 대조했다. UE 5.8의 `StateTreeComponent.cpp`, `StateTreeExecutionContext.cpp`, `TimerManager.cpp`로 러너 재진입 제한·상태 재선택·실행 상태 통지·예약 수명을 확인했다.
- **미검토 / 한계**: 빌드·PIE·BP/WBP 및 StateTree 바이너리 내부는 검증하지 않았다. 예약 충돌은 C++ 호출 순서상 확인한 실패 경로이며 실제 레벨에서의 발생 빈도는 확인하지 않았다. 싱글플레이/리슨 호스트 전제, 단일 퀘스트 러너, 저널 미복제·미저장은 명시된 설계로 보고 결함에 포함하지 않았다. 기준 커밋 이후 변경은 포함하지 않는다.

---
*문서 기준 커밋 `39f3629a4` · 리뷰일 2026-09-25 · 소스 14파일 — `/module-review`로 갱신*
