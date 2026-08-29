# WxCore — 코드 리뷰

> 공용 태그·채널·도메인 계약을 얇게 유지해 의존 경계는 건강하다. 이번 리뷰는 `WxCore`의 11개 C++ 소스와 Build 규칙을 읽고, `IWxInteractable` 계약의 소비 지점 및 관련 구현체를 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 토글을 지원하지 않는 상호작용 대상이 조용히 성공 처리된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:37`
- **범주**: 설계/구조
- **문제**: `SetInteractionEnabled()`는 기본 구현이 no-op인데, 계약에 토글 지원 여부를 표현하는 방법이 없다. 호출자인 `FWxStateTreeTask_EnableInteraction`은 대상이 `IWxInteractable`이면 호출 직후 항상 `Succeeded`를 반환한다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:76`, `:84`~`:86`). 그러나 `AWxItemPickup`과 `AWxEnemyCharacter`는 이 함수를 재정의하지 않는다(`Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h:41`~`:44`, `Source/WxGame/Character/WxEnemyCharacter.h:37`~`:40`). 따라서 StateTree가 이들을 토글 대상으로 지목하면 아무 변화나 경고 없이 성공해 저작 오류를 발견하기 어렵다.
- **제안**: `SetInteractionEnabled()`를 순수 가상 함수로 만들어 구현체가 지원 여부를 명시하게 하거나, 별도 지원 여부 API/결과값을 도입해 호출자가 미지원 대상을 경고하도록 한다.
- **확신도**: 높음

### 2. 🟢 상호작용 프롬프트의 키 힌트 소유자가 계약에 정의되지 않았다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:35`
- **범주**: 설계/구조
- **문제**: `GetInteractionPrompt()`가 액션 설명만 반환하는지 입력 키까지 포함하는지 명시하지 않는다. 그 결과 픽업은 `"[F] {0}"`를 반환하는 반면(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:107`~`:108`), 장치와 대화 액터는 순수 문구를 반환한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:93`~`:96`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:27`~`:29`). 스캐너는 이 값을 그대로 UI에 전달하므로(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:73`~`:86`) 표현이 대상별로 갈리고, 키 리바인딩 때 픽업 문자열만 실제 입력과 달라질 수 있다.
- **제안**: 계약에 "입력 힌트를 제외한 표시 문구"를 반환한다고 명시하고 입력 키 표시는 UI가 일관되게 조합하도록 정한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **미검토 / 한계**: Native Gameplay Tag가 바이너리 에셋에서 실제로 소비되는지와 런타임 DataTable/BP 의도는 검토하지 않았다. 태그 선언과 정의의 대응, `ECC_WxAttack`과 `Config/DefaultEngine.ini`의 채널 등록 대응, `IWxSavable`의 저장 경로는 확인했으나 문제로 분류할 근거는 없었다.

---
*문서 기준 커밋 `b48c1930` · 리뷰일 2026-08-29 · 소스 11파일 — `/module-review`로 갱신*
