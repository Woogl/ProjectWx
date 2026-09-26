---
title: "아이템 VM 단일화와 WxToolset의 enum 변수·MVVM 도구"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, inventory, editor, static-review]
summary: "WxGame 인벤토리 아이템 VM을 WxUI 아이템 VM으로 단일화하고 PC당 공유 인벤토리 VM이 값을 공급하게 한 결정, MVVM 변환 함수 위치 제약, WxToolset의 AddEnumVariable·MVVM 편집 도구 근거. 런타임은 미검증."
revision: 7d2a20408
---

# 아이템 VM 단일화와 WxToolset의 enum 변수·MVVM 도구

2026-09-23 HEAD `7d2a20408` 기준 정적 조사와 사용자 결정 기록이다. 구현 커밋은 `ba396fc16`(VM 단일화), `9b41020cc`(enum 변수·변환 바인딩 도구), `2bfc61535`(MVVM 이벤트 목적지 도구)이다. 작업 상태·빌드·WBP 수정 근거는 [아이템 VM Task](../../../.agents/workflow/tasks/item-viewmodel-unification.md)와 [대화 VM Task](../../../.agents/workflow/tasks/dialogue-presentation-vm.md)에 있다.

## 결정 (사용자 확정)

- WxGame `UWxViewModel_InventoryItem`을 삭제하고 WxUI `UWxViewModel_Item`으로 단일화한다. WxCore는 건드리지 않는다.
- WxUI는 WxInventory에 의존하지 않으므로 `UWxViewModel_Item`은 값을 받기만 하는 VM이다. 인벤토리 관찰과 값 공급은 WxGame `UWxViewModel_Inventory`가 맡는다. Ability System VM → Ability VM 구조와 같다.
- `UWxViewModel_Inventory`는 PC당 공유 합성 VM(`GetOrCreate(PC)`, Outer = PC)이다. 두 리졸버(`UWxViewModelResolver_Inventory`, `UWxViewModelResolver_Item`)가 같은 공유본을 쓰며, 공유본이라 리졸버가 `DestroyInstance`에서 정리하지 않는다.
- 카테고리 선택 상태(`CurrentCategory`)와 거른 목록(`CategorizedItems`)은 공유 인벤토리 VM이 가진다. 인벤토리를 다시 열어도 마지막 탭이 유지된다. 인벤토리 창은 하나뿐이라 선택이 부딪히지 않는다. 변환 함수 라이브러리(`UWxInventoryConversionLibrary`)와 위젯 변수 방식은 더 단순한 이 방식으로 철회했다.
- 퀵슬롯 사용 요청은 Ability VM(`UWxViewModelResolver_Ability`, AbilityTags = `Ability.UseItem`)의 `TryActivateAbility`로 바꿨다. 기존 `RequestUseConsumable`은 `TryActivateAbilitiesByTag(Ability.UseItem)`만 호출했다.
- 어떤 WBP도 바인딩하지 않는 필드(`Grade`, `MaxCharges`, `bIsInventoryAvailable`, VM_Inventory의 `LastChanged*`·`GetCurrencyAmount`)를 제거했다.
- 동작 변화: 획득 토스트 VM은 획득 시점의 값으로 한 번만 채우고, 이후 수량이 바뀌어도 갱신하지 않는다.
- WxToolset 도구(`AddEnumVariable`, `WxMVVMToolset`)는 재사용을 위해 유지한다.

## 확인한 계약

- `UWxViewModel_Item`은 원본 타입을 해석하지 않는다. `SourceObject`·`DisplayName`·`Icon`·`TotalCount`·`CurrentCharges`·`GradeColor`를 setter로 받는다. `AcquiredCount`는 FieldNotify가 없는 1회용 표시 채널이다.
- 아이템 VM의 `SourceObject`는 슬롯 VM이면 ItemInstance, 합계 VM이면 ItemDefinition이다. 합계 VM은 ItemDef를 공유 키로 하며, 인벤토리가 아직 없어도 정적 정보로 만들어지고 인벤토리가 다시 생겨도 같은 VM을 계속 채운다.
- 인벤토리는 클라에서 복제로 위젯보다 늦게 도착할 수 있다. 공유 VM은 교체되지 않고 `OnAnyInventoryReady`·`OnAnyInventoryEnded`로 인벤토리의 등장·제거를 관찰해 내부 연결만 바꾼다.
- 슬롯 수량·충전 변경은 목록 구성을 바꾸지 않으므로 만들어 둔 VM의 값만 다시 채운다. 목록 변경은 `AllItems`와 `CategorizedItems`를 다시 만든다. 카테고리는 SourceObject(Instance)의 Def에서 읽는다.
- `Deinitialize`는 공유본을 쓰는 모든 화면이 사용을 마친 뒤에만 호출한다. 합계 VM은 배열에서 떼기만 한다.

## MVVM 변환 함수의 위치 제약 (엔진 사실)

- MVVM 변환 함수는 위젯 블루프린트 자신의 Pure·const 함수이거나 `UBlueprintFunctionLibrary`의 정적 Pure 함수여야 한다(UE 5.8 `MVVMBlueprintViewConversionFunction.cpp:36-45`). VM 클래스의 정적 함수는 엔진이 거부했다.
- MVVM 암시적 변환기는 enum을 제외한다(`MVVMNumericImplicitConverter.cpp`). enum 값을 숫자로 바꿔 넘기면 표시 VM에 의미 없는 숫자 필드가 생긴다.

