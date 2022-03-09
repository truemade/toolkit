import bpy
import os
import subprocess
import glob
from zipfile import ZipFile
from mathutils import Vector
from platform import system
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

###########################################################################
# TODO Change to add-on
###########################################################################
# bl_info = {
#     "name": "True Skate Export",
#     "blender": (2, 80, 0),
#     "category": "Object",
# }

###########################################################################
# file path of the running script to call fbx2ts located inside the same folder
###########################################################################
# script_filepath = os.path.realpath(__file__)
script_filepath = bpy.context.space_data.text.filepath

###########################################################################
# helper functions for transfering values between Custom Properties and Blender shader node settings. (Custom properties survive export and import from fbx)
###########################################################################
def rbg2argb(v):
    return (v[0], v[1], v[2], 1.0)

def arbg2rgb(v):
    return (v[0], v[1], v[2])

###########################################################################
# fix UV map order
###########################################################################
def make_active(uvs, name):
    for uv in uvs:
        if uv.name == name:
            uvs.active = uv
            return
    print("Could not find:", name, "\n(this should never happen)")

def move_to_bottom(uvs, index):
    uvs.active_index = index
    new_name = uvs.active.name

    bpy.ops.mesh.uv_texture_add()

    # delete the "old" one
    make_active(uvs, new_name)
    bpy.ops.mesh.uv_texture_remove()

    # set the name of the last one
    uvs.active_index = len(uvs) - 1
    uvs.active.name = new_name

def fixUVOrder(object):
    uvs = object.data.uv_layers
    uvs.active_index = 0
    if uvs.active.name.lower() != "map1":
        orig_ind = uvs.active_index
        if orig_ind == len(uvs) - 1:
            return
        # use "trick" on the one after it
        move_to_bottom(uvs, orig_ind + 1)
        # use the "trick" on the UV map
        move_to_bottom(uvs, orig_ind)
        # use the "trick" on the rest that are after where it was
        for i in range(orig_ind, len(uvs) - 2):
            move_to_bottom(uvs, orig_ind)

# textures
def setTexture(material, strTextureName:str, strTexture:str):
    image = None
    try:
        image = bpy.data.images.load(filepath="//"+strTexture, check_existing = True)
    except:
        try:
            image = bpy.data.images.load(filepath="//Texture/"+bpy.path.basename(strTexture), check_existing = True)
        except:
            image = None
    if image:
        image.colorspace_settings.name = 'Raw'        
        material.node_tree.nodes[strTextureName].image = image
    
def setTextureFromCustomProperties(material, strTextureName:str, strFfxName:str):
    strTexture = material[strFfxName]
    setTexture(material, strTextureName, strTexture)    
    
def setEnvTextureFromCustomProperties(material, strTextureName:str, strFfxName:str):
    string = material[strFfxName]+".eq.png" 
    if string.endswith('.dds'):
        string = string + ".eq.png" # blender requires a equirectancular image
    setTexture(material, strTextureName, string)    
    
def setCustomPropertiesFromTexture(material, strTextureName:str, strFfxName:str):
    string = bpy.path.relpath(material.node_tree.nodes[strTextureName].image.filepath)  
    if string.startswith('//'):
        string = string[2:]
    material[strFfxName] = string   
    
def setCustomPropertiesFromEnvTexture(material, strTextureName:str, strFfxName:str):
    string = bpy.path.relpath(material.node_tree.nodes[strTextureName].image.filepath);
    if string.endswith('.eq.png'):
        string = string[:-7]
    if string.startswith('//'):
        string = string[2:]
    material[strFfxName] = string    
    
# Colour
def shaderTech2Color(shaderTech2, strTo:str, material, strFrom:str):
    shaderTech2.inputs[strTo].default_value = rbg2argb(material[strFrom])
    
def CustomPropertyColor(shaderTech2, strFrom:str, material, strTo:str):
    material[strTo] = arbg2rgb(shaderTech2.inputs[strFrom].default_value)

# Scalar
def shaderTech2Value(shaderTech2, strTo:str, material, strFrom:str):
    shaderTech2.inputs[strTo].default_value = material[strFrom]
    
