# WxCore — 코드 리뷰

> foundation 모듈답게 실행 로직이 거의 없고 태그 선언·인터페이스 계약·콜리전 상수만 담겨 있어 여전히 매우 건강하다. 직전 리뷰(`04420d246`) 이후 이 모듈의 C++ 변경은 0건이고 `README.md`만 갱신되었으므로 이번은 재검증 패스다 — 13개 소스 전부를 다시 읽고, 114개 Native Tag의 선언·정의 일치, foundation 의존 경계(`*.Build.cs`·`*.uplugin`), CLAUDE.md 코딩 규칙 전수, doc-comment가 서술하는 도메인 간 계약을 실제 소비 코드와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 README 「주요 네임스페이스」 목록에 `Movement.*`만 빠져 있다
- **위치**: `Plugins/WxCore/README.md:29-38`
- **범주**: 설계/구조
- **문제**: 직전 리뷰가 지적한 README 갭 중 `IWxMinion` 누락과 파일 수 불일치는 해소되었으나, 「주요 네임스페이스」 목록에 `Movement.*`(`Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:51-52`)가 여전히 없다. 이 네임스페이스는 실사용 계약이다 — `Movement.Sprint`는 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp:132`가 loose 태그로 발행하고 `WxEffect_DrainSP.cpp:16`·`WxEffect_RegenSP.cpp:17`이 SP 소모·회복의 조건으로 읽으며, `Movement.InAir`는 `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp:82,86`이 갱신한다. 목록이 `HitReact.*`(5개)·`SetByCaller.*`(4개)처럼 작은 묶음까지 다 담고 있어 실질적으로 전수 목록이라, 유일하게 빠진 `Movement.*`는 큐레이션보다 누락으로 읽힌다. README를 지도로 삼는 세션이 SP 게이팅의 태그 축을 놓친다.
- **제안**: `/readme-writer`로 갱신해 `Movement.*` 한 줄(스프린트 상태 → SP 소모·회복 게이트, 공중 상태)을 추가한다. 코드 수정은 필요 없다.
- **확신도**: 중간(「주요」 목록의 편집 판단일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`.
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`.
- **검증해 문제없음으로 판정한 항목**:
  - 태그 선언 114개와 정의 114개가 심볼 단위로 완전히 일치하고(선언만 있고 정의가 없는 태그 0건, 정의만 있는 태그 0건), 심볼의 `_`를 `.`으로 치환한 문자열이 `UE_DEFINE_GAMEPLAY_TAG`의 문자열과 전부 일치한다 — 이름 오타·중복 없음.
  - 프로젝트 전체에서 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`FNativeGameplayTag`는 `WxGameplayTags.h`에만 존재하고 `Config/`에 `DefaultGameplayTags.ini`가 없다 — "태그는 이 두 파일에만"이라는 단일 선언처 규약이 지켜진다.
  - foundation 경계: `WxCore.Build.cs`의 의존은 엔진 모듈뿐이고(`Core`/`CoreUObject`/`Engine`/`GameplayTags`, 에디터 빌드에서만 `UniversalObjectLocator`), `WxCore.uplugin`에도 `Plugins` 항목이 없다 — 다른 Wx 플러그인을 거꾸로 참조하는 지점 0건. 반대 방향(7개 도메인 플러그인 + `WxGame`·`WxEditor`이 `WxCore`를 참조)만 성립한다.
  - `UniversalObjectLocator`를 `Target.bBuildEditor` 조건부로 넣은 것이 `WxLocatorUtils.h:12`·`WxLocatorUtils.cpp:5`의 `WITH_EDITOR` 가드와 정확히 맞고, 이 타입을 런타임 구조체에 쓰는 소비 모듈(`WxWorld`·`WxUI`·`WxQuest`·`WxDialogue`)은 각자 무조건부로 의존을 선언하고 있어 WxCore의 조건부 노출에 기대는 모듈이 없다 — 패키징 빌드에서 끊어질 전파 경로 없음.
  - CLAUDE.md 코딩 규칙 전수: 13개 소스와 `WxCore.Build.cs` 전부 첫 줄 Copyright 있음, `Wx` prefix 누락 없음, 람다 0건, `BlueprintCallable` 0건, 인라인 함수 정의 0건(`WxCollisionChannels.h:15`의 `inline constexpr`은 함수가 아니라 변수라 규칙 대상이 아니다), 델리게이트 콜백 자체가 없어 `Handle` prefix 대상 없음.
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 `WxAttack` 등록값과 일치한다. 헤더 주석의 "메시 Overlap / 캡슐 Ignore"도 `Source/WxGame/Character/WxCharacterBase.cpp:26,30`의 C++ override와 일치한다(ini의 `WxCharacterMesh` 프로파일은 WxAttack에 Block이지만 생성자가 덮으므로 주석이 옳다).
  - 인터페이스 계약이 소비처에서 지켜진다 — `IWxMinion::GetMaxCountPerMaster`의 "음수는 0으로 보정"은 `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:45`의 `FMath::Max(0, ...)`가 실제로 수행하고, `IsAggroIgnored`는 `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:78-79`가 `Execute_` 경로로 읽는다. `IWxUIData`는 `WxAbilityBase`·`WxEffectComponent_Table`이 구현하고 `WxUI`의 ViewModel이 `IWxUIData` 포인터로만 소비해 도메인 역참조가 없다. `IWxInteractable`은 doc-comment대로 액터만 구현한다(`AWxDevice`·`AWxDialogueActor`·`AWxItemPickup`·`AWxNpc`·`AWxEnemyCharacter`, 컴포넌트 구현체 0건).
- **발견으로 올리지 않은 판단**: `Ability.Skill.3/4`와 `Ability.Pattern.5`~`.9` 7개 태그는 C++·Content 어디에서도 참조되지 않지만, 4스킬·9패턴이라는 슬롯 체계의 예약 번호이고 헤더가 그 명명 규약을 명시하므로 삭제가 옳은 조치가 아니다(직전 리뷰와 동일 판정 — 다음 리뷰에서 재판정하지 않아도 된다). 짝이 되는 `Cooldown.Skill.3/4`는 `WxEffect_Cooldown.cpp:69,74`가 실제로 부여한다. `Device.*`·`GameplayCue.*`·`Ability.*` 다수가 C++에서 참조되지 않는 것도 정상이다 — 각각 State Tree·큐 애셋·어빌리티 애셋이 저작으로 소비하며 Content 에셋에서 문자열을 확인했다. `Movement.*`·`Damage.CanCritical`에 doc-comment가 없는 것은 주변 태그와의 일관성 문제일 뿐이라 제외했고, `FWxCoreModule`의 빈 `StartupModule`/`ShutdownModule`은 모든 Wx 플러그인이 공유하는 하우스 패턴이라 제외했다.
- **미검토 / 한계**: 정적 리뷰이며 이 환경에는 엔진이 없어 빌드·PIE·패키징을 실행하지 않았고, 엔진 소스도 없어 `FUniversalObjectLocator::SyncFind()`의 내부 비용(동기 로드 유발 여부)을 이번에는 재확인하지 못했다 — `WxLocatorUtils.cpp`가 직전 리뷰 시점과 바이트 단위로 동일하므로 그때의 "에디터 UI 경로에 성능 위험 없음" 판정을 그대로 유지한다. Content 에셋 대조는 `.uasset` 안의 문자열 존재 여부만 본 것이라 문자열이 유효한 프로퍼티에 들어 있는지까지는 보장하지 못한다. `IWxMinion`이 doc-comment로 요구하는 "구현체는 `IGenericTeamAgentInterface`도 함께 구현" 조건은 코드로 강제되지 않으며, BP 구현체가 이를 지키는지는 에셋 검증 범위다.

---
*문서 기준 커밋 `231068b` · 리뷰일 2026-09-13 · 소스 13파일 — `/module-review`로 갱신*
