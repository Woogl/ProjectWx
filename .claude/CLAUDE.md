# CLAUDE.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 작업 워크플로우

코드를 작성·수정하는 작업은 다음 흐름을 따른다(시간 순).

1. **설계 → 승인 (plan mode)**: plan mode로 접근 방식을 정리·제시하고, 승인(ExitPlanMode) 전에는 어떤 코드도 작성하지 않는다.

2. **계획 기록**: 승인 직후·코드 착수 전, `.claude/worklog/_TEMPLATE.md`를 복사한 `YYYY-MM-DD-제목.md`에 「계획」을 채운다. 승인받은 그 plan이 곧 「계획」 내용이다. (plan mode에선 파일 쓰기가 막히므로 승인 후 기록한다.)

3. **구현**: 코드를 작성·수정한다.

4. **검증**: WxEditor(Development) 타겟을 빌드해 컴파일을 확인한다. 엔진 경로는 `Wx.uproject`의 `EngineAssociation`에서 해석한다. 에디터 재실행은 불필요, 실패 시 `build-doctor`.

5. **완료 기록**: 검증 후, 같은 파일의 「완료」를 채운다.

예외: 동작·구조를 바꾸지 않는 사소한 작업(오타·주석·로그 문구 등)은 절차 없이 진행할 수 있다. 동작·구조를 바꾸는 작업은 비사소로 보아 절차를 따르며, 애매하면 절차를 택한다.

---

## 코딩 규칙

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
3. 람다식은 반드시 필요한 경우에만 사용한다.

4. Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

5. `BlueprintCallable` 지정자는 Blueprint Function Library, Blueprint Async Action의 팩토리 함수에서만 사용한다.

6. 인라인 함수 정의를 금지한다. (`FORCEINLINE` 등)

---

## 모듈 및 플러그인 규칙

* 게임의 주요 시스템은 Unreal Engine Plugin 단위로 분리하여 개발한다.
* 모든 `Wx` 플러그인은 `WxCore`를 제외한 다른 플러그인을 참조하면 안된다.
* 예외: `Plugins/GameFeatures/` 아래의 GameFeature 플러그인(콘텐츠 분류)은 DAG 최상단으로, `WxGame`과 도메인 플러그인을 참조할 수 있다. 역방향(코드·에셋이 GameFeature 플러그인을 참조)은 금지하며, Experience 에셋의 `GameFeaturesToEnable` 이름 문자열만 예외다.
* GameFeature 플러그인은 `ExplicitlyLoaded=true`, `BuiltInInitialFeatureState=Registered`로 만든다 — 발견은 되되 Experience가 켤 때까지 비활성. 이름은 GF 표식 없이 `Wx`+콘텐츠명으로 짓는다(예: WxFishing).
* 플러그인 분류는 아래 표에 명시한다.

| Module       | 분류              | Description                       |
| ------------ | ----------------- | --------------------------------- |
| `WxGame`     | 게임 모듈         | 기본 게임 모듈 (플러그인 아님)    |
| `WxCore`     | 도메인 (foundation) | 공용 정의        |
| `WxCombat`   | 도메인            | 전투 시스템                       |
| `WxInventory`| 도메인            | 인벤토리 시스템                   |
| `WxUI`       | 도메인            | UI 시스템                         |
| `WxWorld`    | 도메인            | 월드 오브젝트 및 상호작용         |
| `WxAI`       | 도메인            | AI 시스템                         |
| `WxDialogue` | 도메인            | 대화 시스템                       |
| `WxQuest`    | 도메인            | 퀘스트 시스템                     |
| `WxSave`     | 도메인            | 세이브/로드 시스템                |
