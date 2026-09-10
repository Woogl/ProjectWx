# WxCore — 코드 리뷰

> 11파일 중 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지는 전부 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 소스 11파일과 `WxCore.Build.cs`·`WxCore.uplugin`·`README.md`를 전부 통독했고, 태그 111개의 선언↔정의 짝과 프로젝트 전체(C++ 심볼, `Content`·`Plugins/*/Content`의 에셋 문자열, `Config`) 참조 여부를 전수 대조했으며, `ECC_WxAttack`↔`Config/DefaultEngine.ini` 채널 바인딩과 `Event.*` 15개 각각의 실제 소비 방식(게임플레이 이벤트냐 루스 태그냐), `FUniversalObjectLocator::SyncFind`의 UE 5.8 구현(컨텍스트 없이도 에디터 경로로 해석, 동기 로드 없음), 두 공용 인터페이스의 구현체·소비처까지 실물로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `Event.Ragdoll` 은 이벤트가 아니라 상태 태그다 — `Event.*` 네임스페이스 계약과 어긋난다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:102` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:39`)
- **범주**: 설계/구조
- **문제**: `Event.*` 는 "서버에서 ASC 로 보내는 Gameplay Event"라는 계약이고(`Plugins/WxCore/README.md:32`, 헤더 `WxGameplayTags.h:63`·`:80` doc-comment), 실제로 나머지 14개는 전부 `TriggerTag`·`HandleGameplayEvent`·`SendGameplayEventToActor`·`GenericGameplayEventCallbacks` 중 하나를 탄다. `Event.Ragdoll` 만 예외로, 소비처 세 곳 모두 이벤트가 아니라 **지속 상태**로 다룬다 — `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:100` 이 `AddLooseGameplayTag(..., EGameplayTagReplicationState::TagOnly)` 로 올리고, `Source/WxGame/Character/WxCharacterBase.cpp:57` 이 `RegisterGameplayTagEvent` 로 구독하고 `:61` 이 `HasMatchingGameplayTag` 로 late-join 초기값을 폴링한다. 이벤트로 발송되는 지점은 코드에도 에셋에도 없다. foundation 모듈의 유일한 책임이 태그 분류표인데 그 분류표가 스스로 세운 규약을 한 칸에서 깨고 있어, 앞으로 `Event.*` 를 이벤트로 뭉쳐 다루는 코드(예: 이벤트 태그 컨테이너 델리게이트 일괄 구독, `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp:150` 형태)가 상태 태그를 이벤트로 오인할 여지를 남긴다.
- **제안**: `State.Ragdoll` 로 옮긴다(`State.*` = "ASC 에 붙는 상태"에 정확히 들어맞고, `Ability.Death` 와 나란히 사망 처리 상태로 읽힌다). 문자열이 에셋에도 남아 있으므로(`Event.Ragdoll` 이 `Content` 에서 검출됨) 이름을 바꿀 땐 `GameplayTagRedirects` 를 함께 넣어야 저작 데이터가 끊기지 않는다. 옮기지 않기로 한다면 헤더에 "이벤트가 아니라 복제 상태 태그다"라는 예외 사유를 남겨 다음 사람이 같은 판정을 반복하지 않게 한다.
- **확신도**: 높음 (소비 방식은 사실 확인 완료. 이름을 옮길지 예외로 못박을지는 취향 문제)

