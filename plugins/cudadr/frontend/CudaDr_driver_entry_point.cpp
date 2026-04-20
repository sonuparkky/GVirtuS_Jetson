#include <cuda.h>
#include <dlfcn.h>

#ifdef cuGetProcAddress
#undef cuGetProcAddress
#endif

extern "C" CUresult cuGetProcAddress_v2(
    const char* symbol,
    void** pfn,
    int cudaVersion,
    cuuint64_t flags,
    CUdriverProcAddressQueryResult* symbolStatus)
{
    if (!symbol || !pfn) {
        if (symbolStatus) {
            *symbolStatus = CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND;
        }
        return CUDA_ERROR_INVALID_VALUE;
    }

    dlerror();
    void* fp = dlsym(RTLD_DEFAULT, symbol);

    if (!fp) {
        if (symbolStatus) {
            *symbolStatus = CU_GET_PROC_ADDRESS_SYMBOL_NOT_FOUND;
        }
        *pfn = nullptr;
        return CUDA_ERROR_NOT_FOUND;
    }

    *pfn = fp;

    if (symbolStatus) {
        *symbolStatus = CU_GET_PROC_ADDRESS_SUCCESS;
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
