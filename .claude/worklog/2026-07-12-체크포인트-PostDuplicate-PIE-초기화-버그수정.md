# 체크포인트 PostDuplicate가 PIE 월드 복제에서 시작지점 값을 날리는 버그 수정

## 계획

### 목표
PIE로 실행할 때마다 `bIsDefaultStart` 지점을 못 찾아 `ChoosePlayerStart`가 nullptr을 반환하고 에러 로그가 찍히는 회귀를 고친다. 원인은 `AWxCheckPoint::PostDuplicate`가 `DuplicateMode`를 가리지 않고 `PlayerStartTag`·`bIsDefaultStart`를 초기화하는 것. PIE는 에디터 월드를 `EDuplicateMode::PIE`로 통째 복제하므로 플레이 월드의 모든 체크포인트에서 두 값이 매번 리셋된다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxCheckPoint.cpp` | `PostDuplicate`에 `EDuplicateMode::Normal` 가드 추가(에디터 단일 복제일 때만 초기화) | 수정 |

### 접근 방식
- **초기화를 `Normal` 모드로 한정**: `PostDuplicate` 초기화의 의도는 디자이너의 에디터 단일 액터 복제(Ctrl+W = `EDuplicateMode::Normal`)에서 사본을 "미완성"으로 만들어 새 태그를 강제하는 것. PIE 월드 복제(`EDuplicateMode::PIE`)·레벨 복제(`EDuplicateMode::World`)는 배치된 원본 값을 보존해야 하므로, `DuplicateMode != Normal`이면 조기 반환한다.
- **부수 효과**: 같은 리셋이 `PlayerStartTag`도 날려 PIE에서 step 1(저장 태그 부활)까지 깨뜨리고 있었다. 가드 하나로 두 증상(최초 시작지점·체크포인트 부활)이 함께 해소된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WorldObject/WxCheckPoint.cpp` | `PostDuplicate`에 `DuplicateMode != Normal` 조기 반환 가드 추가 | 수정 |

### 구현·결정과 그 이유
- **`Normal` 모드만 초기화**: 사본 리셋의 목적은 디자이너의 에디터 단일 복제 때 새 태그를 강제하는 것뿐. `PIE`·`World` 모드는 배치 원본을 그대로 살려야 하므로 `!= Normal`이면 초기화 이전에 반환한다. `PIE` 하나만 배제하지 않고 `Normal`만 통과시켜, 레벨 복제(`World`)에서도 태그가 보존되도록 화이트리스트 방식으로 좁혔다.
- **회귀 진입점**: 7/9 명시화 작업에서 들어온 `PostDuplicate` 초기화가 `DuplicateMode`를 가리지 않아, PIE 월드 복제마다 `bIsDefaultStart`·`PlayerStartTag`가 함께 날아가 `ChoosePlayerStart` step 2·step 1을 동시에 무력화하고 있었다. 가드 한 줄로 두 증상이 해소된다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 사용자 확인: PIE 재실행 시 에러 로그(`bIsDefaultStart ... 없음`)가 사라지고 `bIsDefaultStart` 지점에 스폰되는지, 체크포인트 상호작용 후 부활이 그 지점으로 되는지 확인. (앞서 언급한 잔여 `Saved/SaveGames/Test.sav`의 저장 태그가 남아 있으면 step 1이 우선하므로, 순수 최초 시작 확인 시엔 슬롯 파일을 지우고 테스트)
