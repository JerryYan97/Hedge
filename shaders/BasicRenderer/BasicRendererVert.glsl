#version 440

layout (binding = 0) uniform MyUBO {
    mat4 mvpMat;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 v_color;

void main() {
    // gl_Position = vec4(position, 1.0);
    gl_Position = ubo.mvpMat * vec4(position, 1.0);
    v_color = color;
    // v_color = vec3(ubo.mvpMat[0][0], 0.0, 0.0);
}
