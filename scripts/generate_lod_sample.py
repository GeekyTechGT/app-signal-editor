#!/usr/bin/env python3
"""Generate a dense CSV fixture for manual LOD rendering checks."""

from __future__ import annotations

import argparse
import math
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("tests/01.data/generated/sample_lod_dense_120s_dt_100us.csv"),
        help="CSV path to generate.",
    )
    parser.add_argument("--duration", type=float, default=120.0, help="Signal duration in seconds.")
    parser.add_argument("--dt", type=float, default=0.0001, help="Sampling time in seconds.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    sample_count = int(args.duration / args.dt) + 1
    args.output.parent.mkdir(parents=True, exist_ok=True)

    spike_indices = {
        int(25.0 / args.dt): 18.0,
        int(40.0 / args.dt): -14.0,
        int(60.0 / args.dt): 18.0,
        int(80.0 / args.dt): -14.0,
        int(95.0 / args.dt): 18.0,
    }

    with args.output.open("w", encoding="utf-8", newline="") as csv:
        csv.write("time,dense_signal,spiky_signal\n")
        for index in range(sample_count):
            t = index * args.dt
            dense = math.sin(2.0 * math.pi * 3.0 * t) + 0.2 * math.sin(2.0 * math.pi * 97.0 * t)
            spike = spike_indices.get(index, 0.1 * math.sin(2.0 * math.pi * 1.5 * t))
            csv.write(f"{t:.4f},{dense:.8f},{spike:.8f}\n")

    print(f"Generated {args.output} with {sample_count} samples per signal")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
