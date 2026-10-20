# Lunar surface texture

`moon_lroc_color_2k.jpg` is the 2048 × 1024 color map and
`moon_lola_elevation_1k.jpg` is the 1024 × 512 LOLA displacement map from NASA
Goddard Scientific Visualization Studio's
[CGI Moon Kit](https://svs.gsfc.nasa.gov/4720/). The color map is based on the
Lunar Reconnaissance Orbiter Camera (LROC) WAC Hapke-normalized mosaic. NASA SVS
credits Ernie Wright (USRA) as visualizer and Noah Petro (NASA/GSFC) as
scientist. The asset page describes SVS content as public domain unless an
item says otherwise.

The maps are used by a render-only sphere shader for surface color and
low-cost relief shading. Godot does not simulate lunar orbit, gravity, or an
additional gameplay clock; its direction and lighting are derived from the
existing native sun/day state with an explicitly accelerated 29.53059-native-
day phase approximation and a stylized 5.2-degree inclination with a 173.31-
native-day eclipse-season rhythm. Because the game has no Earth location or
civil calendar, this is a recognizable phase/orbit approximation, not an
ephemeris or eclipse prediction for a real terrestrial observer.
