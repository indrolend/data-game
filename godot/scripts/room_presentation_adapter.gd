extends Node3D
## Render-only materialization of native room structural records.
## Nodes have no collision and never feed transforms back into the simulation.

var _nodes: Dictionary = {}
var _lawn_nodes: Dictionary = {}
var _rock_nodes: Dictionary = {}
var _house_nodes: Dictionary = {}
var _marker_nodes: Dictionary = {}
var _ruin_nodes: Dictionary = {}
var _field_grass_texture: ImageTexture
var _field_grass_image: Image
var _city_asphalt_texture: ImageTexture
var _native_material_textures: Dictionary = {}
var _city_facade_nodes: Dictionary = {}
var _field_backdrop_tree_nodes: Dictionary = {}
var _capture_point_nodes: Dictionary = {}
var _native_tree_meshes: Dictionary = {}
var _city_facade_meshes: Dictionary = {}
var _grass_node: Node3D
var _grass_snapshot_key := ""
var _grass_blade_records: Array = []
var _coastal_water: MeshInstance3D
var _coastal_water_material: ShaderMaterial

func set_native_field_grass_pixels(pixels: PackedByteArray) -> void:
    if _field_grass_texture != null:
        return
    if pixels.size() != 64 * 64 * 3:
        push_error("Native field-grass texture must contain 64x64 RGB pixels")
        return
    var image := Image.create_from_data(64, 64, false, Image.FORMAT_RGB8, pixels)
    _field_grass_image = image
    _field_grass_texture = ImageTexture.create_from_image(image)

func set_native_city_asphalt_pixels(pixels: PackedByteArray) -> void:
    if _city_asphalt_texture != null:
        return
    if pixels.size() != 32 * 32 * 3:
        push_error("Native city-asphalt texture must contain 32x32 RGB pixels")
        return
    var image := Image.create_from_data(32, 32, false, Image.FORMAT_RGB8, pixels)
    _city_asphalt_texture = ImageTexture.create_from_image(image)

func native_material_texture(material_name: String) -> ImageTexture:
    if material_name == "field_grass":
        return _field_grass_texture
    if _native_material_textures.has(material_name):
        return _native_material_textures[material_name]
    if material_name not in ["stone", "metal"]:
        return null
    var material_path := ProjectSettings.globalize_path(
        "res://../parent/models/materials/photo-%s-runtime.rgb" % material_name)
    var file := FileAccess.open(material_path, FileAccess.READ)
    if file == null:
        push_error("Could not open native runtime material: " + material_path)
        return null
    var pixels := file.get_buffer(file.get_length())
    file.close()
    if pixels.size() != 512 * 512 * 3:
        push_error("Native runtime material must contain 512x512 RGB pixels: " + material_path)
        return null
    var image := Image.create_from_data(512, 512, false, Image.FORMAT_RGB8, pixels)
    var texture := ImageTexture.create_from_image(image)
    _native_material_textures[material_name] = texture
    return texture

func _apply_native_material_texture(material: StandardMaterial3D, material_name: String,
        world_scale: float, phase: float = 0.0) -> void:
    material.set_meta("native_texture", material_name)
    material.set_meta("native_texture_world_scale", world_scale)
    material.set_meta("native_texture_phase", phase)
    var texture := native_material_texture(material_name)
    material.albedo_texture = texture
    if texture != null:
        material.uv1_triplanar = true
        material.uv1_scale = Vector3.ONE / maxf(world_scale, 0.05)
        material.uv1_offset = Vector3(phase, fposmod(phase * 1.618, 1.0), 0.0)
    else:
        material.uv1_triplanar = false

func sync_snapshot(snapshot: Dictionary) -> void:
    var retained: Dictionary = {}
    for record in snapshot.structures as Array:
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.mesh = BoxMesh.new()
            add_child(mesh_instance)
            _nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        (mesh_instance.mesh as BoxMesh).size = record.size
        mesh_instance.material_override = _material_from_native(record.material)
        mesh_instance.set_meta("native_kind", int(record.kind))
    for stable_id in _nodes.keys():
        if not retained.has(stable_id):
            (_nodes[stable_id] as Node).queue_free()
            _nodes.erase(stable_id)
    set_meta("native_room_seed", int(snapshot.room_seed))
    set_meta("native_room_index", int(snapshot.room_index))
    set_meta("native_tile_index", int(snapshot.tile_index))
    _sync_lawn_objects(snapshot.objects as Array, int(snapshot.tile_index))
    _sync_capture_points(snapshot.get("capture_points", []), float(snapshot.get("presentation_time", 0.0)))
    _sync_grass(snapshot)
    _sync_coastal_water(snapshot)
    _sync_rock_objects(snapshot.objects as Array)
    _sync_tree_objects(snapshot.objects as Array)
    _sync_field_backdrop_trees(snapshot.get("field_backdrop_trees", []))
    _sync_house_objects(snapshot.objects as Array)
    _sync_marker_objects(snapshot.objects as Array)
    _sync_ruin_objects(snapshot.objects as Array)
    _sync_city_facades(snapshot.get("city_facade_modules", []))

