# WxCore — 코드 리뷰

> 런타임 상태·복제·할당이 전혀 없는 선언 전용 foundation 모듈이라 전반적으로 건강하며, 심각 결함은 발견되지 않았다. 소스 9파일 전량을 통독하고, 계약의 실제 소비처(WxAbility_Interact, WxInteractionScannerComponent, WxGimmickStateTreeComponent, WxDialogueComponent, WxItemPickup, WxEnemyCharacter, WxSaveWorldSubsystem)와 `Config/DefaultEngine.ini`의 채널·프로파일 등록까지 교차 검증했다.

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
- **문제**: `Find` 는 액터 자신의 구현을 먼저 보고(16행), 없을 때만 컴포넌트를 훑는다(21행). 그런데 계약의 활성 판정은 메시 단위(`IsInteractionMeshActive`)라, 한 액터에 액터 구현체와 컴포넌트 구현체가 함께 있으면 컴포넌트 쪽 영역은 영원히 도달 불가능해진다. 구체 시나리오: `AWxEnemyCharacter`(액터가 구현, `Source/WxGame/Character/WxEnemyCharacter.h:25`)에서 파생된 BP 에 `UWxDialogueComponent`(컴포넌트가 구현)를 붙이면, 스캐너가 대화 영역 메시로 `Find` 를 불러도 적 캐릭터 구현체가 답하고 그 `IsInteractionMeshActive` 는 `InMesh == GetMesh()` 만 참이라(`Source/WxGame/Character/WxEnemyCharacter.cpp:68`) 대화 영역이 후보에서 탈락한다. 에러도 경고도 없이 대화가 안 되는 형태로만 드러난다. 컴포넌트가 둘 이상인 경우의 비결정성은 `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:19` 에 이미 알려진 한계로 적혀 있으나, 액터+컴포넌트 조합은 모듈 경계가 막아 주지 않아 지금도 만들 수 있다.
- **제안**: 메시를 아는 오버로드(`Find(const UActorComponent*)`)는 후보(액터 구현체 + 인터페이스 구현 컴포넌트들) 중 `IsInteractionMeshActive(Mesh)` 가 참인 첫 구현체를 답하도록 좁힌다. 최소 대응으로는 구현체가 둘 이상일 때 개발 빌드에서 `ensureMsgf` 로 드러내는 것만으로도 침묵 실패를 없앨 수 있다.
- **확신도**: 중간

### 2. 🟡 계약의 `Source` 파라미터가 `UActorComponent*` 로 넓혀져 구현체마다 다운캐스트를 강요한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:70`
- **범주**: 설계/구조
- **문제**: 같은 "영역 메시"를 가리키는데 `IsInteractionMeshActive` 만 `const UPrimitiveComponent*`(56행)이고 `OnInteracted`(70행)·`CanBeInteractedBy`(79행)·`GetInteractionPrompt`(85행)는 `const UActorComponent*` 다. 실제 호출부는 전부 `UPrimitiveComponent*` 를 넘긴다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:92`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:84`) — 넓은 타입이 사 오는 이득이 없고, 대신 구현체가 되돌리는 비용을 낸다: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:136`, `:158` 에서 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))` 로 매 호출 다운캐스트한다. 또 `Cast` 가 실패하면 조용히 `nullptr` 영역 조회로 흘러 실패 경로가 프롬프트 공백으로만 나타난다.
- **제안**: `Source` 를 `const UPrimitiveComponent*` 로 통일해 계약 전체를 한 타입으로 맞춘다(구현체의 `Cast` 제거, 호출부 변경 없음).
- **확신도**: 중간

### 3. 🟢 `IWxSavable` 에 짝 헬퍼가 없어 조회 로직이 WxSave 에 그대로 복제돼 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:30`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것을, 아니면 그 액터의 컴포넌트를" 찾는 동일 로직이 `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9-22` 와 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:88-102` 두 곳에 있고, 헤더 주석 문장까지 사실상 같다(`WxInteractable.h:31` vs `WxSaveWorldSubsystem.h:57`). 계약은 WxCore 에 있는데 그 계약의 표준 조회 방식만 소비 모듈이 재구현하는 비대칭이라, 1번 항목 같은 규칙 변경이 생기면 두 곳을 따로 고쳐야 한다.
- **제안**: WxCore 에 공용 조회 헬퍼(액터 + 인터페이스 UClass → 구현 UObject)를 하나 두고 `IWxInteractable::Find` 와 `IWxSavable` 쪽 조회가 함께 쓰게 한다.
- **확신도**: 중간

