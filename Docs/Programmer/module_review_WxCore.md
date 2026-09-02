# WxCore — 코드 리뷰

> foundation 모듈로서 매우 건강하다. 실질 로직은 `WxLocatorUtils` 하나뿐이고 나머지는 선언·상수·순수 가상 계약이며, `Build.cs`·include 어디에도 Wx 의존이 없어 DAG 최하단 계약이 그대로 지켜진다. `CLAUDE.md` 코딩 규칙 위반은 한 건도 없다. 이번 리뷰는 10개 소스 전부를 정독하고 `WxCore.Build.cs`·`WxCore.uplugin`·`README.md`까지 본 뒤, 선언된 107개 태그를 저장소 전역 C++와 `.uasset`/`.umap` 4869개·`Config`에서 역참조 검증하고, `ECC_WxAttack`을 `Config/DefaultEngine.ini`의 채널·프로파일 등록과 대조했다. 지난 리뷰(`a8c6c495`) 이후 이 모듈의 코드 변경은 없고 주석·README만 바뀌었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 아무도 쓰지 않는 SetByCaller 키 `SetByCaller.Magnitude`
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:223`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:121`
- **범주**: 중복/복잡도
- **문제**: 이 태그는 저장소 어디에서도 참조되지 않는다 — `WxGameplayTags::SetByCaller_Magnitude` 심볼 참조 0건, `.uasset`/`.umap` 4869개 전수 문자열 검색 0건, `Config` 0건. 형제 태그 `SetByCaller_Duration`·`SetByCaller_Coeff_ATK`·`SetByCaller_MoveSpeedScale`은 셋 다 소비처를 명시한 주석을 달고 실제로 쓰이는데 이것만 주석도 소비처도 없다. 이름이 가장 범용적이라 "매그니튜드는 이 키를 쓰면 되겠다"는 오해를 사기 쉬운 자리인데, GE에서 이 키로 SetByCaller Magnitude를 저작해도 `AssignTagSetByCallerMagnitude`를 부르는 쪽이 없어 GE가 0으로 적용되고 런타임 로그로만 새는 실패가 된다.
- **제안**: 삭제하거나, 예약 목적이라면 형제 태그처럼 "어느 GE가 저작하고 누가 대입하는가"를 주석으로 못 박는다.
- **확신도**: 중간 (의도된 예약일 수 있음)

### 2. 🟡 `ECC_WxAttack`의 ini 정합성을 아무도 검증하지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:8,15`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp:6`
- **범주**: 성능/안전
- **문제**: 헤더 주석 스스로 "DefaultEngine.ini의 채널 등록 순서와 일치해야 한다"고 경고하지만, 그 일치를 확인하는 장치가 하나도 없다. 채널 이름과 인덱스의 결합은 `Config/DefaultEngine.ini:39`의 `+DefaultChannelResponses` 나열 순서뿐이라, 누군가 Project Settings > Collision 에서 오브젝트 채널을 지우고 새로 추가하면 `ECC_GameTraceChannel1`이 조용히 다른 채널을 가리키게 된다. 그 순간 무기 스윕(`Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:209`)과 캐릭터 메시/캡슐 응답 설정(`Source/WxGame/Character/WxCharacterBase.cpp:28,32`)이 전부 엉뚱한 채널로 돌아 전투 피격 판정이 통째로 죽는데, 컴파일도 부팅도 성공하고 로그도 남지 않는다. 이 상수는 WxCore가 전 도메인에 파는 단일 계약이라 실패 반경이 프로젝트 전체다. 마침 `FWxCoreModule::StartupModule()`은 비어 있다.
- **제안**: `StartupModule()`(또는 `FCoreDelegates::OnPostEngineInit`)에서 `UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(ECC_WxAttack)`이 `WxAttack`인지 `ensureMsgf`로 확인한다. 4줄이면 침묵하는 실패가 부팅 시점 단언으로 바뀐다.
- **확신도**: 중간

### 3. 🟡 태그 헤더가 도메인 전용 어휘까지 안고 있어 콘텐츠 태그 하나에 전 모듈이 재빌드된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:103-124` (Device), `:134-137` (AttackTelegraph), `:194-203` (Ability.Pattern), `:237-253` (UI)
- **범주**: 설계/구조
- **문제**: 선언된 107개 중 36개는 C++ 어디에서도 심볼로 불리지 않고 순전히 에셋(State Tree 상태값·GE·어빌리티 BP·Cue) 안에서만 쓰인다. 헤더 주석도 `Device.*`에 대해 "코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다"고 인정한다(`:103-104`). 그런데 `WxGameplayTags.h`는 64개 번역 단위가 include 하는 최하단 공용 헤더라, 기획자가 장치 상태 하나·패턴 하나를 늘릴 때마다 WxCore와 그 아래 전 도메인이 재컴파일된다. 네이티브 심볼로 얻는 이득(컴파일 타임 오타 검출·직접 참조)이 애초에 필요 없는 태그들이 비용만 물리는 구조다. 또한 이 목록이 도메인 전용 어휘(엘리베이터 층, CommonUI 액션, 적 패턴 번호)까지 담고 있어 "무엇이 도메인 간 계약인가"라는 구분이 헤더에서 사라진다.
- **제안**: 도메인 간 계약 태그(Event·Damage·HitReact·State·Effect 등 실제로 C++이 주고받는 것)만 네이티브로 남기고, 에셋 전용 어휘는 `Config/DefaultGameplayTags.ini`의 `+GameplayTagList`로 내리거나 소유 도메인 모듈의 `UE_DEFINE_GAMEPLAY_TAG_STATIC`으로 옮긴다. 현재 프로젝트에 `DefaultGameplayTags.ini` 자체가 없으므로 ini 경로는 신설이 필요하다.
- **확신도**: 낮음 (README가 "태그 추가는 이 두 파일에만"을 명시적 규약으로 세워둔 만큼 의도된 설계다 — 단일 원천의 이점과 재빌드 비용 중 무엇을 살지의 판단)

