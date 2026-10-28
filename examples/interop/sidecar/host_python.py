# host_python.py — Python host verifying the Lume proof certificate and invoking the proven sidecar (docs/22 G2, #196).
import json
import subprocess
import sys

cert_path = sys.argv[1]
bin_path = sys.argv[2]

with open(cert_path, "r", encoding="utf-8") as f:
    cert = json.load(f)

if not cert.get("ok") or not cert.get("kernel_verified") or cert.get("badge") != "Lume Verified ✓":
    sys.stderr.write("sidecar certificate verification failed\n")
    sys.exit(1)

out = subprocess.check_output([bin_path], text=True).strip().splitlines()
print(f"python_sidecar_ok badge=\"{cert['badge']}\" settled={out[0]} total={out[1]}")
