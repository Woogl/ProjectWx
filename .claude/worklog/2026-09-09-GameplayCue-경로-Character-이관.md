# GameplayCue 경로 Character 이관

## 계획

### 목표
GameplayCue 에셋이 `/Game/AbilitySystem/Cue`에서 `/Game/Character/Shared/Cues`로 옮겨졌으므로, Cue 탐색 경로와 강제 쿠킹 경로를 `/Game/Character`로 바꾼다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Config/DefaultGame.ini` | `GameplayCueNotifyPaths`, `DirectoriesToAlwaysCook` 를 `/Game/Character` 로 교체 | 수정 |

### 접근 방식
- **두 설정 동시 교체**: 탐색 경로만 바꾸면 Editor·Development 는 통과하고 Shipping 에서만 Cue 가 빠지므로, 강제 쿠킹 경로도 같은 값으로 맞춘다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Config/DefaultGame.ini` | 두 경로를 `/Game/Character` 로 교체 | 수정 |

### 구현·결정과 그 이유
- **추가가 아닌 교체**: 디스크에 `Content/AbilitySystem/Cue` 폴더가 더 이상 없고 GC_ 에셋은 전부 `Content/Character/Shared/Cues` 에 있다.
- **쿠킹 경로도 `/Game/Character`**: 이후 캐릭터별 하위 폴더에 Cue 를 두어도 Shipping 에서 빠지지 않도록 탐색 경로와 동일하게 맞췄다. 대신 `Content/Character` 전체가 강제 쿠킹 대상이 된다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- C++ 변경이 없어 빌드 검증은 생략. Shipping 패키징 시 Cue 재생 확인 권장.
