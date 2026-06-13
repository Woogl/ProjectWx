# CLAUDE.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 작업 워크플로우

코드를 작성·수정하는 작업은 다음 절차를 반드시 따른다.

1. **착수 전 plan 수립**: plan mode로 진입해 계획을 세우고 사용자에게 제시한다.
2. **승인 게이트**: 사용자가 plan을 검토·승인하기 전에는 어떤 코드도 작성하지 않는다.
3. **plan 문서화**: 승인 직후, 코드 작성에 들어가기 전에 가장 먼저 `.claude/plans/`에 plan을 기록한다. (plan mode에서는 파일 쓰기가 막히므로 반드시 승인 후 기록한다.)
   * 파일명: `YYYY-MM-DD-제목.md`
   * 구성: 목표 / 변경 범위(파일·모듈) / 접근 방식 / 기각한 대안. 1페이지를 넘기지 않는다.

예외: 읽기 전용 작업(질의·탐색·분석)과 명백히 사소한 단일 수정(오타·주석·로그 문구 등)은 plan 없이 진행할 수 있다. 판단이 애매하면 plan을 거친다.

---

## 게임 스펙

| 항목       | 내용                |
| -------- | ----------------- |
| Engine   | Unreal Engine 5.7 |
| Platform | PC |
| Player   | 최대 4인 |

---

## 코딩 규칙

1. Unreal Engine 5의 공식 코딩 컨벤션을 따른다.

2. Unreal 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
3. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
4. 람다식은 delegate one-shot 바인딩 등 명시적 named function 작성이 과한 경우에만 사용한다. 알고리즘 술어 등은 named function을 선호한다.

5. if-else 문의 실행 블록은 반드시 중괄호`{}`를 사용한다.
  
6. 함수 override 시, `Super::`로 부모 클래스의 함수를 호출한다.
   
7. 모든 Gameplay Tag는 C++ Native Tag로 선언한다.

8. Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

9. `BlueprintCallable` 지정자는 Blueprint Function Library, Blueprint Async Action의 팩토리 함수에서만 사용한다.

10. 모든 코드는 UE 5.7 API에서 검증되어야하며, Deprecated 처리된 API는 사용하지 않는다.

11. 모든 소스 파일의 인코딩은 UTF-8 (No BOM) 을 사용한다.

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
| `WxAI`       | 도메인            | AI 시스템                         |
| `WxQuest`    | 도메인            | 퀘스트 시스템                     |
| `WxSave`     | 도메인            | 세이브/로드 시스템                |

---

## Blueprint 분석

`.uasset`(BP/WBP) 의 내부 구조를 알아야 하는 작업(예: BP 클래스의 디폴트값 확인, 컴포넌트 트리, 이벤트 그래프 로직, WBP의 위젯 계층/MVVM 바인딩)에서는 `Plugins/WxBlueprintSnapshot/Snapshots/` 아래의 동명 JSON을 우선 참조한다.

* 스냅샷은 `Content/` 폴더 구조를 미러링한다. 예: `Content/Game/Character/Player/BP_Player.uasset` → `Plugins/WxBlueprintSnapshot/Snapshots/Game/Character/Player/BP_Player.json`
* 스냅샷은 BP 저장 시 자동 갱신되므로 최신 상태에 가깝지만, 마지막 저장 이후 수정사항은 반영되지 않을 수 있다.
* 스냅샷이 없는 BP는 IncludeDirectories/ExcludeDirectories 필터로 제외된 경우이거나 아직 저장된 적이 없는 경우다.
