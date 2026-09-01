# WxCore — 코드 리뷰

> foundation 모듈로서 매우 건강하다. 코드량이 적고(실질 로직은 `WxLocatorUtils` 하나뿐), Build.cs에 Wx 의존이 하나도 없어 DAG 최상단 계약이 그대로 지켜지며, `CLAUDE.md` 코딩 규칙 위반은 한 건도 없다. 이번 리뷰는 10개 소스 전부를 정독하고 `WxCore.Build.cs`·`WxCore.uplugin`·`README.md`까지 본 뒤, 선언된 107개 태그를 저장소 전역 C++·바이너리 에셋·Config에서 역참조 검증하고 `ECC_WxAttack`을 `Config/DefaultEngine.ini` 등록 순서와 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 아무도 쓰지 않는 SetByCaller 키 `SetByCaller.Magnitude`
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:224`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:121`
- **범주**: 중복/복잡도
- **문제**: 이 태그는 저장소 어디에서도 참조되지 않는다 — `WxGameplayTags::SetByCaller_Magnitude` 심볼 참조 0건, `Content`/`Plugins` 아래 `.uasset`·`.umap` 문자열 참조 0건, `Config` 참조 0건. 형제 태그 `SetByCaller_Duration`·`SetByCaller_Coeff_ATK`·`SetByCaller_MoveSpeedScale`은 셋 다 소비처를 명시한 주석을 달고 실제 코드에서 쓰이는데, 이것만 주석도 소비처도 없다. 이름이 가장 범용적이라 "매그니튜드는 이 키를 쓰면 되겠다"는 오해를 사기 쉬운 자리인데, GE에서 이 키로 SetByCaller Magnitude를 저작해도 `AssignTagSetByCallerMagnitude`를 부르는 쪽이 없어 GE가 0으로 적용되고 런타임 로그로만 새는 실패가 된다.
- **제안**: 삭제하거나, 예약 목적이라면 형제 태그처럼 "어느 GE가 저작하고 누가 대입하는가"를 주석으로 못 박는다.
- **확신도**: 중간 (의도된 예약일 수 있음)

### 2. 🟢 미사용 예약 슬롯 태그 `Ability.Pattern.6` ~ `Ability.Pattern.9`
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:201-204`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:110-113`
- **범주**: 중복/복잡도
- **문제**: `Ability.Pattern.1` ~ `.5`는 에셋에서 실제로 쓰이는데(각 1~3건), `.6`~`.9` 4개는 C++·에셋·Config 통틀어 참조가 0건이다. 태그 파일이 프로젝트 전역 어휘의 단일 원천이라 목록을 훑는 사람이 "패턴이 9개까지 있다"고 읽게 된다.
- **제안**: 적 패턴이 실제로 늘 때 함께 추가하는 편이 목록의 신뢰도를 지킨다. 다만 적 캐릭터 저작 편의를 위해 슬롯을 미리 깔아둔 것이라면 그대로 두고 예약이라는 주석 한 줄만 붙인다.
- **확신도**: 낮음 (의도된 예약 슬롯일 수 있음)

### 3. 🟢 README 진입점 표에 `IWxUIData`가 빠져 있다
- **위치**: `Plugins/WxCore/README.md:19-22`, `Plugins/WxCore/README.md:52`
- **범주**: 설계/구조
- **문제**: `IWxUIData`(`Public/WxUIData.h`)는 `IWxInteractable`과 나란한 도메인 간 계약 인터페이스이고 소비처도 넓다 — `WxCombat`의 `UWxAbilityBase`·`UWxEffectComponent_Table`이 구현하고, `WxUI`의 3개 뷰모델과 `Source/WxEditor/WxUIDataThumbnailRenderer.cpp`가 소비한다. 그런데 README의 「핵심 타입 (진입점)」 표에도, 「여기서부터 읽어라」에도 없다. provenance도 `소스 9파일`로 남아 있어 현재 10파일과 어긋난다. README가 모듈의 선언된 오리엔테이션 맵인데 공용 계약 둘 중 하나가 지도에 없는 셈이다.
- **제안**: `/readme-writer`로 갱신해 표에 `IWxUIData` 행을 넣는다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **확인해서 문제 없던 항목**:
  - 태그 선언/정의 정합성 — 헤더 선언 107개와 cpp 정의 107개가 완전히 일치하고, 중복 태그 문자열도 없다.
  - 모듈 경계 — `WxCore.Build.cs`의 의존은 `Core`/`CoreUObject`/`Engine`/`GameplayTags`와 에디터 전용 `UniversalObjectLocator`뿐이고, 소스 include에도 Wx 도메인 헤더가 하나도 없다. foundation 계약이 지켜진다.
  - `ECC_WxAttack = ECC_GameTraceChannel1` — `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1 ... Name="WxAttack"`과 일치한다. 헤더 주석의 "메시 Overlap / 캡슐 Ignore" 서술도 `Source/WxGame/Character/WxCharacterBase.cpp:28,32`와 일치한다.
  - `FWxLocatorUtils`의 `#if WITH_EDITOR` 가드와 `bBuildEditor` 조건부 의존이 대칭이고, 호출하는 5개 모듈(`WxEditor`·`WxQuest`·`WxUI`·`WxWorld`·`WxDialogue`)이 모두 자기 Build.cs에 `UniversalObjectLocator`를 직접 선언한다 — private 의존 전이 누락 문제 없음.
  - `SyncFind()`는 UE 5.8에서 find-only(`SyncLoad`와 별개)라 표시명 헬퍼가 동기 로드 히치를 내지 않는다.
  - 코딩 규칙 — 10파일 모두 첫 줄 저작권 표기, `Wx` prefix 준수, 람다 0건, `BlueprintCallable` 0건, 헤더 인라인 함수 정의 0건, 델리게이트 자체가 없어 `Handle` prefix 규칙은 해당 없음. (`WxGameplayTags.h`/`.cpp`에 UTF-8 BOM이 있으나 프로젝트 452개 소스 중 53개가 같은 상태인 기존 패턴이라 이 모듈의 문제로 보지 않았다.)
- **미검토 / 한계**: 태그 사용 여부 판정 중 에셋 쪽 근거는 `.uasset`/`.umap` 바이너리 문자열 검색이라, 태그가 다른 형태로 직렬화된 에셋이 있다면 위음성(실제로는 쓰이는데 미사용으로 보임)이 날 수 있다. 발견 1·2는 C++·Config 참조가 함께 0건이라는 점을 더 무겁게 봤다. 태그 자체의 게임플레이 의미(예: `Ability.*` 식별 태그가 실제 어빌리티 애셋에 정확히 하나씩 붙어 있는지)는 BP/에셋 내부 영역이라 범위 밖이다.

---
*문서 기준 커밋 `a8c6c495` · 리뷰일 2026-09-01 · 소스 10파일 — `/module-review`로 갱신*
