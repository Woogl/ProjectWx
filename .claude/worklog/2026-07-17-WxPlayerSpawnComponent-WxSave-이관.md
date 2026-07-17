# WxPlayerSpawnComponent 를 WxSave 플러그인으로 이관

## 계획

### 목표

`UWxPlayerSpawnComponent` 는 저장된 재개 지점을 로그인 시 `StartSpot` 으로 심고 빙의 시 저장 스탯을 복원한다. 현재 `Source/WxGame/Framework/` 에 있으나 이 컴포넌트가 하는 일이 곧 세이브 복원이므로 WxSave 플러그인에 둔다. 동작 변경은 없고 파일 이동과 모듈 배선 조정이다.

WxCombat 에 선례가 있다 — `WXCOMBAT_API UWxTimeDilationComponent : UGameStateComponent` 가 플러그인 안에 있고 `WxCombat.Build.cs` 가 `ModularGameplay` 를 의존한다. ModularGameplay 는 엔진 플러그인이라 "플러그인은 WxCore 외 다른 플러그인을 참조하지 않는다" 규칙에 저촉되지 않는다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`<br>`Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp` | `Source/WxGame/Framework/` 에서 `git mv`. WxSave 는 플랫 구조라 `Public/`·`Private/` 직하 | 이동 |
| `Plugins/WxSave/Source/WxSave/WxSave.Build.cs` | `PublicDependencyModuleNames` 에 `"ModularGameplay"` 추가 | 수정 |
| 이동한 `.h` | `WXSAVE_API` 추가 (모듈 경계를 넘으므로 export 필요) | 수정 |
| 이동한 `.cpp` | include 경로 조정, 로그 카테고리 `WxGame.h`/`LogWxGame` → `WxSaveModule.h`/`LogWxSave` | 수정 |

`AWxPlayerController` 의 receiver 등록은 그대로 둔다 — ModularGameplay 인프라일 뿐 WxSave 의존이 아니라 PC 는 여전히 세이브를 모른다. WxGame 은 이미 WxSave 를 의존하므로(`WxCheckPoint`) 배선 추가는 없다.

### 접근 방식

- **`git mv` 로 히스토리 유지**: 파일 내용은 include·API 매크로·로그 카테고리만 바뀐다.
- **`ModularGameplay` 는 Public 의존**: 공개 헤더가 `UControllerComponent` 를 베이스로 노출하므로 Private 로는 부족하다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`<br>`Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp` | `Source/WxGame/Framework/` 에서 이동. `WXSAVE_API` 추가, include 경로 조정, 로그 카테고리 `LogWxGame` → `LogWxSave` | 이동 |
| `Plugins/WxSave/Source/WxSave/WxSave.Build.cs` | `PublicDependencyModuleNames` 에 `ModularGameplay` 추가 | 수정 |
| `Plugins/WxSave/README.md` | 클래스 표에 `UWxPlayerSpawnComponent` 행 추가(에셋 등록 필요 명시) | 수정 |
| `Source/WxGame/README.md` | 읽는 순서의 위임 문구를 `[[WxSave]]의 UWxPlayerSpawnComponent` 로 정정 | 수정 |

### 구현·결정과 그 이유

- **`ModularGameplay` 를 Public 의존으로**: 공개 헤더가 `UControllerComponent` 를 베이스로 노출하므로 Private 로는 부족하다. WxCombat 이 같은 이유로 이미 Public 에 두고 있다(`UWxTimeDilationComponent : UGameStateComponent`).

- **`WXSAVE_API` 추가**: WxGame 안에 있을 땐 불필요했지만 플러그인 모듈 경계를 넘으므로 export 가 필요하다.

- **PC 의 receiver 등록은 그대로**: `AddGameFrameworkComponentReceiver` 는 ModularGameplay 인프라일 뿐 WxSave 의존이 아니다. 이관 후에도 `AWxPlayerController` 는 세이브를 모른다 — WxGame 에서 WxSave 를 아는 파일은 `WxCheckPoint.cpp` 하나로 줄었다.

### 계획 대비 달라진 점
- **README 2건 갱신을 추가했다**(계획에 없었음). 이동으로 WxGame README 의 "읽는 순서" 가 없는 파일을 가리키게 됐고, WxSave README 클래스 표엔 새 식구가 빠져 있었다. 이동이 만든 문서 불일치라 함께 고쳤다.

### 후속 과제
- **에셋 등록 필요(미완)**: `GM_Combat`·`GM_ChangYoung` 의 `FrameworkComponents` 에 `WxPlayerSpawnComponent` 등록. 이번 이관으로 클래스 경로가 `/Script/WxGame` → `/Script/WxSave` 가 됐으나 아직 등록 전이라 재지정할 기존 참조는 없다. 죽은 `WxPlayerSpawningComponent` 빈 엔트리 정리도 같은 화면에서.
- **런타임 미검증**: 이동뿐이라 동작은 그대로여야 하지만, 에셋 등록 후 확인이 남았다 — 체크포인트 저장 → 사망/로드 재개, 신규 세션, PIE "여기서 플레이"(위치 무시·스탯 복원), WP 레벨.
- **멀티플레이는 데이터 모델이 싱글 전제**: 배선(authority 게이트·자기-PC 필터)은 안전하나 세이브가 플레이어 1명분(`GetFirstPlayerController` 기준)이라 2명 이상이면 같은 지점·같은 스탯이 된다. 기존 한계로 이번 변경과 무관.
