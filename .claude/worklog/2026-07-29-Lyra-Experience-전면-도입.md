# Lyra Experience 전면 도입

## 계획

### 목표
미니게임·사이드미션 같은 콘텐츠를 자유롭게 넣고 뺄 수 있도록, 2026-07-27 축소 이식 때 생략했던 Lyra Experience의 확장 축(GameFeature 플러그인·비동기 로드·GameFeatureAction·ActionSet·PawnData·WorldSettings 선택)을 전면 도입한다. 매치메이킹·세션 등 무관 요소는 배제한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxExperienceManagerComponent.h/.cpp` | GameState 서브오브젝트. Experience 복제·비동기 로드 상태머신·GF 활성·액션 실행·로드 완료 후크 | 신규 |
| `Source/WxGame/Framework/WxExperienceActionSet.h/.cpp` | 액션 묶음 프라이머리 데이터 에셋 | 신규 |
| `Source/WxGame/Framework/WxExperienceManager.h/.cpp` | 엔진 서브시스템. PIE 다중 세션의 GF 활성 카운팅 | 신규 |
| `Source/WxGame/Framework/WxExperienceDefinition.h/.cpp` | UPrimaryDataAsset 승격, GameFeaturesToEnable·Actions·ActionSets(·3단계 DefaultPawnData). FrameworkComponents 삭제 | 수정 |
| `Source/WxGame/Framework/WxGameState.h/.cpp` | Experience 적용 로직 제거, 매니저 컴포넌트 서브오브젝트화 | 수정 |
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | Experience 확정 3단 해석, 폰 지연 스폰, 지급·리스타트를 로드 완료 콜백으로 이관 | 수정 |
| `Plugins/WxSave/.../WxPlayerSpawnComponent.cpp` | 등록 시점 캐치업(로드 전 로그인한 접속자의 StartSpot 심기 보정) | 수정 |
| `Source/WxGame/Player/WxPlayerState.h/.cpp`, `Character/WxCharacterBase.h/.cpp` | 프레임워크 컴포넌트 receiver 등록 추가 | 수정 |
| `Source/WxGame/Framework/WxPawnData.h/.cpp`, `WxWorldSettings.h/.cpp` | 3단계: 폰 클래스 데이터·맵별 기본 Experience | 신규 |
| `Wx.uproject`, `WxGame.Build.cs`, `Config/DefaultGame.ini`, `Config/DefaultEngine.ini` | GameFeatures 활성·의존, 프라이머리 에셋 스캔, CoreRedirect, WorldSettings 클래스 | 수정 |
| `Plugins/GameFeatures/WxGFMiniGame/` | 2단계: 파이프라인 실증용 콘텐츠 온리 GF 플러그인 | 신규 |
| `Content/Framework/WAS_CoreGameplay·EXP_MiniGame·PawnData_Player.uasset` 등 | 코어 6종 주입 액션셋·샘플 Experience·폰 데이터 | 신규·수정 |

### 접근 방식
- **선택 → 로드 → 적용의 3분리**: GameMode가 URL(?Experience=) → WorldSettings → 자체 폴백 순으로 Experience를 확정해 GameState의 매니저 컴포넌트에 넘긴다. 매니저는 서버·클라 공통 상태머신(에셋 번들 비동기 로드 → GF 플러그인 활성 → 월드 한정 컨텍스트로 액션 실행 → 완료 브로드캐스트)을 주행한다. 클라는 복제 OnRep으로 같은 경로를 탄다.
- **주입의 액션화**: 자체 컴포넌트 목록·receiver 추론을 폐기하고 엔진 스톡 AddComponents 액션으로 이전한다. Experience 본체와 GF 플러그인이 같은 액션 타입을 쓰므로 콘텐츠를 에셋 이동만으로 GF로 승격할 수 있다.
- **동기 전제의 해체**: 기존 "로그인 전 주입 완료" 전제를 폰 지연 스폰(로드 완료까지 스타트 스킵 후 일괄 리스타트)과 스폰 컴포넌트의 등록 시점 캐치업으로 대체한다.
- **유지되는 기존 결정**: 사이드 플래그 미사용(엔진 규칙+컴포넌트 자기 가드), 수동 receiver 등록 관례(ModularGameplayActors 미도입), 인스턴스 에셋(BP CDO 아님).
- **단계 분할**: 1단계 코어 전환 → 2단계 샘플 GF 플러그인 실증(+GF 참조 규약 명문화) → 3단계 PawnData·WorldSettings. 단계마다 빌드·PIE 검증.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Framework/WxExperienceManagerComponent.h/.cpp` | 로드 상태머신·복제·GF 활성·액션 실행·완료 후크 | 신규 |
| `Framework/WxExperienceActionSet.h/.cpp`, `WxExperienceManager.h/.cpp`, `WxPawnData.h`, `WxWorldSettings.h/.cpp` | 액션셋·PIE GF 카운팅 서브시스템·폰 데이터·월드 세팅 | 신규 |
| `Framework/WxExperienceDefinition.h/.cpp` | UPrimaryDataAsset 승격, GF 목록·액션·액션셋·PawnData, 컴포넌트 목록 삭제 | 수정 |
| `Framework/WxGameState.h/.cpp` | 적용 로직 제거, 매니저 서브오브젝트화 | 수정 |
| `Framework/WxGameMode.h/.cpp` | 3단 해석(URL→WorldSettings→폴백), 폰 지연 스폰, 완료 콜백 일괄 스폰·지급, PawnData 폰 클래스 | 수정 |
| `Player/WxPlayerState.h/.cpp`, `Character/WxCharacterBase.h/.cpp` | receiver 등록 추가 | 수정 |
| `Plugins/WxSave/.../WxPlayerSpawnComponent.h/.cpp` | OnRegister 캐치업(부착이 로그인보다 늦는 경우) | 수정 |
| `Source/WxEditor/*` | BeginPIE에서 GF 카운터 리셋, WxGame 의존 추가 | 수정 |
| `Wx.uproject`, `WxGame.Build.cs`, `Config/*.ini` | GameFeatures 활성·의존, 스캔 3종, CoreRedirect, WorldSettings 클래스 | 수정 |
| `Plugins/GameFeatures/WxGFMiniGame/` | 콘텐츠 온리 GF 샘플(ExplicitlyLoaded·Registered·EnabledByDefault) | 신규 |
| `Content/Framework/WAS_CoreGameplay·EXP_MiniGame·PawnData_Player.uasset`, `EXP_Combat`, `GM_Combat`, 두 맵 | 코어 6종 주입 액션셋, 샘플 Experience, 폰 데이터, 참조 갱신, WorldSettings 재저장 | 신규·수정 |
| `.claude/CLAUDE.md`, `Source/WxGame/README.md` | GF 플러그인 참조 규약·Experience 서술 갱신 | 수정 |

