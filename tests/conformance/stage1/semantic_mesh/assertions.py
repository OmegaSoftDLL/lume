#!/usr/bin/env python3
import hashlib
import json
import sys


def load(path):
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def field(value):
    raw = value.encode("ascii")
    return str(len(raw)) + ":" + value


def state_hash(state):
    canonical = "".join(field(value) for value in [
        "lume-semantic-intent-state/0.1",
        state["goal"],
        state["target"],
        ",".join(state["capabilities"]) if state["capabilities"] else "pure",
        "max_source_bytes={0},max_transactions={1}".format(
            state["resources"]["max_source_bytes"],
            state["resources"]["max_transactions"],
        ),
        state["acceptance"],
        state["ast"],
        state["source"],
    ])
    return "sha256:" + hashlib.sha256(canonical.encode("ascii")).hexdigest()


def check_lineage(result):
    nodes = result["lineage"]
    for node in nodes:
        canonical = "".join(field(value) for value in [
            "lume-semantic-intent-node/0.1",
            node["parent_id"] or "",
            node["op"],
            node["state_sha256"],
        ])
        expected = "sha256:" + hashlib.sha256(canonical.encode("ascii")).hexdigest()
        assert node["id"] == expected, (node["index"], node["id"], expected)
        if node["parent_index"] < 0:
            assert node["parent_id"] is None
        else:
            assert node["parent_id"] == nodes[node["parent_index"]]["id"]


def require_error(result, code):
    assert result["ok"] is False
    assert result["protocol"] == "lume-semantic-intent/0.1"
    assert result["diagnostics"], result
    assert result["diagnostics"][0]["code"] == code, result
    assert "state" not in result and "current" not in result


def main():
    mode = sys.argv[1]
    if mode == "error":
        require_error(load(sys.argv[2]), sys.argv[3])
        return
    if mode == "canonical":
        a = load(sys.argv[2])
        b = load(sys.argv[3])
        for result in (a, b):
            assert result["ok"] is True
            assert result["state"]["state_sha256"] == state_hash(result["state"])
            check_lineage(result)
        assert a["state"]["goal"] != b["state"]["goal"]
        assert a["state"]["state_sha256"] != b["state"]["state_sha256"]
        assert a["current"] != b["current"]
        return
    result = load(sys.argv[2])
    assert result["ok"] is True, result
    assert result["protocol"] == "lume-semantic-intent/0.1"
    check_lineage(result)
    if mode == "negotiate":
        assert result["mode"] == "negotiate"
        assert result["state"]["target"] == "x86_64-linux-elf"
        assert result["validation"]["compiler_ok"] is True
        assert result["validation"]["target_supported"] is True
        assert result["validation"]["proof_status"] == "proven"
        assert result["validation"]["result"] == "42"
        assert result["validation"]["proofs"]
        assert all(p["status"] == "proven" for p in result["validation"]["proofs"])
        assert len(result["lineage"]) == 1
        assert result["active_branch"] == [result["current"]]
    elif mode == "transact":
        nodes = result["lineage"]
        assert result["mode"] == "transact"
        assert len(nodes) == 7, nodes
        assert result["state"]["target"] == "lumb"
        assert result["validation"]["compiler_ok"] is True
        assert result["validation"]["target_supported"] is True
        assert result["validation"]["proof_status"] == "proven"
        assert len({nodes[i]["state_sha256"] for i in [0, 1, 2, 3]}) == 4
        assert nodes[4]["state_sha256"] == nodes[1]["state_sha256"]
        assert nodes[5]["state_sha256"] == nodes[2]["state_sha256"]
        assert nodes[3]["state_sha256"] == nodes[6]["state_sha256"]
        assert nodes[3]["id"] != nodes[6]["id"]
        assert nodes[4]["op"].startswith("rollback:")
        assert nodes[4]["parent_index"] == 3
        assert nodes[5]["parent_index"] == 4
        assert nodes[6]["parent_index"] == 5
        assert nodes[5]["op"].startswith("replay:" + nodes[2]["id"] + ":")
        assert nodes[6]["op"].startswith("replay:" + nodes[3]["id"] + ":")
        assert result["active_branch"] == [nodes[i]["id"] for i in [0, 1, 4, 5, 6]]
        for index in [1, 2, 5]:
            assert nodes[index]["affected_symbols"] == ["inc"], nodes[index]
            assert set(nodes[index]["invalidated_symbols"]) == {"inc", "caller"}, nodes[index]
        assert set(nodes[4]["affected_symbols"]) == {"inc", "caller"}
        assert set(nodes[4]["invalidated_symbols"]) == {"inc", "caller"}
        assert set(nodes[3]["invalidated_symbols"]) == {"inc", "caller"}
        assert set(nodes[6]["invalidated_symbols"]) == {"inc", "caller"}
        assert "ensures ((result - x) == 1)" in result["state"]["source"]
    elif mode == "out_of_fragment":
        assert result["validation"]["compiler_ok"] is True
        assert result["validation"]["target_supported"] is True
        assert result["validation"]["proof_status"] == "runtime_checked"
        cube = [p for p in result["validation"]["proofs"] if p["name"] == "cube_abs"]
        assert len(cube) == 1
        assert cube[0]["status"] == "runtime_checked"
        assert cube[0]["status"] != "proven"
    else:
        raise AssertionError("unknown assertion mode: " + mode)


if __name__ == "__main__":
    main()
