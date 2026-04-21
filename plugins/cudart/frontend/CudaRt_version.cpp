/*
 * gVirtuS -- A GPGPU transparent virtualization component.
 *
 * Copyright (C) 2009-2010  The University of Napoli Parthenope at Naples.
 *
 * This file is part of gVirtuS.
 *
 * gVirtuS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gVirtuS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gVirtuS; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Written By: Giuseppe Coviello <giuseppe.coviello@uniparthenope.it>,
 *             Department of Applied Science
 *
 * Edited By: Theodoros Aslanidis <theodoros.aslanidis@ucdconnect.ie>,
 *             Department of Computer Science, University College Dublin
 */

#include "CudaRt.h"
#include <cuda.h>
#include <dlfcn.h>

using namespace std;

extern "C" __host__ cudaError_t CUDARTAPI cudaDriverGetVersion(int *driverVersion) {
    if (driverVersion == nullptr) {
        return cudaErrorInvalidValue;
    }

    using CuDriverGetVersionFn = CUresult (*)(int *);

    dlerror();
    void *symbol = dlsym(RTLD_DEFAULT, "cuDriverGetVersion");
    if (symbol == nullptr) {
        symbol = dlsym(RTLD_DEFAULT, "cuDriverGetVersion_v2");
    }
    if (symbol == nullptr) {
        void *cudaHandle = dlopen("libcuda.so.1", RTLD_LAZY | RTLD_LOCAL);
        if (cudaHandle != nullptr) {
            symbol = dlsym(cudaHandle, "cuDriverGetVersion");
            if (symbol == nullptr) {
                symbol = dlsym(cudaHandle, "cuDriverGetVersion_v2");
            }
        }
    }
    if (symbol == nullptr) {
        *driverVersion = 0;
        return cudaErrorInsufficientDriver;
    }

    CUresult driverExit =
        reinterpret_cast<CuDriverGetVersionFn>(symbol)(driverVersion);
    if (driverExit == CUDA_SUCCESS) {
        return cudaSuccess;
    }

    *driverVersion = 0;
    return cudaErrorInsufficientDriver;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaRuntimeGetVersion(int *runtimeVersion) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::Execute("cudaRuntimeGetVersion");
    if (CudaRtFrontend::Success()) *runtimeVersion = CudaRtFrontend::GetOutputVariable<int>();
    return CudaRtFrontend::GetExitCode();
}
