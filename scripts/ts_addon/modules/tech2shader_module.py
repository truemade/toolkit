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

################ START OF SUPPORT FUNCTIONS ################

# Create reflection group node
def createReflectionGroupNode():
    reflection_group = bpy.data.node_groups.get("GetReflectionVector")
    if(reflection_group is None):
        reflection_group = bpy.data.node_groups.new('GetReflectionVector', 'ShaderNodeTree')
        # Input nodes
        reflection_group_input = reflection_group.nodes.new('NodeGroupInput')
        reflection_group_input.location = (-800,0)
        # Output node
        reflection_group_output = reflection_group.nodes.new('NodeGroupOutput')
        reflection_group_output.location = (0,0)
        reflection_group_output.label = "Reflection Vector"
        reflection_group.outputs.new('NodeSocketVector','Reflection')
        # Normalize node
        normalize_node = reflection_group.nodes.new(type='ShaderNodeVectorMath')
        normalize_node.label = "Normalize"
        normalize_node.operation = 'NORMALIZE'
        normalize_node.location = (-200,0)
        # Reflect node
        reflect_node = reflection_group.nodes.new(type='ShaderNodeVectorMath')
        reflect_node.label = "Reflect"
        reflect_node.operation = 'REFLECT'
        reflect_node.location = (-400,0)
        # Scale node
        scale_node = reflection_group.nodes.new(type='ShaderNodeVectorMath')
        scale_node.label = "Scale"
        scale_node.operation = 'SCALE'
        scale_node.inputs["Scale"].default_value = -1.0
        scale_node.location = (-600,+100)
        # Geometry node
        geometry_node = reflection_group.nodes.new(type='ShaderNodeNewGeometry')
        geometry_node.label = "Geometry"
        geometry_node.location = (-800,0)

        #### Internal links ####
        reflection_group.links.new(normalize_node.outputs[0], reflection_group_output.inputs[0])
        reflection_group.links.new(reflect_node.outputs[0], normalize_node.inputs[0])
        reflection_group.links.new(scale_node.outputs[0], reflect_node.inputs[0])
        reflection_group.links.new(geometry_node.outputs[1], reflect_node.inputs[1])
        reflection_group.links.new(geometry_node.outputs[4], scale_node.inputs[0])
    
    return reflection_group

# Create Mix group node
def createMixGroupNode():
    mix_group = bpy.data.node_groups.get("Mix")
    if(mix_group is None):
        mix_group = bpy.data.node_groups.new('Mix', 'ShaderNodeTree')
        # Input nodes
        mix_group_input = mix_group.nodes.new('NodeGroupInput')
        mix_group_input.location = (-800,0)
        mix_group.inputs.new('NodeSocketFloat','fraction')
        mix_group.inputs.new('NodeSocketFloat','from')
        mix_group.inputs.new('NodeSocketFloat','to')
        mix_group.inputs[0].default_value = 0.5
        mix_group.inputs[1].default_value = 0.5  
        mix_group.inputs[2].default_value = 1.0
        # Output node
        mix_group_output = mix_group.nodes.new('NodeGroupOutput')
        mix_group_output.location = (0,0)
        mix_group.outputs.new('NodeSocketFloat','Value')        
        # Add node
        add_node = mix_group.nodes.new(type='ShaderNodeMath')
        add_node.label = "Add"
        add_node.operation = 'ADD'
        add_node.location = (-200,50)
        # Multiply node
        multiply_node = mix_group.nodes.new(type='ShaderNodeMath')
        multiply_node.label = "Multiply"
        multiply_node.operation = 'MULTIPLY'
        multiply_node.location = (-400,100)
        # Subtract node
        subtract_node = mix_group.nodes.new(type='ShaderNodeMath')
        subtract_node.label = "Subtract"
        subtract_node.operation = 'SUBTRACT'
        subtract_node.location = (-600,-100)

        #### Internal links ####
        mix_group.links.new(add_node.outputs[0], mix_group_output.inputs[0])
        mix_group.links.new(mix_group_input.outputs[0], multiply_node.inputs[0])
        mix_group.links.new(mix_group_input.outputs[1], add_node.inputs[1])
        mix_group.links.new(mix_group_input.outputs[1], subtract_node.inputs[1])
        mix_group.links.new(mix_group_input.outputs[2], subtract_node.inputs[0])
        mix_group.links.new(multiply_node.outputs[0], add_node.inputs[0])
        mix_group.links.new(subtract_node.outputs[0], multiply_node.inputs[1])
    
    return mix_group

# Create Inverse group node
def createInverseGroupNode():
    inverse_group = bpy.data.node_groups.get("Inverse")
    if(inverse_group is None):
        inverse_group = bpy.data.node_groups.new('Inverse', 'ShaderNodeTree')
        # Input nodes
        inverse_group_input = inverse_group.nodes.new('NodeGroupInput')
        inverse_group_input.location = (-400,0)
        inverse_group.inputs.new('NodeSocketFloat','Value')
        inverse_group.inputs[0].default_value = 0.5
        # Output node
        inverse_group_output = inverse_group.nodes.new('NodeGroupOutput')
        inverse_group_output.location = (0,0)
        inverse_group.outputs.new('NodeSocketFloat','Value')        
        # Divide node
        divide_node = inverse_group.nodes.new(type='ShaderNodeMath')
        divide_node.label = "Invert"
        divide_node.operation = 'DIVIDE'   
        divide_node.inputs[0].default_value = 1.0
        divide_node.inputs[1].default_value = 0.5
        divide_node.location = (-200,0)

        #### Internal links ####
        inverse_group.links.new(divide_node.outputs[0], inverse_group_output.inputs[0])
        inverse_group.links.new(inverse_group_input.outputs[0], divide_node.inputs[1])

    return inverse_group

