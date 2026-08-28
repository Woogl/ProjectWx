# ApplyDamage 반환값을 대미지 GE 적용 여부로

## 계획

### 목표
`UWxCombatLibrary::ApplyDamage`가 상태이상 GE 하나만 걸려도 true를 돌려준다. "대미지 피해를 입혔으면 true, 회피·적용 실패면 false"로 계약을 좁히고 주석을 맞춘다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../WxCombatLibrary.cpp` | 대미지 GE 스펙의 적용 결과만 반환 | 수정 |
| `Plugins/WxCombat/.../WxCombatLibrary.h` | `@return` 정정 | 수정 |

### 접근 방식
- **피해를 입혔다 = 대미지 GE가 필터를 통과해 실행됐다**: 엔진은 Instant GE가 실행되면 성공 표식이 켜진 핸들을, 태그 요건·면역에 걸리면 기본 핸들을 돌려준다. 퍼펙트 가드·대미지 0 히트도 GE는 실행됐으므로 true. HP 실감소까지 보려면 어트리뷰트를 전후로 읽어야 해 택하지 않는다(반환값 소비자도 없다).
- **스펙은 순서가 아니라 클래스로 지목**: 대미지 스펙이 앞에 오는 건 `MakeSpecs`의 우연이라 `IsA<UWxEffect_Damage>`로 본다.
- 회피 경로는 이미 GE를 걸기 전에 false — 변경 없음. 호출자 셋 다 반환값을 버리고 BP 참조도 없어 동작 영향 없음.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../WxCombatLibrary.cpp` | 적용 루프에서 `IsA<UWxEffect_Damage>` 스펙의 `WasSuccessfullyApplied()`만 결과로 삼음, `bAppliedAny` → `bDamageApplied` | 수정 |
| `Plugins/WxCombat/.../WxCombatLibrary.h` | `@return` — 대미지 GE 적용이면 true, 회피(DodgeSuccess)·적용 실패는 false | 수정 |

### 구현·결정과 그 이유
- **대미지 GE 실행 여부를 기준으로**: 엔진이 실행된 Instant GE에는 성공 표식이 켜진 핸들을, 필터에 걸린 GE에는 기본 핸들을 주므로 기존 판정 함수를 그대로 쓸 수 있다. 상태이상만 걸린 히트는 더 이상 true가 아니다.
- **퍼펙트 가드·대미지 0 히트는 true**: GE는 실행됐고 HP 실감소를 보려면 어트리뷰트를 전후로 읽어야 해 우회적이다. 반환값을 쓰는 곳이 없어 이 전제를 주석에 못박는 선에서 그쳤다.
- **클래스로 지목**: 대미지 스펙이 배열 앞에 오는 건 `MakeSpecs`의 구성 순서일 뿐이라 순서에 기대지 않았다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. 호출자 셋 다 반환값을 버리고 BP 참조도 없어 동작 영향 없음.
