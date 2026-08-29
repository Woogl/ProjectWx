# WxGame — 코드 리뷰

> Experience 부팅과 캐릭터·MVVM 접착의 책임은 대체로 명확하고, 수명 정리와 권한 검증에도 방어 코드가 갖춰져 있다. 이번에는 README, Build.cs, Framework·Character·Controller·Ability·MVVM의 핵심 구현과 나머지 66개 C++ 헤더·소스를 함께 검토했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 0 |

## 결과

### 1. 🔴 GameFeature 활성 실패 뒤에도 Experience를 로드 완료로 발행한다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:251`
- **범주**: 버그/정확성
- **문제**: `HandleGameFeaturePluginLoaded`는 활성화 결과가 오류여도 로그만 남기고 카운터를 감소시킨다. 마지막 요청의 콜백이면 `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:258`에서 그대로 `FinishExperienceLoad()`를 호출하고, 이 함수는 `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:291`에서 상태를 `Loaded`로 바꾼 뒤 GameMode의 폰 스폰·기본 인벤토리 지급 대기를 해제한다. 필수 GameFeature가 비활성인 상태로 프레임워크 컴포넌트나 콘텐츠 의존 기능 없이 게임플레이를 시작하므로, 설정·패키징 오류가 즉시 실패하지 않고 불완전한 세션으로 전파된다.
- **제안**: 실패 결과를 누적해 로드를 실패 상태로 전이하고, 필수 플러그인 활성 실패 시 `OnExperienceLoaded`를 방송하거나 폰 스폰을 해제하지 않는다. 선택 기능을 허용하려면 Experience 데이터에 필수·선택 여부를 명시해 선택 항목만 계속 진행한다.
- **확신도**: 높음

### 2. 🟡 HUD 클래스가 HUD 주입 액션보다 늦게 발행된다
- **위치**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:280`
- **범주**: 버그/정확성
- **문제**: `FinishExperienceLoad`는 액션을 먼저 활성화하고, HUD 클래스는 그 뒤 `Source/WxGame/Framework/WxExperienceManagerComponent.cpp:289`에서 UI 매니저에 넣는다. 액션이 HUD 컴포넌트를 동기 주입하고 클라이언트가 이미 폰을 빙의한 상태라면, HUD 컴포넌트의 초기화가 아직 비어 있거나 이전 세계에서 비운 HUD 클래스를 읽을 수 있다. 이후 HUD 클래스 발행 자체는 기존 컴포넌트의 초기화를 재실행하지 않으므로 해당 클라이언트의 HUD가 다음 폰 변경까지 빠질 수 있다.
- **제안**: `WxPublishGameHUDClass(this, CurrentExperience)`를 액션 활성화 루프보다 앞으로 옮겨, HUD를 읽는 모든 액션이 확정된 값을 보게 한다.
- **확신도**: 중간

### 3. 🟡 MetaHuman 리더 메시의 애니메이션 틱 설정을 해제 시 복원하지 않는다
- **위치**: `Source/WxGame/Character/WxMetaHumanComponent.cpp:54`
- **범주**: 성능/안전
- **문제**: 바디를 조립할 때 리더 메시를 숨기고 `AlwaysTickPoseAndRefreshBones`로 변경하지만, `Source/WxGame/Character/WxMetaHumanComponent.cpp:127`의 `OnUnregister`는 가시성만 복원한다. BP 리컴파일·레벨 스트리밍처럼 등록과 해제가 반복된 뒤에도 리더 메시가 화면 밖에서 항상 포즈와 본을 갱신하므로, 부착물을 제거한 상태에서도 불필요한 스켈레탈 평가 비용이 남는다.
- **제안**: 변경 전 `VisibilityBasedAnimTickOption`을 저장해 `OnUnregister`에서 가시성과 함께 복원한다.
- **확신도**: 높음

### 4. 🟡 ViewModel 일반 메서드를 `BlueprintCallable`로 노출한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_Dialogue.h:29`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:44`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h:47`, `Source/WxGame/MVVM/WxViewModel_Inventory.h:83`, `Source/WxGame/MVVM/WxViewModel_Item.h:47`
- **범주**: 규칙 위반
- **문제**: 프로젝트 규칙은 `BlueprintCallable`을 Blueprint Function Library와 Blueprint Async Action의 팩토리 함수에만 허용한다. 위 다섯 함수는 MVVM ViewModel의 명령·세터이며 해당 예외에 해당하지 않아 규칙과 불일치한다.
- **제안**: 현재 규칙을 유지한다면 ViewModel 명령 노출 방식을 Blueprint 이벤트·바인딩 설계로 바꾸고 지정자를 제거한다. ViewModel 명령 호출을 허용할 의도라면 AGENTS.md에 범위를 명시적으로 추가해 규칙을 갱신한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxEnemyController.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`
- **훑은 파일**: `Source/WxGame/README.md`, `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/WxGame.cpp`, `Source/WxGame/Framework/WxExperienceManagerComponent.h`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxBossCharacter.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/Input/WxInputConfig.cpp`, 나머지 MVVM 헤더·구현
- **미검토 / 한계**: BP/WBP 내부 바인딩과 Experience·GameFeature 데이터 에셋의 실제 조합은 범위 밖이다. 도메인 플러그인은 이 모듈이 호출하는 계약만 교차 확인했으며 내부 구현은 리뷰하지 않았다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 66파일 — `/module-review`로 갱신*
