# WxCore — 코드 리뷰

> 11파일 전부가 선언·상수·인터페이스 계약이고 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이라, foundation 모듈로서 매우 얇고 건강하다. 소스 11파일을 모두 통독했고, 여기에 더해 태그 109개의 선언·정의 짝과 프로젝트 전체(C++·Content·Config) 참조 여부, `ECC_WxAttack`의 `DefaultEngine.ini` 매핑, `FWxLocatorUtils` 소비처 6곳의 빌드 의존성을 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `ECC_WxAttack`이 ini 등록 순서에 묶여 있으나 어긋나도 조용히 지나간다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:15`
- **범주**: 설계/구조
- **문제**: `ECC_WxAttack = ECC_GameTraceChannel1`은 `Config/DefaultEngine.ini:39`의 `+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1, ..., Name="WxAttack")` 한 줄과만 짝을 이룬다. 현재는 정확히 일치하지만(확인함), 이 대응은 컴파일러도 엔진도 검사하지 않는다. 프로젝트 설정에서 채널을 지우거나 순서를 바꾸면 `ECC_WxAttack`이 다른 채널을 가리키게 되고, 그 결과는 빌드 에러나 로그가 아니라 **무기·투사체 히트 판정이 통째로 어긋나는 런타임 증상**으로만 드러난다. 무기 히트박스는 이 채널을 Object Type으로 쓰고 캐릭터 메시는 `WxCharacterBase.cpp:30`에서 이 채널에 Overlap을 건다 — 전투 전체가 이 한 상수 위에 서 있다.
- **제안**: `FWxCoreModule::StartupModule()`에서 `UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(ECC_WxAttack)`이 `"WxAttack"`인지 `ensureMsgf`로 한 번 확인한다. 지금 비어 있는 그 함수가 유일하게 자연스러운 자리다. 반대로 진단 가드를 두지 않기로 하면 `FWxCoreModule` 클래스 자체가 빈 override 두 개뿐이므로(`WxCoreModule.h:8-13`, `WxCoreModule.cpp:6-9`) `IMPLEMENT_MODULE(FDefaultModuleImpl, WxCore)` 한 줄로 줄이고 두 파일을 지우는 쪽이 정직하다. 둘 중 하나는 정해야 한다.
- **확신도**: 중간

### 2. 🟢 어디에서도 참조되지 않는 태그 6개
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:181`, `:205-209`
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`, `Ability.Pattern.5` ~ `Ability.Pattern.9` 여섯 개는 C++(`WxGameplayTags::` 심볼 참조)에서도, `Content`·`Plugins/*/Content`의 에셋 문자열에서도, `Config`에서도 단 한 건도 참조되지 않는다. 나머지 103개는 모두 코드나 에셋에 실제 참조가 있다. 같은 축의 `Ability.Skill.3`·`Ability.Skill.4`·`Ability.Pattern.1`~`.4`는 에셋에 존재하므로, 빠진 자리만 비어 있는 모양이다.
- **제안**: 슬롯 예약이 맞다면 그대로 두되 "슬롯 예약, 아직 구현 없음" 주석을 한 줄 남긴다. 예약 의도가 아니면 지운다 — 어빌리티 식별 태그는 에셋에서 소비되므로 미사용 태그가 쌓이면 슬롯 표에서 어디까지가 실재인지 구분이 안 된다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 Public 헤더가 노출하는 `FUniversalObjectLocator`의 모듈 의존성이 Private이다
- **위치**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs:19-25`
- **범주**: 설계/구조
- **문제**: `WxLocatorUtils.h`는 Public 헤더인데 두 함수 시그니처가 모두 `FUniversalObjectLocator`를 받고(`WxLocatorUtils.h:14,17`), 정작 `UniversalObjectLocator` 모듈은 `PrivateDependencyModuleNames`에 있다. 헤더가 전방 선언만 하므로 지금은 컴파일되고, 현 소비처 4개 모듈(`WxQuest`·`WxUI`·`WxWorld`·`WxEditor`)은 전부 스스로 `UniversalObjectLocator`를 선언하고 있어 실제 고장은 없다. 다만 새 소비 모듈이 `WxLocatorUtils.h`만 include하면 불완전 타입 에러를 만나게 된다.
- **제안**: `bBuildEditor` 블록의 `PrivateDependencyModuleNames`를 `PublicDependencyModuleNames`로 바꾸면 전파된다. 실익이 작다고 판단하면 헤더 doc-comment에 "소비 모듈이 `UniversalObjectLocator` 의존성을 직접 선언해야 한다"고 적어 두는 것으로 갈음해도 된다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`
- **확인했고 문제 없었던 것**:
  - 태그 109개가 헤더 선언·cpp 정의 1:1로 짝을 이루며 중복·누락이 없다.
  - `ECC_WxAttack`의 doc-comment(`WxCollisionChannels.h:11-13`)가 `Config/DefaultEngine.ini`의 `DefaultResponse=ECR_Block`, `WxProjectile` 프리셋, `WxCharacterBase.cpp:30,34`의 메시 Overlap·캡슐 Ignore override와 모두 일치한다.
  - `FUniversalObjectLocator::SyncFind()`는 `FActorLocatorFragment` 경로에서 `ResolveObject()`만 타므로 동기 패키지 로드를 유발하지 않는다 — 에디터 표시명 경로의 성능 위험은 없다.
  - 규칙 위반 없음: 전 파일 첫 줄 저작권 표기, `Wx` prefix, 인라인 함수 정의 없음(`ECC_WxAttack`은 `constexpr` 상수라 해당 없음), `BlueprintCallable` 없음, 람다 없음, 델리게이트·콜백 없음. 인터페이스 기본 구현 두 개(`IWxInteractable::CanInteract`, `IWxUIData::GetMaxRecharges`)는 모두 cpp에 내려가 있고 그에 맞게 I-class에 `WXCORE_API`가 붙어 있다.
  - WxCore는 다른 Wx 플러그인을 참조하지 않는다(`Core`/`CoreUObject`/`Engine`/`GameplayTags` + 에디터 전용 `UniversalObjectLocator`뿐).
- **미검토 / 한계**: 태그의 에셋 참조 여부는 `.uasset` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 2번 발견이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다.

---
*문서 기준 커밋 `491dd7ec` · 리뷰일 2026-09-05 · 소스 11파일 — `/module-review`로 갱신*
