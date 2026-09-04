# WxGame — 코드 리뷰

> 조립 모듈답게 각 파일이 얇고 책임이 뚜렷하며, 심각 결함은 이번에도 찾지 못했다. 직전 리뷰(`491dd7ec`)가 지적한 6건 중 `BaseWalkSpeed` 초기화·README 스테일·점프 무적 예측 3건은 닫혔고, 적 역할 컴포넌트를 액터로 흡수한 이번 변경은 구독·해제와 사망·교전 시드 경로가 대칭이라 새 결함을 만들지 않았다. Framework 전 파일, Character 전 파일, MVVM 전 파일, AbilitySystem·Inventory·Controller·Cheat 를 cpp 까지 읽었고, 경계 확인을 위해 `WxUI` 의 뷰모델 베이스와 `WxCombat` 의 `UWxLockOnComponent`·`UWxCombatAttributeSet` 을 함께 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 MetaHuman 컴포넌트가 리더 메시에 건 설정을 `OnUnregister` 에서 되돌리지 않는다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `:84`, `:131`
- **범주**: 성능/안전
- **문제**: `OnRegister` 는 바디를 만들 때 리더 메시를 숨기면서 `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올린다(`:56`). `OnUnregister` 는 `SetVisibility(true)` 만 되돌리고(`:131`) 이 틱 옵션은 원복하지 않아, 부착물이 사라져 리더가 다시 표시 주체가 된 뒤에도 화면 밖에서 본 리프레시가 계속 돈다 — 원래 이 옵션을 피하려던 비용을 그대로 문다. 같은 맥락으로 `BodyComponentName`·`FaceComponentName`(`:84`~`:85`)도 정리되지 않아, 해제 후에는 이미 파괴된 컴포넌트 이름을 가리킨 채 남는다. 레벨 스트리밍 아웃/인이나 BP 리컴파일처럼 등록이 반복되는 경로에서 드러난다. 직전 리뷰의 1번이 그대로 남아 있다.
- **제안**: `OnRegister` 진입 시 리더의 원래 `VisibilityBasedAnimTickOption` 을 기억했다가 `OnUnregister` 에서 복원하고, 두 이름 문자열도 함께 비운다.
- **확신도**: 중간

### 2. 🟡 `AWxCharacterBase` 가 도메인 컴포넌트 10종을 무조건 생성한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:36`~`:53`
- **범주**: 설계/구조
- **문제**: 베이스 생성자가 ASC·어트리뷰트셋·MotionWarping·LockOn·HitStop·ProjectileManager·MinionManager·Equipment·WeaponActor·MetaHuman 을 예외 없이 만든다. 그 결과 발사체도 소환도 하지 않는 잡몹·NPC 폰까지 투사체·소환 매니저를 달고 다닌다. 모듈 README 가 선언한 확장 경로("도메인 컴포넌트를 만들고 Experience 의 AddComponents 액션으로 주입한다")와 실제 구조가 어긋나 있어, 새 기능을 어디에 붙여야 하는지 읽는 사람이 헷갈린다. 이 매니저들은 틱도 복제도 하지 않아 런타임 비용은 크지 않고, 비용은 주로 조직·가독성 쪽이다. `.codex/worklog/2026-09-04-CharacterBase-네이티브-컴포넌트-조합.md` 가 "선택적 Manager 컴포넌트 분리는 BP 마이그레이션 이후 별도 축소 단계로 남겼다"고 적어 둔 항목이고, 그 선행 조건인 BP 이관은 `2026-09-05-AICharacter-EnemyComponent-전체-통합.md`·`2026-09-05-캐릭터-이관-호환층-제거.md` 로 끝나 지금은 착수 가능한 상태다.
- **제안**: 예정된 축소 단계를 진행해 소환·투사체 매니저를 실제로 쓰는 캐릭터에만 남긴다(BP SCS 조립 또는 Experience 주입). 베이스에 남길 것과 뺄 것의 기준을 README 확장 포인트 절에 한 줄로 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 이미 후속 단계로 추적 중인 항목이다)

### 3. 🟢 Enhanced Input 콜백에 `Handle` prefix 가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:112`, `:116`, `:125`, `:130`, `:131` (선언은 `Source/WxGame/Character/WxPlayerCharacter.h:58`~`:63`)
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `BindAction` 으로 묶이는 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 가 이 규칙에서 벗어난 유일한 사례다 — 모듈의 나머지 바인딩 20여 곳은 전부 `Handle` 로 시작한다. 프로젝트 전체에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 관례로 굳은 예외라 보기도 어렵다. (`Jump`·`StopJumping` 은 엔진 `ACharacter` 함수라 대상이 아니다.) 직전 리뷰의 3번이 그대로 남아 있다.
- **제안**: `HandleMove`·`HandleLook`·`HandleToggleCrouch`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased` 로 개명하거나, 입력 액션 핸들러를 규칙 4의 예외로 `CLAUDE.md` 에 명시한다.
- **확신도**: 높음

### 4. 🟢 "초기화 경계에서 서브오브젝트 유효성을 확인한다"는 클래스 규약을 `WeaponActor` 만 지킨다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:32`, `Source/WxGame/Character/WxCharacterBase.cpp:72`, `:91`
- **범주**: 설계/구조
- **문제**: 클래스 doc 이 "생성자 서브오브젝트는 기본적으로 존재하지만, 직렬화된 BP·레벨 인스턴스를 다루는 초기화 경계에서는 유효성을 확인한다"고 규약을 세웠는데, 같은 `PostInitializeComponents` 안에서 `WeaponActor` 만 검사하고(`:91`) `AbilitySystemComponent` 는 그보다 앞선 네 지점(`:72`, `:76`, `:83`, `:86`)에서 무가드로 역참조한다. 이 문구는 `.codex/worklog/2026-09-04-캐릭터-장비-컴포넌트-널-크래시-수정.md` 가 기록한 실제 크래시(컴포넌트 클래스 개명 과정에서 직렬화 인스턴스의 서브오브젝트가 해석되지 않음) 때문에 "널이 아니다"에서 지금 문장으로 완화된 것이라, 같은 사고가 ASC 쪽에서 나면 가드에 닿기도 전에 `:72` 에서 죽는다. 규약과 코드 중 하나가 틀린 상태다.
- **제안**: 둘 중 하나로 정리한다 — (a) 규약대로 초기화 경계에서 ASC 도 함께 검사하거나, (b) 규약 문장을 "개명 이력이 있는 `WeaponActor` 한정"으로 좁히고 나머지는 불변식으로 되돌린다. 어느 쪽이든 결정 이유를 worklog 에 남겨야 2026-09-03 → 09-04 처럼 방향이 다시 뒤집히지 않는다.
- **확신도**: 중간

