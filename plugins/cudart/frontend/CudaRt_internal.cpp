
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
 *            School of Computer Science, University College Dublin
 */

#include <CudaRt_internal.h>
#include <dlfcn.h>
#include <lz4.h>

#include <cstdint>
#include <cstdio>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "CudaRt.h"

// Helper: allocate and copy section headers table
Elf64_Shdr *copySectionHeaders(const Elf64_Ehdr *eh) {
    // cout << "Will malloc "<< eh->e_shnum << " section headers of " << eh->e_shentsize << " bytes
    // each." << endl;
    Elf64_Shdr *sh_table = (Elf64_Shdr *)malloc(eh->e_shentsize * eh->e_shnum);
    if (!sh_table) return nullptr;

    byte *baseAddr = (byte *)eh;
    for (uint32_t i = 0; i < eh->e_shnum; i++) {
        // cout << "Section header " << i << " starts at adress: " << hex << (baseAddr + eh->e_shoff
        // + i * eh->e_shentsize) << dec << endl;
        Elf64_Shdr *src = (Elf64_Shdr *)(baseAddr + eh->e_shoff + i * eh->e_shentsize);
        memcpy(&sh_table[i], src, eh->e_shentsize);
    }
    return sh_table;
}

// Helper: allocate and copy section header string table
char *copySectionHeaderStrTable(const Elf64_Ehdr *eh, Elf64_Shdr *sh_table) {
    uint8_t *sh_str_table_bytes = (uint8_t *)&sh_table[eh->e_shstrndx];
    // for (int i = 0; i < sizeof(Elf64_Shdr); i++) {
    //     printf("%02x ", sh_str_table_bytes[i]);
    // }
    // printf("\n");

    size_t offset = sh_table[eh->e_shstrndx].sh_offset;
    size_t size = sh_table[eh->e_shstrndx].sh_size;
    // cout << "sh_table address: " << hex << (void *)sh_table << dec << endl;
    // cout << "offset address: " << hex << (void *)(&sh_table[eh->e_shstrndx].sh_offset) << dec <<
    // endl; cout << "size address: " << hex << (void *)(&sh_table[eh->e_shstrndx].sh_size) << dec
    // << endl; cout << "section header string table index: " << eh->e_shstrndx << " has offset: "
    // << offset << " and size: " << size << endl;
    char *sh_str = (char *)malloc(size);
    if (!sh_str) return nullptr;

    byte *baseAddr = (byte *)eh;
    memcpy(sh_str, baseAddr + offset, size);
    return sh_str;
}

// Helper: parse NvInfo sections and register functions and their parameters
void parseNvInfoKParams(const Elf64_Ehdr *eh, Elf64_Shdr *sh_table, char *sh_str) {
    byte *baseAddr = (byte *)eh;
    // cout << "Processing " << eh->e_shnum << " sections." << endl;
    for (uint32_t i = 0; i < eh->e_shnum; i++) {
        // cout << "Processing section " << i + 1 << " of " << eh->e_shnum << endl;
        char *sectionName = sh_str + sh_table[i].sh_name;
        if (strncmp(".nv.info.", sectionName, strlen(".nv.info.")) != 0) {
            // cout << "Skipping section: " << sectionName << endl;
            continue;
        }

        char *funcName = sectionName + strlen(".nv.info.");
        // cout << "Found NvInfo section: " << funcName << endl;
        byte *sectionData = baseAddr + sh_table[i].sh_offset;

        NvInfoFunction infoFunction;

        NvInfoAttribute *pAttr = (NvInfoAttribute *)sectionData;
        byte *sectionEnd = sectionData + sh_table[i].sh_size;

        // cout << "Section data start at: " << hex << sectionData << " and end at: " << sectionEnd
        // << dec << endl;

        while ((byte *)pAttr < sectionEnd) {
            size_t size = sizeof(NvInfoAttribute);
            // cout << "Processing attribute: " << pAttr->attr << ", fmt: " << pAttr->fmt << ",
            // value: " << pAttr->value << endl;
            if (pAttr->fmt == EIFMT_SVAL) {
                // cout << "Attribute is a string value." << endl;
                size += pAttr->value;
            }
            if (pAttr->attr == EIATTR_KPARAM_INFO) {
                // cout << "Attribute is a KParam info." << endl;
                NvInfoKParam *nvInfoKParam = (NvInfoKParam *)pAttr;
                infoFunction.params.push_back(*nvInfoKParam);
                // cout << nvInfoKParam->index << ", "
                //      << nvInfoKParam->ordinal << ", "
                //      << nvInfoKParam->offset << ", "
                //      << nvInfoKParam->log_alignment() << ", "
                //      << nvInfoKParam->space() << ", "
                //      << nvInfoKParam->cbank() << ", "
                //      << nvInfoKParam->is_cbank() << ", "
                //      << nvInfoKParam->size_bytes() << endl;
            }
            pAttr = (NvInfoAttribute *)((byte *)pAttr + size);
        }
        CudaRtFrontend::addDeviceFunc2InfoFunc(funcName, infoFunction);
    }
}

