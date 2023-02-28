#version 440

layout (binding = 0) uniform MyUBO {
    mat4 model;
    mat4 viewPerspective;
} matrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;

void main() 
{
    // gl_Position = vec4(position, 1.0);
    // gl_Position = ubo.mvpMat * vec4(position, 1.0);
    // v_color = color;
    // v_color = vec3(1.0, 1.0, 1.0);

    gl_Position = matrix.viewPerspective * matrix.model * vec4(position, 1.0);
    v_worldPos = vec3(matrix.model * vec4(position, 1.0));
    v_normal = vec3(matrix.model * vec4(normal, 0.0));
}
