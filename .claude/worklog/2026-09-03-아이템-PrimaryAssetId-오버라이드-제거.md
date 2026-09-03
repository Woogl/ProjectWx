# 아이템 PrimaryAssetId 오버라이드 제거

## 계획

### 목표
`UWxItemDefinition::GetPrimaryAssetId()` 오버라이드가 반환하는 타입 `WxItem` 이 `DefaultGame.ini` 의 등록 타입 `WxItemDefinition` 과 어긋나, 에디터가 에셋 저장마다 "does not match object's real id ... will not load properly at runtime!" 에러를 낸다. 오버라이드를 지워 엔진 기본 구현에 맡긴다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` | `GetPrimaryAssetId()` 선언 제거, BP 서브클래스 금지 주석 추가 | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp` | `GetPrimaryAssetId()` 정의 제거 | 수정 |

### 접근 방식
- **엔진 기본 구현 사용**: `UPrimaryDataAsset::GetPrimaryAssetId()` 는 비-CDO 에셋에 대해 `FPrimaryAssetId(GetClass()->GetFName(), GetFName())` 를 반환한다. 아이템 에셋이 네이티브 `UWxItemDefinition` 인스턴스인 한 타입이 `WxItemDefinition` 이 되어 config 등록과 일치한다.
- **삭제 안전성 (사전 확인 완료)**: `WxItem` 타입 문자열은 코드·config 통틀어 이 한 줄뿐이고, 아이템 참조는 전부 `TSoftObjectPtr<UWxItemDefinition>` 이라 `FPrimaryAssetId` 로 아이템을 로드하는 경로가 없다. `USaveGame` 파생 클래스가 없어 직렬화된 `WxItem:` 문자열도 없다. 커스텀 AssetManager 도 없다.
- **BP 서브클래스 주의 주석**: BP 서브클래스로 아이템 에셋을 만들면 타입이 `BP_Foo_C` 가 되어 다시 불일치한다. `WxExperienceDefinition.h` 의 동일 취지 주석과 짝을 맞춘다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemDefinition.h` | `GetPrimaryAssetId()` 선언 제거, 클래스 주석에 BP 서브클래스 주의 한 줄 추가 | 수정 |
| `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemDefinition.cpp` | `GetPrimaryAssetId()` 정의 제거 | 수정 |

### 구현·결정과 그 이유
- **오버라이드 제거로 해결**: 등록 타입 쪽(`DefaultGame.ini` 의 `WxItemDefinition`)을 `WxItem` 으로 바꾸는 선택지도 있었지만, 엔진 기본 구현이 클래스명을 그대로 쓰므로 타입명을 클래스명과 일치시키는 편이 규약에 맞고 유지할 코드도 줄어든다.
- **삭제 안전성 사전 검증**: 코드·config 전수에서 `WxItem` 타입 문자열이 이 한 줄뿐이었고, 아이템은 전부 `TSoftObjectPtr<UWxItemDefinition>` 로만 참조돼 `FPrimaryAssetId` 로 로드하는 경로가 없었다. `USaveGame` 파생 클래스가 아직 없어 직렬화된 `WxItem:` 문자열도 없고, 커스텀 AssetManager 도 없다. `/Game/Item` 의 아이템 3종은 모두 네이티브 클래스 인스턴스라 기본 구현이 곧바로 `WxItemDefinition` 타입을 낸다.
- **주석을 클래스 doc 로 올린 이유**: 제약이 특정 함수가 아니라 "이 클래스의 에셋을 어떻게 만들어야 하는가"에 걸리므로, `WxExperienceDefinition.h` 의 동일 취지 주석과 위치를 맞췄다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 에디터에서 아이템 DataAsset 을 저장해 `LogAssetManager` 에러가 실제로 사라지는지 눈으로 확인하는 일이 남았다 (빌드로는 검증 불가).
