# Copyright Woogle. All Rights Reserved.
# 프로젝트 에셋을 JSON 텍스트로 덤프해 .claude/asset_dump/ 에 기록한다.
#
# 실행 (셋 다 동일 동작):
#   (A) 헤드리스 커맨드릿 (기본):
#       "<UE>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Wx\Wx.uproject"
#         -run=pythonscript -script="C:/Wx/.claude/skills/dump-assets/dump_assets.py --sha=<sha> --date=<date>"
#         -EnablePlugins=PythonScriptPlugin -unattended -nosplash -nullrhi -nosound -stdout -FullStdOutLogOutput
#   (B) 에디터 기동 실행: UnrealEditor.exe <uproject> -EnablePlugins=PythonScriptPlugin -ExecutePythonScript="<이 파일> <인자>"
#   (C) 에디터 py 콘솔: py "C:/Wx/.claude/skills/dump-assets/dump_assets.py"
#
# 인자:
#   --asset=<에셋명 | /Game 경로>[,...]  지정 에셋의 JSON만 교체 (README.md는 갱신 안 함)
#   --out=<출력 루트>   (기본: <프로젝트>/.claude/asset_dump)
#   --sha=<short-sha> --date=<YYYY-MM-DD>   (README.md provenance 라인용, 오케스트레이터가 전달)
#
# 출력은 결정적이어야 한다(재실행 diff 0): 키 정렬, 에셋 경로 정렬, LF 고정, 날짜/SHA는 README.md에만.

import unreal
import json
import io
import os
import re
import sys

# ---------------------------------------------------------------------------
# 설정
# ---------------------------------------------------------------------------

# 인덱스에 포함할 콘텐츠 루트 (플러그인 Content 마운트 포함)
ROOTS = ["/Game", "/WxUI", "/WxWorld"]

# /Game 바로 아래에서 통째로 제외할 폴더 (마켓플레이스·엔진 샘플·OFPA). 인덱스에 개수만 남긴다.
EXCLUDED_TOP = {
    "ARPG_Pack", "NiagaraExamples", "Grz_HammerPack",
    "UIMaterialLab", "EasyInputPrompts", "Fab", "Nodachi_AnimSet",
    "Imortal_Loot_Drop_VFX", "Mannequins",
    "Collections", "Developers", "__ExternalActors__", "__ExternalObjects__",
}

# 본문 덤프 없이 인덱스만 수록하는 대형/저가치 클래스 — 로드 자체를 건너뛴다.
NEVER_LOAD_CLASSES = {
    "Texture2D", "TextureCube", "TextureRenderTarget2D", "Font", "FontFace",
    "StaticMesh", "SkeletalMesh", "Skeleton", "PhysicsAsset",
    "AnimSequence", "AnimMontage", "AnimBlueprint", "BlendSpace", "BlendSpace1D",
    "AimOffsetBlendSpace", "AimOffsetBlendSpace1D", "PoseAsset",
    "Material", "MaterialInstanceConstant", "MaterialFunction", "MaterialParameterCollection",
    "NiagaraSystem", "NiagaraEmitter", "NiagaraParameterCollection",
    "SoundWave", "SoundCue", "SoundClass", "SoundMix", "SoundAttenuation",
    "World", "LevelSequence", "PCGGraph", "DataLayerAsset",
    "BehaviorTree", "BlackboardData", "EnvQuery",
    "CurveFloat", "CurveVector", "CurveLinearColor",
}

MAX_DEPTH = 16

# str() 폴백 표현에 섞이는 메모리 주소는 비결정적이라 제거한다
PTR_RE = re.compile(r"\s*\(0x[0-9A-Fa-f]+\)")


def strip_ptrs(s):
    return PTR_RE.sub("", s)

CATEGORY_DIRS = {
    "datatables": "DataTables",
    "dataassets": "DataAssets",
    "statetrees": "StateTrees",
    "blueprints": "Blueprints",
    "widgets": "Widgets",
}

# ---------------------------------------------------------------------------
# 인자
# ---------------------------------------------------------------------------

def parse_args(argv):
    opts = {"asset": None, "out": None, "sha": "", "date": ""}
    for a in argv:
        m = re.match(r"^--(asset|out|sha|date)=(.*)$", a)
        if m:
            opts[m.group(1)] = m.group(2)
    asset = opts["asset"]
    opts["asset"] = set(s.strip().lower() for s in asset.split(",") if s.strip()) if asset else None
    return opts

