shader_type spatial;
render_mode unshaded, cull_front, depth_draw_never, fog_disabled, world_vertex_coords;

uniform vec3 zenith_color : source_color = vec3(0.16, 0.34, 0.52);
uniform vec3 horizon_color : source_color = vec3(0.48, 0.24, 0.22);
uniform vec3 ground_color : source_color = vec3(0.025, 0.035, 0.055);
uniform vec3 sun_color : source_color = vec3(1.0, 0.76, 0.54);
uniform vec3 sun_direction = vec3(0.3, 0.8, 0.4);
uniform float daylight : hint_range(0.0, 1.0) = 0.8;
uniform float cloud_coverage : hint_range(0.0, 1.0) = 0.52;
uniform float cloud_opacity : hint_range(0.0, 1.0) = 0.54;
uniform float cloud_form : hint_range(0.0, 2.0) = 0.0;
uniform float cloud_seed = 0.0;
uniform float cloud_phase = 0.0;
uniform vec3 sky_origin = vec3(0.0);

varying vec3 sky_direction;

float hash21(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float value_noise(vec2 p) {
	vec2 cell = floor(p);
	vec2 local = fract(p);
	local = local * local * (3.0 - 2.0 * local);
	float a = hash21(cell);
	float b = hash21(cell + vec2(1.0, 0.0));
	float c = hash21(cell + vec2(0.0, 1.0));
	float d = hash21(cell + vec2(1.0, 1.0));
	return mix(mix(a, b, local.x), mix(c, d, local.x), local.y);
}

void vertex() {
	sky_direction = normalize(VERTEX - sky_origin);
}

void fragment() {
	vec3 direction = normalize(sky_direction);
	float horizon_blend = smoothstep(-0.035, 0.88, direction.y);
	vec3 color = mix(horizon_color, zenith_color, horizon_blend);
	color += horizon_color * exp(-abs(direction.y) * 8.0) * 0.18;
	float below_horizon = smoothstep(0.0, 0.42, -direction.y);
	color = mix(color, ground_color, below_horizon * 0.82);

	if (direction.y > 0.04) {
		vec2 projected = direction.xz / max(0.28, direction.y + 0.34);
		projected = projected * vec2(5.2, 2.8) +
			vec2(cloud_seed * 4.1 + cloud_phase, cloud_seed * 1.7 + cloud_phase * 0.31);
		if (cloud_form < 0.5) {
			projected *= vec2(0.48, 1.7); // stretched, wispy Field cirrus
		} else if (cloud_form < 1.5) {
			projected *= vec2(1.25, 1.25); // broken urban high-cloud deck
		} else {
			projected *= vec2(0.78, 0.78); // broader Coastal cumulus forms
		}
		vec2 warp = vec2(value_noise(projected * 0.42 + vec2(4.3, 8.1)),
			value_noise(projected * 0.42 + vec2(11.7, 2.6))) - 0.5;
		float broad = value_noise(projected + warp * 1.1);
		float detail = value_noise(projected * 2.35 + vec2(13.7, 7.1));
		float wisps = value_noise(projected * 4.8 + vec2(23.1, 17.4));
		float density = broad * 0.63 + detail * 0.27 + wisps * 0.10;
		if (cloud_form < 0.5) {
			float ridge = 1.0 - abs(broad * 2.0 - 1.0);
			density = ridge * 0.38 + detail * 0.44 + wisps * 0.18;
		}
		float threshold = 0.68 - cloud_coverage * 0.30;
		float clouds = smoothstep(threshold, threshold + 0.20, density);
		vec3 cloud_daylight = mix(vec3(0.76, 0.81, 0.88), sun_color * 0.94, 0.20);
		vec3 cloud_light = mix(vec3(0.075, 0.095, 0.16), cloud_daylight, daylight);
		cloud_light *= mix(0.70, 1.0, smoothstep(0.05, 0.70, direction.y));
		color = mix(color, cloud_light, clouds * cloud_opacity);
	}

	vec3 sun_dir = normalize(sun_direction);
	float sun_dot = max(dot(direction, sun_dir), 0.0);
	float sun_visible = smoothstep(-0.10, 0.12, sun_dir.y) * daylight;
	float sun_angle = acos(clamp(sun_dot, 0.0, 1.0));
	float sun_disk = 1.0 - smoothstep(radians(0.245), radians(0.285), sun_angle);
	float sun_radius = clamp(sun_angle / radians(0.266), 0.0, 1.0);
	float limb_darkening = 0.82 + 0.18 * sqrt(max(1.0 - sun_radius * sun_radius, 0.0));
	float sun_halo = exp(-sun_angle * 8.0) * 0.18;
	color += sun_color * (sun_disk * limb_darkening * 2.2 + sun_halo) * sun_visible;

	float night = 1.0 - smoothstep(0.12, 0.58, daylight);
	vec2 star_grid = vec2(atan(direction.z, direction.x) / TAU + 0.5,
		asin(clamp(direction.y, -1.0, 1.0)) / PI + 0.5) * vec2(420.0, 210.0);
	vec2 star_cell = floor(star_grid);
	vec2 star_local = fract(star_grid);
	vec2 star_center = vec2(hash21(star_cell + vec2(7.1, 3.7)),
		hash21(star_cell + vec2(17.3, 11.9)));
	float star_point = 1.0 - smoothstep(0.006, 0.031, distance(star_local, star_center));
	float stars = star_point * step(0.9982, hash21(star_cell + cloud_seed * 19.7));
	color += vec3(0.58, 0.70, 1.0) * stars * night * smoothstep(-0.02, 0.22, direction.y) * 0.78;

	ALBEDO = color;
}
