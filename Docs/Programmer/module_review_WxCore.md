# WxCore — 코드 리뷰

> 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지 10파일은 전부 태그 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 직전 리뷰(`1d91a915`)의 🟡 1건(`Event.Ragdoll` 네임스페이스 불일치)과 🟢 1건(`ECC_WxAttack` doc-comment)은 모두 반영돼 사라졌고, 남은 지적 2건은 현재 코드로 재검증해 그대로 유효하다. 이번엔 소스 11파일·`WxCore.Build.cs`·`WxCore.uplugin`·`README.md`를 전수 통독했고, 태그 111개의 선언↔정의 짝과 심볼→문자열 치환 일치, 태그 111개 각각의 프로젝트 전역 참조(C++ 심볼 · `Content`/`Plugins/*/Content` 에셋 문자열 · `Config`)를 전수 대조했으며, `ECC_WxAttack`↔`Config/DefaultEngine.ini:39` 바인딩과 UE 5.8 의 `FUniversalObjectLocator::SyncFind`·`FActorLocatorFragment::Resolve` 실제 구현까지 엔진 소스로 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟢 `State.Ragdoll` doc-comment 가 이제 존재하지 않는 소비처를 가리킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:24` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:11`)
- **범주**: 버그/정확성
- **문제**: 주석은 소비처를 세 개로 적고 있는데 마지막 하나가 사실이 아니다. 앞 둘은 맞다 — `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:100` 이 `AddLooseGameplayTag(..., EGameplayTagReplicationState::TagOnly)` 로 발행하고, `Source/WxGame/Character/WxCharacterBase.cpp:57,61` 이 구독·폴링한다. 그러나 "타게팅 프리셋이 IgnoreTags로 사용한다"는 부분은 커밋 `0cdb18ce`("타게팅 프리셋에서 옛 Ragdoll 태그를 걷어내고 태그 리다이렉트를 제거")로 무효가 됐다 — `Content/Character/Shared/Targeting` 의 프리셋 5개는 현재 `IgnoreTags` 에 `Ability.Death` 를 쓰고, `State.Ragdoll`/`Event.Ragdoll` 문자열은 `Content`·`Plugins/*/Content`·`Config` 어디에도 0건이다. WxCore 의 실질 산출물이 태그 doc-comment 라는 계약 문서이므로, 이 한 줄을 믿고 "래그돌 상태면 타게팅에서 빠진다"고 가정하는 코드·저작이 나올 수 있다. 지금 게임플레이 구멍은 없다 — `UWxAbility_Death` 는 사망 후 `EndAbility` 를 타지 않아(`HandleMontageCompleted` 가 빈 함수, `WxAbility_Death.cpp:66`) `Ability.Death` 가 액터 파괴까지 남고, 따라서 두 태그의 IgnoreTags 효과가 실질적으로 같다.
- **제안**: 주석에서 타게팅 프리셋 문장을 빼고, 필요하면 "타게팅 제외는 `Ability.Death` 가 담당한다"로 정정한다.
- **확신도**: 높음