# ---------------------------------------------------------------------------
# 범용 프로퍼티 직렬화기
# ---------------------------------------------------------------------------

def parse_prop_names(schema_json):
    # ToolsetLibrary.list_struct_properties 의 반환 JSON에서 프로퍼티 이름만 뽑는다(형태 방어적 파싱).
    data = json.loads(schema_json)
    if isinstance(data, dict):
        inner = data.get("properties")
        if isinstance(inner, dict):
            return list(inner.keys())
        if isinstance(inner, list):
            data = inner
        else:
            return list(data.keys())
    if isinstance(data, list):
        names = []
        for it in data:
            if isinstance(it, str):
                names.append(it)
            elif isinstance(it, dict) and "name" in it:
                names.append(it["name"])
        return names
    return []


def list_prop_names(ustruct, visible_only):
    lib = getattr(unreal, "ToolsetLibrary", None)
    if lib is not None:
        try:
            return parse_prop_names(lib.list_struct_properties(ustruct, visible_only))
        except Exception:
            pass
    return None  # 호출측이 dir() 폴백을 쓰게 한다


def dir_prop_pairs(obj):
    # ToolsetLibrary 부재 시 폴백: 파이썬에 노출된 리플렉션 이름을 훑어 읽히는 것만 취한다.
    pairs = []
    for name in dir(obj):
        if name.startswith("_") or name.upper() == name:
            continue
        try:
            pairs.append((name, obj.get_editor_property(name)))
        except Exception:
            continue
    return pairs


def get_prop_pairs(obj, ustruct, visible_only):
    names = list_prop_names(ustruct, visible_only)
    if names is None:
        return dir_prop_pairs(obj)
    pairs = []
    for name in names:
        try:
            pairs.append((name, obj.get_editor_property(name)))
        except Exception:
            continue
    return pairs


def serialize_value(value, root_pkg, seen, depth, visible_only):
    if depth > MAX_DEPTH:
        return "<max-depth>"
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, (unreal.Name, unreal.Text)):
        return str(value)
    if isinstance(value, unreal.EnumBase):
        try:
            return value.name
        except Exception:
            return str(value)
    if isinstance(value, (unreal.SoftObjectPath, unreal.SoftClassPath)):
        try:
            return value.export_text()
        except Exception:
            return str(value)
    if isinstance(value, unreal.Class):
        return value.get_path_name()
    if isinstance(value, unreal.Object):
        path = value.get_path_name()
        # 같은 패키지 내부의 instanced subobject 만 전개하고, 외부 참조는 경로로 축약한다.
        if root_pkg and path.startswith(root_pkg + ".") and ":" in path:
            if path in seen:
                return "<cycle:%s>" % path
            seen.add(path)
            out = {"__class__": value.get_class().get_path_name()}
            for name, v in get_prop_pairs(value, value.get_class(), visible_only):
                out[name] = serialize_value(v, root_pkg, seen, depth + 1, visible_only)
            return out
        return path
    if isinstance(value, unreal.Array):
        return [serialize_value(v, root_pkg, seen, depth + 1, visible_only) for v in value]
    if isinstance(value, unreal.Set):
        items = [serialize_value(v, root_pkg, seen, depth + 1, visible_only) for v in value]
        return sorted(items, key=lambda x: json.dumps(x, sort_keys=True, ensure_ascii=False))
    if isinstance(value, unreal.Map):
        out = {}
        for k, v in value.items():
            key = serialize_value(k, root_pkg, seen, depth + 1, visible_only)
            if isinstance(key, dict) and len(key) == 1:
                sole = next(iter(key.values()))
                if isinstance(sole, str):
                    key = sole  # 단일 필드 구조체 키(GameplayTag 등)는 그 값으로 축약
            if not isinstance(key, str):
                key = json.dumps(key, sort_keys=True, ensure_ascii=False)
            out[key] = serialize_value(v, root_pkg, seen, depth + 1, visible_only)
        return out
    if isinstance(value, unreal.StructBase):
        try:
            ustruct = type(value).static_struct()
        except Exception:
            ustruct = None
        pairs = get_prop_pairs(value, ustruct, visible_only) if ustruct is not None else []
        if pairs:
            out = {}
            for name, v in pairs:
                out[name] = serialize_value(v, root_pkg, seen, depth + 1, visible_only)
            return out
        # 스키마가 파이썬에 노출되지 않는 구조체(FInstancedStruct 등)는 텍스트 폴백
        try:
            return value.export_text()
        except Exception:
            return strip_ptrs(str(value))
    return strip_ptrs(str(value))


