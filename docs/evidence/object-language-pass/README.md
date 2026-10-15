# Object language pass

Native Release captures from revision `6b8ae98` with the current dirty visual pass.

This pass addresses the renderer's repeated-box vocabulary without changing gameplay collision:

- organic objects use tapered, branching segments and irregular faceted volumes;
- grass blades terminate in points, vary subtly, and use scene lighting;
- manufactured architecture exposes foundations, roof edges, recesses, mullions, frames, supports, and rooftop service shapes;
- Field and Coastal extend beyond their gameplay bounds into low faceted terrain rather than visible room walls;
- exact cubes remain available for the deliberately synthetic soul substrate.

Evidence:

- `field-lineup.png`: six deterministic Field seeds after the initial organic-shape conversion.
- `field-open-final.png`: the final open Field horizon and freestanding threshold treatment.
- `settings-final.png`: matched City, Sterile, and Coastal native captures after the construction pass.

The visual review also establishes the next constraint: generated obstacle proportions remain too monolithic in City and Sterile. Their new construction cues improve intent, but the next high-leverage pass should replace large obstacle prisms with semantic object assemblies instead of adding more trim.
