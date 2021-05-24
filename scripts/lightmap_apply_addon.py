import bpy

gui_active_panel = None

bl_info = {
    "name": "True Skate Lightmap Add-on",
    "author": "True Axis, True Skate",
    "version": (1, 0, 0),
    "blender": (2, 80, 0),
    "location": "Image editor > Sidebar > TS Tools",
    "description": "Apply a single lightmap source to all materials",
    "category": "Lighting"
}

class applylightmap(bpy.types.Operator):
    bl_label = 'Apply light map'
    bl_idname = 'control.applylightmap'
    bl_description = 'Apply lightmap image to all materials'
    bl_context = 'objectmode'
    bl_options = {'REGISTER', 'INTERNAL'}

    def execute(self, context):
        scn = bpy.context.scene
        image = None
        for area in bpy.context.screen.areas:
            if(area.type == 'IMAGE_EDITOR' and area.spaces.active.image is not None):
                image = area.spaces.active.image

        if(image is not None):
            for ob in bpy.data.objects:
                print("OBJECT: " + ob.name + "\n")
                for mat_slot in ob.material_slots:
                    print("MATERIAL: " + mat_slot.name + "\n")
                    for nodes in mat_slot.material.node_tree.nodes:
                        if(nodes.name == "Light Map Texture"):
                            nodes.image = image
        return {'FINISHED'}

class VIEW3D_PT_tools_TrueSkate(bpy.types.Panel):
    bl_label = "True Skate Tools"
    bl_idname = "TS_PT_Tools_panel"
    bl_space_type = 'IMAGE_EDITOR'
    bl_region_type = 'UI'
    bl_category = "TS Tools"

    def draw(self, context):
        global gui_active_panel
        scn = bpy.context.scene
        self.layout.operator("control.applylightmap")

classes = (
    applylightmap,
    VIEW3D_PT_tools_TrueSkate,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register()