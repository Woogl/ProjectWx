# AWxGameMode — 부활 경로를 엔진에 위임

## 계획

### 목표

부활 스폰이 오버라이드 두 개에 흩어져 있다. `SpawnDefaultPawnAtTransform_Implementation` 이 스폰 위치를 저장 트랜스폼으로 바꿔치기하고, 그러면 카메라가 엉뚱한 곳을 보므로 `FinishRestartPlayer` 가 컨트롤 로테이션을 손으로 보정한다. 부활 지점 하나 때문에 `TryGetSavedRespawnTransform` 을 3번 호출한다(`TActorIterator` 도 3번).

엔진에 이미 그 경로가 있다. `RestartPlayerAtTransform` 은 받은 트랜스폼으로 `SpawnDefaultPawnAtTransform` 을 부르고 같은 트랜스폼의 회전을 `FinishRestartPlayer` 의 `StartRotation` 으로 넘기며, 엔진 `FinishRestartPlayer` 는 이미 `SetControlRotation(StartRotation)` 을 한다. 우리가 손으로 하던 카메라 보정이 정확히 그것이다. 이 API 로 라우팅해 흩어진 로직을 한 함수로 모은다.

### 수정 범위

`Source/WxGame/Framework/WxGameMode.h/.cpp` 만. WxSave 는 변경 없다.

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `RestartPlayer(AController*)` override 추가(저장 지점 있으면 Yaw 만 남긴 트랜스폼으로 `RestartPlayerAtTransform`, 없으면 `Super`). `SpawnDefaultPawnAtTransform_Implementation` 오버라이드 제거. `FinishRestartPlayer` 의 카메라 블록 제거(스탯 복원만 남김). `TryGetSavedRespawnTransform` 주석의 "스폰 위치와 카메라 시선이 공유한다" 문구 정리 | 수정 |

`TryGetSavedRespawnTransform` 본체(PIE 체크 + WxSave 위임)는 그대로 두고 호출만 3회→1회.

`InitGame` 의 컴포넌트 추론과 GM 에셋의 죽은 `WxPlayerSpawningComponent` 참조는 범위 밖 — 각각 유지·사용자가 직접 정리.

### 접근 방식

- **`RestartPlayer` 하나에서 경로를 가른다**: 저장 지점이 있으면 `RestartPlayerAtTransform`, 없으면 `Super`(= `FindPlayerStart` → `RestartPlayerAtPlayerStart` 기본 경로). 스폰과 카메라를 엔진이 처리하므로 GameMode 에는 "어느 트랜스폼으로 재시작할까" 결정만 남는다.

- **Yaw 만 남겨 넘긴다**: 엔진 `SpawnDefaultPawnFor_Implementation` 은 폰을 pitch/roll 없이 스폰하는 정책이다. 저장 경로에도 같은 정책을 적용해 `FTransform(YawOnlyRotation, SavedTransform.GetLocation())` 을 넘긴다. 카메라 결과는 현재와 동일하고, 체크포인트가 기울어지거나 스케일을 갖고 있어도 폰이 따라 기울지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Framework/WxGameMode.h/.cpp` | `RestartPlayer` override 추가(저장 지점 → Yaw 만 남긴 트랜스폼으로 `RestartPlayerAtTransform`, 없으면 `Super`). `SpawnDefaultPawnAtTransform_Implementation` 제거. `FinishRestartPlayer` 는 스탯 복원만 남김. 헤더 주석 갱신 | 수정 |

### 구현·결정과 그 이유

- **엔진이 이미 하던 일을 되찾았다**: 엔진 `FinishRestartPlayer` 는 `SetControlRotation(StartRotation)` 을 이미 한다. 우리는 `SpawnDefaultPawnAtTransform` 에서 위치만 바꿔치기하느라 `StartRotation` 이 PlayerStart 회전으로 남았고, 그래서 카메라를 손으로 덮어썼다. `RestartPlayerAtTransform` 은 넘긴 트랜스폼의 회전을 그대로 `StartRotation` 으로 전달하므로 보정이 통째로 불필요해졌다.

- **Yaw 만 남겨 넘김**: 엔진 `SpawnDefaultPawnFor_Implementation` 은 폰을 pitch/roll 없이 스폰한다. 기존 코드는 `SavedTransform` 을 통째로 넘겨 체크포인트의 pitch/roll/스케일이 폰에 실릴 수 있었다 — Yaw 만 남기면서 엔진 정책과 맞췄다. 카메라 결과는 기존과 동일하다.

- **조회 3회 → 1회**: 부활 지점 결정이 `RestartPlayer` 한 곳으로 모여 `TActorIterator` 순회도 1회로 줄었다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **런타임 동작 미검증**: 스폰 호출 경로 자체가 바뀌었으므로 에디터 확인이 남았다 — 체크포인트 저장 후 사망/로드 시 그 지점·그 방향으로 스탯과 함께 부활하는지, 신규 세션이 `ChoosePlayerStart` 로 스폰되는지, PIE "여기서 플레이" 가 저장 위치를 무시하는지.
- **override 추가 축소 요청**: `InitGame` 을 제외한 나머지 override(`RestartPlayer`·`FinishRestartPlayer`) 제거가 후속 목표로 제기됨 — 별도 작업으로 검토.
