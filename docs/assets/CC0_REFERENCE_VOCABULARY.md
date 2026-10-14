# CC0 reference vocabulary

This pass uses public assets as measurable construction references rather than
dropping unrelated marketplace scenes into generated rooms. Runtime output is
assembled by Digital Breakdown's deterministic room generator and retains its
existing gameplay collision and traversal contracts.

## Modular urban apartments facade

- Source: https://polyhaven.com/a/modular_urban_apartments_facade
- Author: James Ray Cock
- License: CC0
- Studied source: glTF 1K package, acquired 2026-10-14 through Poly Haven's API
- Local study copy: `artifacts/reference-assets/modular_urban_apartments_facade`

The source contains 147 separately named modules. Its useful grammar is more
important than any one mesh:

- wall courses are 3 m high;
- the standard facade bay is 3 m wide;
- corner and angled modules preserve the same course height;
- base, dado, wall, cornice, and crown are independent vertical layers;
- doors and windows are selected per bay instead of stretched with the wall;
- terminal piers and crowns stop repetition at a building edge.

`game/CityFacadeGrammar.hpp` translates those relationships into the game's
smaller traversal scale. Story count, bay count, entrance position, opening
proportions, recess depth, trim family, and roof termination vary independently.
The renderer places the authored face toward the center route, which fixes the
previous invisible-facade error.

## Dead tree trunk 02

- Source: https://polyhaven.com/a/dead_tree_trunk_02
- Authors: Jenelle van Heerden and Rico Cilliers
- License: CC0
- Studied source: glTF 1K package, acquired 2026-10-14 through Poly Haven's API
- Local study copy: `artifacts/reference-assets/dead_tree_trunk_02`

The scan is roughly 4.05 m long and 1.06 m across at its widest section. It is
used as proportion evidence for non-uniform trunk taper, exposed roots, scars,
and grounded deadwood rather than as a uniformly scaled cylinder.

## Fir tree 01 foliage

- Source: https://polyhaven.com/a/fir_tree_01
- Author information and canonical metadata are supplied by Poly Haven
- License: CC0
- Studied source: 1K diffuse and alpha foliage maps, acquired 2026-10-14
- Local study copy: `artifacts/reference-assets/fir_tree_01`
- Runtime derivative: `models/materials/cc0-fir-foliage.rgba`

One branch island is cropped from the atlas and stored as a 512 x 512 RGBA card.
The renderer arranges crossed cards around computed branch endpoints. Structural
variation remains authored by `tree_geometry::Profile`: sentinel, broad/old,
and wind-shaped trees have different proportions, roots, lean, branch counts,
branch direction, crown density, and foliage color drift. They are not uniform
scale variants.

## Authored tree silhouettes

- Source catalog: https://github.com/Papyszoo/CC0-Public-Domain-Models
- Original creator: Quaternius, Stylized Nature MegaKit
- License: CC0 1.0 Universal
- Studied models: `common_tree_1`, `twisted_tree_1`, `dead_tree_2`, and `pine_2`
- Local source copies: `artifacts/reference-assets/quaternius-source`
- Runtime derivatives: `models/environment/*.dbmesh`

The source GLBs contribute actual branching, taper, crown mass, and species
silhouettes. `tools/convert-glb-to-dbmesh.mjs` traverses the glTF scene,
flattens indexed triangles, grounds and normalizes each mesh, discards embedded
marketplace presentation, and remaps bark and foliage into the game's semantic
palette. The four runtime meshes total less than one megabyte; the approximately
40 MB source packages remain ignored reference material.

Field generation uses these as a vocabulary rather than random scatter. One
gameplay tree remains tied to the existing collider and climb height, while an
asymmetric visual-only edge group frames the clearing with common, twisted,
dead, and pine forms. The center route remains unobstructed. Muted contact
shadows, shorter grass blades, and family-specific greens keep the authored
forms attached to the same ground and lighting world.

## Evidence and present limitation

Matched native captures live under `artifacts/asset-grammar-evidence`. City now
shows readable bays, stories, entrances, recesses, cornices, and distinct height
families. Field now has a near/mid/far treeline, four recognizable botanical
families, grounded deadwood, and a continuous photographic ground material.
The center encounter still needs stronger value grouping and the distant terrain
is still too geometrically simple. Public assets raise the vocabulary floor;
they do not replace composition, lighting, camera, or encounter staging.
