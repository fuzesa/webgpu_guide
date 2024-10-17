# WebGPU Guide

Based on the walkthrough by Elie Michel from the online guide at [https://eliemichel.github.io/LearnWebGPU/](https://eliemichel.github.io/LearnWebGPU/)

## Cloning the repo

When cloning the repo, make sure to **_recursively get the submodules_** as well

```shell
git clone --recurse-submodules https://github.com/fuzesa/webgpu_guide.git
```

## Additional dep versions

These don't need to be installed as a system dependency, instead they're added as Git submodules and included as additional CMake projects

| Dep                                                   | Version | Web Link                                                           |
|-------------------------------------------------------|---------|--------------------------------------------------------------------|
| **GLFW** - **G**raphics **L**ibrary **F**rame**W**ork | 3.4     | https://github.com/glfw/glfw/tree/3.4                              |
| WebGPU distribution                                   | 0.2.0   | https://github.com/eliemichel/WebGPU-distribution/tree/main-v0.2.0 |
| GLFW WebGPU Extension                                 | 1.2.0   | https://github.com/eliemichel/glfw3webgpu/tree/v1.2.0              |