func structure_count() -> int:
    return _nodes.size()

func structure(stable_id: String) -> MeshInstance3D:
    return _nodes.get(stable_id)

func capture_point(stable_id: String) -> Node3D:
    return _capture_point_nodes.get(stable_id)

func capture_point_count() -> int:
    return _capture_point_nodes.size()

func _sync_capture_points(records: Array, native_time: float) -> void:
    var retained: Dictionary = {}
    for record in records:
        var stable_id := str(record.id)
        retained[stable_id] = true
        var root: Node3D = _capture_point_nodes.get(stable_id)
        if root == null:
            root = Node3D.new()
            root.name = stable_id.replace("/", "_")
            root.set_meta("native_id", stable_id)
            root.set_meta("presentation_only", true)
            for part_name in ["Surround", "Recess", "Soul"]:
                var part := MeshInstance3D.new()
                part.name = part_name
                part.mesh = BoxMesh.new()
                root.add_child(part)
            add_child(root)
            _capture_point_nodes[stable_id] = root
        root.position = record.position
        root.set_meta("native_filled", bool(record.filled))
        var surround := root.get_node("Surround") as MeshInstance3D
        var recess := root.get_node("Recess") as MeshInstance3D
        var soul := root.get_node("Soul") as MeshInstance3D
        surround.position = Vector3(0, 0, -0.04)
        (surround.mesh as BoxMesh).size = Vector3(0.72, 0.72, 0.06)
        surround.material_override = _material_from_native(record.surround_material)
        (recess.mesh as BoxMesh).size = Vector3(0.52, 0.52, 0.08)
        recess.material_override = _material_from_native(record.recess_material)
        soul.position = Vector3(0, 0, 0.12)
        soul.rotation = Vector3(native_time * 1.5, native_time * 2.0, native_time)
        (soul.mesh as BoxMesh).size = Vector3(0.36, 0.36, 0.36)
        soul.material_override = _material_from_native(record.soul_material)
        soul.visible = bool(record.filled)
    for stable_id in _capture_point_nodes.keys():
        if not retained.has(stable_id):
            (_capture_point_nodes[stable_id] as Node).queue_free()
            _capture_point_nodes.erase(stable_id)

func lawn_object_count() -> int:
    return _lawn_nodes.size()

func lawn_object(stable_id: String) -> MeshInstance3D:
    return _lawn_nodes.get(stable_id)

func grass_blade_count() -> int:
    return _grass_node.get_child_count() if _grass_node != null else 0

func grass_blade(index: int) -> MeshInstance3D:
    return _grass_node.get_child(index) as MeshInstance3D if _grass_node != null else null

func coastal_water_node() -> MeshInstance3D:
    return _coastal_water

