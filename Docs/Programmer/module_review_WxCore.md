# WxCore — 코드 리뷰

> foundation 모듈답게 실행 로직이 거의 없고, 태그 선언·인터페이스 계약·콜리전 상수만 담겨 있어 전반적으로 매우 건강하다. 이번 리뷰는 13개 소스 전부를 읽고, 유일하게 로직이 있는 `FWxLocatorUtils`를 엔진 UOL 구현까지 내려가 검증했으며, 114개 Native Tag의 선언·정의 일치와 doc-comment가 서술하는 도메인 간 계약을 실제 소비 코드와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 README 오리엔테이션 맵이 `IWxMinion` 추가를 반영하지 못했다
- **위치**: `Plugins/WxCore/README.md:17-24`, `Plugins/WxCore/README.md:29-38`, `Plugins/WxCore/README.md:54`
- **범주**: 설계/구조
- **문제**: `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`의 `IWxMinion`은 `WxCombat`(`Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:46,60`)과 `WxAI`(`Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:78-79`) 양쪽이 쓰는 정식 도메인 간 계약인데, README의 「책임」·「핵심 타입」·「여기서부터 읽어라」 어디에도 없다. README를 지도로 삼는 세션은 WxCore가 계약 4개를 소유한다고 읽지만 실제로는 5개다. 같은 맥락에서 「주요 네임스페이스」 목록에 `Movement.*`(`WxGameplayTags.h:51-52`, `WxCharacterMovementComponent.cpp:82`·`WxAbility_Sprint.cpp:132`에서 실사용)가 빠져 있고, 말미의 "소스 11파일"도 현재 13파일과 어긋난다.
- **제안**: `/readme-writer`로 갱신한다. 코드 수정은 필요 없다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`.
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`.
- **검증해 문제없음으로 판정한 항목**:
  - 태그 114개의 선언·정의가 1:1로 대응하고, 심볼의 `_`를 `.`으로 치환한 문자열이 정의 문자열과 전부 일치한다. 누락·중복·오타 없음.
  - 프로젝트 전체에서 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`FNativeGameplayTag`는 WxCore에만 존재하고 `Config/DefaultGameplayTags.ini`도 없다 — 단일 선언처 규약이 지켜지고 있다.
  - `FWxLocatorUtils::GetDisplayName`의 `SyncFind()`는 엔진 구현상 `FSoftObjectPath::ResolveObject()`로 끝나 동기 로드를 일으키지 않으므로 에디터 UI 경로의 성능 위험이 없다(`Engine/Private/UniversalObjectLocators/ActorLocatorFragment.cpp:144-164`). `TryGetPayloadAs`는 payload가 null일 수 있어 뒤따르는 `&& Payload`도 잉여가 아니다. 이 헬퍼를 쓰는 UOL 프로퍼티는 전부 `meta = (AllowedLocators = "Actor")`라 `FActorLocatorFragment` 가정이 성립한다.
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 `WxAttack` 등록값과 일치한다. 헤더 주석의 "메시 Overlap / 캡슐 Ignore"도 `Source/WxGame/Character/WxCharacterBase.cpp:26,30`의 C++ override와 일치한다(ini의 `WxCharacterMesh` 프로파일은 Block이지만 생성자에서 덮으므로 주석이 옳다).
  - doc-comment가 서술하는 계약을 실제 발행·구독처와 대조했다 — `State.Minion.Active`(TagOnly 복제, `WxMinionSubsystem.cpp:228`), `State.Ragdoll`(`WxAbility_Death.cpp:100` → `WxCharacterBase.cpp:57,61`), `State.Dialogue`(`WxDialogueSessionComponent.cpp:153` → `WxAbility_Interact.cpp:36`), `Effect.HitStop`("애니메이션과 이동을 함께" — `WxHitStopComponent.cpp:107,114`가 둘 다 처리) 등 모두 코드와 일치했다. 낡은 계약 서술을 찾지 못했다.
  - CLAUDE.md 코딩 규칙 전수 확인: 13개 소스 + `Build.cs` 전부 첫 줄 Copyright 있음, `Wx` prefix 누락 없음, `BlueprintCallable` 0건, 람다 0건, 인라인 함수 정의 0건(`WxCollisionChannels.h:15`의 `inline constexpr`은 변수라 규칙 6 대상이 아니다), 델리게이트 콜백 자체가 없어 `Handle` prefix 대상 없음.
  - `WxMinion.h:25,30`의 `BlueprintNativeEvent`는 UHT가 `FUNC_BlueprintCallable`(0x04000000)을 부여하지 않는다(생성 코드의 함수 플래그 `0x48020C00`으로 확인). 규칙 5 위반이 아니다.
  - `Build.cs`에 다른 Wx 플러그인 참조가 없고, `UniversalObjectLocator`만 `bBuildEditor` 조건부로 들어가 헤더의 `WITH_EDITOR` 가드와 정확히 맞는다. foundation 경계 위반 없음.
- **발견으로 올리지 않은 판단**: `Ability.Skill.3/4`, `Cooldown.Skill.3/4`, `Ability.Pattern.5`~`.9` 9개 태그는 C++·Content 어디에서도 참조되지 않는다. 다만 이들은 4스킬·9패턴이라는 슬롯 체계의 예약 번호이고 헤더가 그 명명 규약을 명시하므로, 삭제가 옳은 조치가 아니라 발견에서 뺐다. 다음 리뷰에서 재판정하지 않아도 된다. `Movement.*`와 `Damage.CanCritical`에 doc-comment가 없는 것도 주변 태그와의 일관성 문제일 뿐이라 제외했다. `FWxCoreModule`의 빈 `StartupModule`/`ShutdownModule`은 모든 Wx 플러그인이 공유하는 하우스 패턴이라 제외했다.
- **미검토 / 한계**: 정적 리뷰이며 빌드·PIE·패키징은 실행하지 않았다. Content 에셋 대조는 `.uasset` 안의 문자열 존재 여부만 확인한 것이라, 문자열이 있어도 실제로 유효한 프로퍼티에 들어 있는지까지는 보장하지 못한다. `IWxMinion`이 doc-comment로 요구하는 "구현체는 `IGenericTeamAgentInterface`도 함께 구현" 조건은 코드로 강제되지 않지만, 진단 가드를 두지 않는 기존 방침에 따라 발견으로 올리지 않았다 — BP 구현체가 이를 지키는지는 에셋 검증 범위다.

---
*문서 기준 커밋 `04420d246` · 리뷰일 2026-09-12 · 소스 13파일 — `/module-review`로 갱신*