# Create Interpolate group node
def createInterpolateGroupNode():
    interpolate_group = bpy.data.node_groups.get("Interpolate")
    if(interpolate_group is None):
        interpolate_group = bpy.data.node_groups.new('Interpolate', 'ShaderNodeTree')
        # Frame
        interpolate_frame = interpolate_group.nodes.new('NodeFrame')
        interpolate_frame.label = "fInterpolate=fInterpolate*u_fBlendMult-u_fBlendOffset*fTo"
        # Input nodes
        interpolate_group_input = interpolate_group.nodes.new('NodeGroupInput')
        interpolate_group_input.location = (-800,0)
        interpolate_group.inputs.new('NodeSocketFloat','Interpolate')
        interpolate_group.inputs.new('NodeSocketFloat','BlendMult')
        interpolate_group.inputs.new('NodeSocketFloat','BlendOffset')
        interpolate_group.inputs.new('NodeSocketFloat','To')
        interpolate_group.inputs[0].default_value = 0.5
        interpolate_group.inputs[1].default_value = 0.5
        # Output node
        interpolate_group_output = interpolate_group.nodes.new('NodeGroupOutput')
        interpolate_group_output.location = (0,0)
        interpolate_group.outputs.new('NodeSocketFloat','Interpolate')
        # Subtract node
        subtract_node = interpolate_group.nodes.new(type='ShaderNodeMath')
        subtract_node.label = "Subtract"
        subtract_node.operation = 'SUBTRACT'
        subtract_node.use_clamp = True
        subtract_node.inputs[0].default_value = 0.5
        subtract_node.inputs[1].default_value = 0.5
        subtract_node.location = (-200,0)
        # Multiply 1 node
        multiply_node_1 = interpolate_group.nodes.new(type='ShaderNodeMath')
        multiply_node_1.label = "Multiply"
        multiply_node_1.operation = 'MULTIPLY'
        multiply_node_1.inputs[0].default_value = 0.5
        multiply_node_1.inputs[1].default_value = 0.5
        multiply_node_1.location = (-400,+400)
        # Multiply 2 node
        multiply_node_2 = interpolate_group.nodes.new(type='ShaderNodeMath')
        multiply_node_2.label = "Multiply"
        multiply_node_2.operation = 'MULTIPLY'
        multiply_node_2.inputs[0].default_value = 0.5
        multiply_node_2.inputs[1].default_value = 0.5
        multiply_node_2.location = (-400,-400)
        # Preshader node
        preshader_node = interpolate_group.nodes.new(type='ShaderNodeMath')
        preshader_node.label = "Preshader gotcha"
        preshader_node.operation = 'MULTIPLY'
        preshader_node.inputs[0].default_value = 0.5
        preshader_node.inputs[1].default_value = 0.5
        preshader_node.location = (-600,0)

        #### Add nodes to frame ####
        subtract_node.parent = interpolate_frame
        multiply_node_1.parent = interpolate_frame
        multiply_node_2.parent = interpolate_frame
        preshader_node.parent = interpolate_frame

        #### Internal links ####
        interpolate_group.links.new(subtract_node.outputs[0], interpolate_group_output.inputs[0])
        interpolate_group.links.new(interpolate_group_input.outputs[0], multiply_node_1.inputs[0])
        interpolate_group.links.new(interpolate_group_input.outputs[1], multiply_node_1.inputs[1])
        interpolate_group.links.new(interpolate_group_input.outputs[1], preshader_node.inputs[0])
        interpolate_group.links.new(interpolate_group_input.outputs[2], preshader_node.inputs[1])
        interpolate_group.links.new(interpolate_group_input.outputs[3], multiply_node_2.inputs[1])
        interpolate_group.links.new(multiply_node_1.outputs[0], subtract_node.inputs[0])
        interpolate_group.links.new(multiply_node_2.outputs[0], subtract_node.inputs[1])
        interpolate_group.links.new(preshader_node.outputs[0], multiply_node_2.inputs[0])
    
    return interpolate_group

# Create MaxComponent group node
def createMaxComponentGroupNode():
    maxcomponent_group = bpy.data.node_groups.get("MaxComponent")
    if(maxcomponent_group is None):
        maxcomponent_group = bpy.data.node_groups.new('MaxComponent', 'ShaderNodeTree')
        # Input nodes
        maxcomponent_group_input = maxcomponent_group.nodes.new('NodeGroupInput')
        maxcomponent_group_input.location = (-800,0)
        maxcomponent_group.inputs.new('NodeSocketColor','vector')
        maxcomponent_group.inputs[0].default_value = (1, 1, 1, 1)
        # Output node
        maxcomponent_group_output = maxcomponent_group.nodes.new('NodeGroupOutput')
        maxcomponent_group_output.location = (0,0)
        maxcomponent_group.outputs.new('NodeSocketFloat','max')        
        # Maximum 1 node
        maximum_node_1 = maxcomponent_group.nodes.new(type='ShaderNodeMath')
        maximum_node_1.label = "Maximum"
        maximum_node_1.operation = 'MAXIMUM'
        maximum_node_1.location = (-200,0)
        # Maximum 2 node
        maximum_node_2 = maxcomponent_group.nodes.new(type='ShaderNodeMath')
        maximum_node_2.label = "Maximum"
        maximum_node_2.operation = 'MAXIMUM'
        maximum_node_2.location = (-400,100)
        # Separate RGB node
        separateRGB_node = maxcomponent_group.nodes.new(type='ShaderNodeSeparateRGB')
        separateRGB_node.label = "SeparateRGB"
        separateRGB_node.location = (-600,0)

        #### Internal links ####
        maxcomponent_group.links.new(maximum_node_1.outputs[0], maxcomponent_group_output.inputs[0])
        maxcomponent_group.links.new(maxcomponent_group_input.outputs[0], separateRGB_node.inputs[0])
        maxcomponent_group.links.new(maximum_node_2.outputs[0], maximum_node_1.inputs[0])
        maxcomponent_group.links.new(separateRGB_node.outputs[2], maximum_node_1.inputs[1])
        maxcomponent_group.links.new(separateRGB_node.outputs[0], maximum_node_2.inputs[0])
        maxcomponent_group.links.new(separateRGB_node.outputs[1], maximum_node_2.inputs[1])

    return maxcomponent_group

