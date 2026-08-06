# WxAnimNotify_AreaAttack 클래스 제거

## 계획

### 목표
어떤 콘텐츠도 사용하지 않는 범위 공격 AnimNotify를 제거해 전투 모듈 표면을 줄인다. 헤더에 "게임 로직 이관 필요" TODO가 남아 있고, 모듈 리뷰가 지적한 결함도 이 클래스 안에만 있어 고치기보다 지우는 편이 맞다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotify_AreaAttack.h` | 파일 제거 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaAttack.cpp` | 파일 제거 | 삭제 |

### 접근 방식
- **사용처 선확인 후 단순 삭제**: 코드 참조는 자기 자신 외에 없고, 콘텐츠 전체 `.uasset`/`.umap`을 바이너리 검색해도 이 클래스 이름이 잡히지 않는다(대조군으로 다른 노티파이 클래스는 몽타주에서 다수 검출). 몽타주에 배치된 인스턴스가 없으므로 리다이렉터 없이 파일만 지운다.
- **주변 정리 없음**: 타게팅 모듈 의존성은 락온·타게팅 필터가 계속 쓰므로 빌드 스크립트는 손대지 않는다. 리뷰 문서는 리뷰 시점 스냅샷이라 그대로 둔다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotify_AreaAttack.h` | 파일 제거 | 삭제 |
| `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotify_AreaAttack.cpp` | 파일 제거 | 삭제 |

### 구현·결정과 그 이유
- **고치지 않고 지움**: 리뷰에서 나온 지적(무의미한 캐스트 분기)이 이 클래스 한정이라, 쓰이지 않는 코드를 다듬는 대신 삭제해 유지 대상에서 뺐다.
- **AoE 대미지 경로는 라이브러리 쪽에 남김**: 무기·투사체 밖 단일 대미지 진입점은 전투 라이브러리에 그대로 있어, 나중에 범위 공격이 필요해지면 타게팅 쿼리 결과를 그 진입점에 흘리는 형태로 어빌리티 쪽에 다시 만들면 된다. 노티파이가 직접 대미지를 적용하던 구조(TODO가 가리키던 문제)를 되살릴 이유는 없다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음
