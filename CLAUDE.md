# CLAUDE.md

## 기본 지시사항
너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 클라이언트 프로그래머 역할을 수행한다.


## 게임 스펙

| 항목       | 내용                |
| -------- | ----------------- |
| Engine   | Unreal Engine 5.7 |
| Platform | PC |
| Player   | Single Player     |



## 코딩 규칙

1. Unreal Engine 5의 공식 코딩 컨벤션을 따른다.

2. Unreal 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
3. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.

4. 함수 선언 시 줄바꿈을 하지 않는다.
   
5. `.h` 파일에서 inline 함수 정의를 금지한다.
   
6. 람다식은 반드시 필요한 경우에만 사용한다.

7.  if-else 문의 실행 블록은 반드시 중괄호`{}`를 사용한다.
   
8. 모든 Gameplay Tag는 C++ Native Tag로 선언한다. 태그 정의는 `WxCore` 플러그인에서 관리한다.

9.  Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

10. `BlueprintCallable` 지정자는 Blueprint Function Library 내부에서만 사용한다.

11. 연속된 `UFUNCTION()` 또는 `UPROPERTY()` 선언 사이에는 빈 줄을 삽입한다.


## 모듈 및 플러그인 구조

* 게임의 주요 시스템은 Unreal Engine Plugin 단위로 분리하여 개발한다.
* 모든 플러그인은 `WxCore`에 의존할 수 있다.
* `WxCore`를 제외한 플러그인 간 직접 의존은 금지한다.

| Module     | Description                                |
| ---------- | --------------------------------- |
| `WxGame`   | 기본 게임 모듈  |
| `WxCore`   | 공용 정의  |
| `WxCombat` | 전투 시스템                            |
| `WxUI`     | UI 시스템                            |


### 기본 게임 모듈 (`WxGame`)

다른 플러그인들을 사용해 구체적인 게임 컨텐츠를 만든다.

#### 주요 구성 요소
* Character
  * Character Base
  * Player Character
  * Enemy Character
* Controller
  * Player Controller
  * Enemy Controller
* Component
  * Ragdoll Component

### 공용 정의 (`WxCore`)

프로젝트 전체 플러그인이 공유하는 공용 정의를 관리한다. (Gameplay Tag, Enum 등)


### 전투 시스템 (`WxCombat`)

전투 시스템은 Unreal Engine 5 의 Gameplay Ability System (GAS) 기반으로 구현한다.

#### 주요 구성 요소
* Ability System
  * Ability System Component
  * Ability Set
  * Attribute Set
  * Ability
  * Effect
* Weapon
  * Weapon
  * Projectile


### UI 시스템 (`WxUI`)
추후 개발 예정.

<!-- 나중에 개발 시작할 때 주석 해제 예정
UI 시스템은 Unreal Engine 5 의 Common UI 기반으로 멀티플랫폼을 고려하여 구현한다.

Unreal Engine 5의 UMG MVVM을 사용하여, 비즈니스 로직과 프레젠테이션 로직을 완전히 분리해야한다.

#### 주요 구성 요소
* System
  * UI Manager Subsystem
  * Primary Game Layout
  * UI Library
* Widget
  * Activatable Widget
  * Confirmation Widget
  * Notification Entry Widget
  * Button
  * Action Button
* MVVM
  * View Model
    * Character View Model
    * Ability View Model
    * Effect View Model
* Action
  * AsyncAction_PushContentToLayer
  * AsyncAction_ShowConfirmation
  * AsyncAction_ShowNotification -->