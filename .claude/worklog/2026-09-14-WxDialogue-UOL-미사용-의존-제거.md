# WxDialogue 에서 쓰지 않는 UniversalObjectLocator 의존 제거

## 계획

### 목표
`WxDialogue.Build.cs` 가 `UniversalObjectLocator`(UOL)를 Public 의존으로 선언하지만 모듈 소스 어디서도 쓰지 않는다(`/module-review` WxDialogue 6번 🟢). UOL 을 쓰던 `Enable Npc Interaction` 태스크가 WxWorld 통합 태스크로 흡수되며 삭제됐을 때 선언만 남은 흔적이라, 쓰지 않는 링크·include 경로가 소비자로 전파되고 "대화가 UOL 을 쓴다"는 오해를 남긴다. 한 줄을 지운다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs` | `PublicDependencyModuleNames` 에서 `"UniversalObjectLocator"` 제거 | 삭제 |

### 접근 방식
- **자기 사용 0건**: 모듈 소스 grep 히트는 Build.cs 한 줄뿐이고, UOL 타입을 쓰는 리플렉션 필드·시그니처도 없다. UOL 은 플러그인이 아닌 엔진 모듈이라 `.uplugin` 항목도 없다.
- **포함 헤더가 UOL 을 요구하지 않음**: 엔진 공개 헤더 중 UOL 헤더를 포함하는 곳은 UOL 모듈 자신·`MovieScene`·`Engine/Public/UniversalObjectLocators/*` 뿐인데, WxDialogue 가 포함하는 헤더는 어느 것도 거기에 닿지 않는다. `Engine` 모듈도 UOL 을 Private 로만 둬 암묵 전파가 없다.
- **하류 소비자 무영향**: WxDialogue 를 참조하는 모듈은 WxGame 하나이고, WxGame 이 포함하는 UOL 헤더(WxWorld 태스크 헤더 경유)의 경로는 WxWorld·WxQuest·WxUI 의 Public UOL 의존으로 이미 전이받는다.
- **한계**: 에디터 빌드에선 WxCore 가 UOL 을 Public 으로 전파해 WxEditor 빌드만으로는 게임 타겟의 누락을 잡지 못한다. 위 헤더 포함 검증으로 대신한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/Source/WxDialogue/WxDialogue.Build.cs` | `PublicDependencyModuleNames` 에서 `"UniversalObjectLocator"` 한 줄 제거 | 삭제 |

### 구현·결정과 그 이유
- **대체 선언 없이 삭제**: 모듈이 쓰지 않고, 포함하는 헤더도 요구하지 않으며, 유일한 소비자 WxGame 은 다른 도메인 모듈에서 같은 경로를 이미 전이받는다. 옮기거나 Private 로 내릴 이유가 없어 의존 자체를 없앴다.
- **같은 제거가 다시 되돌아온 경위를 확인**: 07-29 에 한 번 지웠다가 08-03 `Enable Npc Interaction` 태스크가 UOL 필드를 쓰면서 재추가됐고, 08-14 그 태스크가 WxWorld 로 흡수될 때 선언만 남았다. 지금 모듈 안에는 UOL 을 다시 필요로 할 노드가 없다.
- **검증**: UE 5.8 `WxEditor Win64 Development` 빌드 `Result: Succeeded`. UBT 가 Build.cs 변경으로 makefile 을 무효화하고 `UnrealEditor-WxDialogue.dll` 만 다시 링크했으며, 갱신된 링크 응답 파일에서 `UnrealEditor-UniversalObjectLocator.lib` 가 빠졌다. 소스에 UOL 참조가 남지 않았음을 grep 으로 확인했다(히트는 `Intermediate/` 산출물뿐).
- **재컴파일이 없었던 이유**: 에디터 빌드에선 WxCore 가 `bBuildEditor` 블록에서 UOL 을 Public 으로 전파해 컴파일 include 경로가 그대로라서다. 그래서 이 빌드는 게임 타겟에서의 누락을 증명하지 못하며, 그쪽은 헤더 포함 경로 정적 확인으로 대신했다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 게임 타겟(`Wx`) 빌드는 돌리지 않았다(마지막 빌드 08-12). 다음 게임 타겟 빌드·패키징 때 WxDialogue 컴파일이 통과하는지 확인한다 — 정적 확인상 문제는 없다.