void writeCudaFatBinaryToFile(const void *data, const unsigned long long int fatBinSize,
                              const std::string &filename) {
    FILE *file = fopen(filename.c_str(), "rb");
    if (file) {
        // File already exists, skip writing
        fclose(file);
        return;
    }
    file = fopen(filename.c_str(), "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return;
    }
    size_t written = fwrite(data, 1, fatBinSize, file);
    if (written != fatBinSize) {
        perror("Failed to write fat binary to file");
    }
    fclose(file);
}

void maybeDumpCudaFatBinary(const void *data, const unsigned long long int fatBinSize) {
    const char *dumpDir = getenv("GVIRTUS_DUMP_FATBIN_DIR");
    if (!dumpDir || dumpDir[0] == '\0') return;

    struct stat st;
    if (stat(dumpDir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        cerr << "*** Warning: GVIRTUS_DUMP_FATBIN_DIR is not a directory: " << dumpDir << endl;
        return;
    }

    static unsigned long dumpIndex = 0;
    std::ostringstream path;
    path << dumpDir << "/fatbin_" << dumpIndex++ << ".bin";
    writeCudaFatBinaryToFile(data, fatBinSize, path.str());
}

bool decompressFatBinaryPayload(const char *compressed_data, int compressed_size,
                                unsigned long long uncompressed_size, std::vector<char> &output,
                                std::ostream &log) {
    static const unsigned char zstdMagic[4] = {0x28, 0xB5, 0x2F, 0xFD};
    output.resize(uncompressed_size);

    if (compressed_size >= 4 &&
        memcmp(compressed_data, zstdMagic, sizeof(zstdMagic)) == 0) {
        typedef size_t (*ZSTD_decompress_fn)(void *, size_t, const void *, size_t);
        typedef unsigned int (*ZSTD_isError_fn)(size_t);
        typedef const char *(*ZSTD_getErrorName_fn)(size_t);

        static void *zstdHandle = dlopen("libzstd.so.1", RTLD_LAZY | RTLD_LOCAL);
        static ZSTD_decompress_fn zstdDecompress =
            zstdHandle ? (ZSTD_decompress_fn)dlsym(zstdHandle, "ZSTD_decompress") : nullptr;
        static ZSTD_isError_fn zstdIsError =
            zstdHandle ? (ZSTD_isError_fn)dlsym(zstdHandle, "ZSTD_isError") : nullptr;
        static ZSTD_getErrorName_fn zstdGetErrorName =
            zstdHandle ? (ZSTD_getErrorName_fn)dlsym(zstdHandle, "ZSTD_getErrorName") : nullptr;

        if (!zstdDecompress || !zstdIsError || !zstdGetErrorName) {
            log << "*** Warning: ZSTD payload detected but libzstd symbols are unavailable" << endl;
            return false;
        }

        size_t decompressed_size =
            zstdDecompress(output.data(), output.size(), compressed_data, compressed_size);
        if (zstdIsError(decompressed_size)) {
            log << "*** Warning: ZSTD decompression failed: "
                << zstdGetErrorName(decompressed_size) << endl;
            return false;
        }
        if (decompressed_size != output.size()) output.resize(decompressed_size);
        return true;
    }

    int decompressed_size =
        LZ4_decompress_safe(compressed_data, output.data(), compressed_size, output.size());
    if (decompressed_size < 0) {
        log << "*** Warning: LZ4 decompression failed with code " << decompressed_size << endl;
        return false;
    }
    if ((unsigned long long)decompressed_size != uncompressed_size) output.resize(decompressed_size);
    return true;
}

/*
 Routines not found in the cuda's header files.
 KEEP THEM WITH CARE
 */

extern "C" __host__ void **__cudaRegisterFatBinary(void *fatCubin) {
    /* Fake host pointer */
    __fatBinC_Wrapper_t *bin = (__fatBinC_Wrapper_t *)fatCubin;
    if (bin->magic != FATBINWRAPPER_MAGIC) {
        cerr << "*** Error: Invalid fat binary magic number" << endl;
        return nullptr;  // Not a valid fat binary
    }
    // cout << "Fat binary wrapper magic: " << hex << bin->magic << endl;
    struct fatBinaryHeader *fatBinHdr = (struct fatBinaryHeader *)bin->data;
    if (fatBinHdr->magic != FATBIN_MAGIC || fatBinHdr->version != 1) {
        cerr << "*** Error: Invalid fat binary" << endl;
        return nullptr;  // Not a valid fat binary
    }
    // cout << "Fat binary header size: " << fatBinHdr->headerSize << endl;
    // cout << "Fat binary size: " << fatBinHdr->fatSize << endl;

    maybeDumpCudaFatBinary(fatBinHdr, fatBinHdr->headerSize + fatBinHdr->fatSize);

    uint8_t *data_ptr = (uint8_t *)bin->data + fatBinHdr->headerSize;
    size_t remaining_size = fatBinHdr->fatSize;

    std::vector<char> cubin;
    while (remaining_size > 0) {
        fatBinData_t *fatBinData = (fatBinData_t *)data_ptr;
        if (fatBinData->version != 0x0101 || (fatBinData->kind != 1 && fatBinData->kind != 2)) {
            cerr << "*** Warning: Unsupported fat binary data version or kind; skipping metadata parse"
                 << endl;
            break;
        }

        // cout << "Processing fat binary data of kind: " << fatBinData->kind
        //     << " and smVersion: " << fatBinData->smVersion << endl;

        data_ptr += fatBinData->headerSize;

        if (fatBinData->uncompressedPayload != 0) {
            const char *compressed_data = (char *)data_ptr;
            int compressed_size = fatBinData->payloadSize;
            data_ptr += fatBinData->paddedPayloadSize;

            if (!decompressFatBinaryPayload(compressed_data, compressed_size,
                                            fatBinData->uncompressedPayload, cubin, cerr)) {
                cerr << "*** Warning: skipping metadata parse" << endl;
                break;
            }
        } else {
            cubin.resize(fatBinData->paddedPayloadSize);
            memcpy(cubin.data(), data_ptr, fatBinData->paddedPayloadSize);
            data_ptr += fatBinData->paddedPayloadSize;
        }

        if (fatBinData->kind == 2) {
            if (memcmp(cubin.data(), ELF_MAGIC, ELF_MAGIC_SIZE) != 0) {
                cerr << "*** Warning: Invalid ELF magic number in fat binary; skipping metadata parse"
                     << endl;
                break;
            }
            Elf64_Ehdr *eh = (Elf64_Ehdr *)(cubin.data());

            Elf64_Shdr *sh_table = copySectionHeaders(eh);
            if (!sh_table) break;

            char *sh_str = copySectionHeaderStrTable(eh, sh_table);
            if (!sh_str) {
                free(sh_table);
                break;
            }

            parseNvInfoKParams(eh, sh_table, sh_str);

            free(sh_str);
            free(sh_table);
        }
        remaining_size -= (fatBinData->headerSize + fatBinData->paddedPayloadSize);
    }

    Buffer *input_buffer = new Buffer();
    input_buffer->AddString(CudaUtil::MarshalHostPointer((void **)bin));
    input_buffer = CudaUtil::MarshalFatCudaBinary(bin, input_buffer);

    CudaRtFrontend::Prepare();
    CudaRtFrontend::Execute("cudaRegisterFatBinary", input_buffer);
    if (CudaRtFrontend::Success()) return (void **)fatCubin;

    return nullptr;
}

extern "C" __host__ void **__cudaRegisterFatBinaryEnd(void *fatCubin) {
    /* Fake host pointer */
    __fatBinC_Wrapper_t *bin = (__fatBinC_Wrapper_t *)fatCubin;
    char *data = (char *)bin->data;

    Buffer *input_buffer = new Buffer();
    input_buffer->AddString(CudaUtil::MarshalHostPointer((void **)bin));
    input_buffer = CudaUtil::MarshalFatCudaBinary(bin, input_buffer);

    CudaRtFrontend::Prepare();
    CudaRtFrontend::Execute("cudaRegisterFatBinaryEnd", input_buffer);
    if (CudaRtFrontend::Success()) return (void **)fatCubin;
    return NULL;
}

extern "C" __host__ void __cudaUnregisterFatBinary(void **fatCubinHandle) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddStringForArguments(CudaUtil::MarshalHostPointer(fatCubinHandle));
    CudaRtFrontend::Execute("cudaUnregisterFatBinary");
}

