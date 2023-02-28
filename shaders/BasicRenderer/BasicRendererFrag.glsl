#version 440

layout (binding = 1) uniform MyLight {
    vec3 lightColor;
    vec3 lightPos;
} light;

layout (location = 0) in vec3 fragPos;
layout (location = 1) in vec3 normal;
layout (location = 0) out vec4 outColor;

void main() {
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * light.lightColor;

    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.lightPos;

    vec3 result = (diffuse + ambient) * vec3(1.0, 1.0, 1.0);

    outColor = vec4(result, 1.0);
}