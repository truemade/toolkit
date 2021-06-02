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

class TS_PT_tools_panel(bpy.types.Panel):
    bl_label = "True Skate Tools"
    bl_idname = "ts.tools_panel"
    bl_region_type = 'UI'
    bl_space_type = 'VIEW_3D'
    bl_category = "TS Tools"

    def draw(self, context):
        layout = self.layout
        layout.operator('ts.lightmap_op')
        layout.operator('ts.tech2_op')

def register():
    bpy.utils.register_class(TS_PT_tools_panel)

def unregister():
    bpy.utils.unregister_class(TS_PT_tools_panel)