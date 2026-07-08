# WxSave DeleteSaveFile bool 반환

## 계획

### 목표
`DeleteSaveFile` 을 삭제 성공 여부(bool)를 반환하도록 바꿔, 호출 측(UI 등)이 삭제 결과에 따라 분기할 수 있게 한다. (사용자 명시 지시)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxPersistenceGameSubsystem.h` (+`.cpp`) | `void DeleteSaveFile` → `bool` (`DeleteGameInSlot` 결과 반환) | 수정 |
| `Plugins/WxSave/.../Public/WxSaveFilePersistenceUtils.h` (+`.cpp`) | BFL 래퍼 `void` → `bool` (서브시스템 결과 반환, 서브시스템 부재 시 false) | 수정 |

### 접근 방식
- 엔진 `UGameplayStatics::DeleteGameInSlot` 가 이미 bool 을 반환하므로 그 값을 그대로 전파. 로그 분기는 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSave/.../Public/WxPersistenceGameSubsystem.h` (+`.cpp`) | `DeleteSaveFile` 반환형 `void`→`bool`, `DeleteGameInSlot` 결과를 지역 변수로 받아 로그 후 반환 | 수정 |
| `Plugins/WxSave/.../Public/WxSaveFilePersistenceUtils.h` (+`.cpp`) | BFL `DeleteSaveFile` 반환형 `void`→`bool`, 서브시스템 결과 반환·부재 시 false | 수정 |

### 구현·결정과 그 이유
- **엔진 반환값 전파**: `DeleteGameInSlot` 의 bool 을 서브시스템→BFL 로 그대로 올려, UI 가 "삭제됨/파일 없음" 을 구분할 수 있게 했다. BFL 은 서브시스템을 못 찾으면 false(삭제 안 됨)를 반환.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **UI 배선(사용자 몫)**: `WBP_MainMenu` `[Delete Saved Data]` 노드가 반환 bool 로 결과 피드백(삭제 성공/실패 메시지) 처리 가능.
- WxEditor(Development) 컴파일·링크 성공 확인. 런타임 미검증.
