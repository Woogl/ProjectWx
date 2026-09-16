# Subtitle 익명 namespace 헬퍼 제거

## 계획

### 목표

`Docs/Programmer/module_review_WxUI.md` 5번 이슈 해결. `MakeSubtitleContext()`는 호출부가 한 곳뿐인 익명 namespace 헬퍼이고, 헬퍼 주석이 내세우는 "등록·조회가 같은 값" 보장은 이미 호출부의 지역 변수 하나를 조회·등록에 함께 쓰는 것으로 성립한다. `.cpp` 내부 헬퍼는 호출부에 인라인한다는 방침에 맞춘다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp` | 익명 namespace 블록 삭제, 문맥 구성 3줄을 `GetOrCreate` 안으로 이동 | 수정 |

### 접근 방식

- **호출부 인라인**: `const FMVVMViewModelContext Context = MakeSubtitleContext();`를 기본 생성 + 두 필드 대입 3줄로 대체한다. `FMVVMViewModelContext`에 두 필드를 한 번에 채우는 생성자가 없어 `const`는 뗀다.
- **주석 미이동**: 같은 지역 변수를 조회·등록에 쓰는 코드가 바로 아래 이어져 주변 맥락에서 읽힌다.
- **include 유지**: 타입을 여전히 값으로 쓴다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Subtitle.cpp` | 익명 namespace의 `MakeSubtitleContext()` 삭제, 문맥 구성 3줄을 `GetOrCreate` 안으로 이동 | 수정 |

### 구현·결정과 그 이유
- **`const` 제거**: `FMVVMViewModelContext`에 두 필드를 한 번에 채우는 생성자가 없어 기본 생성 후 대입해야 한다.
- **헬퍼 주석 미이동**: 만든 `Context` 하나를 바로 아래에서 조회·등록에 쓰는 코드가 이어져 주변 맥락에서 읽힌다.
- **include 유지**: `Types/MVVMViewModelContext.h`의 타입을 여전히 값으로 쓴다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- `Docs/Programmer/module_review_WxUI.md` 5번 항목 정리는 `module-review` 재실행 몫으로 남겨 뒀다.
