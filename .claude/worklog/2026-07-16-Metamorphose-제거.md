# Metamorphose(변신) 기능 전면 제거

## 계획

### 목표
GE 유효 기간 동안 캐릭터 SkeletalMesh를 교체하는 GameplayCue 기반 "변신(Metamorphose)" 기능을 코드·데이터·태그·문서에서 전부 제거한다. 태그 기반 자체완결 기능이라 서로만 참조하며, 텍스트 참조는 아래 6개 파일 + uasset 2개가 전부다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Cue/WxCueNotify_Metamorphose.h` | 클래스 파일 삭제 | 삭제 |
| `Plugins/WxCombat/.../Cue/WxCueNotify_Metamorphose.cpp` | 클래스 파일 삭제 | 삭제 |
| `Plugins/WxCore/.../Public/WxGameplayTags.h` | `GameplayCue_Metamorphose` 선언+주석 제거 | 수정 |
| `Plugins/WxCore/.../Private/WxGameplayTags.cpp` | `GameplayCue_Metamorphose` 정의 제거 | 수정 |
| `Content/AbilitySystem/Effect/GE_Metamorphose.uasset` | 변신 GE 삭제 | 삭제 |
| `Content/AbilitySystem/Cue/GC_Metamorphose.uasset` | 변신 GC(BP 자식) 삭제 | 삭제 |
| `Plugins/WxCore/README.md` | GameplayCue 목록에서 Metamorphose 제거 | 수정 |
| `Docs/Programmer/Module_Boundary_Audit.md` | GameplayCue 목록에서 Metamorphose 제거 | 수정 |

### 접근 방식
- **함께 삭제**: `GC_Metamorphose`는 C++ Abstract 클래스의 BP 자식 → C++ 파일과 uasset을 함께 삭제해야 부모 클래스 소실 로드 에러가 없다.
- **Config/스냅샷 무변경**: Cue 폴더는 폴더 단위 등록이고 다른 큐가 남음 → `GameplayCueNotifyPaths` 등 ini 무변경. `Metamorphose` 스냅샷 없음 → 정리 불필요.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Cue/WxCueNotify_Metamorphose.h` | 삭제 | 삭제 |
| `Plugins/WxCombat/.../Cue/WxCueNotify_Metamorphose.cpp` | 삭제 | 삭제 |
| `Content/AbilitySystem/Effect/GE_Metamorphose.uasset` | 삭제 | 삭제 |
| `Content/AbilitySystem/Cue/GC_Metamorphose.uasset` | 삭제 | 삭제 |
| `Plugins/WxCore/.../Public/WxGameplayTags.h` | `GameplayCue_Metamorphose` 선언+주석 제거 | 수정 |
| `Plugins/WxCore/.../Private/WxGameplayTags.cpp` | `GameplayCue_Metamorphose` 정의 제거 | 수정 |
| `Plugins/WxCore/README.md` | GameplayCue 목록에서 Metamorphose 제거 | 수정 |
| `Docs/Programmer/Module_Boundary_Audit.md` | GameplayCue 목록에서 Metamorphose 제거 | 수정 |

### 구현·결정과 그 이유
- **C++ + uasset 동시 삭제**: `GC_Metamorphose`가 Abstract C++ 클래스의 BP 자식이라 함께 지워야 부모 소실 로드 에러가 없다. `git rm`으로 4개 파일 삭제.
- **ini/스냅샷 무변경**: `GameplayCueNotifyPaths`는 폴더 단위 등록이고 Cue 폴더에 다른 큐가 남아 변경 불필요. `Metamorphose` 스냅샷 부재로 정리 불필요.
- **검증**: WxEditor(Development) 빌드 성공. source 제거로 UHT 재실행, WxCore/WxCombat 재빌드 통과 → 코드/태그 참조 잔존 없음 확인. 잔여 텍스트 참조는 본 worklog(제거 기록)뿐.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- uasset 역참조는 바이너리라 정적으로 완전 확인 불가 — 에디터 재기동 시 로그에 `GE_Metamorphose`/`GC_Metamorphose` 관련 missing reference 경고가 없는지 확인 권장(있다면 그 참조원 BP에서 수동 정리).