extern "C" __host__ void __cudaRegisterFunction(void **fatCubinHandle, const char *hostFun,
                                                char *deviceFun, const char *deviceName,
                                                int thread_limit, uint3 *tid, uint3 *bid,
                                                dim3 *bDim, dim3 *gDim, int *wSize) {
    char *originalDeviceFun = deviceFun;
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddStringForArguments(CudaUtil::MarshalHostPointer(fatCubinHandle));

    CudaRtFrontend::AddVariableForArguments((gvirtus::common::pointer_t)hostFun);
    CudaRtFrontend::AddStringForArguments(deviceFun);
    CudaRtFrontend::AddStringForArguments(deviceName);
    CudaRtFrontend::AddVariableForArguments(thread_limit);
    CudaRtFrontend::AddHostPointerForArguments(tid);
    CudaRtFrontend::AddHostPointerForArguments(bid);
    CudaRtFrontend::AddHostPointerForArguments(bDim);
    CudaRtFrontend::AddHostPointerForArguments(gDim);
    CudaRtFrontend::AddHostPointerForArguments(wSize);

    CudaRtFrontend::Execute("cudaRegisterFunction");

    if (CudaRtFrontend::Success()) {
        try {
            deviceFun = CudaRtFrontend::GetOutputString();
            tid = CudaRtFrontend::GetOutputHostPointer<uint3>();
            bid = CudaRtFrontend::GetOutputHostPointer<uint3>();
            bDim = CudaRtFrontend::GetOutputHostPointer<dim3>();
            gDim = CudaRtFrontend::GetOutputHostPointer<dim3>();
            wSize = CudaRtFrontend::GetOutputHostPointer<int>();
        } catch (const std::exception &e) {
            cerr << "*** Warning: cudaRegisterFunction output parse failed: " << e.what()
                 << "; falling back to original device function mapping" << endl;
            deviceFun = originalDeviceFun;
        }
    } else {
        deviceFun = originalDeviceFun;
    }

    CudaRtFrontend::addHost2DeviceFunc((void *)hostFun, deviceFun);
}

