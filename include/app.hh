#ifndef APP_HH
#define APP_HH

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>

class Application {
public:
    // Initialize everything and return true if it went all right
    bool Initialize();

    // Uninitialize everything that was initialized
    void Terminate();

    // Draw a frame and handle events
    void MainLoop();

    // Return true as long as the main loop should keep on running
    bool IsRunning();

private:
    // We put here all the variables that are shared between init and main loop
    GLFWwindow *window = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    WGPUSurface surface = nullptr;

    void inspectDevice(WGPUDevice device);
    WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const *options);
    WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const *descriptor);
    WGPUTextureView GetNextSurfaceTextureView();

};

#endif
