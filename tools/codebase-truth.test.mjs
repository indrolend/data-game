import assert from "node:assert/strict";
import { execFileSync, spawnSync } from "node:child_process";
import fs, { mkdtempSync, rmSync } from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const tool = path.join(root, "tools", "codebase-truth.mjs");

test("codebase truth is complete, internally consistent, and current", () => {
  const output = execFileSync(process.execPath, [tool, "--json", "--check", "--base", "origin/main"], {
    cwd: root,
    encoding: "utf8",
  });
  const report = JSON.parse(output);
  const expectedPaths = execFileSync("git", ["ls-files", "--cached", "--others", "--exclude-standard"], { cwd: root, encoding: "utf8" })
    .trim().split(/\r?\n/).filter(Boolean).sort();
  assert.deepEqual(report.inventory.files.map((file) => file.path), expectedPaths);
  assert.equal(report.inventory.files.every((file) => /^[a-f0-9]{64}$/.test(file.sha256)), true);
  assert.deepEqual(report.comparison.droppedTests, []);
  assert.deepEqual(report.issues.filter((issue) => issue.severity === "error"), []);
  assert.equal(report.authority.head, execFileSync("git", ["rev-parse", "HEAD"], { cwd: root, encoding: "utf8" }).trim());
  assert.equal(fs.existsSync(path.join(root, report.inventory.files[0].path)), true);
});

test("codebase truth rejects dropped tests and duplicate authority roots", () => {
  const fixture = mkdtempSync(path.join(os.tmpdir(), "data-codebase-truth-"));
  try {
    execFileSync("git", ["init", "-b", "main"], { cwd: fixture, stdio: "pipe" });
    execFileSync("git", ["config", "user.email", "truth@example.invalid"], { cwd: fixture });
    execFileSync("git", ["config", "user.name", "Truth Test"], { cwd: fixture });
    fs.mkdirSync(path.join(fixture, "tests"));
    fs.writeFileSync(path.join(fixture, "tests", "contract_test.cpp"), "int main() { return 0; }\n");
    fs.writeFileSync(path.join(fixture, "CMakeLists.txt"), [
      "add_executable(ContractTest tests/contract_test.cpp)",
      "add_test(NAME ContractTest COMMAND ContractTest)",
      "",
    ].join("\n"));
    execFileSync("git", ["add", "."], { cwd: fixture });
    execFileSync("git", ["commit", "-m", "baseline"], { cwd: fixture, stdio: "pipe" });

    fs.mkdirSync(path.join(fixture, "native"));
    fs.writeFileSync(path.join(fixture, "native", "duplicate.cpp"), "int duplicate() { return 1; }\n");
    fs.writeFileSync(path.join(fixture, "CMakeLists.txt"), "add_executable(App native/duplicate.cpp)\n");
    const result = spawnSync(process.execPath, [tool, "--root", fixture, "--base", "main", "--json", "--check"], {
      encoding: "utf8",
    });
    assert.equal(result.status, 1, result.stderr);
    const report = JSON.parse(result.stdout);
    assert.deepEqual(report.comparison.droppedTests, ["ContractTest"]);
    assert.equal(report.issues.some((issue) => issue.code === "unregistered-test-source"), true);
    assert.equal(report.issues.some((issue) => issue.code === "duplicate-authority-root"), true);
  } finally {
    rmSync(fixture, { recursive: true, force: true });
  }
});
