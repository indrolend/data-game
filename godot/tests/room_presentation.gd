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
    var snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(snapshot)
    var lawn_records: Array = []
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
            check(lawn_node.material_override == adapter.structure("room/0/structure/floor").material_override,
                "lawn fragment reuses the native room floor material")
            check(lawn_node.get_class() == "MeshInstance3D",
                "render-only lawn node introduces no Godot collision")
    var floor: MeshInstance3D = adapter.structure("room/0/structure/floor")
    check(floor != null, "adapter preserves native stable floor ID")
    if floor != null:
        check((floor.mesh as BoxMesh).size == Vector3(84.0, 0.08, 42.0),
            "adapter uses native floor dimensions")
        var material := floor.material_override as StandardMaterial3D
        check(absf(material.albedo_color.r - 63.0 / 255.0) <= 0.000001,
            "adapter uses native field-ground color")
        check(int(material.get_meta("native_texture")) == 1,
            "adapter retains native generated-texture identity")
    runtime.start_city_room_fixture()
    var city_snapshot: Dictionary = runtime.room_presentation_snapshot(-1)
    adapter.sync_snapshot(city_snapshot)
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
            if bool(ruin_record.visual_has_scar):
                var scar := ruin_node.get_node_or_null("NativeScar") as MeshInstance3D
                check(scar != null and (scar.mesh as BoxMesh).size == ruin_record.visual_scar_size and
                    scar.position == ruin_record.visual_scar_center - ruin_record.center,
                    "render-only ruin scar geometry matches native draw record")
            check(ruin_node.get_class() == "MeshInstance3D",
                "native ruin prop remains render-only in Godot")
    adapter.free()
    runtime.free()
    if not failures.is_empty():
        print("RESULT: FAIL count=", failures.size())
        quit(2)
        return
    print("RESULT: PASS native room structures, lawn, rocks, HouseGeometry, MarkerPillarGeometry, and RuinGeometry drive render-only Godot nodes")
    quit(0)
