import subprocess
import sys

def compile_shader(shader_name, shader_type):
    shader_path = "./assets/shaders/source/" + shader_name + "." + shader_type + ".glsl"
    shader_bin_path = "./assets/shaders/bin/" + shader_name + "." + shader_type + ".spv"
    print("compiling " + shader_name + "." + shader_type + ".glsl")
    shader_compile_result = subprocess.run(["glslc", "-g", "-fshader-stage=" + shader_type, shader_path, "-o", shader_bin_path], capture_output=True, text=True)
    if shader_compile_result.stdout != "":
        print(shader_compile_result.stdout)
    if shader_compile_result.stderr != "":
        print(shader_compile_result.stderr)

shader_list = [
    ("builtin.material_shader", ["vert", "frag"]),
    ("builtin.ui_shader", ["vert", "frag"]),
    ("builtin.skybox_shader", ["vert", "frag"]),
    ("builtin.depth_shader", ["vert", "frag"]),
    ("builtin.ao_shader", ["vert", "frag"]),
    ("builtin.blur_shader", ["vert", "frag"]),
    ("builtin.shadowmap_directional_shader", ["vert", "frag"]),
    ("builtin.shadowmap_sampling_shader", ["vert", "frag"]),
    ("builtin.ssr_shader", ["vert", "frag"]),
    ("builtin.volumetrics_shader", ["vert", "frag"]),
    ("builtin.volumetrics_blur_shader", ["vert", "frag"]),
    ("builtin.post_processing_effects_shader", ["vert", "frag"])
]

for shader, phases in shader_list:
    for phase in phases:
        compile_shader(shader, phase)

print("finished")