### 2. 🟢 `ECC_WxAttack` doc-comment 의 불변식이 실제 ini 동작과 다르다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:8` (같은 문구가 `Plugins/WxCore/README.md:23`·`:42` 에도 반복됨)
- **범주**: 버그/정확성
- **문제**: 주석은 "DefaultEngine.ini의 채널 등록 **순서**와 일치해야 한다"고 못박지만, `Config/DefaultEngine.ini:39` 는 `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,...,Name="WxAttack")` 처럼 채널 enum 을 키로 **명시 바인딩**한다. 즉 줄 순서는 채널 정체성과 무관하고, 실제로 지켜야 하는 불변식은 "`WxAttack` 항목의 `Channel=` 값이 `ECC_GameTraceChannel1` 로 유지되는 것"이다. 지금 상태에서 위험 방향이 뒤집혀 있다 — 줄을 재배치하는 사람은 깨지지 않는 것을 겁내고, `Channel=` 값을 손대거나 그 줄을 지우는 사람은 무기 스윕(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:155,202`)·회피 판정 캡슐(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp:254`)·캐릭터 메시 응답(`Source/WxGame/Character/WxCharacterBase.cpp:26,30`)이 조용히 다른 채널을 가리키게 되는 실제 위험을 경고받지 못한다. 현재 값 자체는 정합하다(채널 1개만 등록, 이름 일치).
- **제안**: 주석 문구를 "채널 등록 순서" → "`DefaultEngine.ini` 의 `WxAttack` 등록 항목이 쓰는 `Channel=` 값"으로 고친다. README 의 같은 두 문장도 함께 정정한다.
- **확신도**: 높음

