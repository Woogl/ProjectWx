---
type: source
title: "결정 노트 - 2026-09-23-item-viewmodel-unification"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "UI"
  - "MVVM"
  - "인벤토리"
summary: "WxGame 인벤토리 아이템 VM을 WxUI 아이템 VM으로 단일화하고 PC당 공유 인벤토리 VM이 값을 공급하게 한 결정, MVVM 변환 함수 제약, WxToolset 도구 기록."
source_type: decision-note
source_id: src-b8abe92095b411c6840f
sha256: e40c8a9d21db2023e89b8f5f920d52e5fa062932cad545c583e286d8d2115910
authority: primary
independence_key: ".wiki/raw/notes/2026-09-23-item-viewmodel-unification.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-23-item-viewmodel-unification.md"
raw_copy: ".raw/captured/e40c8a9d21db2023e89b8f5f920d52e5fa062932cad545c583e286d8d2115910.md"
claim_ids:
  - clm-af54e35eb6-c1
  - clm-af54e35eb6-c2
  - clm-af54e35eb6-c3
  - clm-af54e35eb6-c4
key_claims:
  - "사용자는 WxGame UWxViewModel_InventoryItem을 삭제하고 WxUI UWxViewModel_Item으로 단일화하기로 확정했다."
  - "UWxViewModel_Inventory는 PC당 공유 합성 VM이며 인벤토리·아이템 두 리졸버가 같은 공유본을 쓰고 DestroyInstance에서 정리하지 않는다."
  - "UE 5.8 MVVM 변환 함수는 위젯 블루프린트의 Pure·const 함수나 BlueprintFunctionLibrary의 정적 Pure 함수여야 하며 VM 클래스의 정적 함수는 거부된다."
  - "아이템 VM 단일화의 런타임 동작은 노트 시점에 검증되지 않았다."
---

# 결정 노트 - 2026-09-23-item-viewmodel-unification

- 원본: `.wiki/raw/notes/2026-09-23-item-viewmodel-unification.md`
- 원자료 사본: `.raw/captured/e40c8a9d21db2023e89b8f5f920d52e5fa062932cad545c583e286d8d2115910.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-23-item-viewmodel-unification.md` (source: MANUAL, ingested: 2026-09-23, revision: `7d2a20408`).
- 2026-09-23 HEAD `7d2a20408` 기준 **정적 조사(빌드·실행 검증 아님)**와 사용자 결정 기록이다. 구현 커밋은 `ba396fc16`(VM 단일화), `9b41020cc`(enum 변수·변환 바인딩 도구), `2bfc61535`(MVVM 이벤트 목적지 도구). 노트 날짜 기준이라 현재 코드와 다를 수 있다.

## 확정 결정(사용자 확정으로 기록됨)

노트의 결정 문장 원문 중 판단 부분:

> "WxGame `UWxViewModel_InventoryItem`을 삭제하고 WxUI `UWxViewModel_Item`으로 단일화한다. WxCore는 건드리지 않는다."
> "변환 함수 라이브러리(`UWxInventoryConversionLibrary`)와 위젯 변수 방식은 더 단순한 이 방식으로 철회했다."
> "WxToolset 도구(`AddEnumVariable`, `WxMVVMToolset`)는 재사용을 위해 유지한다."

- WxUI는 WxInventory에 의존하지 않으므로 `UWxViewModel_Item`은 값을 받기만 한다. 관찰과 공급은 WxGame `UWxViewModel_Inventory`가 맡는다(Ability System VM → Ability VM 구조와 같음).
- `UWxViewModel_Inventory`는 PC당 공유 합성 VM(`GetOrCreate(PC)`, Outer = PC)이다. 두 리졸버(`UWxViewModelResolver_Inventory`, `UWxViewModelResolver_Item`)가 같은 공유본을 쓰며 `DestroyInstance`에서 정리하지 않는다.
- 카테고리 선택(`CurrentCategory`)과 거른 목록(`CategorizedItems`)은 공유 VM이 가져 인벤토리를 다시 열어도 마지막 탭이 유지된다.
- 퀵슬롯 사용 요청은 Ability VM(`Ability.UseItem`)의 `TryActivateAbility`로 바꿨다.
- 어떤 WBP도 바인딩하지 않는 필드(`Grade`, `MaxCharges`, `bIsInventoryAvailable`, `LastChanged*`·`GetCurrencyAmount`)를 제거했다.
- 동작 변화: 획득 토스트 VM은 획득 시점 값으로 한 번만 채우고 이후 수량 변화를 갱신하지 않는다.

