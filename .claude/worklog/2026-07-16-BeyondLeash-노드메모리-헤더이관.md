# BeyondLeash 노드 메모리 구조체를 헤더로 이관

## 계획

### 목표
`WxBTDecorator_BeyondLeash.cpp`가 노드 메모리 구조체 `FWxBeyondLeashMemory`를 익명 네임스페이스로 .cpp에 선언하던 것을, 엔진 BTDecorator 관례(헤더 전역 스코프 선언)와 자체 선례(`FWxBTRandomChoiceMemory`)에 맞춰 헤더로 옮긴다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` | `UCLASS` 직전에 `FWxBeyondLeashMemory` 선언(설명 주석 동반) | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp` | 상단 익명 네임스페이스 블록 삭제 | 수정 |

### 접근 방식
- **헤더 전역 선언**: 엔진 `FBTConeCheckDecoratorMemory` 배치와 동일하게 `UCLASS` 앞에 구조체를 둔다. 이름·필드(`bool bWasBeyond;`)는 현행 유지, 참조부는 타입명이 같아 수정 불필요.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_BeyondLeash.h` | `UCLASS` 직전에 `FWxBeyondLeashMemory` + 설명 주석 선언 | 수정 |
| `Plugins/WxAI/Source/WxAI/Private/WxBTDecorator_BeyondLeash.cpp` | 익명 네임스페이스 블록 제거 | 수정 |

### 구현·결정과 그 이유
- **헤더 선언으로 통일**: 엔진의 모든 BTDecorator가 노드 메모리를 헤더 전역에 선언하고 .cpp 익명 네임스페이스는 쓰지 않는다. 자체 선례(`FWxBTRandomChoiceMemory`)도 동일. BeyondLeash만 어긋나 있어 정렬했다.
- **리네임·typedef 생략**: 요청은 위치 이관이므로 스코프를 최소화. `FWxBeyondLeashMemory` 이름·`bool bWasBeyond;` 필드 그대로 두고, `TNodeInstanceMemory` typedef도 명시 캐스팅(`CastInstanceNodeMemory<...>`)을 쓰므로 추가하지 않았다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
