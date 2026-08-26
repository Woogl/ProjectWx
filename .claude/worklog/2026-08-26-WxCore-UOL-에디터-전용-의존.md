# WxCore UOL 의존을 에디터 전용 Private 으로 내리기

## 계획

### 목표
`/module-review WxCore` 발견 3을 해소한다. WxCore 는 `UniversalObjectLocator` 를 Public 의존으로 올려두지만 실제 사용처는 `#if WITH_EDITOR` 로 묶인 `FWxLocatorUtils` 하나뿐이라, Shipping 런타임이 쓰지 않는 모듈을 링크한다. 이를 에디터 타겟 한정 Private 의존으로 내린다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/Source/WxCore/WxCore.Build.cs` | Public 목록에서 `UniversalObjectLocator` 를 빼고, `Target.bBuildEditor` 블록의 Private 의존으로 이동 | 수정 |

### 접근 방식
- **Public 이어야 할 근거 없음**: 공개 헤더는 `FUniversalObjectLocator` 를 전방 선언만 하고 UOL 헤더를 포함하지 않으며, UHT 가 보는 리플렉션 타입도 없다. 실제 포함은 전부 cpp 의 에디터 가드 안이다.
- **전이 의존 무의존 확인**: WxCore 밖에서 UOL 을 쓰는 모듈은 전부 자기 Build.cs 에 UOL 을 직접 선언하고 있어, 이 이동으로 끊기는 소비자가 없다.
- **기존 패턴 답습**: 에디터 전용 의존 블록은 WxUI·WxWorld 가 이미 쓰는 `if (Target.bBuildEditor)` + `PrivateDependencyModuleNames` 형태를 그대로 따른다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `<파일>` | <수정 요약> | <신규·수정·삭제 명시> |

### 구현·결정과 그 이유
- **<결정>**: <왜 이렇게 했는가>

### 계획 대비 달라진 점
- <무엇이, 왜 달라졌는가> (없으면 "계획대로")

### 후속 과제
- <남은 일·미검증 항목> (없으면 "없음")
