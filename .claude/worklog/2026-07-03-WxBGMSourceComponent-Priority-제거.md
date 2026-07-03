# WxBGMSourceComponent Priority 제거

## 계획

### 목표
소스별 정수 `Priority`를 제거하고 BGM 우선순위 해소를 Chooser 테이블의 Row 순서에 맡긴다. 컴포넌트·서브시스템에 흩어진 `Priority` 배관을 걷어낸다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h` | `Priority` UPROPERTY 필드 제거, 클래스 doc 주석 정리 | 수정 |
| `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp` | `RegisterBGMSource` 호출에서 `Priority` 인자 제거 | 수정 |
| `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h` | `FWxBGMSourceRequest::Priority` 제거, `RegisterBGMSource` 파라미터 제거, 주석 정리 | 수정 |
| `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp` | `RegisterBGMSource`·`GetTopSource`에서 우선순위 로직 제거(뒤에서부터 첫 유효 소스 반환) | 수정 |
| `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` | `SourceOwner` 주석 문구만 정리 | 수정 |

### 접근 방식
- **컨텍스트 구조 유지**: `FWxBGMChooserContext`는 단일 `BGMTag`/`SourceOwner`만 담으므로 Chooser엔 항상 소스 하나만 전달된다. `Priority` 제거 시 다중 활성 소스 승자는 "가장 최근 등록(배열 뒤)"으로 결정 — 기존 동률 tie-break와 동일하며 실질 대부분 소스가 `Priority = 0`이라 지금도 지배적 동작이다.
- **재설계 배제**: 다중 소스 태그를 컨테이너로 넘기는 방식은 범위 밖. 순수하게 `Priority`만 제거.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxSound/Source/WxSound/Public/WxBGMSourceComponent.h` | `Priority` UPROPERTY 필드 제거, 클래스 doc 주석 정리 | 수정 |
| `Plugins/WxSound/Source/WxSound/Private/WxBGMSourceComponent.cpp` | `RegisterBGMSource(this, MusicTag)` 로 인자 축소 | 수정 |
| `Plugins/WxSound/Source/WxSound/Public/System/WxMusicSubsystem.h` | `FWxBGMSourceRequest::Priority` 제거, `RegisterBGMSource` 파라미터·`GetTopSource`/`ActiveSources` 주석 정리 | 수정 |
| `Plugins/WxSound/Source/WxSound/Private/System/WxMusicSubsystem.cpp` | `RegisterBGMSource` 본문·`GetTopSource` 우선순위 로직 제거(뒤에서 첫 유효 소스 반환) | 수정 |
| `Plugins/WxSound/Source/WxSound/Public/WxBGMChooserContext.h` | `SourceOwner` 주석 문구 정리 | 수정 |

### 구현·결정과 그 이유
- **`GetTopSource`를 최근 우선 단일 루프로 축소**: 우선순위 비교 대상이 사라졌으므로 배열을 뒤에서부터 훑어 처음 만난 유효 소스(가장 최근 등록)를 반환. 유효성 검사(파괴된 소스·무효 MusicTag skip)는 그대로 유지해 빈 태그가 베이스라인 폴백을 덮지 않게 했다.
- **컨텍스트 구조 미변경**: `FWxBGMChooserContext`가 단일 `BGMTag`/`SourceOwner`만 담아 Chooser엔 소스 하나만 전달된다. 다중 소스 승자는 "가장 최근 등록"으로 자연 귀결되며, 이는 기존 동률 tie-break와 동일해 실질 동작 변화가 없다. 우선순위 해소는 Chooser Row 순서로 이관.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. (기존에 `Priority`를 설정해 둔 BP/컴포넌트 인스턴스가 있었다면 해당 값은 이제 무시되며, Chooser 테이블 Row 순서로 우선순위를 표현해야 함 — 데이터 측 후속은 기획/데이터 작업 영역)
