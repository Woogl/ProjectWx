# PCG_DevScaleRef — 볼륨 범위 내 마네킹 일정 간격 배치

## 계획

### 목표

레벨 작업 시 크기 감각을 잡을 사람 기준물이 필요하다. 볼륨을 놓기만 하면 그 XY 범위 안에 마네킹을 일정 간격 격자로 지면 위에 세워주는 에디터 전용 PCG 그래프를 완성한다. 작업하다 만 `PCG_DevScaleRef`(`Get Volume Data` → `Volume Sampler` → `Debug`, 마네킹 배치 없음)를 이어서 마무리한다.

### 수정 범위

| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Content/LevelDesign/PCG/PCG_DevScaleRef.uasset` | 샘플러 교체, 지면 레이캐스트·마네킹 스폰 추가, 사문 파라미터 제거 | 수정 |
| `Content/LevelDesign/PCG/PCG_Mannequin.uasset` | 리네임 잔재 ObjectRedirector, 참조자 0건 확인 후 삭제 | 삭제 |

### 접근 방식

- **`Surface Sampler`로 배치**: 요구가 "바닥 1층 + 지면 투영"이라 `Volume Sampler`(3D)는 맞지 않다. 1층만 뽑으려면 voxel Z를 볼륨보다 크게 잡는 편법이 필요한데 복셀 정렬에 따라 층이 0개가 되거나 엉뚱한 높이에 생길 수 있고, 3D 샘플을 지면에 투영하면 같은 XY에 마네킹이 겹쳐 쌓인다. `Surface Sampler`는 정의상 2D 도메인에서 샘플링해 서피스에 투영하고 `Bounding Shape`로 범위를 제한하므로 1층·지면 투영·볼륨 범위가 노드 하나에 들어간다. `looseness = 0`이면 점이 셀 정중앙에 놓여 정확한 균등 격자가 된다.

- **지면은 `World Ray Hit Query`로**: `Get Landscape Data`가 아닌 레이캐스트인 이유는 개발 레벨이 랜드스케이프가 아니라 스태틱 메시 바닥일 가능성이 크기 때문이다. 레이 히트 쿼리는 둘 다 잡는다. 기본값이 "생성 액터 바운드에서 아래로 트레이스"라 볼륨 바운드가 곧 트레이스 범위가 된다. `bIgnorePCGHits = true`로 재생성 시 이전에 스폰된 마네킹 위에 얹히는 것을 막는다.

- **`Spawn Actor` + `SkeletalMeshActor`**: 마네킹은 스켈레탈 메시뿐이라(프로젝트·엔진 어디에도 마네킹 스태틱 메시나 BP 액터가 없다) `Static Mesh Spawner`를 못 쓴다. 사문 파라미터 `staticMesh`가 무의미한 이유이기도 하다. 템플릿 액터 편집을 열어 메시를 `SKM_Manny_Simple`로 지정한다.

- **에디터 전용은 템플릿 액터의 `bIsEditorOnlyActor`로**: 템플릿 프로퍼티가 스폰 액터에 복사되므로 마네킹이 쿡에서 통째로 빠진다. 그래프 자체에 에디터 전용 스위치가 없으니 이 지점이 요구를 만족시키는 자리다.

- **간격 환산**: `Surface Sampler`는 간격을 직접 받지 않고 `pointsPerSquaredMeter`로 받는다. `ppsm = 10000 / (간격cm)²` — 300cm → 0.1111. 나중에 헤매지 않게 이 식을 노드 코멘트로 남긴다.

```
World Ray Hit Query ──> [Surface]        Surface Sampler ──> Spawn Actor
Get Volume Data ─────> [Bounding Shape]                       (SkeletalMeshActor)
```

### 검증

C++ 변경이 없어 빌드할 것이 없다. 대신 에디터에서 실제로 돌린다. 테스트 PCGVolume을 띄워 실행하고, `Surface Sampler` 출력의 점 개수·격자 간격·바닥 밀착 여부를 데이터 뷰로 확인한 뒤 뷰포트로 눈으로 본다. 경사면에서 표면 노멀 때문에 마네킹이 누우면 `Transform Points`로 회전을 눌러 세운다(실측 후 필요할 때만). 확인 후 테스트 볼륨은 제거한다.

---

## 완료

작업 중 요구가 여러 번 바뀌어(볼륨 범위 → 랜드스케이프 전체, 간격 300cm → 10m → 100m → 50m → 100m) 최종 설계는 계획과 상당히 다르다. 계획 절은 승인 당시 기록으로 그대로 둔다.

### 수정한 파일

| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Content/LevelDesign/PCG/PCG_DevScaleRef.uasset` | `Get Landscape Data` → `Surface Sampler`(Unbounded) → `Static Mesh Spawner` 로 전면 재구성 | 수정 |
| `Content/LevelDesign/PCG/SM_Quinn_Simple.uasset` | `SKM_Quinn_Simple`을 Make Static Mesh로 변환 (사용자 생성) | 신규 |