func _sync_grass(snapshot: Dictionary) -> void:
    var key := "%d:%d:%d:%d" % [int(snapshot.room_seed), int(snapshot.room_index),
        int(snapshot.tile_index), int(snapshot.grass)]
    var rebuild := key != _grass_snapshot_key
    if rebuild:
        _grass_snapshot_key = key
    if _grass_node == null:
        _grass_node = Node3D.new()
        _grass_node.name = "NativeFieldGrass"
        _grass_node.set_meta("presentation_only", true)
        add_child(_grass_node)
    var blades: Array = snapshot.get("grass_blades", [])
    if rebuild:
        for old_blade in _grass_node.get_children():
            _grass_node.remove_child(old_blade)
            old_blade.free()
        _grass_blade_records = blades.duplicate()
    if not bool(snapshot.grass) or (rebuild and blades.is_empty()):
        _grass_node.visible = false
        return
    if rebuild:
        var blade_mesh := _grass_blade_mesh()
        var blade_material := StandardMaterial3D.new()
        blade_material.cull_mode = BaseMaterial3D.CULL_DISABLED
        blade_material.vertex_color_use_as_albedo = true
        blade_material.vertex_color_is_srgb = false
        blade_material.albedo_color = Color.WHITE
        blade_material.roughness = 0.93
        blade_mesh.surface_set_material(0, blade_material)
        for index in range(_grass_blade_records.size()):
            var blade: Dictionary = _grass_blade_records[index]
            var blade_node := MeshInstance3D.new()
            blade_node.name = "NativeBlade_%03d" % index
            blade_node.mesh = blade_mesh
            blade_node.set_meta("native_height", float(blade.height))
            blade_node.set_meta("native_width", float(blade.width))
            var tone := float(blade.get("tone", 1.0))
            blade_node.set_meta("native_tone", tone)
            var per_blade_material := blade_material.duplicate() as StandardMaterial3D
            per_blade_material.albedo_color = Color(tone, tone, tone, 1.0)
            blade_node.material_override = per_blade_material
            _grass_node.add_child(blade_node)
    var tips: Array = snapshot.get("grass_tips", [])
    for index in range(mini(_grass_blade_records.size(), _grass_node.get_child_count())):
        var blade: Dictionary = _grass_blade_records[index]
        var blade_node := _grass_node.get_child(index) as MeshInstance3D
        var root_position: Vector3 = blade.root
        var tip_position: Vector3 = tips[index] if index < tips.size() else root_position + Vector3.UP * float(blade.height)
        var tip_delta := tip_position - root_position
        if tip_delta.length_squared() < 0.000001:
            tip_delta = Vector3.UP * float(blade.height)
        blade_node.position = root_position
        blade_node.scale = Vector3(float(blade.width), tip_delta.length(), float(blade.width))
        blade_node.quaternion = Quaternion(Vector3.UP, tip_delta.normalized()) * Quaternion(Vector3.UP, float(blade.phase))
        blade_node.set_meta("native_tip", tip_position)
    _grass_node.visible = true

func _sync_coastal_water(snapshot: Dictionary) -> void:
    var water: Dictionary = snapshot.get("coastal_water", {})
    if not bool(water.get("enabled", false)):
        if _coastal_water != null:
            _coastal_water.visible = false
        return
    if _coastal_water == null:
        var shore_x := float(water.shore_x)
        var far_x := float(water.far_x)
        var min_z := float(water.min_z)
        var max_z := float(water.max_z)
        var plane := PlaneMesh.new()
        plane.size = Vector2(far_x - shore_x, max_z - min_z)
        plane.subdivide_width = 64
        plane.subdivide_depth = 128
        _coastal_water = MeshInstance3D.new()
        _coastal_water.name = "NativeCoastalWater"
        _coastal_water.mesh = plane
        _coastal_water.position = Vector3((shore_x + far_x) * 0.5, 0.0,
            float(water.tile_offset_z) + (min_z + max_z) * 0.5)
        _coastal_water.set_meta("presentation_only", true)
        _coastal_water.set_meta("native_bounds", Vector4(shore_x, far_x, min_z, max_z))
        _coastal_water_material = ShaderMaterial.new()
        _coastal_water_material.shader = load("res://shaders/coastal_water.gdshader")
        _coastal_water.material_override = _coastal_water_material
        add_child(_coastal_water)
    _coastal_water.visible = true
    _coastal_water.position.z = (float(water.tile_offset_z) +
        (float(water.min_z) + float(water.max_z)) * 0.5)
    _coastal_water_material.set_shader_parameter("native_time", float(water.native_time))
    _coastal_water_material.set_shader_parameter("shore_bounds", Vector2(
        float(water.shore_x), float(water.far_x)))
    _coastal_water_material.set_shader_parameter("source_position", Vector2(
        (water.source_position as Vector3).x, (water.source_position as Vector3).z))
    _coastal_water_material.set_shader_parameter("source_speed", float(water.source_speed))
    _coastal_water_material.set_shader_parameter("source_active", bool(water.source_active))

func _grass_blade_mesh() -> ArrayMesh:
    var vertices := PackedVector3Array([
        Vector3(-0.5, 0.0, 0.0), Vector3(0.5, 0.0, 0.0), Vector3(0.0, 1.0, 0.0)])
    # DesktopRenderer draws one phase-oriented triangle with a root-to-tip
    # color ramp. It is intentionally distinct from the tiled Field floor.
    var colors := PackedColorArray([
        Color(0x38 / 255.0, 0x68 / 255.0, 0x40 / 255.0, 1.0),
        Color(0x38 / 255.0, 0x68 / 255.0, 0x40 / 255.0, 1.0),
        Color(0x4d / 255.0, 0x7d / 255.0, 0x4e / 255.0, 1.0)])
    var normals := PackedVector3Array([
        Vector3(0.18, 0.96, 0.0).normalized(), Vector3(0.18, 0.96, 0.0).normalized(),
        Vector3(0.18, 0.96, 0.0).normalized()])
    var arrays: Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
    arrays[Mesh.ARRAY_NORMAL] = normals
    arrays[Mesh.ARRAY_COLOR] = colors
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    return mesh