# Create Blend1 group node
def createBlend1GroupNode():
    blend1_group = bpy.data.node_groups.get("Blend1")
    if(blend1_group is None):
        blend1_group = bpy.data.node_groups.new('Blend1', 'ShaderNodeTree')
        # Frame
        frame_1 = blend1_group.nodes.new('NodeFrame')
        frame_1.label = "v3Blend1*=mix(v3ShadowColor,v3HighlightColour,fTo)*v3ColorOveri"
        frame_2 = blend1_group.nodes.new('NodeFrame')
        frame_2.label = "v3Blend1=mix(v3Blend1, vec3(1.0, 1.0, 1.0), u_fIgnoreBase)"
        frame_3 = blend1_group.nodes.new('NodeFrame')
        frame_3.label = "v3Blend1=v3Diffuse/mix(MaxCompt(v3Diffuse),1.0,u_fBlendMode)"
        # Input nodes
        blend1_group_input = blend1_group.nodes.new('NodeGroupInput')
        blend1_group_input.location = (-1400,200)
        
        blend1_group.inputs.new('NodeSocketColor','Diffuse')
        blend1_group.inputs[0].default_value = (0.8, 0.8, 0.8, 1)
        
        blend1_group.inputs.new('NodeSocketVector','ColourOveride')
        blend1_group.inputs[1].default_value = (0.0, 0.0, 0.0)
        
        blend1_group.inputs.new('NodeSocketFloat','BlendMode')
        blend1_group.inputs[2].default_value = 0.5
        
        blend1_group.inputs.new('NodeSocketFloat','IgnoreBaseColour')
        blend1_group.inputs[3].default_value = 0.5
        
        blend1_group.inputs.new('NodeSocketColor','ShadowColour')
        blend1_group.inputs[4].default_value = (0.5, 0.5, 0.5, 1)
        
        blend1_group.inputs.new('NodeSocketColor','HighlightColour')
        blend1_group.inputs[5].default_value = (1, 1, 1, 1)
        
        blend1_group.inputs.new('NodeSocketFloat','To')
        blend1_group.inputs[6].default_value = 0.5
        # Output node
        blend1_group_output = blend1_group.nodes.new('NodeGroupOutput')
        blend1_group_output.location = (0,0)
        blend1_group.outputs.new('NodeSocketVector','Blend1')
        # Multiply 1 node
        multiply_node_1 = blend1_group.nodes.new(type='ShaderNodeVectorMath')
        multiply_node_1.label = "Multiply"
        multiply_node_1.operation = 'MULTIPLY'
        multiply_node_1.location = (-200,0)
        # Multiply 2 node
        multiply_node_2 = blend1_group.nodes.new(type='ShaderNodeVectorMath')
        multiply_node_2.label = "Multiply"
        multiply_node_2.operation = 'MULTIPLY'
        multiply_node_2.location = (-400,-200)
        # Mix Shader 1 node
        mix_shader_node_1 = blend1_group.nodes.new(type='ShaderNodeMixRGB')
        mix_shader_node_1.location = (-600,0)
        # Mix Shader 2 node
        mix_shader_node_2 = blend1_group.nodes.new(type='ShaderNodeMixRGB')
        mix_shader_node_2.location = (-400,300)
        mix_shader_node_2.inputs[2].default_value = (1, 1, 1, 1)
        # Scale node
        scale_node = blend1_group.nodes.new(type='ShaderNodeVectorMath')
        scale_node.label = "Scale"
        scale_node.operation = 'SCALE'
        scale_node.location = (-600,600)
        # Inverse Group node 
        inverse_group = createInverseGroupNode()
        inverse_node = blend1_group.nodes.new(type='ShaderNodeGroup')
        inverse_node.name = "Inverse"
        inverse_node.label = "Inverse"
        inverse_node.node_tree = bpy.data.node_groups[inverse_group.name]
        inverse_node.location = (-800,600)
        # Mix Group node
        mix_group = createMixGroupNode()
        mix_node = blend1_group.nodes.new(type='ShaderNodeGroup')
        mix_node.name = "Mix"
        mix_node.label = "Mix"
        mix_node.node_tree = bpy.data.node_groups[mix_group.name]
        mix_node.location = (-1000,600)
        # MaxComponent Group node
        maxComponent_group = createMaxComponentGroupNode()
        maxComponent_node = blend1_group.nodes.new(type='ShaderNodeGroup')
        maxComponent_node.name = "MaxComponent"
        maxComponent_node.label = "MaxComponent"
        maxComponent_node.node_tree = bpy.data.node_groups[maxComponent_group.name]
        maxComponent_node.location = (-1200,600)

        #### Add nodes to frame ####
        multiply_node_1.parent = frame_1
        multiply_node_2.parent = frame_1
        mix_shader_node_1.parent = frame_1
        
        mix_shader_node_2.parent = frame_2
        
        scale_node.parent = frame_3
        inverse_node.parent = frame_3
        mix_node.parent = frame_3
        maxComponent_node.parent = frame_3
        
        #### Internal links ####
        # Output link
        blend1_group.links.new(multiply_node_1.outputs[0], blend1_group_output.inputs[0])
        # Input link
        blend1_group.links.new(blend1_group_input.outputs[1], multiply_node_2.inputs[1])
        blend1_group.links.new(blend1_group_input.outputs[4], mix_shader_node_1.inputs[1])
        blend1_group.links.new(blend1_group_input.outputs[5], mix_shader_node_1.inputs[2])
        blend1_group.links.new(blend1_group_input.outputs[6], mix_shader_node_1.inputs[0])
        blend1_group.links.new(blend1_group_input.outputs[3], mix_shader_node_2.inputs[0])
        blend1_group.links.new(blend1_group_input.outputs[0], scale_node.inputs[0])
        blend1_group.links.new(blend1_group_input.outputs[2], mix_node.inputs[0])
        blend1_group.links.new(blend1_group_input.outputs[0], maxComponent_node.inputs[0])
        # Frame 1 link
        blend1_group.links.new(multiply_node_2.outputs[0], multiply_node_1.inputs[1])
        blend1_group.links.new(mix_shader_node_2.outputs[0], multiply_node_1.inputs[0])
        blend1_group.links.new(mix_shader_node_1.outputs[0], multiply_node_2.inputs[0])
        # Frame 2 link
        blend1_group.links.new(scale_node.outputs[0], mix_shader_node_2.inputs[1])
        # Frame 3 link
        blend1_group.links.new(inverse_node.outputs[0], scale_node.inputs["Scale"])
        blend1_group.links.new(mix_node.outputs[0], inverse_node.inputs[0])
        blend1_group.links.new(maxComponent_node.outputs[0], mix_node.inputs[1])

    return blend1_group

