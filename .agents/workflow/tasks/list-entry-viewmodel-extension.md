# 목록 항목 WBP를 ListView 항목 뷰모델 확장으로 전환

상태: 확인 대기 · 착수 대기
다음 행동: 착수를 지시하면 AI가 조사해 질문과 구현 계획을 돌려준다.

- woogle 결정(2026-09-30): "각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠" → UI 코드 점검 결과의 4번(상호작용 목록 일감 Q1의 후속)을 이 일감으로 둔다.

## 요청

- 요청 · woogle 2026-09-30

> 현재 우리 게임의 UI 코드를 분석하고 원칙이 적절한지, 오히려 제약이 되거나 복잡해지고 있지 않은지 점검해주세요.

> 각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠

## 배경 · 2026-09-30

착수할 때 현재 코드와 다시 대조한다. 자세한 조사는 [상호작용 목록 뷰모델을 UI 설계 원칙에 맞추기](interaction-list-vm-principles.md)의 Q1과 조사 절에 있다.

- 원칙 6은 목록 항목 VM을 ListView·패널의 항목 뷰모델 확장으로 넘기라고 한다. 지금은 항목 WBP 5개가 `OnListItemObjectSet` → Cast → `Set<VM>` BP 노드로 받는다: WBP_AcquiredItemEntry, WBP_Effect, WBP_Interaction, WBP_ItemSlot, WBP_QuestObjective. 확장을 쓰는 에셋은 없다.
- 엔진 확장의 대가: 항목 VM을 행이 만들어진 프레임이 그려진 뒤에 넣는다(`ListViewBase.cpp:320-345` 코어 티커 공지, 코어 티커는 Slate 그리기 뒤 `LaunchEngineLoop.cpp:5991`·`6103`). 새 행은 한 프레임 동안 디자이너 기본값이나 재사용된 이전 행 값으로 그려진다. 상호작용 목록은 선택이 바뀔 때마다 행을 다시 만든다. 반면 BP 연결은 행을 만들 때 바로 불린다(`SObjectTableRow.h:856-861`).
- 확장의 항목 VM 지정 필드 `EntryViewModelId`는 편집 지정자 없는 private `UPROPERTY()`라, 붙이려면 WxMVVMToolset에 함수가 필요하다(`CreateBlueprintWidgetExtension` + 리플렉션).
- BP 연결은 Cast 실패 핀이 비어 실패해도 로그가 없다(원칙 6 "Manual로 넣는 쪽은 실패를 로그로 남긴다"). 엔진 확장은 실패를 로그로 남긴다.
- WBP_Interaction을 고칠 때 같이 할 정리: `bSelected` 바인딩을 OneTime으로(행 VM이 불변), `Image_89`를 뜻이 드러나는 이름으로, 비활성 기본 이벤트 노드 세 개 삭제. 문구 바인딩은 이미 OneTime이며, 엔진이 VM을 새로 넣을 때 OneTime까지 다시 실행하므로(`MVVMView.cpp:1077-1078`) 풀 재사용에도 문제없다.
