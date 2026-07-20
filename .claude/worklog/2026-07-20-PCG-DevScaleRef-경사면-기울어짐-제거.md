# PCG_DevScaleRef 경사면 기울어짐 제거

## 계획

### 목표
스케일 참조용 PCG 그래프 `PCG_DevScaleRef`가 뿌리는 마네킹 메시가 경사면에서 지표면을 따라 기울어진다. 경사와 무관하게 모두 수직으로 서게 한다. 위치·간격·스케일은 유지.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/LevelDesign/PCG/PCG_DevScaleRef.uasset` | Surface Sampler와 Static Mesh Spawner 사이에 Transform Points 노드 삽입·재배선 | 수정 |

### 접근 방식
- **원인**: Surface Sampler가 랜드스케이프에 점을 투영하며 각 점 회전을 지표면 노멀에 정렬한다. 샘플러에 이를 끄는 옵션이 없다.
- **해법**: 샘플러 뒤에 `Transform Points`(`bAbsoluteRotation=true`, 회전 0,0,0)를 넣어 회전을 수직으로 덮어쓴다. offset은 기본 (0,0,0) 가산, scale은 기본 (1,1,1) 곱이라 위치·스케일 불변.

```
Get Landscape Data ─▶ Surface Sampler ─▶ [Transform Points(회전 수직 리셋)] ─▶ Static Mesh Spawner
```

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/LevelDesign/PCG/PCG_DevScaleRef.uasset` | `TransformPoints_Upright`(Transform Points) 노드 추가, `bAbsoluteRotation=true`. 배선: SurfaceSampler → TransformPoints → StaticMeshSpawner | 수정 |

### 구현·결정과 그 이유
- **Surface Sampler를 안 건드리고 뒤에 노드를 붙임**: 샘플러엔 노멀 정렬을 끄는 옵션이 없다(스키마 확인). 후속 Transform Points로 회전만 덮어쓰는 게 최소·표준 방법.
- **`bAbsoluteRotation`만 세팅**: offset(기본 0 가산)·scale(기본 1 곱)은 무연산이라 위치·간격·스케일은 그대로 두고 회전만 수직으로 리셋.

### 검증
- LV_Openworld의 PCG 볼륨을 재생성 → 실행 메시지 무오류(빈 배열).
- 데이터 뷰 대조: Surface Sampler 출력 회전은 비-identity(경사 노멀로 pitch/roll 실림), Transform Points 출력은 400개 전부 `[0,0,0,1]`(수직). 노드가 실제로 기울어짐을 제거함을 확인.
- 애셋 저장 완료. (C++ 변경 없어 빌드 검증 불필요)

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음.
