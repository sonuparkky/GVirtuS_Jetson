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
 * Written By: Theodoros Aslanidis <theodoros.aslanidis@ucdconnect.ie>,
 *             Department of Computer Science, University College Dublin
 *
 *             Ting-Hui Cheng <tinghc@es.aau.dk>
 *             Department of Electronic Systems, Aalborg University
 */

#include "CudaRt.h"

using namespace std;

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphGetNodes(cudaGraph_t graph,
                                                            cudaGraphNode_t* nodes,
                                                            size_t* numNodes) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graph);
    CudaRtFrontend::AddHostPointerForArguments(nodes);
    CudaRtFrontend::Execute("cudaGraphGetNodes");

    if (CudaRtFrontend::Success()) {
        *numNodes = CudaRtFrontend::GetOutputVariable<size_t>();
        // cout << "Get a node for graph." << endl;
    }
    return CudaRtFrontend::GetExitCode();
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphExecDestroy(cudaGraphExec_t graphExec) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graphExec);
    CudaRtFrontend::Execute("cudaGraphExecDestroy");
    // cout << "Destroy graph execution." << endl;
    return CudaRtFrontend::GetExitCode();
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphInstantiate(cudaGraphExec_t* pGraphExec,
                                                               cudaGraph_t graph,
                                                               unsigned long long flags) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graph);
    CudaRtFrontend::AddVariableForArguments(flags);
    CudaRtFrontend::Execute("cudaGraphInstantiate");

    if (CudaRtFrontend::Success()) {
        *pGraphExec = CudaRtFrontend::GetOutputVariable<cudaGraphExec_t>();
        // cout << "Creates an executable graph from a graph." << endl;
    }
    return CudaRtFrontend::GetExitCode();
}

// TODO: needs testing
extern "C" __host__ cudaError_t CUDARTAPI cudaGraphInstantiateWithFlags(cudaGraphExec_t* pGraphExec,
                                                                        cudaGraph_t graph,
                                                                        unsigned long long flags) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graph);
    CudaRtFrontend::AddVariableForArguments(flags);
    CudaRtFrontend::Execute("cudaGraphInstantiateWithFlags");
    // cout << "Graph:" << graph << endl;                                                                           
    if (CudaRtFrontend::Success()) {
        *pGraphExec = CudaRtFrontend::GetOutputVariable<cudaGraphExec_t>();
        // cout << "Creates an executable graph from a graph." << endl;
    }
    return CudaRtFrontend::GetExitCode();
}

// TODO: needs testing
extern "C" __host__ cudaError_t CUDARTAPI cudaGraphDebugDotPrint(cudaGraph_t graph,
                                                                 const char* path,
                                                                 unsigned int flags) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graph);
    CudaRtFrontend::AddStringForArguments(path);
    CudaRtFrontend::AddVariableForArguments<unsigned int>(flags);
    CudaRtFrontend::Execute("cudaGraphDebugDotPrint");

    return CudaRtFrontend::GetExitCode();
}


extern "C" __host__ cudaError_t CUDARTAPI cudaGraphLaunch(cudaGraphExec_t graphExec,
                                                          cudaStream_t stream) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graphExec);
    CudaRtFrontend::AddDevicePointerForArguments(stream);
    CudaRtFrontend::Execute("cudaGraphLaunch");
    // cout << "Graph Launch" << endl;                                                        
    return CudaRtFrontend::GetExitCode();
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphCreate(cudaGraph_t* pGraph, unsigned int flags) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddVariableForArguments(flags);
    // cout << "Graph is Create" << *pGraph << endl;
    CudaRtFrontend::Execute("cudaGraphCreate");
    
    if (CudaRtFrontend::Success()) *pGraph = CudaRtFrontend::GetOutputVariable<cudaGraph_t>();
    
    return CudaRtFrontend::GetExitCode();
}


