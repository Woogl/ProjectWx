# WxCore — 코드 리뷰

> 9파일·400줄 남짓의 얇은 foundation 모듈이라 전반적으로 건강하다. 태그 선언/정의는 97개가 완전히 짝을 이루고, `WxCore.Build.cs`는 Core/CoreUObject/Engine/GameplayTags만 참조해 플러그인 경계 규칙을 지킨다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 모두 읽었고, `IWxInteractable`/`IWxSavable`의 실제 소비처(WxWorld 스캐너, WxGame 상호작용 어빌리티, WxSave 서브시스템)까지 따라가 계약이 지켜지는지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 삭제된 `Trait.*` 네임스페이스를 아직 가리키는 주석 — 태그 색인이 오독을 유발한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:148`, `Plugins/WxCore/README.md:7`, `Plugins/WxCore/README.md:35`
- **범주**: 중복/복잡도
- **문제**: 커밋 `b38b4654`("배타 액션을 태그 배선에서 ActivationGroup으로 전환")가 `Trait_Ability_Exclusive` 선언·정의를 `WxGameplayTags.h`/`.cpp`에서 제거했는데, Ability 섹션 머리주석은 여전히 "어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 Trait.\*에 있다"고 안내한다. 이 헤더는 README가 "다른 모듈을 이해하는 색인"으로 지목한 프로젝트 유일의 태그 어휘 원본이라, 존재하지 않는 네임스페이스로 독자를 보내는 비용이 크다. 배타성은 이제 `UWxAbilitySystemComponent`/`UWxAbilityBase`의 ActivationGroup이 담당한다.
  또한 그 정리에서 에셋 잔재가 남았다 — `Content/AbilitySystem/Ability/` 아래 12개 GA 에셋(`GA_Attack_Light`, `GA_Attack_Heavy`, `GA_Attack_Air`, `GA_Attack_DodgeCounter`, `GA_Pattern_1`~`4`, `GA_Skill_2`~`4`, `GA_Skill_E`)이 여전히 `Trait.Ability.Exclusive` 문자열을 참조한다. 프로젝트에 `DefaultGameplayTags.ini`가 없고(커밋 `289a15e6`에서 제거) 네이티브 선언도 사라졌으므로 선언 소스가 하나도 없는 태그다. 엔진은 이런 태그를 에디터 로드 시 `Invalid GameplayTag %s found in property %s` 경고만 내고 컨테이너에 그대로 남겨두므로(`GameplayTagsManager.cpp:RedirectTagsForContainer`), 조용한 경고 스팸 + 죽은 데이터로 누적된다.
- **제안**: 주석 3곳을 ActivationGroup 안내로 갱신하고, 12개 GA 에셋의 AssetTags/BlockAbilitiesWithTag/CancelAbilitiesWithTag에서 잔여 태그를 걷어낸 뒤 재저장한다(태그 리다이렉트를 쓰지 않기로 했으므로 수동 정리).
- **확신도**: 높음

### 2. 🟡 `IWxInteractable::Find`의 컴포넌트 갈래가 비결정적 — 인터페이스가 스스로 약속한 클라/서버 수렴이 깨질 수 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:21`
- **범주**: 버그/정확성
- **문제**: `AActor::FindComponentByInterface`는 `GetComponents()`(= `TSet<UActorComponent*>`)를 순회하며 첫 매치를 반환한다(`Engine/Private/Actor.cpp:4080`). TSet 순회는 해시 순서라 머신·실행마다 달라질 수 있으므로, 한 액터에 `IWxInteractable` 구현 컴포넌트가 둘이면 클라와 서버가 서로 다른 구현체를 골라도 이상하지 않다. 그런데 `WxInteractable.h:48-49`·`67-68`은 "스캐너가 클라에서, 어빌리티가 서버에서 같은 두 함수를 물어 같은 답에 수렴한다"를 명시적 계약으로 선언한다 — 서로 다른 구현체를 고르면 그 계약이 조용히 무너져 프롬프트는 뜨는데 서버가 거부하거나(또는 그 반대) 엉뚱한 대상이 응답하는 형태로 나타난다.
  `WxDialogueComponent.h:19`가 이 모호성을 인지하고 "모듈 경계상 지금은 만들 수 없는 조합"이라 적어두었지만, 그 근거는 C++ 코드에만 해당한다. `UWxGimmickStateTreeComponent`(WxWorld)와 `UWxDialogueComponent`(WxDialogue)는 둘 다 `IWxInteractable` 구현 컴포넌트이고, 디자이너가 BP 액터에 두 컴포넌트를 함께 붙이는 것을 막는 장치는 없다.