### 2. 🟢 어디에서도 참조되지 않는 태그 8개가 세 번째 리뷰째 그대로다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:191-193`, `:215-219` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:93-95`, `:113-117`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`~`.4`, `Ability.Pattern.5`~`.9` 여덟 개는 C++ 심볼 참조 0건, `Content`·`Plugins/*/Content` 에셋 문자열 0건, `Config` 0건이다(태그 111개 전수 대조). 나머지 `Ability.Skill.1`·`Ability.Pattern.1`~`.4` 는 전부 에셋 참조가 잡히므로 "에셋에서만 쓰이는 태그라 안 잡힌 것"이 아니다. 직전 리뷰(`1d91a915`)와 그 전 리뷰가 같은 지적을 했고 코드는 그대로다 — 매 리뷰가 같은 전수 대조를 반복하는 비용이 계속 든다.
- **제안**: 예약 슬롯이 맞다면 `Ability.Skill`·`Ability.Pattern` 블록에 "슬롯 예약, 아직 구현 없음" 한 줄을 남겨 다음 리뷰·다음 세션이 다시 파헤치지 않게 한다. 예약 의도가 아니면 지운다(`Cooldown.*` 는 실재 슬롯만 선언돼 있다 — `WxGameplayTags.h:232` 의 `Cooldown.Skill.1` 뿐).
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `State.Minion.Active` 는 여전히 발행만 되고 읽히지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:22` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:10`)
- **범주**: 중복/복잡도
- **문제**: 쓰기 경로는 `Source/WxGame/Character/WxEnemyCharacter.cpp:39,58,226`(`MasterStateTag` 로 담아 주인 ASC 에 카운트 올리고 내림) 하나뿐이고, `HasMatchingGameplayTag`·`RequireTags`·`ActivationBlockedTags` 등 읽는 쪽은 C++ 도 에셋도 0건이다. 발행이 `EGameplayTagReplicationState::TagOnly` 라 소환물이 뜨고 죽을 때마다 아무도 읽지 않는 태그가 주인 ASC 에서 클라로 복제된다. 헤더 주석(`:21`)이 이미 "아직 이 태그를 읽지 않는다"고 인정하고 있다.
- **제안**: 읽는 쪽을 붙일 계획이 살아 있으면 주석에 "누가 언제 읽을 예정"까지 남긴다. 계획이 유동적이면 발행 자체를 걷어내고 필요해질 때 되살리는 편이 복제 트래픽과 상태 수를 줄인다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 주석이 미완성임을 스스로 밝히고 있다)

