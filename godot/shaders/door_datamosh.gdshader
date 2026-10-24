shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, filter_linear;
uniform float strength : hint_range(0.0, 1.0) = 0.0;
uniform vec2 motion_pixels = vec2(0.0);
uniform vec2 viewport_size = vec2(1280.0, 720.0);

float cell_noise(vec2 cell, float salt) {
    return fract(sin(dot(cell + salt, vec2(12.9898, 78.233))) * 43758.5453);
}

void fragment() {
    vec2 grid = vec2(48.0, 27.0);
    vec2 cell = floor(SCREEN_UV * grid);
    vec2 cell_center = (cell + vec2(0.5)) / grid;
    vec2 radial = normalize(cell_center - vec2(0.5) + vec2(0.00001));
    vec2 tangent = vec2(-radial.y, radial.x);
    float release = 1.0 - strength;
    float eased = 1.0 - (1.0 - release) * (1.0 - release);
    float angle = (cell_noise(cell, 17.0) - 0.5) * 1.1;
    vec2 direction = radial * cos(angle) + tangent * sin(angle);
    float travel = (42.0 + cell_noise(cell, 31.0) * 150.0) * eased;
    vec2 cell_motion = direction * travel + motion_pixels * (0.35 + cell_noise(cell, 61.0));
    vec2 base_motion = motion_pixels * (0.42 + strength * 0.92);
    vec2 displaced_uv = clamp(SCREEN_UV - (cell_motion + base_motion) / viewport_size, vec2(0.0), vec2(1.0));
    vec4 present = texture(screen_texture, SCREEN_UV);
    vec4 displaced = texture(screen_texture, displaced_uv);
    float alpha = clamp(0.34 + strength * 0.56, 0.34, 0.90);
    float cell_alpha = strength * (0.72 + 0.24 * cell_noise(cell, 89.0));
    COLOR = mix(present, displaced, max(alpha * 0.32, cell_alpha));
}