# Create MagicBlend group node
def createMagicBlendGroupNode():
    magicBlend_group = bpy.data.node_groups.get("MagicBlend")
    if(magicBlend_group is None):
        magicBlend_group = bpy.data.node_groups.new('MagicBlend', 'ShaderNodeTree')
        # Frame
        frame_1 = magicBlend_group.nodes.new('NodeFrame')
        frame_1.label = "Specular"
        frame_2 = magicBlend_group.nodes.new('NodeFrame')
        frame_2.label = "v3Diffuse = mix(v3Diffuse, v3Blend1, fInterpolate)"
        # Input nodes
        magicBlend_group_input = magicBlend_group.nodes.new('NodeGroupInput')
        magicBlend_group_input.location = (-1200,0)
        
        magicBlend_group.inputs.new('NodeSocketColor','Diffuse')
        magicBlend_group.inputs[0].default_value = (0.8, 0.8, 0.8, 1)

        magicBlend_group.inputs.new('NodeSocketFloat','Specular')
        magicBlend_group.inputs[1].default_value = 0.0
        
        magicBlend_group.inputs.new('NodeSocketVector','ColourOveride')
        magicBlend_group.inputs[2].default_value = (0.0, 0.0, 0.0)
        
        magicBlend_group.inputs.new('NodeSocketFloat','BlendMode')
        magicBlend_group.inputs[3].default_value = 0.5
        
        magicBlend_group.inputs.new('NodeSocketFloat','IgnoreBaseColour')
        magicBlend_group.inputs[4].default_value = 0.5
        
        magicBlend_group.inputs.new('NodeSocketColor','ShadowColour')
        magicBlend_group.inputs[5].default_value = (0.5, 0.5, 0.5, 1)
        
        magicBlend_group.inputs.new('NodeSocketColor','HighlightColour')
        magicBlend_group.inputs[6].default_value = (1, 1, 1, 1)

        magicBlend_group.inputs.new('NodeSocketFloat','Interpolate')
        magicBlend_group.inputs[7].default_value = 0.0
        
        magicBlend_group.inputs.new('NodeSocketFloat','To')
        magicBlend_group.inputs[8].default_value = 0.5
        
        magicBlend_group.inputs.new('NodeSocketFloat','BlendMult')
        magicBlend_group.inputs[9].default_value = 0.0

        magicBlend_group.inputs.new('NodeSocketFloat','BlendOffset')
        magicBlend_group.inputs[10].default_value = 0.0
        
        magicBlend_group.inputs.new('NodeSocketVector','SpecularOveride')
        magicBlend_group.inputs[11].default_value = (0.0, 0.0, 0.0)
        # Output node
        magicBlend_group_output = magicBlend_group.nodes.new('NodeGroupOutput')
        magicBlend_group_output.location = (0,0)

        magicBlend_group.outputs.new('NodeSocketColor','Diffuse')
        magicBlend_group.outputs[0].default_value = (0.8, 0.8, 0.8, 1)
        
        magicBlend_group.outputs.new('NodeSocketFloat','Specular')
        magicBlend_group.outputs[1].default_value = 0.0
        # Mix Group node
        mix_group = createMixGroupNode()
        mix_node = magicBlend_group.nodes.new(type='ShaderNodeGroup')
        mix_node.name = "Mix"
        mix_node.label = "Mix"
        mix_node.node_tree = bpy.data.node_groups[mix_group.name]
        mix_node.location = (-200,-400)
        # Multiply node
        multiply_node = magicBlend_group.nodes.new(type='ShaderNodeMath')
        multiply_node.label = "Multiply"
        multiply_node.operation = 'MULTIPLY'
        multiply_node.location = (-400,-500)
        # Dot Product node
        dotProduct_node = magicBlend_group.nodes.new(type='ShaderNodeVectorMath')
        dotProduct_node.label = "Dot Product"
        dotProduct_node.operation = 'DOT_PRODUCT'
        dotProduct_node.location = (-600,-600)
        # Mix Shader 2 node
        mix_shader_node = magicBlend_group.nodes.new(type='ShaderNodeMixRGB')
        mix_shader_node.location = (-200,400)
        # Blend1 Group node
        blend1_group = createBlend1GroupNode()
        blend1_node = magicBlend_group.nodes.new(type='ShaderNodeGroup')
        blend1_node.name = "Blend1"
        blend1_node.label = "Blend1"
        blend1_node.node_tree = bpy.data.node_groups[blend1_group.name]
        blend1_node.location = (-800,100)
        # Interpolate Group node 
        interpolate_group = createInterpolateGroupNode()
        interpolate_node = magicBlend_group.nodes.new(type='ShaderNodeGroup')
        interpolate_node.name = "Interpolate"
        interpolate_node.label = "Interpolate"
        interpolate_node.node_tree = bpy.data.node_groups[interpolate_group.name]
        interpolate_node.location = (-800,-200)

        #### Add nodes to frame ####
        mix_node.parent = frame_1
        multiply_node.parent = frame_1
        dotProduct_node.parent = frame_1
        
        mix_shader_node.parent = frame_2
        #### Internal links ####
        # Output link
        magicBlend_group.links.new(mix_node.outputs[0], magicBlend_group_output.inputs[1])
        magicBlend_group.links.new(mix_shader_node.outputs[0], magicBlend_group_output.inputs[0])
        # Input link
        magicBlend_group.links.new(magicBlend_group_input.outputs[1], mix_node.inputs[1])
        magicBlend_group.links.new(magicBlend_group_input.outputs[11], multiply_node.inputs[1])
        magicBlend_group.links.new(magicBlend_group_input.outputs[0], mix_shader_node.inputs[1])
        magicBlend_group.links.new(magicBlend_group_input.outputs[0], blend1_node.inputs[0])
        magicBlend_group.links.new(magicBlend_group_input.outputs[2], blend1_node.inputs[1])
        magicBlend_group.links.new(magicBlend_group_input.outputs[3], blend1_node.inputs[2])
        magicBlend_group.links.new(magicBlend_group_input.outputs[4], blend1_node.inputs[3])
        magicBlend_group.links.new(magicBlend_group_input.outputs[5], blend1_node.inputs[4])
        magicBlend_group.links.new(magicBlend_group_input.outputs[6], blend1_node.inputs[5])
        magicBlend_group.links.new(magicBlend_group_input.outputs[8], blend1_node.inputs[6])
        magicBlend_group.links.new(magicBlend_group_input.outputs[7], interpolate_node.inputs[0])
        magicBlend_group.links.new(magicBlend_group_input.outputs[8], interpolate_node.inputs[3])
        magicBlend_group.links.new(magicBlend_group_input.outputs[9], interpolate_node.inputs[1])
        magicBlend_group.links.new(magicBlend_group_input.outputs[10], interpolate_node.inputs[2])
        # Frame 1 link
        magicBlend_group.links.new(interpolate_node.outputs[0], mix_node.inputs[0])
        magicBlend_group.links.new(multiply_node.outputs[0], mix_node.inputs[2])
        # Operation Dot product has an issue of not showing the link if uses output 0
        # Check here https://developer.blender.org/T72901
        magicBlend_group.links.new(dotProduct_node.outputs[1], multiply_node.inputs[0])
        magicBlend_group.links.new(blend1_node.outputs[0], dotProduct_node.inputs[0])
        # Frame 2 link
        magicBlend_group.links.new(interpolate_node.outputs[0], mix_shader_node.inputs[0])
        magicBlend_group.links.new(blend1_node.outputs[0], mix_shader_node.inputs[2])

    return magicBlend_group

