# Paragon/Shinbi 에셋 폐기

## 계획

### 목표
메타휴먼 전환으로 쓰임을 잃은 Shinbi 계열 콘텐츠를 저장소에서 걷어낸다. 2026-08-06 작업이 폐기를 완료로 적었으나 실제 삭제는 없었고 원본 콘텐츠 정리도 범위 밖으로 남아 있었다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/ParagonShinbi/` | 마켓플레이스 원본 전체(1268개·947.7MB) 제거 | 삭제 |
| `Content/Character/Player/ABP_Shinbi.uasset` | Shinbi 전용 애님 BP 제거 | 삭제 |
| `Content/Character/Player/HR/` | 구 Shinbi 플레이어 전용 세트 46개 제거 | 삭제 |
| `Content/DesignerTables/DT_Damage.uasset` | 죽은 행 `AM_HR_Attack_L` 제거 | 수정 |
| `Content/DesignerTables/DT_Ability.uasset` | 죽은 행 `GA_HR_Skill_1` 제거 | 수정 |
| `.claude/skills/dump-assets/dump_assets.py` | `EXCLUDED_TOP`에서 `ParagonShinbi` 제외 규칙 제거 | 수정 |
| `.claude/asset_dump/Blueprints/BP_HR_Player.json`, `.claude/asset_dump/DataAssets/ABS_HR_Player.json` | 스테일 덤프 제거 | 삭제 |

### 접근 방식
- **참조 0 확인 후 파일 시스템 삭제**: 에디터를 거치지 않고 `git rm -r`로 지운다. `Content`·`Plugins`·`Config`·`Source`를 바이너리 포함으로 스캔해 ParagonShinbi를 참조하는 에셋이 위 삭제 대상 3개뿐임을 확인했으므로, 참조 수정과 리다이렉터 정리가 필요 없다.
- **HR 세트 통째 폐기**: 구 Shinbi 플레이어가 유일한 소비자였고 지금은 어디서도 참조되지 않는다. 애님 원본은 현행 세트와 동일한 `ARPG_Samurai` 시퀀스를 `SK_Mannequin` 위에서 감싼 중복이라 남길 실익이 없다.
- **죽은 DataTable 행은 이름뿐**: 오브젝트 참조가 아닌 FName 행이라 삭제해도 경고를 유발하지 않는다. 에디터가 필요해 unreal-mcp로 처리한다.
- **검증은 빌드가 아닌 에디터 기동**: C++ 변경이 없어 컴파일로는 확인되지 않는다. 레지스트리 재생성 시 누락 참조 경고와 PIE 실동작으로 대신한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/ParagonShinbi/` | 원본 전체 1268개 제거(리타게터 포함) | 삭제 |
| `Content/Character/Player/ABP_Shinbi.uasset` | 제거 | 삭제 |
| `Content/Character/Player/HR/` | 43개 제거 | 삭제 |
| `Content/DesignerTables/DT_Damage.uasset` `DT_Ability.uasset` | 죽은 행 각 1건 제거 | 수정 |
| `.claude/asset_dump/` | 사라진 에셋의 덤프 10개 제거, DataTable 덤프 2건 동기화 | 삭제·수정 |
| `.claude/skills/dump-assets/dump_assets.py` | 제외 목록에서 폐기 폴더 항목 제거 | 수정 |

### 구현·결정과 그 이유
- **에디터를 거치지 않고 파일 시스템에서 삭제**: 사전 스캔으로 폐기 대상 바깥에서 오는 참조가 0임을 확인했으므로 참조 수정 절차가 필요 없었다. 에디터 삭제 다이얼로그를 쓰면 1300여 개를 훑느라 오래 걸리고 리다이렉터만 남는다.
- **HR 세트 통째 폐기**: 구 플레이어 세트의 몽타주는 현행 세트와 같은 원본 시퀀스를 같은 스켈레톤 위에서 감싼 사본이었다. 남겨도 중복이고 유일한 소비자가 사라져 통째로 지웠다.
- **죽은 DataTable 행 제거**: 오브젝트 참조가 아니라 이름만 남은 행이라 경고를 내지 않지만, 삭제된 몽타주·어빌리티를 가리키는 유령 데이터라 디자이너가 오인할 여지가 있어 함께 걷었다.

### 계획 대비 달라진 점
- **HR 폴더 실제 개수는 43개**(계획서의 46개는 삭제 대상이 아닌 형제 에셋까지 센 값이었다).
- **덤프 정리 범위가 늘었다**: 계획은 2개였으나 구 어빌리티 덤프 8개가 더 있어 총 10개를 지웠다. 또한 DataTable 덤프 2건은 행 제거를 반영해야 해서 함께 손봤다.
- **전체 재덤프는 하지 않았다**: 정확도에 필요한 부분만 고쳤다. 320개로 적힌 덤프 README의 개수 스탬프는 다음 `/dump-assets` 때 갱신된다.

### 검증 결과
- 에디터 타겟 빌드 성공(C++ 변경이 없어 up-to-date).
- 에디터 재기동 후 로그에 폐기 대상 관련 언급 0건 — 누락 참조·로드 실패·리다이렉터 경고 모두 없음.
- `LV_DevCombat` PIE 정상: 메타휴먼 플레이어 스폰·외형·카타나 부착·HUD 표시 확인, 액터 94개 전부 로드.
- DataTable은 저장 후 재조회로 행 제거를 확인했고, 디스크의 에셋 바이너리에서도 해당 문자열이 사라졌다.

### 후속 과제
- 로그에 남은 기존 경고는 이번 작업과 무관하다 — 사라진 네이티브 클래스를 가리키는 컴포넌트 참조가 샌드백·일반 몹·보스 BP에 남아 있다. 별도로 정리 필요.
- 권한 파일에 폐기된 경로 문자열이 죽은 항목으로 남아 있으나 무해해 두었다.
- 푸시는 지시 대기.
