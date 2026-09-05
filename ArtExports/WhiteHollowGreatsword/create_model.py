# Copyright Woogle. All Rights Reserved.
import bpy, bmesh, math, json
from pathlib import Path
from mathutils import Vector

OUT = Path(__file__).parent
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.scale_length = 1.0

def material(name, color, metal, rough):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Metallic'].default_value = metal
    p.inputs['Roughness'].default_value = rough
    return m

white = material('M_WxIvoryBlade', (.88,.9,.92), .32, .3)
silver = material('M_WxSilverEdge', (.69,.75,.81), .82, .23)
wrap = material('M_WxWhiteWrap', (.93,.91,.85), .0, .64)
seam = material('M_WxGripUnderlay', (.43,.47,.49), .15, .55)
parts = []

def finish(obj, mat, bevel=0):
    obj.data.materials.append(mat)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    if bevel:
        mod = obj.modifiers.new('Crafted edges', 'BEVEL')
        mod.width = bevel
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(obj)
    obj.select_set(False)
    return obj

def prism(name, outline, depth):
    n = len(outline)
    verts = [(x,y,z-.15) for y in [-depth/2,depth/2] for x,z in outline]
    faces = [tuple(reversed(range(n))), tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name,mesh)
    scene.collection.objects.link(obj)
    bm = bmesh.new(); bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh); bm.free()
    return obj

outline = [(-.057,.3),(-.095,.335),(-.083,.39),(-.075,.88),(-.062,1.17),
           (-.027,1.36),(.086,1.5),(.048,1.35),(.10,1.12),(.12,.83),
           (.145,.79),(.112,.795),(.097,.41),(.117,.345),(.045,.3)]
blade = prism('Blade',outline,.024)
finish(blade,white)
blade.data.materials.append(silver)
bpy.context.view_layer.objects.active = blade
blade.select_set(True)
bev = blade.modifiers.new('Silver cutting bevel','BEVEL')
bev.width=.009; bev.segments=1; bev.material=1
bpy.ops.object.modifier_apply(modifier=bev.name)
slot = prism('Slot cutter',[(-.05,.371),(-.045,.365),(-.02,.379),(.005,.93),(-.007,.975),(-.025,.943)],.12)
boolean = blade.modifiers.new('Real through slot','BOOLEAN')
boolean.operation='DIFFERENCE'; boolean.solver='EXACT'; boolean.object=slot
bpy.ops.object.modifier_apply(modifier=boolean.name)
bpy.data.objects.remove(slot,do_unlink=True)
bev=blade.modifiers.new('Slot lip','BEVEL'); bev.width=.0008; bev.segments=2
bpy.ops.object.modifier_apply(modifier=bev.name)
blade.select_set(False)

def cylinder(name, radius, depth, z, mat, bevel):
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=radius, depth=depth, location=(0,0,z-.15))
    obj=bpy.context.object; obj.name=name
    return finish(obj,mat,bevel)

cylinder('Grip core',.018,.266,.153,seam,.001)
cylinder('Pommel',.021,.014,.007,silver,.002)
cylinder('Pommel ivory cap',.018,.008,.018,white,.001)
cylinder('Blade collar',.023,.017,.288,silver,.002)
# A continuous closed ribbon gives the grip a shallow, visible spiral seam.
verts=[]; faces=[]; steps=600; turns=13; pitch=.258/turns
for i in range(steps+1):
    t=i/steps; a=t*turns*2*math.pi; center=.025+t*.246
    for r,dz in [(.0182,-pitch*.43),(.0196,-pitch*.43),(.0196,pitch*.43),(.0182,pitch*.43)]:
        verts.append((r*math.cos(a),r*math.sin(a),center+dz-.15))
for i in range(steps):
    for k in range(4): faces.append((4*i+k,4*i+(k+1)%4,4*(i+1)+(k+1)%4,4*(i+1)+k))
faces += [(3,2,1,0),tuple(4*steps+k for k in range(4))]
mesh=bpy.data.meshes.new('Wrap ribbon'); mesh.from_pydata(verts,[],faces); mesh.update()
obj=bpy.data.objects.new('White spiral wrap',mesh); scene.collection.objects.link(obj); finish(obj,wrap)

bpy.ops.object.select_all(action='DESELECT')
for obj in parts: obj.select_set(True)
bpy.context.view_layer.objects.active=blade
bpy.ops.object.join()
weapon=bpy.context.object; weapon.name='SM_WxWhiteHollowGreatsword_150cm'
scene.cursor.location=(0,0,0)
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.normals_make_consistent(inside=False)
bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.015)
bpy.ops.object.mode_set(mode='OBJECT')
tri=weapon.modifiers.new('Stable game triangulation','TRIANGULATE')
bpy.ops.object.modifier_apply(modifier=tri.name)
zmin=min(v.co.z for v in weapon.data.vertices)
zmax=max(v.co.z for v in weapon.data.vertices)
for v in weapon.data.vertices:
    v.co.z=(v.co.z-zmin)*1.5/(zmax-zmin)-.15
weapon.data.update()
bpy.context.view_layer.update()
bm=bmesh.new(); bm.from_mesh(weapon.data)
report={'dimensions_m':list(weapon.dimensions),'triangles':len(weapon.data.polygons),
        'non_manifold_edges':sum(not e.is_manifold for e in bm.edges),
        'uv_layers':len(weapon.data.uv_layers),'pivot':'Grip center; blade +Z; meters in Blender, FBX centimeters metadata'}
bm.free()
assert abs(weapon.dimensions.z-1.5)<.0001,report
assert report['non_manifold_edges']==0,report
hit=weapon.ray_cast(Vector((-.025,-.1,.45)),Vector((0,1,0)))[0]
report['slot_ray_clear']=not hit
assert not hit, 'Slot is not open'
fbx=OUT/'SM_WxWhiteHollowGreatsword_150cm.fbx'
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH'},
    axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
    mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)

def aim(obj, target):
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()

bpy.ops.object.camera_add(location=(1.0,-3.5,1.4))
camera=bpy.context.object; aim(camera,(0,0,.59)); camera.data.type='ORTHO'; camera.data.ortho_scale=1.77
scene.camera=camera
for name,loc,power,size in [('Key',(-1,-2,2.8),180,2),('Rim',(1,1,2),230,1.5),('Fill',(1,-1,.2),60,1.2)]:
    bpy.ops.object.light_add(type='AREA',location=loc)
    light=bpy.context.object; light.name=name; light.data.energy=power; light.data.shape='DISK'; light.data.size=size; aim(light,(0,0,.6))
scene.world.color=(.15,.15,.15)
scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=900; scene.render.resolution_y=1400; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs[0].default_value=(.055,.072,.095,1)
scene.world.node_tree.nodes['Background'].inputs[1].default_value=.5
scene.view_settings.view_transform='AgX'
bpy.ops.object.select_all(action='DESELECT'); weapon.select_set(True); bpy.context.view_layer.objects.active=weapon
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'WxWhiteHollowGreatsword.blend'))
scene.render.filepath=str(OUT/'Preview.png'); bpy.ops.render.render(write_still=True)
camera.location=(0,-4,.6); aim(camera,(0,0,.6))
scene.render.filepath=str(OUT/'Front.png'); bpy.ops.render.render(write_still=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(fbx))
imported=[o for o in scene.objects if o.type=='MESH']
assert len(imported)==1
report['fbx_roundtrip_dimensions_m']=list(imported[0].dimensions)
assert abs(imported[0].dimensions.z-1.5)<.0001
(OUT/'Validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('VALIDATION',json.dumps(report))
