# Contributing

This kit sits in the **dcompute / LDC DirectX** domain. Keep changes focused and reviewable — same discipline as [DMD CONTRIBUTING — “will not be viewed with favor”](https://github.com/dlang/dmd/blob/master/CONTRIBUTING.md#the-following-will-not-be-viewed-with-favor):

1. Do not shuffle unrelated code.  
2. No tree-wide sed / bulk renames for style.  
3. Do not reformat to personal taste; match surrounding style / [D Style](https://dlang.org/dstyle.html) for new D code.

## Rules for this repo

- One concern per change (fixture, validator rule, HLSL kernel, or docs).  
- Document **why** (e.g. “LLVM test X requires attr Y”), not only what.  
- Prefer user-visible / mentor-visible outcomes: failing fixtures, DXIL on disk, clearer backend map.  
- Do not claim this *is* LDC `targetDirectX` or official dcompute until it lands upstream.  
- Keep this repository focused on DirectX compute reference work; do not mix unrelated tooling.
