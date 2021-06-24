import bpy
import logging

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

console_write("\n##### CHECKING POLYGONS WITH MORE THAN 12 FACES #####\n")

bpy.ops.object.mode_set(mode = 'EDIT')

for ob in bpy.data.objects:
    for poly in ob.data.polygons:
            if(len(poly.vertices) > 12):
                console_write("Found object: " + ob.name)
                console_write("Number of vertices: " + str(len(poly.vertices)))
                console_write("Polygon initial coordinate: " + str(ob.matrix_world @ ob.data.vertices[poly.vertices[0]].co) + "\n")