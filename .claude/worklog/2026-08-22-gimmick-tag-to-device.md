# 태그 정의의 Gimmick → Device 이름 변경

## 계획

### 목표
기믹 시스템이 `AWxDevice`/`AWxTriggerDevice` 로 이관되면서 클래스·폴더·문서는 모두 "Device" 어휘를 쓰는데 네이티브 태그만 `Gimmick.*` 로 남아 있다. 이 11개 태그를 `Device.*` 로 옮겨 어휘를 통일한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 섹션 주석과 `Gimmick_*` 선언 11줄을 `Device_*` 로 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | `Gimmick_*`/`"Gimmick.*"` 정의 11줄을 `Device_*`/`"Device.*"` 로 | 수정 |
| `Config/DefaultGameplayTags.ini` | `GameplayTagRedirects` 11줄 | 신규 |
| `Plugins/WxCore/README.md` | 태그 네임스페이스 표기 정정 | 수정 |

### 접근 방식
- **심볼·문자열 동시 rename**: 태그를 읽거나 쓰는 코드가 없어(선언·정의가 전부) 컴파일 파급이 없다. 그대로 바꾼다.
- **에셋 호환은 태그 리다이렉트로**: `ST_Door`/`ST_Elevator`/`ST_TreasureChest`/`ST_CheckPoint` 4개가 옛 문자열을 들고 있으므로, 리다이렉트 없이 바꾸면 상태 태그가 미해결이 되어 전이가 깨진다. UE 5.8 정식 위치인 `Config/DefaultGameplayTags.ini` 의 `[/Script/GameplayTags.GameplayTagsSettings]` 에 넣는다 — `DefaultEngine.ini` 쪽은 deprecated 경로라 로드 시 에러 로그를 뱉는다.
- **범위 밖**: `AWxTriggerDevice::GimmickStateRequirements` 프로퍼티명(저작값 유실 방지에 `CoreRedirects` 별도 필요)과 `Content/WorldObject/Gimmick/` 폴더명은 건드리지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` | 섹션 주석 및 `Gimmick_*` 선언 11개를 `Device_*` 로 | 수정 |
| `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` | `Device_*` / `"Device.*"` 로 정의 갱신 | 수정 |
| `Config/DefaultGameplayTags.ini` | `GameplayTagRedirects` 11줄 | 신규 |
| `Plugins/WxCore/README.md` | 태그 네임스페이스 표기를 `Device` 로 정정 | 수정 |

### 구현·결정과 그 이유
- **리다이렉트를 `DefaultGameplayTags.ini` 에 둔 이유**: `UGameplayTagsSettings` 가 `config = GameplayTags` 라 이 파일이 정식 위치다. `DefaultEngine.ini` 의 `[/Script/Engine.Engine]` 도 읽히긴 하지만 deprecated 경로로 분류되어 로드마다 에러 로그가 남는다.
- **에셋을 직접 고치지 않은 이유**: 태그 문자열을 든 ST 에셋 4개는 바이너리다. 리다이렉트로 로드 시 새 태그로 넘긴 뒤 에디터에서 재저장해 굳히는 편이 안전하다.

### 계획 대비 달라진 점
- 계획대로. 다만 빌드 검증이 **WxWorld 의 선행 파손**에 막혔다 — 이번 변경과 무관하다.
  - `Private/Device/`·`Public/Device/` 가 아직 untracked 인 기믹→장치 이관 중간 상태이고, `WxDevice.h` 와 `WxDevice.cpp` 의 멤버가 어긋나 100개 넘는 에러가 난다(`StateTag`·`bTreeRunning`·`FWxDeviceStateTreeExecutionExtension` 미정의 등). `WxDeviceStateTreeComponent.cpp:194` 는 const 메서드에서 `FStateTreeReadOnlyExecutionContext` 에 const `InstanceData` 를 넘기는 별개 에러.
  - 이번 변경분인 `WxGameplayTags.cpp` 는 adaptive unity 로 단독 재컴파일되어 무에러 통과했고, WxCore 에 의존하는 WxUI·WxGame 도 정상 링크했다.

### 후속 과제
- WxWorld 이관 마무리 후 WxEditor 전체 빌드 재확인.
- 에디터에서 `ST_Door` / `ST_Elevator` / `ST_TreasureChest` / `ST_CheckPoint` 를 열어 `Device.*` 로 넘어왔는지 확인하고 재저장(재컴파일). 굳힌 뒤에는 `Config/DefaultGameplayTags.ini` 의 리다이렉트를 지워도 된다.
- 기존 세이브 슬롯에 남은 `Gimmick.*` 상태값도 리다이렉트로 넘어가지만, 슬롯을 새로 만들면 신경 쓸 필요 없다.
