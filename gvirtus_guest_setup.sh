#!/usr/bin/env bash
set -euo pipefail

GVIRTUS_HOME="${GVIRTUS_HOME:-/opt/gvirtus}"
PORT="${GVIRTUS_PORT:-8888}"
CONFIG="${GVIRTUS_CONFIG:-/tmp/gvirtus-frontend.json}"
MODE="${1:-shell}"

need_pkg() {
  dpkg -s "$1" >/dev/null 2>&1 || return 0
  return 1
}

install_if_missing() {
  local pkg="$1"
  if ! dpkg -s "$pkg" >/dev/null 2>&1; then
    apt-get update
    DEBIAN_FRONTEND=noninteractive apt-get install -y "$pkg"
  fi
}

if ! command -v ip >/dev/null 2>&1; then
  install_if_missing iproute2
fi
if ! ldconfig -p | grep -q 'liblog4cplus-2.0.so.3'; then
  install_if_missing liblog4cplus-2.0.5
fi

HOST_IP="${GVIRTUS_HOST_IP:-$(ip route | awk '/default/ {print $3; exit}')}"
mkdir -p /tmp/gvirtus-libcuda-only
ln -sf "$GVIRTUS_HOME/lib/frontend/libcuda.so" /tmp/gvirtus-libcuda-only/libcuda.so
if [ -e "$GVIRTUS_HOME/lib/frontend/libcuda.so.1" ]; then
  ln -sf "$GVIRTUS_HOME/lib/frontend/libcuda.so.1" /tmp/gvirtus-libcuda-only/libcuda.so.1
else
  ln -sf "$GVIRTUS_HOME/lib/frontend/libcuda.so" /tmp/gvirtus-libcuda-only/libcuda.so.1
fi

cat > "$CONFIG" <<JSON
{
  "communicator": [
    {
      "endpoint": {
        "suite": "tcp/ip",
        "protocol": "tcp",
        "server_address": "${HOST_IP}",
        "port": "${PORT}"
      },
      "plugins": [
        "cuda",
        "cudart",
        "cublas",
        "curand",
        "cudnn",
        "cufft",
        "cusolver",
        "cusparse",
        "nvrtc",
        "nvml"
      ]
    }
  ],
  "secure_application": false
}
JSON

export GVIRTUS_HOME
export GVIRTUS_CONFIG="$CONFIG"
unset LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$GVIRTUS_HOME/lib/frontend:$GVIRTUS_HOME/lib:/tmp/gvirtus-libcuda-only:/usr/local/cuda/lib64:/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu"

cat <<MSG
[guest] GVIRTUS_HOME=$GVIRTUS_HOME
[guest] GVIRTUS_CONFIG=$GVIRTUS_CONFIG
[guest] HOST_IP=$HOST_IP
[guest] PORT=$PORT
[guest] LD_LIBRARY_PATH=$LD_LIBRARY_PATH
MSG

case "$MODE" in
  shell)
    exec bash
    ;;
  driver)
    python3 - <<'PY'
import ctypes
lib = ctypes.CDLL('libcuda.so.1')
cuInit = lib.cuInit; cuInit.argtypes=[ctypes.c_uint]; cuInit.restype=ctypes.c_int
cuDeviceGetCount = lib.cuDeviceGetCount; cuDeviceGetCount.argtypes=[ctypes.POINTER(ctypes.c_int)]; cuDeviceGetCount.restype=ctypes.c_int
count = ctypes.c_int(-1)
print('cuInit =', cuInit(0))
print('cuDeviceGetCount =', cuDeviceGetCount(ctypes.byref(count)))
print('count =', count.value)
PY
    ;;
  runtime)
    python3 - <<'PY'
import ctypes
lib = ctypes.CDLL('libcudart.so.12')
fn = lib.cudaGetDeviceCount; fn.argtypes=[ctypes.POINTER(ctypes.c_int)]; fn.restype=ctypes.c_int
count = ctypes.c_int(-1)
print('cudaGetDeviceCount =', fn(ctypes.byref(count)))
print('count =', count.value)
PY
    ;;
  torch)
    python3 - <<'PY'
import torch
print('torch import ok')
print('cuda available:', torch.cuda.is_available())
print('device count:', torch.cuda.device_count())
PY
    ;;
  versions)
    python3 - <<'PY'
import ctypes

cuda = ctypes.CDLL('libcuda.so.1')
rt = ctypes.CDLL('libcudart.so.12')

cuInit = cuda.cuInit
cuInit.argtypes = [ctypes.c_uint]
cuInit.restype = ctypes.c_int

cuDriverGetVersion = cuda.cuDriverGetVersion
cuDriverGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cuDriverGetVersion.restype = ctypes.c_int

cudaDriverGetVersion = rt.cudaDriverGetVersion
cudaDriverGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cudaDriverGetVersion.restype = ctypes.c_int

cudaRuntimeGetVersion = rt.cudaRuntimeGetVersion
cudaRuntimeGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cudaRuntimeGetVersion.restype = ctypes.c_int

v1 = ctypes.c_int(-1)
v2 = ctypes.c_int(-1)
v3 = ctypes.c_int(-1)

print('cuInit =', cuInit(0))
print('cuDriverGetVersion =', cuDriverGetVersion(ctypes.byref(v1)), 'value =', v1.value)
print('cudaDriverGetVersion =', cudaDriverGetVersion(ctypes.byref(v2)), 'value =', v2.value)
print('cudaRuntimeGetVersion =', cudaRuntimeGetVersion(ctypes.byref(v3)), 'value =', v3.value)
PY
    ;;
  version-driver)
    python3 - <<'PY'
import ctypes

cuda = ctypes.CDLL('libcuda.so.1')
cuInit = cuda.cuInit
cuInit.argtypes = [ctypes.c_uint]
cuInit.restype = ctypes.c_int
cuDriverGetVersion = cuda.cuDriverGetVersion
cuDriverGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cuDriverGetVersion.restype = ctypes.c_int
v = ctypes.c_int(-1)
print('cuInit =', cuInit(0))
print('cuDriverGetVersion =', cuDriverGetVersion(ctypes.byref(v)), 'value =', v.value)
PY
    ;;
  version-runtime)
    python3 - <<'PY'
import ctypes

rt = ctypes.CDLL('libcudart.so.12')
cudaDriverGetVersion = rt.cudaDriverGetVersion
cudaDriverGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cudaDriverGetVersion.restype = ctypes.c_int
cudaRuntimeGetVersion = rt.cudaRuntimeGetVersion
cudaRuntimeGetVersion.argtypes = [ctypes.POINTER(ctypes.c_int)]
cudaRuntimeGetVersion.restype = ctypes.c_int
v1 = ctypes.c_int(-1)
v2 = ctypes.c_int(-1)
print('cudaDriverGetVersion =', cudaDriverGetVersion(ctypes.byref(v1)), 'value =', v1.value)
print('cudaRuntimeGetVersion =', cudaRuntimeGetVersion(ctypes.byref(v2)), 'value =', v2.value)
PY
    ;;
  yolo-import)
    python3 - <<'PY'
from ultralytics import YOLO
print('YOLO import ok')
model = YOLO('yolov8n.pt')
print('model loaded')
PY
    ;;
  *)
    echo "unknown mode: $MODE" >&2
    echo "modes: shell | driver | runtime | torch | versions | version-driver | version-runtime | yolo-import" >&2
    exit 2
    ;;
esac
