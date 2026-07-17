# UWxSaveLibrary 에 현재 슬롯 조회 함수 추가

## 계획

### 목표
BP 에서 활성 슬롯이 무엇인지 물어볼 수단이 없다. 라이브러리는 슬롯을 만들고·로드하고·저장하고·지울 수 있지만, 지금 어느 슬롯이 활성인지는 알 수 없다. 메인메뉴 등이 현재 슬롯 이름을 표시하거나 저장 UI 에 프리필할 수 있게 조회 함수를 노출한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` | `GetCurrentSlotName` 선언을 `public:` 최상단(`StartNewSaveFile` 앞)에 추가 | 수정 |
| `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp` | 헤더 순서대로 `StartNewSaveFile` 앞에 정의 추가, `WxSaveGame.h` 명시 include | 수정 |
| `Plugins/WxSave/README.md` | 핵심 타입 표의 `UWxSaveLibrary` 표면 설명에 활성 슬롯 조회 반영 | 수정 |

### 접근 방식
- **기존 게터의 BP 래핑**: 슬롯 정체성은 `UWxSaveGame` 이 보유하고 서브시스템이 `GetSaveGame()` 으로 이미 노출하지만 C++ 전용이라 BP 에서 닿지 않는다. 슬롯 이름 필드도 `UPROPERTY()` 뿐이라 SaveGame 객체를 BP 로 넘겨도 읽히지 않는다. 라이브러리가 이 게터를 감싸 이름 문자열만 꺼내주면 서브시스템·SaveGame 을 건드리지 않고 해결된다.
- **반환은 슬롯 이름 문자열만**: 활성 SaveGame 이 없으면 빈 문자열. `UserIndex` 는 이 프로젝트에서 사실상 항상 0 이라 노출하지 않고, 필요해지면 그때 추가한다.
- **`BlueprintPure`**: 메모리 읽기뿐인 const 질의다. 같은 파일의 존재확인 함수가 impure 인 건 디스크 I/O 라서이므로 선례로 삼지 않는다.
- **배치**: 상태 조회는 수명 API 와 성격이 달라 앞에 모은다. 서브시스템 헤더가 이미 같은 순서(상태 게터 → 수명 API)를 쓰므로 라이브러리도 그대로 미러링한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h` | `GetCurrentSlotName` 을 `BlueprintPure` 로 선언, `public:` 최상단 배치 | 수정 |
| `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp` | 동명 정의를 헤더 순서대로 최상단에 추가, `WxSaveGame.h` 명시 include | 수정 |
| `Plugins/WxSave/README.md` | 핵심 타입 표의 라이브러리 표면 설명에 활성 슬롯 조회 추가 | 수정 |

### 구현·결정과 그 이유
- **기존 게터를 감싸기만 함**: 서브시스템이 활성 SaveGame 을 이미 노출하고 있어 새 상태나 저장 경로를 만들 이유가 없었다. 라이브러리가 그 게터에서 이름만 꺼내면 되므로 서브시스템·SaveGame 은 손대지 않았다.
- **SaveGame 객체 노출 대신 문자열 반환**: 슬롯 이름 필드가 BP 에 열려 있지 않아 객체를 넘겨도 어차피 읽히지 않는다. 필드를 BP 에 여는 건 슬롯 정체성을 쓰기 가능한 표면으로 넓히는 셈이라, 조회 목적엔 문자열 반환이 더 좁고 정확했다.
- **`UserIndex` 미노출**: 이 프로젝트에선 사실상 항상 0 이라 지금 노출하면 쓰이지 않을 인자가 된다. 필요해지면 그때 추가한다.
- **`BlueprintPure`**: 메모리 읽기뿐인 const 질의다. 같은 파일의 존재확인 함수가 impure 인 건 디스크 I/O 라서이므로 선례로 삼지 않았다.
- **이중 널 체크**: 서브시스템 부재(월드/GameInstance 없는 컨텍스트)와 활성 SaveGame 부재는 별개 경로다. 둘 다 빈 문자열로 수렴시켜 호출부가 한 가지만 보면 되게 했다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 소비하는 BP 는 아직 없다. `WBP_MainMenu` 의 슬롯 표시·프리필 연결은 UI 작업 시 별건으로 진행한다.
