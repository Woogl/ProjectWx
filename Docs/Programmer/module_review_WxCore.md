# WxCore — 코드 리뷰

> 11파일 중 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지는 전부 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 소스 11파일을 모두 통독했고, 태그 110개의 선언↔정의 짝·심볼↔문자열 표기 일치·프로젝트 전체(C++·`*.uasset`/`*.umap`·`Config`) 참조 여부를 전수 대조했으며, `ECC_WxAttack`↔`DefaultEngine.ini` 짝과 `FWxLocatorUtils` 소비처 7곳을 교차 검증했다. 직전 리뷰의 🟢 2번(`WxGameplayTags` 두 파일의 UTF-8 BOM)은 이번 커밋에서 해결됐다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 어디에서도 참조되지 않는 태그 6개 (직전 두 리뷰에서 이월)
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:186`, `:210-214` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:92`, `:112-116`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`, `Ability.Pattern.5` ~ `Ability.Pattern.9` 여섯 개는 C++(`WxGameplayTags::` 심볼)에서도, `Content`·`Plugins/*/Content`의 에셋 문자열에서도, `Config`에서도 참조가 0건이다. 태그 110개 중 나머지 104개는 모두 코드나 에셋에 실제 참조가 있다. 같은 축의 `Ability.Skill.1`·`.3`·`.4`와 `Ability.Pattern.1`~`.4`는 에셋에 있으므로, 연속된 슬롯 표에서 이 여섯 자리만 비어 있는 모양이다. 직전 두 리뷰(`491dd7ec`, `303d8d7f`) 시점과 동일한 여섯 개로, 그동안 늘지도 줄지도 않았다.
- **제안**: 슬롯 예약이 맞다면 그대로 두되 "슬롯 예약, 아직 구현 없음" 한 줄을 남긴다. 예약 의도가 아니면 지운다 — 어빌리티 식별 태그는 대부분 에셋 쪽에서 소비되므로, 미사용이 쌓이면 슬롯 표에서 어디까지가 실재인지 코드만 봐서는 구분되지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **확인했고 문제 없었던 것**:
  - 직전 리뷰의 🟢 2번이 해결됐다 — `WxGameplayTags.h`/`.cpp`의 선두 BOM(`EF BB BF`)이 이번 커밋에서 제거되어, 모듈 12개 소스(`*.h`/`*.cpp`/`*.cs`) 전부가 `2f 2f 20`(`// `)로 시작한다. 규칙 2(첫 줄 Copyright)를 바이트 단위로도 만족한다.
  - 태그 110개가 헤더 선언·cpp 정의 1:1로 짝을 이루고(누락·잉여 0), 문자열 중복도 없으며, 심볼명을 `_`→`.`로 치환한 값이 정의 문자열과 110개 전부 정확히 일치한다(오타 0). 단일 출처 규약도 지켜진다 — WxCore 밖에서 `UE_DEFINE_GAMEPLAY_TAG`을 쓰는 곳이 없다.
  - 이번 커밋 범위의 태그 변경이 실제 구현과 맞는다. 추가된 `State.Minion.Active`의 doc-comment("`UWxMinionSubsystem`이 주인 ASC에 복제 loose 태그로 발행")는 `WxMinionSubsystem.cpp:227`의 `SetLooseGameplayTagCount(..., EGameplayTagReplicationState::TagOnly)`와 일치하고, `Event.CommandMinionAbility`도 같은 파일 `:249`에서 소비된다. `Effect.HitStop` 주석이 새로 가리키는 `InstigatorHitStop`·`VictimHitStop`은 `WxWeaponBase.h:38,42`·`WxProjectileBase.h:46,50`에 실재하며 `WxWeaponBase.cpp:267-268`·`WxProjectileBase.cpp:132-133`에서 그대로 쓰인다. 삭제된 `Event.SpawnProjectile`·`Event.SpawnMinion`은 서브시스템 리팩터링(`6512437`)으로 소비처가 사라진 뒤 함께 정리된 것으로, 잔존 참조가 0건이다.
  - `Cooldown.*` 3개(`Dodge`·`Skill.1`·`Ultimate`)는 모두 짝이 되는 `UWxEffect_Cooldown_*` GE 클래스가 `WxEffect_Cooldown.cpp:52-65`에 실재하고, 뿌리 태그 `Cooldown`은 `WxEffect_NoCooldown.cpp:17`의 일괄 제거 쿼리가 쓴다. 쿨다운 태그가 `Skill.2`~`.4`에 없는 것은 해당 GE 클래스도 없기 때문이라 어긋남이 아니다.
  - `ECC_WxAttack`(`WxCollisionChannels.h:15`)과 `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`/`Name="WxAttack"`/`DefaultResponse=ECR_Block` 짝이 정확히 일치하고, doc-comment가 서술하는 메시 Overlap·캡슐 Ignore override도 `WxCharacterBase.cpp:28,32`와 맞는다. `WxProjectile` 프리셋 서술도 `WxProjectileBase.cpp:24`와 일치한다.
  - 규칙 위반 없음: `Wx` prefix, 인라인 함수 정의 없음(`ECC_WxAttack`은 `inline constexpr` **상수**라 규칙 6과 무관), `BlueprintCallable` 없음, 람다 없음, 델리게이트 콜백 없음. 인터페이스 기본 구현 두 개(`IWxInteractable::CanInteract`, `IWxUIData::GetMaxRecharges`)는 모두 cpp에 내려가 있고, 그에 맞게 U-class는 `MinimalAPI`, I-class는 `WXCORE_API`로 짝이 맞다.
  - WxCore는 다른 Wx 플러그인을 참조하지 않는다 — `Core`/`CoreUObject`/`Engine`/`GameplayTags`에 에디터 전용 `UniversalObjectLocator`(엔진 런타임 모듈, 플러그인 아님)뿐이고, `WxCore.uplugin`에 `Plugins` 항목이 없다. `Build.cs`의 `bBuildEditor` 블록과 `WxLocatorUtils.h`의 `#if WITH_EDITOR` 구간도 정합하며, Content 폴더가 없어 에셋 역방향 의존도 성립하지 않는다.
  - `FWxLocatorUtils` 소비처 7곳(`WxActorLocatorCustomization.cpp:143`, StateTree 노드 `GetDescription()` 5곳, `WxSpawnerLocatorUtils.cpp:29`의 컴파일 검증)이 전부 에디터 경로라 `SyncFind`·`FText` 왕복이 핫패스에 놓이지 않는다.
  - 빈 `FWxCoreModule`(`WxCoreModule.h:8-13`, `WxCoreModule.cpp:6-9`)은 WxAI·WxCombat·WxDialogue·WxInventory·WxQuest·WxWorld가 모두 똑같이 쓰는 프로젝트 공통 보일러플레이트라 WxCore만 지적할 사안이 아니다.
- **미검토 / 한계**: 태그의 에셋 참조 여부는 `.uasset`/`.umap` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 1번 발견이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다. 엔진 소스가 이 환경에 없어 `FUniversalObjectLocator::SyncFind`가 동기 로드를 유발하지 않는다는 점은 직전 리뷰(엔진 5.8 대조)의 결론을 그대로 승계했고 이번에 재검증하지 못했다.

---
*문서 기준 커밋 `6ea7624` · 리뷰일 2026-09-06 · 소스 11파일 — `/module-review`로 갱신*
