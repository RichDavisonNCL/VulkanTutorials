# MSc Computer Graphics Programming - Vulkan

## Building the project

You'll need CMake, or an IDE that has integrated CMake support. Visual Studio 2022 is recommended. 

From Command line:
```
mk dir build
cd build
cmake ..
```

## Choosing an example to run
The codebase has a number of Vulkan-based tutorials that explore both Vulkan API usage, and common rendering techniques. You can choose which tutorial to run using command line arguments. In Visual Studio you can find this by right-clicking the __VulkanTutorials project->Properties->Debugging->Command Arguments__ and adding the __-Example__ argument with one of the following:


| Argument              | Description |
| :---------------- | :------ |
| MyFirstTriangle        |   Demonstrates use of pipelines, and core classes for meshes and shaders.    |
| DescriptorExample           |   How to create use use descriptors and descriptor sets to provide shaders with data.   |
| PushConstantExample    |  How to push small amounts of user-data directly into the command buffer for later use in shaders.    |
| UniformBufferExample |  How to create buffers in GPU memory for shaders to later , and upload data to the GPU in your program per-frame.   |
| PushDescriptorExample |  How to add descriptors directly to the command buffer instead of managing sets separately.   |
| TexturingExample |  How to load and utilise a texture.   |
| InlineUniformBufferExample |  How to add data directly to the command buffer instead of managing a uniform buffer separately. Useful for transient data larger than a push-constant. |
| MultiViewportExample |  How to manipulate the viewport to render into different areas of the screen.   |
| TextureUploadExample |  How to create a texture, and then upload an array of data to it.   |
| TessellationExample |  How to use the tessellation control and tessellation evaluation shaders.    |
| GeometryShaderExample |  How to use the geometry shader stage.    |
| SkinningExample |  How moving characters can be represented, and what extra data is required to be sent to the GPU to move these chracters per-frame.   |
| ComputeExample |  How to use the compute shader stage to calculate data for use in rendering.   |
| CubeMapExample |  Hows to load cube maps, and how to sample them in a shader.   |
| LightingExample |  A demonstration of how lighting information can be represented in data, and the common calculations use to create the appearance of lighting in a rasterised scene.   |
| PostProcessingExample |  A demonstration of teh steps required to render the scene into an off-screen image, and then render it to the screen using a custom fragment shader.   |
| ShadowmappingExample |  A demonstration of how to create the appearance of shadows in the scene, by rendering the scene from the light's point of view into an off-screen buffer.   |
| GLTFExample |  Render a whole scene of objects, loading from a scene in GLTF format.   |
| DeferredExample |  Many games will now defer lighting caluclations until after the scene is drawn, for efficiency. This example examines deferred lighting in a simple scene with point lights.   |
| ComputeSkinningExample |  A common optimisation in games is to calculate character skinning data in a compute shader, instead of calculating it repeatedly in the vertex shader.   |
| BufferDeviceAddressExample |  Modern GPUs support a memory access model more similar to that of a CPU. This example looks at how to request the address of a buffer, and how to send it to a shader for read access.   |
| BindlessExample |  Modern GPUs can read from many GBs of data, spread across many buffers and images. This example looks at how to make use of this using large, unbounded descriptor sets.   |
| AsyncComputeExample |  Modern GPUs can often execute rasterisation and compute shaders simultaneously. This example looks how how to generate vertex information in a compute shader, and how to correctly wait for this data to be ready for a vertex shader invocation.   |
| DescriptorBufferExample |  Some GPUs can store descriptor handles in a buffer, instead of a number of opaque descriptor 'sets'. In this example a basic usage of this is shown, demonstrating how to create descriptor buffers, and how to fill them with descriptor handles.   |
| TestGLTFRayTrace |  Ray tracing is becoming increasingly popular, and presents a vastly different model to rendering than a traditional rasterisation-based shader pipeline. This example shows how to load a scene from a GLTF file, and how to create the ray tracing pipelines and data required to correctly render it.   |


For example, adding __-Example TestGLTFRayTrace__ to the command line will start the program with a ray tracing scene.