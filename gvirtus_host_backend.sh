#!/usr/bin/env bash
set -euo pipefail

GVIRTUS_HOME="${GVIRTUS_HOME:-$HOME/GVirtuS/build}"
PORT="${GVIRTUS_PORT:-8888}"
BIND_ADDR="${GVIRTUS_BIND_ADDR:-0.0.0.0}"
CONFIG="${GVIRTUS_BACKEND_CONFIG:-$GVIRTUS_HOME/etc/backend-kata.json}"

mkdir -p "$(dirname "$CONFIG")"
cat > "$CONFIG" <<JSON
{
  "communicator": [
    {
      "endpoint": {
        "suite": "tcp/ip",
        "protocol": "tcp",
        "server_address": "${BIND_ADDR}",
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
unset LD_LIBRARY_PATH
export LD_LIBRARY_PATH="$GVIRTUS_HOME/lib"

echo "[host] GVIRTUS_HOME=$GVIRTUS_HOME"
echo "[host] config=$CONFIG"
echo "[host] bind=${BIND_ADDR}:${PORT}"
echo "[host] LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "[host] starting backend..."
exec "$GVIRTUS_HOME/bin/gvirtus-backend" "$CONFIG"
