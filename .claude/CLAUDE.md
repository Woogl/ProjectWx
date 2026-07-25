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

4. **검증**: WxEditor(Development) 타겟을 빌드해 컴파일을 확인한다. 엔진 경로는 `Wx.uproject`의 `EngineAssociation`에서 해석하며, 설치 폴더 존재 여부로 짐작하지 않는다(`build-doctor` 스킬의 「Build command」가 이 해석을 포함한 표준 명령이다). 에디터 재실행은 불필요, 실패 시 `build-doctor`.

5. **완료 기록**: 검증 후, 같은 파일의 「완료」를 채운다.

예외: 동작·구조를 바꾸지 않는 사소한 작업(오타·주석·로그 문구 등)은 절차 없이 진행할 수 있다. 동작·구조를 바꾸는 작업은 비사소로 보아 절차를 따르며, 애매하면 절차를 택한다.

---

## 코딩 규칙

1. Unreal Engine 5의 공식 코딩 컨벤션을 따른다.

2. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
3. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
4. 람다식은 반드시 필요한 경우에만 사용한다.
  
5. 함수 override 시, `Super::`로 부모 클래스의 함수를 호출한다.
   
6. Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

7. `BlueprintCallable` 지정자는 Blueprint Function Library, Blueprint Async Action의 팩토리 함수에서만 사용한다.

---

## 모듈 및 플러그인 규칙

* 게임의 주요 시스템은 Unreal Engine Plugin 단위로 분리하여 개발한다.
* 모든 플러그인은 `WxCore`를 제외한 다른 플러그인을 참조하면 안된다.
* 플러그인 분류는 아래 표에 명시한다.

| Module       | 분류              | Description                       |
| ------------ | ----------------- | --------------------------------- |
| `WxGame`     | 게임 모듈         | 기본 게임 모듈 (플러그인 아님)    |
| `WxCore`     | 도메인 (foundation) | 공용 정의        |
| `WxCombat`   | 도메인            | 전투 시스템                       |
| `WxInventory`| 도메인            | 인벤토리 시스템                   |
| `WxUI`       | 도메인            | UI 시스템                         |
| `WxWorld`    | 도메인            | 월드 오브젝트 및 상호작용         |
| `WxSound`    | 도메인            | 오디오/음악 (Chooser 기반 BGM)    |
| `WxAI`       | 도메인            | AI 시스템                         |
| `WxQuest`    | 도메인            | 퀘스트 시스템                     |
| `WxSave`     | 도메인            | 세이브/로드 시스템                |

---

## Blueprint 분석

`.uasset`(BP/WBP) 의 내부 구조를 알아야 하는 작업(예: BP 클래스의 디폴트값 확인, 컴포넌트 트리, 이벤트 그래프 로직, WBP의 위젯 계층/MVVM 바인딩)에서는 `Plugins/WxBlueprintSnapshot/Snapshots/` 아래의 동명 JSON을 우선 참조한다.

* 스냅샷은 `Content/` 폴더 구조를 미러링한다. 예: `Content/Game/Character/Player/BP_Player.uasset` → `Plugins/WxBlueprintSnapshot/Snapshots/Game/Character/Player/BP_Player.json`
* 스냅샷은 BP 저장 시 자동 갱신되므로 최신 상태에 가깝지만, 마지막 저장 이후 수정사항은 반영되지 않을 수 있다.
* 스냅샷이 없는 BP는 IncludeDirectories/ExcludeDirectories 필터로 제외된 경우이거나 아직 저장된 적이 없는 경우다.