def serialize_object(obj, visible_only):
    root_pkg = obj.get_outermost().get_path_name()
    seen = set([obj.get_path_name()])
    out = {}
    for name, v in get_prop_pairs(obj, obj.get_class(), visible_only):
        out[name] = serialize_value(v, root_pkg, seen, 0, visible_only)
    return out

# ---------------------------------------------------------------------------
# 에셋 수집
# ---------------------------------------------------------------------------

def top_folder(package_name):
    parts = package_name.split("/")
    return parts[2] if len(parts) > 2 and parts[1] == "Game" else None


def gather_assets():
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    included = []
    excluded_counts = {}
    for root in ROOTS:
        for ad in ar.get_assets_by_path(root, recursive=True):
            pkg = str(ad.package_name)
            top = top_folder(pkg)
            if top in EXCLUDED_TOP:
                excluded_counts[top] = excluded_counts.get(top, 0) + 1
                continue
            included.append(ad)
    included.sort(key=lambda ad: (str(ad.package_name), str(ad.asset_name)))
    return included, excluded_counts


def asset_class_name(ad):
    # FTopLevelAssetPath("/Script/Engine", "DataTable") → "DataTable"
    try:
        return str(ad.asset_class_path.asset_name)
    except Exception:
        return str(ad.asset_class_path).rsplit(".", 1)[-1]


def get_tag(ad, key):
    try:
        v = ad.get_tag_value(key)
        if v in (None, "", "None"):
            return None
        # export 형식("/Script/CoreUObject.Class'/Script/....X'")이면 안쪽 경로만 취한다
        m = re.search(r"'([^']+)'", str(v))
        return m.group(1) if m else str(v)
    except Exception:
        return None

# ---------------------------------------------------------------------------
# 타입별 핸들러 — 각각 (envelope dict) 를 반환
# ---------------------------------------------------------------------------

def envelope(obj, data):
    return {
        "asset": obj.get_outermost().get_path_name(),
        "class": obj.get_class().get_path_name(),
        "data": data,
    }


# DataTable JSON 익스포터는 FText를 NSLOCTEXT("ns","key","source") 래퍼로 내보낸다 — 소스 문자열만 남긴다
LOCTEXT_RE = re.compile(r'^NSLOCTEXT\("(?:[^"\\]|\\.)*",\s*"(?:[^"\\]|\\.)*",\s*"((?:[^"\\]|\\.)*)"\)$', re.S)
INVTEXT_RE = re.compile(r'^INVTEXT\("((?:[^"\\]|\\.)*)"\)$', re.S)


def unescape_ctext(s):
    return (s.replace("\\\\", "\x00").replace('\\"', '"')
            .replace("\\r", "\r").replace("\\n", "\n").replace("\\t", "\t")
            .replace("\x00", "\\"))


def simplify_text_exports(value):
    if isinstance(value, str):
        m = LOCTEXT_RE.match(value) or INVTEXT_RE.match(value)
        return unescape_ctext(m.group(1)) if m else value
    if isinstance(value, list):
        return [simplify_text_exports(v) for v in value]
    if isinstance(value, dict):
        return dict((k, simplify_text_exports(v)) for k, v in value.items())
    return value


def dump_datatable(dt):
    rows = simplify_text_exports(json.loads(dt.export_to_json_string()))
    try:
        row_struct = dt.get_editor_property("RowStruct")
        row_struct_path = row_struct.get_path_name() if row_struct else None
    except Exception:
        row_struct_path = None
    env = envelope(dt, rows)
    env["row_struct"] = row_struct_path
    return env


def dump_dataasset(da):
    return envelope(da, serialize_object(da, False))