- **제안**: 최소한 두 개 이상 발견 시 `ensureMsgf`로 저작 실수를 드러내거나(비용 거의 0), 결정적 타이브레이크(예: 컴포넌트 `GetFName()` 사전순 최소)를 도입해 전 머신에서 같은 답이 나오게 한다.
- **확신도**: 중간 (비결정성 자체는 확실하나, 두 구현체가 한 액터에 붙는 조합이 현재 콘텐츠에 있는지는 확인하지 않았다)

### 3. 🟢 `IWxSavable`에 `Find` 대응물이 없어 WxSave가 같은 해석 로직을 복제한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:26`, 복제본 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:117-132`
- **범주**: 중복/복잡도
- **문제**: 두 인터페이스 모두 "액터가 직접 구현했으면 그것을, 아니면 컴포넌트를"이라는 동일한 해석 규약을 갖는데, `IWxInteractable`은 그 규약을 `Find` static으로 WxCore에 두고 `IWxSavable`은 두지 않았다. 그 결과 `UWxSaveWorldSubsystem::FindSavable`이 널 가드·액터 우선·`FindComponentByInterface` 폴백까지 구조가 완전히 같은 코드를 다시 갖는다. 세 번째 소비자가 생기면 또 재발명하고, 위 2번 같은 결함을 고칠 때 두 곳을 따로 고쳐야 한다.
- **제안**: `IWxSavable::Find(AActor*)`를 WxCore에 추가하고 `UWxSaveWorldSubsystem::FindSavable`이 그것을 위임하게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`
- **확인했으나 문제 없던 항목**:
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 채널 등록(`Name="WxAttack"`, `DefaultResponse=ECR_Block`, `bTraceType=False`)과 일치한다.
  - 태그 97개가 `.h` 선언 ↔ `.cpp` 정의로 완전히 짝을 이루며, 프로젝트 어디에도 WxCore 밖 `UE_DECLARE_GAMEPLAY_TAG*`·`RequestGameplayTag` 호출이 없어 "태그는 이 두 파일에만" 규약이 실제로 지켜지고 있다.
  - C++에서 참조되지 않는 태그 29개(`Gimmick.*`, `GameplayCue.AttackTelegraph.*`, `Ability.Attack.*`/`Skill.N`/`Pattern.N`)는 데드가 아니다 — 표본 확인 결과 실제 GA/GC 에셋이 참조한다(설계상 에셋 소비 태그).
  - `IsActorInRange`의 `ensureMsgf(bAnyQueryPrimitive, ...)`가 "상호작용 끄기 = 쿼리 콜리전 내리기"(`WxDialogueComponent.cpp:34`) 대상에서 오탐할 수 있는지 확인했으나, `WxAbility_Interact.cpp:73`이 `IsInteractionEnabled()`를 사거리 검사보다 먼저 걸어 도달하지 않는다.
  - `WxGameplayTags.h`/`.cpp`만 UTF-8 BOM이 있고 나머지 한글 주석 파일엔 없지만, UBT가 MSVC에 `/utf-8`을 항상 넘기므로(`VCToolChain.cs:708`) 인코딩 문제가 되지 않는다.
  - `const UPrimitiveComponent*`에 대한 `OverlapComponent` 호출은 const 오버로드로 해석되고, `GetComponents()`는 참조 반환이라 복사가 없다.
- **미검토 / 한계**: `Trait.Ability.Exclusive`가 12개 GA 에셋의 어느 태그 컨테이너(AssetTags / BlockAbilitiesWithTag / CancelAbilitiesWithTag)에 남았는지는 바이너리 문자열 검색까지만 확인했고 에디터로 열어보지 않았다. 발견 2번의 실제 재현 여부(한 액터에 상호작용 구현 컴포넌트 2개가 붙은 콘텐츠가 있는지)는 BP 에셋 검사가 필요해 확인하지 않았다.

---
*문서 기준 커밋 `6b77c352` · 리뷰일 2026-08-21 · 소스 9파일 — `/module-review`로 갱신*
