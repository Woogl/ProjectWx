# 회피 TargetData 타입 검사

## 계획

### 목표
`UWxAbility_Dodge::HandleTargetDataReceived`가 네트워크로 받은 TargetData를 타입 검사 없이 `static_cast`한다. 변조 클라이언트가 다른 파생 타입을 보내면 서버가 남의 레이아웃을 방향 벡터로 읽어 몽타주 섹션과 캐릭터 회전을 정한다. 구조체 타입을 확인한 뒤 캐스트하도록 고친다. (`module_review_WxCombat.md` 이슈 3)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp` | `HandleTargetDataReceived` 도입부를 `GetScriptStruct()` 비교 후 캐스트로 교체 | 수정 |

### 접근 방식
- **모듈 내 기존 패턴 재사용**: `WxEffect_Damage.cpp`가 EffectContext에 쓰는 `GetScriptStruct() == StaticStruct()` 확인 후 캐스트를 그대로 따른다. `FWxAbilityTargetData_Direction::GetScriptStruct()`는 이미 오버라이드돼 있다.
- **불일치 폴백은 영벡터**: `StartDodge`가 "입력 없음"으로 해석해 백스텝으로 흐르는 기존 정상 분기라 새 경로가 생기지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp` | `HandleTargetDataReceived`가 `GetScriptStruct()` 비교 후 캐스트하도록 교체 | 수정 |

### 구현·결정과 그 이유
- **모듈 내 기존 패턴 재사용**: `WxEffect_Damage.cpp`가 EffectContext에 쓰는 `GetScriptStruct() == StaticStruct()` 확인 후 캐스트를 그대로 따랐다. `static_cast`는 널만 걸러줄 뿐 타입을 보증하지 않아, 변조 클라이언트가 다른 파생 타입을 보내면 서버가 남의 레이아웃을 방향 벡터로 읽는다.
- **불일치 폴백은 영벡터**: `StartDodge`가 "입력 없음"으로 해석해 백스텝으로 흐르는 기존 정상 분기라 새 경로가 생기지 않는다. 서버가 조용히 안전한 기본 동작으로 떨어진다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 타입이 맞아도 값 위생은 검사하지 않는다. `NetSerialize`가 `Ar << Direction`으로 원시 double을 그대로 싣기 때문에(`WxAbilityTargetData_Direction.cpp:10-15`) NaN·무한대는 여전히 통과하고, `GetSafeNormal2D`도 NaN을 걸러 주지 않는다. 리뷰 제안 범위 밖이라 이번엔 손대지 않았다.
