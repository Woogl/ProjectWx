# 아이템 VM 단일화

상태: 확인 대기 · 빌드·위젯 컴파일 확인
다음 행동: 게임 맵에서 목록·퀵슬롯·획득 표시와 사용 동작을 확인한다.

이전 상태: 구현 완료 · 인간 코드 리뷰·인게임 확인 대기 · 2026-09-23

> 중간 기록의 위젯 변수·변환 함수·라이브러리 방식은 "카테고리 최종 결정" 절에서 철회됐다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 퀵슬롯 | 게임 맵 PIE: 포션 아이콘·충전 수가 보이고 클릭하면 아이템 사용이 발동하며 사용 뒤 충전 수·아이콘이 갱신된다 | 사람 | 대기 |  |
| 골드 | 게임 맵 PIE: 총량이 보이고 획득하면 갱신된다 | 사람 | 대기 |  |
| 획득 토스트 | 게임 맵 PIE: 이름·아이콘·획득 수가 보인다 | 사람 | 대기 |  |
| 인벤토리 | 게임 맵 PIE: 탭 3개 전환마다 목록이 걸러지고 등급색·수량이 보이며 다시 열면 마지막 탭이 유지된다 | 사람 | 대기 |  |
| 리스폰 뒤 유지 | 게임 맵 PIE: 리스폰한 뒤에도 위 동작이 유지된다 | 사람 | 대기 |  |
| 코드 리뷰 | WxViewModel_Item·Inventory VM 단일화 변경 | 사람 | 대기 |  |

## 확정 설계 · 2026-09-23 사용자 확정

- 요청: WxGame `UWxViewModel_InventoryItem`을 제거하고 WxUI `UWxViewModel_Item`으로 단일화한다. WxCore는 가급적 건드리지 않는다.
- 판단: WxUI는 WxInventory에 의존하지 않으므로 `UWxViewModel_Item`은 값을 받기만 하는 VM으로 둔다. 인벤토리 관찰과 값 공급은 WxGame `UWxViewModel_Inventory`가 맡는다. `WxViewModel_AbilitySystem` → `WxViewModel_Ability` 구조와 같다.
- 판단: `UWxViewModel_Inventory`는 PC당 공유 합성 VM(`GetOrCreate(PC)`, Outer = PC)이다. 두 리졸버가 같은 공유본을 쓰며, 공유본이라 `DestroyInstance`에서 정리하지 않는다.
- 판단(카테고리, 최종): 선택된 탭(`CurrentCategory`)과 거른 목록(`CategorizedItems`)은 공유 VM_Inventory가 가진다. 인벤토리를 다시 열어도 마지막 탭이 유지된다. 경위는 아래 "카테고리 최종 결정" 절에 있다.
- 판단: 퀵슬롯의 사용 요청은 VM_Ability(`Resolver_Ability`, AbilityTags = `Ability.UseItem`)의 `TryActivateAbility`로 바꾼다. 기존 `RequestUseConsumable`은 `TryActivateAbilitiesByTag(Ability.UseItem)`만 호출했고, `UWxAbility_UseItem`의 AssetTags에 `Ability.UseItem`이 있다.
- 필드: `VM_Item`에 `TotalCount`, `CurrentCharges`, `AcquiredCount`, `GradeColor`를 추가했다. `Grade`, `MaxCharges`, `bIsInventoryAvailable`은 어떤 WBP도 바인딩하지 않아 제거했다. VM_Inventory의 `LastChanged*`, `GetCurrencyAmount`, `bIsInventoryAvailable`도 제거했다.

## 구현 · 2026-09-23

- `WxViewModel_Item.h/.cpp`: 필드와 Setter를 추가했다. `SetSourceObject`를 public으로 옮겼다.
- `WxViewModel_Inventory.h/.cpp`:
  - `GetOrCreate`, `GetOrCreateItemViewModel`을 추가했다.
  - Slot·Charge 이벤트를 구독한다.
  - `RefreshItemViewModel`이 SourceObject(Instance 또는 Def)를 보고 값을 채운다.
  - 획득 토스트 VM은 획득 시점의 값으로 한 번만 채운다.
