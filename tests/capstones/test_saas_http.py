#!/usr/bin/env python3
"""Independent HTTP client and on-disk recovery checks for the P0 ledger slice."""

import http.client
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time

HOST = "127.0.0.1"
PORT = 18737


def request(method, path, body=None, tenant=None, token=None):
    headers = {}
    if tenant is not None:
        headers["X-Tenant-ID"] = str(tenant)
    if token is not None:
        headers["Authorization"] = f"Bearer {token}"
    connection = http.client.HTTPConnection(HOST, PORT, timeout=3)
    connection.request(method, path, body=body, headers=headers)
    response = connection.getresponse()
    payload = response.read()
    content_type = response.getheader("Content-Type")
    connection.close()
    assert content_type == "application/json", content_type
    return response.status, json.loads(payload)


def receive_response(client):
    chunks = []
    while True:
        chunk = client.recv(4096)
        if not chunk:
            break
        chunks.append(chunk)
    response = b"".join(chunks)
    assert response.startswith(b"HTTP/1.1 "), response
    status = int(response.split(b" ", 2)[1])
    body = response.split(b"\r\n\r\n", 1)[1]
    return status, json.loads(body)


def raw_request(payload):
    with socket.create_connection((HOST, PORT), timeout=3) as client:
        client.sendall(payload)
        client.shutdown(socket.SHUT_WR)
        return receive_response(client)


def raw_request_parts(headers, *body_parts):
    with socket.create_connection((HOST, PORT), timeout=3) as client:
        client.sendall(headers)
        for part in body_parts:
            time.sleep(0.1)
            client.sendall(part)
        client.shutdown(socket.SHUT_WR)
        return receive_response(client)


class Server:
    def __init__(self, binary, cwd):
        self.binary = binary
        self.cwd = cwd
        self.process = None

    def __enter__(self):
        self.process = subprocess.Popen(
            [self.binary],
            cwd=self.cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                stdout, stderr = self.process.communicate()
                raise AssertionError(f"server exited early: {stdout}\n{stderr}")
            try:
                if request("GET", "/health") == (200, {"ok": True}):
                    return self
            except (OSError, ValueError):
                time.sleep(0.05)
        raise AssertionError("server did not become ready")

    def __exit__(self, exc_type, exc_value, traceback):
        self.process.terminate()
        try:
            stdout, stderr = self.process.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            self.process.kill()
            stdout, stderr = self.process.communicate(timeout=3)
        if exc_type is None and self.process.returncode not in (-15, 0):
            raise AssertionError(f"unexpected server exit {self.process.returncode}: {stdout}\n{stderr}")


def expect(actual, status, **fields):
    actual_status, payload = actual
    assert actual_status == status, (actual_status, payload)
    for key, value in fields.items():
        assert payload.get(key) == value, (key, payload)


def main():
    binary = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="lume-p0-saas-") as temp_dir:
        wal = Path(temp_dir) / "p0_tenant_ledger.wal"
        with Server(binary, temp_dir):
            expect(request("GET", "/v1/balance", tenant=101, token="ledger-101-token"), 200, tenant=101, balance=100, version=1)
            expect(request("GET", "/v1/balance", tenant=202, token="ledger-202-token"), 200, tenant=202, balance=100, version=1)
            expect(request("POST", "/v1/debit", "35", 101, "ledger-101-token"), 200, tenant=101, balance=65, version=2)
            expect(request("POST", "/v1/credit", "25", 202, "ledger-202-token"), 200, tenant=202, balance=125, version=3)
            expect(request("GET", "/v1/balance", tenant=101, token="ledger-101-token"), 200, balance=65)
            expect(request("GET", "/v1/balance", tenant=202, token="ledger-202-token"), 200, balance=125)

            expect(request("GET", "/v1/balance", tenant=202, token="ledger-101-token"), 403, error="cross_tenant_access_denied")
            expect(request("POST", "/v1/debit", "10", 202, "ledger-101-token"), 403, error="cross_tenant_access_denied")
            expect(request("GET", "/v1/balance", tenant=101), 401, error="invalid_credentials")
            expect(request("GET", "/v1/balance", tenant=101, token="wrong-token"), 401, error="invalid_credentials")
            expect(request("POST", "/v1/debit", "999", 101, "ledger-101-token"), 409, error="insufficient_funds")
            expect(request("POST", "/v1/debit", "-5", 101, "ledger-101-token"), 400, error="amount_must_be_a_positive_integer")
            expect(request("POST", "/v1/debit", "abc", 101, "ledger-101-token"), 400, error="amount_must_be_a_positive_integer")
            expect(request("POST", "/v1/debit", "0", 101, "ledger-101-token"), 400, error="amount_out_of_range")
            expect(request("POST", "/v1/debit", "100001", 101, "ledger-101-token"), 400, error="amount_out_of_range")
            expect(request("GET", "/v1/balance", tenant=202, token="ledger-202-token"), 200, balance=125)

            common = (
                b"POST /v1/debit HTTP/1.1\r\nHost: 127.0.0.1\r\n"
                b"Authorization: Bearer ledger-101-token\r\nX-Tenant-ID: 101\r\n"
            )
            expect(raw_request(common + b"Content-Length: abc\r\n\r\n1"), 400, error="invalid_http_framing")
            expect(raw_request(common + b"Content-Length: 1\r\nContent-Length: 1\r\n\r\n1"), 400, error="invalid_http_framing")
            expect(raw_request(common + b"Transfer-Encoding: chunked\r\nContent-Length: 1\r\n\r\n1"), 400, error="invalid_http_framing")
            expect(raw_request(common + b"Content-Length: 2048\r\n\r\n" + b"1" * 2048), 400, error="invalid_http_framing")
            expect(raw_request(common + b"Content-Length: 2\r\n\r\n1"), 400, error="invalid_http_framing")
            expect(raw_request(b"GET /v1/balance HTTP/1.1\r\nAuthorization: Bearer ledger-101-token\r\nX-Tenant-ID: 101\r\n\r\n"), 400, error="invalid_http_framing")
            expect(raw_request_parts(common + b"Content-Length: 2\r\n\r\n", b"1", b"5"), 200, tenant=101, balance=50, version=4)

        assert wal.is_file() and wal.stat().st_size > 0
        assert wal.read_text().count("@tx|") == 4
        with wal.open("a", encoding="utf-8") as stream:
            stream.write("@tx|5|0|torn|tenant,delta~n999,999|@endtx\n")

        with Server(binary, temp_dir):
            expect(request("GET", "/v1/balance", tenant=101, token="ledger-101-token"), 200, balance=50, version=4)
            expect(request("GET", "/v1/balance", tenant=202, token="ledger-202-token"), 200, balance=125, version=4)
            expect(request("POST", "/v1/debit", "5", 101, "ledger-101-token"), 200, balance=45, version=5)

        with Server(binary, temp_dir):
            expect(request("GET", "/v1/balance", tenant=101, token="ledger-101-token"), 200, balance=45, version=5)
            expect(request("GET", "/v1/balance", tenant=202, token="ledger-202-token"), 200, balance=125, version=5)

    print("PASS: independent HTTP, tenant isolation, malformed input, WAL restart and torn-frame recovery")


if __name__ == "__main__":
    main()