### 5. 🟢 `bEngaged` 가 `State.Engaged` 태그와 같은 상태를 이중으로 든다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:200`~`:212`, `Source/WxGame/Character/WxEnemyCharacter.h:92`
- **범주**: 중복/복잡도
- **문제**: `SetEngaged` 는 멤버 `bEngaged` 에 쓰고 같은 값을 `State.Engaged` 루즈 태그로도 발행한다. 소비자는 갈라져서 `UWxViewModel_BossCharacter::SetBoss` 는 `IsEngaged()`(멤버)를 읽고 `UWxNameplateComponent` 는 태그를 읽는다. 지금은 쓰는 곳이 이 한 줄뿐이라 어긋날 수 없지만, 교전 상태의 출처가 둘이라 나중에 어느 한쪽만 손대면 조용히 갈라진다. 변화 감지(`bEngagementChanged`)도 태그 카운트를 먼저 읽으면 멤버 없이 성립한다.
- **제안**: `bEngaged` 를 지우고 `IsEngaged()` 와 변화 감지 모두 `ASC->HasMatchingGameplayTag(State_Engaged)` 에서 파생시켜 태그를 단일 출처로 둔다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/README.md`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/WxTeamTypes.h`, `Source/WxGame/Character/Component/WxAIBehaviorComponent.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp` — 경계 확인용으로 `Plugins/WxUI/.../MVVM/WxViewModel.cpp`·`WxViewModel_Character.cpp`, `Plugins/WxCombat/.../Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/.../Attribute/WxCombatAttributeSet.cpp` 의 해당 함수만 함께 읽었다.
- **직전 리뷰 대비 닫힌 항목**: 4번(`BaseWalkSpeed` 무초기화 — 캐시 멤버를 없애고 CDO 파생으로 전환), 5번(README 의 `AWxMinion` 스테일 — 현행 파생 3종으로 갱신됨), 6번(점프 무적 GE 예측 — 호출부 주석으로 전제를 명시하고 닫기로 결정). 규칙 검사는 전 파일 대상으로 재실행했고 저작권 첫 줄 누락·인라인 정의·불필요 람다·`BlueprintCallable` 오용은 0건이다(`SetCurrentCategory` 는 `BlueprintSetter` 지정 대상이라 UHT 요구사항으로 판단해 제외했다).
- **미검토 / 한계**:
  - 리플리케이션 동작은 코드 독해로만 판단했고 데디케이티드 서버·원격 클라 실측은 하지 않았다. 특히 `AWxEnemyCharacter` 는 클라에서 `InitAbilitySystem` 이 돌지 않아(빙의가 서버 전용) SPD→`MaxWalkSpeed` 구독이 서버에만 생기는데, 시뮬 프록시는 복제 이동을 쓰므로 문제가 없다고 보고 발견으로 올리지 않았다 — 실측으로 확인한 것은 아니다.
  - `UWxExperienceManagerComponent` 의 GameFeature 활성/해제 카운팅은 정상 경로가 균형이 맞는 것까지 확인했으나, `UWxExperienceManager::RequestToDeactivatePlugin` 의 `FindChecked` 는 카운트가 어긋나면 크래시로 드러난다. PIE 다중 세션·심리스 트래블 실측은 하지 않았다.
  - `UWxCharacterMovementComponent::UpdateCharacterStateBeforeMovement` 가 `ASC->GetAnimatingAbility()` 로 `bWantsToCrouch` 를 끄는 것은 압축 플래그 예측과 얽히지만, 서버·클라의 어빌리티 상태가 갈릴 실제 빈도를 재지 못해 발견으로 올리지 않았다.
  - `UWxMetaHumanComponent` 가 `UMetaHumanComponentUE` 에서 물려받는 페이스 리그로직·넥 보정 구동부는 엔진 플러그인 쪽이라 읽지 않았다.
  - BP/WBP 내부 구조(위젯 계층·MVVM 바인딩 행·SCS 컴포넌트 조립)와 Experience·ActionSet 에셋의 실제 데이터는 범위 밖이다.

---
*문서 기준 커밋 `303d8d7f` · 리뷰일 2026-09-05 · 소스 69파일 — `/module-review`로 갱신*
