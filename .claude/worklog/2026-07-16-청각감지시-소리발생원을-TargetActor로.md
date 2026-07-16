# 청각 감지 시 소리 발생원을 TargetActor로 지정

## 계획

### 목표
청각 자극을 "조사형"(위치만 기록)에서 시각/피해와 동일한 "완전 획득(aggro)" 센스로 승격한다. 소리를 감지하면 그 소리 발생원(Actor)을 곧바로 TargetActor로 확정하게 만든다. (최종적으로는 BB에서 `TargetLastKnownLocation` 키를 제거할 예정이며, 이 작업은 그 1단계인 코드 편입이다.)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp` | `HandleTargetPerceptionUpdated`의 청각 전용 early-return 분기 제거 → 공통 획득 경로로 편입. 미사용이 된 `AISense_Hearing.h` include 제거 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` | 클래스 doc 주석을 "시각/청각/피해 모두 TargetActor 확정"으로 갱신 | 수정 |

### 접근 방식
- **청각 분기 제거로 공통 경로 편입**: 청각 자극이 시각/피해와 동일하게 `WasSuccessfullySensed()` → `SetTargetActor(Actor)`(포커스·strafe 발행) → `UpdateRecognition()`(`State.InCombat` 발행)을 타게 한다. 자극 만료(손실) 시 기존 손실 분기(현재 타겟이면 LastKnown 갱신)를 그대로 공유한다.
- **범위 밖 유지**: 손실 분기의 `SetTargetLastKnownLocation`, `Clear*`, accessor는 그대로 둔다 — 키 실제 제거(BB/BT `.uasset` 편집, 에디터 필요)는 후속 2단계.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Private/WxAIPerceptionComponent.cpp` | 청각 전용 early-return 분기를 삭제하고 공통 획득 경로에 편입. 미사용이 된 `AISense_Hearing.h` include 제거 | 수정 |
| `Plugins/WxAI/Source/WxAI/Public/WxAIPerceptionComponent.h` | 클래스 doc 주석을 "세 센스 모두 TargetActor 확정"으로 갱신 | 수정 |

### 구현·결정과 그 이유
- **분기 제거 = 최소·직교 변경**: 청각용 코드를 새로 쓰는 대신 시각/피해가 쓰던 공통 경로를 그대로 재사용했다. 감지 성공이면 `SetTargetActor(Actor)`로 소리 발생원을 확정하고 `UpdateRecognition()`이 `State.InCombat`을 발행한다. 세 센스의 동작이 한 곳으로 수렴해 조건 분기가 줄었다.
- **손실 분기·LastKnown 쓰기 유지**: 자극 만료 시 현재 타겟이면 LastKnown을 갱신하는 기존 손실 처리는 그대로 뒀다. `TargetLastKnownLocation`은 코드에서 읽는 곳이 없고(소비처는 BT 에셋) 키 자체를 없애는 건 BB/BT `.uasset` 편집이 필요한 후속 단계라 이번 범위에서 분리했다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **2단계(에디터 필요)**: BB_Enemy.uasset에서 `TargetLastKnownLocation` 키 삭제 + 참조 BT 노드 정리 → 이후 코드에서 손실 분기의 LastKnown 쓰기/`Clear*`/`WxBlackboardKeys` accessor 제거.
- (선택) 에디터에서 시야 밖 소리 유발 시 적이 즉시 회전·추적하며 네임플레이트 전투 인식이 뜨는지 런타임 관찰.
