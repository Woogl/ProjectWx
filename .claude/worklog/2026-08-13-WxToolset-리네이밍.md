# WxStateTreeToolset 플러그인을 WxToolset으로 리네이밍

## 계획

### 목표
에디터 전용 MCP 툴셋 플러그인의 이름을 특정 도메인(StateTree)에 묶인 `WxStateTreeToolset`에서 중립적인 `WxToolset`으로 바꾼다. 앞으로 StateTree 외의 툴셋도 같은 플러그인에 담을 수 있도록 컨테이너 이름을 일반화하는 것이 목적이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxStateTreeToolset/` → `Plugins/WxToolset/` | 플러그인 폴더 이름 변경 | 수정 |
| `WxStateTreeToolset.uplugin` → `WxToolset.uplugin` | 파일명·FriendlyName·모듈 Name 변경, Description을 컨테이너 관점으로 일반화 | 수정 |
| `Source/WxStateTreeToolset/` → `Source/WxToolset/` | 모듈 폴더 이름 변경 | 수정 |
| `WxStateTreeToolset.Build.cs` → `WxToolset.Build.cs` | 파일명·ModuleRules 클래스명 변경 (의존성 목록은 그대로) | 수정 |
| `Private/WxStateTreeToolsetModule.h/.cpp` → `Private/WxToolsetModule.h/.cpp` | 파일명, `FWxToolsetModule`, `LogWxToolset`, `IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)` | 수정 |
| `Private/WxStateTreeToolset.h/.cpp` | **변경 없음** — 툴셋 정의 클래스 `UWxStateTreeToolset` 유지 | 유지 |
| `Plugins/WxToolset/Binaries/`, `Intermediate/` | 옛 모듈명 산출물 삭제 (재생성 대상) | 삭제 |
| `Wx.uproject` | 플러그인 활성화 항목 이름 `WxToolset`으로 변경 | 수정 |
| `.claude/CLAUDE.md` | 플러그인 표 행 이름 변경 | 수정 |

### 접근 방식
- **컨테이너와 툴셋의 이름을 분리한다**: 리네이밍 대상은 플러그인·모듈(그릇)까지이고, 툴셋 정의 클래스 `UWxStateTreeToolset`은 그대로 둔다. MCP에 노출되는 툴셋 이름은 모듈이 아니라 `UToolsetRegistry`에 등록되는 클래스에서 나오므로, 클래스를 함께 바꾸면 이미 기록해 둔 MCP 호출 규약이 전부 깨진다. 또 이 클래스는 여전히 StateTree 전용 도구 모음이라 이름이 내용과 맞는다.
- **빌드 산출물은 지우고 다시 만든다**: 모듈 이름이 바뀌므로 옛 이름의 DLL·UHT 생성물이 남아 있으면 에디터가 사라진 모듈을 붙잡는다. 폴더째 삭제해 UBT가 새 이름으로 다시 생성하게 한다. 이를 위해 실행 중인 에디터를 먼저 종료해야 한다(폴더 안 DLL이 로드돼 잠겨 있음).
- **참조 그래프는 손댈 필요가 없다**: 이 모듈에 의존하는 다른 모듈이 없는 리프 모듈이라, 다른 `.Build.cs`나 include 경로 수정이 발생하지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxToolset/` | 플러그인 폴더 이름 변경 (구 `WxStateTreeToolset`) | 수정 |
| `Plugins/WxToolset/WxToolset.uplugin` | 파일명·FriendlyName·모듈 Name 변경, Description을 컨테이너 관점으로 일반화 | 수정 |
| `Plugins/WxToolset/Source/WxToolset/` | 모듈 폴더 이름 변경 | 수정 |
| `Plugins/WxToolset/Source/WxToolset/WxToolset.Build.cs` | 파일명·ModuleRules 클래스명 변경 (의존성 목록 그대로) | 수정 |
| `.../Private/WxToolsetModule.h/.cpp` | 파일명, `FWxToolsetModule`, `LogWxToolset`, `IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)` | 수정 |
| `.../Private/WxStateTreeToolset.h/.cpp` | 변경 없음 — 툴셋 정의 클래스 `UWxStateTreeToolset` 유지 | 유지 |
| `Wx.uproject` | 플러그인 활성화 항목 이름 변경 | 수정 |
| `.claude/CLAUDE.md` | 플러그인 표 행 이름·설명 변경 | 수정 |
| `Plugins/WxToolset/Binaries/`, `Intermediate/` | 옛 모듈명 산출물 삭제 후 재생성 | 삭제·재생성 |

### 구현·결정과 그 이유
- **그릇만 바꾸고 툴셋 클래스는 남겼다**: MCP 툴셋 이름은 엔진이 `모듈명.클래스명`으로 합성하며 이름을 따로 지정하는 훅이 없다. 클래스까지 바꾸면 `WxToolset.WxToolset`이 되어 이름에서 도메인이 사라지고, 이미 기록해 둔 MCP 호출 규약도 전부 깨진다. 클래스를 남겨 `WxToolset.WxStateTreeToolset`이 되었고, 그릇은 도메인 중립·툴셋은 도메인 유지라는 의도한 구분이 이름에 그대로 드러난다.
- **빌드 산출물을 폴더째 지웠다**: 모듈 이름이 바뀌면 옛 이름의 DLL과 UHT 생성물이 사라진 모듈을 가리키게 된다. 남겨두면 에디터가 이를 붙잡을 수 있어 재생성으로 정리했다.
- **다른 코드는 손대지 않았다**: 이 모듈에 의존하는 모듈이 없는 리프라, 다른 `.Build.cs`나 include 경로에 파급이 없었다.

### 계획 대비 달라진 점
- 계획에 없던 IDE 프로젝트 파일 재생성을 추가했다. 기존 `Wx.vcxproj`가 옛 플러그인 경로를 가리키고 있어 재생성하지 않으면 IDE에 없는 파일로 남는다. `Wx.sln` 자체는 프로젝트 목록이 그대로라 갱신되지 않았다(정상).

### 후속 과제
- 에디터 실행 후 `list_toolsets`로 `WxToolset.WxStateTreeToolset` 등록을 실증하지 않았다. 빌드·링크까지만 확인한 상태다.
- `LogWxToolset` 로그 카테고리는 선언·정의만 있고 사용처가 없다. 리네이밍 범위 밖이라 그대로 두었다.
