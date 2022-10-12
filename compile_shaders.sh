glslc -fshader-stage=vert ./src/shaders/builtin.material_shader.vert.glsl -o ./assets/shaders/builtin.material_shader.vert.spv
glslc -fshader-stage=frag ./src/shaders/builtin.material_shader.frag.glsl -o ./assets/shaders/builtin.material_shader.frag.spv

glslc -fshader-stage=vert ./src/shaders/builtin.ui_shader.vert.glsl -o ./assets/shaders/builtin.ui_shader.vert.spv
glslc -fshader-stage=frag ./src/shaders/builtin.ui_shader.frag.glsl -o ./assets/shaders/builtin.ui_shader.frag.spv