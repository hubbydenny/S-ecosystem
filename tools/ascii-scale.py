#!/usr/bin/env python3
"""ascii-scale: shrink ASCII art preserving the picture.

Usage: ascii-scale.py <input.txt> <new_width> [output.txt]
"""
import sys

RAMP = list(" .`:;+xX$#")


def ink(ch: str) -> float:
    if ch == " ":
        return 0.0
    idx = RAMP.index(ch) if ch in RAMP else len(RAMP) - 1
    return idx / (len(RAMP) - 1)


def main() -> None:
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src = open(sys.argv[1]).read().splitlines()
    new_w = max(1, int(sys.argv[2]))
    out_path = sys.argv[3] if len(sys.argv) > 3 else None

    width = max(len(l) for l in src)
    lines = [l.ljust(width) for l in src]
    height = len(lines)

    sx = width / new_w
    new_h = max(1, round(height / (sx * 2)))
    sy = height / new_h

    out_lines = []
    for j in range(new_h):
        row = []
        for i in range(new_w):
            y0, y1 = int(j * sy), max(int((j + 1) * sy), int(j * sy) + 1)
            x0, x1 = int(i * sx), max(int((i + 1) * sx), int(i * sx) + 1)
            total = n = 0
            for y in range(y0, min(y1, height)):
                for x in range(x0, min(x1, width)):
                    total += ink(lines[y][x])
                    n += 1
            avg = total / n if n else 0.0
            row.append(RAMP[round(avg * (len(RAMP) - 1))])
        out_lines.append("".join(row).rstrip())

    result = "\n".join(out_lines) + "\n"
    if out_path:
        open(out_path, "w").write(result)
    else:
        sys.stdout.write(result)


if __name__ == "__main__":
    main()
