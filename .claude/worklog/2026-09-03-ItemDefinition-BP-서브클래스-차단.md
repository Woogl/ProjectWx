# ItemDefinition BP 서브클래스 차단

## 계획

### 목표
`UWxItemDefinition` 의 BP 서브클래스를 만들 수 없게 막는다. 앞선 작업(`2026-09-03-아이템-PrimaryAssetId-오버라이드-제거.md`)에서 주석으로만 남겼던 제약을 컴파일러·에디터가 강제하게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` | `UCLASS(BlueprintType)` → `UCLASS(BlueprintType, NotBlueprintable)`, 클래스 주석을 지정자 사유로 교체 | 수정 |

### 접근 방식
- **`NotBlueprintable` 이 필요한 이유**: 엔진의 `UPrimaryDataAsset` 이 `UCLASS(abstract, MinimalAPI, Blueprintable)` 로 선언돼 있어(`Engine/Classes/Engine/DataAsset.h:46`) 지금은 BP 서브클래스가 만들어진다. `CanCreateBlueprintOfClass` 가 `GetBoolMetaDataHierarchical(MD_IsBlueprintBase)` 로 판정하므로(`UnrealEd/Private/Kismet2/Kismet2.cpp:1065-1080`), 파생 쪽에 `NotBlueprintable` 을 달면 부모의 `Blueprintable` 을 덮는다.
- **`BlueprintType` 은 유지한다**: 둘은 직교한다. `WBP_TotalGold`·`WBP_AcquiredItemEntry`·`WBP_ItemQuickSlot`·`GA_UseItem` 이 이 타입을 변수·핀 타입으로 참조하고 있어 `BlueprintType` 을 빼면 그쪽이 깨진다. `NotBlueprintable` 은 서브클래스 생성만 막고 타입 참조에는 영향이 없다.
- **네이티브 DataAsset 저작 경로는 그대로**: `MD_IsBlueprintBase` 는 BP 생성 경로와 `bIsBlueprintBaseOnly` 클래스 뷰어만 참조한다. Data Asset 팩토리로 `DA_Katana` 류를 만드는 경로는 이 메타데이터를 보지 않는다.

### 사전 확인
- `UWxItemDefinition` 의 네이티브 파생 클래스가 없다.
- BP 서브클래스 에셋도 없다. `Content/Item/BP_Katana`·`BP_MinionKatana` 는 `AWxWeaponBase` BP 이고, 위 4개 BP 는 타입을 참조만 한다. 따라서 지정자 추가로 깨지는 기존 에셋이 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` | `NotBlueprintable` 추가, 클래스 주석을 지정자 사유로 교체 | 수정 |

### 구현·결정과 그 이유
- **주석을 규약 서술에서 지정자 사유로 바꿨다**: 이제 제약을 에디터가 강제하므로 "네이티브로만 만든다"는 당부는 불필요해졌다. 대신 코드에서 읽히지 않는 정보 — 왜 굳이 막았는가 — 만 남겼다.
- **`BlueprintType` 을 남긴 이유**: 두 지정자는 직교한다. 서브클래스 생성만 막고 BP 변수·핀 타입 참조는 그대로 둬야 기존 위젯·어빌리티 BP 가 유지된다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음
