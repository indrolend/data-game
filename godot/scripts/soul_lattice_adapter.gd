extends Node3D

# Read-only visualization of the parent C++ 3x3x3 soul lattice.
# All lattice deformation and ingestion remain native-authoritative.

var mesh_instance: MeshInstance3D
var material: StandardMaterial3D

func _ready() -> void:
    mesh_instance = MeshInstance3D.new()
    mesh_instance.name = "NativeSoulLattice"
    add_child(mesh_instance)

    material = StandardMaterial3D.new()
    material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    material.cull_mode = BaseMaterial3D.CULL_DISABLED
    material.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    material.emission_enabled = true
    mesh_instance.material_override = material
    visible = false

func _index(x: int, y: int, z: int) -> int:
    return x + 3 * y + 9 * z

func _quad(vertices: PackedVector3Array, a: int, b: int, c: int, d: int) -> void:
    vertices.append_array(PackedVector3Array([
        _point(a), _point(b), _point(c),
        _point(a), _point(c), _point(d)
    ]))

var points: PackedVector3Array

func _point(index: int) -> Vector3:
    return points[index]

func sync_from_native(snapshot: Dictionary, lattice: PackedVector3Array) -> void:
    visible = bool(snapshot.get("visible", false)) and float(snapshot.get("morph_scale", 0.0)) > 0.001
    if not visible or lattice.size() != 27:
        mesh_instance.mesh = null
        return

    points = lattice

    position = snapshot.position + Vector3.UP * (0.57 + float(snapshot.vertical_offset))
    rotation = Vector3.ZERO
    # The native lattice exists throughout the morph, but its presentation
    # grows from the collapsing shell rather than popping in at full size.
    var morph_scale := clampf(float(snapshot.get("morph_scale", 0.0)), 0.0, 1.0)
    scale = snapshot.get("scale", Vector3.ONE) * morph_scale

    var triangles := PackedVector3Array()

    for axis in range(3):
        for side in [0, 2]:
            for u in range(2):
                for v in range(2):
                    var coords := []
                    for offset in [Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(0, 1)]:
                        var p := [0, 0, 0]
                        p[axis] = side
                        p[(axis + 1) % 3] = u + offset.x
                        p[(axis + 2) % 3] = v + offset.y
                        coords.append(_index(p[0], p[1], p[2]))
                    _quad(triangles, coords[0], coords[1], coords[2], coords[3])

    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = triangles

    var surface := ArrayMesh.new()
    surface.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    mesh_instance.mesh = surface

    var color: Color = snapshot.color
    color.a = clampf(float(snapshot.shell_opacity), 0.0, 1.0)
    material.albedo_color = color
    material.emission = Color(color.r, color.g, color.b) * float(snapshot.emission)
    material.emission_energy_multiplier = 1.0
