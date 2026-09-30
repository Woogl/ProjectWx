# 아이템 카테고리를 게임플레이 태그 한 벌로

상태: 확인 대기 · 체크리스트 4/5 통과
다음 행동: 코드 리뷰를 확인한다.

- woogle 결정(2026-09-30): "각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠" → UI 코드 점검 결과의 2번(아이템 카테고리 이중 정의와 PC의 익명 namespace)을 바로 해결한다. 지시가 구현 승인이다.

## 요청

- 요청 · woogle 2026-09-30

> 현재 우리 게임의 UI 코드를 분석하고 원칙이 적절한지, 오히려 제약이 되거나 복잡해지고 있지 않은지 점검해주세요.

> 각각을 별도 일김으로 만듭시다. 2는 지금 바로 작업해서 해결하죠

## 질문

추가 질문 없음.

## 구현 계획

구현 승인: woogle 2026-09-30

**문제**

- 아이템 분류가 두 벌이다. WxInventory의 `EWxItemCategory` enum(아이템 정의 `Category` 필드)과 WxCore의 `Item.Category.*` 태그(인벤토리 VM 탭 키)가 같은 뜻이고, PC가 익명 namespace의 switch로 enum을 태그로 바꾼다(`WxPlayerController.cpp:24-40`). 분류를 하나 늘리려면 enum·태그·switch·탭 네 곳을 고쳐야 한다.
- 익명 namespace는 프로젝트 규칙(호출부에 인라인)에도 어긋난다.

**변경**

1. `UWxItemDefinition`(WxInventory)
   - `Category`의 타입을 `FGameplayTag`로 바꾸고 `meta = (Categories = "Item.Category")`로 고르게 한다. `GetItemCategory()`는 태그를 돌려준다.
   - `EWxItemCategory` enum과 그것만 초기화하던 생성자를 지운다. enum을 쓰는 곳은 아이템 정의와 PC 변환뿐이다(코드 3개 파일, 에셋은 아이템 정의 3개).
2. `AWxPlayerController`: 익명 namespace의 변환을 지우고 `SetCategory(ItemDef->GetItemCategory())`로 바로 넣는다.
3. `WxGameplayTags.h`의 `Item_Category_*` 주석을 "아이템 정의의 분류, 인벤토리 VM 탭 키로도 쓴다"로 고친다.
4. 에셋: 타입이 바뀌면 기존 값이 로드 때 버려지므로, 바꾸기 전에 읽어 둔 값을 헤드리스 Python으로 다시 써 넣고 저장한다. DA_Gold=Item.Category.Currency, DA_Katana=Item.Category.Equipment, DA_Potion=Item.Category.Consumable(현재 빌드로 읽은 값).

**검증**

- 빌드: WxEditor DebugGame·Development(구현 뒤, 임시 테스트 삭제 뒤).
- 에셋: 저장 뒤 새 프로세스로 세 정의의 태그를 다시 읽고, 로드 로그에 타입 불일치 경고가 없는지 본다.
- 헤드리스 임시 자동화 테스트(LV_DevCombat -game -nullrhi): 세 정의를 지급한 뒤 인벤토리 VM의 아이템 VM마다 Category가 그 정의의 태그와 같고, 탭 셋마다 CategorizedItems가 그 분류만 담는다.
- 코드 확인(grep): `EWxItemCategory` 0건, 코드의 익명 namespace 변환 제거.

**테스트 체크리스트 초안**

| 항목 | 확인 방법 | 담당 |
| --- | --- | --- |
| 빌드 | WxEditor DebugGame·Development | AI |
| 에셋 값 이관 | 헤드리스 Python으로 세 정의의 태그를 다시 읽고 로드 경고 확인 | AI |
| 인벤토리 탭 | 임시 자동화 테스트: 아이템 VM 분류와 탭별 목록 | AI |
| 코드 확인 | grep: EWxItemCategory 0건 | AI |
| 코드 리뷰 | 변경 파일과 볼 점 | 사람 |

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 빌드 | WxEditor DebugGame·Development(구현 뒤, 임시 테스트 삭제 뒤) | AI | 통과 | 구현 뒤 두 구성 모두 Result: Succeeded. 임시 테스트를 지운 뒤 두 구성 다시 Succeeded |
| 에셋 값 이관 | 헤드리스 Python으로 세 정의의 태그를 다시 읽고 로드 경고 확인 | AI | 통과 | 바꾸기 전 빌드로 읽은 값은 DA_Gold=CURRENCY, DA_Katana=EQUIPMENT, DA_Potion=CONSUMABLE. 새 빌드로 태그를 써 넣고 저장했다(이 로드에서만 옛 enum 값의 타입 불일치 경고 3건). 새 프로세스로 다시 읽으면 Item.Category.Currency·Equipment·Consumable이고 타입 불일치 경고 0건 |
| 인벤토리 탭 | 임시 자동화 테스트: 아이템 VM 분류와 탭별 목록 | AI | 통과 | 헤드리스 게임(LV_DevCombat -game -nullrhi) 임시 테스트 Wx.Temp.ItemCategory 성공. Katana·Gold·Potion을 지급한 뒤 슬롯 VM 4개의 분류가 모두 정의의 태그와 같고, 탭별 표시 수가 장비 1·소모품 2·재화 1로 기대값과 같으며 다른 분류가 섞이지 않았다. 결과 기록 뒤 임시 테스트를 지웠다 |
| 코드 확인 | grep: EWxItemCategory 0건 | AI | 통과 | 코드·에셋 모두 EWxItemCategory 0건, WxPlayerController.cpp의 익명 namespace 0건 |
| 코드 리뷰 | 변경 파일: WxItemDefinition.h/.cpp(Category를 FGameplayTag로·Categories 메타·enum과 생성자 삭제), WxPlayerController.cpp(익명 namespace 변환과 쓰지 않게 된 WxGameplayTags include 삭제, 분류를 그대로 넣음), WxGameplayTags.h(Item_Category 주석). 에셋: DA_Gold·DA_Katana·DA_Potion의 Category를 enum에서 같은 뜻의 태그로 다시 저장. 볼 점: 도메인(WxInventory)이 WxCore 태그를 분류로 쓰는 것이 괜찮은지 | 사람 | 대기 |  |

## 구현 진행 · 2026-09-30

- 계획대로 구현했다. 변환 함수를 지우자 PC에서 `WxGameplayTags.h`를 쓰는 곳이 없어져 include도 지웠다.
- 아이템 정의 에셋은 AssetRegistry 기준으로 세 개뿐이다(분류가 None인 정의는 없었다).

## 배경 · 2026-09-30

- UI 코드 점검(대화)에서 원칙 2(뷰모델은 도메인 타입 금지)의 비용 중 "규칙 두 벌"로 짚은 항목이다. 같은 점검의 다른 항목은 [빙의 때 HUD 재생성 순서 계약 해소](possession-hud-rebuild-contract.md), [UI 설계 원칙 정리](ui-principles-trim.md), [목록 항목 WBP를 ListView 항목 뷰모델 확장으로 전환](list-entry-viewmodel-extension.md)이다.
- 태그는 2026-09-30 [MVVM 원칙에 맞춘 뷰모델 재설계](viewmodel-mvvm-redesign.md) Q6에서 인벤토리 탭 키로 만들었다("태그로 합시다"). 이번 변경은 도메인도 같은 태그를 쓰게 해 enum을 없앤다.
