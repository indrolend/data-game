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
var _city_facade_nodes: Dictionary = {}
var _city_facade_meshes: Dictionary = {}
var _grass_node: Node3D
var _grass_snapshot_key := ""

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
    _sync_grass(snapshot)
    _sync_rock_objects(snapshot.objects as Array)
    _sync_house_objects(snapshot.objects as Array)
    _sync_marker_objects(snapshot.objects as Array)
    _sync_ruin_objects(snapshot.objects as Array)
    _sync_city_facades(snapshot.get("city_facade_modules", []))

func structure_count() -> int:
    return _nodes.size()

func structure(stable_id: String) -> MeshInstance3D:
    return _nodes.get(stable_id)

func lawn_object_count() -> int:
    return _lawn_nodes.size()

func lawn_object(stable_id: String) -> MeshInstance3D:
    return _lawn_nodes.get(stable_id)

func grass_blade_count() -> int:
    return _grass_node.get_child_count() if _grass_node != null else 0

func grass_blade(index: int) -> MeshInstance3D:
    return _grass_node.get_child(index) as MeshInstance3D if _grass_node != null else null

func _sync_grass(snapshot: Dictionary) -> void:
    var key := "%d:%d:%d:%d" % [int(snapshot.room_seed), int(snapshot.room_index),
        int(snapshot.tile_index), int(snapshot.grass)]
    if key == _grass_snapshot_key:
        return
    _grass_snapshot_key = key
    if _grass_node == null:
        _grass_node = Node3D.new()
        _grass_node.name = "NativeFieldGrass"
        _grass_node.set_meta("presentation_only", true)
        add_child(_grass_node)
    for old_blade in _grass_node.get_children():
        _grass_node.remove_child(old_blade)
        old_blade.free()
    var blades: Array = snapshot.get("grass_blades", [])
    if not bool(snapshot.grass) or blades.is_empty():
        _grass_node.visible = false
        return
    var blade_mesh := _grass_blade_mesh()
    var blade_material := StandardMaterial3D.new()
    blade_material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    blade_material.cull_mode = BaseMaterial3D.CULL_DISABLED
    var grass_image := _field_grass_image
    if grass_image != null:
        var average := Color(0, 0, 0, 1)
        for y in range(grass_image.get_height()):
            for x in range(grass_image.get_width()):
                average += grass_image.get_pixel(x, y)
        var pixel_count := grass_image.get_width() * grass_image.get_height()
        blade_material.albedo_color = Color(average.r / pixel_count,
            average.g / pixel_count, average.b / pixel_count, 1.0)
    blade_mesh.surface_set_material(0, blade_material)
    for index in range(blades.size()):
        var blade: Dictionary = blades[index]
        var root: Vector3 = blade.root
        var height := float(blade.height)
        var width := float(blade.width)
        var phase := float(blade.phase)
        var blade_node := MeshInstance3D.new()
        blade_node.name = "NativeBlade_%03d" % index
        blade_node.mesh = blade_mesh
        blade_node.position = root
        blade_node.rotation.y = phase
        blade_node.scale = Vector3(width, height, width)
        blade_node.set_meta("native_height", height)
        blade_node.set_meta("native_width", width)
        _grass_node.add_child(blade_node)
    _grass_node.visible = true

func _grass_blade_mesh() -> ArrayMesh:
    var vertices := PackedVector3Array([
        Vector3(-0.5, 0.0, 0.0), Vector3(0.5, 0.0, 0.0), Vector3(0.0, 1.0, 0.0),
        Vector3(0.0, 0.0, -0.5), Vector3(0.0, 0.0, 0.5), Vector3(0.0, 1.0, 0.0)])
    var arrays: Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = vertices
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

func _sync_lawn_objects(objects: Array, tile_index: int) -> void:
    var retained: Dictionary = {}
    var floor_node := structure("room/%d/structure/floor" % tile_index)
    var native_floor_material: Material = floor_node.material_override if floor_node != null else null
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
            add_child(mesh_instance)
            _lawn_nodes[stable_id] = mesh_instance
        mesh_instance.position = record.center
        mesh_instance.rotation.y = float(record.yaw)
        (mesh_instance.mesh as BoxMesh).size = record.size
        mesh_instance.material_override = native_floor_material
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
        material.set_meta("native_texture_world_scale", float(record.visual_texture_world_scale))
        material.set_meta("native_texture", "stone; pixels not yet ported")
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
        house_material.set_meta("native_texture", str(record.visual_texture))
        house_material.set_meta("native_texture_world_scale", float(record.visual_texture_world_scale))
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
        marker_material.set_meta("native_texture", str(record.visual_texture))
        marker_material.set_meta("native_texture_world_scale", float(record.visual_texture_world_scale))
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
        ruin_material.set_meta("native_texture", str(record.visual_texture))
        ruin_material.set_meta("native_texture_world_scale", float(record.visual_texture_world_scale))
        ruin_material.set_meta("native_texture_phase", float(record.visual_texture_phase))
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
    material.set_meta("native_shininess", float(definition.shininess))
    material.set_meta("native_fog", bool(definition.fog))
    return material
