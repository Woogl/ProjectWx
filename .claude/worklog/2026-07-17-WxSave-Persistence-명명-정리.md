# WxSave — Persistence 명명 정리 (파일·클래스 리네임)

## 계획

### 목표

WxSave 플러그인은 샘플 PersistenceLab(`Plugins/PersistenceUtils`)의 골격을 이식하면서 그쪽 어휘인 "Persistence" 를 그대로 들고 왔다. 그 결과 플러그인 이름(WxSave)과 내부 타입 이름(WxPersistence\*)이 어긋나 있고, BP 진입점은 `UWxSaveFilePersistenceUtils` 라는 두 어휘가 섞인 이름이 됐다. 플러그인 어휘를 `WxSave` 로 통일해 이름만 보고도 소속 도메인이 읽히게 만든다. 동작 변경은 없는 순수 리네임이다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxPersistenceGameSubsystem.h/.cpp` | `WxSaveGameSubsystem.h/.cpp` 로 이동, `UWxPersistenceGameSubsystem` → `UWxSaveGameSubsystem`, `SetPersistenceTravelData` → `SetTravelData` | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public/WxPersistenceSaveGame.h` | `WxSaveGame.h` 로 이동, `UWxPersistenceSaveGame` → `UWxSaveGame`, `FWxPersistenceTravelData` → `FWxSaveTravelData` | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxSaveFilePersistenceUtils.h/.cpp` | `WxSaveLibrary.h/.cpp` 로 이동, `UWxSaveFilePersistenceUtils` → `UWxSaveLibrary` | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxPersistenceWorldSubsystem.h/.cpp` | `WxSaveWorldSubsystem.h/.cpp` 로 이동, `UWxPersistenceWorldSubsystem` → `UWxSaveWorldSubsystem` | 수정(이동) |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | include 경로·타입 참조 갱신 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | include 경로·타입 참조 갱신 | 수정 |
| `Config/DefaultEngine.ini` | 빈 `[CoreRedirects]` 섹션에 클래스 4개 + 구조체 1개 리다이렉트 추가 | 수정 |
| `Plugins/WxSave/README.md`, `Source/WxGame/README.md`, `Docs/Programmer/WxSave_PersistenceLab_Comparison.md` | 문서의 클래스명·경로 갱신 | 수정 |

`FWxActorRecord` / `FWxComponentRecord`, 로그 카테고리 `LogWxSave`, 콘솔 명령 `Wx.Save.Dump` 는 그대로 둔다.

### 접근 방식

- **파일 이동은 `git mv`**: 히스토리를 잇는다. `#include` 경로와 `.generated.h` 이름이 새 파일명을 따라간다.

- **치환 대상은 `Wx` 접두가 붙은 이름뿐**: WxSave 소스 주석의 "샘플 `UPersistenceGameSubsystem` 골격 이식", "샘플 `USaveFilePersistenceUtils` 골격 이식" 같은 문구는 샘플 쪽 클래스를 가리키므로 건드리지 않는다.

- **CoreRedirects 로 애셋 참조 보존**: BP 위젯 2개(`WBP_MainMenu`, `WBP_DeathScreen`)가 `UWxSaveFilePersistenceUtils::SaveToFile` 호출 노드와 `UWxPersistenceSaveGame` 핀을 들고 있어, 리다이렉트 없이 리네임하면 노드가 깨진다. BP 참조가 없는 서브시스템·구조체까지 전부 넣는 이유는 기존 `.sav` 파일에 그 이름들이 직렬화돼 있기 때문이다. 함수 이름은 안 바뀌므로 `+FunctionRedirects` 는 불필요하고, `SetPersistenceTravelData` 는 BP 미노출이라 대상이 아니다.

- **worklog 는 갱신하지 않음**: 기존 8개 문서는 그 시점의 기록이다. BP 스냅샷 JSON 도 BP 저장 시 자동 갱신되므로 손대지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h` | 구 `WxPersistenceSaveGame.h`. `UWxSaveGame`, `FWxSaveTravelData` | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxSaveGameSubsystem.h/.cpp` | 구 `WxPersistenceGameSubsystem.*`. `UWxSaveGameSubsystem`, `SetTravelData`. 콘솔 명령 주석의 "WxPersistence 슬롯" → "WxSave 슬롯" | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxSaveWorldSubsystem.h/.cpp` | 구 `WxPersistenceWorldSubsystem.*`. `UWxSaveWorldSubsystem` | 수정(이동) |
| `Plugins/WxSave/Source/WxSave/Public\|Private/WxSaveLibrary.h/.cpp` | 구 `WxSaveFilePersistenceUtils.*`. `UWxSaveLibrary` | 수정(이동) |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | include 경로·타입 참조 갱신 | 수정 |
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | include 경로·타입 참조 갱신 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]` 에 ClassRedirects 4 + StructRedirects 1 | 수정 |
| `Plugins/WxSave/README.md`, `Source/WxGame/README.md`, `Docs/Programmer/WxSave_PersistenceLab_Comparison.md` | 클래스명·경로 갱신 | 수정 |

### 구현·결정과 그 이유

- **치환 패턴에 `Wx` 접두를 포함**: WxSave 소스 주석은 이식 출처로 샘플 클래스명(`UPersistenceGameSubsystem`, `USaveFilePersistenceUtils`, `UPersistenceWorldSubsystem`)을 인용한다. 접두를 포함한 패턴만 치환해 이 인용들이 살아남았고, 치환 후 세 곳 모두 원문 그대로임을 확인했다.

- **BP 참조가 없는 타입까지 리다이렉트**: 위젯 두 개가 참조하는 건 라이브러리와 SaveGame 클래스뿐이지만, 서브시스템과 트래블 구조체 이름은 기존 `.sav` 파일에 직렬화돼 있다. 구 세이브 파일이 조용히 깨지는 걸 막으려 다섯 개를 전부 넣었다.

- **비교 문서의 "함수명 1:1" 주장 수정**: `SetPersistenceTravelData` → `SetTravelData` 축약으로 그 목록이 사실과 어긋나게 됐다. 목록에서 빼고 축약 사실을 비고에 남겼다.

### 계획 대비 달라진 점
- 계획대로. 비교 문서의 "함수명 1:1" 줄 수정은 계획에 없었으나, 이번 리네임이 만든 사실 오류라 함께 고쳤다.

### 후속 과제
- **리다이렉트 실동작 미검증**: 빌드는 통과했고 UHT 도 새 파일명으로 전부 재생성됐지만, WBP_MainMenu / WBP_DeathScreen 의 노드가 실제로 보존되는지는 에디터를 열어야 확인된다. 확인 후 두 위젯을 재저장하면 리다이렉트가 애셋에 구워지고, 그 시점에 `[CoreRedirects]` 항목을 걷어낼 수 있다.
