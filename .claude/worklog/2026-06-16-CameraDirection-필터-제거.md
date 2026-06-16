# CameraDirection 타겟팅 필터 제거

## 계획

### 목표
더 이상 사용하지 않는 `UWxTargetingFilterTask_CameraDirection`(시점 정면 각도 기준 후방 제외)을 삭제한다. 화면 밖 제외는 새로 추가한 `ScreenBounds` 필터가 FOV·종횡비까지 반영해 더 정확히 대체한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/Targeting/WxTargetingFilterTask_CameraDirection.h` | 클래스 삭제 | 삭제 |
| `Plugins/WxCombat/.../Private/Targeting/WxTargetingFilterTask_CameraDirection.cpp` | 구현 삭제 | 삭제 |

### 접근 방식
- 삭제 전 참조 확인: 코드 참조는 자기 자신뿐, 컨텐츠 애셋(`TP_LockOn` 포함) 어디에서도 이 클래스를 참조하지 않음을 확인 후 제거.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/Targeting/WxTargetingFilterTask_CameraDirection.h` | 삭제 | 삭제 |
| `Plugins/WxCombat/.../Private/Targeting/WxTargetingFilterTask_CameraDirection.cpp` | 삭제 | 삭제 |

### 구현·결정과 그 이유
- **안전 확인 후 삭제**: 외부 코드/애셋 참조가 없음을 먼저 확인해 로드 시 끊긴 참조가 생기지 않게 했다. 락온 프리셋은 이미 이 필터를 떼어 둔 상태였다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. WxEditor(Development) 빌드 성공으로 검증 완료.
