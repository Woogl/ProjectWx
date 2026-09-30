# 빙의 때 HUD 재생성 순서 계약 해소

상태: 확인 대기 · 착수 대기
다음 행동: 착수를 지시하면 AI가 조사해 질문과 구현 계획을 돌려준다.

- woogle 결정(2026-09-30): "각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠" → UI 코드 점검 결과의 1번을 이 일감으로 둔다.

## 요청

- 요청 · woogle 2026-09-30

> 현재 우리 게임의 UI 코드를 분석하고 원칙이 적절한지, 오히려 제약이 되거나 복잡해지고 있지 않은지 점검해주세요.

> 각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠

## 배경 · 2026-09-30

착수할 때 현재 코드와 다시 대조한다. 아래는 점검 당시의 사실이다.

- 빙의가 바뀌면 `UWxPlayerLayoutComponent`(WxUI)가 HUD를 걷고 다시 띄운다. 필요한 이유는 원칙 6의 "리졸버는 위젯이 뜰 때 한 번만 불린다"이다. 스킬 슬롯·어트리뷰트처럼 키로 받는 자식 VM이 플레이어 Character VM의 AbilitySystem VM 아래에 있어, ASC가 바뀌면 새 AbilitySystem VM 아래에서 다시 받아야 한다.
- 이 흐름은 PC의 VM 재초기화가 HUD 재생성보다 먼저 돈다는 순서에 기댄다. `AWxPlayerController::SetPawn`이 `OnPossessedPawnChanged`보다 먼저 불린다는 엔진 순서이고(`WxPlayerController.cpp:86-88` 주석), 레이아웃 쪽도 같은 전제를 적는다(`WxPlayerLayoutComponent.h:31`). WxGame과 WxUI에 흩어진 암묵 계약이다.
- 이 계약에서 이미 버그가 났다: `702f88c05` 복제 빙의 시 플레이어 VM 갱신 누락.
- `WxPlayerLayoutComponent.cpp`의 `HandlePossessedPawnChanged` 주석 "ViewModel은 생성 당시 Pawn의 ASC를 소유자로 삼으므로"는 재설계 전 이야기다. 지금은 같은 Character VM이 새 ASC로 다시 초기화된다.
- 점검 때 낸 후보(검증 전)
  - (a) HUD를 다시 띄우는 계기를 폰 교체가 아니라 플레이어 Character VM의 AbilitySystem 교체로 바꾼다. 자식을 다시 받아야 하는 바로 그 순간이라 순서 계약이 없어진다. 같은 ASC로 다시 빙의하면 AbilitySystem VM이 유지되므로 HUD도 다시 만들지 않아도 된다.
  - (b) 슬롯·어트리뷰트 VM이 ASC 교체 뒤에도 살아남아 새 ASC에 다시 연결되게 한다. HUD 재생성 자체가 필요 없어지지만 VM 로직이 늘어난다.
- 사망·대화 화면은 폰 ASC 태그 관찰로 띄우며, 이 관찰은 폰 교체를 계속 따라가야 한다.
