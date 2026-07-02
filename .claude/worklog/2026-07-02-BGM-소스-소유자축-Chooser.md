# BGM Chooser 소스 소유자 축 추가

## 계획

### 목표
BGM Chooser 가 곡을 소스 소유자의 **클래스**(플레이어/적/보스/개별 보스 BP)로도 구분할 수 있도록, 우선순위 승자 소스의 소유자 액터를 컨텍스트에 전달한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxSound/Public/WxBGMChooserContext.h` | `AActor* SourceOwner` 필드 추가(+`class AActor;` 전방선언) | 수정 |
| `WxSound/Public/System/WxMusicSubsystem.h` | `FWxBGMSourceRequest` 에 `Owner` 보관, `GetEffectiveBGMTag → GetTopSource`(승자 반환)로 리팩터 | 수정 |
| `WxSound/Private/System/WxMusicSubsystem.cpp` | 등록 시 소유자 해석·보관, `EvaluateBGM` 이 승자의 `BGMTag`+`SourceOwner` 를 채움(없으면 베이스라인+null) | 수정 |

### 접근 방식
- **Object Class 컬럼 활용**: 엔진 Chooser 의 "Object Class" 컬럼이 컨텍스트 오브젝트 참조의 클래스를 `SubClassOf` 등으로 필터한다. 컨텍스트에 승자 소스의 소유자 액터를 넣으면, 디자이너가 그 클래스로 행을 매칭한다.
- **의존성 clean**: 컨텍스트는 제네릭 `AActor*` 만 보유 → WxSound 는 WxGame 클래스명을 모른다. 구체 클래스(`AWxBossCharacter` 등)는 Chooser 테이블(데이터)에서만 참조하므로 WxSound→WxGame 역참조가 없다.
- **소유자는 내재**: 소스의 owner 액터가 곧 구분자라 컴포넌트에 별도 카테고리 필드/캐릭터 C++ 변경이 필요 없다. 소유자는 등록 시 소스(UActorComponent)의 `GetOwner()` 로 해석해 요청에 보관.
- **규칙 유지**: 소스는 유효 `MusicTag` 가 있어야 등록(빈 태그가 베이스라인을 덮지 않도록). 승자가 되면 `BGMTag` 와 `SourceOwner` 가 함께 설정된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxSound/Public/WxBGMChooserContext.h` | `TObjectPtr<AActor> SourceOwner` 필드 + `class AActor;` 전방선언 | 수정 |
| `WxSound/Public/System/WxMusicSubsystem.h` | `FWxBGMSourceRequest.Owner` 추가, `GetEffectiveBGMTag → GetTopSource`(승자 반환) | 수정 |
| `WxSound/Private/System/WxMusicSubsystem.cpp` | 등록 시 소유자 해석·보관, `EvaluateBGM` 이 승자의 `BGMTag`+`SourceOwner` 채움, `ActorComponent.h` include | 수정 |

### 구현·결정과 그 이유
- **enum 대신 소유자 액터 전달**: 엔진 Chooser 의 "Object Class" 컬럼이 오브젝트의 클래스를 `SubClassOf` 로 필터할 수 있음을 확인. 승자 소스의 owner 액터를 컨텍스트에 넣으면 디자이너가 클래스로 행을 매칭한다. enum 은 닫힌 소집합이라 확장·개별 보스 구분이 어렵지만, 클래스 매칭은 `AWxPlayerCharacter`/`AWxEnemyCharacter`/`AWxBossCharacter`부터 개별 보스 BP 까지 자연 계층으로 커버.
- **의존성 clean**: 컨텍스트는 제네릭 `AActor*` 만 보유 → WxSound 는 WxGame 클래스명을 모른다. 구체 클래스는 Chooser 테이블(데이터)에서만 참조하므로 WxSound→WxGame 역참조 없음. 그래서 컴포넌트 필드/캐릭터 C++ 변경도 불필요.
- **소유자는 등록 시 1회 해석**: 소스(UActorComponent)의 `GetOwner()` 를 요청에 보관해 평가 때 재계산·캐스팅 반복을 피함(컴포넌트는 소유자가 바뀌지 않음).
- **GetTopSource 로 리팩터**: 이전엔 태그만 반환했으나, 이제 승자의 MusicTag·Owner 둘 다 필요해 승자 요청 자체를 반환.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 데이터: Chooser 테이블에 Object Class 컬럼 추가(InputValue=`SourceOwner`), 보스/적/플레이어 행을 `SubClassOf` 로 구성(보스 행을 일반 적 행 위에). 실동작은 데이터가 채워져야 확인 가능(현재 컴파일만 검증).
- MusicTag 없이 소유자만으로 등록하는 방식(빈 MusicTag 소스 허용)은 현재 미지원 — 필요 시 분리.
