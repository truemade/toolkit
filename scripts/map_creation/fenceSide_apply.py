import bpy
import re

# Create new material
def fenceSideMaterial():
    name = "fenceSide"
    mat = bpy.data.materials.get(name)
    if mat is None:
        mat = bpy.data.materials.new(name="fenceSide")
        mat.diffuse_color = (255,0,0,1)
    return mat

# Search for material specific type
pattern = re.compile(".*(transition|bowl|rail|incline|lip|vert|grindedge).*", re.IGNORECASE)
def matchMaterial(material):
    result = re.search(pattern, material)
    return result

FSMaterial = fenceSideMaterial()
FSIndex = bpy.data.materials.find(FSMaterial.name)

for ob in bpy.data.objects:
    if(ob.name.lower().endswith("_col")):
        if(ob.material_slots.get("fenceSide") is None):
            ob.data.materials.append(FSMaterial)
        FSIndex = ob.material_slots.find(FSMaterial.name)
        bpy.ops.object.mode_set(mode='OBJECT')
        for poly in ob.data.polygons:
            if(poly.normal[2]<=0.01):
                if(matchMaterial(ob.material_slots[poly.material_index].name) == None):
                    poly.material_index = FSIndex