def node_summary(ed, node_struct):
    # FStateTreeEditorNode: 설명 텍스트 + export_text 에서 노드 클래스 경로 추출
    out = {}
    try:
        out["desc"] = str(ed.get_node_description(node_struct))
    except Exception:
        pass
    try:
        txt = node_struct.export_text()
        m = re.search(r"/Script/[\w./]+", txt)
        if m:
            out["node"] = m.group(0)
    except Exception:
        pass
    if not out:
        out["node"] = "<unknown>"
    return out


def dump_state(ed, state):
    out = {"name": str(state.get_editor_property("Name"))}
    for prop, key in (("Type", "type"), ("SelectionBehavior", "selection_behavior")):
        try:
            v = state.get_editor_property(prop)
            out[key] = v.name if isinstance(v, unreal.EnumBase) else str(v)
        except Exception:
            pass
    for prop, key in (("Tasks", "tasks"), ("EnterConditions", "enter_conditions")):
        try:
            out[key] = [node_summary(ed, n) for n in state.get_editor_property(prop)]
        except Exception:
            pass
    try:
        transitions = []
        for t in state.get_editor_property("Transitions"):
            tr = {}
            try:
                tr["trigger"] = t.get_editor_property("Trigger").name
            except Exception:
                pass
            try:
                link = serialize_value(t.get_editor_property("State"), None, set(), 0, False)
                if isinstance(link, dict):
                    link.pop("iD", None)  # 상태 링크 GUID는 노이즈
                tr["state"] = link
            except Exception:
                pass
            transitions.append(tr)
        out["transitions"] = transitions
    except Exception:
        pass
    try:
        out["children"] = [dump_state(ed, c) for c in state.get_editor_property("Children")]
    except Exception:
        pass
    return out


def dump_statetree(st):
    ed = None
    try:
        ed = unreal.StateTreeEditorData.get_editor_data(st)
    except Exception:
        pass
    if ed is None:
        try:
            ed = st.get_editor_property("EditorData")
        except Exception:
            ed = None
    if ed is None:
        return envelope(st, {"error": "editor data unavailable"})
    data = {}
    try:
        schema = ed.get_editor_property("Schema")
        data["schema"] = schema.get_class().get_path_name() if schema else None
    except Exception:
        pass
    for prop, key in (("GlobalTasks", "global_tasks"), ("Evaluators", "evaluators")):
        try:
            data[key] = [node_summary(ed, n) for n in ed.get_editor_property(prop)]
        except Exception:
            pass
    try:
        trees = []
        for sub in ed.get_editor_property("SubTrees"):
            trees.append(dump_state(ed, sub))
        data["sub_trees"] = trees
    except Exception as e:
        data["sub_trees_error"] = str(e)
    return envelope(st, data)


def load_generated_class(bp):
    path = "%s.%s_C" % (bp.get_outermost().get_path_name(), bp.get_name())
    return unreal.load_object(None, path)


def gather_components(bp):
    comps = []
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    for handle in sds.k2_gather_subobject_data_for_blueprint(bp):
        data = lib.get_data(handle)
        obj = lib.get_object(data)
        if obj is None or lib.is_actor(data):
            continue
        comps.append({"name": obj.get_name(), "class": obj.get_class().get_path_name()})
    return comps


# CDO 전체 덤프 대상: CDO 자체가 데이터인 베이스 (GAS 어빌리티/이펙트/큐)
def full_cdo_bases():
    bases = []
    for name in ("GameplayAbility", "GameplayEffect", "GameplayCueNotify_Static", "GameplayCueNotify_Actor"):
        cls = getattr(unreal, name, None)
        if cls is not None:
            bases.append(cls)
    return tuple(bases)


FULL_CDO_BASES = None  # 지연 초기화 (모듈 로드 순서 방어)


