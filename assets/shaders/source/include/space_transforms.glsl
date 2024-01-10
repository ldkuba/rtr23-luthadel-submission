
vec3 screen_to_clip(
    vec2 coords,
    float ndc_depth
) {
    // Convert coords and fragment_depth to
    // normalized device coordinates (clip space)
    vec3 clip_pos = vec3(
        coords.x * 2.0 - 1.0,
        1.0 - coords.y * 2.0,
        ndc_depth
    );
    
    return clip_pos;
}

vec3 clip_to_view(
    vec3 coords,
    mat4 projection_inverse
) {
    // Transform to view space using inverse camera projection
    vec4 view_pos = projection_inverse * vec4(coords, 1.0);
    // Since we used a projection transformation (even if it was in inverse)
    // we need to convert our homogeneous coordinates using the perspective divide.
    view_pos /= view_pos.w;
    
    return view_pos.xyz;
}

vec3 screen_to_view(
    vec2 coords,
    float ndc_depth,
    mat4 projection_inverse
) {
    vec3 clip_pos = screen_to_clip(coords, ndc_depth);
    return clip_to_view(clip_pos, projection_inverse);
}

vec3 view_to_world(
    vec3 coords,
    mat4 view_inverse
) {
    // Transform to world space using inverse view projection
    vec4 world_pos = view_inverse * vec4(coords, 1);
    return world_pos.xyz;
}

vec3 screen_to_world(
    vec2 coords,
    float ndc_depth,
    mat4 projection_inverse,
    mat4 view_inverse
) {
    vec3 view_pos = screen_to_view(coords, ndc_depth, projection_inverse);
    return view_to_world(view_pos, view_inverse);
}

vec3 world_to_view(
    vec3 coords,
    mat4 view
) {
    vec4 view_pos = view * vec4(coords, 1);
    return view_pos.xyz;
}

vec3 view_to_clip(
    vec3 coords,
    mat4 projection
) {
    vec4 clip_pos = projection * vec4(coords, 1);
    clip_pos /= clip_pos.w;
    return clip_pos.xyz;
}

vec2 clip_to_screen(
    vec3 coords
) {
    vec3 screen_pos = coords * 0.5 + 0.5;
    screen_pos.y = 1.0 - screen_pos.y;
    return screen_pos.xy;
}

vec2 view_to_screen(
    vec3 coords,
    mat4 projection
) {
    vec3 clip_pos = view_to_clip(coords, projection);
    return clip_to_screen(clip_pos);
}

vec2 world_to_screen(
    vec3 coords,
    mat4 view,
    mat4 projection
) {
    vec3 view_pos = world_to_view(coords, view);
    return view_to_screen(view_pos, projection);
}