def CustomPropertyValue(shaderTech2, strFrom:str, material, strTo:str):
    material[strTo] = shaderTech2.inputs[strFrom].default_value
    
###########################################################################
# Set blender shader values from Custom Properties (Custom properties survive export and import from fbx, especially useful when transfering stuff from maya exports)
###########################################################################
def sfxToBlender(material:bpy.types.Material, bAlpha:bool):
    if bAlpha:
        shaderTech2 = material.node_tree.nodes["Shader Tech2 Alpha"]
        material.blend_method =  'HASHED'
    else:
        shaderTech2 = material.node_tree.nodes["Shader Tech2"]
        material.blend_method = 'OPAQUE'
    
    shaderTech2Color(shaderTech2, "Colour", material, "Color")
    shaderTech2Value(shaderTech2, "Specular", material, "Specular")
    
    if not bAlpha:
        shaderTech2Value(shaderTech2, "Blend Mode G", material, "BlendMode_G")
        shaderTech2Value(shaderTech2, "Ignore Base Colour G", material, "IgnoreBaseColor_G")
        shaderTech2Color(shaderTech2, "Shadow Colour G", material, "ShadowColor_G")
        shaderTech2Color(shaderTech2, "Highlight Colour G", material, "HighlightColor_G")
        shaderTech2Value(shaderTech2, "Blend Sharpness G", material, "BlendSharpness_G")
        shaderTech2Value(shaderTech2, "Blend Level G", material, "BlendLevel_G")
        shaderTech2Value(shaderTech2, "Specular G", material, "Specular_G")
        
        shaderTech2Value(shaderTech2, "Blend Mode B", material, "BlendMode_B")
        shaderTech2Value(shaderTech2, "Ignore Base Colour B", material, "IgnoreBaseColor_B")
        shaderTech2Color(shaderTech2, "Shadow Colour B", material, "ShadowColor_B")
        shaderTech2Color(shaderTech2, "Highlight Colour B", material, "HighlightColor_B")
        shaderTech2Value(shaderTech2, "Blend Sharpness B", material, "BlendSharpness_B")
        shaderTech2Value(shaderTech2, "Blend Level B", material, "BlendLevel_B")
        shaderTech2Value(shaderTech2, "Specular B", material, "Specular_B")
    
    shaderTech2.inputs["Game Lighting"].default_value = 1;

    setTextureFromCustomProperties(material, "Base Texture", "SfxTextureBase")
    if bAlpha:
        setTextureFromCustomProperties(material, "Specular Texture", "SfxTextureSpecular")
    else:
        setTextureFromCustomProperties(material, "Overlay Texture", "SfxTextureOverlay")
    setTextureFromCustomProperties(material, "Light Map Texture", "SfxTextureLightmap")
    setEnvTextureFromCustomProperties(material, "Environment Texture", "SkyboxTexture")
    
    
    if material.node_tree.nodes["Environment Texture"].image:
        shaderTech2.inputs["Game Lighting"].default_value = 1
    else:
        shaderTech2.inputs["Game Lighting"].default_value = 0
        
#end sfxToBlender