def dump_blueprint(bp):
    global FULL_CDO_BASES
    if FULL_CDO_BASES is None:
        FULL_CDO_BASES = full_cdo_bases()
    data = {}
    lib = getattr(unreal, "BlueprintEditorLibrary", None)
    if lib is not None:
        for fn, key in (("list_member_variable_names", "variables"),
                        ("list_graph_names", "graphs"),
                        ("list_event_dispatchers", "event_dispatchers")):
            try:
                data[key] = sorted(str(n) for n in getattr(lib, fn)(bp))
            except Exception:
                continue
        # BlueprintFunctionInfo 배열 — 이 BP가 실제 구현한 것만 이름으로 남긴다(미구현 상속 이벤트는 노이즈)
        for fn, key in (("list_functions", "functions"), ("list_events", "events")):
            try:
                names = []
                for it in getattr(lib, fn)(bp):
                    try:
                        if not it.get_editor_property("is_implemented"):
                            continue
                        names.append(str(it.get_editor_property("name")))
                    except Exception:
                        names.append(strip_ptrs(str(it)))
                data[key] = sorted(names)
            except Exception:
                continue
    try:
        data["components"] = gather_components(bp)
    except Exception:
        pass
    try:
        gen_class = load_generated_class(bp)
        cdo = unreal.get_default_object(gen_class)
        visible_only = not isinstance(cdo, FULL_CDO_BASES)
        data["cdo"] = serialize_object(cdo, visible_only)
    except Exception as e:
        data["cdo_error"] = str(e)
    return envelope(bp, data)


def walk_widget(w):
    node = {"name": w.get_name(), "class": w.get_class().get_path_name()}
    if isinstance(w, unreal.PanelWidget):
        node["children"] = [walk_widget(c) for c in w.get_all_children()]
    return node


def dump_widget_blueprint(wbp):
    env = dump_blueprint(wbp)
    # WidgetTree/RootWidget 프로퍼티는 protected — 로드된 위젯 서브오브젝트를 훑어 get_parent()로 트리를 재구성한다
    try:
        prefix = "%s.%s:WidgetTree." % (wbp.get_outermost().get_path_name(), wbp.get_name())
        widgets = [w for w in unreal.ObjectIterator(unreal.Widget) if w.get_path_name().startswith(prefix)]
        widgets.sort(key=lambda w: w.get_path_name())
        roots = [w for w in widgets if w.get_parent() is None]
        env["data"]["widget_tree"] = [walk_widget(r) for r in roots]
    except Exception as e:
        env["data"]["widget_tree_error"] = str(e)
    return env

# ---------------------------------------------------------------------------
# 라우팅·출력
# ---------------------------------------------------------------------------

def route(obj):
    # (카테고리 키, 핸들러). 위젯 검사가 블루프린트 검사보다 먼저여야 한다(서브클래스).
    if isinstance(obj, unreal.DataTable):
        return "datatables", dump_datatable
    if isinstance(obj, unreal.StateTree):
        return "statetrees", dump_statetree
    if isinstance(obj, unreal.WidgetBlueprint):
        return "widgets", dump_widget_blueprint
    if isinstance(obj, unreal.Blueprint):
        return "blueprints", dump_blueprint
    if isinstance(obj, unreal.DataAsset):
        return "dataassets", dump_dataasset
    return None, None


def write_json(path, payload):
    text = json.dumps(payload, indent=2, sort_keys=True, ensure_ascii=False)
    with io.open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text + "\n")


def unique_name(used, asset_name, package_name):
    if asset_name not in used:
        used.add(asset_name)
        return asset_name
    slug = package_name.strip("/").replace("/", "_")
    used.add(slug)
    return slug


def find_existing_file(cat_dir, asset_path):
    # 단일 에셋 갱신 시 기존 파일명(충돌 슬러그 포함)을 보존하려고 봉투의 asset 경로로 역탐색한다
    if not os.path.isdir(cat_dir):
        return None
    for f in sorted(os.listdir(cat_dir)):
        if not f.endswith(".json"):
            continue
        try:
            with io.open(os.path.join(cat_dir, f), encoding="utf-8") as fp:
                if json.load(fp).get("asset") == asset_path:
                    return f[:-5]
        except Exception:
            continue
    return None


