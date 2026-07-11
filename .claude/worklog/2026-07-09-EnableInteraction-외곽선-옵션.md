# EnableInteraction 태스크에 외곽선 강조 옵션 추가

## 계획

### 목표
엘리베이터 플랫폼 바닥 메시는 상호작용 대상이지만 바닥 전체 외곽선 강조가 시각적으로 과하다. 상호작용 가용 여부를 상태별로 선언하는 StateTree 태스크(EnableInteraction)에 강조 허용 옵션을 추가해, ST 에셋에서 "상호작용은 켜되 강조는 끈다"를 저작할 수 있게 한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` | 강조 게이트(bUseHighlight) public 런타임 세터 SetUseHighlight 선언 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | 세터 구현 — 게이트를 닫기 전에 켜진 외곽선을 먼저 끔 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | EnableInteraction InstanceData 에 bUseHighlight(기본 true) 추가, 노드 모음 주석 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | EnterState 에서 게이트 적용, GetDescription 에 강조 끔 표기 | 수정 |

### 접근 방식
- **컴포넌트 게이트 + 태스크 옵션**: 컴포넌트에 이미 있는 강조 게이트에 런타임 세터만 열고, 상태별 선언 주체인 EnableInteraction 태스크가 진입 시 활성 토글과 함께 게이트를 적용한다. 기본값 true 라 기존 ST 에셋 동작은 불변.
- **끄기 순서**: 게이트가 닫힌 뒤에는 기존 외곽선 토글 함수가 early-return 이라, 세터는 게이트를 닫기 전에 켜져 있을 수 있는 외곽선을 먼저 끈다.
- **복제 불필요**: 강조는 클라 로컬 비주얼이고 기믹 ST 는 복제 State 로 양측에서 동일 구동되므로, EnterState 가 매번 게이트를 적용하는 것으로 복원/레이트조인까지 일관된다.
- **콘텐츠 저작(코드 범위 밖)**: ST_Elevator 의 PlatformInteraction 대상 노드에서 옵션을 끄는 것은 사용자가 에디터에서 수행. AWxElevator C++ 는 변경 없음(SetHighlightTarget 유지 — 에셋에서 언제든 되살릴 수 있게).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h` | SetUseHighlight 선언 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionComponent.cpp` | SetUseHighlight 구현(헤더 선언 순서에 맞춰 SetHighlightEnabled 뒤에 배치) | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | EnableInteraction InstanceData 에 bUseHighlight(기본 true) 추가, 노드 모음·태스크 주석 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | EnterState 에서 게이트 적용, GetDescription 은 강조 끔일 때만 ", no highlight" 표기 | 수정 |

### 구현·결정과 그 이유
- **명명은 Enable 대신 Use**: 처음엔 bEnableHighlight/SetEnableHighlight 로 구현했으나, 선택 대상 외곽선을 켜고 끄는 기존 SetHighlightEnabled 와 단어 순서만 다른 꼴이라 게이트와 표시 토글이 이름으로 구분되지 않았다. 게이트는 "이 컴포넌트가 강조 기능을 사용한다"는 설정·정책이므로 bUseHighlight/SetUseHighlight 로 리네임했다(리뷰 후 결정). BP 에 기본값에서 벗어나게 저작된 값이 없어 리다이렉트는 불필요.
- **끄기 순서**: 게이트가 닫힌 뒤에는 표시 토글 함수가 early-return 이라 끌 수 없으므로, 세터가 게이트를 닫기 전에 켜져 있을 수 있는 외곽선을 먼저 끈다.
- **기본값 true**: 기존 ST 에셋의 동작을 바꾸지 않는다. 강조를 끄고 싶은 노드만 명시적으로 끈다.
- **에디터 요약은 예외만 표기**: 대부분의 노드가 기본값이므로 강조 끔일 때만 덧붙여 요약을 짧게 유지한다.

### 계획 대비 달라진 점
- 식별자 명명을 bEnableHighlight → bUseHighlight 로 변경(위 명명 결정 참고). 그 외 계획대로.

### 후속 과제
- (사용자, 에디터) ST_Elevator 의 PlatformInteraction 대상 Wx Enable Interaction 노드에서 bUseHighlight 를 끄고 PIE 확인 — 플랫폼 바닥은 HUD 리스트에만 뜨고 외곽선 없음, 콜 콘솔은 기존대로 강조.