`PCG_Mannequin.uasset`(리다이렉터)은 **삭제하지 않았다**. 아래 참조.

### 최종 구성

```
Get Landscape Data ──> [Surface] Surface Sampler (Unbounded, 100m) ──> Static Mesh Spawner
```

볼륨은 범위를 정하지 않고 그래프를 돌리는 호스트일 뿐이다. 랜드스케이프(2016m×2016m) 전체에 100m 간격 20×20 = 400개. 간격은 50m(1600개)까지 실측으로 문제없었고, 최종적으로 100m를 택했다.

### 구현·결정과 그 이유

- **스켈레탈 액터 → 스태틱 메시 ISM**: 이 작업의 가장 큰 전환. 스켈레탈 메시 액터 1600개를 깔았더니 에디터가 멈췄고, 로그에 `GPUSkinCache_UpdateSkinningBatches`와 `UpdateAllPrimitiveSceneInfos`가 프레임마다 쌓이는 것으로 원인이 확인됐다. **바인드 포즈로 서 있기만 하는 마네킹은 스켈레탈일 이유가 없고 스키닝 비용이 전부 낭비다.** 스태틱 메시로 바꾸니 액터 0개, ISM 컴포넌트 1개에 1600 인스턴스로 끝났다. 인스턴싱을 위해 PCG `Skinned Mesh Spawner`(`UInstancedSkinnedMeshComponent`)도 검토했으나, 메시마다 AnimBank 애셋을 요구하는 애니메이션 군중용 기능이라 정지한 기준물에는 과했다.

- **`Get Landscape Data`로 서피스 교체**: `Surface Sampler`의 `Unbounded`는 "서피스 전체를 덮어라"인데, `World Ray Hit Query`는 바운드가 없는 무한 서피스라 덮을 대상이 정의되지 않는다. 엔진이 "bounds must be provided"로 경고하고 0개를 뱉는다. 무제한을 쓰려면 바운드가 있는 서피스가 필수라 랜드스케이프로 교체했다. 대가로 랜드스케이프만 샘플링하므로 개발 레벨의 스태틱 메시 바닥은 무시된다.

- **`bMustOverlapSelf = false`**: `Get Landscape Data`는 새 노드일 때 이 값이 true가 기본이라 볼륨과 겹치는 프록시만 가져온다. 끄지 않으면 전체를 못 덮는다.

- **에디터 전용은 ISM 디스크립터의 `bIsEditorOnly`**: 액터가 없어졌으니 `bIsEditorOnlyActor`를 쓸 자리가 없다. 디스크립터에 같은 뜻의 플래그가 있어 그쪽에 걸었고, 생성된 ISM 컴포넌트에서 `true`로 확인했다.

- **콜리전 없음**: 기준물이 플레이어를 막으면 안 되므로 디스크립터의 `collisionProfileName`·`collisionEnabled`를 `NoCollision`으로 뒀다.

- **그림자 없음**: 기준물이 레벨 라이팅을 오염시키면 안 되므로 디스크립터의 `bCastShadow`·`bCastDynamicShadow`·`bCastStaticShadow`·`bAffectDistanceFieldLighting`를 모두 껐다.

- **Data Layer는 볼륨에 건다**: `Static Mesh Spawner`에는 `dataLayerSettings`가 없다(`Spawn Actor`엔 있다). 데이터 레이어는 액터 단위 개념인데 ISM 전환으로 액터가 사라졌기 때문이다. ISM은 볼륨 액터에 붙으므로 볼륨을 `DL_EditorOnly`(DataLayerType=Editor)에 넣어 통째로 토글되게 했다.

- **런타임 배제는 데이터 레이어가 아니라 `bIsEditorOnly`가 한다**: Editor 타입 데이터 레이어는 런타임에 존재 자체가 없어(`DataLayerUtils::ResolveRuntimeDataLayerInstanceNames`가 `IsRuntime()`인 것만 남기고 Editor 타입은 버린다) "런타임에 끄기"라는 조작이 성립하지 않는다. **그러나 액터를 쿡에서 빼주지도 않는다** — 에디터 조직·가시성 도구일 뿐이다. 실제로 마네킹을 런타임에서 빼는 것은 ISM 디스크립터의 `bIsEditorOnly`이고, 볼륨·PCG 컴포넌트 껍데기까지 빼려면 볼륨의 `bIsEditorOnlyActor`가 필요해 그것도 켰다. 진짜 런타임 토글이 필요하면 Runtime 타입 데이터 레이어여야 하지만, 개발용 기준물엔 과하다.

### 계획 대비 달라진 점

