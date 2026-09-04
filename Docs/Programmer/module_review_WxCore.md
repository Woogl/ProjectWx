# WxCore — 코드 리뷰

> 11파일 중 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지는 전부 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 소스 11파일을 모두 통독했고, 태그 111개의 선언·정의 짝과 프로젝트 전체(C++·Content·Config)에서의 참조 여부를 전수 대조했으며, `ECC_WxAttack`↔`DefaultEngine.ini` 짝·`FWxLocatorUtils` 소비처 7곳·`UniversalObjectLocator`의 엔진 모듈 종류(플러그인 아님)를 교차 검증했다. 직전 리뷰의 🟢 3번(Public 헤더가 노출하는 UOL 의존성이 Private)은 해결됐다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟢 어디에서도 참조되지 않는 태그 6개 (직전 리뷰에서 이월)
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:187`, `:211-215` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:93`, `:113-117`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`, `Ability.Pattern.5` ~ `Ability.Pattern.9` 여섯 개는 C++(`WxGameplayTags::` 심볼)에서도, `Content`·`Plugins/*/Content`의 에셋 문자열에서도, `Config`에서도 참조가 0건이다. 태그 111개 중 나머지 105개는 모두 코드나 에셋에 실제 참조가 있다. 같은 축의 `Ability.Skill.1`·`.3`·`.4`와 `Ability.Pattern.1`~`.4`는 에셋에 있으므로, 연속된 슬롯 표에서 이 여섯 자리만 비어 있는 모양이다. 직전 리뷰(`491dd7ec`) 시점과 동일한 여섯 개로, 그동안 늘지도 줄지도 않았다.
- **제안**: 슬롯 예약이 맞다면 그대로 두되 "슬롯 예약, 아직 구현 없음" 한 줄을 남긴다. 예약 의도가 아니면 지운다 — 어빌리티 식별 태그는 대부분 에셋 쪽에서 소비되므로, 미사용이 쌓이면 슬롯 표에서 어디까지가 실재인지 코드만 봐서는 구분되지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 2. 🟢 `WxGameplayTags` 두 파일에만 UTF-8 BOM이 붙어 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:1`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:1`
- **범주**: 규칙 위반
- **문제**: 두 파일의 선두 바이트가 `EF BB BF`(UTF-8 BOM)로 시작해 첫 줄이 실제로는 `// Copyright ...`가 아니라 BOM + 그 문자열이다. 모듈의 나머지 9개 소스 파일(`WxCollisionChannels.h`·`WxInteractable.h/.cpp`·`WxUIData.h/.cpp`·`WxLocatorUtils.h/.cpp`·`WxCoreModule.h/.cpp`·`WxCore.Build.cs`)에는 BOM이 없다. UBT가 MSVC에 `/source-charset:utf-8`을 넘기므로 한글 주석 인코딩 관점에서 컴파일은 멀쩡하고 실제 고장은 없다. 다만 "모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다"를 바이트 단위로 검사하는 스크립트·훅은 이 두 파일에서만 실패한다(리뷰 중 실제로 재현했다). 하필 이 두 파일이 직전 커밋에서 편집된 파일이라, 편집 도구가 남긴 흔적으로 보인다.
- **제안**: 두 파일을 BOM 없는 UTF-8로 다시 저장해 나머지 9개와 맞춘다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`
- **확인했고 문제 없었던 것**:
  - 태그 111개가 헤더 선언·cpp 정의 1:1로 짝을 이루고, 심볼·문자열 모두 중복이나 누락이 없다. 태그 단일 출처 규약도 지켜진다 — `Config`에 `GameplayTagList` 항목이 없고, WxCore 밖에서 `UE_DEFINE_GAMEPLAY_TAG`을 쓰는 곳도 없다(`WxDeviceStateTreeComponent.cpp`의 `RequestGameplayTag`는 저작 문자열을 태그로 되돌리는 용도라 별건).
  - 태그 doc-comment가 가리키는 구현체가 전부 실재한다(`UWxExecCalc_Damage`·`UWxEffect_Cooldown`·`UWxHitStopComponent`·`WxEffect_Invincible/GuardReduction/PerfectGuard/Exhaust/SuperArmor/HitStop/MoveSpeedScale/NoCooldown/InfiniteMP`). `Damage.GuardBreak`→`Event.Hit.GuardBreak` 흐름 서술도 `WxEffect_Damage.cpp:207`·`WxCombatAttributeSet.cpp:309-311`과 일치한다. README가 말하는 "태그 헤더가 곧 전투·장치 지도" 역할이 실제로 유효하다.
  - `ECC_WxAttack`(`WxCollisionChannels.h:15`)과 `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`/`Name="WxAttack"` 짝이 현재 정확히 일치하고, doc-comment의 `DefaultResponse=Block`·`WxProjectile` 프리셋 서술과 `WxCharacterBase.cpp:30,34`의 메시 Overlap·캡슐 Ignore override도 모두 맞다. 직전 리뷰의 🟡 1번(ini 어긋남을 검사하지 않음)은 계약을 코드·README 양쪽에 문서화하는 쪽으로 이미 정리된 사안이라 발견으로 다시 올리지 않았다.
  - `FUniversalObjectLocator::SyncFind(nullptr)`은 엔진 5.8 `FActorLocatorFragment::Resolve`에서 `Path.ResolveObject()`까지만 내려가고 `bLoadIfExists=false`라 동기 패키지 로드를 유발하지 않는다. 소비처 7곳이 전부 StateTree 노드의 `GetDescription()`·`WxActorLocatorCustomization`·컴파일 검증이라 핫패스도 아니다.
  - `UniversalObjectLocator`는 엔진 **런타임 모듈**(플러그인 아님)이라 `WxCore.uplugin`에 의존성 항목이 필요 없다. `bBuildEditor` 블록의 Public 승격도 `WxLocatorUtils.h`의 `#if WITH_EDITOR` 구간과 정합한다.
  - 규칙 위반 없음: `Wx` prefix, 인라인 함수 정의 없음(`ECC_WxAttack`은 `constexpr` 상수라 해당 없음), `BlueprintCallable` 없음, 람다 없음, 델리게이트 콜백 없음. 인터페이스 기본 구현 두 개(`IWxInteractable::CanInteract`, `IWxUIData::GetMaxRecharges`)는 모두 cpp에 내려가 있고 그에 맞게 I-class에 `WXCORE_API`가 붙어 있다.
  - WxCore는 다른 Wx 플러그인을 참조하지 않고(`Core`/`CoreUObject`/`Engine`/`GameplayTags` + 에디터 전용 `UniversalObjectLocator`), Content 폴더도 없다. 도메인 타입이 흘러든 흔적은 없다.
  - 빈 `FWxCoreModule`(`WxCoreModule.h:8-13`, `WxCoreModule.cpp:6-9`)은 WxAI·WxCombat·WxDialogue·WxInventory·WxQuest·WxWorld가 모두 똑같이 쓰는 프로젝트 공통 보일러플레이트라 WxCore만 지적할 사안이 아니다.
- **미검토 / 한계**: 태그의 에셋 참조 여부는 `.uasset` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 1번 발견이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `303d8d7f` · 리뷰일 2026-09-05 · 소스 11파일 — `/module-review`로 갱신*