- `UWxViewModelResolver_Item`을 `WxViewModel_Inventory.h`로 옮겼다. 클래스 이름은 그대로다.
- `WxViewModel_InventoryItem.h/.cpp`를 삭제했다.
- `DefaultEngine.ini`에 `[CoreRedirects]` ClassRedirects(`WxViewModel_InventoryItem` → `/Script/WxUI.WxViewModel_Item`)를 추가했다가, WBP를 재저장한 뒤 제거했다.

## 검증 · 2026-09-23

- Development 빌드 성공. 같은 시각 다른 세션의 빌드가 이번 변경까지 컴파일했다. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_105827_436_15860.log`이다.
- DebugGame 빌드 성공. 사용자 승인을 받아 에디터를 재시작했다.

## WBP 수정 · 2026-09-23 (MCP, HTTP 직접 호출)

- 이 세션에는 unreal-mcp 도구가 로드되지 않았다. 그래서 `http://127.0.0.1:8000/mcp`를 JSON-RPC로 직접 호출했다(`call_tool` + toolset_name).
- WBP_ItemSlot, WBP_TotalGold, WBP_AcquiredItemEntry: 리다이렉트로 로드되고 컴파일된다.
  - 저장이 되려면 에셋이 dirty여야 한다. `bExposeInstanceInEditor`를 뒤집었다가 원래 값으로 돌려 dirty로 만든 뒤 저장했다.
  - 저장된 에셋에 `WxViewModel_InventoryItem` 문자열이 남아 있지 않다. Content 전체를 검색해도 0건이라 리다이렉트를 제거했다.
- WBP_ItemQuickSlot:
  - MVVM ViewModel 목록에 `WxViewModel_Ability` 컨텍스트를 추가했다. Resolver_Ability를 붙이고 AbilityTags를 `Ability.UseItem`으로 설정했다.
  - MVVM 이벤트(`OnButtonBaseClicked → RequestUseConsumable`)는 삭제했다. `MVVMBlueprintViewEvent.destinationPath`는 ObjectTools로 설정할 수 없기 때문이다.
  - 대신 이벤트 그래프에 `CommonButton|EventOnClicked → GetWxViewModel_Ability → TryActivateAbility`를 추가했다.
- 5개 위젯(AcquiredItemList 포함)을 컴파일했고 LogBlueprint 경고·오류는 0건이다.
- WBP_Inventory: 미완료. 컴파일 오류는 `SetCurrentCategory` 노드 3개와 `CategorizedItems` 바인딩이다. 순정 MCP로는 다음을 할 수 없다.
  - enum 멤버 변수 추가(`add_variable`은 기본 타입만 지원한다)
  - MVVM 변환 함수 바인딩 구성(conversion 객체의 경로·인자 설정)

## 변환 함수 위치 변경 · 2026-09-23 사용자 확정

- 사실: MVVM 변환 함수는 위젯 블루프린트 자신의 Pure·const 함수이거나 `UBlueprintFunctionLibrary`의 정적 Pure 함수여야 한다(UE 5.8 `MVVMBlueprintViewConversionFunction.cpp:36-45`). VM 클래스의 정적 함수는 엔진이 거부했다.
- 판단: WxGame에 `UWxInventoryConversionLibrary`를 새로 만들었다. 다음 대안은 기각했다.
  - WBP 내부 BP 함수: WxInventory에 UFUNCTION을 노출해야 하고, 로직이 BP로 복제된다.
  - 기존 라이브러리: WxRespawnLibrary·WxFrontEndLibrary는 도메인이 맞지 않고, WxRewardLibrary·WxMVVMConversionLibrary는 의존 방향상 넣을 수 없다.
  - uint8 변환: MVVM 암시적 변환기가 enum을 제외하므로(`MVVMNumericImplicitConverter.cpp`) 탭 값이 숫자가 되고, VM_Item에 의미 없는 숫자 필드가 생긴다.

## WxToolset 확장 · 2026-09-23 사용자 확정

