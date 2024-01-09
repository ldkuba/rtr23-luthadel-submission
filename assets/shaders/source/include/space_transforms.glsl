
vec3 screen_to_view(
    vec2 coords,
    sampler2D depth_sampler,
    mat4 projection_inverse
) {
    float fragment_depth = texture(depth_sampler, coords).w;
    
    // Convert coords and fragment_depth to
    // normalized device coordinates (clip space)
    vec4 ndc = vec4(
        coords.x * 2.0 - 1.0,
        1.0 - coords.y * 2.0,
        fragment_depth,
        1.0
    );
    
    // Transform to view space using inverse camera projection
    vec4 view_pos = projection_inverse * ndc;
    // Since we used a projection transformation (even if it was in inverse)
    // we need to convert our homogen eous coordinates using the perspective divide.
    view_pos /= view_pos.w;
    
    return view_pos.xyz;
}

vec3 view_to_world(
    vec3 view_pos,
    mat4 view_inverse
) {
    // Transform to world space using inverse view projection
    vec4 world_pos = view_inverse * vec4(view_pos, 1);
    return world_pos.xyz;
}

vec3 screen_to_world(
    vec2 coords,
    sampler2D depth_sampler,
    mat4 projection_inverse,
    mat4 view_inverse
) {
    vec3 view_pos = screen_to_view(coords, depth_sampler, projection_inverse);
    return view_to_world(view_pos, view_inverse);
}