## 구현 관찰(확인한 계약)

- `UWxViewModel_Item`은 원본 타입을 해석하지 않고 `SourceObject`·`DisplayName`·`Icon`·`TotalCount`·`CurrentCharges`·`GradeColor`를 setter로 받는다. `AcquiredCount`는 FieldNotify 없는 1회용 채널이다.
- `SourceObject`는 슬롯 VM이면 ItemInstance, 합계 VM이면 ItemDefinition이다. 합계 VM은 ItemDef를 공유 키로 한다.
- 클라에서 인벤토리가 위젯보다 늦게 복제될 수 있어, 공유 VM은 교체되지 않고 `OnAnyInventoryReady`·`OnAnyInventoryEnded`로 내부 연결만 바꾼다.
- 슬롯 수량·충전 변경은 값만 다시 채우고, 목록 변경은 `AllItems`·`CategorizedItems`를 다시 만든다.

## 엔진 사실: MVVM 변환 함수 위치 제약

- 변환 함수는 위젯 블루프린트 자신의 Pure·const 함수이거나 `UBlueprintFunctionLibrary`의 정적 Pure 함수여야 한다(UE 5.8 `MVVMBlueprintViewConversionFunction.cpp:36-45`). VM 클래스의 정적 함수는 엔진이 거부했다.
- MVVM 암시적 변환기는 enum을 제외한다(`MVVMNumericImplicitConverter.cpp`).

## WxToolset 도구

- `UWxBlueprintToolset::AddEnumVariable`: 순정 `BlueprintTools.add_variable`이 enum 멤버 변수를 만들 수 없어 추가했다.
- `UWxMVVMToolset::SetBindingConversionFunction`: 변환 함수 지정과 인자 경로 연결. 잘못된 인자 키는 스크립트 에러로 막고, 엔진 `SetGraphPin`의 `check` 크래시를 코드 리뷰에서 막았다.
- `UWxMVVMToolset::SetEventDestinationWidgetFunction`: `UMVVMEditorSubsystem::SetEventDestinationPath`를 써서 MVVM 이벤트 목적지를 위젯 함수로 바꾼다(`WBP_DialogueScreen` 진행 이벤트에 사용).
- WxToolset 모듈은 AnimMontage·Blueprint·MVVM·StateTree 네 도구 클래스를 등록·해제한다.

## 검증 범위

- Task 기록 기준 Development·DebugGame 빌드와 관련 위젯 6개 컴파일 성공. Content 전체에서 `WxViewModel_InventoryItem` 참조 0건, 임시 ClassRedirect 제거.
- 런타임 미검증(PIE 맵이 FrontEnd라 HUD 미생성). 퀵슬롯 표시·사용, 골드 갱신, 획득 토스트, 탭 필터링·유지, 리스폰 후 동작은 인간 확인 대상.

## 관련 주제

- [[UI 표시 구조]]
- [[아이템과 회복]]
- [[에디터 도구]]

## 핵심 주장

- 사용자는 WxGame UWxViewModel_InventoryItem을 삭제하고 WxUI UWxViewModel_Item으로 단일화하기로 확정했다. ^c1
- UWxViewModel_Inventory는 PC당 공유 합성 VM이며 인벤토리·아이템 두 리졸버가 같은 공유본을 쓰고 DestroyInstance에서 정리하지 않는다. ^c2
- UE 5.8 MVVM 변환 함수는 위젯 블루프린트의 Pure·const 함수나 BlueprintFunctionLibrary의 정적 Pure 함수여야 하며 VM 클래스의 정적 함수는 거부된다. ^c3
- 아이템 VM 단일화의 런타임 동작은 노트 시점에 검증되지 않았다. ^c4