###########################################################################
# Set Custom Properties from blender shaders (Custom properties survive export and import from fbx)
###########################################################################
def BlenderToSfx(material:bpy.types.Material):  
    if not material.use_nodes:
        return
    shaderTech2 = None  
    bAlpha = False
    if material.node_tree.nodes.find("Shader Tech2") != -1:
        shaderTech2 = material.node_tree.nodes["Shader Tech2"]        
    elif material.node_tree.nodes.find("Shader Tech2 Alpha") != -1:
        shaderTech2 = material.node_tree.nodes["Shader Tech2 Alpha"]        
        bAlpha = True
    
    if not shaderTech2:
        return
            
    print('copy blender shader node settings to custom properties: ' + material.name)
                 
    
    CustomPropertyColor(shaderTech2, "Colour", material, "Color")
    CustomPropertyValue(shaderTech2, "Specular", material, "Specular")
    
    if not bAlpha:
        CustomPropertyValue(shaderTech2, "Blend Mode G", material, "BlendMode_G")
        CustomPropertyValue(shaderTech2, "Ignore Base Colour G", material, "IgnoreBaseColor_G")
        CustomPropertyColor(shaderTech2, "Shadow Colour G", material, "ShadowColor_G")
        CustomPropertyColor(shaderTech2, "Highlight Colour G", material, "HighlightColor_G")
        CustomPropertyValue(shaderTech2, "Blend Sharpness G", material, "BlendSharpness_G")
        CustomPropertyValue(shaderTech2, "Blend Level G", material, "BlendLevel_G")
        CustomPropertyValue(shaderTech2, "Specular G", material, "Specular_G")
        
        CustomPropertyValue(shaderTech2, "Blend Mode B", material, "BlendMode_B")
        CustomPropertyValue(shaderTech2, "Ignore Base Colour B", material, "IgnoreBaseColor_B")
        CustomPropertyColor(shaderTech2, "Shadow Colour B", material, "ShadowColor_B")
        CustomPropertyColor(shaderTech2, "Highlight Colour B", material, "HighlightColor_B")
        CustomPropertyValue(shaderTech2, "Blend Sharpness B", material, "BlendSharpness_B")
        CustomPropertyValue(shaderTech2, "Blend Level B", material, "BlendLevel_B")
        CustomPropertyValue(shaderTech2, "Specular B", material, "Specular_B")
    
    setCustomPropertiesFromTexture(material, "Base Texture", "SfxTextureBase")
    if bAlpha:
        setCustomPropertiesFromTexture(material, "Specular Texture", "SfxTextureSpecular")
    else:
        setCustomPropertiesFromTexture(material, "Overlay Texture", "SfxTextureOverlay")
    setCustomPropertiesFromTexture(material, "Light Map Texture", "SfxTextureLightmap")
    setCustomPropertiesFromEnvTexture(material, "Environment Texture", "SkyboxTexture")
    #end if
#end BlenderToSfx
  
