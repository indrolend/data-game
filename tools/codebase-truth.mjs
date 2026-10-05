#!/usr/bin/env node

import { createHash } from "node:crypto";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const args = new Set(process.argv.slice(2));
const option = (name, fallback = null) => {
  const index = process.argv.indexOf(name);
  return index >= 0 && process.argv[index + 1] ? process.argv[index + 1] : fallback;
};
const scriptRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const root = path.resolve(option("--root", process.cwd() || scriptRoot));
const baseRef = option("--base", "origin/main");

function git(argv, { optional = false } = {}) {
  try {
    return execFileSync("git", ["-C", root, ...argv], { encoding: "utf8", stdio: ["ignore", "pipe", "pipe"] }).trim();
  } catch (error) {
    if (optional) return null;
    throw error;
  }
}

function lines(value) {
  return value ? value.split(/\r?\n/).filter(Boolean) : [];
}

function sha256(buffer) {
  return createHash("sha256").update(buffer).digest("hex");
}

function classify(file) {
  if (file.startsWith("tests/")) return "test";
  if (file.startsWith("desktop/")) return "desktop";
  if (file.startsWith("game/")) return "game";
  if (file.startsWith("network/")) return "network";
  if (file.startsWith("multiplayer-server/")) return "server";
  if (file.startsWith("tools/")) return "tooling";
  if (/^(models|tv)\//.test(file) || /\.(mp3|wav|ttf|dbgif|obj|mtl)$/i.test(file)) return "asset";
  return "repository";
}

function parseCmake(text) {
  const targets = [];
  const tests = [];
  for (const match of text.matchAll(/add_executable\s*\(([\s\S]*?)\)/gi)) {
    const tokens = match[1].match(/"[^"]+"|[^\s]+/g)?.map((token) => token.replace(/^"|"$/g, "")) || [];
    if (tokens.length) targets.push({ name: tokens[0], sources: tokens.slice(1) });
  }
  for (const match of text.matchAll(/add_test\s*\(\s*NAME\s+([^\s\)]+)\s+COMMAND\s+([^\s\)]+)/gi)) {
    tests.push({ name: match[1], command: match[2] });
  }
  return { targets, tests };
}

function resolveInclude(source, request, knownFiles) {
  const candidates = [path.posix.join(path.posix.dirname(source), request), request, `game/${request}`, `desktop/${request}`, `network/${request}`]
    .map((candidate) => path.posix.normalize(candidate));
  const direct = candidates.find((candidate) => knownFiles.has(candidate));
  if (direct) return direct;
  const suffix = `/${request}`;
  const matches = [...knownFiles].filter((candidate) => candidate.endsWith(suffix));
  return matches.length === 1 ? matches[0] : null;
}

const head = git(["rev-parse", "HEAD"]);
const branch = git(["branch", "--show-current"], { optional: true }) || null;
const upstream = git(["rev-parse", "--abbrev-ref", "@{upstream}"], { optional: true });
const statusLines = lines(git(["status", "--short"]));
const paths = lines(git(["ls-files", "--cached", "--others", "--exclude-standard"])).sort();
const knownFiles = new Set(paths);
const files = paths.map((relativePath) => {
  const absolute = path.join(root, relativePath);
  const data = fs.readFileSync(absolute);
  return {
    path: relativePath,
    kind: classify(relativePath),
    bytes: data.length,
    lines: /\.(c|cc|cpp|cxx|h|hpp|m|mm|js|mjs|ts|json|md|txt|cmake)$/i.test(relativePath)
      ? data.toString("utf8").split(/\r?\n/).length
      : null,
    sha256: sha256(data),
  };
});

