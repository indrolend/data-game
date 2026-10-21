extends SceneTree

var failures: Array[String] = []

func check(condition: bool, message: String) -> void:
    if not condition:
        failures.append(message)
        push_error("FAIL: " + message)

func _initialize() -> void:
    var runtime = ClassDB.instantiate("ParentGameNode")
    var adapter = preload("res://scripts/room_presentation_adapter.gd").new()
    root.add_child(adapter)
    var grass_pixels: PackedByteArray = runtime.field_grass_texture_pixels()
    adapter.set_native_field_grass_pixels(grass_pixels)
    var asphalt_pixels: PackedByteArray = runtime.city_asphalt_texture_pixels()
    adapter.set_native_city_asphalt_pixels(asphalt_pixels)
    for material_name in ["stone", "metal"]:
        var native_texture: ImageTexture = adapter.native_material_texture(material_name)
        check(native_texture != null,
            "native %s runtime RGB material loads as a Godot texture" % material_name)
        if native_texture != null:
            var texture_image := native_texture.get_image()
            check(texture_image.get_width() == 512 and texture_image.get_height() == 512,
                "native %s runtime map retains its 512x512 dimensions" % material_name)
            var source_path := ProjectSettings.globalize_path(
                "res://../parent/models/materials/photo-%s-runtime.rgb" % material_name)
            var source_file := FileAccess.open(source_path, FileAccess.READ)
            check(source_file != null, "native %s runtime RGB source remains available" % material_name)
            if source_file != null:
                var source_pixels := source_file.get_buffer(source_file.get_length())
                source_file.close()
                check(texture_image.get_data() == source_pixels,
                    "Godot %s image bytes exactly match the source runtime RGB map" % material_name)
            check(adapter.native_material_texture(material_name) == native_texture,
                "native %s texture is cached and reused" % material_name)
    var snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    snapshot["grass_blades"] = runtime.grass_blade_presentation_snapshot(int(snapshot.tile_index))
    snapshot["grass_tips"] = runtime.grass_blade_tip_presentation_snapshot(int(snapshot.tile_index))
    adapter.sync_snapshot(snapshot)
    var capture_records: Array = snapshot.get("capture_points", [])
    check(capture_records.size() == 5 and adapter.capture_point_count() == capture_records.size(),
        "native objective state supplies and Godot renders all five Field capture points")
    if not capture_records.is_empty():
        var capture_record: Dictionary = capture_records[0]
        var capture_node: Node3D = adapter.capture_point(str(capture_record.id))
        check(capture_node != null and capture_node.position == capture_record.position and
            not bool(capture_node.get_meta("native_filled")),
            "capture-point stable ID, transform, and empty state come from the native snapshot")
        if capture_node != null:
            check((capture_node.get_node("Surround") as MeshInstance3D).mesh is BoxMesh and
                ((capture_node.get_node("Surround") as MeshInstance3D).mesh as BoxMesh).size == Vector3(0.72, 0.72, 0.06) and
                ((capture_node.get_node("Recess") as MeshInstance3D).mesh as BoxMesh).size == Vector3(0.52, 0.52, 0.08) and
                not (capture_node.get_node("Soul") as MeshInstance3D).visible,
                "Godot recovers the desktop capture surround/recess dimensions and hides an unfilled soul")
    var lawn_records: Array = []
    var backdrop_trees: Array = snapshot.get("field_backdrop_trees", [])
    check(backdrop_trees.size() == 9,
        "native Field snapshot supplies the nine desktop perimeter tree records")
    check(adapter.field_backdrop_tree_count() == backdrop_trees.size(),
        "Godot materializes every native Field backdrop tree as render-only mesh geometry")
    if not backdrop_trees.is_empty():
        var first_backdrop: Dictionary = backdrop_trees[0]
        var first_backdrop_node: MeshInstance3D = adapter.field_backdrop_tree(str(first_backdrop.id))
        check(first_backdrop_node != null and first_backdrop_node.mesh is ArrayMesh and
            first_backdrop_node.position.distance_to(first_backdrop.origin) <= 0.000001 and
            first_backdrop_node.scale.distance_to(first_backdrop.scale) <= 0.000001 and
            str(first_backdrop_node.get_meta("native_model")) == str(first_backdrop.model) and
            not bool(first_backdrop_node.get_meta("native_physical")),
            "Field backdrop tree keeps its native mesh family, transform, and render-only ownership")
    var rock_records: Array = []
    for object in snapshot.objects as Array:
        if int(object.primitive) == 2 and int(object.geometry) == 0:
            lawn_records.append(object)
        if int(object.geometry) == 1:
            rock_records.append(object)
    check(adapter.structure_count() == (snapshot.structures as Array).size(),
        "adapter creates one render node per native structure")
    check(not lawn_records.is_empty(),
        "native current-room snapshot contains the selected box-geometry lawn family")
    var grass_blades: Array = snapshot.grass_blades
    var grass_tips: Array = snapshot.grass_tips
    check(bool(snapshot.grass) and grass_blades.size() == 160,
        "native field generator supplies the established low-detail grass blade set")
    check(grass_tips.size() == grass_blades.size(),
        "native grass reaction contract supplies a state-driven tip for each blade")
    check(adapter.grass_blade_count() == grass_blades.size(),
        "Godot materializes every native grass blade as render-only geometry")
    if adapter.grass_blade_count() > 0 and not grass_blades.is_empty():
        var first_blade: Dictionary = grass_blades[0]
        var first_node: MeshInstance3D = adapter.grass_blade(0)
        var native_root: Vector3 = first_blade.root
        var native_tip: Vector3 = grass_tips[0] if not grass_tips.is_empty() else native_root + Vector3.UP * float(first_blade.height)
        check(first_node != null and first_node.position == native_root and
            absf(first_node.scale.y - native_tip.distance_to(native_root)) <= 0.000001 and
            absf(float(first_node.get_meta("native_width")) - float(first_blade.width)) <= 0.000001,
            "first Godot grass blade preserves native root/width and reaction tip length")
        if first_node != null:
            check(first_node.get_meta("native_tip") == native_tip and
                first_node.get_class() == "MeshInstance3D",
                "native bent tip drives a render-only Godot blade with no collision")
    check(adapter.lawn_object_count() == lawn_records.size(),
        "adapter renders every native lawn fragment and no other computed-object family")
    check(not rock_records.is_empty(),
        "native current-room fixture contains a source-generated faceted rock")
    check(adapter.rock_object_count() == rock_records.size(),
        "adapter materializes every native faceted-mesh object record")
    if not rock_records.is_empty():
        var rock_record: Dictionary = rock_records[0]
        var rock_node: MeshInstance3D = adapter.rock_object(str(rock_record.id))
        check(rock_node != null, "adapter preserves native rock stable ID")
        if rock_node != null:
            var rock_mesh := rock_node.mesh as ArrayMesh
            check(rock_mesh != null and rock_mesh.get_surface_count() == 1,
                "native rock presentation becomes one Godot triangle surface")
            if rock_mesh != null and rock_mesh.get_surface_count() == 1:
                var rock_arrays: Array = rock_mesh.surface_get_arrays(0)
                var rendered_vertices: PackedVector3Array = rock_arrays[Mesh.ARRAY_VERTEX]
                var rendered_normals: PackedVector3Array = rock_arrays[Mesh.ARRAY_NORMAL]
                var native_vertices: PackedVector3Array = rock_record.visual_mesh_vertices
                var native_normals: PackedVector3Array = rock_record.visual_mesh_normals
                var geometry_matches := rendered_vertices.size() == native_vertices.size() and \
                    rendered_normals.size() == native_normals.size()
                var max_geometry_error := 0.0
                var max_geometry_index := -1
                if geometry_matches:
                    for vertex in range(native_vertices.size()):
                        var vertex_error := rendered_vertices[vertex].distance_to(native_vertices[vertex])
                        var normal_error := rendered_normals[vertex].distance_to(native_normals[vertex])
                        if vertex_error > max_geometry_error:
                            max_geometry_error = vertex_error
                            max_geometry_index = vertex
                        if normal_error > max_geometry_error:
                            max_geometry_error = normal_error
                            max_geometry_index = vertex
                        geometry_matches = geometry_matches and \
                            rendered_vertices[vertex].distance_to(native_vertices[vertex]) <= 0.000001 and \
                            rendered_normals[vertex].distance_to(native_normals[vertex]) <= 0.0001
                if not geometry_matches:
                    print("ROCK_GEOMETRY_DEBUG rendered_vertices=%d native_vertices=%d rendered_normals=%d native_normals=%d" % [
                        rendered_vertices.size(), native_vertices.size(),
                        rendered_normals.size(), native_normals.size()])
                    if rendered_vertices.size() > 0 and native_vertices.size() > 0:
                        print("ROCK_GEOMETRY_FIRST rendered=%s native=%s" % [
                            str(rendered_vertices[0]), str(native_vertices[0])])
                    print("ROCK_GEOMETRY_MAX_ERROR=%.9f" % max_geometry_error)
                    if max_geometry_index >= 0:
                        print("ROCK_GEOMETRY_MAX_INDEX=%d vertex=%s/%s normal=%s/%s" % [
                            max_geometry_index, str(rendered_vertices[max_geometry_index]),
                            str(native_vertices[max_geometry_index]),
                            str(rendered_normals[max_geometry_index]),
                            str(native_normals[max_geometry_index])])
                check(geometry_matches,
                    "Godot preserves native rock vertices (1e-6) and normalized face normals (1e-4)")
                check(rock_node.position == rock_record.center and
                    rock_node.rotation == Vector3.ZERO,
                    "rock world placement derives from its native object record")
                check(rock_node.get_class() == "MeshInstance3D" and
                    bool(rock_node.get_meta("native_physical")),
                    "rock presentation adds no Godot collision or gameplay authority")
                var rock_material := rock_node.material_override as StandardMaterial3D
                check(rock_material.albedo_texture == adapter.native_material_texture("stone") and
                    rock_material.uv1_triplanar and
                    absf(rock_material.uv1_scale.x - 1.0 / 1.35) <= 0.000001,
                    "native rock uses tiled source stone pixels at the native world scale")
    if not lawn_records.is_empty():
        var lawn_record: Dictionary = lawn_records[0]
        var lawn_node: MeshInstance3D = adapter.lawn_object(str(lawn_record.id))
        check(lawn_node != null, "adapter preserves native lawn object ID")
        if lawn_node != null:
            check((lawn_node.mesh as BoxMesh).size == lawn_record.size,
                "lawn mesh uses native dimensions")
            check(lawn_node.position == lawn_record.center,
                "lawn mesh uses native center")
            check(absf(lawn_node.rotation.y - float(lawn_record.yaw)) <= 0.000001,
                "lawn mesh uses native yaw")
            var lawn_material := lawn_node.material_override as StandardMaterial3D
            check(lawn_material.albedo_texture == adapter.native_material_texture("field_grass") and
                lawn_material.albedo_color.is_equal_approx(lawn_record.visual_color) and
                lawn_material.uv1_triplanar and
                absf(lawn_material.uv1_scale.x - 1.0 / float(lawn_record.visual_texture_world_scale)) <= 0.000001 and
                absf(lawn_material.uv1_offset.x - float(lawn_record.visual_texture_phase)) <= 0.000001,
                "lawn fragment uses native grass pixels, tint, world scale, and variant phase")
            check(lawn_node.get_class() == "MeshInstance3D",
                "render-only lawn node introduces no Godot collision")
    var floor: MeshInstance3D = adapter.structure("room/0/structure/floor")
    var field_floor_definition: Dictionary = {}
    for structure_record in snapshot.structures as Array:
        if str(structure_record.id).ends_with("/floor"):
            field_floor_definition = structure_record.material
    check(floor != null, "adapter preserves native stable floor ID")
    if floor != null:
        check((floor.mesh as BoxMesh).size == Vector3(26.0, 0.08, 35.0),
            "adapter uses the authoritative finite Field-island top dimensions")
        var material := floor.material_override as StandardMaterial3D
        var field_shininess := float(field_floor_definition.shininess)
        var field_specular: Color = field_floor_definition.specular
        check(absf(material.roughness - sqrt(2.0 / (field_shininess + 2.0))) <= 0.000001 and
            material.get_meta("native_specular") == field_specular,
            "Field floor PBR response is mapped from the native specular/shininess contract")
        check(absf(material.albedo_color.r - 63.0 / 255.0) <= 0.000001,
            "adapter uses native field-ground color")
        check(int(material.get_meta("native_texture")) == 1,
            "adapter retains native generated-texture identity")
        var grass_texture := material.albedo_texture as ImageTexture
        check(grass_texture != null,
            "native FieldGrass identity resolves to a rendered Godot texture")
        if grass_texture != null:
            var grass_image := grass_texture.get_image()
            check(grass_image.get_width() == 64 and grass_image.get_height() == 64 and
                grass_image.get_data() == grass_pixels,
                "Godot floor texture pixels are the native generated RGB texture")
            check(material.uv1_triplanar and
                absf(material.uv1_scale.x - 1.0 / 6.4) <= 0.000001,
                "native 6.4 world-scale texture is projected at the native scale")
    runtime.start_city_room_fixture()
    var city_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(city_snapshot)
    var facade_modules: Array = city_snapshot.city_facade_modules
    check(not facade_modules.is_empty(),
        "native City obstacle grammar selects mature authored facade modules")
    check(adapter.city_facade_module_count() == facade_modules.size(),
        "Godot materializes every native City facade module")
    for module_record in facade_modules:
        var facade_node: MeshInstance3D = adapter.city_facade_module(str(module_record.id))
        check(facade_node != null and facade_node.get_class() == "MeshInstance3D",
            "authored City module remains render-only under its native stable ID")
        if facade_node != null:
            var expected_origin: Vector3 = module_record.origin
            var expected_scale: Vector3 = module_record.scale
            var facade_mesh := facade_node.mesh as ArrayMesh
            check(str(facade_node.get_meta("native_model")) == str(module_record.model) and
                facade_node.position.distance_to(expected_origin) <= 0.000001 and
                facade_node.scale.distance_to(expected_scale) <= 0.000001 and
                absf(facade_node.rotation.y - float(module_record.yaw)) <= 0.000001,
                "Godot preserves native authored-module selection and transform")
            check(facade_mesh != null and facade_mesh.get_surface_count() > 0,
                "native DBM1 facade asset becomes a batched Godot ArrayMesh")
            if facade_mesh != null and facade_mesh.get_surface_count() > 0:
                var first_surface: Array = facade_mesh.surface_get_arrays(0)
                check((first_surface[Mesh.ARRAY_VERTEX] as PackedVector3Array).size() > 0 and
                    facade_mesh.surface_get_material(0) is StandardMaterial3D,
                    "authored facade triangle batch and source material are retained")
    var city_street_records: Array = []
    var city_mass_records: Array = []
    var city_skyline_records: Array = []
    var city_window_records: Array = []
    var city_lamp_records: Array = []
    var city_service_records: Array = []
    for structure_record in city_snapshot.structures as Array:
        if int(structure_record.kind) == 5:
            city_street_records.append(structure_record)
        elif int(structure_record.kind) == 6:
            city_mass_records.append(structure_record)
        elif int(structure_record.kind) == 7:
            city_skyline_records.append(structure_record)
        elif int(structure_record.kind) == 8:
            city_window_records.append(structure_record)
        elif int(structure_record.kind) == 9:
            city_lamp_records.append(structure_record)
        elif int(structure_record.kind) == 10:
            city_service_records.append(structure_record)
    check(not city_street_records.is_empty(),
        "native City street grammar supplies deterministic road and frontage surfaces")
    for structure_record in city_street_records:
        var street_node: MeshInstance3D = adapter.structure(str(structure_record.id))
        check(street_node != null and street_node.get_class() == "MeshInstance3D",
            "Godot materializes each native City street record as render-only geometry")
        if street_node != null:
            var expected_center: Vector3 = structure_record.center
            var expected_size: Vector3 = structure_record.size
            var street_mesh := street_node.mesh as BoxMesh
            check(street_node.position.distance_to(expected_center) <= 0.000001 and
                street_mesh != null and street_mesh.size.distance_to(expected_size) <= 0.000001,
                "City street mesh preserves native position and dimensions")
    check(city_mass_records.size() == 10,
        "City fixture presents all ten authoritative native obstacle masses")
    for mass_record in city_mass_records:
        var mass_node: MeshInstance3D = adapter.structure(str(mass_record.id))
        var mass_center: Vector3 = mass_record.center
        var mass_size: Vector3 = mass_record.size
        var mass_color: Color = mass_record.material.base_color
        var mass_mesh := mass_node.mesh as BoxMesh if mass_node != null else null
        check(mass_node != null and mass_node.get_class() == "MeshInstance3D" and
            mass_node.position.distance_to(mass_center) <= 0.000001 and
            mass_mesh != null and mass_mesh.size.distance_to(mass_size) <= 0.000001,
            "native City obstacle mass dimensions and center become render-only geometry")
        if mass_node != null:
            var mass_material := mass_node.material_override as StandardMaterial3D
            check(mass_material != null and mass_material.albedo_color == mass_color and
                int(mass_node.get_meta("native_kind")) == 6,
                "native City building mass retains the desktop plaster color mapping")
    check(not city_skyline_records.is_empty(),
        "native City street grammar supplies deterministic skyline bands and caps")
    for skyline_record in city_skyline_records:
        var skyline_node: MeshInstance3D = adapter.structure(str(skyline_record.id))
        var skyline_center: Vector3 = skyline_record.center
        var skyline_size: Vector3 = skyline_record.size
        var skyline_mesh := skyline_node.mesh as BoxMesh if skyline_node != null else null
        check(skyline_node != null and skyline_node.get_class() == "MeshInstance3D" and
            skyline_node.position.distance_to(skyline_center) <= 0.000001 and
            skyline_mesh != null and skyline_mesh.size.distance_to(skyline_size) <= 0.000001,
            "native skyline body/cap dimensions and placement are rendered without gameplay collision")
        if skyline_node != null:
            var skyline_material := skyline_node.material_override as StandardMaterial3D
            check(skyline_material != null and
                skyline_material.albedo_color == skyline_record.material.base_color and
                int(skyline_node.get_meta("native_kind")) == 7,
                "native skyline family selects the desktop's corresponding source color")
    check(not city_window_records.is_empty(),
        "native City fixture generates deterministic occupied and dark skyline windows")
    for window_record in city_window_records:
        var window_node: MeshInstance3D = adapter.structure(str(window_record.id))
        var window_center: Vector3 = window_record.center
        var window_size: Vector3 = window_record.size
        var window_mesh := window_node.mesh as BoxMesh if window_node != null else null
        check(window_node != null and window_node.get_class() == "MeshInstance3D" and
            window_node.position.distance_to(window_center) <= 0.000001 and
            window_mesh != null and window_mesh.size.distance_to(window_size) <= 0.000001,
            "native skyline window dimensions and positions remain render-only")
        if window_node != null:
            var window_material := window_node.material_override as StandardMaterial3D
            check(window_material != null and
                window_material.albedo_color == window_record.material.base_color and
                window_material.shading_mode == BaseMaterial3D.SHADING_MODE_UNSHADED and
                window_material.disable_fog and not bool(window_record.material.fog),
                "native occupied-window warmth and unlit visibility map to Godot material state")
    check(not city_lamp_records.is_empty(),
        "native City street plan supplies quality-scaled lamps")
    for lamp_record in city_lamp_records:
        var lamp_node: MeshInstance3D = adapter.structure(str(lamp_record.id))
        var lamp_center: Vector3 = lamp_record.center
        var lamp_size: Vector3 = lamp_record.size
        var lamp_mesh := lamp_node.mesh as BoxMesh if lamp_node != null else null
        check(lamp_node != null and lamp_node.get_class() == "MeshInstance3D" and
            lamp_node.position.distance_to(lamp_center) <= 0.000001 and
            lamp_mesh != null and lamp_mesh.size.distance_to(lamp_size) <= 0.000001,
            "native lamp component transforms remain render-only")
        if lamp_node != null:
            var lamp_material := lamp_node.material_override as StandardMaterial3D
            check(lamp_material != null and
                lamp_material.albedo_color == lamp_record.material.base_color and
                lamp_material.disable_fog == (not bool(lamp_record.material.fog)) and
                int(lamp_node.get_meta("native_kind")) == 9,
                "native lamp component colors and fog state are preserved")
    check(not city_service_records.is_empty(),
        "native City street plan supplies deterministic service clusters")
    for service_record in city_service_records:
        var service_node: MeshInstance3D = adapter.structure(str(service_record.id))
        var service_center: Vector3 = service_record.center
        var service_size: Vector3 = service_record.size
        var service_mesh := service_node.mesh as BoxMesh if service_node != null else null
        check(service_node != null and service_node.get_class() == "MeshInstance3D" and
            service_node.position.distance_to(service_center) <= 0.000001 and
            service_mesh != null and service_mesh.size.distance_to(service_size) <= 0.000001,
            "native service cluster component transforms remain render-only")
        if service_node != null:
            var service_material := service_node.material_override as StandardMaterial3D
            check(service_material != null and
                service_material.albedo_color == service_record.material.base_color and
                service_material.disable_fog == (not bool(service_record.material.fog)) and
                int(service_node.get_meta("native_kind")) == 10,
                "native service cluster panel/vent/lamp colors remain source-aligned")
    check(adapter.find_children("", "CollisionObject3D", true, false).is_empty(),
        "City street and facade presentation add no Godot collision; native support remains authoritative")
    var city_floor := adapter.structure("room/%d/structure/floor" % int(city_snapshot.tile_index))
    check(city_floor != null, "city adapter preserves native stable floor ID")
    if city_floor != null:
        var asphalt_material := city_floor.material_override as StandardMaterial3D
        check(int(asphalt_material.get_meta("native_texture")) == 2,
            "native CityAsphalt texture identity is retained on the generated city floor")
        var asphalt_texture := asphalt_material.albedo_texture as ImageTexture
        check(asphalt_texture != null,
            "native CityAsphalt identity resolves to a rendered Godot texture")
        if asphalt_texture != null:
            var asphalt_image := asphalt_texture.get_image()
            check(asphalt_image.get_width() == 32 and asphalt_image.get_height() == 32 and
                asphalt_image.get_data() == asphalt_pixels,
                "Godot city floor texture pixels are the native generated RGB texture")
            check(asphalt_material.uv1_triplanar and
                absf(asphalt_material.uv1_scale.x - 1.0 / 3.2) <= 0.000001,
                "native 3.2 world-scale asphalt texture is projected at the native scale")
    var house_records: Array = []
    for object in city_snapshot.objects as Array:
        if int(object.primitive) == 0:
            house_records.append(object)
    check(int(city_snapshot.setting) == 1 and not house_records.is_empty(),
        "native generated city fixture contains native HouseGeometry parts")
    if int(city_snapshot.setting) != 1 or house_records.is_empty():
        print("CITY_FIXTURE_DEBUG seed=%d room=%d setting=%d object_count=%d house_count=%d" % [
            city_snapshot.room_seed, city_snapshot.room_index, city_snapshot.setting,
            (city_snapshot.objects as Array).size(), house_records.size()])
    check(adapter.house_object_count() == house_records.size(),
        "adapter renders every native house part record")
    var textured_house_record: Dictionary = {}
    for record in house_records:
        if str(record.visual_texture) in ["stone", "metal"]:
            textured_house_record = record
            break
    check(not textured_house_record.is_empty(),
        "native City house fixture includes a textured structural surface")
    if not textured_house_record.is_empty():
        var textured_house_node: MeshInstance3D = adapter.house_object(str(textured_house_record.id))
        var textured_house_material := textured_house_node.material_override as StandardMaterial3D
        check(textured_house_material.albedo_texture ==
            adapter.native_material_texture(str(textured_house_record.visual_texture)) and
            textured_house_material.uv1_triplanar and
            absf(textured_house_material.uv1_scale.x -
                1.0 / float(textured_house_record.visual_texture_world_scale)) <= 0.000001,
            "native City house surface uses its source metal map at the native world scale")
    if not house_records.is_empty():
        var house_record: Dictionary = house_records[0]
        var house_node: MeshInstance3D = adapter.house_object(str(house_record.id))
        check(house_node != null, "house adapter preserves native stable part ID")
        if house_node != null:
            var house_material := house_node.material_override as StandardMaterial3D
            check((house_node.mesh as BoxMesh).size == house_record.size and
                house_node.position == house_record.center and
                absf(house_node.rotation.y - float(house_record.yaw)) <= 0.000001,
                "house part dimensions, center, and yaw come from native geometry")
            check(house_material.albedo_color == house_record.visual_color,
                "house surface uses source-renderer color mapping from native snapshot")
            check(house_material.get_meta("native_texture") == house_record.visual_texture,
                "house stone/metal texture identity is retained without substituted pixels")
            check(bool(house_node.get_meta("native_physical")) == bool(house_record.physical),
                "house physical classification is preserved as metadata, not Godot collision")
            check(house_node.get_class() == "MeshInstance3D",
                "house presentation adds no Godot collision body")
    runtime.start_sterile_room_fixture()
    var sterile_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(sterile_snapshot)
    var marker_records: Array = []
    for object in sterile_snapshot.objects as Array:
        if int(object.primitive) == 3:
            marker_records.append(object)
    if int(sterile_snapshot.setting) != 2 or marker_records.size() != 8:
        print("STERILE_FIXTURE_DEBUG seed=%d room=%d setting=%d form=%d objects=%d markers=%d" % [
            sterile_snapshot.room_seed, sterile_snapshot.room_index, sterile_snapshot.setting,
            sterile_snapshot.form, (sterile_snapshot.objects as Array).size(), marker_records.size()])
    check(int(sterile_snapshot.setting) == 2 and marker_records.size() == 8,
        "native sterile-chamber fixture supplies four two-part MarkerPillarGeometry props")
    var sterile_floor: MeshInstance3D = adapter.structure(
        "room/%d/structure/floor" % int(sterile_snapshot.tile_index))
    var sterile_ceiling: MeshInstance3D = adapter.structure(
        "room/%d/structure/ceiling" % int(sterile_snapshot.tile_index))
    var sterile_floor_record: Dictionary = {}
    var sterile_ceiling_record: Dictionary = {}
    for structure_record in sterile_snapshot.structures as Array:
        if str(structure_record.id).ends_with("/floor"):
            sterile_floor_record = structure_record
        elif str(structure_record.id).ends_with("/ceiling"):
            sterile_ceiling_record = structure_record
    check(sterile_floor != null and sterile_ceiling != null and
        sterile_floor.material_override is StandardMaterial3D and
        sterile_ceiling.material_override is StandardMaterial3D and
        (sterile_floor.material_override as StandardMaterial3D).albedo_color ==
            sterile_floor_record.material.base_color and
        (sterile_ceiling.material_override as StandardMaterial3D).albedo_color ==
            sterile_ceiling_record.material.base_color,
        "Sterile chamber floor and ceiling preserve their native panel color contracts")
    if sterile_floor != null:
        var sterile_material := sterile_floor.material_override as StandardMaterial3D
        var sterile_shininess := float(sterile_floor_record.material.shininess)
        check(absf(sterile_material.roughness - sqrt(2.0 / (sterile_shininess + 2.0))) <= 0.000001 and
            sterile_material.get_meta("native_specular") == sterile_floor_record.material.specular,
            "Sterile panel roughness/specular strength derive from the native material response")
    check(adapter.marker_object_count() == marker_records.size(),
        "adapter renders each native marker pillar part")
    if not marker_records.is_empty():
        var marker_record: Dictionary = marker_records[0]
        var marker_node: MeshInstance3D = adapter.marker_object(str(marker_record.id))
        check(marker_node != null, "marker adapter preserves native stable part ID")
        if marker_node != null:
            var marker_material := marker_node.material_override as StandardMaterial3D
            check((marker_node.mesh as BoxMesh).size == marker_record.size and
                marker_node.position == marker_record.center,
                "marker dimensions and center come from native geometry")
            check(marker_material.albedo_color == marker_record.visual_color and
                marker_material.get_meta("native_texture") == marker_record.visual_texture,
                "marker surface color and metal texture identity come from native renderer mapping")
            check(bool(marker_node.get_meta("native_physical")) and
                marker_node.get_class() == "MeshInstance3D",
                "physical marker remains render-only in Godot")
    runtime.start_ruin_room_fixture()
    var ruin_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(ruin_snapshot)
    var ruin_records: Array = []
    for object in ruin_snapshot.objects as Array:
        if int(object.primitive) == 4:
            ruin_records.append(object)
    check(int(ruin_snapshot.setting) == 0 and ruin_records.size() == 4,
        "native field fixture supplies two two-part RuinGeometry props")
    check(adapter.ruin_object_count() == ruin_records.size(),
        "adapter renders every native ruin part")
    if not ruin_records.is_empty():
        var ruin_record: Dictionary = ruin_records[0]
        var ruin_node: MeshInstance3D = adapter.ruin_object(str(ruin_record.id))
        check(ruin_node != null, "ruin adapter preserves native stable part ID")
        if ruin_node != null:
            var ruin_material := ruin_node.material_override as StandardMaterial3D
            check((ruin_node.mesh as BoxMesh).size == ruin_record.size and
                ruin_node.position == ruin_record.center,
                "ruin dimensions and center come from native geometry")
            check(ruin_material.albedo_color == ruin_record.visual_color and
                ruin_material.get_meta("native_texture") == "stone" and
                absf(float(ruin_material.get_meta("native_texture_phase")) -
                    float(ruin_record.visual_texture_phase)) <= 0.000001,
                "ruin preserves source-renderer weather tint, texture identity, and phase")
            check(ruin_material.albedo_texture == adapter.native_material_texture("stone") and
                ruin_material.uv1_triplanar,
                "native ruin stone texture pixels are rendered with triplanar projection")
            if bool(ruin_record.visual_has_scar):
                var scar := ruin_node.get_node_or_null("NativeScar") as MeshInstance3D
                check(scar != null and (scar.mesh as BoxMesh).size == ruin_record.visual_scar_size and
                    scar.position == ruin_record.visual_scar_center - ruin_record.center,
                    "render-only ruin scar geometry matches native draw record")
            check(ruin_node.get_class() == "MeshInstance3D",
                "native ruin prop remains render-only in Godot")
    runtime.start_coastal_room_fixture()
    for _frame in range(60):
        runtime.submit_input(1.0, 0.0, 0.0, 0.0, false, true, false, false, false, false)
        runtime.tick()
    var coastal_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(coastal_snapshot)
    var coastal_water: Dictionary = coastal_snapshot.coastal_water
    var water_node: MeshInstance3D = adapter.coastal_water_node()
    var water_mesh := water_node.mesh as PlaneMesh if water_node != null else null
    var water_material := water_node.material_override as ShaderMaterial if water_node != null else null
    check(int(coastal_snapshot.setting) == 3 and bool(coastal_water.enabled) and
        water_node != null and water_node.visible and water_mesh != null,
        "native Coastal shore state creates a dedicated visible render-only water surface")
    if water_mesh != null and water_material != null:
        check(absf(water_mesh.size.x - (float(coastal_water.far_x) - float(coastal_water.shore_x))) <= 0.000001 and
            absf(water_mesh.size.y - (float(coastal_water.max_z) - float(coastal_water.min_z))) <= 0.000001 and
            bool(water_node.get_meta("presentation_only")) and
            absf(float(water_material.get_shader_parameter("native_time")) - float(coastal_water.native_time)) <= 0.000001,
            "Coastal mesh bounds and shader time come from the native shoreline snapshot")
        var source_position: Vector3 = coastal_water.source_position
        check(water_material.shader != null and
            water_material.get_shader_parameter("source_position") == Vector2(source_position.x, source_position.z) and
            absf(float(water_material.get_shader_parameter("source_speed")) - float(coastal_water.source_speed)) <= 0.000001 and
            bool(water_material.get_shader_parameter("source_active")) == bool(coastal_water.source_active) and
            bool(coastal_water.source_active),
            "native player entry/speed state activates the Coastal wake presentation")
        check(water_material.get_shader_parameter("shore_bounds") == Vector2(
            float(coastal_water.shore_x), float(coastal_water.far_x)),
            "native shoreline bounds drive shallow-to-deep water shading")
        check(water_node.get_class() == "MeshInstance3D" and
            adapter.find_children("", "CollisionObject3D", true, false).is_empty(),
            "Coastal water adds no Godot collision or gameplay authority")
    adapter.free()
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native room structures, City presentation, reactive Field grass, Coastal water, Sterile surfaces, environment props, source textures, and render-only geometry")
    quit(0)