###########################################################################
# Overrite the material with a Shader Tec2 material for True Skate, copying across any Custom Properties to the shader nodes
###########################################################################
def SetMaterialToShaderTech2(material:bpy.types.Material, bAlpha:bool):
     
    material.node_tree.nodes.clear()    
    
    group = material.node_tree.nodes.new("ShaderNodeGroup")
    group.location = Vector((1000.0, 0.0))     
    if bAlpha:
        group.name = "Shader Tech2 Alpha"
        group.label = "Shader Tech2 Alpha"   
        group.node_tree = bpy.data.node_groups['Shader Tech2 Alpha']
    else:
        group.name = "Shader Tech2"
        group.label = "Shader Tech2"   
        group.node_tree = bpy.data.node_groups['Shader Tech2']
    
    
    imageBase = material.node_tree.nodes.new("ShaderNodeTexImage")
    imageBase.name = "Base Texture"
    imageBase.label = "Base Texture"        
    imageBase.location = Vector((500.0, 0.0))
    material.node_tree.links.new(group.inputs['Base Texture'], imageBase.outputs['Color'])
    
    if bAlpha:
        material.node_tree.links.new(group.inputs['Alpha'], imageBase.outputs['Alpha'])
    
    imageOverlayOrSpecular = material.node_tree.nodes.new("ShaderNodeTexImage")
    imageOverlayOrSpecular.location = Vector((500.0, -250.0))
    if bAlpha:
        imageOverlayOrSpecular.name = "Specular Texture"
        imageOverlayOrSpecular.label = "Specular Texture"        
        material.node_tree.links.new(group.inputs['Specular Texture'], imageOverlayOrSpecular.outputs['Color'])
    else:
        imageOverlayOrSpecular.name = "Overlay Texture"
        imageOverlayOrSpecular.label = "Overlay Texture"        
        material.node_tree.links.new(group.inputs['Overlay Texture'], imageOverlayOrSpecular.outputs['Color'])
        
    imageLightMap = material.node_tree.nodes.new("ShaderNodeTexImage")
    imageLightMap.name = "Light Map Texture"
    imageLightMap.label = "Light Map  Texture"        
    imageLightMap.location = Vector((500.0, -500.0))
    material.node_tree.links.new(group.inputs['Light Map Texture'], imageLightMap.outputs['Color'])

    imageEnvironment = material.node_tree.nodes.new("ShaderNodeTexEnvironment")
    imageEnvironment.name = "Environment Texture"
    imageEnvironment.label = "Environment Texture"        
    imageEnvironment.location = Vector((500.0, -750.0))
    material.node_tree.links.new(group.inputs['Environment Texture'], imageEnvironment.outputs['Color'])
    
    uvMapBase = material.node_tree.nodes.new("ShaderNodeUVMap")
    uvMapBase.location = Vector((0.0, 0.0))
    uvMapBase.name = "uv Base Map"
    uvMapBase.label = "uv Base Map"   
    uvMapBase.uv_map = "map1"; # taken from what havasu used, could be a more descriptive name     
    material.node_tree.links.new(imageBase.inputs['Vector'], uvMapBase.outputs['UV'])
    material.node_tree.links.new(imageOverlayOrSpecular.inputs['Vector'], uvMapBase.outputs['UV'])
    
    if not bAlpha:
        vertexColour = material.node_tree.nodes.new("ShaderNodeVertexColor")
        vertexColour.location = Vector((0.0, -150.0))
        vertexColour.name = "Vertex Colour"
        vertexColour.label = "Vertex Colour"
        vertexColour.layer_name = "colorSet"; # taken from what havasu used, could be a more descriptive name
        material.node_tree.links.new(group.inputs['VertexColour'], vertexColour.outputs['Color'])
        
    uvMapLightmap = material.node_tree.nodes.new("ShaderNodeUVMap")
    uvMapLightmap.location = Vector((0.0, -300.0))
    uvMapLightmap.name = "uv Light Map"
    uvMapLightmap.label = "uv uvMapLightmap"        
    uvMapLightmap.uv_map = "UVMap"; # taken from what havasu used, could be a more descriptive name
    material.node_tree.links.new(imageLightMap.inputs['Vector'], uvMapLightmap.outputs['UV'])
    
    if not bAlpha:
        vertexColour2 = material.node_tree.nodes.new("ShaderNodeVertexColor")
        vertexColour2.location = Vector((0.0, -450.0))
        vertexColour2.name = "Vertex Colour2"
        vertexColour2.label = "Vertex Colour2"
        vertexColour2.layer_name = "colorSet1"; # taken from what havasu used, could be a more descriptive name
        material.node_tree.links.new(group.inputs['VertexColour2'], vertexColour2.outputs['Color'])
    
        
    reflectionGroup = material.node_tree.nodes.new("ShaderNodeGroup")
    reflectionGroup.name = "Reflection"
    reflectionGroup.label = "Reflection"   
    reflectionGroup.location = Vector((0.0, -600.0))
    reflectionGroup.node_tree = bpy.data.node_groups['GetReflectionVector']
    material.node_tree.links.new(imageEnvironment.inputs['Vector'], reflectionGroup.outputs['Reflection'])
    
    
    materialOutput = material.node_tree.nodes.new("ShaderNodeOutputMaterial")
    materialOutput.location = Vector((1400.0, 0.0))
    material.node_tree.links.new(materialOutput.inputs['Surface'], group.outputs['Surface'])


    sfxToBlender(material, bAlpha)
#end SetMaterialToShaderTech2
    
###########################################################################
# Copy all the Shader Node settings to custom properties, then export to fbx (Custom properties survive export and import from fbx, especially useful when transfering stuff from maya exports)
###########################################################################
def exportScene(context, strFilepath:str, bExportSeletedOnly:bool):    
    if bExportSeletedOnly:
        for object in bpy.context.selected_objects:
            for materialSlot in object.material_slots:
                BlenderToSfx(materialSlot.material) 
    else:
        for object in bpy.data.objects :
            for materialSlot in object.material_slots:
                BlenderToSfx(materialSlot.material) 
      
    # export fbx                
    bpy.ops.export_scene.fbx(filepath = strFilepath, use_custom_props=True, use_selection = bExportSeletedOnly)

def createTxtFromFbx(root_filename):
    col = root_filename + "_col.fbx"
    vis = root_filename + "_vis.fbx"
    try:
        if system() == 'Windows':
            fbx2ts = os.path.dirname(script_filepath) + '\\fbxToTrueSkate'
        else:
            fbx2ts = os.path.dirname(script_filepath) + '/fbxToTrueSkate'
        subprocess.call([fbx2ts, "-vis", vis, "-col", col, "-out", root_filename + ".txt"])
    except:
        print("Failed calling")