- `UWxBlueprintToolset::AddEnumVariable`: 순정 `add_variable`이 기본 타입만 받아 추가했다.
- `UWxMVVMToolset::SetBindingConversionFunction`(신규): `UMVVMEditorSubsystem`을 감싼다. 변환 함수를 지정하고, 인자마다 "뷰모델이름.필드" 또는 "Self.필드" 경로를 연결한다. 서브시스템이 함수를 조용히 거부하면 스크립트 에러를 낸다.
- 의존성: Build.cs에 BlueprintGraph, ModelViewViewModel, ModelViewViewModelBlueprint, ModelViewViewModelEditor, UMGEditor를 추가했다. uplugin에는 ModelViewViewModel을 추가했다.
- 변수 FieldNotify: 기존 `SetVariableMeta`에 `{"FieldNotify":""}`를 넘긴다(`FBlueprintMetadata::MD_FieldNotify`). 버튼의 Set 노드가 `Set with Broadcast`로 바뀌는 것으로 인식을 확인했다.

## WBP_Inventory · 2026-09-23

- `CurrentCategory` 변수를 추가했다(EWxItemCategory, 기본값 Equipment, FieldNotify).
- 버튼 3개의 핸들러를 `SetCurrentCategory`(VM 함수)에서 `Set with Broadcast CurrentCategory`로 교체했다.
- 바인딩 `TileView_105.BP_SetListItems`를 `FilterItemsByCategory(Items = WxViewModel_Inventory.AllItems, Category = Self.CurrentCategory)` 변환으로 바꿨다. 두 핀 모두 `Valid`다.
- 위젯 6개(Inventory, ItemSlot, TotalGold, AcquiredItemEntry, ItemQuickSlot, AcquiredItemList)를 다시 컴파일했다. 오류 0이고 모두 저장했다.

## 최종 검증 · 2026-09-23

- DebugGame 빌드 성공(에디터를 세 번 재시작했다. 사용자 승인).
- Content 전체에서 `WxViewModel_InventoryItem` 참조는 0건이다. 리다이렉트는 제거했다.
- PIE: 에디터에 열린 맵이 LV_FrontEnd라 HUD 위젯이 생성되지 않았다. 따라서 런타임은 미검증이다.
- 인간 확인 필요(게임 맵 PIE):
  - 퀵슬롯: 포션 아이콘과 충전 수가 표시되는지, 클릭하면 아이템 사용 어빌리티가 발동하는지, 사용 후 충전 수와 아이콘이 갱신되는지
  - 골드: 총량이 표시되고, 획득하면 갱신되는지
  - 획득 토스트: 이름, 아이콘, 획득 수가 표시되는지
  - 인벤토리: 탭 3개를 전환할 때마다 목록이 걸러지는지, 등급색·수량이 표시되는지, 다시 열었을 때 마지막 탭이 유지되는지
  - 리스폰한 뒤에도 위 동작이 유지되는지
- 동작 변화 두 가지:
  - 획득 토스트 VM은 획득한 뒤 수량이 바뀌어도 갱신되지 않는다(토스트는 AcquiredCount·이름·아이콘만 쓴다).
- 완료 단계에서 할 일: Wiki의 UI/MVVM 관련 기사에 다음을 반영한다. ① VM_Item(WxUI)은 수동 VM이고 인벤토리 VM이 채운다. ② MVVM 변환 함수의 위치 제약(위젯 함수 또는 BlueprintFunctionLibrary). ③ WxMVVMToolset·AddEnumVariable 사용법.

## VM_Item 정리 · 2026-09-23 사용자 확정

