# StateTree 태스크 완료 판정 정책

## 계획

### 목표

즉발 태스크가 상태 완료 판정에 끼어들어 대기 태스크보다 먼저 상태를 끝내는 사고를 코드 쪽에서 닫는다. 완료 판정은 자기 시간을 갖는 태스크(Wait·Move·Play 계열)에만 맡기고, 나머지 즉발 태스크는 판정에서 빼고 에디터에서 되돌릴 수 없게 잠근다. 지금까지 상태마다 `TasksCompletion=All` 을 손으로 지정해 막아 왔는데 저작자가 매번 기억해야 하는 규칙이라 잘 빠진다.

### 수정 범위

`FStateTreeTaskCommonBase` 파생 26종 중 지연 완료형 12종(Wait 3 · Move 3 · Play 4 · PrintSubtitle · SaveGame)은 현행 유지, 즉발 14종만 생성자에 두 줄을 추가한다.

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxInventory/.../WxStateTreeTask_GiveRewards.cpp` | 생성자에 완료 판정 제외 두 줄 추가 | 수정 |
| `WxInventory/.../WxStateTreeTask_RefillItemCharges.cpp` | 〃 | 수정 |
| `WxQuest/.../WxStateTreeTask_SetQuestObjective.cpp` | 〃 | 수정 |
| `WxQuest/.../WxStateTreeTask_SetQuestTitle.cpp` | 〃 | 수정 |
| `WxQuest/.../WxStateTreeTask_StartNextQuest.cpp` | 〃 | 수정 |
| `WxUI/.../WxStateTreeTask_MarkIndicator.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_EnablePlayerInput.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_PlaySound.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_RespawnSpawners.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_SpawnNiagara.cpp` | 〃 | 수정 |
| `WxWorld/Gimmick/WxStateTreeTask_TriggerSpawners.cpp` | 〃 | 수정 |
| `WxWorld/Interaction/WxStateTreeTask_EnableInteraction.cpp` | 〃 | 수정 |
| `WxWorld/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp` | 〃 | 수정 |

### 접근 방식

- **분류 기준은 이름이 아니라 완료 시점**: `EnterState` 가 `Running` 을 돌려주고 나중에 완료하는 태스크만 상태의 수명을 결정한다. `PlaySound` 는 이름만 Play 일 뿐 즉발이라 제외하고, `PrintSubtitle`·`SaveGame` 은 이름이 계열에 없어도 지연 완료형이라 남긴다.
- **생성자에서 선언**: 두 필드 모두 에디터 전용 데이터라 `WITH_EDITORONLY_DATA` 가드 안에 넣는다. 엔진의 `FStateTreeDebugTextTask` 가 같은 형태다.
- **에디터 잠금 동반**: `bCanEditConsideredForCompletion` 까지 꺼 저작 중 실수로 되살리지 못하게 한다.
- **주석 없음**: 14곳에 같은 문장을 반복하는 대신 정책은 이 기록에 남긴다.

---

## 완료

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| 즉발 태스크 14종의 `.cpp` (WxInventory 2 · WxQuest 3 · WxUI 1 · WxWorld 8) | 생성자에 완료 판정 제외 두 줄 추가 | 수정 |
| `WxUI/.../WxStateTreeTask_MarkIndicator.cpp` | 위에 더해 매 틱 바인딩 복사를 끔 | 수정 |
| `WxQuest/.../WxStateTreeTask_SetQuestTitle.h` | 판정 참여를 전제하던 클래스 주석을 새 정책으로 교체 | 수정 |
| `WxQuest/.../WxStateTreeTask_SetQuestObjective.h` | 〃 (Failed 가 상태를 실패시킨다는 서술 정정) | 수정 |
| `WxQuest/.../WxStateTreeTask_StartNextQuest.h` | 〃 | 수정 |
| `WxQuest/README.md` | 완료 판정 규약을 새 정책으로 갱신 | 수정 |

### 구현·결정과 그 이유

- **Move·Play 계열은 판정에 남긴다**: 이들의 완료가 곧 상태의 끝인 자리가 실제로 있다(문·엘리베이터의 이동, 보물상자의 애님, 퀘스트의 대화). 판정에서 빼면 그 상태를 끝낼 신호가 사라져 기믹이 갇힌다.
- **이름이 아니라 완료 시점으로 갈랐다**: `PlaySound` 는 이름만 Play 일 뿐 진입 즉시 끝나는 즉발이라 뺐고, `PrintSubtitle`·`SaveGame` 은 이름이 계열에 없어도 지연 완료형이라 남겼다.
- **성능은 실측 구조를 확인한 뒤 한 곳만 손봤다**: 엔진은 완료한 태스크를 더 틱하지 않고(`bIsTaskRunning` 게이트), 틱하지 않는 태스크는 바인딩 복사도 건너뛴다. 그래서 14종 중 12종은 이미 프레임 비용이 0이고, 대상 해석을 재시도하는 `EnableInteraction` 은 해석되는 순간 `Succeeded` 로 틱이 멎는다. 매 프레임 계속 도는 것은 완료가 없는 `MarkIndicator` 하나뿐이라 거기서만 틱 바인딩 복사를 껐다.
- **플래그를 더 얹지 않았다**: `bShouldCopyBoundPropertiesOnExitState` 나 `bConsideredForScheduling` 은 이득이 상태 이탈 1회분이거나(전자) 인디케이터 갱신을 놓치게 만들어(후자) 순정 동작을 흔든 값에 못 미친다.
- **낡은 주석을 정정했다**: `SetQuestTitle` 헤더와 WxQuest README 는 "판정에서 빼면 형제 완료를 물려받으니 반드시 하나는 참여시켜라"는 정반대 규약을 문서로 못 박고 있어, 그대로 두면 다음 사람이 이 변경을 되돌린다.

### 계획 대비 달라진 점

- 계획에 없던 주석·README 정정을 함께 했다. 코드와 정면으로 어긋나는 서술이라 남겨 둘 수 없었다.

### 후속 과제

- **에셋 재컴파일·재저장 미실시**: 이 값은 에디터 전용 데이터로 컴파일 시점에 마스크로 구워지므로, `Content/WorldObject/Gimmick/ST_*` 과 `Content/Quest/ST_*` 를 열어 컴파일·재저장해야 반영된다.
- **PIE 실동작 미검증**: 판정 태스크가 하나도 남지 않은 상태가 생겼는지는 로그가 아니라 실행에서 드러난다. 문 개폐, 엘리베이터 왕복, 보물상자 개봉·보상, ST_Quest_Main1 대화 진행을 확인해야 한다.
- **`TasksCompletion=All` 잔재**: 판정 참여 태스크가 줄어든 만큼, 기존 상태에 걸어 둔 `All` 지정이 이제 불필요하거나 오히려 상태를 잡아 둘 수 있다. 위 PIE 확인 때 함께 본다.
