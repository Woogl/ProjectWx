# StateTree 태스크 완료판정 플래그를 엔진 순정(true)으로 통일

## 계획

### 목표
프로젝트 태스크들에 남은 `bConsideredForCompletion = false` 예외를 걷어내고 엔진 순정 디폴트(true)로 통일한다. 엔진이 이 플래그를 노드 인스턴스에서 author 하도록 설계했으므로(`bCanEditConsideredForCompletion` 기본 true), 정지 leaf 예외는 C++ 생성자가 아니라 ST 에셋에 둔다. 현재 코드는 실제 세팅 2곳과 주석 5곳이 어긋나 있어 정리가 필요하다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `FWxStateTreeTask_PlaySound()` 생성자의 `#if WITH_EDITORONLY_DATA` 블록 제거(`bShouldCallTick = false` 는 유지) | 수정 |
| `Plugins/WxSave/.../Private/WxStateTreeTask_SaveGame.cpp` | 같은 블록 제거 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `EnableInteraction`·`EnablePlayerInput`·`PlaySound`·`SpawnNiagara` struct doc 에서 `bConsideredForCompletion=false` 서술 문장 삭제 | 수정(주석) |
| `Plugins/WxSave/.../Public/WxStateTreeTask_SaveGame.h` | 같은 문장 삭제 | 수정(주석) |
| `Docs/Programmer/Gimmick_State_Authority.md` | 「완료/thrash」 항목을 새 컨벤션으로 재서술 | 수정(문서) |

### 접근 방식
- **C++ 는 순정만 남긴다**: 각 태스크 헤더가 이미 "Succeeded 로 완료한다"고 명시하므로 완료 동작은 그대로 읽힌다. 사실과 어긋나는 플래그 서술만 걷어내고 예외 규칙을 태스크마다 반복하지 않는다.
- **컨벤션은 기믹 문서 한 곳에**: 정지 leaf 는 에셋에서 해당 노드의 완료판정을 끄거나 머무는 태스크를 둔다(C++ 디폴트는 순정 true)로 고친다. 기존 문장의 "순간 토글 태스크는 기본 off" 는 사실이 아니므로 함께 제거한다.
- **소급 인지**: `FInstancedStruct::Serialize` 는 기본 생성값 대비 델타 저장이라, 디자이너가 손대지 않은 노드는 다음 로드·컴파일 때 새 디폴트를 따라간다. 코드 변경만으로 끝나지 않고 에셋 감사가 뒤따른다.
- **에셋은 이번 범위 밖**: `PlaySound`·`SaveGame` 이 유일한 완료 구동자인 정지 leaf 만 위험하며, 판별에 에디터가 필요해 후속 작업으로 분리한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `FWxStateTreeTask_PlaySound()` 의 `bConsideredForCompletion = false` 블록 제거 | 수정 |
| `Plugins/WxSave/.../Private/WxStateTreeTask_SaveGame.cpp` | 같은 블록 제거 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `EnableInteraction`·`EnablePlayerInput`·`PlaySound`·`SpawnNiagara` struct doc 에서 플래그 서술 문장 삭제 | 수정(주석) |
| `Plugins/WxSave/.../Public/WxStateTreeTask_SaveGame.h` | 같은 문장 삭제 | 수정(주석) |
| `Docs/Programmer/Gimmick_State_Authority.md` | 「완료/thrash」를 새 컨벤션으로 재서술 | 수정(문서) |

### 구현·결정과 그 이유
- **예외를 코드에서 데이터로 옮김**: 엔진은 이 플래그를 노드 인스턴스에서 author 하도록 만들었고(편집 가능 여부가 기본 열려 있다), 태스크 타입 단위로 끄면 그 타입을 쓰는 모든 상태에 일괄 적용돼 순차 choreography 처럼 완료가 필요한 상태까지 영향을 받는다. 정지 leaf 는 leaf 의 성질이지 태스크의 성질이 아니므로 에셋에서 결정하는 편이 맞다.
- **주석은 삭제, 컨벤션은 문서 한 곳**: 각 태스크 헤더가 이미 완료를 반환한다고 밝히고 있어 플래그 서술은 중복이었고, 실제 코드와 어긋난 채 방치되기 쉬웠다(이번에도 셋이 어긋나 있었다). 규칙은 기믹 문서에만 두고 재서술 시 완료 전이가 없을 때 엔진이 Root 재선택 후 실패하면 트리를 정지시킨다는 결말까지 명시했다 — 이걸 알아야 정지 leaf 를 왜 비워야 하는지가 설명된다.
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`(exit 0). 변경 4개 모듈 파일 컴파일·링크 정상.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **ST 에셋 감사(사용자, 에디터)**: ST 8종에서 정지 leaf 의 태스크가 전부 즉시완료인 상태를 찾아, 그 노드의 완료판정을 인스턴스별로 끄고 Compile·저장한다. 1순위는 체크포인트 `Lit`(SpawnNiagara + PlaySound + SaveGame). PIE 로그에 `Could not trigger completion transition, jump back to root state.` 또는 `Failed to select root state.` 가 뜨면 걸린 leaf 다.
- **`Docs/Programmer/module_review_WxWorld.md` 발견 4 무효화**: 그 리뷰는 반대 방향(세 생성자에 플래그 추가)을 제안했는데 이번에 순정 유지로 결론이 났다. 다음 `module-review` 실행 시 같은 지적이 재등장할 수 있다.
