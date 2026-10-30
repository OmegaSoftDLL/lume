#!/usr/bin/env python3
"""Observe the freestanding RTC driver under two independent QEMU clock bases."""

from datetime import datetime, timedelta
from pathlib import Path
import re
import shutil
import subprocess
import sys

LINE = re.compile(
    r"p0_rtc_kernel=ok year=(\d+) month=(\d+) day=(\d+) hour=(\d+) "
    r"minute=(\d+) second=(\d+) status_b=(\d+)"
)


def run_qemu(image, base):
    completed = subprocess.run(
        [
            "qemu-system-x86_64",
            "-drive",
            f"format=raw,file={image}",
            "-serial",
            "stdio",
            "-display",
            "none",
            "-no-reboot",
            "-device",
            "isa-debug-exit,iobase=0xf4,iosize=0x04",
            "-rtc",
            f"base={base}",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=20,
    )
    match = LINE.search(completed.stdout)
    assert match, f"missing RTC serial evidence\nstdout={completed.stdout}\nstderr={completed.stderr}"
    values = [int(value) for value in match.groups()]
    observed = datetime(*values[:6])
    expected = datetime.strptime(base, "%Y-%m-%dT%H:%M:%S")
    assert expected <= observed <= expected + timedelta(seconds=10), (base, observed, completed.stdout)
    assert 0 <= values[6] <= 255
    return observed, completed.stdout.strip()


def main():
    image = Path(sys.argv[1]).resolve()
    assembly = Path(sys.argv[2]).resolve()
    assert shutil.which("qemu-system-x86_64"), "qemu-system-x86_64 is required"
    image_bytes = image.read_bytes()
    assert image_bytes[:2] != b"\x7fELF", "a hosted ELF is not a freestanding boot image"
    assert image_bytes[510:512] == b"\x55\xaa", "missing real MBR boot signature"
    assert image.stat().st_size > 512
    asm = assembly.read_text()
    assert ".kernel" in asm
    assert "port_in8:" in asm and "inb %dx, %al" in asm
    assert "port_out8:" in asm and "outb %al, %dx" in asm
    assert not re.search(r"^\s*syscall$", asm, re.MULTILINE), "hosted syscall remained in kernel"

    first_base = "2024-06-15T12:34:56"
    second_base = "2031-11-22T01:02:03"
    first, first_serial = run_qemu(str(image), first_base)
    second, second_serial = run_qemu(str(image), second_base)
    assert first != second
    assert first.year == 2024 and second.year == 2031
    print(f"PASS: {first_serial}")
    print(f"PASS: {second_serial}")


if __name__ == "__main__":
    main()
