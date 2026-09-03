# WxCore — 코드 리뷰

> 로직이 거의 없는 계약·상수 전용 foundation 모듈이라 전반적으로 매우 건강하다. 실행 코드는 `WxLocatorUtils` 하나뿐이고 나머지는 태그 선언과 인터페이스 계약이다. 이번 리뷰는 11개 소스 전부를 읽고, 추가로 선언된 109개 네이티브 태그의 실제 소비처(C++·`*.uasset`·`*.umap`·`*.ini`)와 `ECC_WxAttack`·`DefaultEngine.ini` 짝을 교차 검증했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `Cooldown.Skill.2~4` 부재 — 스킬 슬롯이 쿨다운 태그를 공유하게 된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:213-223` (짝 규약 주석 + `Cooldown_Dodge`/`Cooldown_Skill_1`/`Cooldown_Ultimate`), 대비 `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:180-183` (`Ability_Skill_1~4`)
- **범주**: 설계/구조
- **문제**: 주석은 "이름은 위 Ability.X 식별 태그를 따른다 — 어빌리티가 지정한 `UWxEffect_Cooldown` 파생 GE가 짝이 되는 태그를 부여한다"고 1:1 규약을 선언하지만, 실제 선언된 쿨다운 태그는 슬롯 1뿐이다. 소비 측 `UWxAbility_Skill` 생성자는 `CooldownGameplayEffectClass = UWxEffect_Cooldown_Skill_1::StaticClass()`를 기본값으로 깔고(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:21`), `UWxEffect_Cooldown` 파생 클래스는 Dodge/Skill_1/Ultimate 3개가 전부다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp:54,59,64`). 프로젝트에 `DefaultGameplayTags.ini`가 없어 모든 태그가 네이티브 선언뿐이므로, BP가 슬롯별 쿨다운 GE를 저작하려 해도 고를 태그 자체가 없다. 결과적으로 `GA_Skill_3`·`GA_Skill_4`(`Ability.Skill.3`/`.4` 사용 확인)가 발동하면 순정 `CheckCooldown`이 보는 태그가 `Cooldown.Skill.1` 하나라 슬롯 1·3·4가 한 쿨다운을 공유한다. 다만 두 GA는 현재 어떤 어빌리티 세트에도 참조되지 않아(에셋 역참조 0건) **아직 발현되지 않은 잠재 결함**이다.
- **제안**: 슬롯 2~4를 실제로 붙일 시점에 `Cooldown.Skill.2~4`를 이 파일에 선언하고 WxCombat에 짝 GE 파생을 추가한다. 반대로 "스킬 슬롯은 쿨다운을 공유한다"가 의도라면 태그 이름을 `Cooldown.Skill`로 바꿔 규약 주석과 어긋나지 않게 한다.
- **확신도**: 중간

### 2. 🟡 미사용 태그 5개 — 선언·정의만 있고 코드·에셋·설정 어디서도 참조되지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:206-209`, `:228` / `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:112-115`, `:123`
- **범주**: 중복/복잡도
- **문제**: `Ability.Pattern.6`~`Ability.Pattern.9`, `SetByCaller.Magnitude`가 전 저장소에서 참조 0건이다(C++ `WxGameplayTags::` 참조, `*.uasset`/`*.umap`/`*.ini`/`*.json` 문자열 전수 스캔 모두 0). `Ability.Pattern.1~5`는 에셋에서 실제로 쓰이고 있어 6~9만 남아 있는 형태이고, `SetByCaller.Magnitude`는 같은 블록의 나머지 3개 SetByCaller 키가 모두 사용 중인 것과 대비된다 — 특히 이 키는 doc-comment도 없어 남은 잔재로 보인다. 태그는 부팅 시 전역 레지스트리에 올라가고 저작 UI의 선택 목록을 오염시키므로, 쓰지 않는 항목은 기획자에게 "있는데 안 먹는 태그"로 오인된다.
- **제안**: `SetByCaller.Magnitude`는 제거한다. `Ability.Pattern.6~9`가 향후 패턴 슬롯 예약이라면 그 취지를 블록 주석에 한 줄 남기고, 예약이 아니라면 함께 제거한다.
- **확신도**: 중간 (Pattern 6~9는 의도된 예약일 수 있음 / `SetByCaller.Magnitude` 미사용 자체는 높음)

