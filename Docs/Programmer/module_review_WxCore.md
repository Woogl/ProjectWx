# WxCore — 코드 리뷰

> 런타임 상태·복제·동적 할당이 전혀 없는 선언 전용 foundation 모듈이라 전반적으로 건강하며, 심각 결함은 없다. 소스 9파일(헤더 5·cpp 4) 전량을 통독하고 계약의 실제 소비처(WxAbility_Interact, WxInteractionScannerComponent, WxGimmickStateTreeComponent, WxDialogueComponent, WxItemPickup, WxEnemyCharacter, WxSaveWorldSubsystem)와 `Config/DefaultEngine.ini`의 채널·프로파일 등록까지 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `IWxInteractable::Find` 가 액터 구현체를 먼저 답해 컴포넌트 구현체를 조용히 가린다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:16`
- **범주**: 설계/구조
- **문제**: `Find` 는 액터 자신의 구현을 먼저 보고(16행), 없을 때만 컴포넌트를 훑는다(21행). 그런데 계약의 활성 판정은 메시 단위(`IsInteractionMeshActive`)라 한 액터에 액터 구현체와 컴포넌트 구현체가 함께 있으면 컴포넌트 쪽 영역은 영원히 도달 불가능해진다. 구체 시나리오: `AWxEnemyCharacter`(액터가 구현, `Source/WxGame/Character/WxEnemyCharacter.h:25`)에서 파생된 BP 에 `UWxDialogueComponent`(컴포넌트가 구현, `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:22`)를 붙이면, 스캐너가 대화 영역 메시로 `Find` 를 불러도(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:178`) 적 캐릭터 구현체가 답하고 그 판정은 `InMesh == GetMesh()` 만 참이라(`Source/WxGame/Character/WxEnemyCharacter.cpp:68`) 대화 영역이 후보에서 탈락한다. 에러도 경고도 없이 "대화가 안 된다" 로만 드러난다. 컴포넌트가 둘 이상일 때의 비결정성은 `WxDialogueComponent.h:19` 에 알려진 한계로 적혀 있으나 "액터 + 컴포넌트" 조합은 그 메모의 사정권 밖이고, 모듈 경계도 이 조합을 막지 않는다(WxGame 이 WxDialogue 를 볼 수 있고 컴포넌트 부착은 BP 디테일 패널만으로 가능). 현재 콘텐츠에는 실제 조합이 없어(전수 검색 0건) 잠재 함정 단계다.
- **제안**: 메시를 아는 오버로드(`Find(const UActorComponent*)`)가 후보(액터 구현체 + 인터페이스 구현 컴포넌트) 중 `IsInteractionMeshActive(Mesh)` 를 참으로 답하는 구현체를 고르게 좁힌다. 최소 대응으로는 구현체가 둘 이상 발견될 때 개발 빌드에서 `ensureMsgf` 로 드러내는 것만으로도 침묵 실패는 없앨 수 있다.
- **확신도**: 중간

### 2. 🟡 계약의 `Source` 파라미터만 `UActorComponent*` 로 넓혀져 구현체가 매 호출 다운캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:70`
- **범주**: 설계/구조
- **문제**: 같은 "영역 메시" 를 가리키는데 `IsInteractionMeshActive` 만 `const UPrimitiveComponent*`(56행)이고 `OnInteracted`(70행)·`CanBeInteractedBy`(79행)·`GetInteractionPrompt`(85행)는 `const UActorComponent*` 다. 실제 호출부는 전부 `UPrimitiveComponent*` 를 넘긴다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:92`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:84`) — 넓은 타입이 사 오는 이득은 없고 구현체가 되돌리는 비용만 낸다: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:136`, `:158` 이 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))` 로 매 호출 되돌린다. 게다가 `Cast` 실패는 `nullptr` 영역 조회로 흘러 프롬프트 공백·무동작으로만 나타나 실패 경로가 눈에 띄지 않는다.
- **제안**: `Source` 를 `const UPrimitiveComponent*` 로 통일해 계약 전체를 한 타입으로 맞춘다. 호출부는 이미 `UPrimitiveComponent*` 를 넘기므로 변경이 없고, 구현체의 `Cast` 가 사라진다.
- **확신도**: 중간

### 3. 🟢 `WxCharacterMesh` 콜리전 프로파일이 헤더가 못 박은 WxAttack 규약과 반대다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:12`
- **범주**: 버그/정확성
- **문제**: 헤더는 "캐릭터 메시는 WxAttack 에 Overlap, 캡슐은 Ignore" 를 규약으로 선언하고 코드도 그렇게 한다(`Source/WxGame/Character/WxCharacterBase.cpp:24`, `:28`). 그런데 `Config/DefaultEngine.ini:41` 의 `WxCharacterMesh` 프로파일은 `(Channel="WxAttack",Response=ECR_Block)` 이고 HelpMessage 도 "월드/공격에 Block" 이다. 프로파일을 지정하면 응답 컨테이너가 프로파일 값으로 통째로 덮이므로, 누군가 BP 디테일에서 이 프리셋을 고르는 순간 코드가 세운 Overlap 이 사라진다. 현재 이 프로파일을 쓰는 에셋·코드는 0건이라 실동작 버그는 아니지만, 이름 때문에 "캐릭터 메시에 쓰라고 있는 프리셋" 으로 보이는 함정으로 남아 있다 — README 가 경고하는 "코드와 ini 가 어긋나 히트 판정이 조용히 깨지는" 바로 그 형태다.
- **제안**: `.ini` 의 WxAttack 응답을 Overlap 으로 고쳐 헤더 규약과 맞추거나, 쓰이지 않는 프로파일이므로 제거한다.
- **확신도**: 중간

