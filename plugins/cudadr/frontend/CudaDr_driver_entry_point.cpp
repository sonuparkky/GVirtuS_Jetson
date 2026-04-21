#include <cuda.h>
#include <dlfcn.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifdef cuGetProcAddress
#undef cuGetProcAddress
#endif

extern "C" CUresult gvirtusUnsupportedDriverEntryPoint(
    unsigned long long arg0,
    unsigned long long arg1,
    unsigned long long arg2,
    unsigned long long arg3,
    unsigned long long arg4,
    unsigned long long arg5)
{
    if (std::getenv("GVIRTUS_CUDADR_TRACE_STUB") != nullptr) {
        void* caller = __builtin_return_address(0);
        Dl_info info{};
        if (dladdr(caller, &info) != 0) {
            std::fprintf(stderr, "GVirtuS unsupported driver stub called from %s %p+0x%zx (%s)\n",
                         info.dli_fname ? info.dli_fname : "unknown", caller,
                         static_cast<size_t>(reinterpret_cast<char*>(caller) -
                                             reinterpret_cast<char*>(info.dli_fbase)),
                         info.dli_sname ? info.dli_sname : "unknown");
        } else {
            std::fprintf(stderr, "GVirtuS unsupported driver stub called from %p\n", caller);
        }
        Dl_info argInfo{};
        if (dladdr(reinterpret_cast<void*>(arg1), &argInfo) != 0) {
            std::fprintf(stderr,
                         "GVirtuS unsupported driver stub args=%#llx %#llx+0x%zx %#llx %#llx %#llx %#llx\n",
                         arg0, arg1,
                         static_cast<size_t>(reinterpret_cast<char*>(arg1) -
                                             reinterpret_cast<char*>(argInfo.dli_fbase)),
                         arg2, arg3, arg4, arg5);
        } else {
            std::fprintf(stderr,
                         "GVirtuS unsupported driver stub args=%#llx %#llx %#llx %#llx %#llx %#llx\n",
                         arg0, arg1, arg2, arg3, arg4, arg5);
        }
    }
    if (std::getenv("GVIRTUS_CUDADR_STUB_SUCCESS") != nullptr) {
        if (arg0 != 0) {
            *reinterpret_cast<unsigned long long*>(arg0) = 0;
        }
        return CUDA_SUCCESS;
    }
    return CUDA_ERROR_NOT_SUPPORTED;
}

extern "C" CUresult cuDeviceGetP2PAttribute(int* value, CUdevice_P2PAttribute, CUdevice, CUdevice)
{
    if (!value) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *value = 0;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetByPCIBusId(CUdevice* dev, const char*)
{
    if (!dev) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *dev = 0;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetPCIBusId(char* pciBusId, int len, CUdevice)
{
    if (!pciBusId || len <= 0) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    std::snprintf(pciBusId, static_cast<size_t>(len), "0000:00:00.0");
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetUuid(CUuuid* uuid, CUdevice)
{
    if (!uuid) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    std::memset(uuid, 0, sizeof(*uuid));
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetTexture1DLinearMaxWidth(size_t* maxWidthInElements, CUarray_format, unsigned, CUdevice)
{
    if (!maxWidthInElements) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *maxWidthInElements = 0;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDevicePrimaryCtxSetFlags(CUdevice, unsigned int)
{
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetDefaultMemPool(CUmemoryPool* pool, CUdevice)
{
    if (!pool) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *pool = reinterpret_cast<CUmemoryPool>(1);
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetMemPool(CUmemoryPool* pool, CUdevice)
{
    if (!pool) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *pool = reinterpret_cast<CUmemoryPool>(1);
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceSetMemPool(CUdevice, CUmemoryPool)
{
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGraphMemTrim(CUdevice)
{
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetGraphMemAttribute(CUdevice, CUgraphMem_attribute, void* value)
{
    if (!value) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *static_cast<unsigned long long*>(value) = 0;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceSetGraphMemAttribute(CUdevice, CUgraphMem_attribute, void*)
{
    return CUDA_SUCCESS;
}

extern "C" CUresult cuDeviceGetNvSciSyncAttributes(void*, CUdevice, int)
{
    return CUDA_SUCCESS;
}

extern "C" CUresult cuCtxGetFlags(unsigned int* flags)
{
    if (!flags) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *flags = 0;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuCtxGetApiVersion(CUcontext, unsigned int* version)
{
    if (!version) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *version = 12060;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuGetExportTable(const void** ppExportTable, const CUuuid*)
{
    if (!ppExportTable) {
        return CUDA_ERROR_INVALID_VALUE;
    }
    *ppExportTable = nullptr;
    return CUDA_SUCCESS;
}

extern "C" CUresult cuGetProcAddress_v2(
    const char* symbol,
    void** pfn,
    int cudaVersion,
    cuuint64_t flags,
    CUdriverProcAddressQueryResult* symbolStatus)
{
    const bool trace = std::getenv("GVIRTUS_CUDADR_TRACE_GETPROC") != nullptr;

    if (!symbol || !pfn) {
        if (symbolStatus) {
            *symbolStatus = CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND;
        }
        if (trace) {
            std::fprintf(stderr, "GVirtuS cuGetProcAddress_v2 invalid symbol=%p pfn=%p\n",
                         static_cast<const void*>(symbol), static_cast<void*>(pfn));
        }
        return CUDA_ERROR_INVALID_VALUE;
    }

    if (symbol[0] == '\0') {
        if (symbolStatus) {
            *symbolStatus = CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND;
        }
        *pfn = nullptr;
        if (trace) {
            std::fprintf(stderr, "GVirtuS cuGetProcAddress_v2 empty symbol\n");
        }
        return CUDA_SUCCESS;
    }

    dlerror();
    void* fp = dlsym(RTLD_DEFAULT, symbol);

    if (!fp) {
        char versionedSymbol[256];
        if (std::snprintf(versionedSymbol, sizeof(versionedSymbol), "%s_v2", symbol) > 0 &&
            std::strlen(versionedSymbol) < sizeof(versionedSymbol)) {
            fp = dlsym(RTLD_DEFAULT, versionedSymbol);
        }
    }

    if (!fp) {
        fp = reinterpret_cast<void*>(&gvirtusUnsupportedDriverEntryPoint);
        if (symbolStatus) {
            *symbolStatus = CU_GET_PROC_ADDRESS_SUCCESS;
        }
        *pfn = fp;
        if (trace) {
            std::fprintf(stderr,
                         "GVirtuS cuGetProcAddress_v2 stub symbol=%s cudaVersion=%d flags=%llu fp=%p\n",
                         symbol, cudaVersion, static_cast<unsigned long long>(flags), fp);
        }
        return CUDA_SUCCESS;
    }

    *pfn = fp;

    if (symbolStatus) {
        *symbolStatus = CU_GET_PROC_ADDRESS_SUCCESS;
    }

    if (trace) {
        std::fprintf(stderr,
                     "GVirtuS cuGetProcAddress_v2 hit symbol=%s cudaVersion=%d flags=%llu fp=%p\n",
                     symbol, cudaVersion, static_cast<unsigned long long>(flags), fp);
    }

    return CUDA_SUCCESS;
}

extern "C" CUresult cuGetProcAddress(
    const char* symbol,
    void** pfn,
    int cudaVersion,
    cuuint64_t flags)
{
    CUdriverProcAddressQueryResult symbolStatus{};
    return cuGetProcAddress_v2(symbol, pfn, cudaVersion, flags, &symbolStatus);
}
