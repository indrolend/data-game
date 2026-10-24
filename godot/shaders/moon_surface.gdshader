shader_type spatial;
render_mode unshaded, cull_back, world_vertex_coords, fog_disabled;

uniform sampler2D moon_albedo : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D moon_elevation : filter_linear_mipmap, repeat_enable;
uniform vec3 sun_direction = vec3(0.3, 0.8, 0.4);

varying vec3 lunar_normal;

void vertex() {
	lunar_normal = normalize(NORMAL);
}

void fragment() {
	vec3 lunar_color = texture(moon_albedo, UV).rgb;
	vec2 texel = 1.0 / vec2(textureSize(moon_elevation, 0));
	float relief_left = texture(moon_elevation, UV - vec2(texel.x, 0.0)).r;
	float relief_right = texture(moon_elevation, UV + vec2(texel.x, 0.0)).r;
	float relief_up = texture(moon_elevation, UV - vec2(0.0, texel.y)).r;
	float relief_down = texture(moon_elevation, UV + vec2(0.0, texel.y)).r;
	vec3 tangent = cross(vec3(0.0, 1.0, 0.0), normalize(lunar_normal));
	if (length(tangent) < 0.01) {
		tangent = cross(vec3(0.0, 0.0, 1.0), normalize(lunar_normal));
	}
	tangent = normalize(tangent);
	vec3 bitangent = normalize(cross(normalize(lunar_normal), tangent));
	vec3 terrain_normal = normalize(lunar_normal + tangent * (relief_left - relief_right) * 0.12 +
		bitangent * (relief_up - relief_down) * 0.12);
	float incidence = max(dot(terrain_normal, normalize(sun_direction)), 0.0);
	// The Sun still illuminates the Moon when it is below the player's horizon.
	// A small earthshine term keeps the unlit side from becoming a hard cutout.
	float illumination = 0.025 + incidence;
	ALBEDO = lunar_color * illumination;
	EMISSION = lunar_color * incidence * 0.035;
}
