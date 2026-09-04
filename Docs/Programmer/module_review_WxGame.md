# WxGame — 코드 리뷰

> 조립 모듈답게 각 파일이 얇고 책임이 뚜렷하다. Experience 파이프라인은 Lyra 이식을 충실히 따르며 실패·PIE 다중 세션 경로까지 닫혀 있고, 뷰모델은 관찰 시작·해제가 대칭이다. 심각 결함은 찾지 못했다. Framework 전 파일, Character 전 파일, MVVM 전 파일, AbilitySystem·Inventory·Controller·Cheat 를 cpp 까지 읽었고, 참조 경계 확인을 위해 WxCombat·WxWorld·WxUI 의 호출 대상 일부를 함께 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 MetaHuman 컴포넌트가 리더 메시에 건 설정을 OnUnregister 에서 되돌리지 않는다
- **위치**: `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp:56`, `:84`, `:126`
- **범주**: 성능/안전
- **문제**: `OnRegister` 는 바디를 만들 때 리더 메시를 숨기면서 `VisibilityBasedAnimTickOption` 을 `AlwaysTickPoseAndRefreshBones` 로 올린다(:56). `OnUnregister` 는 `SetVisibility(true)` 만 되돌리고(:129) 이 틱 옵션은 원복하지 않아, 부착물이 사라져 리더가 다시 표시 주체가 된 뒤에도 화면 밖에서 본 리프레시가 계속 돈다 — 원래 이 옵션을 피하려던 비용을 그대로 문다. 같은 맥락으로 `BodyComponentName`·`FaceComponentName`(:84~85)도 정리되지 않아, 해제 후에는 이미 파괴된 컴포넌트 이름을 가리킨 채 남는다. 레벨 스트리밍 아웃/인이나 BP 리컴파일처럼 등록이 반복되는 경로에서 드러난다.
- **제안**: `OnRegister` 진입 시 리더의 원래 `VisibilityBasedAnimTickOption` 을 기억했다가 `OnUnregister` 에서 복원하고, 두 이름 문자열도 함께 비운다.
- **확신도**: 중간

### 2. 🟡 `AWxCharacterBase` 가 도메인 컴포넌트 10종을 무조건 생성한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:36`~`:53`
- **범주**: 설계/구조
- **문제**: 베이스 생성자가 ASC·어트리뷰트셋·MotionWarping·LockOn·HitStop·ProjectileManager·MinionManager·Equipment·WeaponActor·MetaHuman 을 예외 없이 만든다. 그 결과 발사체도 소환도 하지 않는 잡몹·NPC 폰까지 투사체·소환 매니저를 달고 다닌다. 모듈 README 가 선언한 확장 경로("도메인 컴포넌트를 만들고 Experience 의 AddComponents 액션으로 주입한다")와 실제 구조가 어긋나 있어, 새 기능을 어디에 붙여야 하는지 읽는 사람이 헷갈린다. 이 매니저들은 틱도 복제도 하지 않아 런타임 비용은 크지 않고, 비용은 주로 조직·가독성 쪽이다. `.codex/worklog/2026-09-04-CharacterBase-네이티브-컴포넌트-조합.md` 가 "선택적 Manager 컴포넌트 분리는 BP 마이그레이션 이후 별도 축소 단계로 남겼다"고 적어 둔 항목이며, `2026-09-05-캐릭터-이관-호환층-제거.md` 로 그 선행 조건인 BP 이관이 끝나 지금은 착수 가능한 상태다.
- **제안**: 예정된 축소 단계를 진행해 소환·투사체 매니저를 실제로 쓰는 캐릭터에만 남긴다(BP SCS 조립 또는 Experience 주입). 베이스에 남길 것과 뺄 것의 기준을 README 확장 포인트 절에 한 줄로 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 이미 후속 단계로 추적 중인 항목이다)

### 3. 🟢 Enhanced Input 콜백에 `Handle` prefix 가 없다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:112`, `:116`, `:120`, `:125`, `:130`, `:131`
- **범주**: 규칙 위반
- **문제**: `CLAUDE.md` 코딩 규칙 4는 델리게이트에 바인딩되는 콜백에 `Handle` prefix 를 요구한다. `BindAction` 으로 묶이는 `Move`·`Look`·`ToggleCrouch`·`AbilityInputTriggered`·`AbilityInputReleased` 가 이 규칙에서 벗어난 유일한 사례다(모듈의 나머지 바인딩 20여 곳은 전부 `Handle` 로 시작한다). 프로젝트 전체에서 `BindAction` 을 쓰는 곳은 이 파일뿐이라 관례로 굳은 예외라 보기도 어렵다.
- **제안**: `HandleMove`·`HandleLook`·`HandleToggleCrouch`·`HandleAbilityInputTriggered`·`HandleAbilityInputReleased` 로 개명하거나, 입력 액션 핸들러를 규칙 4의 예외로 `CLAUDE.md` 에 명시한다.
- **확신도**: 높음

