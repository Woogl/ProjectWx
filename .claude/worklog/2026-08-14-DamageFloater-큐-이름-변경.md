# DamageFloater 큐 클래스 이름 변경

## 계획

### 목표
데미지 플로터 전용으로 축소된 큐 클래스가 여전히 임팩트 연출 큐와 헷갈리는 이름을 쓰고 있어, 실제 책임에 맞는 이름으로 바꾼다. 동작 변경은 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Cue/WxCueNotify_Damage.h` → `WxCueNotify_DamageFloater.h` | 파일·클래스 이름 변경, generated.h 인클루드 갱신 | 수정 |
| `WxCombat/.../Cue/WxCueNotify_Damage.cpp` → `WxCueNotify_DamageFloater.cpp` | 파일 이름과 인클루드 경로, 정의부 한정자 갱신 | 수정 |
| `Config/DefaultEngine.ini` | CoreRedirects 섹션 신설, 클래스 리다이렉트 한 줄 추가 | 수정 |

### 접근 방식
- **파일명 동반 변경**: UHT가 generated.h 이름을 헤더 파일명에서 끌어오므로 클래스만 바꿀 수 없다.
- **CoreRedirect**: 이 클래스를 부모로 삼는 블루프린트 큐 에셋이 하나 있어, 리다이렉트 없이 바꾸면 에디터에서 부모를 잃는다.
- **동거 타입 유지**: 같은 헤더의 플로터 액터와 인터페이스는 이미 플로터 이름이라 손대지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat/.../Cue/WxCueNotify_DamageFloater.h` | 파일 이름과 클래스 이름 변경 | 수정 |
| `WxCombat/.../Cue/WxCueNotify_DamageFloater.cpp` | 파일 이름·인클루드·정의부 한정자 변경 | 수정 |
| `Config/DefaultEngine.ini` | CoreRedirects 섹션과 클래스 리다이렉트 추가 | 수정 |

### 구현·결정과 그 이유
- **에셋 이름은 그대로**: 블루프린트 큐 에셋 이름 변경은 요청 범위 밖이고, 리다이렉트만으로 부모 연결이 유지된다.
- **옛 worklog 미수정**: 지난 문서의 클래스 이름은 그 시점의 기록이라 손대지 않았다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 에디터에서 블루프린트 큐 에셋을 열어 부모가 새 클래스로 잡히는지 확인(리다이렉트 실동작). 재저장하면 리다이렉트가 에셋에 굳는다.
