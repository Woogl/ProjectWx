# CLAUDE.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머입니다.

모든 응답은 한국어로 작성한다.

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

4. 함수 선언 시 줄바꿈을 하지 않는다.
   
5. `.h` 파일에서 inline 함수 정의를 금지한다. `FORCEINLINE`도 금지한다. (단, 템플릿 함수와 `constexpr` 상수는 예외)
   
6. 람다식은 delegate one-shot 바인딩 등 명시적 named function 작성이 과한 경우에만 사용한다. 알고리즘 술어 등은 named function을 선호한다.

7. if-else 문의 실행 블록은 반드시 중괄호`{}`를 사용한다.
  
8. 함수 override 시, `Super::`로 부모 클래스의 함수를 호출한다.
   
9. 모든 Gameplay Tag는 C++ Native Tag로 선언한다.

10. Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

11. `BlueprintCallable` 지정자는 Blueprint Function Library, Blueprint Async Action의 팩토리 함수에서만 사용한다.

12. 연속된 `UFUNCTION()` 또는 `UPROPERTY()` 선언 사이에는 빈 줄을 삽입한다.

13. 모든 코드는 UE 5.7 API에서 검증되어야하며, Deprecated 처리된 API는 사용하지 않는다.

14. 모든 소스 파일의 인코딩은 UTF-8 (No BOM) 을 사용한다.

15. 코드 작성 후 빌드가 성공하는지 반드시 점검한다. 빌드 명령어: `"BatchFiles/BuildProjectFiles.bat"`

---

## 모듈 및 플러그인 규칙

* 게임의 주요 시스템은 Unreal Engine Plugin 단위로 분리하여 개발한다.
* 모든 플러그인은 다음 두 분류 중 하나다.
  * **도메인 플러그인** — 특정 시스템을 제공. `WxCore` 외 다른 플러그인에 의존하지 않는다.
  * **통합 플러그인** — 여러 도메인 플러그인을 소비하여 cross-cutting 기능을 제공. 모든 플러그인에 의존할 수 있으나, 어떤 플러그인도 통합 플러그인에 의존해서는 안 된다(단방향 leaf).
* 플러그인 분류는 아래 표에 명시한다.

| Module       | 분류              | Description                       |
| ------------ | ----------------- | --------------------------------- |
| `WxGame`     | 게임 모듈         | 기본 게임 모듈 (플러그인 아님)    |
| `WxCore`     | 도메인 (foundation) | 공용 정의, 공용 유틸리티        |
| `WxCombat`   | 도메인            | 전투 시스템                       |
| `WxInventory`| 도메인            | 인벤토리 시스템                   |
| `WxUI`       | 도메인            | UI 시스템                         |
| `WxWorld`    | 도메인            | 월드 오브젝트 및 상호작용         |
| `WxAI`       | 도메인            | AI 시스템                         |
| `WxQuest`    | 도메인            | 퀘스트 시스템                     |
| `WxSave`     | 도메인            | 세이브/로드 시스템                |

### 기본 게임 모듈 (`WxGame`)

다른 플러그인들을 사용해 구체적인 게임 컨텐츠를 만든다.

### 공용 기능 (`WxCore`)

프로젝트 전체 플러그인이 공유하는 공용 정의를 관리한다. (Gameplay Tag, Enum 등)

프로젝트 전체 플러그인에서 사용할 수 있는 유틸리티를 관리한다.

### 전투 시스템 (`WxCombat`)

전투 시스템은 Unreal Engine 5 의 Gameplay Ability System (GAS) 기반으로 구현한다.

### UI 시스템 (`WxUI`)

UI 시스템은 Unreal Engine 5 의 CommonUI 기반으로 멀티플랫폼을 고려하여 구현한다.

Unreal Engine 5의 UMG View Model을 사용하여, 비즈니스 로직과 프레젠테이션 로직을 완전히 분리해야한다.

### 월드 시스템 (`WxWorld`)

월드에 배치되는 각종 오브젝트 및 관련 상호작용을 구현한다.

---

## Blueprint 분석

`.uasset`(BP/WBP) 의 내부 구조를 알아야 하는 작업(예: BP 클래스의 디폴트값 확인, 컴포넌트 트리, 이벤트 그래프 로직, WBP의 위젯 계층/MVVM 바인딩)에서는 `Plugins/WxBlueprintSnapshot/Snapshots/` 아래의 동명 JSON을 우선 참조한다.

* 스냅샷은 `Content/` 폴더 구조를 미러링한다. 예: `Content/Game/Character/Player/BP_Player.uasset` → `Plugins/WxBlueprintSnapshot/Snapshots/Game/Character/Player/BP_Player.json`
* 스냅샷은 BP 저장 시 자동 갱신되므로 최신 상태에 가깝지만, 마지막 저장 이후 수정사항은 반영되지 않을 수 있다.
* 스냅샷이 없는 BP는 IncludeDirectories/ExcludeDirectories 필터로 제외된 경우이거나 아직 저장된 적이 없는 경우다.
