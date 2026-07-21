# WxQuest — 코드 리뷰

> 현재 모듈 부트스트랩 골격만 존재하는 빈 스텁 플러그인이다. 소스 2파일(`WxQuestModule.h/.cpp`)과 `WxQuest.Build.cs`를 전수 검토했으며, 퀘스트 도메인 로직·상태 머신·조건 평가 코드는 아직 존재하지 않는다. 실질적 결함은 발견되지 않았다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 `IModuleInterface` override에서 `Super::` 미호출
- **위치**: `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp:8`
- **범주**: 규칙 위반
- **문제**: `StartupModule()`/`ShutdownModule()`가 `IModuleInterface`의 가상 함수를 override하면서 `Super::StartupModule()` 등을 호출하지 않는다. CLAUDE.md 코딩 규칙 5("함수 override 시 `Super::`로 부모 함수를 호출한다")의 문자적 위반이다.
- **제안**: 규칙을 엄격히 적용하려면 `Super::` 호출을 추가한다. 다만 `IModuleInterface`의 두 함수는 본문이 빈 기본 구현이며 Unreal 관례상 모듈 진입점에서는 `Super::`를 호출하지 않는 것이 표준이므로, 규칙의 이 케이스 예외 여부를 팀에서 정리하는 편이 실용적이다.
- **확신도**: 낮음(엔진 관례상 의도된 형태일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxQuest/Source/WxQuest/WxQuest.Build.cs`, `Plugins/WxQuest/Source/WxQuest/Public/WxQuestModule.h`, `Plugins/WxQuest/Source/WxQuest/Private/WxQuestModule.cpp`
- **훑은 파일**: `Plugins/WxQuest/README.md`
- **미검토 / 한계**: 없음(모듈 전체가 소스 2파일 규모라 전수 검토). 단, 퀘스트 상태 머신·조건 평가·이벤트 구독 등 리뷰 핵심 대상 로직은 아직 미구현이라 검토할 코드 자체가 존재하지 않는다.

### 참고(발견 아님)
- 소스 첫 줄 `// Copyright Woogle. All Rights Reserved.` 규칙, `Wx`/`FWx` prefix 규칙, 모듈 경계 규칙(`Build.cs`의 Wx 의존이 `WxCore` 단일 — 다른 Wx 플러그인 참조 없음)을 모두 준수한다.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 2파일 — `/module-review`로 갱신*
