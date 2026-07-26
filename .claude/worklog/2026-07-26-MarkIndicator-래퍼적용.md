# MarkIndicator(WxUI) Target 에 UOL 래퍼 적용 — FWxActorTarget 의 WxCore 승격

## 계획

### 목표
WaitMoveToTarget 에서 검증된 단일 대상 래퍼(`FWxActorTarget`)를 MarkIndicator 에도 적용해 픽커를 복구한다. WxUI↔WxQuest 상호 참조 금지 + USTRUCT 전역 유일 제약으로 래퍼를 WxCore 로 승격한다(도메인 무관 지정 계약 타입 — WxCore 원칙 부합, 예정했던 확산 시점 도래).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../Public/WxActorTarget.h`, `WxCore.Build.cs` | 래퍼 이동(주석 유지), UniversalObjectLocator 의존성 추가 | 신규·수정 |
| `WxQuestStateTreeNodes.h` | 구조체 정의 제거 → WxCore include | 수정 |
| `WxIndicatorStateTreeNodes.h/.cpp` (WxUI) | Target 타입 교체, 접근 `.Locator` | 수정 |
| `/Game/Quest/ST_Quest` | 구조체 경로 변경(/Script/WxQuest→/Script/WxCore)으로 WaitMoveToTarget 값 재기입 + MarkIndicator 값 재기입(중첩 JSON), 컴파일·저장 | 수정(MCP) |

### 접근 방식
- 리다이렉트 대신 재기입(1개 에셋·2개 필드). AllowedClasses 필터는 현행 미적용 유지.
- 병행 세션 메모의 편집 유실 함정 반영: 저장 후 재조회로 잔존 확인을 검증 절차에 포함.
- 에디터 재시작은 All Saved 확인 후(플랜 승인에 포함).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCore/.../Public/WxActorTarget.h` | `FWxActorTarget` 이동(주석 유지, 공용화 사유 추가) | 신규 |
| `Plugins/WxCore/.../WxCore.Build.cs` | `UniversalObjectLocator` 의존성 추가(Public) | 수정 |
| `WxQuestStateTreeNodes.h` | 구조체 정의 제거 → WxCore include | 수정 |
| `WxIndicatorStateTreeNodes.h/.cpp` (WxUI) | MarkIndicator 의 Target 을 `FWxActorTarget` 으로 교체, 접근 `.Locator`, 모듈 주석 갱신 | 수정 |

### 구현·결정과 그 이유
- **에셋 재기입이 불필요해짐**: 구조체 이름("WxActorTarget")이 유지된 채 소속 모듈만 바뀌어 태그드 직렬화가 이름으로 매칭 — WaitMoveToTarget 의 기존 래퍼 값이 그대로 생존했다(로드 경고 0, 컴파일 성공). 계획의 "재기입 필요" 가정은 좋은 방향으로 빗나감.
- **MarkIndicator 에셋 작업 없음**: 그 사이 퀘스트 재구성으로 Mark Indicator 노드가 트리에서 빠져 있었다(Step1 = 목표 설정 + WaitMoveToTarget→캠프 스포너·반경 300, 제목 "Scout the Camp" 통일, Start 게이트 복구 상태). 코드가 준비됐으므로 노드를 다시 배치하면 래퍼 픽커를 즉시 쓸 수 있다.
- UI 회귀 확인: WxCore 이동 후에도 Target→Locator 행이 정상 렌더(2단 UOL 위젯).

### 계획 대비 달라진 점
- ST_Quest 재기입 단계 전체 생략(위 두 사유). 편집 유실 함정 방어(재조회)는 수행 — 배열 크기 불일치를 사전에 잡아 병행 세션의 재구성 상태를 훼손하지 않았다.

### 후속 과제
- Mark Indicator 노드를 다시 배치할 때 픽커 실사용 확인(코드·타입은 검증 동일).