def createZip(root_filename):
    root_folder = os.path.dirname(root_filename)
    filname = os.path.basename(root_filename)

    with ZipFile(root_filename + ".zip", 'w') as zipObj:
        zipObj.write(root_filename + ".txt", filname + ".txt")
        zipObj.write(root_folder + "/_mod.json", "_mod.json")
        if os.path.isdir(root_folder + "/textures"):
            texture_path = root_folder + "/textures"
        elif os.path.isdir(root_folder + "/Textures"):
            texture_path = root_folder + "/Textures"
        else:
            texture_path = root_folder
        for file in glob.glob(texture_path + "/*.jpg"):
            zipObj.write(file, os.path.basename(file))
        for file in glob.glob(texture_path + "/*.png"):
            zipObj.write(file, os.path.basename(file))
    zipObj.close()

def exportBatchScene(context, strFilepath:str, bExportTxt:bool, bExportZip:bool):
    scene = bpy.context.scene
    filename, fileExt = os.path.splitext(strFilepath)
    bpy.ops.object.mode_set(mode='OBJECT')
    for obj in scene.objects:
        if obj.name.lower().endswith("_vis"):
            if obj.users_collection[0].name != "Master Collection":
                bpy.context.view_layer.layer_collection.children[obj.users_collection[0].name].hide_viewport = False
            if not obj.visible_get():
                obj.hide_set(False)
            bpy.ops.object.select_all(action='DESELECT')
            bpy.context.scene.objects[obj.name].select_set(True)
            fixUVOrder(bpy.context.scene.objects[obj.name])
            for object in bpy.context.selected_objects:
                for materialSlot in object.material_slots:
                    BlenderToSfx(materialSlot.material)
            # export vis
            bpy.ops.export_scene.fbx(filepath = filename + "_vis" + fileExt, use_custom_props=True, use_selection = True)
        elif obj.name.lower().endswith("_col"):
            if obj.users_collection[0].name != "Master Collection":
                bpy.context.view_layer.layer_collection.children[obj.users_collection[0].name].hide_viewport = False
            if not obj.visible_get():
                obj.hide_set(False)
                print(obj.users_collection[0].name)
            bpy.ops.object.select_all(action='DESELECT')
            bpy.context.scene.objects[obj.name].select_set(True)
            for object in bpy.context.selected_objects:
                for materialSlot in object.material_slots:
                    BlenderToSfx(materialSlot.material)
            # export col
            bpy.ops.export_scene.fbx(filepath = filename + "_col" + fileExt, use_custom_props=True, use_selection = True)

    if bExportZip:
        createTxtFromFbx(filename)
        createZip(filename)

    if bExportTxt:
        createTxtFromFbx(filename)

###########################################################################
# User Interface
###########################################################################
class SfxToBlender(bpy.types.Operator) :
    bl_idname = "true_skate.sfx_to_blender"
    bl_label = "Set material to Tech2"
    #bl_options = {"UNDO"}

    def execute(context, event) : 
        bpy.context.scene.display_settings.display_device = 'None'
        SetMaterialToShaderTech2(bpy.context.active_object.active_material, bAlpha=False)  
        return {"FINISHED"}
    #end execute
#end class

class SfxToBlenderAlpha(bpy.types.Operator) :
    bl_idname = "true_skate.sfx_to_blender_alpha"
    bl_label = "Set material to Tech2 (Alpha)"
    #bl_options = {"UNDO"}

    def execute(context, event) : 
        bpy.context.scene.display_settings.display_device = 'None'
        
        SetMaterialToShaderTech2(bpy.context.active_object.active_material, bAlpha=True)  
        return {"FINISHED"}
    #end execute
#end class
    
class SfxToBlenderAll(bpy.types.Operator) :
    bl_idname = "true_skate.sfx_to_blender_all"
    bl_label = "Set all materias To tech2 (using custom properties)"
    #bl_options = {"UNDO"}
    bpy.context.scene.display_settings.display_device = 'None'

    def execute(context, event) : 
        for materialSlot in bpy.context.active_object.material_slots:
            if "SfxTextureOverlay" in materialSlot.material:
                SetMaterialToShaderTech2(materialSlot.material, bAlpha=False)      
            elif "SfxTextureSpecular" in materialSlot.material:   
                SetMaterialToShaderTech2(materialSlot.material, bAlpha=True)                    
        return {"FINISHED"}
    #end execute  