### 4. 🟢 미사용 예약 슬롯 태그 `Ability.Pattern.6` ~ `Ability.Pattern.9`
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:200-203`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:110-113`
- **범주**: 중복/복잡도
- **문제**: `Ability.Pattern.1` ~ `.5`는 에셋에서 실제로 쓰이는데 `.6`~`.9` 4개는 C++·에셋·Config 통틀어 참조가 0건이다. 태그 파일이 프로젝트 전역 어휘의 단일 원천이라 목록을 훑는 사람이 "패턴이 9개까지 있다"고 읽게 된다.
- **제안**: 적 패턴이 실제로 늘 때 함께 추가하는 편이 목록의 신뢰도를 지킨다. 저작 편의로 슬롯을 미리 깔아둔 것이라면 그대로 두고 예약이라는 주석 한 줄만 붙인다.
- **확신도**: 낮음 (의도된 예약 슬롯일 수 있음)

### 5. 🟢 `WxCharacterMesh` 콜리전 프로파일이 헤더가 선언한 채널 규약과 반대다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:12`, `Config/DefaultEngine.ini:41`
- **범주**: 버그/정확성
- **문제**: 헤더는 "캐릭터 메시는 WxAttack에 Overlap"이라고 규약을 못 박고 실제 코드도 그렇게 설정하는데(`Source/WxGame/Character/WxCharacterBase.cpp:28`), ini에 등록된 `WxCharacterMesh` 프리셋은 `(Channel="WxAttack",Response=ECR_Block)`이다. 이 프리셋은 C++ 참조 0건, `.uasset`/`.umap` 참조 0건으로 현재 아무도 쓰지 않지만, `bCanModify=False`로 등록돼 에디터 콜리전 프리셋 드롭다운에는 그대로 노출된다. 이름만 보고 캐릭터 메시에 적용하면 무기 스윕(`SweepMultiByChannel`)이 첫 대상에서 블로킹으로 끊겨 다중 타격이 잘리는데, 헤더 규약을 따랐다고 믿는 상태라 원인 추적이 어렵다.
- **제안**: 프리셋의 WxAttack 응답을 `ECR_Overlap`으로 고쳐 헤더 규약과 맞추거나, 쓰지 않는 프리셋이면 ini에서 지운다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`, `Config/DefaultEngine.ini`
- **확인해서 문제 없던 항목**:
  - 태그 선언/정의 정합성 — 헤더 선언 107개와 cpp 정의 107개가 이름까지 완전히 일치하고, 중복 태그 문자열도 없다. 저장소 전체에서 `UE_DEFINE_GAMEPLAY_TAG`은 이 파일에만 있어 "태그는 두 파일에만" 규약이 실제로 지켜진다.
  - 모듈 경계 — `WxCore.Build.cs`의 의존은 `Core`/`CoreUObject`/`Engine`/`GameplayTags`와 에디터 전용 `UniversalObjectLocator`뿐이고, 소스 include에도 Wx 도메인 헤더가 하나도 없다.
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`의 `Name="WxAttack"`과 현재는 일치한다(장래 어긋남에 대한 지적은 발견 2). `WxProjectile` 프리셋도 헤더 주석 서술과 일치한다.
  - `FWxLocatorUtils`의 `#if WITH_EDITOR` 가드와 `bBuildEditor` 조건부 의존이 대칭이고, 호출하는 6개 지점(`WxEditor`·`WxQuest`·`WxUI`·`WxWorld`) 전부가 `#if WITH_EDITOR` 안에 있다 — 쿠킹/Shipping 링크 실패 위험 없음. `SyncFind(UObject* Context = nullptr)`는 UE 5.8에서 find-only라 동기 로드 히치도 없고, `GetLastFragment()`/`TryGetPayloadAs` 널 체크도 둘 다 있다.
  - 코딩 규칙 — 10파일 모두 첫 줄 저작권 표기, `Wx` prefix 준수, 람다 0건, `BlueprintCallable` 0건, 인라인 함수 정의 0건(`ECC_WxAttack`은 `inline constexpr` 변수라 함수 규칙 대상 아님), 델리게이트 자체가 없어 `Handle` prefix 규칙은 해당 없음.
  - 지난 리뷰의 🟢 "README에 `IWxUIData` 누락"은 `60ec772e`에서 해소됐다.
- **미검토 / 한계**: 태그 사용 여부 판정 중 에셋 쪽 근거는 `.uasset`/`.umap` 바이너리 문자열 전수 검색이라, 태그가 다른 형태로 직렬화된 에셋이 있다면 위음성(실제로는 쓰이는데 미사용으로 보임)이 날 수 있다. 발견 1·4는 C++·Config 참조가 함께 0건이라는 점을 더 무겁게 봤다. 태그 자체의 게임플레이 의미(예: `Ability.*` 식별 태그가 실제 어빌리티 애셋에 정확히 하나씩 붙어 있는지)는 BP/에셋 내부 영역이라 범위 밖이다.

---
*문서 기준 커밋 `e9630dc2` · 리뷰일 2026-09-02 · 소스 10파일 — `/module-review`로 갱신*