def write_readme(out_root, total, sha, date):
    # 파생 가능한 정보는 싣지 않는다 — 에셋의 존재·경로는 Content/의 .uasset이 원본(SSOT)이다.
    cats = "·".join("`%s/`" % CATEGORY_DIRS[c] for c in sorted(CATEGORY_DIRS))
    lines = ["# AssetDump", ""]
    lines.append("에셋을 JSON으로 덤프한 텍스트 미러다. 에셋 내용 검색은 여기서 grep으로 한다. 갱신은 `/dump-assets`.")
    lines.append("")
    lines.append("본문 덤프는 %s에 에셋당 1파일이다. 몽타주·BehaviorTree·레벨·아트·마켓플레이스 에셋은 본문 덤프가 없다 — 에셋의 존재·경로는 `Content/`의 `.uasset`이 원본이므로 거기서 직접 찾는다." % cats)
    lines.append("")
    if sha:
        lines.append("*문서 기준 커밋 `%s` · 생성일 %s · 에셋 %d개 — `/dump-assets`로 갱신*" % (sha, date, total))
        lines.append("")
    with io.open(os.path.join(out_root, "README.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


def main():
    opts = parse_args(sys.argv[1:])
    project_dir = os.path.abspath(unreal.Paths.project_dir())
    out_root = opts["out"] or os.path.join(project_dir, ".claude", "asset_dump")
    asset_filter = opts["asset"]
    full_run = asset_filter is None

    assets, excluded_counts = gather_assets()
    unreal.log("DUMP: project assets=%d (excluded folders=%d)" % (len(assets), sum(excluded_counts.values())))

    if full_run and not os.path.isdir(out_root):
        os.makedirs(out_root)

    # 본문 덤프
    counts = {}
    errors = []
    if full_run:
        # 기존 산출물을 비워 삭제·개명된 에셋의 잔존 파일을 막는다
        for cat in CATEGORY_DIRS:
            cat_dir = os.path.join(out_root, CATEGORY_DIRS[cat])
            if os.path.isdir(cat_dir):
                for f in os.listdir(cat_dir):
                    if f.endswith(".json"):
                        os.remove(os.path.join(cat_dir, f))
            else:
                os.makedirs(cat_dir)

    used_names = {cat: set() for cat in CATEGORY_DIRS}
    matched = set()
    for ad in assets:
        pkg = str(ad.package_name)
        name = str(ad.asset_name)
        if not full_run:
            hit = {name.lower(), pkg.lower(), ("%s.%s" % (pkg, name)).lower()} & asset_filter
            if not hit:
                continue
            matched |= hit
        cls_name = asset_class_name(ad)
        if cls_name in NEVER_LOAD_CLASSES:
            if not full_run:
                errors.append("본문 덤프 대상 아님(%s): %s" % (cls_name, pkg))
            continue
        obj = unreal.load_asset(pkg)
        if obj is None:
            errors.append("load failed: %s" % pkg)
            continue
        cat, handler = route(obj)
        if cat is None:
            if not full_run:
                errors.append("본문 덤프 대상 아님(%s): %s" % (cls_name, pkg))
            continue
        try:
            payload = handler(obj)
        except Exception as e:
            errors.append("%s: %s" % (pkg, e))
            continue
        if cat in ("blueprints", "widgets"):
            # ParentClass UPROPERTY는 protected — AssetRegistry 태그로 채운다
            for tag, key in (("ParentClass", "parent"), ("NativeParentClass", "native_parent")):
                v = get_tag(ad, tag)
                if v:
                    payload["data"][key] = v
        cat_dir = os.path.join(out_root, CATEGORY_DIRS[cat])
        if not os.path.isdir(cat_dir):
            os.makedirs(cat_dir)
        if full_run:
            fname = unique_name(used_names[cat], name, pkg)
        else:
            # 단일 에셋 모드: 기존 파일명을 보존해 그 파일만 교체한다
            fname = find_existing_file(cat_dir, payload["asset"])
            if fname is None:
                fname = name
                if os.path.exists(os.path.join(cat_dir, fname + ".json")):
                    fname = pkg.strip("/").replace("/", "_")
        write_json(os.path.join(cat_dir, fname + ".json"), payload)
        counts[cat] = counts.get(cat, 0) + 1

    if not full_run:
        for miss in sorted(asset_filter - matched):
            errors.append("에셋을 찾지 못함: %s" % miss)

    for cat in sorted(counts):
        unreal.log("DUMP: %s=%d" % (cat, counts[cat]))

    # README.md(provenance)는 전체 실행에서만 재작성한다 — 단일 에셋 갱신이
    # 본문 전체가 낡은 채 신선도 기록만 앞서가게 만드는 것을 막는다.
    if full_run:
        write_readme(out_root, len(assets), opts["sha"], opts["date"])

    for msg in errors:
        unreal.log_error("DUMP: " + msg)
    unreal.log("DUMP: DONE files=%d errors=%d out=%s" % (sum(counts.values()), len(errors), out_root))


main()
