# FWxStateTreeTask_PlaySkeletalAnim → FWxStateTreeTask_PlayAnimation 이름 변경

## 계획

### 목표
공용 기믹 StateTree 태스크 `FWxStateTreeTask_PlaySkeletalAnim`을 더 일반적인 이름 `FWxStateTreeTask_PlayAnimation`으로 개명한다. 동작 변경·인자 추가는 없다(이름만). 이 구조체를 노드로 참조하는 `ST_Elevator.uasset`의 참조가 끊기지 않도록 CoreRedirect로 보존한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | 구조체 2개(`FWxStateTreeTask_PlayAnimation`, `...InstanceData`) 개명, `using FInstanceDataType` 갱신, 섹션·개요 주석 표기 정정, `DisplayName` → "Wx Play Animation" | 수정 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | `EnterState`/`Tick`/`GetDescription` 정의의 구조체명 개명, 섹션 헤더 주석·Description 문자열 정정 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 StructRedirect 2건(태스크 + InstanceData) | 수정 |
| `Plugins/WxWorld/README.md` (L28) | 태스크 목록 표기 `PlaySkeletalAnim` → `PlayAnimation` | 수정 |

### 접근 방식
- **순수 개명**: 시그니처·동작·필드 불변, 구조체명과 그 참조(주석 포함)만 일괄 개명.
- **에셋 참조 보존**: StateTree는 노드를 구조체 path name으로 참조하므로 구이름→새이름 StructRedirect 2건 추가(태스크 노드와 인스턴스 데이터가 각각 직렬화됨). USTRUCT path는 `F` 접두를 떼므로 `/Script/WxWorld.WxStateTreeTask_PlaySkeletalAnim[InstanceData]` 형태.
- **DisplayName 동반 변경**: 에디터 표기를 C++ 이름과 일치시킴. path name 기반 참조 해석엔 무영향.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | 구조체 2개 개명, `using FInstanceDataType` 갱신, 개요·섹션 주석 표기 정정, `DisplayName` → "Wx Play Animation" | 수정 |
| `WxGimmickStateTreeNodes.cpp` | `EnterState`/`Tick`/`GetDescription` 정의 개명, 섹션 헤더 주석·Description 문자열 "Wx Play Animation"으로 정정 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 StructRedirect 2건(태스크 + InstanceData) | 수정 |
| `Plugins/WxWorld/README.md` | 태스크 목록 표기 `PlaySkeletalAnim` → `PlayAnimation` | 수정 |
| `WxTreasureChest.h` / `.cpp` | 옛 DisplayName("Wx Play Skeletal Anim")을 가리키던 주석 2곳을 "Wx Play Animation"으로 정정 | 수정 |

### 구현·결정과 그 이유
- **순수 개명**: 시그니처·동작·인스턴스 데이터 필드는 그대로 두고 구조체명과 그 참조(`using`·cpp 정의·주석)만 일괄 바꿨다. 인자 추가는 사용자 요청에 따라 제외했다.
- **에셋 참조 보존(CoreRedirect)**: StateTree 에셋은 노드를 구조체 path name으로 참조하므로 개명만 하면 `ST_Elevator`/트레저체스트 ST의 노드 참조가 끊긴다. 태스크 구조체와 인스턴스 데이터 구조체가 각각 별도로 직렬화되므로 두 건 모두 리다이렉트를 걸었다. 이전 worklog가 "이름 보존"으로 회피했던 그 참조를 이번엔 리다이렉트로 보존한 셈이다.
- **DisplayName 동반 변경**: 에디터 표기를 C++ 이름과 일치시키려 DisplayName·Description 문자열도 바꿨다. 표기는 path name 기반 참조 해석에 영향이 없어 에셋 호환을 깨지 않는다. 같은 표기를 가리키던 트레저체스트 주석도 함께 정정했다.

### 계획 대비 달라진 점
- 트레저체스트(`.h`/`.cpp`)에 옛 DisplayName을 가리키는 주석 2곳이 더 있어 정정 범위에 포함했다(계획엔 노드 파일·설정·README만 명시).

### 후속 과제
- **에디터 검증(사용자)**: `ST_Elevator`·트레저체스트 ST를 열어 해당 노드가 리다이렉트로 보존되고 "Wx Play Animation"으로 표시되는지 확인. 빌드 검증으로 코드 개명 정합성은 확보됨.
