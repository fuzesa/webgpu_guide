#include <webgpu/webgpu.h>
#include <cassert>
#include <iostream>
#include <vector>
#include "app.hh"
#include <glfw3webgpu.h>

#ifdef WEBGPU_BACKEND_WGPU
#  include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapterSync(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter Application::requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const *options) {
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const *message,
                                    void *pUserData) {
        UserData &userData = *static_cast<UserData *>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void *) &userData
    );


    // When using wgpu or Dawn, this isn't necessary, only with emscripten
    // We wait until userData.requestEnded gets true
#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice Application::requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const *message,
                                   void *pUserData) {
        UserData &userData = *static_cast<UserData *>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void *) &userData
    );

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
#endif // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.device;
}

// We also add an inspect device function:
void Application::inspectDevice(WGPUDevice device) {
    std::vector<WGPUFeatureName> features;
    size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
    features.resize(featureCount);
    wgpuDeviceEnumerateFeatures(device, features.data());

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (auto f: features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    WGPUSupportedLimits limits = {};
    limits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;
#else
    bool success = wgpuDeviceGetLimits(device, &limits);
#endif

    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
    }
}


bool Application::Initialize() {
    // Create WebGPU instance
    // We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    // Make sure the uncaptured error callback is called as soon as an error
    // occurs rather than at the next call to "wgpuDeviceTick".
    WGPUDawnTogglesDescriptor toggles;
    toggles.chain.next = nullptr;
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.disabledToggleCount = 0;
    toggles.enabledToggleCount = 1;
    const char *toggleName = "enable_immediate_error_handling";
    toggles.enabledToggles = &toggleName;

    desc.nextInChain = &toggles.chain;
#endif // WEBGPU_BACKEND_DAWN

    // We create the instance using this descriptor
#ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(nullptr);
#else //  WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(&desc);
#endif //  WEBGPU_BACKEND_EMSCRIPTEN

    // We can check whether there is actually an instance created
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    // Display the object (WGPUInstance is a simple pointer, it may be
    // copied around without worrying about its size).
    std::cout << "WGPU instance: " << instance << std::endl;

    std::cout << "Requesting adapter..." << std::endl;

    surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    WGPUAdapter adapter = requestAdapterSync(instance, &adapterOpts);

    std::cout << "Got adapter: " << adapter << std::endl;

    // INSPECTING THE ADAPTER
#ifndef __EMSCRIPTEN__
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
#else
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);
#endif

    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    }
#endif // NOT __EMSCRIPTEN__

    // ADAPTER FEATURES
    std::vector<WGPUFeatureName> features;

    // Call the function a first time with a null return address, just to get
    // the entry count.
    size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

    // Allocate memory (could be a new, or a malloc() if this were a C program)
    features.resize(featureCount);

    // Call the function a second time, with a non-null return address
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
    for (auto f: features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec; // Restore decimal numbers

    // ADAPTER PROPERTIES
    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendorName) {
        std::cout << " - vendorName: " << properties.vendorName << std::endl;
    }
    if (properties.architecture) {
        std::cout << " - architecture: " << properties.architecture << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.name) {
        std::cout << " - name: " << properties.name << std::endl;
    }
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers

    // DEVICE REQUEST
    std::cout << "Requesting device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};

    WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);

    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    // A function that is invoked whenever the device stops being available.
    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const *message, void * /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };

    auto onDeviceError = [](WGPUErrorType type, char const *message, void * /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

    std::cout << "Got device: " << device << std::endl;

    // COMMAND QUEUE
    WGPUQueue queue = wgpuDeviceGetQueue(device);

    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void * /* pUserData */) {
        std::cout << "Queued work finished with status: " << status << std::endl;
    };
    wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    wgpuCommandEncoderInsertDebugMarker(encoder, "Do one thing");
    wgpuCommandEncoderInsertDebugMarker(encoder, "Do another thing");

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(encoder); // release encoder after it's finished

    // Finally submit the command queue
    std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(queue, 1, &command);
    wgpuCommandBufferRelease(command);
    std::cout << "Command submitted." << std::endl;

    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;

    // Configuration of the textures created for the underlying swap chain
    config.width = 640;
    config.height = 480;
    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
    config.format = surfaceFormat;
    // And we do not need any particular view format:
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;

    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(surface, &config);

    // We clean up the WebGPU instance
    wgpuInstanceRelease(instance);

    // Release adapter
    wgpuAdapterRelease(adapter);

    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return 1;
    }

    // Create the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return 1;
    }

    return true;
}

void Application::Terminate() {
    // Release device
    wgpuDeviceRelease(device);

    // Release command queue
    wgpuQueueRelease(queue);

    wgpuSurfaceUnconfigure(surface);

    // Release surface
    wgpuSurfaceRelease(surface);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void Application::MainLoop() {
    glfwPollEvents();

    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;

    WGPURenderPassColorAttachment renderPassColorAttachment = {};

    WGPUTextureView targetView = GetNextSurfaceTextureView();

    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

#ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    // [...] Main loop content

    // Also move here the tick/poll but NOT the emscripten sleep
    // [...] Poll WebGPU Events

    for (int i = 0; i < 5; ++i) {
        std::cout << "Tick/Poll device..." << std::endl;
#if defined(WEBGPU_BACKEND_DAWN)
        wgpuDeviceTick(device);
#elif defined(WEBGPU_BACKEND_WGPU)
        wgpuDevicePoll(device, false, nullptr);
#endif
    }
}

bool Application::IsRunning() {
    return !glfwWindowShouldClose(window);
}

WGPUTextureView Application::GetNextSurfaceTextureView() {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return nullptr;
    }

    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

    // At the end of the frame
    wgpuTextureViewRelease(targetView);
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(surface);
#endif

    return targetView;
}