#ifndef APP_HH
#define APP_HH

#include <webgpu/webgpu.h>

#include "webgpu_utils.hh"
#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif  // WEBGPU_BACKEND_WGPU

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif  // __EMSCRIPTEN__

#include <iostream>

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
  GLFWwindow* window = nullptr;
  WGPUDevice device = nullptr;
  WGPUQueue queue = nullptr;
  WGPUSurface surface = nullptr;
  WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;
  WGPURenderPipeline pipeline = nullptr;

  WGPUTextureView GetNextSurfaceTextureView();
  void InitializePipeline();
};

#endif
