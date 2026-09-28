---
type: source
title: "작업 - item-viewmodel-unification"
created: 2026-09-28
updated: 2026-09-28
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "인벤토리"
summary: "WxGame 인벤토리 아이템 VM을 WxUI 아이템 VM으로 단일화한 작업의 완료 기록으로, 2026-09-27 헤드리스 표시 테스트와 2026-09-28 사람 확인으로 체크리스트 7/7 통과"
source_type: task-record
source_id: src-8019eb1136705395467d
sha256: 74f0285e53158c77bbbbf5ef602db33f923edb3a2a20506f2f11344286eca208
authority: primary
independence_key: ".agents/workflow/tasks/item-viewmodel-unification.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/item-viewmodel-unification.md"
raw_copy: ".raw/captured/74f0285e53158c77bbbbf5ef602db33f923edb3a2a20506f2f11344286eca208.md"
claim_ids:
  - clm-74f0285e53-c1
  - clm-74f0285e53-c2
  - clm-74f0285e53-c3
key_claims:
  - "2026-09-23 확정 설계로 WxGame UWxViewModel_InventoryItem을 지우고 값을 받기만 하는 WxUI UWxViewModel_Item으로 단일화했으며, PC당 공유 UWxViewModel_Inventory가 값을 공급하고 카테고리 선택(CurrentCategory·CategorizedItems)을 가져 인벤토리를 다시 열어도 마지막 탭이 유지된다."
  - "2026-09-27 헤드리스 임시 자동화 테스트로 퀵슬롯 사용(충전 3→2·아이콘 갱신), 골드 갱신, 획득 토스트, 인벤토리 탭 필터·다시 열기 유지, 리스폰 뒤 유지를 AI가 확인했고 woogle이 2026-09-28 화면 모양과 코드 리뷰를 통과시켰다."
  - "게임 시작 때 WBP_AcquiredItemList가 null LastAcquiredItem을 검사하지 않아 \"Cannot add null item into ListView\" 경고를 한 번 남기며, 동작 영향이 없어 고치지 않았다."
---

# 작업 - item-viewmodel-unification

- 원본: `.agents/workflow/tasks/item-viewmodel-unification.md`
- 원자료 사본: `.raw/captured/74f0285e53158c77bbbbf5ef602db33f923edb3a2a20506f2f11344286eca208.md`
- 수집: 2026-09-28 UTC · 재확인 기한: 2027-03-28

## 개요

WxGame `UWxViewModel_InventoryItem`을 제거하고 WxUI `UWxViewModel_Item`으로 단일화한 2026-09-23 작업의 완료 기록이다. 설계·구현 이력은 [[결정 노트 - 2026-09-23-item-viewmodel-unification]]에도 있고, 이 기록은 그 뒤 2026-09-27 헤드리스 테스트와 2026-09-28 사람 확인까지 담는다. 상태는 완료(체크리스트 7/7 통과)다.

## 확정 설계(2026-09-23)

- WxUI는 WxInventory에 의존하지 않으므로 `UWxViewModel_Item`은 값을 받기만 하고, 인벤토리 관찰과 값 공급은 WxGame `UWxViewModel_Inventory`가 맡는다. `UWxViewModel_Inventory`는 PC당 공유 합성 VM(`GetOrCreate(PC)`)이라 `DestroyInstance`에서 정리하지 않는다.
- 퀵슬롯 사용 요청은 VM_Ability(`Ability.UseItem`)의 `TryActivateAbility`로 바꿨다.
- 카테고리 최종 결정: 사용자가 변환 함수 라이브러리보다 단순한 방법을 원해, 선택 탭(`CurrentCategory`)과 거른 목록(`CategorizedItems`)을 공유 VM_Inventory로 되돌렸다. 인벤토리 창은 하나뿐이라 선택이 부딪히지 않고, 대가로 다시 열어도 마지막 탭이 유지된다. `UWxInventoryConversionLibrary`는 지웠고 WxToolset 도구(`AddEnumVariable`, `WxMVVMToolset`)는 재사용을 위해 유지한다(사용자).
- `UWxViewModel_Item`의 `Deinitialize`·`Initialize`는 베이스 계약과 어긋나거나 호출처가 없어 지웠다. 획득 토스트 VM은 획득 시점 값으로 한 번만 채운다.

## 헤드리스 테스트(2026-09-27)

- LV_DevCombat `-game -nullrhi`에서 임시 자동화 테스트로 CommonUI 실제 클릭 처리기(`HandleButtonClicked`)를 눌렀다. 퀵슬롯 클릭으로 Ability.UseItem이 발동해 사용 몽타주 노티파이 시점에 포션 충전이 3→2, 아이콘이 potion_two_third로 바뀌었다. 골드 표시는 325→335, 골드 25 획득 토스트가 이름·아이콘·수로 채워져 HUD 목록에 들어갔다.
- 인벤토리 탭 3개가 목록을 걸렀고(Consumable 포션·Equipment 카타나·Currency 골드), 닫았다 다시 열면 마지막 탭이 유지됐다. 치트 사망 뒤 부활해도 새 HUD에서 같은 동작이 유지됐다.
- 확인한 사실: HUD의 인벤토리 액션 위젯(`InventoryWidgetClass`)은 비어 있고 인벤토리는 메인 메뉴 Inventory 버튼으로 연다. 인벤토리를 닫으면 메인 메뉴로 돌아오고 메인 메뉴까지 닫아야 일시정지가 풀린다. 부활하면 HUD가 새로 만들어지고 퀵슬롯 사용 VM은 새 폰 ASC에 묶인다.
- 남은 경고: `WBP_AcquiredItemList`의 "Cannot add null item into ListView" 스크립트 경고(첫 뷰 초기화의 null `LastAcquiredItem`)는 동작 영향이 없어 고치지 않았다. 고치려면 블루프린트 그래프에 유효성 검사를 넣어야 한다.

## 검증 범위

- AI: 2026-09-23 Development·DebugGame 빌드와 위젯 6개 컴파일, 2026-09-27 헤드리스 표시 테스트(목록 항목 위젯은 헤드리스에서 생성되지 않아 모양은 사람 항목).
- 사람: woogle 2026-09-28 화면 모양·코드 리뷰 통과.

## 관련 주제

- [[UI 표시 구조]]
- [[아이템과 회복]]
- [[에디터 도구]]

## 핵심 주장

- 2026-09-23 확정 설계로 WxGame UWxViewModel_InventoryItem을 지우고 값을 받기만 하는 WxUI UWxViewModel_Item으로 단일화했으며, PC당 공유 UWxViewModel_Inventory가 값을 공급하고 카테고리 선택(CurrentCategory·CategorizedItems)을 가져 인벤토리를 다시 열어도 마지막 탭이 유지된다. ^c1
- 2026-09-27 헤드리스 임시 자동화 테스트로 퀵슬롯 사용(충전 3→2·아이콘 갱신), 골드 갱신, 획득 토스트, 인벤토리 탭 필터·다시 열기 유지, 리스폰 뒤 유지를 AI가 확인했고 woogle이 2026-09-28 화면 모양과 코드 리뷰를 통과시켰다. ^c2
- 게임 시작 때 WBP_AcquiredItemList가 null LastAcquiredItem을 검사하지 않아 "Cannot add null item into ListView" 경고를 한 번 남기며, 동작 영향이 없어 고치지 않았다. ^c3
