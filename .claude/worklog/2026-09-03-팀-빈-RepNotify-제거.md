# 팀 빈 RepNotify 제거

## 계획

### 목표
`AWxCharacterBase::Team` 의 RepNotify 가 본문 없는 껍데기라 "여기서 뭔가 한다"는 오해만 남긴다. 지정자를 단순 복제로 바꾸고 콜백을 없앤다. (module_review_WxGame 발견 4)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxCharacterBase.h` | `ReplicatedUsing = OnRep_Team` → `Replicated`, `OnRep_Team` 선언 제거, RepNotify 가 없는 이유를 한 줄 주석으로 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | `OnRep_Team` 정의 제거 | 수정 |

### 접근 방식
- **소비자를 먼저 확인하고 결정했다**: 팀 값을 읽는 곳(AI 인지의 가해자 피아 판정, 적대 판정 라이브러리, 타게팅 필터, 적 상호작용 가부, 투사체)이 전부 질의 시점에 복제 변수를 직접 읽는다. 클라이언트에서 팀을 캐시하거나 팀에 따라 표시를 바꾸는 소비자가 없어 통지받을 대상 자체가 없다. 유일한 캐시인 AI 컨트롤러의 1회 복사는 서버에만 있는 컨트롤러의 일이라, 클라이언트에서만 도는 OnRep 으로는 애초에 손댈 수 없다. 소환수가 주인의 팀을 물려받는 경로도 스폰 확정 전에 값을 심어 최초 복제부터 옳다.
- **없앤 자리에 이유를 남긴다**: OnRep 본문에 있던 "복제된 값을 직접 읽으므로 캐시 갱신이 필요 없다"는 설명이 함수와 함께 사라지면 다음 사람이 같은 RepNotify 를 다시 붙인다. 값 선언 위로 한 줄 옮긴다.
- **복제 자체는 유지한다**: 수명 등록은 그대로 두고 지정자만 바꾼다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/Character/WxCharacterBase.h` | 지정자를 `Replicated` 로 바꾸고 `OnRep_Team` 선언 제거, 값 선언 위에 이유 한 줄 | 수정 |
| `Source/WxGame/Character/WxCharacterBase.cpp` | `OnRep_Team` 정의 제거 | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` | 범위 밖 — 누락돼 있던 `Components/SkinnedMeshComponent.h` include 추가 | 수정 |

### 구현·결정과 그 이유
- **`Replicated` 가 맞았다**: 팀 값을 읽는 모든 경로가 질의 시점에 복제 변수를 직접 읽고, 클라이언트에 캐시나 팀 기반 표시가 하나도 없다. 통지받을 대상이 없으니 RepNotify 는 형태만 남은 껍데기였다. 서버에만 있는 AI 컨트롤러의 1회 복사는 클라이언트에서만 도는 OnRep 이 애초에 닿지 못하는 지점이라 근거가 되지 못한다.
- **이유를 값 옆으로 옮겼다**: 삭제한 함수 본문에만 있던 설명이라, 그대로 지우면 다음 사람이 같은 껍데기를 다시 붙인다.
- **막힌 빌드를 뚫으려 남의 파일 한 줄을 고쳤다**: `WxAbilitySystemComponent.h` 가 `EVisibilityBasedAnimTickOption` 을 쓰면서 선언 헤더를 include 하지 않아, 이 헤더를 무는 파일이 유니티에서 빠지는 순간 컴파일이 깨진다. 커밋된 상태부터 있던 잠복 버그이며 이번 변경과 무관하지만, 검증을 막고 있어 include 한 줄을 넣었다.

### 계획 대비 달라진 점
- 위 include 추가가 계획에 없던 수정이다.

### 후속 과제
- 전체 `WxEditor` 빌드는 라이브 코딩(에디터 실행 중) 때문에 대기 중이다. UHT 재생성과 `WxCharacterBase.cpp` 단일 파일 컴파일은 통과했고, 생성 코드에서 RepNotify 가 사라지고 `Team` 의 복제는 남은 것까지 확인했다.