# Create Shader Tech2 group node
def createShaderTech2GroupNode():
    shaderTech2_group = bpy.data.node_groups.get("Shader Tech2")
    if(shaderTech2_group is None):
        shaderTech2_group = bpy.data.node_groups.new('Shader Tech2', 'ShaderNodeTree')
        # Input nodes
        shaderTech2_group_input = shaderTech2_group.nodes.new('NodeGroupInput')
        shaderTech2_group_input.location = (-2200,100)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Base Texture')
        shaderTech2_group.inputs[0].default_value = (0.8, 0.8, 0.8, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Overlay Texture')
        shaderTech2_group.inputs[1].default_value = (0.8, 0.8, 0.8, 1)

        shaderTech2_group.inputs.new('NodeSocketColor','Light Map Texture')
        shaderTech2_group.inputs[2].default_value = (0.5, 0.5, 0.5, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Environment Texture')
        shaderTech2_group.inputs[3].default_value = (0.0, 0.0, 0.0, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Colour')
        shaderTech2_group.inputs[4].default_value = (1.0, 1.0, 1.0, 1)

        shaderTech2_group.inputs.new('NodeSocketFloat','Specular')
        shaderTech2_group.inputs[5].default_value = 1.000
        shaderTech2_group.inputs[5].min_value = 0.000
        shaderTech2_group.inputs[5].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketColor','VertexColour')
        shaderTech2_group.inputs[6].default_value = (0.8, 0.8, 0.8, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','VertexColour2')
        shaderTech2_group.inputs[7].default_value = (0.0, 0.0, 0.0, 1)
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Mode G')
        shaderTech2_group.inputs[8].default_value = 0.626
        shaderTech2_group.inputs[8].min_value = 0.000
        shaderTech2_group.inputs[8].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Ignore Base Colour G')
        shaderTech2_group.inputs[9].default_value = 0.000
        shaderTech2_group.inputs[9].min_value = 0.000
        shaderTech2_group.inputs[9].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketColor','Shadow Colour G')
        shaderTech2_group.inputs[10].default_value = (0.795, 0.795, 0.795, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Highlight Colour G')
        shaderTech2_group.inputs[11].default_value = (1, 1, 1, 1)
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Sharpness G')
        shaderTech2_group.inputs[12].default_value = 5.583
        shaderTech2_group.inputs[12].min_value = 0.000
        shaderTech2_group.inputs[12].max_value = 10.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Level G')
        shaderTech2_group.inputs[13].default_value = 0.858
        shaderTech2_group.inputs[13].min_value = 0.000
        shaderTech2_group.inputs[13].max_value = 1.000

        shaderTech2_group.inputs.new('NodeSocketFloat','Specular G')
        shaderTech2_group.inputs[14].default_value = 0.342
        shaderTech2_group.inputs[14].min_value = 0.000
        shaderTech2_group.inputs[14].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Mode B')
        shaderTech2_group.inputs[15].default_value = 0.655
        shaderTech2_group.inputs[15].min_value = 0.000
        shaderTech2_group.inputs[15].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Ignore Base Colour B')
        shaderTech2_group.inputs[16].default_value = 0.023
        shaderTech2_group.inputs[16].min_value = 0.000
        shaderTech2_group.inputs[16].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketColor','Shadow Colour B')
        shaderTech2_group.inputs[17].default_value = (0.838, 0.838, 0.838, 1)
        
        shaderTech2_group.inputs.new('NodeSocketColor','Highlight Colour B')
        shaderTech2_group.inputs[18].default_value = (1, 1, 1, 1)
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Sharpness B')
        shaderTech2_group.inputs[19].default_value = 5.500
        shaderTech2_group.inputs[19].min_value = 0.000
        shaderTech2_group.inputs[19].max_value = 10.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Blend Level B')
        shaderTech2_group.inputs[20].default_value = 0.800
        shaderTech2_group.inputs[20].min_value = 0.000
        shaderTech2_group.inputs[20].max_value = 20.000

        shaderTech2_group.inputs.new('NodeSocketFloat','Specular B')
        shaderTech2_group.inputs[21].default_value = 0.372
        shaderTech2_group.inputs[21].min_value = 0.000
        shaderTech2_group.inputs[21].max_value = 1.000
        
        shaderTech2_group.inputs.new('NodeSocketFloat','Game Lighting')
        shaderTech2_group.inputs[22].default_value = 1.000
        shaderTech2_group.inputs[21].min_value = 0.000
        shaderTech2_group.inputs[21].max_value = 1.000

        # Output node
        shaderTech2_group_output = shaderTech2_group.nodes.new('NodeGroupOutput')
        shaderTech2_group_output.location = (0,0)
        shaderTech2_group.outputs.new('NodeSocketShader','Surface')
        # Mix Shader node
        mix_shader_node = shaderTech2_group.nodes.new(type='ShaderNodeMixShader')
        mix_shader_node.inputs["Fac"].default_value = 0.0
        mix_shader_node.location = (-200,0)
        # Emission node
        emission_node = shaderTech2_group.nodes.new(type='ShaderNodeEmission')
        emission_node.inputs["Strength"].default_value = 1.0
        emission_node.location = (-400,200)
        # Gamma node
        gamma_node = shaderTech2_group.nodes.new(type='ShaderNodeGamma')
        gamma_node.inputs["Gamma"].default_value = 1.0
        gamma_node.location = (-600,200)
        # Diffuse + Environment node
        diffuse_node = shaderTech2_group.nodes.new(type='ShaderNodeMixRGB')
        diffuse_node.label = "Diffuse + Environment"
        diffuse_node.blend_type = 'ADD'
        diffuse_node.inputs["Fac"].default_value = 1.0
        diffuse_node.location = (-800,200)
        # Mult Colour node
        mult_color_node = shaderTech2_group.nodes.new(type='ShaderNodeMixRGB')
        mult_color_node.label = "Mult Colour"
        mult_color_node.blend_type = 'MULTIPLY'
        mult_color_node.inputs["Fac"].default_value = 1.0
        mult_color_node.location = (-1000,200)
        # Mult Lightmap node
        mult_lightmap_node = shaderTech2_group.nodes.new(type='ShaderNodeMixRGB')
        mult_lightmap_node.label = "Mult Lightmap"
        mult_lightmap_node.blend_type = 'MULTIPLY'
        mult_lightmap_node.inputs["Fac"].default_value = 1.0
        mult_lightmap_node.location = (-1200,300)
        # Specular node
        specular_node = shaderTech2_group.nodes.new(type='ShaderNodeEeveeSpecular')
        specular_node.inputs[2].default_value = 0.178 #roughness
        specular_node.inputs["Emissive Color"].default_value = (0, 0, 0, 1)
        specular_node.inputs["Transparency"].default_value = 0.0
        specular_node.inputs["Clear Coat"].default_value = 0.0
        specular_node.inputs["Clear Coat Roughness"].default_value = 0.0
        specular_node.location = (-800,-300)
        # Extra Fudge node
        extra_fudge_node = shaderTech2_group.nodes.new(type='ShaderNodeMath')
        extra_fudge_node.label = "Extra Fudge"
        extra_fudge_node.operation = 'MULTIPLY'
        extra_fudge_node.inputs[1].default_value = 0.05
        extra_fudge_node.location = (-1000,-400)
        # Mult Specular node
        mult_specular_node = shaderTech2_group.nodes.new(type='ShaderNodeMath')
        mult_specular_node.label = "Mult Specular"
        mult_specular_node.operation = 'MULTIPLY'
        mult_specular_node.location = (-1200,-500)
        # Mult Environment node
        mult_env_node = shaderTech2_group.nodes.new(type='ShaderNodeVectorMath')
        mult_env_node.label = "Mult Environment"
        mult_env_node.operation = 'SCALE'
        mult_env_node.location = (-1000,0)
        # MagicBlend 1 node
        magicBlend_group_1 = createMagicBlendGroupNode()
        magicBlend_node_1 = shaderTech2_group.nodes.new(type='ShaderNodeGroup')
        magicBlend_node_1.name = "MagicBlend"
        magicBlend_node_1.label = "MagicBlend"
        magicBlend_node_1.node_tree = bpy.data.node_groups[magicBlend_group_1.name]
        magicBlend_node_1.location = (-1400,100)
        # MagicBlend 2 node
        magicBlend_group_2 = createMagicBlendGroupNode()
        magicBlend_node_2 = shaderTech2_group.nodes.new(type='ShaderNodeGroup')
        magicBlend_node_2.name = "MagicBlend"
        magicBlend_node_2.label = "MagicBlend"
        magicBlend_node_2.node_tree = bpy.data.node_groups[magicBlend_group_2.name]
        magicBlend_node_2.location = (-1600,600)
        # Overlay Texture node
        overlay_node = shaderTech2_group.nodes.new(type='ShaderNodeSeparateRGB')
        overlay_node.label = "Overlay Texture"
        overlay_node.inputs[0].default_value = (0.8, 0.8, 0.8, 1)
        overlay_node.location = (-1800,300)
        # VertexColour node
        vertexColour_node = shaderTech2_group.nodes.new(type='ShaderNodeSeparateRGB')
        vertexColour_node.label = "VertexColour"
        vertexColour_node.inputs[0].default_value = (0.8, 0.8, 0.8, 1)
        vertexColour_node.location = (-1800,-200)

        #### Internal links ####
        # Output link
        shaderTech2_group.links.new(mix_shader_node.outputs[0], shaderTech2_group_output.inputs[0])
        # Input link        
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[22], mix_shader_node.inputs[0])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[2], mult_lightmap_node.inputs[1])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[3], mult_env_node.inputs[0])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[4], mult_color_node.inputs[2])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[5], mult_specular_node.inputs[0])

        shaderTech2_group.links.new(shaderTech2_group_input.outputs[0], magicBlend_node_2.inputs[0])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[7], magicBlend_node_2.inputs[2])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[8], magicBlend_node_2.inputs[3])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[9], magicBlend_node_2.inputs[4])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[10], magicBlend_node_2.inputs[5])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[11], magicBlend_node_2.inputs[6])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[12], magicBlend_node_2.inputs[9])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[13], magicBlend_node_2.inputs[10])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[14], magicBlend_node_2.inputs[11])
        
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[7], magicBlend_node_1.inputs[2])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[15], magicBlend_node_1.inputs[3])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[16], magicBlend_node_1.inputs[4])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[17], magicBlend_node_1.inputs[5])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[18], magicBlend_node_1.inputs[6])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[19], magicBlend_node_1.inputs[9])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[20], magicBlend_node_1.inputs[10])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[21], magicBlend_node_1.inputs[11])

        shaderTech2_group.links.new(shaderTech2_group_input.outputs[1], overlay_node.inputs[0])
        shaderTech2_group.links.new(shaderTech2_group_input.outputs[6], vertexColour_node.inputs[0])

        # Other links
        shaderTech2_group.links.new(emission_node.outputs[0], mix_shader_node.inputs[2])
        shaderTech2_group.links.new(specular_node.outputs[0], mix_shader_node.inputs[1])
        shaderTech2_group.links.new(gamma_node.outputs[0], emission_node.inputs[0])
        shaderTech2_group.links.new(diffuse_node.outputs[0], gamma_node.inputs[0])
        shaderTech2_group.links.new(mult_color_node.outputs[0], specular_node.inputs[0])
        shaderTech2_group.links.new(mult_color_node.outputs[0], diffuse_node.inputs[1])
        shaderTech2_group.links.new(mult_lightmap_node.outputs[0], mult_color_node.inputs[1])
        shaderTech2_group.links.new(mult_env_node.outputs[0], diffuse_node.inputs[2])
        shaderTech2_group.links.new(extra_fudge_node.outputs[0], specular_node.inputs[1])
        shaderTech2_group.links.new(mult_specular_node.outputs[0], mult_env_node.inputs['Scale'])
        shaderTech2_group.links.new(mult_specular_node.outputs[0], extra_fudge_node.inputs[0])
        shaderTech2_group.links.new(magicBlend_node_1.outputs[0], mult_lightmap_node.inputs[2])
        shaderTech2_group.links.new(magicBlend_node_1.outputs[1], mult_specular_node.inputs[1])
        shaderTech2_group.links.new(magicBlend_node_2.outputs[0], magicBlend_node_1.inputs[0])
        shaderTech2_group.links.new(magicBlend_node_2.outputs[1], magicBlend_node_1.inputs[1])
        shaderTech2_group.links.new(overlay_node.outputs[0], magicBlend_node_2.inputs[1])
        shaderTech2_group.links.new(overlay_node.outputs[1], magicBlend_node_2.inputs[8])
        shaderTech2_group.links.new(overlay_node.outputs[2], magicBlend_node_1.inputs[8])
        shaderTech2_group.links.new(vertexColour_node.outputs[1], magicBlend_node_2.inputs[7])
        shaderTech2_group.links.new(vertexColour_node.outputs[2], magicBlend_node_1.inputs[7])

    return shaderTech2_group

