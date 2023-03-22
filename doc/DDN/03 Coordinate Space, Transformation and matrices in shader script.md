# Space, Transformation and matrix in Hedge

This document contains information relating to space and coordinates used in the Hedge.

## Space

### World Space

Hedge uses a left-hand coordinate system. Y is up, x is right and z pointing toward the paper.

### Clip space and Normalized Device Coordinate

In Vulkan or glsl, clip space is the space for coordinates in vertex shader that is not applied homogeneous division. In addition, according to [4], `gl_Position.w` cannot be negative since it would be used to determine whether a coordinate is in a clipping space. If you assign a negative value to all points' w coordinate, then all points would be culled before it sends for homogeneous division.

As for the NDC, Vulkan is also different from OpenGL. OpenGL has (-1, -1, -1) as its NDC bottom left point and (1, 1, 1) as its top right point. But in Vulkan, NDC has (-1, 1, 0) as its bottom left point and (1, -1, 1) as its top right point. ((0, 0) point is the center of the screen. x axis goes rightward and the y axis goes downward) [1] has a clear figure comparing them.

## Transformation and relevant matrices

### From the world space to the view space

The view matrix is written down in the [5] P67, which is usable.

### From the view space to the clip space

As for the perspective matrix, we use the matrix drived from [1]. Note that it also gives the inverse of the matrix, which will be super useful. It is derived by mapping near plane view's (left, right), (up, down) to NDC's (-1, 1) and view's (near, far) to NDC's (1, 0). Besides, due to the fact that the vertex that we used to derive are not literaily in the view space. We need to find the formula that can map a vert in view space to view's near plane space for left-right and up-down in order to truly generate the perspective matrix.

The perspective matrix in [5] is unusable since OpenGL has a quite different NDC layout and the book doesn't give the process of deriving the matrix.

Another thing that needs to be noted is that the perspective matrix used in the [1] uses the reverse z technique [2][3]. As a result, the near plane depth would be mapped to 1 and the far plane depth would be mapped to 0.  

## Passing Matrix to GLSL

In glsl, matrices are column major. It means we need to transpose the matrix before we send them to the GPU memory (Assuming we are using row major matrix for calculation on the host).

In addition, the RenderDoc is a great tool to check what you passed to the shader. You can find relevant information by checking `drawCmd->Pipeline State->VS->Go button at the right of the buffer`. The RenderDoc can also help you check the input geometry and output geometry, which is really helpful.

## MISC

In order to have depth clipping, we may need to provide a depth buffer.

## Reference
1. [The perspective projection matrix in Vulkan](https://vincent-p.github.io/posts/vulkan_perspective_matrix/)
2. [Reverse Z](https://ajweeks.com/blog/2019/04/06/ReverseZ/)
3. [Depth Precision Visualized](https://developer.nvidia.com/content/depth-precision-visualized)
4. [Negative values for gl_Position.w?](https://stackoverflow.com/questions/47233771/negative-values-for-gl-position-w)
5. Real-time Rendering 4th edition