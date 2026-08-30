# WxCore — 코드 리뷰

> 태그·콜리전 상수·도메인 계약만 얇게 들고 있어 의존 경계는 여전히 건강하다. 프로젝트 코딩·모듈 규칙 위반은 이번에도 발견되지 않았다. 이번 리뷰는 `WxCore`의 C++ 11파일과 `WxCore.Build.cs`/`.uplugin`을 전부 읽고, 태그 선언·정의 대응과 105개 태그의 실사용처(코드·바이너리 에셋), `ECC_WxAttack`과 `Config/DefaultEngine.ini` 채널 등록, 두 계약 인터페이스(`IWxInteractable`·`IWxSavable`)의 전 소비 지점과 구현체를 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `CanInteract`의 권위(서버/클라) 계약이 비어 있어 구현체가 서버 전용 상태로 답한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:31`
- **범주**: 설계/구조
- **문제**: 계약이 `CanInteract()`를 어느 머신에서 물어보는지, 답의 근거가 복제되어야 하는지 아무것도 규정하지 않는다. 그런데 실제 소비처는 권위가 갈린 두 곳이다 — 스캐너는 로컬 컨트롤러에서만 돌며(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:35`) 클라이언트에서 `CanInteract(Pawn)`으로 후보를 추리고(`:179`), 어빌리티는 서버에서 같은 함수로 다시 검증한다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:75`). 계약이 침묵하니 구현체가 자연히 복제되지 않는 상태를 물게 된다: `AWxDevice::CanInteract()`는 `bInteractionEnabled`를 그대로 돌려주는데(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:48`) 이 필드는 `UPROPERTY`조차 아니고(`Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h:109`) `GetLifetimeReplicatedProps`에도 없으며(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:22`~`:27`), 오직 서버 권위 StateTree 경로에서만 쓰인다(`:54`, `:139`). 결과적으로 데디케이티드 서버 구성에서 클라이언트의 `bInteractionEnabled`는 기본값 `false`로 고정되어, 서버가 연 장치가 클라이언트 스캔 후보에 영영 오르지 않고 프롬프트도 뜨지 않는다. 이 코드베이스는 `HasAuthority` 분기·`DOREPLIFETIME`·Server RPC를 전반에 두고 있어 네트워크 플레이를 전제로 읽힌다.
- **제안**: 계약 주석에 "`CanInteract()`는 모든 머신에서 같은 답을 내야 하며, 근거 상태는 복제되어야 한다"를 명시하고, 클라 표시 판정과 서버 권위 판정을 분리해야 한다면 그 둘을 계약에서 별도 함수로 갈라 놓는다. 구현체 쪽 수정(`AWxDevice::bInteractionEnabled` 복제)은 [[WxWorld]] 몫이다.
- **확신도**: 높음 (단, 프로젝트가 싱글플레이·리슨서버만 목표라면 현재로선 증상이 드러나지 않는다)

### 2. 🟡 토글을 지원하지 않는 상호작용 대상이 조용히 성공 처리된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:41`
- **범주**: 설계/구조
- **문제**: `SetInteractionEnabled()`의 기본 구현이 no-op인데(`Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:5`~`:7`), 계약에는 대상이 토글을 지원하는지 표현할 방법이 없다. 호출자 `FWxStateTreeTask_EnableInteraction`은 대상이 `IWxInteractable`이기만 하면 호출 직후 무조건 `Succeeded`를 돌려준다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:76`, `:84`, `:86`). 그러나 `AWxItemPickup`과 `AWxEnemyCharacter`는 이 함수를 재정의하지 않으므로(`Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h:41`~`:44`, `Source/WxGame/Character/WxEnemyCharacter.h:36`~`:39`) StateTree가 이들을 토글 대상으로 지목하면 아무 변화도 경고도 없이 성공한다 — 저작 오류가 조용히 묻힌다. 헤더의 `// TODO: 제거하고 CanInteract로 쉽게 처리하길 희망함.`(`:39`)이 같은 문제 인식을 이미 남겨 두었다.
- **제안**: TODO대로 토글을 걷어 `CanInteract()` 하나로 답하게 하거나, 그때까지는 `SetInteractionEnabled()`를 순수 가상으로 올려 구현체가 지원 여부를 명시하게 한다.
- **확신도**: 높음

### 3. 🟢 `GetInteractionPrompt()`의 입력 키 힌트 소유자가 계약에 정의되지 않았다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:35`
- **범주**: 설계/구조
- **문제**: 반환값이 액션 설명만인지 입력 키까지 포함하는지 계약이 정하지 않는다. 그 결과 픽업만 `"[F] {0}"`을 직접 박아 넣고(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:106`~`:108`), 장치와 대화 액터는 순수 문구를 반환한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:93`~`:96`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:27`~`:29`). 스캐너가 이 값을 가공 없이 UI로 넘기므로(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:73`~`:86`) 표현이 대상별로 갈리고, 키 리바인딩·패드 전환 시 픽업 문구만 실제 입력과 어긋난다.
- **제안**: 계약 주석에 "입력 힌트를 제외한 표시 문구만 반환한다"를 명시하고, 키 표시는 UI가 일관되게 조합하도록 옮긴다.
- **확신도**: 높음

### 4. 🟢 태그 doc-comment에 DP→GP 리네이밍 잔재가 남아 시스템 지도를 오도한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:164`, `:220`
- **범주**: 중복/복잡도
- **문제**: `Damage_CanParry` 주석은 "공격자가 DP를 반사받고"라고 하지만 실제 반사는 GP 어트리뷰트에 적용된다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:362`의 `ApplyAttributeChange(SourceASC, GetGPAttribute(), ReflectAmount)`). `SetByCaller_Duration` 주석의 "DrainDP" 역시 현재 클래스명은 `WxEffect_DrainGP`다(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_DrainGP.h`). `DP`라는 어트리뷰트는 `UWxCombatAttributeSet`에 더 이상 존재하지 않는다(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h:59`~`:65`). 이 헤더는 README가 "각 시스템의 데이터·제어 흐름 지도"로 지목한 진입점이라, 낡은 어트리뷰트명이 그대로 남으면 다른 도메인을 읽는 사람이 존재하지 않는 값을 찾아 헤맨다.
- **제안**: 두 주석의 `DP`를 `GP`, `DrainDP`를 `WxEffect_DrainGP`로 정정한다. (같은 잔재가 `Plugins/WxCore/README.md:34`, `Source/WxGame/Cheat/WxCheatManager.h:24`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp:86`~`:87`에도 있다.)
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **대조용으로 읽은 외부 파일**(발견 근거): `Plugins/WxWorld/.../WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/.../WxDevice.h`·`.cpp`, `Plugins/WxWorld/.../WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxInventory/.../WxItemPickup.cpp`·`.h`, `Plugins/WxDialogue/.../WxDialogueActor.cpp`, `Source/WxGame/.../WxAbility_Interact.cpp`, `Plugins/WxSave/.../WxSaveWorldSubsystem.cpp`, `Plugins/WxCombat/.../WxCombatAttributeSet.h`·`.cpp`, `Config/DefaultEngine.ini`
- **문제로 분류하지 않은 확인 사항**:
  - 코딩·모듈 규칙 위반 없음 — 11파일 전부 `// Copyright Woogle. All Rights Reserved.`로 시작하고, `Wx` prefix가 일관되며, 람다·`FORCEINLINE`·인라인 함수 정의·`BlueprintCallable`이 하나도 없다. `WxCore.Build.cs`는 엔진 모듈만 참조해 도메인 플러그인 의존이 없다.
  - 태그 선언 105개와 정의 105개가 이름·개수 모두 정확히 일치한다. 실사용 스캔 결과 미사용은 `Ability.Pattern.6`~`.9` 4개뿐이며, `.1`~`.5`가 쓰이는 점으로 보아 예약 슬롯으로 판단해 발견에서 제외했다.
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 `WxAttack` 채널 등록과, 주석이 지목한 `WxProjectile` 프리셋이 `:40`과 일치한다.
  - `IWxSavable` 헤더의 저장 계약("전부 기본값이면 기록하지 않고 트랜스폼도 빠진다", "무효 GUID면 제외")이 `UWxSaveWorldSubsystem::CaptureActor`/`RestoreActor` 구현과 정확히 일치한다(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:309`, `:319`, `:352`~`:358`).
- **미검토 / 한계**: `FWxLocatorUtils::GetDisplayName`이 매 호출마다 `Locator.SyncFind()`로 동기 해석을 하고 StateTree 노드 `GetDescription()` 다수가 이를 부르지만(WxWorld·WxQuest·WxUI), 에디터 전용이고 실제 호출 빈도를 엔진 없이 확인할 수 없어 성능 문제로 단정하지 않았다. `UE_DEFINE_GAMEPLAY_TAG`·`TryGetPayloadAs` 등 엔진 매크로·API의 시그니처 적합성은 이 샌드박스에 UE 5.8 소스가 없어 컴파일로 검증하지 못했다. 태그가 BP·DataAsset 안에서 의도대로 소비되는지(바이너리 존재 여부만 확인) 및 BP/WBP 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 11파일 — `/module-review`로 갱신*