func rock_object_count() -> int:
    return _rock_nodes.size()

func rock_object(stable_id: String) -> MeshInstance3D:
    return _rock_nodes.get(stable_id)

func house_object_count() -> int:
    return _house_nodes.size()

func house_object(stable_id: String) -> MeshInstance3D:
    return _house_nodes.get(stable_id)

func marker_object(stable_id: String) -> MeshInstance3D:
    return _marker_nodes.get(stable_id)

func marker_object_count() -> int:
    return _marker_nodes.size()

func ruin_object(stable_id: String) -> MeshInstance3D:
    return _ruin_nodes.get(stable_id)

func ruin_object_count() -> int:
    return _ruin_nodes.size()

func city_facade_module_count() -> int:
    return _city_facade_nodes.size()

func city_facade_module(stable_id: String) -> MeshInstance3D:
    return _city_facade_nodes.get(stable_id)

func _sync_city_facades(records: Array) -> void:
    var retained: Dictionary = {}
    for record in records:
        var stable_id := str(record.id)
        retained[stable_id] = true
        var module: MeshInstance3D = _city_facade_nodes.get(stable_id)
        if module == null:
            var model_name := str(record.model)
            var model_mesh: ArrayMesh = _city_facade_meshes.get(model_name)
            if model_mesh == null:
                model_mesh = _load_city_facade_mesh(model_name)
                if model_mesh == null:
                    continue
                _city_facade_meshes[model_name] = model_mesh
            module = MeshInstance3D.new()
            module.name = stable_id.replace("/", "_")
            module.set_meta("native_id", stable_id)
            module.set_meta("native_model", model_name)
            module.set_meta("presentation_only", true)
            module.mesh = model_mesh
            add_child(module)
            _city_facade_nodes[stable_id] = module
        module.position = record.origin
        module.scale = record.scale
        module.rotation = Vector3(0.0, float(record.yaw), 0.0)
    for stable_id in _city_facade_nodes.keys():
        if not retained.has(stable_id):
            (_city_facade_nodes[stable_id] as Node).queue_free()
            _city_facade_nodes.erase(stable_id)

func _load_city_facade_mesh(model_name: String) -> ArrayMesh:
    var project_path := ProjectSettings.globalize_path("res://../parent/models/environment/city")
    var model_path := project_path.path_join(model_name + ".dbmesh")
    var file := FileAccess.open(model_path, FileAccess.READ)
    if file == null:
        push_error("Could not open native City facade mesh: " + model_path)
        return null
    var bytes := file.get_buffer(file.get_length())
    file.close()
    if bytes.size() < 12 or bytes.slice(0, 4).get_string_from_ascii() != "DBM1":
        push_error("Invalid native City facade mesh header: " + model_name)
        return null
    var vertex_count := int(bytes.decode_u32(4))
    var batch_count := int(bytes.decode_u32(8))
    var expected_size := 12 + vertex_count * 12 + batch_count * 24
    if vertex_count <= 0 or vertex_count > 1000000 or batch_count <= 0 or \
        batch_count > 1024 or bytes.size() != expected_size:
        push_error("Invalid native City facade mesh dimensions: " + model_name)
        return null
    var vertex_offset := 12
    var batch_offset := vertex_offset + vertex_count * 12
    var mesh := ArrayMesh.new()
    for batch_index in range(batch_count):
        var batch_at := batch_offset + batch_index * 24
        var first_vertex := int(bytes.decode_u32(batch_at))
        var batch_vertices := int(bytes.decode_u32(batch_at + 4))
        if batch_vertices <= 0 or batch_vertices % 3 != 0 or \
            first_vertex + batch_vertices > vertex_count:
            push_error("Invalid native City facade mesh batch: " + model_name)
            return null
        var vertices := PackedVector3Array()
        var normals := PackedVector3Array()
        vertices.resize(batch_vertices)
        normals.resize(batch_vertices)
        for vertex_index in range(batch_vertices):
            var source_at := vertex_offset + (first_vertex + vertex_index) * 12
            vertices[vertex_index] = Vector3(
                bytes.decode_float(source_at),
                bytes.decode_float(source_at + 4),
                bytes.decode_float(source_at + 8))
        for triangle in range(0, batch_vertices, 3):
            var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
                vertices[triangle + 2] - vertices[triangle]).normalized()
            normals[triangle] = normal
            normals[triangle + 1] = normal
            normals[triangle + 2] = normal
        var arrays: Array = []
        arrays.resize(Mesh.ARRAY_MAX)
        arrays[Mesh.ARRAY_VERTEX] = vertices
        arrays[Mesh.ARRAY_NORMAL] = normals
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
        var material := StandardMaterial3D.new()
        material.albedo_color = Color(
            bytes.decode_float(batch_at + 8),
            bytes.decode_float(batch_at + 12),
            bytes.decode_float(batch_at + 16),
            bytes.decode_float(batch_at + 20))
        mesh.surface_set_material(mesh.get_surface_count() - 1, material)
    return mesh

