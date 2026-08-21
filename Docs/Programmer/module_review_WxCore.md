# WxCore — 코드 리뷰

> 선언이 대부분인 얇은 foundation으로, 의존성·모듈 경계·네이밍·저작권 헤더 등 프로젝트 규칙을 어긴 곳이 없고 실제 로직은 `IWxInteractable`의 정적 헬퍼 둘뿐이라 전반적으로 건강하다. 이번 리뷰는 모듈 9개 파일을 전부 읽고, 소비 측 호출부(`WxSave`/`WxWorld`/`WxGame`)·`Config/DefaultEngine.ini` 채널 등록·엔진 `OverlapComponent` 구현까지 교차 확인해 주석이 주장하는 전제의 사실 여부를 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 사거리 판정의 ensure가 자기 메시지가 지목한 실패 케이스를 못 잡는다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:31-52`
- **범주**: 버그/정확성
- **문제**: `ensureMsgf`의 조건은 `bAnyQueryPrimitive`뿐이고, 이 플래그는 `IsQueryCollisionEnabled()`만 보고 켜진다(35~40행). 그런데 메시지는 "스켈레탈이면 피직스 애셋도 필요"라고 명시한다. 엔진 구현상 `USkeletalMeshComponent::OverlapComponent`는 `Bodies` 배열만 훑으므로(`SkeletalMeshComponentPhysics.cpp:3080`), 피직스 애셋이 없으면 `Bodies`가 비어 항상 `false`를 답한다. 즉 "쿼리 콜리전이 켜진 스켈레탈 메시 하나뿐 + 피직스 애셋 없음"인 액터는 사거리 판정이 영구히 실패하는데 `bAnyQueryPrimitive == true`라 ensure가 침묵한다. 진단 장치가 명시적으로 경고하려던 두 케이스 중 하나를 정확히 놓치는 셈이다.
- **제안**: 플래그를 "쿼리 프리미티브 존재"가 아니라 "실제로 오버랩 테스트를 수행할 바디가 있었는가"로 바꾸거나, 스켈레탈 갈래에서 `GetPhysicsAsset()`/바디 수를 함께 확인해 ensure 조건에 반영한다. 조건을 넓힐 수 없다면 최소한 메시지에서 피직스 애셋 문구를 빼 진단 범위와 문구를 일치시킨다.
- **확신도**: 높음

### 2. 🟡 `IWxSavable`에 `Find` 대응물이 없어 WxSave가 같은 해석 로직을 재구현한다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9-22`
- **범주**: 중복/복잡도
- **문제**: WxCore는 두 공용 인터페이스에 대해 동일한 규약("액터가 직접 구현했으면 그것, 아니면 그 액터의 컴포넌트")을 세워 두었고, `IWxInteractable`에는 그 규약을 코드로 못 박은 `Find(AActor*)`를 제공한다. 반면 `IWxSavable`에는 대응물이 없어 `UWxSaveWorldSubsystem::FindSavable`(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:88-102`)이 널 가드·`Cast`·`FindComponentByInterface` 순서까지 그대로 복제해 두었다. 규약 자체는 WxCore 소유인데 구현이 두 모듈에 갈라져 있어, 해석 정책이 바뀔 때(예: 구현 컴포넌트가 둘일 때의 우선순위 결정 — `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:19`가 이미 미결로 남긴 문제) 한쪽만 고쳐 조용히 어긋날 수 있다.
- **제안**: `IWxSavable`에도 `static IWxSavable* Find(AActor*)`를 두고 `FindSavable`은 그것을 부르게 한다. 두 인터페이스가 같은 정적 헬퍼 형태를 갖게 되어 규약과 구현이 한곳에 모인다.
- **확신도**: 중간(WxCore를 최소로 유지하려는 의도된 선택일 수 있음)

### 3. 🟢 모듈 주석·README가 존재하지 않는 태그와 API를 가리킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:148`
- **범주**: 중복/복잡도
- **문제**: 해당 주석은 "어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 `Trait.*`에 있다"고 안내하지만, `Trait_*` 태그는 이 헤더에도 짝 cpp에도 없고 `Config/*.ini`·`Content`/`Plugins` 에셋 어디에도 없다. `ce04ce1f` 계보의 `b38b4654`("배타 액션을 태그 배선에서 ActivationGroup으로 전환")에서 `Trait.Exclusive`가 사라진 뒤 안내만 남은 것으로 보인다. `Plugins/WxCore/README.md:35`도 같은 잔존 서술을 담고 있고, 같은 파일 44행은 `IWxInteractable::Find(UActorComponent*)` 오버로드를 언급하지만 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:32`에는 `Find(AActor*)` 하나뿐이다. 태그 헤더는 README가 "다른 모듈을 이해하는 색인"이라 부르는 문서라 오독 비용이 작지 않다.
- **제안**: `WxGameplayTags.h:148`의 `Trait.*` 문장을 지우거나 ActivationGroup으로 대체됐다는 사실로 정정하고, README의 두 서술도 함께 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(모듈 밖, 계약 검증 목적)**: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`
- **검증했으나 문제 없던 항목**: 빌드 의존이 `Core`/`CoreUObject`/`Engine`/`GameplayTags`뿐이고 `.uplugin`에 플러그인 의존이 없어 "WxCore는 다른 Wx 플러그인 참조 금지" 규칙을 지킨다. Native 태그 선언이 저장소 전체에서 이 모듈 두 파일에만 존재해 "태그는 여기서만" 규약이 실제로 유지된다. `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 채널 등록과 일치하며, 주석이 주장하는 메시 Overlap·캡슐 Ignore override도 `Source/WxGame/Character/WxCharacterBase.cpp:25,29`에서 실제로 이뤄진다. `GetSaveId()`가 무효값이면 저장·복원에서 제외된다는 계약도 `WxSaveWorldSubsystem.cpp:284,346`에서 지켜진다. 선언된 태그 중 C++ 참조가 없는 것들(`Gimmick.*`, `Ability.Pattern.*`, `GameplayCue.AttackTelegraph.*` 등)은 에셋에서 참조 중임을 확인해 데드 선언이 아니다.
- **미검토 / 한계**: `IWxInteractable::Find`가 컴포넌트 갈래에서 `FindComponentByInterface`로 첫 구현체를 답하는 비결정성은 `WxDialogueComponent.h:19`에 기지 사항으로 명시돼 있고 현재 조합이 존재하지 않아 발견으로 올리지 않았다. `WxGameplayTags.h`/`.cpp` 두 파일만 UTF-8 BOM으로 저장돼 있으나(나머지 7개는 BOM 없음) 명시된 규칙 위반이 아니라 제외했다. 태그가 게임플레이 의미상 올바르게 소비되는지(GE·어빌리티 에셋 내부 배선)는 BP/에셋 영역이라 범위 밖이다.

---
*문서 기준 커밋 `ce04ce1f` · 리뷰일 2026-08-21 · 소스 9파일 — `/module-review`로 갱신*
