#!/usr/bin/env node

// Collapse an authored glTF binary into Digital Breakdown's intentionally
// tiny static-mesh format. The runtime only needs triangles and semantic
// material colors; embedded source textures stay in the reference package.

import fs from "node:fs";
import path from "node:path";

const arguments_ = process.argv.slice(2);
const inputPath = arguments_[0];
const outputPath = arguments_[1];
const metadataFlag = arguments_.indexOf("--metadata");
const metadataPath = metadataFlag >= 0 ? arguments_[metadataFlag + 1] : undefined;
const includeFlag = arguments_.indexOf("--include");
const includePattern = includeFlag >= 0 ? arguments_[includeFlag + 1] : undefined;
if (!inputPath || !outputPath) {
  console.error("usage: node tools/convert-glb-to-dbmesh.mjs input.glb|input.gltf output.dbmesh [--metadata output.structure.json] [--include exact-node-name]");
  process.exit(2);
}
if (metadataFlag >= 0 && !metadataPath) throw new Error("--metadata requires an output path");
if (includeFlag >= 0 && !includePattern) throw new Error("--include requires an exact node name");

const source = fs.readFileSync(inputPath);
let json;
let binary;
if (path.extname(inputPath).toLowerCase() === ".gltf") {
  json = JSON.parse(source.toString("utf8"));
  const uri = json.buffers?.[0]?.uri;
  if (!uri || uri.startsWith("data:")) throw new Error("expected one external binary glTF buffer");
  binary = fs.readFileSync(path.resolve(path.dirname(inputPath), decodeURIComponent(uri)));
} else {
  if (source.readUInt32LE(0) !== 0x46546c67 || source.readUInt32LE(4) !== 2) {
    throw new Error("expected a glTF 2.0 GLB or JSON glTF");
  }
  for (let offset = 12; offset + 8 <= source.length;) {
    const length = source.readUInt32LE(offset);
    const type = source.readUInt32LE(offset + 4);
    const data = source.subarray(offset + 8, offset + 8 + length);
    if (type === 0x4e4f534a) json = JSON.parse(data.toString("utf8").replace(/\0+$/, ""));
    if (type === 0x004e4942) binary = data;
    offset += 8 + length;
  }
}
if (!json || !binary) throw new Error("glTF is missing JSON or binary data");

const identity = () => [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
const multiply = (a, b) => {
  const out = new Array(16).fill(0);
  for (let column = 0; column < 4; ++column)
    for (let row = 0; row < 4; ++row)
      for (let k = 0; k < 4; ++k)
        out[column * 4 + row] += a[k * 4 + row] * b[column * 4 + k];
  return out;
};
const nodeMatrix = (node) => {
  if (node.matrix) return [...node.matrix];
  const [x, y, z, w] = node.rotation ?? [0, 0, 0, 1];
  const [sx, sy, sz] = node.scale ?? [1, 1, 1];
  const [tx, ty, tz] = node.translation ?? [0, 0, 0];
  return [
    (1 - 2 * (y * y + z * z)) * sx, (2 * (x * y + z * w)) * sx, (2 * (x * z - y * w)) * sx, 0,
    (2 * (x * y - z * w)) * sy, (1 - 2 * (x * x + z * z)) * sy, (2 * (y * z + x * w)) * sy, 0,
    (2 * (x * z + y * w)) * sz, (2 * (y * z - x * w)) * sz, (1 - 2 * (x * x + y * y)) * sz, 0,
    tx, ty, tz, 1,
  ];
};
const transform = (m, p) => [
  m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
  m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
  m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14],
];

const componentReaders = {
  5121: { bytes: 1, read: (b, o) => b.readUInt8(o) },
  5123: { bytes: 2, read: (b, o) => b.readUInt16LE(o) },
  5125: { bytes: 4, read: (b, o) => b.readUInt32LE(o) },
  5126: { bytes: 4, read: (b, o) => b.readFloatLE(o) },
};
const widths = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4 };
function accessorValues(index) {
  const accessor = json.accessors[index];
  if (accessor.sparse) throw new Error("sparse accessors are not supported");
  const view = json.bufferViews[accessor.bufferView];
  const reader = componentReaders[accessor.componentType];
  const width = widths[accessor.type];
  if (!reader || !width) throw new Error(`unsupported accessor ${accessor.componentType}/${accessor.type}`);
  const base = (view.byteOffset ?? 0) + (accessor.byteOffset ?? 0);
  const stride = view.byteStride ?? reader.bytes * width;
  return Array.from({ length: accessor.count }, (_, item) =>
    Array.from({ length: width }, (_, component) => reader.read(binary, base + item * stride + component * reader.bytes)));
}

function semanticColor(materialIndex) {
  const material = json.materials?.[materialIndex] ?? {};
  const name = (material.name ?? "").toLowerCase();
  const family = path.basename(inputPath).toLowerCase();
  if (name.includes("leaf") || name.includes("leaves")) {
    if (family.includes("twisted")) return [0.34, 0.315, 0.14, 1];
    if (family.includes("pine")) return [0.115, 0.255, 0.12, 1];
    return [0.19, 0.36, 0.15, 1];
  }
  if (name.includes("bark") || name.includes("wood") || name.includes("trunk")) {
    if (family.includes("dead")) return [0.29, 0.255, 0.205, 1];
    if (family.includes("twisted")) return [0.255, 0.235, 0.205, 1];
    return [0.285, 0.205, 0.125, 1];
  }
  if (name.includes("glass")) return [0.12, 0.24, 0.285, 1];
  if (name.includes("trim_01")) return [0.21, 0.225, 0.225, 1];
  if (name.includes("trim_02")) return [0.42, 0.39, 0.345, 1];
  if (name.includes("plaster")) return [0.49, 0.475, 0.435, 1];
  if (name.includes("objects")) return [0.275, 0.265, 0.245, 1];
  const factor = material.pbrMetallicRoughness?.baseColorFactor;
  return factor ? factor.map(Number) : [0.55, 0.55, 0.52, 1];
}