## WxToolset 도구

- `UWxBlueprintToolset::AddEnumVariable`: 순정 `BlueprintTools.add_variable`이 기본 타입만 받아 enum 멤버 변수를 만들 수 없어 추가했다. 기본값은 enum 항목 이름이며 비우면 첫 항목이다.
- 변수 FieldNotify: 기존 `SetVariableMeta`에 `{"FieldNotify":""}`를 넘긴다. Set 노드가 `Set with Broadcast`로 바뀌는 것으로 인식을 확인했다.
- `UWxMVVMToolset::SetBindingConversionFunction`: `UMVVMEditorSubsystem`을 감싸 Source→Destination 변환 함수를 지정하고 인자마다 "뷰모델이름.필드" 또는 "Self.필드" 경로를 연결한다. 서브시스템이 함수를 조용히 거부하면 스크립트 에러를 낸다. ArgumentsJson 키가 변환 함수의 입력 파라미터가 아니면 스크립트 에러를 낸다. 엔진 `SetGraphPin`이 없는 핀을 `check`로 받아 에디터가 크래시하던 결함을 코드 리뷰에서 막았다.
- `UWxMVVMToolset::SetEventDestinationWidgetFunction`: MVVM 이벤트의 목적지를 위젯 자신의 BlueprintCallable 함수로 바꾼다. 기존 MCP/Python 프로퍼티 쓰기로는 래퍼 그래프가 갱신되지 않아 `UMVVMEditorSubsystem::SetEventDestinationPath`를 쓴다. `WBP_DialogueScreen`의 진행 이벤트를 `Self.RequestAdvance`로 옮길 때 사용했다.
- 의존성: Build.cs에 BlueprintGraph·ModelViewViewModel·ModelViewViewModelBlueprint·ModelViewViewModelEditor·UMGEditor를, uplugin에 ModelViewViewModel을 추가했다.
- WxToolset 모듈은 AnimMontage·Blueprint·MVVM·StateTree 네 도구 클래스를 시작 시 등록하고 종료 시 해제한다.

## 검증 범위

- Task 기록 기준 Development·DebugGame 빌드와 관련 위젯 6개(Inventory, ItemSlot, TotalGold, AcquiredItemEntry, ItemQuickSlot, AcquiredItemList) 컴파일이 성공했다. Content 전체에서 `WxViewModel_InventoryItem` 참조는 0건이며 임시 ClassRedirect는 제거했다.
- 런타임은 검증하지 않았다(PIE 맵이 FrontEnd라 HUD 미생성). 퀵슬롯 표시·사용, 골드 갱신, 획득 토스트, 탭 필터링·유지, 리스폰 후 동작은 인간 확인 대상이다.

## 코드 근거

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [WxViewModel_Item.h](../../../Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Item.h) | 수동 공급 VM 필드·setter | `fe3665deec96410b61d0599f37d490836e169ebc3fb98f218acb35a96a699367` |
| [WxViewModel_Item.cpp](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Item.cpp) | setter·이미지 로드 | `37bec4b36d01f6d7f8ce6d3a94f4e2f194814b9304de057f8dc7f482e78bd87c` |
| [WxViewModel_Inventory.h](../../../Source/WxGame/MVVM/WxViewModel_Inventory.h) | 공유 VM·카테고리·리졸버 계약 | `8e6e3bb648af6636132166121cdf8adaa8a3a11018a0f0dcbfbeb89d1350e297` |
| [WxViewModel_Inventory.cpp](../../../Source/WxGame/MVVM/WxViewModel_Inventory.cpp) | `GetOrCreate`·인벤토리 관찰·값 공급 | `093991b7b40ed4e400bd968405e64a0a6b4ae364595cf20fc02cab69765dde85` |
| [WxMVVMToolset.h](../../../Plugins/WxToolset/Source/WxToolset/Private/WxMVVMToolset.h) | 세 MVVM 편집 함수 계약 | `cd740f3a8b729bd713e5821938246c4df34a7671a8a5df93a5339a5d00d96810` |
| [WxMVVMToolset.cpp](../../../Plugins/WxToolset/Source/WxToolset/Private/WxMVVMToolset.cpp) | 이벤트 목적지·인자 검증 | `7f57d0090d376e616993423cb310bb51055913ea4dd11b6a2f1fe9131313344a` |
| [WxBlueprintToolset.h](../../../Plugins/WxToolset/Source/WxToolset/Private/WxBlueprintToolset.h) | `AddEnumVariable` 계약 | `8842e0cc485bf0e051a739901fab10b170c96631d49f7924e91c8e70f11cd17e` |
| [WxToolsetModule.cpp](../../../Plugins/WxToolset/Source/WxToolset/Private/WxToolsetModule.cpp) | 도구 클래스 등록·해제 | `1d96f29dce6346a0730a6e73b97150a05db2f4bf42951ba398b629f1607849e0` |
| [WxToolset.uplugin](../../../Plugins/WxToolset/WxToolset.uplugin) | 플러그인 의존성 | `2f111b54e973d72feb4c57d3266da7ecaf9291c7e25b3bf9c2156bd8d479650d` |
