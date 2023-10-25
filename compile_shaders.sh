glslc -fshader-stage=vert ./assets/shaders/source/builtin.material_shader.vert.glsl -o ./assets/shaders/bin/builtin.material_shader.vert.spv
glslc -fshader-stage=frag ./assets/shaders/source/builtin.material_shader.frag.glsl -o ./assets/shaders/bin/builtin.material_shader.frag.spv

glslc -fshader-stage=vert ./assets/shaders/source/builtin.ui_shader.vert.glsl -o ./assets/shaders/bin/builtin.ui_shader.vert.spv
glslc -fshader-stage=frag ./assets/shaders/source/builtin.ui_shader.frag.glsl -o ./assets/shaders/bin/builtin.ui_shader.frag.spv