# Create the tech2Shader nodes
def createT2Nodes(material):
    # Output node
    output_node = material.node_tree.nodes.new(type='ShaderNodeOutputMaterial')
    output_node.location = (0,0)
    # Shader Tech2 group node
    tech2_group = createShaderTech2GroupNode()
    tech2_node = material.node_tree.nodes.new(type='ShaderNodeGroup')
    tech2_node.name = "Shader Tech2"
    tech2_node.label = "Shader Tech2"
    tech2_node.node_tree = bpy.data.node_groups[tech2_group.name]
    tech2_node.inputs[0].default_value = (0.8, 0.8, 0.8, 1)
    tech2_node.inputs[1].default_value = (0.8, 0.8, 0.8, 1)
    tech2_node.inputs[2].default_value = (0.5, 0.5, 0.5, 1)
    tech2_node.inputs[3].default_value = (0.0, 0.0, 0.0, 1)
    tech2_node.inputs[4].default_value = (1.0, 1.0, 1.0, 1)
    tech2_node.inputs[5].default_value = 1.000
    tech2_node.inputs[6].default_value = (0.8, 0.8, 0.8, 1)
    tech2_node.inputs[7].default_value = (0.0, 0.0, 0.0, 1)
    tech2_node.inputs[8].default_value = 0.626
    tech2_node.inputs[9].default_value = 0.000
    tech2_node.inputs[10].default_value = (0.795, 0.795, 0.795, 1)
    tech2_node.inputs[11].default_value = (1, 1, 1, 1)
    tech2_node.inputs[12].default_value = 5.583
    tech2_node.inputs[13].default_value = 0.858
    tech2_node.inputs[14].default_value = 0.342
    tech2_node.inputs[15].default_value = 0.655
    tech2_node.inputs[16].default_value = 0.023
    tech2_node.inputs[17].default_value = (0.838, 0.838, 0.838, 1)
    tech2_node.inputs[18].default_value = (1, 1, 1, 1)
    tech2_node.inputs[19].default_value = 5.500
    tech2_node.inputs[20].default_value = 0.800
    tech2_node.inputs[21].default_value = 0.372
    tech2_node.inputs[22].default_value = 1.000
    tech2_node.location = (-200,0)
    # Base Texture node 
    baseTexture_node = material.node_tree.nodes.new(type='ShaderNodeTexImage')
    baseTexture_node.interpolation = 'Linear'
    baseTexture_node.projection = 'FLAT'
    baseTexture_node.extension = 'REPEAT'
    baseTexture_node.name = "Base Texture"
    baseTexture_node.label = "Base Texture"
    baseTexture_node.location = (-600,150)
    # Overlay Texture node 
    overlayTexture_node = material.node_tree.nodes.new(type='ShaderNodeTexImage')
    overlayTexture_node.interpolation = 'Linear'
    overlayTexture_node.projection = 'FLAT'
    overlayTexture_node.extension = 'REPEAT'
    overlayTexture_node.name = "Overlay Texture"
    overlayTexture_node.label = "Overlay Texture"
    overlayTexture_node.location = (-600,-100)
    # Light Map Texture node 
    lightmapTexture_node = material.node_tree.nodes.new(type='ShaderNodeTexImage')
    lightmapTexture_node.interpolation = 'Linear'
    lightmapTexture_node.projection = 'FLAT'
    lightmapTexture_node.extension = 'REPEAT'
    lightmapTexture_node.name = "Light Map Texture"
    lightmapTexture_node.label = "Light Map Texture"
    lightmapTexture_node.location = (-600,-350)
    # Environment Texture node
    environmentTexture_node = material.node_tree.nodes.new(type='ShaderNodeTexEnvironment')
    environmentTexture_node.interpolation = 'Linear'
    environmentTexture_node.projection = 'EQUIRECTANGULAR'
    environmentTexture_node.name = "Environment Texture"
    environmentTexture_node.label = "Environment Texture"
    environmentTexture_node.location = (-600,-600)
    # UV Base Map node
    UVBaseMap_node = material.node_tree.nodes.new(type='ShaderNodeUVMap')
    UVBaseMap_node.name = "uv Base Map"
    UVBaseMap_node.label = "uv Base Map"
    UVBaseMap_node.name = "uv Base Map"
    UVBaseMap_node.uv_map = "map1"
    UVBaseMap_node.location = (-900,100)
    # Vertex Colour node
    vertexColour_node = material.node_tree.nodes.new(type='ShaderNodeVertexColor')
    vertexColour_node.name = "Vertex Colour"
    vertexColour_node.label = "Vertex Colour"
    vertexColour_node.layer_name = "colorSet"
    vertexColour_node.location = (-900,-50)
    # UV Light Map node
    UVLightMap_node = material.node_tree.nodes.new(type='ShaderNodeUVMap')
    UVLightMap_node.name = "uv uvMapLightMap"
    UVLightMap_node.label = "uv uvMapLightMap"
    UVLightMap_node.uv_map = "UVMap"
    UVLightMap_node.location = (-900,-200)
    # Vertex Colour 2 node
    vertexColour2_node = material.node_tree.nodes.new(type='ShaderNodeVertexColor')
    vertexColour2_node.name = "Vertex Colour2"
    vertexColour2_node.label = "Vertex Colour2"
    vertexColour2_node.layer_name = "colorSet1"
    vertexColour2_node.location = (-900,-350)
    # Reflection group node
    reflection_group = createReflectionGroupNode()
    reflection_node = material.node_tree.nodes.new(type='ShaderNodeGroup')
    reflection_node.name = "Reflection"
    reflection_node.label = "Reflection"
    reflection_node.node_tree = bpy.data.node_groups[reflection_group.name]
    reflection_node.location = (-900,-500)

    #### Node links ####
    material.node_tree.links.new(tech2_node.outputs[0], output_node.inputs[0])
    material.node_tree.links.new(baseTexture_node.outputs[0], tech2_node.inputs[0])
    material.node_tree.links.new(overlayTexture_node.outputs[0], tech2_node.inputs[1])
    material.node_tree.links.new(lightmapTexture_node.outputs[0], tech2_node.inputs[2])
    material.node_tree.links.new(environmentTexture_node.outputs[0], tech2_node.inputs[3])
    material.node_tree.links.new(vertexColour_node.outputs[0], tech2_node.inputs[6])
    material.node_tree.links.new(vertexColour2_node.outputs[0], tech2_node.inputs[7])
    material.node_tree.links.new(UVBaseMap_node.outputs[0], baseTexture_node.inputs[0])
    material.node_tree.links.new(UVBaseMap_node.outputs[0], overlayTexture_node.inputs[0])
    material.node_tree.links.new(UVLightMap_node.outputs[0], lightmapTexture_node.inputs[0])
    material.node_tree.links.new(reflection_node.outputs[0], environmentTexture_node.inputs[0])