### 구현·결정과 그 이유
- **주입의 액션화**: 자체 컴포넌트 목록과 receiver 추론을 엔진 스톡 AddComponents 액션으로 대체했다. Experience 본체와 GF 플러그인이 같은 액션 타입을 쓰므로 콘텐츠를 에셋 이동만으로 GF로 승격할 수 있고, 죽은 추론 분기 문제가 코드째 사라졌다. 사이드 플래그는 기본값(양측 true) 고정 규약으로 두어 7/28 결정(컴포넌트 자기 가드)을 유지했다.
- **동기 전제의 해체**: 로드가 비동기가 되면서 "로그인 전 주입 완료" 전제를 폰 지연 스폰(완료까지 스타트 스킵 후 일괄 리스타트)과 스폰 컴포넌트의 등록 시점 캐치업으로 대체했다. 시작 지급은 완료 전 접속자=일괄 경로 / 완료 후 접속자=PostLogin 경로로 상호 배타라 이중 지급이 없다.
- **PIE 카운팅 서브시스템 채택**: 리슨서버+클라 PIE 검증이 상시라, 한 세션 종료가 다른 세션의 GF를 내리는 문제를 원형대로 이식해 막았다(다중 월드 1회 PIE에서 Active→Loaded 전이로 검증됨).
- **인스턴스 에셋 유지**: Lyra의 BP CDO 방식 대신 네이티브 클래스 인스턴스로 통일 — 기존 검증된 복제 경로를 유지하고 PrimaryAssetId 기본 동작으로 충분해서다. BP 서브클래스 인스턴스 금지를 클래스 주석에 명시했다.
- **GM 프로퍼티 개명은 CoreRedirect로 보존** — GM_Combat 값이 리다이렉트로 유지됨을 확인 후 재저장했다.
- **GF 템플릿 함정 둘**: 생성 직후엔 서브시스템 미등록이라 재시작이 필요하고, `EnabledByDefault:false`·`BuiltInInitialFeatureState:Active` 기본값을 각각 true/Registered로 바꿔야 규약대로 동작한다.

### 계획 대비 달라진 점
- PIE에서 `?Experience=` URL 옵션을 걸 방법이 없어(에디터 세팅은 별도 프로세스 전용) URL 분기는 스탠드얼론 `-game` 실행으로 종단 검증했고, 에디터 검증은 GM 폴백·WorldSettings 분기로 수행했다.
- 나머지는 계획대로. 검증: 빌드 3회 전부 성공, PIE(싱글·리슨+클라)에서 상태 전이·6종 주입·GF 활성/비활성·디렉터 주입·PawnData 스폰 교체·재진입 멱등·에러/ensure 없음 확인.

### 후속 과제
- 로그인이 로드 완료보다 앞서는 실제 비동기 시나리오(쿠킹 빌드·데디+클라)의 캐치업 실검증 — 에디터에선 에셋이 메모리에 있어 같은 프레임에 완료된다.
- HUD·사망화면의 Experience 이관(이번에 생긴 CallOrRegister 후크로 보류 조건 해소), 로딩 스크린.
- 세이브 재개 지점·사망 부활 경로의 수동 플레이 확인(구조상 순서 보장은 검증됨).
