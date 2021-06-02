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

bl_info = {
    "name": "True Skate Map Creator",
    "description": "This add-on is a helper to create True Skate custom maps",
    "author": "True Axis, True Skate, Luke Ryan, James Gale, Ricardo Nakayama",
    "version": (1, 0, 0),
    "blender": (2, 80, 0),
    "location": "Image editor > Sidebar > TS Tools",
    "category": "Import-Export"
}

import bpy
from . import addon_interface
from . import modules

def register():
    modules.lightmap_apply.register()
    modules.tech2shader_create.register()
    addon_interface.ui.register()

def unregister():
    modules.lightmap_apply.unregister()
    modules.tech2shader_create.unregister()
    addon_interface.ui.unregister()

if __name__ == '__main__':
    register()