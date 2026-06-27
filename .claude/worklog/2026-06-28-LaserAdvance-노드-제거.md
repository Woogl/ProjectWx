# Wx Laser Advance ST 노드 제거

## 계획

### 목표
사용처가 사라진 공용 기믹 ST 태스크 `FWxStateTreeTask_LaserAdvance`("Wx Laser Advance")와 이를 지탱하던 고아 필드·문서 참조를 모두 제거해 데드코드를 정리한다. 어떤 ST 에셋도 더 이상 이 노드를 author 하지 않으므로 동작 변화는 없다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | LaserAdvance 섹션·USTRUCT 2개·개요 불릿 삭제, SpawnActor 주석의 "Wx Laser Advance" 언급 일반화. `class AActor;`는 SpawnActor가 써서 유지 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | LaserAdvance `EnterState`/`Tick`/`GetDescription` 정의 삭제 | 삭제 |
| `Source/WxGame/WorldObject/WxLaserCorridor.h` | 고아 `MoveSpeed` 필드 삭제, 클래스 주석의 노드 나열에서 "Wx Laser Advance"·"전진" 제거 | 수정 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 추종 태스크 표의 `레이저 전진 | Wx Laser Advance` 행 삭제 | 수정 |
| `Plugins/WxWorld/README.md` | 노드 모음 설명 2곳의 "/레이저" 토큰 제거 | 수정 |

### 접근 방식
- **데드코드 제거**: 노드 정의(h/cpp)와 이를 가리키던 주석·문서를 제거한다. `MoveSpeed`는 `.cpp`에서 미사용이고 오직 노드 Velocity 튜닝용이라 함께 제거.
- **워크로그 불변**: `.claude/worklog/*.md`는 이력이라 손대지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | LaserAdvance 섹션·USTRUCT 2개·개요 불릿 삭제, SpawnActor 주석의 "Wx Laser Advance" 언급을 "이동 노드"로 일반화. `class AActor;`는 SpawnActor가 써서 유지 | 삭제 |
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | LaserAdvance `EnterState`/`Tick`/`GetDescription` 정의 삭제 | 삭제 |
| `Source/WxGame/WorldObject/WxLaserCorridor.h` | 고아 `MoveSpeed` 필드 삭제, 클래스 주석 노드 나열에서 "Wx Laser Advance"·"전진" 제거 | 수정 |
| `Docs/Programmer/Gimmick_State_Authority.md` | 추종 태스크 표의 `레이저 전진 | Wx Laser Advance` 행 삭제 | 수정 |
| `Plugins/WxWorld/README.md` | 노드 모음 설명 2곳의 "/레이저" 토큰 제거 | 수정 |

### 구현·결정과 그 이유
- **데드코드 일괄 제거**: 사용처가 사라진 노드라 정의(h/cpp)와 이를 가리키던 SpawnActor 주석·문서 참조를 함께 정리해 dangling 참조를 남기지 않았다.
- **`MoveSpeed` 동반 제거**: `.cpp`에서 한 번도 안 쓰였고 헤더 주석상 오직 LaserAdvance Velocity 튜닝용이었다. 노드가 사라지면 고아가 되므로 같이 제거.
- **`class AActor;` 유지**: SpawnActor가 `TSubclassOf<AActor>`/`TArray<TObjectPtr<AActor>>`로 계속 쓴다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에디터(필요 시)**: ST_LaserCorridor 에 이 노드가 남아있다면 에디터에서 제거(C++ 삭제로 빌드는 안 깨짐). 사용자 확인상 사용처 없음.