#end class  
    
class SfxExport(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "true_skate.export"
    bl_label = "True Skate (.fbx)"

    filename_ext = ".fbx"
    filter_glob: StringProperty(
        default="*.fbx",
        options={'HIDDEN'},
        maxlen=255,
    )

    selectedObjectsOnly: BoolProperty(
        name="Selected Objects Only",
        description="Selected Objects Only",
        default=True,
    )

    #type: EnumProperty(
    #    name="Example Enum",
    #    description="Choose between two items",
    #    items=(
    #        ('OPT_A', "First Option", "Description one"),
    #        ('OPT_B', "Second Option", "Description two"),
    #    ),
    #    default='OPT_A',
    #)

    def execute(self, context):
        exportScene(context, self.filepath, self.selectedObjectsOnly)
        return {"FINISHED"}
    #end execute
#end class

class SfxBatchExport(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "true_skate.batch_export"
    bl_label = "True Skate Batch (.fbx)"

    filename_ext = ".fbx"
    filter_glob: StringProperty(
        default="*.fbx",
        options={'HIDDEN'},
        maxlen=255,
    )

    generate_txt: BoolProperty(
        name="Generate txt file",
        description="This will convert fbx info to True Skate txt",
        default=False,
    )

    generate_zip: BoolProperty(
        name="Generate zip file",
        description="This will gather all the needed files to generate a True Skate zip",
        default=True,
    )

    def execute(self, context):
        exportBatchScene(context, self.filepath, self.generate_txt, self.generate_zip)
        return {"FINISHED"}
    #end execute
#end class

###########################################################################
# Setup
###########################################################################
def draw_export_menu(self, context):
    layout = self.layout
    layout.separator()
    layout.operator(SfxExport.bl_idname, text=SfxExport.bl_label)
    layout.operator(SfxBatchExport.bl_idname, text=SfxBatchExport.bl_label)

def draw_shader_menu(self, context):
    layout = self.layout
    layout.separator()
    layout.operator(SfxToBlender.bl_idname, text=SfxToBlender.bl_label)
    layout.operator(SfxToBlenderAlpha.bl_idname, text=SfxToBlenderAlpha.bl_label)
    
def draw_object_menu(self, context):
    layout = self.layout
    layout.separator()
    layout.operator(SfxToBlender.bl_idname, text=SfxToBlender.bl_label)
    layout.operator(SfxToBlenderAlpha.bl_idname, text=SfxToBlenderAlpha.bl_label)
    layout.operator(SfxToBlenderAll.bl_idname, text=SfxToBlenderAll.bl_label)
    
def register():
    bpy.utils.register_class(SfxToBlender)
    bpy.utils.register_class(SfxToBlenderAlpha)    
    bpy.utils.register_class(SfxToBlenderAll)
    bpy.utils.register_class(SfxExport)
    bpy.utils.register_class(SfxBatchExport)
    bpy.types.NODE_MT_context_menu.append(draw_shader_menu)
    bpy.types.VIEW3D_MT_object_context_menu.append(draw_object_menu)
    bpy.types.TOPBAR_MT_file_export.append(draw_export_menu) 

def unregister():
    bpy.utils.unregister_class(SfxToBlender)
    bpy.utils.unregister_class(SfxToBlenderAll)
    bpy.utils.unregister_class(SfxToBlenderAlpha)    
    bpy.utils.unregister_class(SfxExport)
    bpy.utils.unregister_class(SfxBatchExport)
    bpy.types.NODE_MT_context_menu.remove(draw_shader_menu)
    bpy.types.VIEW3D_MT_object_context_menu.remove(draw_object_menu)
    bpy.types.TOPBAR_MT_file_export.remove(draw_export_menu)
   
if __name__ == "__main__" :    
#    bpy.ops.script.reload()
    register()
    #bpy.ops.true_skate.export('INVOKE_DEFAULT')