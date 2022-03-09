import bpy
import bmesh

# Create new material
def grindEdgeMaterial():
    name = "grindEdge"
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new("grindEdge")
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        bsdf = nodes.get("Principled BSDF")
        bsdf.inputs[0].default_value = (1.0,0.1,0.5,1.0)
    return mat

GEMaterial = grindEdgeMaterial()
GEIndex = bpy.data.materials.find(GEMaterial.name)

bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_mode(type="EDGE")

for ob in bpy.context.selected_objects:
    #ob = bpy.context.active_object
    bm = bmesh.from_edit_mesh(ob.data)

    if(ob.material_slots.get("grindEdge") is None):
        ob.data.materials.append(GEMaterial)

    list_edges = []
    for edge in bm.edges:
        if(edge.select == True):
            print(edge)
            list_edges.append(edge)

    geom_extrude = bmesh.ops.extrude_edge_only(bm, edges=list_edges, use_select_history=False)['geom']
    verts_extrude_b = [ele for ele in geom_extrude
                   if isinstance(ele, bmesh.types.BMVert)]
    faces_extrude_b = [ele for ele in geom_extrude
                   if isinstance(ele, bmesh.types.BMFace)]

    bmesh.ops.translate(
        bm,
        verts=verts_extrude_b,
        vec=(0.0, 0.0, 0.05))
    
    for face in faces_extrude_b:
        face.material_index = GEIndex

    bmesh.update_edit_mesh(ob.data)