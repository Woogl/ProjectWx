# 인디케이터 BeginPlay 접근 지정자 정정

## 계획

### 목표
인디케이터 액터가 베이스에서 protected인 BeginPlay를 public으로 재정의해 외부에서 직접 부를 수 있게 열어 두고 있다. override는 베이스 접근 지정자를 따른다는 관례에 맞춰 protected로 되돌린다(WxUI 모듈 리뷰 4번).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` | BeginPlay 선언을 새 protected 절로 이동 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp` | BeginPlay 정의를 헤더 선언 순서에 맞춰 이동 | 수정 |

### 접근 방식
- **베이스 접근 지정자를 따른다**: 베이스에서 BeginPlay는 protected이고 Tick은 public이다. 그래서 BeginPlay만 옮긴다. 외부 호출부나 C++ 파생 클래스가 없고 엔진이 호출하는 함수라 동작은 바뀌지 않는다.
- **정의 순서 동기화**: 헤더에서 public 멤버 뒤로 옮겼으므로, cpp에서도 공개 함수 정의 뒤, private 함수 정의 앞으로 옮긴다. 본문은 그대로 둔다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h` | BeginPlay 선언을 새 protected 절로 이동 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/Indicator/WxIndicator.cpp` | BeginPlay 정의를 공개 함수 정의 뒤, private 함수 정의 앞으로 이동(본문 변경 없음) | 수정 |

### 구현·결정과 그 이유
- **BeginPlay만 옮김**: 베이스에서 Tick은 public이라 그대로 두었다. BeginPlay는 엔진만 부르는 함수라, 외부에 열어 둘 이유가 없다.
- **검증**: UE 5.8.1 WxEditor Win64 Development 빌드 성공.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음