class tech2Material:
    # Create all groups that will be used to make a new tech2 material
    def __init__(self):
        createReflectionGroupNode()
        createMixGroupNode()
        createInverseGroupNode()
        createInterpolateGroupNode()
        createMaxComponentGroupNode()
        createBlend1GroupNode()
        createMagicBlendGroupNode()
        createShaderTech2GroupNode()

    def new(self, material:bpy.types.Material=None):
        if(material is not None):
            new_material = bpy.data.materials.new(name="t2_" + material.name)
            # TODO Needs to get all the data from the existing material
        else:
            new_material = bpy.data.materials.new(name="t2_Material")
        new_material.use_nodes=True
        # Clean the nodes of the newly created material
        for node in new_material.node_tree.nodes:
            new_material.node_tree.nodes.remove(node)
        createT2Nodes(new_material)

        return new_material

# TODO move this to a mesh related file
# change object UV map to map1 and recreate UVMap
def create_UV_maps(UV_object):
    if(len(UV_object.data.uv_layers) == 1):
        if(UV_object.data.uv_layers.active.name != "map1"):
            UV_object.data.uv_layers.active.name = "map1"
        UV_object.data.uv_layers.new(name="UVMap")
    else:
        map1_exist = False
        UVMap_exist = False
        for uv in UV_object.data.uv_layers:
            if(uv.name == "map1"):
                map1_exist = True
            if(uv.name == "UVMap"):
                UVMap_exist = True
        if not(map1_exist):
            UV_object.data.uv_layers.active.name = "map1"
        if not(UVMap_exist):
            UV_object.data.uv_layers.new(name="UVMap")

# Set active Vertex color to colorSet
def make_active(vertex_colors, name):
    for vertex_color in vertex_colors:
        if vertex_color.name == name:
            vertex_colors.active = vertex_color
            return
    print("Could not find:", name, "\n(this should never happen)")

def move_to_bottom(vertex_colors, index):
    vertex_colors.active_index = index
    new_name = vertex_colors.active.name
    bpy.ops.mesh.vertex_color_add()

    # delete the "old" one
    make_active(vertex_colors, new_name)
    bpy.ops.mesh.vertex_color_remove()

    # set the name of the last one
    vertex_colors.active_index = len(vertex_colors) - 1
    vertex_colors.active.name = new_name

