# WxSound(BGM) 플러그인 제거

## 계획

### 목표
BGM 시스템은 나중에 재설계·재구현하기로 하여, 현재 구현인 `WxSound` 플러그인을 통째로 걷어낸다. 저장소에 `WxBGM` 이라는 플러그인은 없고 `Plugins/WxSound/` 가 BGM 전용(소스 12파일 전원)이라 이를 대상으로 본다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSound/` 전체 | 플러그인 통째 삭제 (추적 15파일 `git rm -r`, 미추적 `Binaries`·`Intermediate` 는 파일 삭제) | 삭제 |
| `Wx.uproject` | `Plugins` 배열에서 `"WxSound"` 제거, WxSound 전용으로 켰던 `"Chooser"` 도 함께 제거 | 수정 |
| `Source/WxGame/WxGame.Build.cs` | `PrivateDependencyModuleNames` 에서 `"WxSound"` 제거 | 수정 |
| `Source/WxGame/Character/WxPlayerCharacter.h` · `.cpp` | `UWxBGMSourceComponent` 전방선언·프로퍼티·include·`CreateDefaultSubobject` 제거 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h` · `.cpp` | 동일 | 수정 |
| `.claude/CLAUDE.md` | 모듈 표에서 `WxSound` 행 삭제 | 수정 |
| `Docs/Programmer/BGM_Selection_Flow.md` | 사라진 시스템의 흐름 분석 전용 문서 | 삭제 |
| `Docs/Programmer/module_review_WxSound.md` | 사라진 모듈의 코드 리뷰 문서 | 삭제 |
| `Source/WxGame/README.md` | BGM/`[[WxSound]]` 언급 제거(의존 목록·조립 책임·캐릭터 표·BP 지정 항목) | 수정 |
| `Plugins/WxAI/README.md` · `Plugins/WxCore/README.md` | 소비자 목록에서 `[[WxSound]]`·BGM 제거 | 수정 |

과거 worklog(`*BGM*.md`, `*WxAudio-WxSound*.md`)는 작업 기록이므로 유지한다.

### 접근 방식
- **부작용 없는 통째 제거**: 이 시스템은 현재 사실상 죽어 있다 — `Config/` 에 `DefaultBGMChooser` 설정이 없어 Chooser 가 항상 null 이고, `UChooserTable`·`UWxBGMData` 에셋이 하나도 없으며, BP 에서 `StartBGM`/`StopBGM` 호출도 없다. 따라서 단계적 비활성화 없이 한 번에 걷어낸다.
- **BP 에셋은 손대지 않는다**: `BP_Player`/`BP_Enemy`/`BP_Boss` 가 `WxBGMSourceComponent` 를 참조하지만 C++ 이 만든 네이티브 서브오브젝트 참조일 뿐이고 `MusicTag`/`ActivationTag` 오버라이드가 하나도 없다(= 전부 inert). 네이티브 컴포넌트가 사라지면 엔진이 로드 시 조용히 드랍하므로 잃을 데이터가 없다.
- **`Chooser` 엔진 플러그인 동반 제거**: WxSound 를 위해 켠 플러그인이고 `ChooserTable` 에셋도 다른 코드 참조도 없어, 남겨두면 용도 없는 활성 플러그인이 된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSound/` 전체 | 플러그인 통째 제거(`git rm -r` 15파일 + 미추적 빌드 산출물 삭제) | 삭제 |
| `Wx.uproject` | `Plugins` 배열에서 `"WxSound"`·`"Chooser"` 제거 | 수정 |
| `Source/WxGame/WxGame.Build.cs` | `PrivateDependencyModuleNames` 에서 `"WxSound"` 제거 | 수정 |
| `Source/WxGame/Character/WxPlayerCharacter.h` · `.cpp` | 전방선언·`BGMSourceComponent` 프로퍼티·include·`CreateDefaultSubobject` 제거 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h` · `.cpp` | 동일 | 수정 |
| `.claude/CLAUDE.md` | 모듈 표 `WxSound` 행 삭제 | 수정 |
| `Docs/Programmer/BGM_Selection_Flow.md` · `module_review_WxSound.md` | 사라진 시스템 전용 문서 | 삭제 |
| `Source/WxGame/README.md` | 요약문·조립 책임·경계·의존 목록·캐릭터 표·BP 지정 항목·관련 링크에서 BGM/`[[WxSound]]` 제거 | 수정 |
| `Plugins/WxAI/README.md` · `Plugins/WxCore/README.md` | 소비자 목록에서 `[[WxSound]]`·BGM 제거 | 수정 |

### 구현·결정과 그 이유
- **단계적 비활성화 없이 통째 제거**: 이 시스템은 배선된 적이 없다 — `Config/` 에 `DefaultBGMChooser` 항목이 없어 Chooser 가 항상 null 이었고, `UChooserTable`·`UWxBGMData` 에셋도, BP 의 `StartBGM`/`StopBGM` 호출부도 존재하지 않았다. 죽은 코드라 한 번에 걷어내는 편이 안전하고 간결했다.
- **BP 에셋 무수정**: `BP_Player`/`BP_Enemy`/`BP_Boss` 의 `WxBGMSourceComponent` 참조는 C++ 이 만든 네이티브 서브오브젝트 참조뿐이고 `MusicTag`/`ActivationTag` 오버라이드가 하나도 없었다(= 전부 inert). 네이티브 클래스가 사라지면 엔진이 로드 시 드랍하므로 잃을 데이터가 없어 에셋을 건드리지 않았다.
- **`Chooser` 엔진 플러그인 동반 제거**: WxSound 를 위해 켠 것이고 `ChooserTable` 에셋도 다른 코드 참조도 없어, 남기면 용도 없는 활성 플러그인이 된다.
- **과거 기록은 보존**: `.claude/worklog/` 의 BGM 관련 문서와 `Docs/Meeting/2026-07-05 회의 안건.md` 는 시점 기록이라 손대지 않았다. 반면 `Docs/Programmer/` 의 두 문서는 "현재 코드 설명" 목적이라 삭제 대상으로 갈랐다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- BGM 재설계·재구현 시 신규 플러그인으로 착수. 재도입 시 `Chooser` 플러그인 재활성화 여부도 함께 결정한다.
- 다음 에디터 실행에서 `BP_Player`/`BP_Enemy`/`BP_Boss` 로드 시 missing component 경고가 한 번 뜰 수 있다(정상, 재저장하면 정리됨).
