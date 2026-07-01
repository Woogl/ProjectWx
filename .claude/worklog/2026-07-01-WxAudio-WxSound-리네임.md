# WxAudio 플러그인 → WxSound 리네임

## 계획

### 목표
오디오 도메인 플러그인 `WxAudio`를 `WxSound`로 리네임한다. 폴더·`.uplugin`·모듈·API 매크로·`Wx.uproject`·문서까지 일괄 변경한다. 다른 모듈이 `WxAudio`를 참조하지 않음(`.cs`/`.ini` 전수 확인: 자기 자신뿐)이라 외부 의존성 갱신은 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAudio/` → `Plugins/WxSound/` | 폴더 리네임 | 이동 |
| `WxAudio.uplugin` → `WxSound.uplugin` | 파일 리네임 + FriendlyName·Modules[0].Name | 이동+수정 |
| `Source/WxAudio/` → `Source/WxSound/` | 모듈 소스 폴더 리네임 | 이동 |
| `WxAudio.Build.cs` → `WxSound.Build.cs` | `class WxAudio`→`WxSound` | 이동+수정 |
| `WxAudioModule.h/.cpp` → `WxSoundModule.h/.cpp` | `FWxAudioModule`→`FWxSoundModule`, `IMPLEMENT_MODULE(FWxSoundModule, WxSound)` | 이동+수정 |
| 헤더 5종(WxMusicSubsystem/Settings/BGMData/MusicLibrary/BGMChooserContext) | `WXAUDIO_API`→`WXSOUND_API` | 수정 |
| `Wx.uproject` | Plugins 배열 `"WxAudio"`→`"WxSound"` | 수정 |
| `.claude/CLAUDE.md` | 모듈 표 행 `WxAudio`→`WxSound` | 수정 |
| `Plugins/WxSound/README.md`, `Plugins/WxCore/README.md` | `WxAudio` 참조/경로→`WxSound` | 수정 |

### 접근 방식
- **모듈명 변경 = API 매크로 변경**: UBT가 모듈명에서 `WXSOUND_API`를 파생하므로 헤더의 `WXAUDIO_API`를 반드시 교체.
- **git mv로 이력 보존**: 이동 전 구 모듈명 산출물(`Intermediate`/`Binaries`, DLL명이 바뀜)을 삭제해 트래킹 파일만 남긴 뒤 `git mv`.
- **검증**: 에디터 종료(구 DLL 잠금 방지) → WxEditor(Development) 빌드로 컴파일 확인.
- 과거 워크로그의 `WxAudio` 표기는 역사적 기록이라 그대로 둔다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAudio/` → `Plugins/WxSound/` | 플러그인 폴더 git mv | 이동 |
| `WxSound.uplugin` | 파일 리네임 + `FriendlyName`·`Modules[0].Name` → WxSound | 이동+수정 |
| `Source/WxAudio/` → `Source/WxSound/` | 모듈 소스 폴더 git mv | 이동 |
| `WxSound.Build.cs` | 파일 리네임 + `class WxAudio`/생성자 → `WxSound` | 이동+수정 |
| `WxSoundModule.h/.cpp` | 파일 리네임 + `FWxSoundModule`, `IMPLEMENT_MODULE(FWxSoundModule, WxSound)`, include 경로 | 이동+수정 |
| 헤더 5종(Subsystem/Settings/BGMData/MusicLibrary/BGMChooserContext) | `WXAUDIO_API`→`WXSOUND_API` | 수정 |
| `Wx.uproject` | Plugins 배열 `"WxAudio"`→`"WxSound"` | 수정 |
| `.claude/CLAUDE.md` | 모듈 표 행 → `WxSound` | 수정 |
| `Plugins/WxSound/README.md`, `Plugins/WxCore/README.md` | `WxAudio` 참조/경로 → `WxSound` | 수정 |
| `Plugins/WxAudio/Intermediate`·`Binaries` | 구 모듈명 산출물 삭제(재생성) | 삭제 |

### 구현·결정과 그 이유
- **git mv로 이력 보존**: 이동 전 untracked 빌드 산출물(`Intermediate`/`Binaries`)을 삭제해 트래킹 파일만 남긴 뒤 폴더→파일 순으로 `git mv`. DLL명이 `UnrealEditor-WxAudio.dll`→`UnrealEditor-WxSound.dll`로 바뀌므로 구 산출물 삭제는 필수.
- **모듈명 변경 = API 매크로 변경**: UBT가 모듈명에서 매크로를 파생하므로 헤더의 `WXAUDIO_API`를 `WXSOUND_API`로 교체하지 않으면 컴파일 실패. 빌드로 최종 확인.
- **외부 의존성 0 확인**: `.cs`/`.ini` 전수 grep에서 `WxAudio` 참조는 자기 자신뿐이라, 타 모듈 Build.cs·문자열 모듈 로드 갱신이 불필요했다(플러그인 규칙과 일치).
- **과거 워크로그는 보존**: `2026-06-17`·`2026-07-01(이벤트화)` 워크로그의 `WxAudio` 표기는 당시 사실 기록이라 그대로 둔다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **VS 프로젝트 파일 재생성**: `Wx.sln`/`Intermediate/ProjectFiles`가 아직 구 모듈(WxAudio)을 참조 — 빌드엔 무관(UBT는 `.uplugin`/`.Build.cs`를 직접 스캔)하나 IDE 편의를 위해 `/generate-project-files` 권장.
- **README 내용 freshness**: 리네임으로 이름/경로는 맞췄으나, 이전 이벤트화의 "주기 타이머" 서술 등은 여전히 stale — `/readme-writer`로 정식 갱신 권장.
- **에디터 로드 검증**: 에디터 재실행 시 `WxSound` 플러그인 정상 로드·`UWxMusicSubsystem` 동작 확인은 BGM 콘텐츠 배선과 함께 수동으로.
