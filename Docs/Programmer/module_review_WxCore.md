# WxCore — 코드 리뷰

> 11파일 중 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지는 전부 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 소스 11파일을 모두 통독했고, 태그 111개의 선언↔정의 짝·심볼↔문자열 표기 일치·프로젝트 전체(C++ 심볼, `Content`/`Plugins/*/Content`의 `.uasset`·`.umap` 문자열, `Config`) 참조 여부를 전수 대조했으며, `ECC_WxAttack`↔`Config/DefaultEngine.ini` 채널 등록, `GameplayCue.*` 9개↔`Content/AbilitySystem/Cue`의 노티파이 에셋과 쿠킹 경로, `FUniversalObjectLocator::SyncFind`의 엔진 5.8 구현(동기 로드 없음)까지 실물로 확인했다. 직전 리뷰(`53bc7de6`)의 🟡 1건(`State.Minion.Active` 주석이 발행자를 부정하던 문제)은 수정되어 이번 판정에서 제외했고, 새로 생긴 문제는 없다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 어디에서도 참조되지 않는 태그 6개 (네 번째 리뷰 연속 이월)
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:190`, `:214-218` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:93`, `:113-117`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`, `Ability.Pattern.5` ~ `Ability.Pattern.9` 여섯 개는 C++(`WxGameplayTags::` 심볼)에서도, 에셋 문자열에서도, `Config`에서도 참조가 0건이다. 태그 111개 중 나머지 105개는 모두 코드나 에셋에 실제 참조가 있다. 같은 축의 `Ability.Skill.1`·`.3`·`.4`와 `Ability.Pattern.1`~`.4`는 에셋에 있으므로, 연속된 슬롯 표에서 이 여섯 자리만 비어 있는 모양이다. 직전 세 리뷰(`303d8d7f`, `6ea7624`, `53bc7de6`) 시점과 정확히 같은 여섯 개로, 그동안 늘지도 줄지도 않았다 — 서서히 썩는 중이 아니라 고정된 예약 슬롯일 개연성이 높다.
- **제안**: 슬롯 예약이 맞다면 그대로 두되 해당 블록에 "슬롯 예약, 아직 구현 없음" 한 줄을 남긴다(그러면 다음 리뷰가 매번 같은 항목을 다시 파헤치지 않는다). 예약 의도가 아니면 지운다 — 어빌리티 식별 태그는 대부분 에셋 쪽에서 소비되므로, 미사용이 쌓이면 슬롯 표에서 어디까지가 실재인지 코드만 봐서는 구분되지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
  - 대조를 위해 함께 읽은 모듈 밖 파일: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`, `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`
  - 이번에 확인했고 문제 없었던 것: 태그 111개의 헤더 선언↔cpp 정의 1:1(누락·잉여 0), 심볼명 `_`→`.` 치환값과 정의 문자열 111개 전부 일치(오타 0), 태그 단일 출처 규약 유지(WxCore 밖에 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`UE_DEFINE_GAMEPLAY_TAG` 0건, `Config`에 태그 목록 항목 없음). `ECC_WxAttack`(`WxCollisionChannels.h:15`)↔`Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`/`Name="WxAttack"`/`DefaultResponse=ECR_Block` 일치, doc-comment가 서술하는 메시 Overlap·캡슐 Ignore override 실재(`WxCharacterBase.cpp:27,31`), `"WxProjectile"` 프리셋 실재(`:40`). `GameplayCue.*` 9개 모두 `Content/AbilitySystem/Cue`에 짝 노티파이가 있고 그 경로가 `DefaultGame.ini:21,25`의 `GameplayCueNotifyPaths`·`DirectoriesToAlwaysCook` 양쪽에 등록돼 있다. 공용 계약 두 개 모두 살아 있고(`IWxInteractable` 5개 액터 구현·2곳 소비, `IWxUIData` 2개 구현·4곳 소비), `IWxUIData::GetMaxRecharges`가 어빌리티 전용 개념인데도 foundation 계약에 얹힌 것은 WxUI가 WxCombat을 참조할 수 없는 모듈 규칙상 유일한 통로라 설계 결함이 아니다. 규칙 위반 없음 — `Wx` prefix, 소스 12개(`*.h`/`*.cpp`/`*.cs`) 전부 BOM 없이 저작권 첫 줄, 인라인 함수 정의 0(`ECC_WxAttack`은 `inline constexpr` **상수**라 규칙 6과 무관), `BlueprintCallable` 0, 람다 0, 델리게이트 콜백 0. WxCore는 다른 Wx 플러그인을 참조하지 않는다(`Core`/`CoreUObject`/`Engine`/`GameplayTags` + 에디터 전용 엔진 모듈 `UniversalObjectLocator`뿐, `.uplugin`에 `Plugins` 항목 없음, Content 폴더 없음).
- **미검토 / 한계**:
  - 태그의 에셋 참조 여부는 `.uasset`/`.umap` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 1번 발견이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다.
  - `Config/DefaultEngine.ini:41`의 `WxCharacterMesh` 프로파일이 `WxAttack`에 `ECR_Block`을 지정해 `WxCollisionChannels.h`의 doc-comment(메시는 Overlap)와 어긋나지만, 코드·에셋 어디에서도 이 프로파일을 쓰지 않고 무기 스윕이 형상 자신의 응답(`Pawn=Overlap`)을 `FCollisionResponseParams`로 넘기므로(`WxWeaponBase.cpp:200-202`) 판정이 Block으로 승격되지 않는다. WxCore 밖 사안이라 발견으로 올리지 않았고, 프로파일 정리 여부는 판단하지 않았다.
  - 리뷰 대상은 커밋 `262e4cca`의 작업 트리(clean)이며, 라인 번호도 그 기준이다.

---
*문서 기준 커밋 `262e4cca` · 리뷰일 2026-09-08 · 소스 11파일 — `/module-review`로 갱신*
