# Wx ContextEffects 시스템 도입 (오디오+VFX) + Footstep 흡수

## 계획

### 목표
애니메이션에서 표면별 코스메틱(오디오+Niagara VFX)을 태그·데이터 기반으로 트리거하는 시스템을 Lyra ContextEffects의 라이트사이징으로 자체 구축한다. Footstep을 이 위로 흡수하되 AI 소음 보고(서버 권위 게임플레이)는 분리 유지. v1은 서브시스템 없이 컴포넌트-온리(서브시스템 주 목적인 표면→태그 공유설정·async 캐시를 스코프에서 뺐고, 전역 스폰은 캐릭터 기반 v1엔 불필요).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGame/ContextEffects/WxContextEffectsLibrary.{h,cpp}` | DataAsset: `TMap<EffectTag, 표면별 {Sound,VFX}+Default>`. `ResolveEffect` | 신규 |
| `WxGame/ContextEffects/WxContextEffectsComponent.{h,cpp}` | `Libraries[]` 보유, `TriggerEffect(EffectTag, Location)`=트레이스+조회+사운드/VFX 스폰(로컬/비데디) | 신규 |
| `WxGame/ContextEffects/WxAnimNotify_ContextEffects.{h,cpp}` | 범용 노티파이. `EffectTag`→컴포넌트 위임 | 신규 |
| `WxGame/Character/WxFootstepComponent.{h,cpp}` | 소음 보고 유지 + 코스메틱을 ContextEffects로 위임. `UWxFootstepSoundSet`·`PlaySurfaceSound` 제거 | 수정 |
| `WxGame/AnimNotify/WxAnimNotify_Footstep.{h,cpp}` | `SoundSet` 제거, `HearingDistance` 유지 | 수정 |
| `WxGame/Character/WxCharacterBase.{h,cpp}` | `ContextEffectsComponent` 기본 부착 | 수정 |
| `WxCore/.../WxGameplayTags.{h,cpp}` | `Effect_Footstep`("Effect.Footstep") 네이티브 태그 | 수정 |

### 접근 방식
- **데이터 주도 + 표면 인지**: 라이브러리가 EffectTag+EPhysicalSurface로 사운드/VFX 조회. 표면 키는 EPhysicalSurface 직접(프로젝트세팅 매핑 없음).
- **컴포넌트가 실행**: 트레이스·리졸브·스폰을 컴포넌트가 소유. 서브시스템 없음(필요 시 WorldSubsystem 승격).
- **Footstep 분리 위임**: 소음=FootstepComponent(서버), 코스메틱=ContextEffectsComponent(Effect.Footstep).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGame/ContextEffects/WxContextEffectsLibrary.{h,cpp}` | DataAsset. `TMap<EffectTag, FWxContextEffectSurfaceSet(표면별 {Sound,VFX}+Default)>`. `ResolveEffect` | 신규 |
| `WxGame/ContextEffects/WxContextEffectsComponent.{h,cpp}` | `Libraries[]` 보유, `TriggerEffect(EffectTag, Location)`=하방 트레이스+표면 조회+사운드/Niagara 스폰(로컬/비데디) | 신규 |
| `WxGame/ContextEffects/WxAnimNotify_ContextEffects.{h,cpp}` | 범용 노티파이. `EffectTag`→`FindComponentByClass`→`TriggerEffect` | 신규 |
| `WxGame/AnimNotify/WxAnimNotify_Footstep.{h,cpp}` | 노티파이가 AI 소음(서버 신호) 직접 발생 + 코스메틱을 `ContextEffectsComponent->TriggerEffect(AnimNotify.Footstep)`로 위임. `HearingDistance`만 유지 | 수정 |
| `WxGame/Character/WxCharacterBase.{h,cpp}` | `ContextEffectsComponent` 기본 부착. `FootstepComponent` 부착 제거 | 수정 |
| `WxGame/Character/WxFootstepComponent.{h,cpp}` | 삭제(불필요) | 삭제 |
| `WxCore/.../WxGameplayTags.{h,cpp}` | `AnimNotify_Footstep`("AnimNotify.Footstep") 네이티브 태그 | 수정 |

### 구현·결정과 그 이유
- **범용 컴포넌트 하나로 통합(사용자 피드백)**: 애초 계획은 FootstepComponent(소음)+ContextEffectsComponent(코스메틱) 2개였으나, 발소리 코스메틱이 범용 시스템으로 이동하면 FootstepComponent에 남는 건 AI 소음 신호 하나뿐이라 과했다. FootstepComponent를 제거하고 범용 `ContextEffectsComponent`만 부착.
- **AI 소음은 노티파이가 직접(신호)**: `ReportNoiseEvent`는 fire-and-forget 스티뮬러스라 `SendGameplayEventToActor`와 동급의 '신호' → 노티파이에 둬도 우리 기준상 게임 로직 아님. 서버 전용. 코스메틱 시스템과 분리(요구 충족).
- **표면 키는 EPhysicalSurface 직접**: 프로젝트세팅 표면→태그 매핑 레이어 없이 라이브러리를 (EffectTag, 표면)으로 직접 조회 — 라이트사이징.
- **서브시스템 없음**: v1 효과가 캐릭터 지점 기반이라 컴포넌트로 충분(서브시스템 주 목적인 공유 표면매핑·async 캐시는 스코프 밖).
- **태그 `AnimNotify.Footstep`(사용자 피드백)**: `Effect.Footstep` 대신 기존 `ANS.` 컨벤션과 형제격인 `AnimNotify.` 로 직관화.

### 계획 대비 달라진 점
- FootstepComponent 유지 → **제거**(범용 컴포넌트로 통합). 소음은 컴포넌트가 아니라 노티파이가 직접 발생.
- 태그 `Effect_Footstep` → `AnimNotify_Footstep`.

### 후속 과제
- **콘텐츠 필수**: 기존 발소리 데이터를 `UWxContextEffectsLibrary`(AnimNotify.Footstep→표면별 Sound[+VFX])로 저작해 캐릭터 `ContextEffectsComponent.Libraries`에 지정. **그 전까지 발소리 무음**. (구 `UWxFootstepSoundSet` 애셋 있으면 데이터 이식 후 삭제)
- 에디터 플레이 점검: 표면별 발소리+VFX, Default 폴백, 적 AI 청각 반응, 범용 노티파이(착지 등) 동작.
- 남은 프로세스형 3종 이관: SnapToTarget, AreaAttack/SpawnProjectile, CameraMove.
