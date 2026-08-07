# FWxActorTarget 제거 — 단일 대상 지정을 UOL 배열로 통일

## 계획

### 목표
`FWxActorTarget`(WxCore)은 UOL 을 ST 인스턴스 데이터의 직속 멤버로 두면 값 위젯이 만들어지지 않는 UE 5.8 제한을 우회하려고 둔 한 겹 래퍼다. 배열 원소는 그 제한에 걸리지 않으므로, 남은 두 사용처를 배열로 옮기면 래퍼가 필요 없어진다. 레벨 액터 지정 방식을 프로젝트 전체에서 배열 UOL 하나로 통일한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/.../Public/WxDialogueStateTreeNodes.h` | EnableNpcInteraction 의 대상을 UOL 배열로, 적용 기록도 짝 배열로, include·주석 갱신 | 수정 |
| `Plugins/WxDialogue/.../Private/WxDialogueStateTreeNodes.cpp` | 토글 갱신을 인덱스 루프로 확장, 진입 경고 배열화, 목록 표시 텍스트 헬퍼 추가 | 수정 |
| `Plugins/WxQuest/.../Public/Quest/WxQuestStateTreeNodes.h` | WaitMoveToTarget 의 대상을 UOL 배열로, include·주석 갱신 | 수정 |
| `Plugins/WxQuest/.../Private/Quest/WxQuestStateTreeNodes.cpp` | 도달 판정을 배열 순회로, 진입 경고 배열화, 목록 표시 텍스트 헬퍼 추가 | 수정 |
| `Plugins/WxCore/.../Public/WxActorTarget.h` | 래퍼 삭제 | 삭제 |
| `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` | `UniversalObjectLocator` 의존 제거(이 헤더가 유일한 사용처였다) | 수정 |
| `Plugins/WxCore/README.md`, `Plugins/WxQuest/README.md`, `Plugins/WxDialogue/README.md` | 래퍼 항목·계약 서술 정리 | 수정 |
| `/Game/Quest/ST_Quest_Main1` | 타입 변경으로 유실되는 지정 3건 재지정 | 수정(에디터) |

### 접근 방식
- **파라미터 배열화**: `FWxActorTarget Target` → `TArray<FUniversalObjectLocator> Targets`(`AllowedLocators = "Actor"`). `MarkIndicators`(WxUI)·스포너 노드(WxWorld)와 동일한 선언이라 픽커 동작이 이미 검증돼 있다.
- **적용 기록 짝 배열**: EnableNpcInteraction 의 `AppliedNpc` 를 대상과 같은 인덱스로 짝지어, 대상마다 독립적으로 재적용 판단이 되게 한다(하나가 스트리밍 아웃돼도 나머지 토글은 유지). `MarkIndicators` 의 등록 기록과 같은 구조다.
- **WaitMoveToTarget 판정 규칙**: 지정 중 **하나라도** `AcceptRadius` 안이면 완료. 목적지 후보가 여럿인 도착 목표가 일반적이고, 같은 상태에 얹히는 `MarkIndicators` 가 이미 배열로 여러 곳을 가리킨다.
- **동작 규약 유지**: 매 틱 해석, 미해석(스트리밍 아웃)은 조용히 대기, 잘못된 조립만 진입 시 1회 경고. 루프로만 넓힌다.
- **표시 텍스트**: 라벨 3개까지 나열하고 초과분은 `+N`. WxDialogue 판에는 없던 `FActorLocatorFragment` 경로-끝-이름 폴백을 넣는다 — 배열이면 `unloaded` 만 여러 줄 나열돼 어느 대상인지 구분할 수 없다.
- **헬퍼 중복 허용**: 도메인 간 참조가 금지라 표시용 헬퍼는 각 모듈에 둔다(MarkIndicators 배열화 때와 같은 판단).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/.../Public/WxDialogueStateTreeNodes.h` | EnableNpcInteraction 의 대상을 UOL 배열로, 적용 기록을 짝 배열로, include·주석 갱신 | 수정 |
| `Plugins/WxDialogue/.../Private/WxDialogueStateTreeNodes.cpp` | 토글 갱신을 인덱스 루프로 확장, 진입 경고 배열화, 표시명 폴백·목록 텍스트 헬퍼 추가 | 수정 |
| `Plugins/WxQuest/.../Public/Quest/WxQuestStateTreeNodes.h` | WaitMoveToTarget 의 대상을 UOL 배열로, include·주석 갱신 | 수정 |
| `Plugins/WxQuest/.../Private/Quest/WxQuestStateTreeNodes.cpp` | 도달 판정을 배열 순회로, 진입 경고 배열화, 목록 텍스트 헬퍼 추가 | 수정 |
| `Plugins/WxUI/.../Public/Indicator/WxIndicatorStateTreeNodes.h` | 삭제된 래퍼 주석을 가리키던 참조를 자기완결 문장으로 교체 | 수정 |
| `Plugins/WxCore/.../Public/WxActorTarget.h` | 래퍼 삭제 | 삭제 |
| `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` | `UniversalObjectLocator` 의존 제거 | 수정 |
| `Plugins/WxCore/README.md`, `Plugins/WxQuest/README.md`, `Plugins/WxDialogue/README.md` | 래퍼 항목·계약·의존 서술 정리 | 수정 |

