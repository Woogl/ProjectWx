# WxCore Minion 디렉토리 제거

## 계획

### 목표
`WxCore`의 1파일짜리 하위 디렉토리 `Minion`을 없애고, `IWxMinion`을 `IWxInteractable`·`IWxUIData`와 같이 `Public/` 바로 아래로 꺼낸다. 파일 이동과 include 경로 갱신뿐이며 코드 내용·동작은 그대로다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h` → `Public/WxMinion.h` | 이동 (내용 무변경) | 이동 |
| `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp` → `Private/WxMinion.cpp` | 이동 + 자기 include 를 `"WxMinion.h"` 로 | 이동·수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp` | include 를 `"WxMinion.h"` 로 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp` | `IWxMinion` include 를 `"WxMinion.h"` 로 (이 파일 자신의 위치는 유지) | 수정 |
| `Plugins/WxCore/README.md` | 핵심 타입 표의 `IWxMinion` 경로 갱신 | 수정 |

### 접근 방식
- **순수 이동**: `Minion/WxMinion.h`를 참조하는 곳은 저장소 전체에서 위 3개 `#include`뿐이다. 클래스·UINTERFACE 이름과 `/Script/WxCore.WxMinion` 경로는 파일 위치와 무관하므로 BP·에셋·`MustImplement` 메타는 영향을 받지 않는다. `WxMinion.generated.h`도 헤더 파일명 기준이라 그대로다.
- **빌드 설정 무영향**: UBT가 모듈 `Public/` 이하를 재귀로 include 경로에 넣는다. `Minion` 문자열을 언급하는 `Build.cs`·`.uplugin`·`Config` 파일도 없다.
- **`WxCombat`의 `Minion/`은 유지**: 요청 범위가 WxCore이고, WxCombat은 하위 디렉토리 분류가 유효한 큰 모듈이다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/Public/WxMinion.h` | `Public/Minion/`에서 이동, 내용 무변경 | 이동 |
| `Plugins/WxCore/Source/WxCore/Private/WxMinion.cpp` | `Private/Minion/`에서 이동, 자기 include 갱신 | 이동·수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp` | include 갱신 + Wx 헤더 묶음으로 위치 이동 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp` | include 갱신 + 알파벳 순서 유지 위해 위치 이동 | 수정 |
| `Plugins/WxCore/README.md` | 핵심 타입 표의 `IWxMinion` 경로 갱신 | 수정 |

### 구현·결정과 그 이유
- **`git mv` 사용**: 이력에서 이동으로 인식되게 했다. 두 파일 모두 내용 변경은 include 한 줄뿐이다.
- **include 순서 재정렬**: 경로가 `Minion/WxMinion.h`에서 `WxMinion.h`로 바뀌면서 기존 정렬이 어긋났다. WxCombat 쪽은 알파벳 순서 끝으로, WxAI 쪽은 파일의 그룹 규칙(Wx 헤더 먼저, 엔진 헤더 나중)에 맞춰 위로 올렸다.
- **`WxCombat`의 `Minion/` 유지**: 요청 범위 밖이고, 하위 디렉토리로 분류할 만한 크기의 모듈이다.

### 계획 대비 달라진 점
- include 순서 재정렬이 추가됐다. 경로 단축의 부작용이라 같은 작업으로 묶었다.

### 후속 과제
- 없음
