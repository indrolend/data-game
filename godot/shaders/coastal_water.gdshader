shader_type spatial;
render_mode cull_disabled;

uniform float native_time = 0.0;
uniform vec2 source_position = vec2(0.0);
uniform vec2 shore_bounds = vec2(2.25, 8.91);
uniform float source_speed = 0.0;
uniform bool source_active = false;

varying vec3 water_position;

float native_wave_height(vec2 p) {
	float primary = p.x * 1.27 + p.y * 0.34 - native_time * 1.65;
	float crossing = p.y * 0.58 + native_time * 0.72;
	float long_swell = p.x * 0.31 - p.y * 0.19 + native_time * 0.38;
	float swell = 0.040 + sin(primary) * 0.044 + sin(crossing) * 0.017 + sin(long_swell) * 0.010;
	if (!source_active || source_speed < 0.35) {
		return swell;
	}
	float distance_to_source = distance(p, source_position);
	if (distance_to_source > 2.8) {
		return swell;
	}
	float speed_amount = clamp((source_speed - 0.35) / 8.0, 0.0, 1.0);
	float envelope = exp(-distance_to_source * 1.18) * clamp(distance_to_source / 0.16, 0.0, 1.0);
	float wake = sin(distance_to_source * 10.5 - native_time * (5.8 + speed_amount * 3.0))
		* envelope * (0.012 + speed_amount * 0.032);
	return swell + wake;
}

float native_wave_dx(vec2 p) {
	float primary = p.x * 1.27 + p.y * 0.34 - native_time * 1.65;
	float long_swell = p.x * 0.31 - p.y * 0.19 + native_time * 0.38;
	return cos(primary) * 0.044 * 1.27 + cos(long_swell) * 0.010 * 0.31;
}

float native_wave_dz(vec2 p) {
	float primary = p.x * 1.27 + p.y * 0.34 - native_time * 1.65;
	float crossing = p.y * 0.58 + native_time * 0.72;
	float long_swell = p.x * 0.31 - p.y * 0.19 + native_time * 0.38;
	return cos(primary) * 0.044 * 0.34 + cos(crossing) * 0.017 * 0.58
		- cos(long_swell) * 0.010 * 0.19;
}

void vertex() {
	vec3 world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	vec2 p = world_position.xz;
	float height = native_wave_height(p);
	float dx = native_wave_dx(p);
	float dz = native_wave_dz(p);
	VERTEX.y += height;
	NORMAL = normalize(vec3(-dx, 1.0, -dz));
	water_position = vec3(p.x, height, p.y);
}

void fragment() {
	float crest = sin(water_position.x * 2.2 + water_position.z * 1.7 - native_time * 1.65)
		* sin(water_position.z * 0.9 + native_time * 0.72);
	float fresnel = pow(1.0 - clamp(dot(normalize(NORMAL), normalize(VIEW)), 0.0, 1.0), 3.0);
	float shore_amount = smoothstep(shore_bounds.x, shore_bounds.y, water_position.x);
	vec3 deep = vec3(0.025, 0.17, 0.22);
	vec3 shallow = vec3(0.10, 0.43, 0.40);
	vec3 color = mix(shallow, deep, shore_amount * 0.82);
	color += vec3(0.025, 0.045, 0.040) * (0.5 + crest * 0.5 + fresnel * 0.35);
	ALBEDO = color;
	EMISSION = color * 0.10;
	METALLIC = 0.12;
	ROUGHNESS = 0.24;
	SPECULAR = 0.68;
}