### 구현·결정과 그 이유
- **적용 기록을 지정과 같은 인덱스로 짝지음**: 대상마다 해석 결과가 갈리므로(하나만 스트리밍 아웃) 재적용 판단이 대상별로 독립적이어야 한다. 인덱스 짝이면 별도 매핑 없이 그 관계가 유지된다 — MarkIndicators 의 등록 기록과 같은 구조다.
- **WaitMoveToTarget 은 하나라도 닿으면 완료**: 전원 도달을 요구하면 순회 목표가 되어 지금 쓰는 도착 목표와 다른 물건이 된다. 같은 상태에 얹히는 MarkIndicators 가 후보들을 함께 가리키므로, 표시한 곳 중 아무 데나 닿으면 되는 쪽이 에셋 조립과 맞물린다.
- **폰 조회를 루프 밖으로**: 대상 수와 무관한 조회이고, 폰이 없으면 어느 대상도 판정할 수 없다. 대상 해석만 루프 안에 남겼다.
- **WxDialogue 에 표시명 폴백 도입**: 이 모듈만 미해석 대상을 `unloaded` 로 뭉뚱그렸는데, 배열이 되면 그 문자열만 여러 개 나열돼 어느 대상인지 읽을 수 없다. 다른 세 모듈이 쓰는 액터 프래그먼트 경로-끝-이름 폴백으로 맞췄다.
- **엔진 제한 지식을 세 모듈 주석에 분산**: 래퍼 헤더 주석이 "직속 UOL 멤버는 값 위젯이 안 만들어진다"는 사실의 유일한 정본이었다. 삭제하면 그 지식이 사라져 누군가 다시 직속 멤버를 선언하게 되므로, UOL 을 쓰는 세 모듈 주석에 한 줄씩 남겼다(원인 분석 전문은 2026-07-27 워크로그에 있다).
- **NPC 타입 검사는 진입 경고 유지**: 스포너 노드처럼 `Compile` 검증으로 올릴 수도 있으나 이번 변경의 범위 밖이고, 기존 계약(잘못된 조립만 1회 경고, 미해석은 침묵)을 그대로 배열로 넓히는 편이 확인할 거리가 적다.

### 계획 대비 달라진 점
- WxUI 인디케이터 헤더 주석 1줄을 추가로 고쳤다 — 삭제된 래퍼 헤더를 "주석 참조"로 가리키고 있었다.
- 배열을 쓰는 이유를 WxQuest·WxDialogue 모듈 주석에도 남겼다(계획에는 WxDialogue 주석 갱신만 있었다). 위의 정본 소실 때문이다.
- WxQuest README 의 WxCore 의존 서술이 래퍼뿐이었으므로 `Quest.Fail` 태그로 교체했다.
- 에셋 재지정은 미착수 — 실행 중인 에디터가 DebugGame(구 코드)이라 새 필드로 열 수 없다.

### 후속 과제
- `/Game/Quest/ST_Quest_Main1` 지정 3건 재기입: `Before Start` 의 Enable Npc Interaction, `Step1` 의 Disable Npc Interaction, `Step1` 의 Wait Move To Target. 타입이 바뀌어 기존 값은 로드 시 버려진다. 원래 지정값은 `PersistentLevel.BP_Npc_C_UAID_345A6050E8ACB4F102_1162324671`(NPC 둘 다)과 `PersistentLevel.WxSpawner_UAID_345A6050E8ACCDF102_1126150075`(도달 대상)이다.
- 같은 에셋 `Step1` 의 Mark Indicators 재저작(2026-08-07 배열화 작업의 미완 과제)도 함께 처리하면 된다.
- PIE 실측 미수행 — 잠금 적용·해제, 도달 완료, 대상 2개 이상 지정 시 개별 동작.
- `Docs/Programmer/module_review_WxCore.md`·`module_review_WxQuest.md` 가 래퍼를 전제로 쓰여 stale 하다(다음 `/module-review` 때 갱신).
