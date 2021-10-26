#version 330 core
layout(location = 0) in vec3 Position_in;

layout (std140) uniform model {
  // Model matrix
  mat4 modelMatrix;
  // inverse(transpose(model)), precalculate using CPU for efficiency
  mat4 normalMatrix;
};

layout (std140) uniform light {
  // Projection * View matrix
  mat4 lightSpaceMatrix;
  // Position or direction of the light
  vec4 lightVector;
  // inner cutoff, outer cutoff, isSpotlight, isDirectionalLight
  vec4 coefficients;
};

void main() {
  gl_Position = lightSpaceMatrix * modelMatrix * vec4(Position_in, 1.0f);
}