### 3. 🟢 어디에서도 참조되지 않는 태그가 8개로 늘었다 (직전 리뷰 6개)
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:190-192`, `:214-218` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:93-95`, `:113-117`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`~`.4`, `Ability.Pattern.5`~`.9` 여덟 개는 C++ 심볼 참조도, `Content`·`Plugins/*/Content` 에셋 문자열도, `Config` 항목도 0건이다(태그 111개 전수 대조). 직전 리뷰 시점(`262e4cca`)의 6개에서 `Ability.Skill.3`·`.4` 가 추가됐는데, 원인은 `e0e3ecc5` "캐릭터 폴더 정리"로 `GA_Skill_3`·`GA_Skill_4` 가 사라지고 현재 플레이어 템플릿에 `Content/Character/TemplatePlayer/Abilities/Skill_1` 하나만 남은 것이다. 태그 자체는 유효한 예약 슬롯일 수 있으나, 코드만 봐서는 어디까지가 실재하는 어빌리티인지 구분되지 않고 실제로 숫자가 움직이고 있다.
- **제안**: 예약 슬롯이 맞다면 `Ability.Skill`·`Ability.Pattern` 블록에 "슬롯 예약, 아직 구현 없음" 한 줄을 남긴다(직전 리뷰도 같은 제안을 했고 아직 반영되지 않았다 — 매 리뷰가 이 항목을 다시 파헤치는 비용이 계속 든다). 예약 의도가 아니면 지운다. 참고로 `Cooldown.*` 는 실재하는 슬롯만 선언돼 있어(`Cooldown.Skill.1` 뿐, `WxGameplayTags.h:231`) 이 축과 어긋나 있지는 않다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 `State.Minion.Active` 는 발행만 되고 읽히지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:22` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:10`)
- **범주**: 중복/복잡도
- **문제**: 헤더 주석이 이미 "소환·명령 어빌리티 분기는 아직 이 태그를 읽지 않는다"고 인정하고 있고, 전수 대조 결과도 같다 — 쓰기 경로는 `Source/WxGame/Character/WxEnemyCharacter.cpp:39,58,226` 뿐이고 `HasMatchingGameplayTag`·`RequireTags`·`ActivationBlockedTags` 등 읽는 쪽은 C++ 도 에셋도 0건이다. 그런데 발행은 `EGameplayTagReplicationState::TagOnly` 로 이뤄지므로 소환물이 뜨고 죽을 때마다 아무도 읽지 않는 태그가 주인 ASC 에서 클라로 복제된다.
- **제안**: 읽는 쪽을 붙일 계획이 살아 있다면 그대로 두되 주석에 "언제/누가 읽을 예정"까지 남긴다. 계획이 유동적이면 발행 자체(`WxEnemyCharacter` 의 `MasterStateTag` 올리기·내리기)를 걷어내고 필요해질 때 되살리는 편이 복제 트래픽과 상태 수를 줄인다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 주석이 미완성임을 이미 밝히고 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
  - 대조를 위해 함께 읽은 모듈 밖 파일: `Wx.uproject`, `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxEditor/WxUIDataThumbnailRenderer.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxWorld|WxQuest|WxUI|WxDialogue/Source/*/*.Build.cs`, UE 5.8 엔진의 `UniversalObjectLocator.h`·`ActorLocatorFragment.h/.cpp`·`CollisionProfile.h`
  - 이번에 확인했고 문제 없었던 것: 태그 111개의 헤더 선언↔cpp 정의 1:1(누락·잉여 0), 심볼명 `_`→`.` 치환값과 정의 문자열 전부 일치(오타 0), 태그 단일 출처 규약 유지(`Config` 에 태그 목록 항목 0건이므로 프로젝트의 모든 태그가 이 두 파일에서만 나온다). `FWxLocatorUtils::GetDisplayName` 의 세 분기 전부 도달 가능하고 널 가드도 온전하다 — `SyncFind(nullptr)` 는 엔진 5.8 의 `FActorLocatorFragment::Resolve` 에서 컨텍스트 없이도 에디터 폴백(`Path.ResolveObject()`)까지 내려가 로드된 액터를 찾고, 실패 시 서브패스 파싱 분기로 넘어간다(동기 로드 없음 → 디테일 패널·ST 노드 설명 재그리기에서 비용 문제 없음). `WITH_EDITOR` 가드와 `Build.cs:19-25` 의 에디터 전용 `UniversalObjectLocator` 의존이 일치하고, 소비 모듈 4개는 모두 이 모듈을 무조건 의존으로 선언해 두어 비에디터 빌드에서 전파 누락으로 깨질 여지가 없다. `IWxUIData::GetMaxRecharges` 가 표시용 계약에 얹혀 있으면서 `WxAbilityBase.cpp:306` 의 `CheckCooldown` 게이트 입력으로도 쓰이지만, 어빌리티가 남이 아니라 자기 테이블 행을 읽는 구조이고 WxUI 가 WxCombat 을 참조할 수 없는 모듈 규칙상 다른 통로가 없어 결함으로 올리지 않았다. `GetIcon` 의 `TSoftObjectPtr` 계약도 소비처 양쪽이 각자 적절히 처리한다(에디터 썸네일은 `LoadSynchronous`, 런타임 VM 은 `RequestImageAsync`). 규칙 위반 없음 — `Wx` prefix, 소스 12개(`*.h`/`*.cpp`/`*.cs`) 전부 저작권 첫 줄, 인라인 함수 정의 0(`ECC_WxAttack` 은 `inline constexpr` **상수**라 규칙 6과 무관), `BlueprintCallable` 0, 람다 0, 델리게이트 콜백 0. WxCore 는 다른 Wx 플러그인을 참조하지 않는다(`Core`/`CoreUObject`/`Engine`/`GameplayTags` + 에디터 전용 엔진 모듈뿐, `.uplugin` 에 `Plugins` 항목 없음, Content 폴더 없음).
- **미검토 / 한계**:
  - 태그의 에셋 참조 여부는 `.uasset`/`.umap` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 3번·4번이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다.
  - 1번의 이름 변경 비용(에셋 리다이렉터 필요 범위)은 `Event.Ragdoll` 문자열이 `Content` 에 존재한다는 사실까지만 확인했고, 어느 에셋의 어느 필드인지는 열어 보지 않았다.
  - `Config/DefaultEngine.ini:41` 의 `WxCharacterMesh` 프로파일이 `WxAttack` 에 `ECR_Block` 을 지정해 `WxCollisionChannels.h` doc-comment(메시는 Overlap)와 어긋나지만, 코드·에셋 어디에서도 이 프로파일을 쓰지 않으므로(현재 응답은 `WxCharacterBase.cpp:26,30` 의 C++ override 가 결정) WxCore 밖 사안으로 두고 발견으로 올리지 않았다.
  - 리뷰 대상은 커밋 `1d91a915` 의 작업 트리(clean)이며, 라인 번호도 그 기준이다.

---
*문서 기준 커밋 `1d91a915` · 리뷰일 2026-09-10 · 소스 11파일 — `/module-review`로 갱신*
