# UI 설계 원칙 정리

상태: 확인 대기 · 착수 대기
다음 행동: 착수를 지시하면 AI가 조사해 질문과 구현 계획을 돌려준다.

- woogle 결정(2026-09-30): "각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠" → UI 코드 점검 결과의 3번을 이 일감으로 둔다.

## 요청

- 요청 · woogle 2026-09-30

> 현재 우리 게임의 UI 코드를 분석하고 원칙이 적절한지, 오히려 제약이 되거나 복잡해지고 있지 않은지 점검해주세요.

> 각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠

## 배경 · 2026-09-30

착수할 때 현재 코드와 다시 대조한다. 아래는 점검 당시의 사실이다.

- 원칙 전문은 [MVVM 원칙에 맞춘 뷰모델 재설계](viewmodel-mvvm-redesign.md) 구현 계획 절 맨 앞에 있다. 완료된 일감 기록 안이라 원칙을 고칠 곳(정본 위치)부터 정해야 한다.
- 원칙 6(뷰모델 받는 법)이 실제보다 크다. 뷰모델을 쓰는 WBP 22개의 생성 방식은 Resolver 13, Manual 9(목록 항목 5는 BP 연결, 자식 위젯 3, 월드 공간 2)이고, Context·PropertyPath·Global Collection 생성 방식과 자동 갱신 플래그는 0건이다. 실제 규칙은 "루트는 리졸버, 자식 위젯·목록 항목·월드 공간은 Manual"로 더 단순하다.
- 원칙이 다루지 않는 빈틈
  - 일회성 이벤트: 획득 토스트를 `UWxViewModel_Inventory::LastAcquiredItem` 상태로 흘려보내, 첫 초기화 때 null이 들어가 "Cannot add null item into ListView" 경고가 난다(기존 경고의 원인).
  - 게임 흐름이 거는 연출 UI: 자막(StateTree 노드가 VM 직접 호출)은 알려진 예외로 적혀 있지만, 인디케이터(StateTree 태스크 `FWxStateTreeTask_MarkIndicator`가 UI 액터를 스폰)는 같은 부류인데 예외 목록에 없다.
  - 화면 흐름: 어떤 화면을 언제 띄우는지(`UWxPlayerLayoutComponent`가 ASC 태그로 사망·대화 화면을 띄움)가 원칙의 층 구분 밖에 있다.
- 부수: 자막 VM이 Global Collection 조회 코드를 따로 갖고 있다(`UWxViewModel_Subtitle::GetOrCreate`, `WxViewModel::GetGlobalCollection` 미사용).