func field_backdrop_tree_count() -> int:
    return _field_backdrop_tree_nodes.size()

func field_backdrop_tree(stable_id: String) -> MeshInstance3D:
    return _field_backdrop_tree_nodes.get(stable_id)

func _sync_field_backdrop_trees(records: Array) -> void:
    var retained: Dictionary = {}
    for record in records:
        var stable_id := str(record.id)
        retained[stable_id] = true
        var tree: MeshInstance3D = _field_backdrop_tree_nodes.get(stable_id)
        if tree == null:
            var model_name := str(record.model)
            var mesh: ArrayMesh = _native_tree_meshes.get(model_name)
            if mesh == null:
                mesh = _load_field_backdrop_tree_mesh(model_name)
                if mesh == null:
                    continue
                _native_tree_meshes[model_name] = mesh
            tree = MeshInstance3D.new()
            tree.name = stable_id.replace("/", "_")
            tree.set_meta("native_id", stable_id)
            tree.set_meta("native_model", model_name)
            tree.set_meta("native_physical", false)
            tree.set_meta("presentation_only", true)
            tree.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
            tree.mesh = mesh
            add_child(tree)
            _field_backdrop_tree_nodes[stable_id] = tree
        tree.position = record.origin
        tree.scale = record.scale
        tree.quaternion = Basis.from_euler(Vector3(float(record.pitch), float(record.yaw),
            float(record.roll)), EULER_ORDER_XYZ).get_rotation_quaternion()
    for stable_id in _field_backdrop_tree_nodes.keys():
        if not retained.has(stable_id):
            (_field_backdrop_tree_nodes[stable_id] as Node).queue_free()
            _field_backdrop_tree_nodes.erase(stable_id)

func _load_field_backdrop_tree_mesh(model_name: String) -> ArrayMesh:
    if model_name not in ["common-tree", "twisted-tree", "dead-tree", "pine-tree"]:
        push_error("Unsupported native Field backdrop tree: " + model_name)
        return null
    var model_path := ProjectSettings.globalize_path(
        "res://../parent/models/environment/" + model_name + ".dbmesh")
    var file := FileAccess.open(model_path, FileAccess.READ)
    if file == null:
        push_error("Could not open native Field backdrop tree mesh: " + model_path)
        return null
    var bytes := file.get_buffer(file.get_length())
    file.close()
    if bytes.size() < 12 or bytes.slice(0, 4).get_string_from_ascii() != "DBM1":
        push_error("Invalid native Field backdrop tree mesh header: " + model_name)
        return null
    var vertex_count := int(bytes.decode_u32(4))
    var batch_count := int(bytes.decode_u32(8))
    var expected_size := 12 + vertex_count * 12 + batch_count * 24
    if vertex_count <= 0 or vertex_count > 1000000 or batch_count <= 0 or \
            batch_count > 1024 or bytes.size() != expected_size:
        push_error("Invalid native Field backdrop tree mesh dimensions: " + model_name)
        return null
    var vertex_offset := 12
    var batch_offset := vertex_offset + vertex_count * 12
    var mesh := ArrayMesh.new()
    for batch_index in range(batch_count):
        var batch_at := batch_offset + batch_index * 24
        var first_vertex := int(bytes.decode_u32(batch_at))
        var batch_vertices := int(bytes.decode_u32(batch_at + 4))
        if batch_vertices <= 0 or batch_vertices % 3 != 0 or \
                first_vertex + batch_vertices > vertex_count:
            push_error("Invalid native Field backdrop tree mesh batch: " + model_name)
            return null
        var vertices := PackedVector3Array()
        var normals := PackedVector3Array()
        vertices.resize(batch_vertices)
        normals.resize(batch_vertices)
        for vertex_index in range(batch_vertices):
            var source_at := vertex_offset + (first_vertex + vertex_index) * 12
            vertices[vertex_index] = Vector3(bytes.decode_float(source_at),
                bytes.decode_float(source_at + 4), bytes.decode_float(source_at + 8))
        for triangle in range(0, batch_vertices, 3):
            var normal := (vertices[triangle + 1] - vertices[triangle]).cross(
                vertices[triangle + 2] - vertices[triangle]).normalized()
            normals[triangle] = normal
            normals[triangle + 1] = normal
            normals[triangle + 2] = normal
        var arrays: Array = []
        arrays.resize(Mesh.ARRAY_MAX)
        arrays[Mesh.ARRAY_VERTEX] = vertices
        arrays[Mesh.ARRAY_NORMAL] = normals
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
        var material := StandardMaterial3D.new()
        material.albedo_color = Color(bytes.decode_float(batch_at + 8),
            bytes.decode_float(batch_at + 12), bytes.decode_float(batch_at + 16),
            bytes.decode_float(batch_at + 20))
        material.roughness = 0.86
        material.cull_mode = BaseMaterial3D.CULL_DISABLED
        mesh.surface_set_material(mesh.get_surface_count() - 1, material)
    return mesh