### 4. 🟢 README 네임스페이스 목록에서 `Movement.*` 가 빠졌다
- **위치**: `Plugins/WxCore/README.md:29-38`
- **범주**: 중복/복잡도
- **문제**: README 개편(`ffc6360d`) 때 "주요 네임스페이스" 목록에서 `Movement.*` 항목이 사라졌는데, 이 네임스페이스는 실재하고 활발히 쓰인다 — `Movement.InAir` 는 `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp:82,86` 이 발행하고 플레이어 어빌리티 에셋 4종이 참조하며, `Movement.Sprint` 는 `Plugins/WxCombat/.../WxAbility_Sprint.cpp:98,132`·`WxEffect_DrainSP.cpp:16`·`WxEffect_RegenSP.cpp:17` 이 SP 소모/회복 게이트로 쓴다. README 가 이 모듈의 진입 지도 역할을 하므로, 목록에 없는 네임스페이스는 "없는 축"으로 읽힌다. 헤더의 두 선언(`WxGameplayTags.h:51-52`)에도 doc-comment 가 없어 다른 경로로 보완되지도 않는다.
- **제안**: README 목록에 `Movement.*` 한 줄을 되살리고, 헤더의 두 선언에 한 줄짜리 doc-comment(발행 주체와 소비 방식)를 붙인다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
  - 대조를 위해 함께 읽은 모듈 밖 파일: `Wx.uproject`, `Config/DefaultEngine.ini`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxEditor/WxActorLocatorCustomization.cpp`, `Source/WxEditor/WxUIDataThumbnailRenderer.cpp`, `Plugins/WxCombat/.../WxAbility_Death.cpp`, `Plugins/WxCombat/.../WxAbilityBase.cpp`, `Content/Character/Shared/Targeting/*.uasset`(이름 테이블 문자열), UE 5.8 엔진의 `UniversalObjectLocator.h`·`UniversalObjectLocatorResolveParams.h`·`ActorLocatorFragment.h/.cpp`
  - **이번에 확인했고 문제 없었던 것**: 태그 111개의 헤더 선언↔cpp 정의 1:1(누락·잉여 0), 심볼명 `_`→`.` 치환값과 정의 문자열 전부 일치(오타 0), 프로젝트 내 다른 곳의 공용 태그 선언 0건(`WxDeviceStateTreeComponent.cpp:49,67` 의 `RequestGameplayTag` 는 저작된 이름 문자열 조회라 단일 출처 규약과 무관). 직전 리뷰의 🟡 1번은 `Event.Ragdoll`→`State.Ragdoll` 이관(`8e145265`)으로, 🟢 2번은 `WxCollisionChannels.h:8` 주석 정정으로 각각 해소됐고 `GameplayTagRedirects` 잔재도 없다(`Config` 0건). `ECC_WxAttack = ECC_GameTraceChannel1` 은 `Config/DefaultEngine.ini:39` 의 `Channel=ECC_GameTraceChannel1, Name="WxAttack"` 과 정합하고, 소비처(`WxWeaponBase.cpp:155,202`·`WxAbility_Dodge.cpp:254`·`WxCharacterBase.cpp:26,30,256,279`·`WxItemPickup.cpp:39`)도 주석이 서술한 응답 구성과 일치한다. `FWxLocatorUtils::GetDisplayName` 의 세 분기 전부 도달 가능하고 널 가드가 온전하다 — `SyncFind(nullptr)` 는 `FResolveParams::SyncFind` 라 `ELocatorResolveFlags::Load` 가 없어 동기 로드를 하지 않고, `FActorLocatorFragment::Resolve` 의 에디터 폴백(`Path.ResolveObject()`)까지 내려가 로드된 액터만 찾는다(디테일 패널·ST 노드 설명 재그리기에서 비용 문제 없음). `WITH_EDITOR` 가드와 `WxCore.Build.cs:19-25` 의 에디터 전용 `UniversalObjectLocator` 의존이 일치한다. **규칙 위반 0건** — `Wx` prefix 일관, 소스 12개(`*.h`/`*.cpp`/`*.cs`) 전부 `// Copyright Woogle. All Rights Reserved.` 첫 줄, 인라인 함수 정의 0(`ECC_WxAttack` 은 `inline constexpr` **상수**라 규칙 6과 무관), `UFUNCTION`·`BlueprintCallable` 0, 람다 0, 델리게이트 콜백 0. **모듈 경계도 깨끗하다** — `Build.cs` 의존은 `Core`/`CoreUObject`/`Engine`/`GameplayTags` + 에디터 전용 엔진 모듈뿐이고, `.uplugin` 에 `Plugins` 항목이 없으며 Content 폴더도 없다(다른 Wx 플러그인 참조 0).
  - **발견으로 올리지 않은 것**: `IWxUIData::GetMaxRecharges`(`WxUIData.h:34`)가 "UI 가 그대로 표시하는 데이터" 계약에 속하면서 `Plugins/WxCombat/.../WxAbilityBase.cpp:317` 의 어빌리티 발동 게이트(`GetAggregatedStackCount(CooldownQuery) < GetMaxRecharges()`) 입력으로도 쓰인다. 표시 계약이 게임플레이 판정으로 새는 모양이지만, 어빌리티가 남이 아니라 자기 테이블 행을 읽는 구조이고 WxUI→WxCombat 참조가 모듈 규칙상 불가해 현재 다른 통로가 없다. 직전 리뷰도 같은 판단이었고, 이 축(MaxRecharges 공급 통로)은 이미 별도 안건으로 보류 중이라 중복 제기하지 않는다.
- **미검토 / 한계**:
  - 태그·프리셋의 에셋 참조 여부는 `.uasset`/`.umap` 이름 테이블 문자열 검색으로 판정했다(ASCII 태그 문자열이 실제로 잡히는 것은 `Device.Piston.Off` 등 다수 사례로 검증). 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 1·2번이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 그래프 구조는 범위 밖이다.
  - `FWxLocatorUtils::GetDisplayName` 이 액터가 아닌 오브젝트로 해석되는 로케이터를 받으면(해석 성공인데도) `unresolved` 로 표시된다는 점은 확인했으나, 현 소비처 6곳이 전부 액터 대상 로케이터여서 발견으로 올리지 않았다.
  - 리뷰 대상은 커밋 `4a8e5d4b` 의 작업 트리이며, 라인 번호도 그 기준이다.

---
*문서 기준 커밋 `4a8e5d4b` · 리뷰일 2026-09-11 · 소스 11파일 — `/module-review`로 갱신*
