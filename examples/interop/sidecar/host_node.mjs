// host_node.mjs — Node.js host verifying the Lume proof certificate and calling the WASM sidecar (docs/22 G2, #196).
import fs from "node:fs";

const certPath = process.argv[2];
const cexportPath = process.argv[3];
const cert = JSON.parse(fs.readFileSync(certPath, "utf8"));
const cexp = JSON.parse(fs.readFileSync(cexportPath, "utf8"));

if (!cert.ok || !cert.kernel_verified || cert.badge !== "Lume Verified ✓") {
    console.error("sidecar certificate verification failed");
    process.exit(1);
}
const wasmBytes = Buffer.from(cexp.exports[0].wasm_hex, "hex");
const mod = new WebAssembly.Module(wasmBytes);
const inst = new WebAssembly.Instance(mod, {});
const sum = inst.exports.add(735n, 265n);
console.log(`node_sidecar_ok badge="${cert.badge}" obligations=${cert.proven_count}/${cert.total_obligations} wasm_sum=${sum}`);