func _sync_lawn_objects(objects: Array, tile_index: int) -> void:
    var retained: Dictionary = {}
    for record in objects:
        # EnvironmentPrimitive::LawnFragment == 2 and GeometryKind::Box == 0.
        # Other object families remain unmaterialized until their native visual
        # geometry/material contracts are available.
        if int(record.primitive) != 2 or int(record.geometry) != 0:
            continue
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _lawn_nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.set_meta("native_primitive", int(record.primitive))
            mesh_instance.set_meta("native_part", int(record.part))
            mesh_instance.mesh = BoxMesh.new()
            var material := StandardMaterial3D.new()
            material.roughness = 0.9
            mesh_instance.material_override = material
            add_child(mesh_instance)
            _lawn_nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        mesh_instance.rotation.y = float(record.yaw)
        (mesh_instance.mesh as BoxMesh).size = record.size
        var lawn_material := mesh_instance.material_override as StandardMaterial3D
        lawn_material.albedo_color = record.visual_color
        _apply_native_material_texture(lawn_material, str(record.visual_texture),
            float(record.visual_texture_world_scale), float(record.visual_texture_phase))
        mesh_instance.set_meta("native_physical", bool(record.physical))
    for stable_id in _lawn_nodes.keys():
        if not retained.has(stable_id):
            (_lawn_nodes[stable_id] as Node).queue_free()
            _lawn_nodes.erase(stable_id)

func _sync_rock_objects(objects: Array) -> void:
    var retained: Dictionary = {}
    for record in objects:
        # Faceted rock vertices/normals are generated by the original native
        # faceted_rock::makeVisualMesh routine and passed through the snapshot.
        if int(record.geometry) != 1 or not record.has("visual_mesh_vertices"):
            continue
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _rock_nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.set_meta("native_primitive", int(record.primitive))
            mesh_instance.set_meta("native_part", int(record.part))
            mesh_instance.set_meta("native_geometry", int(record.geometry))
            add_child(mesh_instance)
            _rock_nodes[stable_id] = mesh_instance
        var vertices: PackedVector3Array = record.visual_mesh_vertices
        var normals: PackedVector3Array = record.visual_mesh_normals
        var arrays: Array = []
        arrays.resize(Mesh.ARRAY_MAX)
        arrays[Mesh.ARRAY_VERTEX] = vertices
        arrays[Mesh.ARRAY_NORMAL] = normals
        var mesh := ArrayMesh.new()
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
        mesh_instance.mesh = mesh
        mesh_instance.position = record.center
        mesh_instance.rotation = Vector3.ZERO
        var material := StandardMaterial3D.new()
        material.albedo_color = record.visual_color
        material.roughness = 0.92
        _apply_native_material_texture(material, str(record.get("visual_texture", "stone")),
            float(record.visual_texture_world_scale))
        mesh_instance.material_override = material
        mesh_instance.set_meta("native_physical", bool(record.physical))
    for stable_id in _rock_nodes.keys():
        if not retained.has(stable_id):
            (_rock_nodes[stable_id] as Node).queue_free()
            _rock_nodes.erase(stable_id)