### 4. 🟢 `WxCharacterMesh` 콜리전 프로파일이 헤더가 규정한 WxAttack 규약과 반대다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:12`
- **범주**: 버그/정확성
- **문제**: 헤더는 "캐릭터 메시는 WxAttack 에 Overlap" 을 규약으로 못 박고 코드도 그렇게 한다(`Source/WxGame/Character/WxCharacterBase.cpp:24`). 그런데 `Config/DefaultEngine.ini:41` 의 `WxCharacterMesh` 프로파일은 `(Channel="WxAttack",Response=ECR_Block)` 에 HelpMessage 도 "월드/공격에 Block" 이다. 프로파일을 지정하면 응답 컨테이너가 프로파일 값으로 덮이므로, 누군가 BP 디테일에서 이 프리셋을 고르는 순간 무기 히트박스가 Overlap 대신 Block 이 되어 오버랩 기반 피격 판정이 조용히 사라진다 — README 가 경고하는 바로 그 "코드와 ini 가 어긋나 히트 판정이 조용히 깨지는" 사례다. 현재 이 프로파일을 쓰는 에셋은 없어(Content 전수 검색 0건) 실동작 버그는 아니지만 함정으로 남아 있다.
- **제안**: `.ini` 의 `WxCharacterMesh` 프로파일에서 WxAttack 응답을 Overlap 으로 고치거나, 쓰이지 않는 프로파일이라면 제거한다.
- **확신도**: 중간

### 5. 🟢 Gimmick 태그 명명이 갈려 있고, 이 값은 세이브 슬롯에 직렬화된다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:40`
- **범주**: 설계/구조
- **문제**: 같은 "닫힌 상태"인데 `Gimmick.Door.Close`(동사형)만 다르고 나머지는 형용사·상태형이다(`Gimmick.Elevator.Closed`, `Gimmick.TreasureChest.Closed`, `Gimmick.CheckPoint.Unlit`). 이 태그는 `UWxGimmickStateTreeComponent::StateTag` 로 `SaveGame` 직렬화되므로(`Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeComponent.h:149`) 나중에 이름을 고치면 기존 슬롯의 기믹 상태가 매칭되지 않는다 — 고칠 거면 슬롯이 쌓이기 전인 지금이 비용이 가장 싸다.
- **제안**: `Gimmick.Door.Close` → `Gimmick.Door.Closed` 로 맞추고 ST 에셋의 상태값을 함께 갱신한다(이미 배포된 슬롯이 있으면 그대로 두는 편이 낫다).
- **확신도**: 낮음(의도된 설계일 수 있음)

### 6. 🟢 태그 설명이 헤더에만 있어 에디터 태그 피커에서는 보이지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:7`
- **범주**: 설계/구조
- **문제**: 정의는 전부 `UE_DEFINE_GAMEPLAY_TAG`(설명 없음)이라 태그 피커·툴팁에 아무 설명이 뜨지 않는다. 정의된 92개 태그 중 28개(Gimmick 전량, GameplayCue.AttackTelegraph.*, Ability.Skill.*/Pattern.*/Attack.Light·Heavy)는 C++ 참조가 0건으로 오직 GE/GA·StateTree 에셋에서만 선택되는 태그라, 정작 그 태그를 고르는 자리에서 헤더의 설명을 볼 수 없다.
- **제안**: 설명이 있는 태그는 `UE_DEFINE_GAMEPLAY_TAG_COMMENT` 로 바꿔 헤더 주석의 요지를 에디터에도 노출한다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`
- **교차 확인**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`
- **규칙 준수 확인 결과**: `CLAUDE.md` 코딩/모듈 규칙 위반 없음 — 전 파일 첫 줄 Copyright 존재, `Wx` prefix 준수, 람다·`FORCEINLINE`·인라인 함수 정의·`BlueprintCallable` 사용 0건, 델리게이트 콜백 없음, `WxCore.Build.cs` 는 Core/CoreUObject/Engine/GameplayTags 만 의존(Wx 플러그인 참조 없음). 네이티브 태그 선언도 WxCore 밖에는 없고(`UE_DEFINE_GAMEPLAY_TAG` 전수 검색), `DefaultGameplayTags.ini` 가 없어 태그 단일 소스 규약이 지켜지고 있다. `ECC_WxAttack = ECC_GameTraceChannel1` 은 `Config/DefaultEngine.ini:39` 의 채널 등록과 일치한다.
- **미검토 / 한계**: 태그의 에셋 측 실제 사용 여부는 `Content` 바이너리 문자열 검색 수준으로만 확인했고, GE/GA·StateTree 에셋 내부 구조는 보지 않았다. `IsMeshInRange` 가 전제하는 스켈레탈 피직스 애셋 존재 여부는 에셋 검증이 필요해 코드 수준에서만 확인했다.

---
*문서 기준 커밋 `e9440f73` · 리뷰일 2026-08-15 · 소스 9파일 — `/module-review`로 갱신*
