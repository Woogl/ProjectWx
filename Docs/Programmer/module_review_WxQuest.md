# WxQuest — 코드 리뷰

> 아직 퀘스트 도메인 로직이 전혀 없는 부트스트랩 골격 플러그인이며, 있는 코드 자체는 깨끗하다. 소스 2파일(`WxQuestModule.h/.cpp`)과 `WxQuest.Build.cs`·`WxQuest.uplugin`을 전수 검토했고, 모듈이 프로젝트에 어떻게 물려 있는지 확인하기 위해 `Wx.uproject`와 타 모듈의 `*.Build.cs` 의존 관계까지 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟢 `Wx.uproject`의 플러그인 목록에 `WxQuest`가 빠져 있다
- **위치**: `Wx.uproject:18`(Plugins 배열 전체 18~98행), `Plugins/WxQuest/WxQuest.uplugin:1`
- **범주**: 설계/구조
- **문제**: `WxCore`/`WxCombat`/`WxUI`/`WxWorld`/`WxInventory`/`WxAI`/`WxSave`/`WxSound`/`WxDialogue` 등 다른 Wx 도메인 플러그인은 모두 `Wx.uproject`의 `Plugins` 배열에 명시되어 있으나 `WxQuest`만 없다. 프로젝트 `Plugins/` 하위 플러그인은 `.uplugin`에 `EnabledByDefault`가 없으면 기본 활성으로 취급되므로 현재 로드 자체는 되는 것으로 보이지만(빌드 산출물 `Plugins/WxQuest/Binaries/Win64/UnrealEditor-WxQuest.dll` 존재), 활성화가 명시가 아닌 암묵적 기본값에 의존한다. 나중에 플러그인 활성 정책이 바뀌거나 배포 구성에서 플러그인 목록을 기준으로 패키징 대상을 추리면 이 모듈만 조용히 빠질 수 있다.
- **제안**: 다른 도메인 플러그인과 동일하게 `Wx.uproject`의 `Plugins`에 `{"Name": "WxQuest", "Enabled": true}`를 추가하거나, 구현 전까지 의도적으로 비활성 상태로 두는 것이라면 `"Enabled": false`로 명시해 의도를 드러낸다.
- **확신도**: 중간

### 2. 🟢 아무 코드도 사용하지 않는 빌드 의존 4종
- **위치**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:11`(Public 목록 11~19행), `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs:21`(Private 목록 21~24행)
- **범주**: 중복/복잡도
- **문제**: `GameplayTags`, `StateTreeModule`, `WxCore`(Public)와 `DeveloperSettings`(Private)를 선언했지만 모듈의 유일한 소스 2파일은 `CoreMinimal.h`/`Modules/ModuleInterface.h`/`Modules/ModuleManager.h`만 포함하며 이들 모듈의 타입을 하나도 참조하지 않는다. 특히 Public 의존은 향후 `WxQuest`를 참조할 모듈에까지 전파되므로, 실제로 필요해지기 전까지는 불필요한 링크·헤더 전파와 빌드 시간 비용이다.
- **제안**: 실제 사용 시점에 추가하는 방향으로 정리하고, 사용이 확정된 것도 Public이 정말 필요한지(공개 헤더에 노출되는지) 판단해 가능하면 Private으로 내린다. 다만 스캐폴딩 의도로 미리 선언해 둔 것이라면 그대로 두어도 기능상 문제는 없다.
- **확신도**: 낮음(향후 구현을 위한 의도적 선반영일 수 있음)

### 3. 🟢 `IModuleInterface` override에서 `Super::` 미호출
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp:8`
- **범주**: 규칙 위반
- **문제**: `StartupModule()`/`ShutdownModule()`이 `IModuleInterface`의 가상 함수를 override하면서 `Super::`를 호출하지 않는다. CLAUDE.md 코딩 규칙 5("함수 override 시 `Super::`로 부모 클래스의 함수를 호출한다")의 문자적 위반이다.
- **제안**: 규칙을 문자 그대로 적용하려면 `Super::` 호출을 추가한다. 다만 `IModuleInterface`의 두 함수는 빈 기본 구현이고 프로젝트의 다른 모든 모듈 진입점(`FWxCombatModule`, `FWxAIModule`, `FWxDialogueModule` 등)이 동일한 형태이므로, 개별 수정보다는 모듈 진입점을 규칙 5의 예외로 명문화하는 편이 실용적이다.
- **확신도**: 낮음(엔진 관례이자 프로젝트 전반의 일관된 형태라 의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`, `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/WxQuest.uplugin`
- **훑은 파일**: `Plugins/WxQuest/README.md`, `Wx.uproject`
- **미검토 / 한계**: 모듈 전체가 소스 2파일 규모라 코드는 전수 검토했다. 다만 퀘스트 진행/목표 추적/조건 평가/이벤트 구독 등 리뷰의 실질적 대상이 될 로직이 아직 존재하지 않아, 버그·리플리케이션 권위·성능 차원은 검토할 대상 자체가 없었다. `"CanContainContent": true`이지만 `Content/` 폴더가 아직 없어 에셋 측 검토도 대상 없음.
- **참고**: 첫 줄 저작권 표기, `FWx` prefix, Public/Private 구조, "WxCore 외 Wx 플러그인 미참조" 모듈 경계 규칙은 모두 준수한다. 또한 어떤 모듈의 `*.Build.cs`도 아직 `WxQuest`를 의존하지 않아, 현재는 컴파일만 되고 소비되지 않는 모듈이다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 2파일 — `/module-review`로 갱신*