func _sync_house_objects(objects: Array) -> void:
    var retained: Dictionary = {}
    for record in objects:
        # House part extents, yaw and surface identity are native
        # HouseGeometry::parts records; this adapter only materializes them.
        if int(record.primitive) != 0 or int(record.geometry) != 0:
            continue
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _house_nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.set_meta("native_primitive", int(record.primitive))
            mesh_instance.set_meta("native_part", int(record.part))
            mesh_instance.set_meta("native_surface", int(record.surface))
            mesh_instance.mesh = BoxMesh.new()
            var material := StandardMaterial3D.new()
            material.roughness = 0.86
            mesh_instance.material_override = material
            add_child(mesh_instance)
            _house_nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        mesh_instance.rotation.y = float(record.yaw)
        (mesh_instance.mesh as BoxMesh).size = record.size
        var house_material := mesh_instance.material_override as StandardMaterial3D
        house_material.albedo_color = record.visual_color
        _apply_native_material_texture(house_material, str(record.visual_texture),
            float(record.visual_texture_world_scale))
        mesh_instance.set_meta("native_physical", bool(record.physical))
    for stable_id in _house_nodes.keys():
        if not retained.has(stable_id):
            (_house_nodes[stable_id] as Node).queue_free()
            _house_nodes.erase(stable_id)

func _sync_marker_objects(objects: Array) -> void:
    var retained: Dictionary = {}
    for record in objects:
        # MarkerPillar records and their parts come from native geometry; this
        # adapter only creates the corresponding render-only box and material.
        if int(record.primitive) != 3:
            continue
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _marker_nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.set_meta("native_primitive", int(record.primitive))
            mesh_instance.set_meta("native_part", int(record.part))
            mesh_instance.set_meta("native_surface", int(record.surface))
            mesh_instance.mesh = BoxMesh.new()
            var material := StandardMaterial3D.new()
            material.roughness = 0.2 if int(record.surface) == 1 else 0.48
            mesh_instance.material_override = material
            add_child(mesh_instance)
            _marker_nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        (mesh_instance.mesh as BoxMesh).size = record.size
        var marker_material := mesh_instance.material_override as StandardMaterial3D
        marker_material.albedo_color = record.visual_color
        _apply_native_material_texture(marker_material, str(record.visual_texture),
            float(record.visual_texture_world_scale))
        mesh_instance.set_meta("native_physical", bool(record.physical))
    for stable_id in _marker_nodes.keys():
        if not retained.has(stable_id):
            (_marker_nodes[stable_id] as Node).queue_free()
            _marker_nodes.erase(stable_id)

func _sync_ruin_objects(objects: Array) -> void:
    var retained: Dictionary = {}
    for record in objects:
        # Ruin blocks and scar accents are render-only expansions of native
        # RuinGeometry and DesktopRenderer records.
        if int(record.primitive) != 4:
            continue
        var stable_id := str(record.id)
        retained[stable_id] = true
        var mesh_instance: MeshInstance3D = _ruin_nodes.get(stable_id)
        if mesh_instance == null:
            mesh_instance = MeshInstance3D.new()
            mesh_instance.name = stable_id.replace("/", "_")
            mesh_instance.set_meta("native_id", stable_id)
            mesh_instance.set_meta("native_primitive", int(record.primitive))
            mesh_instance.set_meta("native_part", int(record.part))
            mesh_instance.set_meta("native_surface", int(record.surface))
            mesh_instance.mesh = BoxMesh.new()
            var material := StandardMaterial3D.new()
            material.roughness = 0.94
            mesh_instance.material_override = material
            add_child(mesh_instance)
            _ruin_nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        (mesh_instance.mesh as BoxMesh).size = record.size
        var ruin_material := mesh_instance.material_override as StandardMaterial3D
        ruin_material.albedo_color = record.visual_color
        _apply_native_material_texture(ruin_material, str(record.visual_texture),
            float(record.visual_texture_world_scale), float(record.visual_texture_phase))
        var scar: MeshInstance3D = mesh_instance.get_node_or_null("NativeScar")
        if bool(record.visual_has_scar):
            if scar == null:
                scar = MeshInstance3D.new()
                scar.name = "NativeScar"
                scar.mesh = BoxMesh.new()
                var scar_material := StandardMaterial3D.new()
                scar_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
                scar_material.albedo_color = record.visual_scar_color
                scar.material_override = scar_material
                mesh_instance.add_child(scar)
            (scar.mesh as BoxMesh).size = record.visual_scar_size
            scar.position = record.visual_scar_center - record.center
    for stable_id in _ruin_nodes.keys():
        if not retained.has(stable_id):
            (_ruin_nodes[stable_id] as Node).queue_free()
            _ruin_nodes.erase(stable_id)

