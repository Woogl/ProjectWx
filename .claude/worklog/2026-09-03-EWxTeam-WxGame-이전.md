# EWxTeam 정의를 WxGame으로 이전

## 계획

### 목표
피아 구분 열거형 `EWxTeam`이 WxAI에 정의돼 있지만 WxAI 안에는 사용처가 하나도 없고 실제 사용처는 전부 WxGame의 캐릭터 쪽이다. 소유 모듈과 사용 모듈이 어긋난 상태를 정리해 정의를 실사용처로 내린다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h` | WxGame으로 이동 | 삭제(이동) |
| `Source/WxGame/Character/WxTeamTypes.h` | 이동해 온 열거형 정의 | 신규(이동) |
| `Source/WxGame/Character/WxCharacterBase.h` | include를 모듈 상대 경로로 갱신 | 수정 |
| `Plugins/WxAI/README.md` | 핵심 타입 표에서 해당 행 제거 | 수정 |
| `Source/WxGame/README.md` | 핵심 타입 표에 해당 행 추가 | 수정 |

### 접근 방식
- **WxCore가 아니라 WxGame으로**: 모듈 리뷰 문서는 도메인 간 공용 계약이라며 WxCore를 제안했지만, 지금 이 열거형을 읽는 코드는 WxGame 하나뿐이다. WxCore에는 실제로 여러 도메인이 공유하는 계약만 두기로 했으므로 쓰이지도 않는 상태로 올려두지 않는다. 다른 도메인이 필요해지는 시점에 올린다.
- **값·이름은 그대로**: 열거자와 숫자값을 건드리지 않는다. WxCombat의 투사체 팀 판별이 엔진 `NoTeam`(255)과 `Neutral`이 같은 값이라는 전제 위에 서 있어서, 값이 바뀌면 그쪽 중립 처리가 조용히 어긋난다.
- **리다이렉트 없이 이동**: 콘텐츠 전체를 훑어 이 열거형을 참조하는 에셋이 없음을 확인했다. 기본값을 덮어쓴 블루프린트도 없어 패키지 경로가 바뀌어도 깨질 참조가 없다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxTeamTypes.h` | WxAI에서 옮겨온 열거형 정의, 내용은 그대로 | 신규(이동) |
| `Plugins/WxAI/Source/WxAI/Public/WxTeamTypes.h` | 이동으로 제거 | 삭제(이동) |
| `Source/WxGame/Character/WxCharacterBase.h` | include를 `Character/` 상대 경로로 | 수정 |
| `Plugins/WxAI/README.md` | 핵심 타입 표에서 해당 행 제거 | 수정 |
| `Source/WxGame/README.md` | 핵심 타입 표에 해당 행 추가 | 수정 |

### 구현·결정과 그 이유
- **WxCore가 아니라 WxGame**: 리뷰 문서의 제안은 WxCore였지만 실제로 이 열거형을 읽는 곳이 WxGame 하나뿐이라 공용 계약이라 부를 근거가 없다. 쓰는 데가 늘면 그때 올린다.
- **`git mv`로 이력 보존**: 내용이 그대로라 이동으로 기록돼야 나중에 이 타입의 유래를 따라갈 수 있다.
- **include를 모듈 상대 경로로**: 같은 폴더라 파일명만으로도 찾히지만, WxGame의 다른 헤더가 전부 모듈 루트 기준 경로를 쓴다.
- **리다이렉트를 두지 않았다**: 이 열거형을 참조하는 에셋이 콘텐츠 전체에 없어 패키지 경로 변경으로 끊길 참조가 없다. 쓰이지 않을 리다이렉트를 ini에 남기지 않는다.

### 계획 대비 달라진 점
계획대로.

### 후속 과제
- 없음. 빌드(WxEditor Development) 통과했고, WxAI 쪽 UHT 산출물도 정리돼 WxGame으로 다시 생성됐다.
