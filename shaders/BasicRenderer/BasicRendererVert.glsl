#version 440

layout (binding = 0) uniform MyUBO {
    mat4 mvpMat;
} ubo;

layout (binding = 1) uniform MyLight {
    vec3 lightColor;
    vec3 lightPos;
} light;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
// layout(location = 1) in vec3 color;

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec3 v_normal;

void main() 
{
    // gl_Position = vec4(position, 1.0);
    gl_Position = ubo.mvpMat * vec4(position, 1.0);
    // v_color = color;
    // v_color = vec3(1.0, 1.0, 1.0);
    v_color = vec3(light.lightPos);
    v_normal = normal;
}
