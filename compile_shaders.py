import subprocess
import sys

def compile_shader(shader_name, shader_type):
    shader_path = "./assets/shaders/source/" + shader_name + "." + shader_type + ".glsl"
    shader_bin_path = "./assets/shaders/bin/" + shader_name + "." + shader_type + ".spv"
    print("compiling " + shader_name + "." + shader_type + ".glsl")
    shader_compile_result = subprocess.run(["glslc", "-g", "-fshader-stage=" + shader_type, shader_path, "-o", shader_bin_path], capture_output=True, text=True)
    print(shader_compile_result.stdout)
    print(shader_compile_result.stderr)

# builtin.material_shader
compile_shader("builtin.material_shader", "vert")
compile_shader("builtin.material_shader", "frag")

# builtin.ui_shader
compile_shader("builtin.ui_shader", "vert")
compile_shader("builtin.ui_shader", "frag")

print("finished")