# ActivatePattern BTTask 제거

## 계획

### 목표
참조 없는 고립 클래스 `UWxBTTask_ActivatePattern`(WxAI)을 제거해 AI 모듈을 정리한다. "접근 후 발동" 조합은 실제로 상위 데코레이터(거리 판정)+`WxBTTask_ActivateAbility`(발동) 구성이 대체하고 있어 이 노드를 쓰는 Behavior Tree가 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivatePattern.h` | 파일 삭제 | 삭제 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivatePattern.cpp` | 파일 삭제 | 삭제 |

### 접근 방식
- **참조 조사 선행**: 코드 참조(`#include`/심볼) 및 `Content/` 전 `.uasset` 문자열 참조 모두 없음 확인. 대조군(`ActivateAbility`/`TargetDistance`)은 정상 검출되어 검색 유효성 검증.
- **생성물 정리**: Intermediate 하위 UHT/UBT 생성물은 재빌드로 자동 정리(수동 삭제 안 함).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTTask_ActivatePattern.h` | `git rm` 삭제 | 삭제 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTTask_ActivatePattern.cpp` | `git rm` 삭제 | 삭제 |

### 구현·결정과 그 이유
- **삭제 전 참조 조사 우선**: 코드·데이터 어디서도 참조되지 않는 고립 클래스임을 확인한 뒤 제거해, 깨질 참조가 없음을 담보했다.
- **생성물 수동 삭제 안 함**: UBT가 소스 제거를 감지("Invalidating makefile — source file removed")해 WxAI 모듈을 재컴파일·재링크하므로 Intermediate 생성물은 자동 정리에 맡겼다.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 `Result: Succeeded` (WxAI 모듈 재컴파일·`UnrealEditor-WxAI.dll` 재링크 정상).

### 후속 과제
- 없음.
