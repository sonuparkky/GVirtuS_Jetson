# AGENTS.md

This repository is used for Jetson Orin Nano + Kata Containers + GVirtuS experiments.

Read this file before making changes. Keep edits minimal, reversible, and easy to test.

## Primary goal

Help the user get **remote CUDA working from a Kata guest to a Jetson host** using:
- a **host-side GVirtuS backend**
- a **guest-side frontend/interpose setup**
- a **YOLO / PyTorch validation path**

The current working direction is **libcuda-only interpose** inside the guest.
Do **not** assume full `libcudart` interpose works for PyTorch.

## Current technical state

What is already known:
- Host GVirtuS backend can listen on TCP.
- Guest can connect to the host backend.
- `cuInit()` and `cuDeviceGetCount()` through `libcuda.so.1` can succeed and report 1 device.
- `cudaGetDeviceCount()` from `libcudart.so.12` still fails, currently with CUDA runtime error 36 in the latest observed state.
- PyTorch import can succeed with `libcuda-only interpose`, but `torch.cuda.is_available()` is still false because runtime-level compatibility is incomplete.
- `cuGetProcAddress` / `cuGetProcAddress_v2` handling is an active debugging area.

Treat the above as the baseline unless the repo state clearly shows otherwise.

## Repo priorities

Prioritize these files and paths when relevant:
- `plugins/cudadr/frontend/CudaDr_driver_entry_point.cpp`
- `plugins/cudadr/backend/CudaDrHandler_driver_entry_point.cpp`
- `build/lib/frontend/libcuda.so`
- host/guest helper scripts used to launch backend and configure guest env

If you add scripts, keep them small and shellcheck-friendly.

## Working model

There are two distinct execution contexts. Do not mix them up.

### Host
- Runs `gvirtus-backend`
- Should use `LD_LIBRARY_PATH=$GVIRTUS_HOME/lib`
- Should **not** load frontend shim libraries
- Usually binds `0.0.0.0:<port>`

### Guest
- Usually mounts the built GVirtuS tree at `/opt/gvirtus`
- Uses a generated frontend JSON config pointing to the host-reachable IP
- Uses **libcuda-only interpose** by exposing GVirtuS `libcuda.so` ahead of the system driver libraries
- Should **not** prepend the entire frontend directory to `LD_LIBRARY_PATH` for PyTorch tests

## Environment assumptions

Unless the user says otherwise, assume:
- Host build/install tree: `~/GVirtuS/build`
- Guest mount path: `/opt/gvirtus`
- Frontend config path: `/tmp/gvirtus-frontend.json`
- Default test port: `8888`
- Guest host-reachable IP often comes from:
  - `ip route | awk '/default/ {print $3; exit}'`

## What to do first on coding tasks

1. Read this file.
2. Inspect the exact file(s) that are likely involved.
3. Prefer a minimal patch over broad refactors.
4. Preserve existing behavior unless the change is directly related to the current bug.
5. After changes, explain how to test from both host and guest.

## Safe workflow

When you change behavior related to CUDA driver/runtime interaction:
- Prefer **instrumentation first**, then behavior changes.
- Add temporary debug logs only where they directly help locate the next missing symbol or unsupported path.
- Remove or gate noisy logs once the issue is understood.

For `cuGetProcAddress*` work:
- Remember that returned function pointers must be usable in the **guest** process.
- Do **not** return raw host-side pointers from the backend.
- Prefer local/frontend symbol resolution for guest-callable wrappers.

## Testing order

Use this order. Do not jump straight to YOLO unless earlier steps pass.

1. **Host listener check**
   - verify backend is listening on the expected port
2. **Guest driver API check**
   - `cuInit()`
   - `cuDeviceGetCount()`
3. **Guest runtime API check**
   - `cudaGetDeviceCount()`
4. **PyTorch availability check**
   - `import torch`
   - `torch.cuda.is_available()`
   - `torch.cuda.device_count()`
5. **YOLO import / model load**
6. **YOLO inference**

If a lower layer fails, fix that layer before moving up.

## Failure interpretation guide

- `liblog4cplus-2.0.so.3` missing:
  - guest runtime dependency is missing; install the guest package
- direct `cuDeviceGetCount()` works but `cudaGetDeviceCount()` fails:
  - driver-level path works, runtime compatibility is incomplete
- PyTorch imports but reports no CUDA device:
  - runtime/c10 path still lacks required CUDA behavior
- undefined symbol from `libcudart.so.12`:
  - avoid full frontend `libcudart` interpose for PyTorch unless deliberately working on that problem

## Code style expectations

- Keep C/C++ patches tight and explicit.
- Do not introduce large abstractions.
- Prefer readable control flow over clever macros.
- Add comments only when the reason is non-obvious.
- Do not silently change host/guest assumptions.

## Shell script expectations

If creating or editing shell scripts:
- use `#!/usr/bin/env bash`
- use `set -euo pipefail`
- make paths configurable via environment variables
- print the effective config before executing
- avoid destructive actions unless explicitly requested

## Git and patch hygiene

- Use small commits.
- Keep one logical change per commit.
- Include enough context in commit messages to explain whether the change affects:
  - frontend symbol export
  - guest interpose setup
  - backend transport/config
  - runtime/PyTorch validation

## When stuck

If behavior is unclear, do not guess.
Instead:
- add a narrowly scoped log
- print the requested symbol/API name
- compare guest-visible symbols against the real CUDA library
- state clearly whether the failure is at the driver layer, runtime layer, or framework layer

## Avoid these mistakes

- Do not assume `127.0.0.1` inside the guest reaches the host backend.
- Do not prepend `/opt/gvirtus/lib/frontend` wholesale for PyTorch testing.
- Do not return host function pointers to the guest from `cuGetProcAddress`-style APIs.
- Do not claim YOLO works unless an actual guest inference test succeeded.
- Do not skip intermediate verification steps.

## Preferred deliverables

When finishing a task, provide:
- the exact file(s) changed
- the minimal commands to test on the host
- the minimal commands to test in the guest
- what result is expected at each step
- any remaining blocker, if not fully fixed
