import bpy
import logging
import re
import bmesh

def console_get():
    for area in bpy.context.screen.areas:
        if area.type == 'CONSOLE':
            for space in area.spaces:
                if space.type == 'CONSOLE':
                    return area, space
    return None, None

def console_write(text):
    area, space = console_get()
    if space is None:
        return

    context = bpy.context.copy()
    context.update(dict(
        space=space,
        area=area,
    ))
    for line in text.split("\n"):
        bpy.ops.console.scrollback_append(context, text=line, type='OUTPUT')

# Search for material specific type
pattern = re.compile(".*grindedge.*", re.IGNORECASE)
def matchMaterial(material):
    result = re.search(pattern, material)
    return result

# Triangulate selected objects (https://blender.stackexchange.com/questions/45698/triangulate-mesh-in-python)
def triangulate_edit_object(obj):
    # create list of grindedge materials that won't be triangulated
    list_index = []
    for index, mat in enumerate(obj.material_slots):
        if(matchMaterial(mat.name)):
            list_index.append(index)
    data = obj.data
    bm = bmesh.from_edit_mesh(data)
    # create list of faces of the object, that doesn't have the specific material
    list_faces = []
    for face in bm.faces:
        if(face.material_index not in list_index and len(face.verts) > 12):
            list_faces.append(face)
    bmesh.ops.triangulate(bm, faces=list_faces[:])
#    # V2.79 : bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method=0, ngon_method=0)

#    # Show the updates in the viewport
    bmesh.update_edit_mesh(data, True)

console_write("\n##### CHECKING POLYGONS WITH MORE THAN 12 FACES #####\n")

bpy.ops.object.mode_set(mode = 'EDIT')

for ob in bpy.data.objects:
    if(ob.name.lower().endswith("_col")):
        for poly in ob.data.polygons:
            if(len(poly.vertices) > 12):
                console_write("Found object: " + ob.name)
                console_write("Number of vertices: " + str(len(poly.vertices)))
                console_write("Polygon initial coordinate: " + str(ob.matrix_world @ ob.data.vertices[poly.vertices[0]].co) + "\n")
                triangulate_edit_object(ob)
console_write("Auto triangulating faces...")