### 4. 🟢 `BaseWalkSpeed` 가 초기화자 없이 선언돼 있다
- **위치**: `Source/WxGame/Character/WxCharacterBase.h:148`
- **범주**: 버그/정확성
- **문제**: 모듈에서 초기화자가 없는 유일한 스칼라 멤버다. 현재는 `InitAbilitySystem` 이 `SPDChanged` 구독과 같은 블록에서 값을 잡으므로(`WxCharacterBase.cpp:232`) `HandleSPDAttributeChanged` 가 쓰레기 값을 읽을 경로는 없고, UObject 메모리도 0 으로 채워져 실제 사고는 나지 않는다. 다만 "구독보다 대입이 먼저"라는 암묵 계약에만 안전성이 걸려 있어, 나중에 구독 지점이 옮겨지면 `MaxWalkSpeed` 가 조용히 0 이 된다.
- **제안**: `float BaseWalkSpeed = 0.f;` 로 선언하거나, 생성자에서 `GetCharacterMovement()->MaxWalkSpeed` 로 초기화한다.
- **확신도**: 높음

### 5. 🟢 README 가 삭제된 `AWxMinion` 을 여전히 파생 클래스로 소개한다
- **위치**: `Source/WxGame/README.md:9`
- **범주**: 중복/복잡도
- **문제**: `AWxMinion` 은 `2026-09-05-캐릭터-이관-호환층-제거` 에서 제거됐는데, 모듈 진입점 문서가 아직 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxMinion`/`AWxNpc` 를 파생으로 적고 있어 없는 타입을 찾게 만든다. 하단 provenance 도 71파일(현재 72)로 낡았다.
- **제안**: `/readme-writer` 로 갱신한다.
- **확신도**: 높음

### 6. 🟢 점프 무적 GE 가 리모트 클라에서는 예측되지 않는다
- **위치**: `Source/WxGame/Character/WxPlayerCharacter.cpp:149`
- **범주**: 설계/구조
- **문제**: `OnJumped_Implementation` 은 `CheckJumpInput` 경로라 권위 서버와 오토노머스 클라 양쪽에서 돈다. 클라 쪽 호출은 `UWxCombatLibrary::ApplyEffect(..., nullptr)` 로 빈 `FPredictionKey` 를 넘기는데, 엔진이 `ApplyGameplayEffectSpecToSelf` 초입의 `HasNetworkAuthorityToApplyGameplayEffect` 에서(`AbilitySystemComponent.cpp:1016`) 권위도 예측 키도 없는 적용을 걸러낸다. 그래서 이중 적용은 일어나지 않고, 리모트 클라에서는 로컬 적용이 통째로 무시된 뒤 서버가 복제한 GE 만 남는다. 남는 손해는 소유 클라에서 무적 태그가 RTT 만큼 늦게 뜬다는 것 하나다.
- **제안**: 예측 키를 실으려면 발동 어빌리티가 있어야 하는데, 점프는 CharacterMovement 자체 예측으로 서버에 전달돼 키를 태울 GAS RPC 가 없다. 직접 만든 키는 서버가 catch 하지 못해 확인도 거부도 못 받으니 키를 넘기는 것만으로는 해결되지 않고, 제대로 예측하려면 점프 무적을 이벤트 트리거 어빌리티로 옮겨야 한다. 태그 지연이 실제로 문제되기 전까지는 비용이 이득보다 크다고 보아, 2026-09-05 에 호출부 주석으로 이 전제를 명시해 두는 선에서 닫았다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Framework/WxExperienceManagerComponent.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/Framework/WxGameFeatureAction_AddComponents.cpp`, `Source/WxGame/Framework/WxExperienceManager.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Character/Component/WxEnemyComponent.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_Item.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModel_BossCharacter.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Inventory/WxItemUseComponent.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/README.md`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Source/WxGame/Framework/WxExperienceDefinition.cpp`, `Source/WxGame/Framework/WxExperienceActionSet.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxAIBehaviorComponent.cpp`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Player/WxPlayerState.cpp`, `Source/WxGame/Input/WxInputConfig.h`, `Source/WxGame/AnimNotify/WxAnimNotify_UseItem.cpp`, `Source/WxGame/MVVM/WxViewModel_Quest.cpp`, `Source/WxGame/MVVM/WxViewModel_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModel_QuestObjective.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp` — 경계 확인용으로 `Plugins/WxUI/.../MVVM/WxViewModel.h`, `Plugins/WxCombat/.../WxCombatLibrary.cpp`, `Plugins/WxCombat/.../WxAbilitySystemComponent.cpp`, `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`, `Plugins/WxDialogue/.../WxDialogueActor.cpp` 의 해당 함수만 함께 읽었다.
- **미검토 / 한계**:
  - 리플리케이션 동작은 코드 독해로만 판단했고 데디케이티드 서버·원격 클라 실측은 하지 않았다. 6번 발견은 엔진의 권한 검사 코드까지 따라가 확인했으나 그 역시 실측은 아니다.
  - `UWxExperienceManagerComponent` 의 GameFeature 활성/해제 카운팅은 정상 경로가 균형이 맞는 것까지 확인했으나, `UWxExperienceManager::RequestToDeactivatePlugin` 의 `FindChecked` 는 카운트가 어긋나면 크래시로 드러난다. PIE 다중 세션·심리스 트래블 실측은 하지 않았다.
  - `UWxMetaHumanComponent` 가 `UMetaHumanComponentUE` 에서 물려받는 페이스 리그로직·넥 보정 구동부는 엔진 플러그인 쪽이라 읽지 않았다.
  - BP/WBP 내부 구조(위젯 계층·MVVM 바인딩 행·SCS 컴포넌트 조립)와 Experience·ActionSet 에셋의 실제 데이터는 범위 밖이다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 72파일 — `/module-review`로 갱신*