- **볼륨 범위 → 랜드스케이프 전체**: 사용자 요청. 볼륨을 키우는 것으로도 같은 효과를 낼 수 있었으나(개수는 면적/간격²이라 바운딩 방식과 무관), 볼륨 크기 조절이 번거롭다는 이유로 Unbounded를 택했다.

- **`Spawn Actor`·`World Ray Hit Query`·`Get Volume Data` 전부 제거**: 위 두 전환의 결과. `Unbounded`가 켜지면 `Surface Sampler`는 Bounding Shape 입력을 아예 읽지 않으므로(엔진 소스 511행) `Get Volume Data`는 죽은 배선이 된다.

- **메시가 `SKM_Manny_Simple` → `SM_Quinn_Simple`**: 사용자 지정. 변환 결과는 버텍스 45,993개로 원본과 정확히 일치하고 머티리얼 2개·LOD 3단계가 보존됐으며, 피벗이 발바닥(바운드 Z 0~180cm)이라 지면 포인트에 그대로 세울 수 있다.

- **`Transform Points` 불필요**: 계획에서 경사면 기울어짐 시 넣기로 했으나 회전이 단위 쿼터니언으로 나왔다. 언덕(고도 144m) 위 마네킹을 레이 트레이스로 확인한 결과 발밑 0.36cm에 지형이 있어 정확히 서 있다.

- **리다이렉터를 삭제하지 못했다**: 계획의 전제("참조자 0건")가 거짓이었다. `PCG_Mannequin`은 `LV_DevCombat`의 PCGVolume 외부 액터가 구 경로로 그래프를 물고 있어 지우면 참조가 끊긴다.

### 알아낸 함정

- **PCG 캐시는 인스턴스 서브오브젝트 변경으로 무효화되지 않는다**: 템플릿 액터의 내부 프로퍼티(메시)만 바꾸면 재생성해도 예전 액터가 그대로 남는다. 노드 파라미터를 건드려 세팅을 더럽혀야 반영된다. **`meshSelectorParameters`에서 실제로 재현됐다** — 그림자를 끈 뒤 재생성해도 ISM은 `CastShadow: true`였고, 노드 파라미터를 토글하자 반영됐다. 이 작업에서 두 번 물린 함정이라 두 스포너 노드 코멘트에 모두 남겼다.

- **`pointExtents`는 간격 노브가 아니다**: 정사각(X=Y)이면 셀 크기 식에서 약분돼 사라진다. 50→200으로 4배 키워도 간격이 1000cm 그대로임을 실측했다. `2*extents`가 셀 크기의 하한으로만 개입한다(기본 50 → 1m 하한). 다만 `bUseLegacyGridCreationMethod`를 켜면 셀 = `2*extents*(1+looseness)`라 extents가 곧 간격이 된다 — 옛 방식이고 기본은 꺼져 있다.

- **Make Static Mesh는 MCP를 끊는다**: 메시 빌드(0.47초)로 게임 스레드가 잠긴 사이 HTTP 응답이 못 나가 `socket_send_failure`로 소켓이 끊어진다. 클릭 자체는 성공하므로 재연결하면 된다.

- **액터의 `dataLayerAssets`만 세팅하면 데이터 레이어로 동작하지 않는다**: 월드에 대응하는 `DataLayerInstance`가 없으면 붕 뜬 참조라 아웃라이너에 `0 data layers`로 뜨고 토글도 안 된다. 프로퍼티 값만 읽어 성공으로 판단했다가 아웃라이너를 보고서야 알았다. 인스턴스는 Data Layers 아웃라이너에 애셋을 드래그해 만든다.

### 후속 과제

- **레벨 저장 필요**: 볼륨 배치·Data Layer 등록·`bIsEditorOnlyActor`는 레벨(액터) 쪽 변경이라 저장해야 남는다. 그래프 애셋은 저장했다.
- **`/Game/StaticMeshTest` 잔재**: Manny로 시도할 때 Make Static Mesh가 기본 이름으로 만든 것. Quinn으로 확정됐으니 삭제 대상.
- **리다이렉터 정리**: Fix Up Redirectors 후 `PCG_Mannequin`·`DL_Editor`(→`DL_EditorOnly` 리네이밍 잔재) 삭제. 외부 액터 재저장이 따르므로 레벨 저장과 함께.
- **쿡 실검증 안 함**: 런타임 배제는 `bIsEditorOnly`·`bIsEditorOnlyActor` 플래그의 의미와 소스 근거로 판단했을 뿐, 실제 패키징해서 확인하지는 않았다.
- **개발 레벨 바닥 미지원**: 랜드스케이프만 샘플링한다. 스태틱 메시 바닥 위에도 세우려면 `World Ray Hit Query`를 서피스로 되돌리고 볼륨 바운드로 제한해야 한다(Unbounded 포기).
