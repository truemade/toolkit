# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

import bpy

#
# Add additional functions here
#

class TS_OT_lightmap_op(bpy.types.Operator):
    bl_label = 'Apply light map'
    bl_idname = 'ts.lightmap_op'
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

def register():
    bpy.utils.register_class(TS_OT_lightmap_op)

def unregister():
    bpy.utils.unregister_class(TS_OT_lightmap_op)