extern "C" __host__ void __cudaRegisterVar(void **fatCubinHandle, char *hostVar,
                                           char *deviceAddress, const char *deviceName, int ext,
                                           int size, int constant, int global) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddStringForArguments(CudaUtil::MarshalHostPointer(fatCubinHandle));
    CudaRtFrontend::AddStringForArguments(hostVar);
    CudaRtFrontend::AddStringForArguments(deviceAddress);
    CudaRtFrontend::AddStringForArguments(deviceName);
    CudaRtFrontend::AddVariableForArguments(ext);
    CudaRtFrontend::AddVariableForArguments(size);
    CudaRtFrontend::AddVariableForArguments(constant);
    CudaRtFrontend::AddVariableForArguments(global);
    // cout << "RegisterVar: fatCubinHandle: " << fatCubinHandle
    //      << ", hostVar: " << hostVar
    //      << ", deviceAddress: " << deviceAddress
    //      << ", deviceName: " << deviceName
    //      << ", ext: " << ext
    //      << ", size: " << size
    //      << ", constant: " << constant
    //      << ", global: " << global << endl;
    CudaRtFrontend::Execute("cudaRegisterVar");
}

extern "C" __host__ void __cudaRegisterShared(void **fatCubinHandle, void **devicePtr) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddStringForArguments(CudaUtil::MarshalHostPointer(fatCubinHandle));
    CudaRtFrontend::AddStringForArguments((char *)devicePtr);
    CudaRtFrontend::Execute("cudaRegisterShared");
}