- `UWxViewModel_Item::Deinitialize` override를 삭제했다. 이 함수는 파괴되거나 목록에서 빠진 VM에서만 불린다. 그런데 필드를 비우면서 변경을 알려, 베이스 계약("표시 필드 변경은 브로드캐스트하지 않는다")과 어긋났다. 또 새 목록이 반영되기 전의 엔트리를 빈칸으로 만들 수 있었다.
- 호출처가 없는 `UWxViewModel_Item::Initialize`를 삭제했다.
- `UWxViewModel_Inventory`에서 목록에서 빠진 슬롯 VM에 `Deinitialize`를 호출하던 루프 두 곳을 삭제했다. 버려진 VM의 이미지 로드는 GC가 `BeginDestroy`에서 취소한다.
- Development 빌드 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_113419_378_45628.log`이다. WBP는 이 함수들을 참조하지 않는다.

## 카테고리 최종 결정 · 2026-09-23 사용자 확정

- 요청: 변환 함수 라이브러리보다 단순한 방법을 원했다.
- 판단: 카테고리 선택 상태를 공유 VM_Inventory에 되돌린다(`CurrentCategory`, `CategorizedItems`, `SetCurrentCategory`).
  - 대가: 선택 탭이 PC마다 공유되어, 인벤토리를 다시 열어도 유지된다.
  - 수용 근거: 인벤토리 창은 하나뿐이라 선택이 부딪히지 않는다. 새 클래스와 BP 작업이 모두 없어진다.
- 구현:
  - `UWxInventoryConversionLibrary`를 삭제했다.
  - VM_Inventory의 `RefreshCategorizedItems`는 SourceObject(Instance)의 Def에서 카테고리를 읽는다. `RefreshAllItems`와 `UnbindSource`가 이 함수를 호출한다.
  - WBP_Inventory는 `git checkout`으로 HEAD 원본에 복원했다. 원본은 `CategorizedItems` 바인딩과 `SetCurrentCategory` 버튼을 쓰고, 이름이 같아 수정 없이 동작한다.
- WxToolset 도구(`AddEnumVariable`, `WxMVVMToolset`)는 재사용을 위해 유지한다(사용자).
- 검증: DebugGame 빌드 성공. 위젯 6개를 컴파일해 오류 0이다. WBP_Inventory는 HEAD와 동일해 재저장하지 않았다.
- 참고: 다른 세션이 `WxViewModelResolver_Ability`를 WxUI로 이관하면서 ClassRedirect를 추가했다. 이번 작업에서 저장한 `WBP_ItemQuickSlot`도 이 리졸버를 참조하므로, 그 세션이 에셋을 재저장할 때 대상에 포함돼야 한다.

## 코드 리뷰 반영 · 2026-09-23

- 리뷰 범위: 이번 작업 파일 8개. 결함은 1건이었다.
- 결함: `WxMVVMToolset::SetBindingConversionFunction`이 ArgumentsJson 키가 변환 함수의 입력 파라미터인지 확인하지 않았다. 엔진 `UMVVMBlueprintViewConversionFunction::SetGraphPin`은 없는 핀을 `check(GraphPin)`으로 받으므로 에디터가 크래시한다.
- 수정: 사전 해석 루프에서 키를 검증한다. `CPF_Parm`이면서 `CPF_ReturnParm`이 아니어야 하며, 아니면 스크립트 에러를 낸다. 헤더 주석의 예시도 실제로 있는 함수로 바꿨다.
- Development 빌드 성공. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_115926_019_30056.log`이다. 에디터에 반영하려면 재시작이 필요하다.
- 참고: 같은 파일의 `SetEventDestinationWidgetFunction`은 다른 세션이 추가한 것이다.

## Wiki 반영 · 2026-09-23

- 사용자 요청(위키 최신화)으로 인게임 확인 전에 반영했다. 퀵슬롯·획득 표시에는 "런타임 미검증"을 표기했다.
- 원자료 `.wiki/raw/notes/2026-09-23-item-viewmodel-unification.md`를 수집했다. 완료 단계 항목을 다음과 같이 편찬했다.
  - ① VM_Item은 수동 VM이고 인벤토리 VM이 채운다: `wiki/topics/ui.md` 「표시 VM의 위치와 연결」, `inventory.md` 「화면 표시와 픽업 문구」
  - ② MVVM 변환 함수 위치 제약: `ui.md`
  - ③ WxMVVMToolset·AddEnumVariable 사용법: `wiki/references/editor-tools.md` 「Blueprint 변수와 MVVM 바인딩 편집」. 도구 등록 수(세 개→네 개)도 정정했다.
- 순정 lint PASS(0건), `CheckWikiLinks.ps1` 오류 0(46개 문서). PowerShell 7이 없어 뷰어는 갱신하지 못했다.
