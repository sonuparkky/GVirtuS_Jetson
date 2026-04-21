# YOLO GVirtuS Blocker Check - 2026-04-21

## Scope

This log records the blocker checks performed before running YOLO in a Kata
guest on the remote Jetson host.

The host was reached with:

```bash
ssh sunwoo@155.230.91.240 -p 51200
```

The Kata launch path was:

```bash
cd ~
GVIRTUS_DIR=/home/sunwoo/GVirtuS/build ./run_gvirtus_kata_shell.sh ...
```

The guest frontend config used the host bridge address:

```json
{
  "server_address": "172.17.0.1",
  "port": "8888"
}
```

## Blockers Found

### Full frontend interpose breaks PyTorch import

Using `/gvirtus/lib/frontend` wholesale in `LD_LIBRARY_PATH` makes PyTorch load
GVirtuS `libcudart.so.12`. Initially, `torch` failed to import because
`libc10_cuda.so` required a runtime symbol that GVirtuS did not export:

```text
undefined symbol: cudaGraphNodeGetDependencies, version libcudart.so.12
```

After adding that export, `libtorch_cuda.so` exposed more missing runtime
symbols:

```text
cudaUserObjectCreate
cudaGraphRetainUserObject
cudaGraphAddDependencies
cudaStreamGetCaptureInfo_v3
cudaGraphAddDependencies_v2
cudaStreamUpdateCaptureDependencies_v2
cudaMallocFromPoolAsync
cudaGraphAddEventRecordNode
cudaStreamUpdateCaptureDependencies
cudaGetFuncBySymbol
```

Those symbols were added as minimal frontend stubs so PyTorch can link farther.

Full `libcudart` interpose is still not usable for PyTorch because import then
reaches `__cudaRegisterFunction`, where GVirtuS runtime buffer unmarshalling
fails:

```text
LZ4 decompression failed with code -6
Buffer::Get(): Can't read any unsigned long
```

### Libcuda-only interpose imports PyTorch but CUDA is unavailable

With only GVirtuS `libcuda.so.1` interposed, `torch` imports successfully, but
CUDA runtime initialization still reports error 36:

```text
torch 2.10.0 cuda_available False count 0
Error 36: API call is not supported in the installed CUDA driver
```

Direct GVirtuS driver calls still work:

```text
cuInit 0
cuDeviceGetCount 0 1
```

This means the current blocker for GPU-backed YOLO is still at the
runtime/PyTorch CUDA initialization layer, not YOLO itself.

## Changes Applied

`plugins/cudadr/frontend/CudaDr_driver_entry_point.cpp`:

- kept guest-local `cuGetProcAddress_v2` resolution
- added fallback from unversioned driver names to `_v2` exports, for example
  `cuDeviceTotalMem` to `cuDeviceTotalMem_v2`
- added small guest-local query wrappers for device/context metadata requested
  by CUDA 12.6 runtime initialization

`plugins/cudart/frontend/CudaRt_graph.cpp`:

- added minimal CUDA graph/user-object/runtime symbols needed by PyTorch shared
  library linking

## Working Path Confirmed

YOLO can currently run in the Kata guest on CPU fallback with `libcuda-only`
interpose:

```text
cuInit 0
cuDeviceGetCount 0 1
torch 2.10.0 cuda_available False count 0
yolo inference ok 1 torch.Size([0, 6])
python_rc=0
```

The inference test used a zero image:

```python
import numpy as np
from ultralytics import YOLO

model = YOLO("yolov8n.pt")
results = model.predict(np.zeros((320, 320, 3), dtype=np.uint8),
                        device="cpu",
                        imgsz=320,
                        verbose=False)
```

## Remaining GPU Blocker

YOLO GPU inference is not working yet because PyTorch still reports no CUDA
device through the CUDA runtime path. The next useful target is to identify the
exact runtime-required CUDA driver API that turns `cudaGetDeviceCount()` into
runtime error 36 under `libcuda-only` interpose.

## Follow-up Debugging

After comparing against the Jetson CUDA 12.6 driver headers and host
`libcuda.so.1`, `cuGetProcAddress_v2` was adjusted to match the real driver
contract for missing symbols:

```text
CUresult = CUDA_SUCCESS
symbolStatus = CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND
pfn = nullptr
```

The previous behavior returned `CUDA_ERROR_NOT_FOUND`, which is not what the
real driver returns for `cuGetProcAddress_v2`.

Additional guest-local driver entry points were added for CUDA runtime
initialization probes:

```text
cuDeviceGetDefaultMemPool
cuDeviceGetMemPool
cuDeviceSetMemPool
cuDeviceGraphMemTrim
cuDeviceGetGraphMemAttribute
cuDeviceSetGraphMemAttribute
cuDeviceGetNvSciSyncAttributes
cuGetExportTable
```

The error changed from runtime error 36 to driver error 801
(`CUDA_ERROR_NOT_SUPPORTED`). Trace output shows that real `libcudart.so.12`
is now reaching a GVirtuS guest-local unsupported driver stub during CUDA
runtime initialization, after driver symbol table setup:

```text
GVirtuS unsupported driver stub called from /usr/local/cuda/lib64/libcudart.so.12
cudaGetDeviceCount 801 -1
```

An opt-in debugging knob, `GVIRTUS_CUDADR_STUB_SUCCESS=1`, was tested to make
generic missing driver stubs return success. That path segfaulted during
PyTorch CUDA initialization, so it is not a valid fix. The remaining blocker is
therefore not just symbol presence; at least one CUDA driver export-table or
runtime-initialization query needs a semantically valid guest-side
implementation.