extern "C" __host__ void __cudaRegisterSharedVar(void **fatCubinHandle, void **devicePtr,
                                                 size_t size, size_t alignment, int storage) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddStringForArguments(CudaUtil::MarshalHostPointer(fatCubinHandle));
    CudaRtFrontend::AddStringForArguments((char *)devicePtr);
    CudaRtFrontend::AddVariableForArguments(size);
    CudaRtFrontend::AddVariableForArguments(alignment);
    CudaRtFrontend::AddVariableForArguments(storage);
    CudaRtFrontend::Execute("cudaRegisterSharedVar");
}

extern "C" __host__ int __cudaSynchronizeThreads(void **x, void *y) {
    // FIXME: implement
    std::cerr << "*** Error: __cudaSynchronizeThreads() not yet implemented!" << std::endl;
    return 0;
}

extern "C" __host__ __device__ unsigned CUDARTAPI __cudaPushCallConfiguration(dim3 gridDim,
                                                                              dim3 blockDim,
                                                                              size_t sharedMem,
                                                                              cudaStream_t stream) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddVariableForArguments(gridDim);
    CudaRtFrontend::AddVariableForArguments(blockDim);
    CudaRtFrontend::AddVariableForArguments(sharedMem);
    CudaRtFrontend::AddDevicePointerForArguments(stream);

    CudaRtFrontend::Execute("cudaPushCallConfiguration");

    return CudaRtFrontend::GetExitCode();
}

extern "C" cudaError_t CUDARTAPI __cudaPopCallConfiguration(dim3 *gridDim, dim3 *blockDim,
                                                            size_t *sharedMem,
                                                            cudaStream_t *stream) {
    CudaRtFrontend::Prepare();

    CudaRtFrontend::Execute("cudaPopCallConfiguration");

    *gridDim = CudaRtFrontend::GetOutputVariable<dim3>();
    *blockDim = CudaRtFrontend::GetOutputVariable<dim3>();
    *sharedMem = CudaRtFrontend::GetOutputVariable<size_t>();
    cudaStream_t stream1 = CudaRtFrontend::GetOutputVariable<cudaStream_t>();

    memcpy(stream, &stream1, sizeof(cudaStream_t));
    return CudaRtFrontend::GetExitCode();
}
