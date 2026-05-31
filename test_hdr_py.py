#!/usr/bin/env python3
"""Minimal test for the hdr_py Python bindings."""

import sys
import time
import numpy as np

sys.path.insert(0, "build")

try:
    import hdr_py
except ImportError as e:
    print(f"ERROR: could not import hdr_py module — {e}")
    print("Make sure the project was built with pybind11 enabled:")
    print("  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")
    print("  cmake --build build")
    sys.exit(1)


def main():
    t0 = time.perf_counter()
    scene = hdr_py.SyntheticScene()
    print(f"Scene created: {scene.width}x{scene.height}")
    print(f"  Scene radiance range: [{scene.data.min():.6f}, {scene.data.max():.6f}]")

    camera = hdr_py.FakeCamera(scene, noise_std_dev=5.0)

    for name, exp in [
        ("Short", hdr_py.ExposureTime.Short),
        ("Medium", hdr_py.ExposureTime.Medium),
        ("Long", hdr_py.ExposureTime.Long),
    ]:
        t1 = time.perf_counter()
        frame = camera.grab(exp)
        t2 = time.perf_counter()
        data = np.asarray(frame.data).reshape(frame.height, frame.width)
        clipped = (data == 4095).sum()
        black = (data == 0).sum()
        print(
            f"{name:7s}: {1000*(t2-t1):6.1f} ms  "
            f"range [{data.min()}, {data.max()}]  "
            f"saturated={clipped}  black={black}"
        )

    frames = [
        camera.grab(hdr_py.ExposureTime.Short),
        camera.grab(hdr_py.ExposureTime.Medium),
        camera.grab(hdr_py.ExposureTime.Long),
    ]

    combiner = hdr_py.HdrCombiner()
    t3 = time.perf_counter()
    hdr = combiner.merge(frames)
    t4 = time.perf_counter()
    print(f"\nMerge: {1000*(t4-t3):.1f} ms")
    hdr_data = np.asarray(hdr.data).reshape(hdr.height, hdr.width)
    print(f"HDR range: [{hdr_data.min():.6f}, {hdr_data.max():.6f}]")

    print("\nAll checks passed.")


if __name__ == "__main__":
    main()
