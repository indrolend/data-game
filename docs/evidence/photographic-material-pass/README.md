# Photographic material pass — native evidence

- Source revision: `1b3d4ad14a1047edb8b8cbae145d8fbd0f09c0bd` plus the dirty visual-humor/material working tree.
- Configuration: Visual Studio 2026, Release, Windows x64.
- Executable: `artifacts/build/1b3d4ad14a10-Release/bin/Release/DigitalBreakdown.exe`.
- Runtime asset report: `photos=grass/bark/stone/metal`.
- `city.png`: deterministic room inspector, City Corridor, seed 26, room 12.
- `coastal.png`: deterministic room inspector, Coastal Shore, seed 11, room 12.
- `frame-start.png`, `frame-mid.png`, `frame-end.png`: selected frames from the real cinematic lunge/capture fixture.
- `cinematic-gameplay.mp4`: continuous 90-frame, 30 fps native lunge/capture sequence; one soul stored.

Observed result: sampled surfaces are legible under the active lighting. City mass reads as rusty perforated metal, coastal obstacles as mossy stone, and the field as dense photographed grass. Synthetic gameplay surfaces, phone emission, portals, souls, water, and navigation accents remain untextured and palette-controlled, preserving the physical-versus-digital split.

Validation: the native executable built successfully and produced every capture above. Direct CTest execution passed 51/51 tests. The repository wrapper's all-target build encountered an unrelated MSBuild path-creation failure in the optional miniaudio channel-combiner/separator targets after the game and test targets had already built; direct CTest was therefore used to execute the completed suite.
