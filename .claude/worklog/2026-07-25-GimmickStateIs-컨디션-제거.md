# GimmickStateIs 컨디션 제거

## 계획

### 목표
기믹 ST 8개를 전부 이벤트(RequiredEventToEnter) 패턴으로 통일한 결과, `FWxStateTreeCondition_GimmickStateIs` 조건 노드가 어떤 에셋·코드에서도 참조되지 않는 죽은 코드가 됐다. 이를 제거해 "AWxGimmick 캐스트 노드" 표면을 줄인다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `FWxStateTreeCondition_GimmickStateIs` + `...InstanceData` 구조체·주석 삭제 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `TestCondition` + `GetDescription` 구현 삭제 | 삭제 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 캐스트 노드 목록에서 GimmickStateIs 제거, 태그 피커 관련 서술 정리 | 수정 |

### 접근 방식
- **정의만 삭제**: `GimmickStateIs`는 owner를 `AWxGimmick`으로 캐스트하는 기믹 전용 조건인데, 8개 기믹 ST 전부 이벤트 전환으로 제거됐고 코드 참조는 자기 정의뿐이라(grep 확인) 순수 삭제로 끝난다. 다른 struct/함수에 얽힌 의존 없음.
- **에셋 안전**: 코드 참조 0건은 빌드로 검증되나, 만약의 잔여 에셋 참조는 로드 경고로만 드러난다(기믹 전용이라 가능성 희박). 8개 외 사용처 없음을 전제로 진행.
- **문서 동기화**: `Gimmick_State_Authority.md`가 GimmickStateIs를 캐스트 노드로 명시하므로 함께 갱신(이제 PlayLevelSequence·EnableInteraction 둘만).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `FWxStateTreeCondition_GimmickStateIs` + `...InstanceData` 구조체·주석 삭제 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `TestCondition` + `GetDescription`(WITH_EDITOR) 구현 삭제 | 삭제 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 캐스트 노드 목록 `GimmickStateIs·PlayLevelSequence·EnableInteraction 셋` → `PlayLevelSequence·EnableInteraction 둘`, 참조표에서 `FWxStateTreeCondition_GimmickStateIs` 제거 | 수정 |

### 구현·결정과 그 이유
- **순수 삭제로 종결**: 코드 참조가 자기 정의뿐이라(grep) 다른 코드 손댈 것 없이 구조체·구현·주석만 제거. WxEditor(Development) 빌드 성공(WxWorld 컴파일+링크 통과)으로 검증.
- **문서를 코드와 동기화**: 이 컨디션이 문서 두 곳(캐스트 노드 규칙·참조표)에 박혀 있어 함께 갱신. 규칙 2(재선택=RequiredEventToEnter)는 이미 정통 패턴을 서술 중이라 변경 불필요.

### 계획 대비 달라진 점
- 계획대로. 빌드가 링크까지 성공(에디터 실행 중이었으나 WxWorld DLL 링크 통과).

### 후속 과제
- 실행 중인 에디터는 구 DLL을 메모리에 물고 있어 태그 피커에 `Gimmick State Is`가 재시작 전까지 남을 수 있음(무해, 사용처 0). 다음 에디터 재시작 시 사라짐.