extern "C" __host__ cudaError_t CUDARTAPI cudaGraphDestroy(cudaGraph_t graph) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graph);
    CudaRtFrontend::Execute("cudaGraphDestroy");
    // cout << "Graph is Destroy" << endl;
    return CudaRtFrontend::GetExitCode();
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphUpload(cudaGraphExec_t graphExec, cudaStream_t stream) {
    CudaRtFrontend::Prepare();
    CudaRtFrontend::AddDevicePointerForArguments(graphExec);
    CudaRtFrontend::AddDevicePointerForArguments(stream);
    CudaRtFrontend::Execute("cudaGraphLaunch");
    
    return CudaRtFrontend::GetExitCode();
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphNodeGetDependencies(cudaGraphNode_t,
                                                                        cudaGraphNode_t*,
                                                                        size_t* pNumDependencies) {
    if (pNumDependencies != nullptr) {
        *pNumDependencies = 0;
    }
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaUserObjectCreate(cudaUserObject_t* object_out,
                                                               void* ptr,
                                                               cudaHostFn_t,
                                                               unsigned int,
                                                               unsigned int) {
    if (object_out != nullptr) {
        *object_out = reinterpret_cast<cudaUserObject_t>(ptr);
    }
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphRetainUserObject(cudaGraph_t,
                                                                    cudaUserObject_t,
                                                                    unsigned int,
                                                                    unsigned int) {
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphAddDependencies(cudaGraph_t,
                                                                   const cudaGraphNode_t*,
                                                                   const cudaGraphNode_t*,
                                                                   size_t) {
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphAddDependencies_v2(cudaGraph_t,
                                                                      const cudaGraphNode_t*,
                                                                      const cudaGraphNode_t*,
                                                                      const cudaGraphEdgeData*,
                                                                      size_t) {
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaGraphAddEventRecordNode(cudaGraphNode_t* pGraphNode,
                                                                      cudaGraph_t,
                                                                      const cudaGraphNode_t*,
                                                                      size_t,
                                                                      cudaEvent_t) {
    if (pGraphNode != nullptr) {
        *pGraphNode = nullptr;
    }
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaStreamGetCaptureInfo_v3(cudaStream_t,
                                                                      enum cudaStreamCaptureStatus* captureStatus_out,
                                                                      unsigned long long* id_out,
                                                                      cudaGraph_t* graph_out,
                                                                      const cudaGraphNode_t** dependencies_out,
                                                                      const cudaGraphEdgeData** edgeData_out,
                                                                      size_t* numDependencies_out) {
    if (captureStatus_out != nullptr) {
        *captureStatus_out = cudaStreamCaptureStatusNone;
    }
    if (id_out != nullptr) {
        *id_out = 0;
    }
    if (graph_out != nullptr) {
        *graph_out = nullptr;
    }
    if (dependencies_out != nullptr) {
        *dependencies_out = nullptr;
    }
    if (edgeData_out != nullptr) {
        *edgeData_out = nullptr;
    }
    if (numDependencies_out != nullptr) {
        *numDependencies_out = 0;
    }
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaStreamUpdateCaptureDependencies(cudaStream_t,
                                                                              cudaGraphNode_t*,
                                                                              size_t,
                                                                              unsigned int) {
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaStreamUpdateCaptureDependencies_v2(cudaStream_t,
                                                                                 cudaGraphNode_t*,
                                                                                 const cudaGraphEdgeData*,
                                                                                 size_t,
                                                                                 unsigned int) {
    return cudaSuccess;
}

extern "C" __host__ cudaError_t CUDARTAPI cudaMallocFromPoolAsync(void** ptr,
                                                                  size_t size,
                                                                  cudaMemPool_t,
                                                                  cudaStream_t stream) {
    return cudaMallocAsync(ptr, size, stream);
}

extern "C" __host__ cudaError_t cudaGetFuncBySymbol(cudaFunction_t* functionPtr, const void*) {
    if (functionPtr != nullptr) {
        *functionPtr = nullptr;
    }
    return cudaErrorNotSupported;
}
