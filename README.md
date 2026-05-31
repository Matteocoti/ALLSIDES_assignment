# FakeCamera + HDR Combiner

Software simulation of an industrial HDR capture pipeline. A `FakeCamera` simulates a
12-bit grayscale sensor; three exposures are acquired, merged into a floating-point HDR
radiance map by an `HdrCombiner`, and tone-mapped to a viewable 8-bit image. No camera
hardware and no dependencies beyond the C++17 standard library.

## Build & run

Requirements: C++17 compiler, CMake >= 3.16.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hdr_scanner
```

Outputs in the working directory: `short.pgm`, `medium.pgm`, `long.pgm` (raw 16-bit
frames) and `hdr.pgm` (8-bit tone-mapped result). Per-grab and merge timings are printed.

## Sensor model (FakeCamera)

Linear exposure model: `pixel = clamp(scene * gain + read_noise, 0, MAX_VAL)`, with
`gain` proportional to the exposure time (normalised so Medium = 1).

- Exposures: Short 5 ms, Medium 40 ms, Long 320 ms.
- Read noise: additive zero-mean Gaussian (sigma configurable).
- Saturation: the upper clamp at `MAX_VAL` models clipping at long exposure.
- Scene consistency: the same radiance map feeds every exposure; only `gain` changes.

Noise choice: only Gaussian read noise is modelled. Poisson shot noise was intentionally
left out.

## HDR merge: Robertson et al

Iterative maximum-likelihood estimation. It jointly estimates the per-pixel irradiance
`x` and the camera response `f`, alternating:

- Step A: `x_j = sum_i w * f(z) * t_i / sum_i w * t_i^2`
- Step B: re-estimate `f(m)` as the weighted mean of `t_i * x_j` over samples of value
  `m`, then normalise at mid-range to fix the scale ambiguity.

Reliability weight: a Gaussian centred at mid-range, zero at 0 and `MAX_VAL`, so saturated
and underexposed pixels are excluded by construction.

Why Robertson:

- Excludes unreliable (clipped / noise-buried) samples through the weight.
- Recovers the response from the data instead of assuming it — more general than a plain
  weighted average.

Assumptions: near-linear response, static scene, known exposure ratios.
