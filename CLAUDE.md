# CLAUDE.md

## 기본 지시사항
너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 클라이언트 프로그래머 역할을 수행한다.


## 게임 스펙

| 항목       | 내용                |
| -------- | ----------------- |
| Engine   | Unreal Engine 5.7 |
| Platform | PC, Console (PS5) |
| Player   | Single Player     |



## 코딩 규칙

1. Unreal Engine 5의 공식 코딩 컨벤션을 따른다.

2. Unreal 기본 Prefix 앞에 `Wx` 접두어를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)

3. 모든 Gameplay Tag는 C++ Native Tag로 선언한다. 태그 정의는 `WxCore` 플러그인에서 관리한다.

4. 함수 선언 시 줄바꿈을 하지 않는다.

5. Delegate에 바인딩되는 Callback 함수는 `Handle`을 Prefix로 사용한다. (예시: `HandleMontageEnded`, `HandleDeath`)

6. `BlueprintCallable` 지정자는 Blueprint Function Library 내부에서만 사용한다.

7. `.h` 파일에서 inline 함수 정의를 금지한다.

8. 람다식은 반드시 필요한 경우에만 사용한다.

9. if-else 문의 실행 블록은 반드시 중괄호`{}`를 사용한다.


## 모듈 및 플러그인 구조

* 게임의 주요 시스템은 Unreal Engine Plugin 단위로 분리하여 개발한다.
* 모든 플러그인은 `WxCore`에 의존할 수 있다. `WxCore`를 제외한 플러그인 간 직접 의존은 금지한다.

| Module     | 설명                                |
| ---------- | --------------------------------- |
| `Wx`       | 기본 게임 모듈. 아래 플러그인들을 사용            |
| `WxCore`   | 공용 정의 (Gameplay Tag, Interface 등) |
| `WxCombat` | 전투 시스템                            |
| `WxUI`     | UI 시스템                            |


### 전투 시스템 (`WxCombat`)

전투 시스템은 Unreal Engine 5 의 Gameplay Ability System (GAS) 기반으로 구현한다.

#### 주요 구성 요소
* Character
  * Player Character
  * Enemy Character
* Ability System
  * Ability System Component
  * Gameplay Ability
  * Gameplay Effect
  * AttributeSet
* Weapon System
  * Weapon
  * Projectile


### UI 시스템 (`WxUI`)
추후 개발 예정이다.
