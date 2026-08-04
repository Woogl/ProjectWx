# IWxSpawnableInterface → IWxSpawnable 리네임

## 계획

### 목표
`IWxSpawnableInterface`의 이름에서 중복된 `Interface`를 떼어 `IWxSpawnable`로 줄인다. `I` 접두사가 이미 인터페이스를 뜻하고, 같은 프로젝트의 다른 계약 인터페이스(`IWxInteractable`, `IWxSavable`)와 어휘를 맞추기 위해서다. 동작 변경 없는 순수 리네임이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h` → `WxSpawnable.h` | 파일명 변경, UINTERFACE·인터페이스 클래스·generated 인클루드 개명 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnableInterface.cpp` → `WxSpawnable.cpp` | 파일명 변경, 인클루드·정의 개명 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` | `MustImplement` 메타 경로 문자열 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp` | 인클루드·`ImplementsInterface`·`Cast`·경고 로그 문구 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h` | 인클루드·상속 목록·구획 주석 | 수정 |
| `Plugins/WxWorld/README.md` | 이름·경로 표기 | 수정 |
| `Docs/Programmer/Spawner_Enemy_Lifecycle.md` | 이름·링크 경로 표기 | 수정 |

### 접근 방식
- **git mv 후 심볼 치환**: 파일 이력을 보존하면서 헤더/구현 쌍을 옮기고, 참조처 5개 코드 파일의 심볼과 인클루드를 갱신한다.
- **리다이렉트 불필요**: 에셋 덤프 전수 검색 결과 이 인터페이스를 직접 구현하는 블루프린트가 없다. BP 적 캐릭터는 C++ `AWxEnemyCharacter`를 통해 상속받으므로 에셋에 클래스 참조가 남지 않아 `[CoreRedirects]` 없이 안전하다.
- **문자열 메타 주의**: 스포너의 `MustImplement`는 컴파일러가 검증하지 않는 경로 문자열이라, 빠뜨리면 조용히 필터가 풀린다. 함께 고친다.
- **기록물 보존**: worklog와 module_review 문서는 시점 스냅샷이므로 손대지 않고, 살아있는 참조 문서만 갱신한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnable.h` | `WxSpawnableInterface.h`에서 개명, UINTERFACE·인터페이스 클래스·generated 인클루드 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp` | `WxSpawnableInterface.cpp`에서 개명, 인클루드·`OnSpawnedBy` 정의 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h` | `MustImplement` 경로를 `/Script/WxWorld.WxSpawnable`로 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp` | 인클루드, `ImplementsInterface`, `Cast`, 미구현 경고 로그 문구 | 수정 |
| `Source/WxGame/Character/WxEnemyCharacter.h` | 인클루드, 상속 목록, `//~ Begin/End` 구획 주석 | 수정 |
| `Plugins/WxWorld/README.md` | 이름·경로 표기 3곳 | 수정 |
| `Docs/Programmer/Spawner_Enemy_Lifecycle.md` | 이름·링크 경로 표기 7곳 | 수정 |

### 구현·결정과 그 이유
- **리다이렉트 없이 단순 개명**: 에셋 덤프의 블루프린트 79개를 전수 검색해 이 인터페이스를 직접 구현하는 에셋이 없음을 먼저 확인했다. BP 적 캐릭터는 C++ 적 캐릭터를 통해 상속받아 에셋에 클래스 참조가 남지 않으므로, `[CoreRedirects]` 항목을 새로 만들 이유가 없었다.
- **파일도 함께 개명**: 타입 이름과 파일 이름이 어긋나면 인클루드 경로에서 옛 이름이 계속 되살아난다. `git mv`로 옮겨 이력은 보존했다.
- **에디터 메타 문자열 동반 수정**: 스포너의 `MustImplement`는 컴파일러가 검증하지 않는 경로 문자열이라, 빠뜨렸다면 빌드는 통과하면서 클래스 픽커 필터만 조용히 풀렸을 자리다.
- **기록물 비수정**: worklog와 `module_review_*`는 특정 시점의 스냅샷이라 당시 이름 그대로 두는 편이 정확하다. 리뷰 문서는 `/module-review`가 다시 쓸 때 갱신된다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음. WxEditor(Development) 빌드 성공(16 액션, `WxSpawnable.cpp`·`WxSpawner.cpp`·`WxEnemyCharacter.cpp` 재컴파일). 우려했던 옛 `generated.h` 잔재는 애초에 없었다.
