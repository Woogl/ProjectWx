# ContextEffects 제거 · 발소리 AI 소음을 WxAnimNotify_ReportNoise(WxAI)로 재구성

## 계획

### 목표
`ContextEffects`(Lyra 참고, 표면별 코스메틱을 EffectTag·데이터 주도로 트리거하는 범용 시스템)는 실사용이 전무하다 — 라이브러리 인스턴스 0개(무음), 범용 노티파이 미배치(오펀), 컴포넌트는 캐릭터에 write-only. 범용성이 값을 못 하므로 제거한다. 발소리의 실질 가치인 **AI 소음 발생**만 남겨, 발소리에 국한하지 않는 단순 노티파이 `WxAnimNotify_ReportNoise`로 새로 작성하고 **AI 도메인(WxAI)** 에 둔다(소음 청취측 `WxAIPerceptionComponent`와 동거해 emit/perceive 응집).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGame/ContextEffects/*.{h,cpp}` (6개) | Component/Library/AnimNotify_ContextEffects 전체 | 삭제 |
| `WxGame/AnimNotify/WxAnimNotify_Footstep.{h,cpp}` | 발소리 전용 노티파이 | 삭제 |
| `WxAI/.../WxAnimNotify_ReportNoise.{h,cpp}` | AI 소음 발생만 하는 단순 노티파이(HearingDistance 유지) | 신규 |
| `WxGame/Character/WxCharacterBase.{h,cpp}` | ContextEffectsComponent 전방선언·멤버·부착·include 제거 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 Footstep→ReportNoise 클래스 리다이렉트 1줄 | 수정 |
| `WxCore/.../WxGameplayTags.{h,cpp}` | `AnimNotify_Footstep` 태그 제거(코스메틱 조회 키였음) | 수정 |
| `WxGame/WxGame.Build.cs` | `Niagara`·`PhysicsCore` 제거(ContextEffects 전용이었음) | 수정 |
| `Content/Sound/DA_FootstepSoundSet.uasset` | 삭제된 클래스 참조하는 오펀 애셋 | 삭제 |
| `WxGame/README.md` | 컨텍스트 이펙트 불릿·Footstep 언급·Niagara 의존성 정리 | 수정 |

### 접근 방식
- **완전 삭제**: ContextEffects는 콘텐츠 인스턴스 0개라 런타임/애셋 손실 없이 6파일 제거. 살아남는 노티파이가 컴포넌트에 하드의존하므로 노티파이 재작성과 함께 처리.
- **소음 노티파이 신규(WxAI)**: 기존 Footstep의 `ReportNoiseEvent` 경로를 그대로 계승(거동 보존). 이름은 발소리 특정이 아닌 기능 그대로 `ReportNoise`. 의존성 Engine+AIModule은 WxAI가 이미 충족.
- **CoreRedirects 필수 짝**: 클래스 경로가 `/Script/WxGame.WxAnimNotify_Footstep` → `/Script/WxAI.WxAnimNotify_ReportNoise`로 바뀌어, 배치된 애님 9개가 깨진다. 리다이렉트 1줄로 투명 연결(HearingDistance 유지). 누락 시 AI 소음 소실.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxAI/.../WxAnimNotify_ReportNoise.{h,cpp}` | AI 소음 발생 전용 노티파이(`ReportNoiseEvent`, 서버 전용, `HearingDistance` 300cm). Public/Private 평면 배치 | 신규 |
| `WxGame/ContextEffects/*.{h,cpp}` (6개) | Component/Library/AnimNotify_ContextEffects 전체 | 삭제 |
| `WxGame/AnimNotify/WxAnimNotify_Footstep.{h,cpp}` | 발소리 전용 노티파이 | 삭제 |
| `WxGame/Character/WxCharacterBase.{h,cpp}` | `ContextEffectsComponent` 전방선언·멤버·부착·include 제거 | 수정 |
| `Config/DefaultEngine.ini` | `[CoreRedirects]`에 `WxGame.WxAnimNotify_Footstep`→`WxAI.WxAnimNotify_ReportNoise` 1줄 | 수정 |
| `WxCore/.../WxGameplayTags.{h,cpp}` | `AnimNotify_Footstep` 태그·카테고리 제거 | 수정 |
| `WxGame/WxGame.Build.cs` | `Niagara`·`PhysicsCore` 제거 | 수정 |
| `Content/Sound/DA_FootstepSoundSet.uasset` | 삭제된 클래스 참조하는 오펀 애셋 | 삭제 |
| `WxGame/README.md` | 컨텍스트 이펙트 불릿·Footstep 언급·Niagara 의존성 정리 | 수정 |

### 구현·결정과 그 이유
- **완전 삭제의 안전성 사전검증**: 콘텐츠 인스턴스 0개(무음), 컴포넌트는 캐릭터에 write-only, 범용 노티파이 미배치임을 전수 조사로 확인 후 제거 — 런타임·애셋 손실 없음.
- **소음 노티파이 신규(WxAI)**: 기존 `ReportNoiseEvent` 경로를 그대로 계승해 거동 보존. 발소리 특정 이름 대신 기능 그대로 `ReportNoise`로 명명(착지 등에도 범용). 소음 청취측 `WxAIPerceptionComponent`와 동거해 emit/perceive를 한 도메인으로 응집.
- **CoreRedirects로 애님 9개 무손실 연결**: 클래스 경로가 모듈을 넘어 바뀌므로(WxGame→WxAI) 리다이렉트 1줄로 투명 연결. `HearingDistance` 값 유지.
- **의존성 제거 입증**: `Niagara`·`PhysicsCore` 제거 후에도 WxGame이 정상 빌드·링크(둘 다 ContextEffects 전용이었음). 빌드 로그에서 새 파일 개별 컴파일·전 모듈 `Result: Succeeded` 확인.

### 계획 대비 달라진 점
- 계획대로. (승인 시점에 이미 노티파이 이동이 아닌 `WxAnimNotify_ReportNoise` 신규 작성으로 확정)

### 후속 과제
- **에디터 런타임 검증**: 로코모션 애님에서 CoreRedirects 정상 해석(로드 경고 없음)·`HearingDistance` 표시, 적 AI 청각 감지가 이관 전과 동일한지. (빌드 통과, 거동 보존 리팩터라 동일 예상)
- **애님 9개 재저장(선택)**: 재저장하면 새 클래스 경로가 각인돼 향후 CoreRedirects 라인 제거 가능.
- **WxAI README 반영**: 새 노티파이를 readme-writer로 갱신.
- **참고**: 동시에 다른 세션이 `Pattern_Phase` 제거 작업을 진행하며 `WxGameplayTags`를 공유 수정 중. 커밋 분리 시 유의.