### 3. 🟢 `GetDisplayName`의 미해석 폴백이 빈 문자열을 낼 수 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp:22-31`
- **범주**: 버그/정확성
- **문제**: `FActorLocatorFragment::Path`(`FSoftObjectPath`)에서 `GetSubPathString()`이 빈 문자열이면 `SubPath`를 그대로 반환해 표시 텍스트가 공란이 된다. 아래의 `INVTEXT("unresolved")` 폴백은 페이로드 자체가 없을 때만 도달하므로 이 경로를 받아주지 못한다. 실제로는 액터 로케이터의 경로가 `.../Map:PersistentLevel.Actor` 형태라 서브패스가 늘 존재하지만, 값이 깨졌거나 서브패스 없이 초기화된 로케이터에서는 디테일 패널·ST 노드 설명이 아무 글자도 없는 칸으로 보인다(`Source/WxEditor/WxActorLocatorCustomization.cpp:143`이 이 값을 그대로 표시).
- **제안**: `SubPath`가 비면 `INVTEXT("unresolved")`로 떨어지도록 한 줄 가드를 둔다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`
- **훑은 파일**: `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/README.md`
- **교차 검증(참고로만 열어본 모듈 밖 파일)**: `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Cooldown.cpp`, `Source/WxEditor/WxActorLocatorCustomization.cpp`
- **확인했고 문제 없던 항목**:
  - 모듈 경계: `WxCore.Build.cs`는 엔진 모듈(`Core`/`CoreUObject`/`Engine`/`GameplayTags`, 에디터 한정 `UniversalObjectLocator`)만 참조하며 다른 `Wx` 플러그인 의존 0건 — DAG 최하단 규칙 준수.
  - 코딩 규칙: 11개 파일 전부 첫 줄 `// Copyright Woogle. All Rights Reserved.`, `Wx` prefix 준수, 인라인 함수 정의·불필요한 람다·`BlueprintCallable` 오용 없음. 델리게이트 콜백이 없어 `Handle` prefix 대상도 없음.
  - 태그 단일 출처: `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`UE_DEFINE_GAMEPLAY_TAG`가 WxCore 밖에 0건이고 `DefaultGameplayTags.ini`도 없어, 선언처가 두 파일로 실제 일원화되어 있다. 에셋 문자열 스캔에서도 미선언 `State.*`/`Effect.*`/`Event.*`/`Device.*`/`GameplayCue.*`/`Damage.*` 태그는 발견되지 않았다.
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 유일한 커스텀 채널 등록(`Name="WxAttack"`)과 일치하고, 헤더 주석이 서술한 메시 Overlap·캡슐 Ignore 오버라이드도 `Source/WxGame/Character/WxCharacterBase.cpp:29,33`과 일치.
  - `GameplayCue.*` 9개 태그 모두 `Content/AbilitySystem/Cue`의 `GC_*` 에셋과 1:1 대응.
- **미검토 / 한계**: BP·GE·StateTree 에셋 내부 구조는 범위 밖이라 바이너리 문자열 스캔으로만 태그 소비 여부를 판정했다 — 압축·리다이렉터로 문자열이 남지 않는 참조가 있다면 "미사용" 판정(발견 2)에 위양성이 있을 수 있다. `FUniversalObjectLocator::SyncFind()`가 디테일 패널 페인트마다 호출되는 경로(`WxActorLocatorCustomization.cpp:143`)는 확인했으나, `Find` 플래그라 에셋 로드를 유발하지 않고 경로 해시 조회 수준이어서 발견으로 올리지 않았다.

---
*문서 기준 커밋 `c486a5c7` · 리뷰일 2026-09-03 · 소스 11파일 — `/module-review`로 갱신*
