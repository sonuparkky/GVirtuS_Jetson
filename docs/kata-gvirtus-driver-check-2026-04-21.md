# Kata GVirtuS Driver Check - 2026-04-21

## Scope

This log records the minimum GVirtuS/Kata validation performed on the remote
Jetson Orin Nano host.

Validation stopped at the CUDA driver API layer:

- host-side GVirtuS backend listener
- Kata container startup
- guest-side `libcuda.so.1` load through GVirtuS frontend
- guest-side `cuInit()`
- guest-side `cuDeviceGetCount()`

Runtime API, PyTorch, and YOLO validation were not tested in this check.

## Remote Host

- SSH target: `sunwoo@155.230.91.240 -p 51200`
- Hostname observed: `sunwoo-desktop`
- GVirtuS source/build tree used: `/home/sunwoo/GVirtuS/build`
- Kata launch script used: `/home/sunwoo/run_gvirtus_kata_shell.sh`

## Host Backend Check

The script was run from the Jetson home directory with the build tree selected:

```bash
cd ~
GVIRTUS_DIR=/home/sunwoo/GVirtuS/build ./run_gvirtus_kata_shell.sh bash
```

The script started or reused the backend and confirmed the listener:

```text
reusing existing gVirtuS backend
gVirtuS backend reachable at 127.0.0.1:8888
starting Kata container image=ultralytics/ultralytics:latest-jetson-jetpack6 runtime=kata
```

Host listener state also showed GVirtuS listening on port `8888`:

```text
LISTEN 0 5 0.0.0.0:8888 0.0.0.0:* users:(("gvirtus-backend",pid=30457,fd=3))
```

## Guest Container Check

Inside the Kata container, the GVirtuS frontend library was visible:

```text
GVIRTUS_HOME=/gvirtus
GVIRTUS_CONFIG=/gvirtus/etc/properties.json
LD_LIBRARY_PATH=/gvirtus/lib/frontend:/gvirtus/lib:/usr/local/cuda/lib64:/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu
lrwxrwxrwx 1 1000 1000     12 Apr 20 11:34 /gvirtus/lib/frontend/libcuda.so -> libcuda.so.1
-rw-r--r-- 1 1000 1000 205768 Apr 20 13:26 /gvirtus/lib/frontend/libcuda.so.1
```

Using the mounted config directly failed because `/gvirtus/etc/properties.json`
points to `127.0.0.1`. In a Kata guest, that address refers to the guest, not
the Jetson host backend:

```text
FATAL - "Frontend.cpp":139: Exception occurred: TcpCommunicator: Can't connect to socket: Connection refused.
```

The successful check used the same `./run_gvirtus_kata_shell.sh` path, but
created a temporary guest frontend config pointing at the host bridge address:

```bash
GVIRTUS_DIR=/home/sunwoo/GVirtuS/build ./run_gvirtus_kata_shell.sh bash -lc 'set -e; cat > /tmp/gvirtus-frontend.json <<JSON
{
  "communicator": [
    {
      "endpoint": {
        "suite": "tcp/ip",
        "protocol": "tcp",
        "server_address": "172.17.0.1",
        "port": "8888"
      },
      "plugins": ["cuda", "cudart", "cublas", "curand", "cudnn", "cufft", "cusolver", "cusparse", "nvrtc", "nvml"]
    }
  ],
  "secure_application": false
}
JSON
export GVIRTUS_CONFIG=/tmp/gvirtus-frontend.json
python3 - <<'"'"'PY'"'"'
import ctypes
lib = ctypes.CDLL("libcuda.so.1")
cuInit = lib.cuInit
cuInit.argtypes = [ctypes.c_uint]
cuInit.restype = ctypes.c_int
cuDeviceGetCount = lib.cuDeviceGetCount
cuDeviceGetCount.argtypes = [ctypes.POINTER(ctypes.c_int)]
cuDeviceGetCount.restype = ctypes.c_int
count = ctypes.c_int(-1)
print("cuInit =", cuInit(0))
print("cuDeviceGetCount =", cuDeviceGetCount(ctypes.byref(count)))
print("count =", count.value)
PY'
```

## Result

The guest driver API check succeeded:

```text
[guest] GVIRTUS_CONFIG=/tmp/gvirtus-frontend.json
INFO - Using properties file: /tmp/gvirtus-frontend.json
INFO - Parsing endpoint config from: /tmp/gvirtus-frontend.json
INFO - Parsed endpoint suite: tcp/ip
INFO - Initializing TCP/IP Endpoint
DEBUG: protocol string is [tcp]
cuInit = 0
cuDeviceGetCount = 0
count = 1
```

## Interpretation

GVirtuS is working through the Kata guest at the CUDA driver API layer when the
guest frontend config targets the Jetson host-reachable address.

The remaining known boundary is above this layer:

- `cudaGetDeviceCount()` was not tested in this check.
- PyTorch CUDA availability was not tested in this check.
- YOLO import/model load/inference was not tested in this check.