const emitted = [];
const structure = [];
function emitMesh(meshIndex, world, nodePath) {
  const mesh = json.meshes[meshIndex];
  let primitiveIndex = 0;
  for (const primitive of mesh.primitives) {
    if ((primitive.mode ?? 4) !== 4) continue;
    const positions = accessorValues(primitive.attributes.POSITION);
    const indices = primitive.indices === undefined
      ? positions.map((_, index) => index)
      : accessorValues(primitive.indices).map((value) => value[0]);
    if (indices.length % 3 !== 0) throw new Error("triangle index count is not divisible by three");
    const vertices = indices.map((index) => transform(world, positions[index]));
    emitted.push({ vertices, color: semanticColor(primitive.material) });
    const bounds = { minimum: [Infinity, Infinity, Infinity], maximum: [-Infinity, -Infinity, -Infinity] };
    for (const vertex of vertices) for (let axis = 0; axis < 3; ++axis) {
      bounds.minimum[axis] = Math.min(bounds.minimum[axis], vertex[axis]);
      bounds.maximum[axis] = Math.max(bounds.maximum[axis], vertex[axis]);
    }
    structure.push({
      nodePath,
      mesh: mesh.name ?? `mesh_${meshIndex}`,
      primitive: primitiveIndex++,
      material: json.materials?.[primitive.material]?.name ?? null,
      sourceVertexCount: positions.length,
      triangleCount: indices.length / 3,
      bounds,
    });
  }
}
function visit(nodeIndex, parent, parentPath = "") {
  const node = json.nodes[nodeIndex];
  const world = multiply(parent, nodeMatrix(node));
  const nodePath = `${parentPath}/${node.name ?? `node_${nodeIndex}`}`;
  if (node.mesh !== undefined && (!includePattern || node.name === includePattern)) emitMesh(node.mesh, world, nodePath);
  for (const child of node.children ?? []) visit(child, world, nodePath);
}
const scene = json.scenes[json.scene ?? 0];
for (const node of scene.nodes ?? []) visit(node, identity());
if (!emitted.length) throw new Error(includePattern ? `node '${includePattern}' has no triangle primitives` : "no triangle primitives found");

const all = emitted.flatMap((batch) => batch.vertices);
const minimum = [Infinity, Infinity, Infinity];
const maximum = [-Infinity, -Infinity, -Infinity];
for (const vertex of all) for (let axis = 0; axis < 3; ++axis) {
  minimum[axis] = Math.min(minimum[axis], vertex[axis]);
  maximum[axis] = Math.max(maximum[axis], vertex[axis]);
}
const height = maximum[1] - minimum[1];
if (!(height > 0)) throw new Error("mesh has no height");
const centerX = (minimum[0] + maximum[0]) * 0.5;
const centerZ = (minimum[2] + maximum[2]) * 0.5;
for (const batch of emitted) for (const vertex of batch.vertices) {
  vertex[0] = (vertex[0] - centerX) / height;
  vertex[1] = (vertex[1] - minimum[1]) / height;
  vertex[2] = (vertex[2] - centerZ) / height;
}

const vertexCount = all.length;
const headerBytes = 12;
const vertexBytes = vertexCount * 12;
const batchBytes = emitted.length * 24;
const output = Buffer.alloc(headerBytes + vertexBytes + batchBytes);
output.write("DBM1", 0, "ascii");
output.writeUInt32LE(vertexCount, 4);
output.writeUInt32LE(emitted.length, 8);
let offset = headerBytes;
let start = 0;
for (const batch of emitted) for (const vertex of batch.vertices) for (const component of vertex) {
  output.writeFloatLE(component, offset);
  offset += 4;
}
for (const batch of emitted) {
  output.writeUInt32LE(start, offset);
  output.writeUInt32LE(batch.vertices.length, offset + 4);
  batch.color.forEach((component, index) => output.writeFloatLE(component, offset + 8 + index * 4));
  start += batch.vertices.length;
  offset += 24;
}
fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, output);
if (metadataPath) {
  const metadata = {
    schema: "digital-breakdown.asset-structure.v1",
    source: path.basename(inputPath),
    runtime: path.basename(outputPath),
    sourceGenerator: json.asset?.generator ?? null,
    sourceCopyright: json.asset?.copyright ?? null,
    selectedNode: includePattern ?? null,
    normalizedBy: "height",
    sourceBounds: { minimum, maximum },
    normalizedBounds: {
      minimum: [(minimum[0] - centerX) / height, 0, (minimum[2] - centerZ) / height],
      maximum: [(maximum[0] - centerX) / height, 1, (maximum[2] - centerZ) / height],
    },
    runtimeVertexCount: vertexCount,
    batches: emitted.map((batch, index) => ({ index, vertexCount: batch.vertices.length, color: batch.color })),
    structure,
  };
  fs.mkdirSync(path.dirname(metadataPath), { recursive: true });
  fs.writeFileSync(metadataPath, `${JSON.stringify(metadata, null, 2)}\n`);
}
console.log(`${path.basename(inputPath)} -> ${path.basename(outputPath)}: ${vertexCount} vertices, ${emitted.length} batches, ${output.length} bytes`);