def fixVertexColorOrder(mesh):
    vertex_colors = mesh.data.vertex_colors
    vertex_colors.active_index = 0
    if vertex_colors.active.name != "colorSet":
        orig_ind = vertex_colors.active_index
        if orig_ind == len(vertex_colors) - 1:
            return
        # use "trick" on the one after it
        move_to_bottom(vertex_colors, orig_ind + 1)
        # use the "trick" on the vertex color
        move_to_bottom(vertex_colors, orig_ind)
        # use the "trick" on the rest that are after where it was
        for i in range(orig_ind, len(vertex_colors) - 2):
            move_to_bottom(vertex_colors, orig_ind)
        vertex_colors.active_index = 0

# create vertex colors "colorSet" and "colorSet1"
def create_vertex_colors(mesh):
    if(mesh.data.vertex_colors.active is None):
        mesh.data.vertex_colors.new(name="colorSet")
        mesh.data.vertex_colors.new(name="colorSet1")
    elif(len(mesh.data.vertex_colors) == 1):
        mesh.data.vertex_colors.active.name = "colorSet1"
        mesh.data.vertex_colors.new(name="colorSet")
    else:
        colorSet_exist = False
        colorSet1_exist = False
        for vertex_color in mesh.data.vertex_colors:
            if(vertex_color.name == "colorSet"):
                colorSet_exist = True
            if(vertex_color.name == "colorSet1"):
                colorSet1_exist = True
        if not(colorSet1_exist):
            mesh.data.vertex_colors.active.name = "colorSet1"
        if not(colorSet_exist):
            mesh.data.vertex_colors.new(name="colorSet")

def create_mockup_images():
    overlay_image = False
    lightmap_image = False
    environment_image = False

    for im in bpy.data.images:
        if(im.name == "black_overlay_texture.png"):
            overlay_image = True
        if(im.name == "white_lightmap_texture.png"):
            lightmap_image = True
        if(im.name == "white_environment_texture.png"):
            environment_image = True

    if(overlay_image == False):
        image = bpy.data.images.new("black_overlay_texture.png", alpha=True, width=16, height=16)
        image.alpha_mode = 'STRAIGHT'
        image.filepath_raw = "//black_overlay_texture.png"
        image.file_format = 'PNG'
        image.generated_color = (0,0,0,1)
        image.save()
    
    if(lightmap_image == False):
        image = bpy.data.images.new("white_lightmap_texture.png", alpha=True, width=2048, height=2048)
        image.alpha_mode = 'STRAIGHT'
        image.filepath_raw = "//white_lightmap_texture.png"
        image.file_format = 'PNG'
        image.generated_color = (1,1,1,1)
        image.save()
        
    if(environment_image == False):
        image = bpy.data.images.new("white_environment_texture.png", alpha=True, width=16, height=16)
        image.alpha_mode = 'STRAIGHT'
        image.filepath_raw = "//white_environment_texture.png"
        image.file_format = 'PNG'
        image.generated_color = (1,1,1,1)
        image.save()

def get_image_texture_from_material(material):
    base_image = None
    if(material.node_tree != None):
        for node in material.node_tree.nodes:
            if("Principled BSDF" == node.name):
                for link in node.inputs[0].links:
                    if("Image Texture" == link.from_node.name):
                        base_image = link.from_node.image

    return base_image

def set_image_texture(material, node_name, image):
    if(material.node_tree != None):
        for node in material.node_tree.nodes:
            if(node_name == node.name):
                node.image = image

def set_mockup_images(material):
    overlay_image = bpy.data.images["black_overlay_texture.png"]
    lightmap_image = bpy.data.images["white_lightmap_texture.png"]
    environment_image = bpy.data.images["white_environment_texture.png"]
    set_image_texture(material, "Overlay Texture", overlay_image)
    set_image_texture(material, "Light Map Texture", lightmap_image)
    set_image_texture(material, "Environment Texture", environment_image)

################ END OF SUPPORT FUNCTIONS ################

class TS_OT_new_tech2_op(bpy.types.Operator):
    bl_label = 'Create new Tech2'
    bl_idname = 'ts.new_tech2_op'
    bl_description = 'Create an empty Shader Tech2 material'
    bl_context = 'objectmode'
    bl_options = {'REGISTER', 'INTERNAL'}

    def execute(self, context):
        t2_material = tech2Material()
        t2_material.new()

        return {'FINISHED'}

class TS_OT_convert_to_tech2_op(bpy.types.Operator):
    bl_label = 'Convert active to Tech2'
    bl_idname = 'ts.convert_to_tech2_op'
    bl_description = 'Convert an existing active material to a Shader Tech2 material'
    bl_context = 'objectmode'
    bl_options = {'REGISTER', 'INTERNAL'}

    def execute(self, context):
        # TODO mockup should only be set if the material is not a tech2 already
        create_mockup_images()

        current_obj = bpy.context.active_object
        active_material = current_obj.active_material

        t2_material = tech2Material()
        new = t2_material.new(active_material)
        image_texture = get_image_texture_from_material(active_material)

        # set new tech2 created material
        current_obj.active_material = new
    
        # create the UV maps for the mesh
        create_UV_maps(current_obj)
    
        # create the vertex colors for the mesh
        create_vertex_colors(current_obj)
        fixVertexColorOrder(current_obj)

        set_image_texture(active_material, "Base Texture", image_texture)

        # TODO mockup should only be set if the material is not a tech2 already
        set_mockup_images(active_material)

        return {'FINISHED'}

class TS_OT_convert_all_to_tech2_op(bpy.types.Operator):
    bl_label = 'Convert all materials to Tech2'
    bl_idname = 'ts.convert_all_to_tech2_op'
    bl_description = 'Convert all existing materials to a Shader Tech2 material'
    bl_context = 'objectmode'
    bl_options = {'REGISTER', 'INTERNAL'}

    def execute(self, context):
        # TODO mockup should only be set if the material is not a tech2 already
        create_mockup_images()
        for ob in bpy.data.objects:
            if(ob.name.lower().endswith("_vis")):
                for material in ob.material_slots:
                    t2_material = tech2Material()
                    new = t2_material.new(material.material)
                    image_texture = get_image_texture_from_material(material.material)

                    # set new tech2 created material
                    material.material = new
    
                    # create the UV maps for the mesh
                    create_UV_maps(ob)
    
                    # create the vertex colors for the mesh
                    create_vertex_colors(ob)
                    fixVertexColorOrder(ob)

                    set_image_texture(material.material, "Base Texture", image_texture)

                    # TODO mockup should only be set if the material is not a tech2 already
                    set_mockup_images(material.material)

        return {'FINISHED'}

def register():
    bpy.utils.register_class(TS_OT_new_tech2_op)
    bpy.utils.register_class(TS_OT_convert_to_tech2_op)
    bpy.utils.register_class(TS_OT_convert_all_to_tech2_op)

def unregister():
    bpy.utils.unregister_class(TS_OT_new_tech2_op)
    bpy.utils.unregister_class(TS_OT_convert_to_tech2_op)
    bpy.utils.unregister_class(TS_OT_convert_all_to_tech2_op)