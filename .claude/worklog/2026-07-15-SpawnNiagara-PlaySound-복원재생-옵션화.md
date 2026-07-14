# SpawnNiagara·PlaySound 복원 재생 옵션화 + 무틱 트리거 완료 비구동

## 계획

### 목표
`SpawnNiagara`·`PlaySound`의 초기·복원 진입 가드를 `bPlayOnRestore` 옵션으로 선택화한다. 기본(false)은 트리거 FX/사운드(라이브 1회, 복원 침묵) 그대로, true 면 로드/복원 시에도 재생해 상태에 묶인 지속 FX(예: 체크포인트 모닥불)를 새 태스크 없이 표현한다. 아울러 두 무틱 즉시완료 태스크의 `bConsideredForCompletion`을 false 로 통일해 정지 leaf 재선택 thrash 를 근절한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.h` | `PlaySoundInstanceData`·`SpawnNiagaraInstanceData`에 `bPlayOnRestore`(기본 false) 추가, 두 태스크 struct 문서 주석·상단 개요 갱신 | 수정 |
| `Plugins/WxWorld/.../Gimmick/WxGimmickStateTreeNodes.cpp` | `PlaySound`·`SpawnNiagara`의 EnterState 가드를 `bInitialEntry && !Instance.bPlayOnRestore`로, 생성자에 `bConsideredForCompletion = false` 추가 | 수정 |

### 접근 방식
- **가드 선택화**: 각 EnterState 에서 `Instance`를 먼저 취하고, 가드 조건을 `bInitialEntry && !Instance.bPlayOnRestore`로 바꾼다. 라이브 진입(SourceStateID 유효)은 옵션과 무관하게 항상 재생하므로 기존 사용처 동작 불변.
- **완료 비구동 통일**: 무틱 즉시완료 side-effect 태스크는 완료를 구동하지 않는다는 규칙(2026-06-27 토글 태스크 결정)을 이 둘로 확장. 생성자에서 `#if WITH_EDITORONLY_DATA bConsideredForCompletion = false; #endif`. 정지 leaf 에 놓여도 즉시 완료→재선택 루프에 빠지지 않는다.
- **기존 에셋 안전성(감사 완료)**: PlaySound 는 어느 ST 에셋에서도 미사용(무해). SpawnNiagara 는 ST_CheckPoint·ST_AlarmConsole 이 사용하나 둘 다 종단 정지 leaf(Lit/Alarmed)라 SpawnNiagara 로 완료를 구동하지 않으므로, 기본값 false 전환이 오히려 양쪽의 잠재 thrash 를 제거한다.
- **에셋 후속(에디터, 코드 외)**: ST_CheckPoint 는 루프 Niagara 지정 + `bPlayOnRestore=true`로 로드 후에도 불 유지. ST_AlarmConsole 은 트리거 그대로(기본값). 완료 기본값 반영을 위해 두 ST 리세이브 필요(미반영 시 인스턴스 체크 해제로 폴백).

---

## 완료

### 수정한 파일

### 구현·결정과 그 이유

### 계획 대비 달라진 점

### 후속 과제
