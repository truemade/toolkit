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

# Reference panel
class TS_PT_tools_panel(bpy.types.Panel):
    bl_label = "True Skate Tools"
    bl_region_type = 'UI'
    bl_category = "TS Tools"

    # creating a different panel layout for each type of space
    def custom_draw(self, context):
        pass

    def draw(self, context):
        self.custom_draw(context)

# 3DView panel
class TS_PT_3DView_panel(TS_PT_tools_panel):
    bl_idname = "ts.3dview_panel"
    bl_space_type = 'VIEW_3D'

    def custom_draw(self, context):
        layout = self.layout
        layout.operator('ts.new_tech2_op')
        layout.operator('ts.convert_to_tech2_op')
        layout.operator('ts.convert_all_to_tech2_op')

# Node editor panel
class TS_PT_nodes_panel(TS_PT_tools_panel):
    bl_idname = "ts.nodes_panel"
    bl_space_type = 'NODE_EDITOR'

    def custom_draw(self, context):
        layout = self.layout
        layout.operator('ts.lightmap_op')
        layout.operator('ts.new_tech2_op')
        layout.operator('ts.convert_to_tech2_op')
        layout.operator('ts.convert_all_to_tech2_op')

# Image editor panel
class TS_PT_images_panel(TS_PT_tools_panel):
    bl_idname = "ts.images_panel"
    bl_space_type = 'IMAGE_EDITOR'

    def custom_draw(self, context):
        layout = self.layout
        layout.operator('ts.lightmap_op')

def register():
    bpy.utils.register_class(TS_PT_3DView_panel)
    bpy.utils.register_class(TS_PT_nodes_panel)
    bpy.utils.register_class(TS_PT_images_panel)

def unregister():
    bpy.utils.unregister_class(TS_PT_3DView_panel)
    bpy.utils.unregister_class(TS_PT_nodes_panel)
    bpy.utils.unregister_class(TS_PT_images_panel)