func _material_from_native(definition: Dictionary) -> StandardMaterial3D:
    var material := StandardMaterial3D.new()
    material.albedo_color = definition.base_color
    var specular_color: Color = definition.specular
    material.metallic_specular = clampf((specular_color.r + specular_color.g + specular_color.b) * 0.8333333, 0.0, 1.0)
    var native_shininess := float(definition.shininess)
    material.roughness = clampf(sqrt(2.0 / (native_shininess + 2.0)), 0.06, 0.98)
    if int(definition.shading) == 0:
        material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    var emission: Color = definition.emission
    if emission.r > 0.0 or emission.g > 0.0 or emission.b > 0.0:
        material.emission_enabled = true
        material.emission = emission
    var opacity := float(definition.opacity)
    if opacity < 0.999:
        material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
        material.albedo_color.a = opacity
    material.set_meta("native_texture", int(definition.texture))
    material.set_meta("native_texture_world_scale", float(definition.texture_world_scale))
    if int(definition.texture) == 1 and _field_grass_texture != null:
        material.albedo_texture = _field_grass_texture
        material.uv1_triplanar = true
        var world_scale := maxf(float(definition.texture_world_scale), 0.001)
        material.uv1_scale = Vector3.ONE / world_scale
    elif int(definition.texture) == 2 and _city_asphalt_texture != null:
        material.albedo_texture = _city_asphalt_texture
        material.uv1_triplanar = true
        var world_scale := maxf(float(definition.texture_world_scale), 0.001)
        material.uv1_scale = Vector3.ONE / world_scale
    material.set_meta("native_shininess", native_shininess)
    material.set_meta("native_specular", specular_color)
    material.set_meta("native_fog", bool(definition.fog))
    material.disable_fog = not bool(definition.fog)
    return material

# Native-driven tree presentation. Geometry and identity originate in
# computed_scene::environmentObjects; no Godot gameplay or collision.
var _tree_nodes: Dictionary = {}
var _native_foliage_texture: ImageTexture
func _load_native_foliage_texture() -> ImageTexture:
    if _native_foliage_texture != null:
        return _native_foliage_texture
    var path := ProjectSettings.globalize_path(
        "res://../parent/models/materials/cc0-fir-foliage.rgba")
    var file := FileAccess.open(path, FileAccess.READ)
    if file == null:
        return null
    var pixels := file.get_buffer(file.get_length())
    file.close()
    var dimension := int(sqrt(float(pixels.size() / 4)))
    if dimension <= 0 or dimension * dimension * 4 != pixels.size():
        push_error("Unsupported native foliage atlas dimensions: " + str(pixels.size()))
        return null
    var image := Image.create_from_data(
        dimension, dimension, false, Image.FORMAT_RGBA8, pixels)
    _native_foliage_texture = ImageTexture.create_from_image(image)
    return _native_foliage_texture
func _sync_tree_objects(objects: Array) -> void:
    var retained: Dictionary = {}
    var foliage := _load_native_foliage_texture()
    for record in objects:
        if int(record.primitive) != 1:
            continue
        var is_crown := int(record.part) == 3
        var part_count := 3 if is_crown else 1
        for plane in range(part_count):
            var stable_id := str(record.id)
            if is_crown:
                stable_id += "/foliage/" + str(plane)
            retained[stable_id] = true
            var instance: MeshInstance3D = _tree_nodes.get(stable_id)
            if instance == null:
                instance = MeshInstance3D.new()
                instance.name = stable_id.replace("/", "_")
                instance.set_meta("native_id", stable_id)
                instance.set_meta("native_primitive", 1)
                instance.set_meta("native_part", int(record.part))
                instance.set_meta("native_physical", false)
                var material := StandardMaterial3D.new()
                material.roughness = 0.95
                material.cull_mode = BaseMaterial3D.CULL_DISABLED
                if is_crown:
                    instance.mesh = QuadMesh.new()
                    material.albedo_color = Color(0.70, 0.83, 0.68)
                    if foliage != null:
                        material.albedo_texture = foliage
                        material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR
                        material.alpha_scissor_threshold = 0.30
                else:
                    instance.mesh = BoxMesh.new()
                    material.albedo_color = Color(0.34, 0.27, 0.19)
                instance.material_override = material
                add_child(instance)
                _tree_nodes[stable_id] = instance
            instance.position = record.center
            if is_crown:
                var size: Vector3 = record.size
                (instance.mesh as QuadMesh).size = Vector2(
                    sqrt(size.x * size.x + size.z * size.z) * 0.70710678,
                    size.y)
                instance.rotation.y = float(record.yaw) + float(plane) * PI / 3.0
            else:
                (instance.mesh as BoxMesh).size = record.size
                instance.rotation.y = float(record.yaw)
    for stable_id in _tree_nodes.keys():
        if not retained.has(stable_id):
            (_tree_nodes[stable_id] as Node).queue_free()
            _tree_nodes.erase(stable_id)
func tree_object_count() -> int:
    return _tree_nodes.size()