const cmakeText = fs.readFileSync(path.join(root, "CMakeLists.txt"), "utf8");
const cmake = parseCmake(cmakeText);
const cppTests = paths.filter((file) => /^tests\/.*\.(c|cc|cpp|cxx|m|mm)$/i.test(file));
const registeredTestSources = new Set(cmake.targets.flatMap((target) => target.sources)
  .map((source) => source.replace(/^\$\{CMAKE_CURRENT_SOURCE_DIR\}\//, "").replace(/^\$\{DB_GAME_ROOT\}\//, "game/"))
  .filter((source) => source.startsWith("tests/")));
const unregisteredTests = cppTests.filter((file) => !registeredTestSources.has(file));

const includeEdges = [];
const externalQuotedIncludes = [];
for (const file of paths.filter((value) => /\.(c|cc|cpp|cxx|h|hpp|m|mm)$/i.test(value))) {
  const text = fs.readFileSync(path.join(root, file), "utf8");
  for (const match of text.matchAll(/^\s*#\s*include\s*"([^"]+)"/gm)) {
    const resolved = resolveInclude(file, match[1].replaceAll("\\", "/"), knownFiles);
    if (resolved) includeEdges.push({ from: file, to: resolved });
    else externalQuotedIncludes.push({ from: file, include: match[1] });
  }
}

const baseHead = git(["rev-parse", "--verify", baseRef], { optional: true });
let comparison = null;
if (baseHead) {
  const counts = git(["rev-list", "--left-right", "--count", `${head}...${baseHead}`]).split(/\s+/).map(Number);
  const baseCmake = git(["show", `${baseRef}:CMakeLists.txt`], { optional: true });
  const baseTests = baseCmake ? parseCmake(baseCmake).tests.map((test) => test.name) : [];
  const currentTests = new Set(cmake.tests.map((test) => test.name));
  comparison = {
    ref: baseRef,
    sha: baseHead,
    ahead: counts[0],
    behind: counts[1],
    droppedTests: baseTests.filter((test) => !currentTests.has(test)).sort(),
  };
}

const legacyRoots = paths.filter((file) => /^(native|native-desktop)\//.test(file));
const issues = [
  ...unregisteredTests.map((file) => ({ severity: "error", code: "unregistered-test-source", detail: file })),
  ...legacyRoots.map((file) => ({ severity: "error", code: "duplicate-authority-root", detail: file })),
  ...(comparison?.droppedTests || []).map((test) => ({ severity: "error", code: "dropped-base-test", detail: test })),
];
if (comparison?.behind) issues.push({ severity: "warning", code: "behind-base", detail: `${comparison.behind} commit(s) behind ${baseRef}` });

const byKind = Object.fromEntries([...new Set(files.map((file) => file.kind))].sort().map((kind) => [kind, files.filter((file) => file.kind === kind).length]));
const report = {
  schemaVersion: 1,
  generatedAt: new Date().toISOString(),
  authority: { root, head, branch, upstream, dirty: statusLines.length > 0, status: statusLines },
  comparison,
  inventory: { fileCount: files.length, bytes: files.reduce((sum, file) => sum + file.bytes, 0), byKind, files },
  build: { executableTargets: cmake.targets, registeredTests: cmake.tests, cppTestSources: cppTests },
  dependencies: { edgeCount: includeEdges.length, edges: includeEdges, externalQuoted: externalQuotedIncludes },
  issues,
};

if (args.has("--json")) {
  process.stdout.write(`${JSON.stringify(report, null, 2)}\n`);
} else {
  console.log("DATA CODEBASE TRUTH");
  console.log(`Authority  ${branch || "detached"} @ ${head.slice(0, 12)}${statusLines.length ? " · DIRTY" : " · clean"}`);
  if (comparison) console.log(`Base       ${baseRef} @ ${baseHead.slice(0, 12)} · ahead ${comparison.ahead} · behind ${comparison.behind}`);
  console.log(`Inventory  ${files.length} files · ${report.inventory.bytes} bytes · ${Object.entries(byKind).map(([kind, count]) => `${kind}:${count}`).join(" ")}`);
  console.log(`Build      ${cmake.targets.length} executables · ${cmake.tests.length} registered tests · ${cppTests.length} C/C++ test sources`);
  console.log(`Includes   ${includeEdges.length} resolved project edges · ${externalQuotedIncludes.length} external/generated quoted includes`);
  if (comparison?.droppedTests.length) console.log(`REGRESSION  dropped base tests: ${comparison.droppedTests.join(", ")}`);
  for (const issue of issues.filter((value) => value.code !== "dropped-base-test")) console.log(`${issue.severity.toUpperCase()} ${issue.code}: ${issue.detail}`);
  console.log("Detail     rerun with --json for the complete file/hash/build/include projection");
}

if (args.has("--check") && issues.some((issue) => issue.severity === "error")) process.exitCode = 1;

export { parseCmake, resolveInclude };