### 4. 🟢 `IWxSavable` 에만 짝 헬퍼가 없어 조회 로직이 WxSave 에 그대로 복제돼 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:30`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것을, 아니면 그 액터의 컴포넌트를" 이라는 동일 조회가 `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9-22` 와 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:88-102` 두 곳에 있고, 인터페이스 타입만 다를 뿐 분기·주석 문장까지 사실상 같다(`WxInteractable.h:31` vs `WxSaveWorldSubsystem.h:57`). 계약은 WxCore 에 두면서 그 계약의 표준 조회 방식만 소비 모듈이 재구현하는 비대칭이라, 1번 같은 규칙 변경이 생기면 두 곳을 따로 고쳐야 한다.
- **제안**: `IWxInteractable::Find` 와 대칭으로 `IWxSavable::Find(AActor*)` 를 WxCore 에 두고 `UWxSaveWorldSubsystem::FindSavable` 이 그것에 위임한다.
- **확신도**: 중간

### 5. 🟢 Gimmick 태그 하나만 명명이 갈려 있고, 이 값은 세이브 슬롯에 직렬화된다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:40`
- **범주**: 설계/구조
- **문제**: 같은 "닫힌 상태" 인데 `Gimmick.Door.Close` 만 동사형이고 형제는 전부 상태형이다(`Gimmick.Elevator.Closed:43`, `Gimmick.TreasureChest.Closed:47`, `Gimmick.CheckPoint.Unlit:50`). 이 태그는 `UWxGimmickStateTreeComponent::StateTag` 로 `SaveGame` 직렬화되므로(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h:149`) 나중에 고치면 기존 슬롯의 문 상태가 매칭되지 않는다 — 고칠 거면 슬롯이 쌓이기 전인 지금이 가장 싸다.
- **제안**: `Gimmick.Door.Closed` 로 맞추고 ST 에셋의 상태값을 함께 갱신한다(태그 문자열 변경이므로 리다이렉트 ini + ResavePackages 절차 필요). 이미 배포된 슬롯이 있으면 그대로 두는 편이 낫다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 태그 설명이 헤더에만 있어 정작 태그를 고르는 에디터 피커에서는 보이지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:7`
- **범주**: 설계/구조
- **문제**: 정의는 전량 `UE_DEFINE_GAMEPLAY_TAG`(설명 인자 없음)이라 태그 피커·툴팁에 아무 설명이 뜨지 않는다. 정의된 95개 중 29개(Gimmick 전량, `GameplayCue.AttackTelegraph.*`, `Ability` 루트, `Ability.Attack.Light/Heavy`, `Ability.Skill.*`, `Ability.Pattern.*`)는 C++ 참조가 0건으로 오직 GA/GE·StateTree 에셋에서만 선택되는 태그라, 헤더 주석이 닿지 않는 자리에서만 쓰인다. 부수 확인: `Ability.Pattern.6~9` 는 C++ 뿐 아니라 에셋에서도 참조 0건이라 실제로 비어 있는 예약 슬롯이다.
- **제안**: 설명이 있는 태그는 `UE_DEFINE_GAMEPLAY_TAG_COMMENT` 로 바꿔 헤더 주석의 요지를 에디터에도 노출한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`
- **규칙 준수 확인 결과**: `CLAUDE.md` 코딩/모듈 규칙 위반 없음 — 전 파일 첫 줄 Copyright 존재, `Wx` prefix 준수, 람다·`FORCEINLINE`·인라인 함수 정의·`BlueprintCallable` 사용 0건, 델리게이트 콜백 없음(Handle prefix 대상 없음), `WxCore.Build.cs` 는 Core/CoreUObject/Engine/GameplayTags 만 의존해 Wx 플러그인 참조가 없다. 태그 단일 소스 규약도 지켜진다 — 프로젝트 전체에 `RequestGameplayTag`·`GameplayTagsManager` 호출 0건, `Config/DefaultGameplayTags.ini` 부재, 선언 95개와 정의 95개가 1:1로 일치하고 식별자↔문자열 표기도 전수 일치한다. `ECC_WxAttack = ECC_GameTraceChannel1` 은 `Config/DefaultEngine.ini:39` 의 채널 등록과 일치한다.
- **미검토 / 한계**: 태그의 에셋 측 사용 여부는 `Content`·`Plugins` 의 `.uasset` 바이너리 문자열 검색 수준으로만 확인했고(6번 항목의 참조 0건 집계가 이 방식에 의존한다), GA/GE·StateTree 에셋 내부 구조는 열어 보지 않았다. `IsMeshInRange` 가 전제하는 스켈레탈 피직스 애셋 존재 여부도 코드 수준에서만 확인했다.

---
*문서 기준 커밋 `b3aec4ef` · 리뷰일 2026-08-20 · 소스 9파일 — `/module-review`로 갱신*
