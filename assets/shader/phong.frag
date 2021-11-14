#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 TextureCoordinate;
in vec3 rawPosition;
in vec3 FragPos;
in vec3 lightPos;
in vec3 Normal;

layout (std140) uniform model {
  // Model matrix
  mat4 modelMatrix;
  // mat4(inverse(transpose(mat3(modelMatrix)))), precalculate using CPU for efficiency
  mat4 normalMatrix;
};

layout (std140) uniform camera {
  // Camera's projection * view matrix
  mat4 viewProjectionMatrix;
  // Position of the camera
  vec4 viewPosition;
};

layout (std140) uniform light {
  // Light's projection * view matrix
  // Hint: If you want to implement shadow, you may use this.
  mat4 lightSpaceMatrix;
  // Position or direction of the light
  vec4 lightVector;
  // inner cutoff, outer cutoff, isSpotlight, isDirectionalLight
  vec4 coefficients;
};
uniform int isBlinn;

uniform sampler2D diffuseTexture;
uniform samplerCube diffuseCubeTexture;
// precomputed shadow
// Hint: You may want to uncomment this to use shader map texture.
// uniform sampler2DShadow shadowMap;
uniform int isCube;
void main() {
  vec4 diffuseTextureColor = texture(diffuseTexture, TextureCoordinate);
  vec4 diffuseCubeTextureColor = texture(diffuseCubeTexture, rawPosition);
  vec3 color = isCube == 1 ? diffuseCubeTextureColor.rgb : diffuseTextureColor.rgb;
  // TODO: vertex shader / fragment shader
  // Hint:
  //       1. how to write a vertex shader:
  //          a. The output is gl_Position and anything you want to pass to the fragment shader. (Apply matrix multiplication yourself)
  //       2. how to write a fragment shader:
  //          a. The output is FragColor (any var is OK)
  //       3. colors
  //          a. For point light & directional light, lighting = ambient + attenuation * shadow * (diffuse + specular)
  //          b. If you want to implement multiple light sources, you may want to use lighting = shadow * attenuation * (ambient + (diffuse + specular))
  //       4. attenuation
  //          a. spotlight & pointlight: see spec
  //          b. directional light = no
  //          c. Use formula from slides 'shading.ppt' page 20
  //       5. spotlight cutoff: inner and outer from coefficients.x and coefficients.y
  //       6. diffuse = kd * max(normal vector dot light direction, 0.0)
  //       7. specular = ks * pow(max(normal vector dot halfway direction), 0.0), 8.0);
  //       8. notice the difference of light direction & distance between directional light & point light
  //       9. we've set ambient & color for you
  float ambient = 0.1;
  float ks = 0.75;
  float kd = 0.75;
  float shininess = 8.0;
  vec3 norm = normalize(Normal);
  //vec3 lightDir = normalize(lightPos - FragPos);
  
  float distance = length(lightVector.xyz - FragPos);
  float attenuation = 0;
  vec3 lightDir;
  // Spotlight
  if (coefficients.z == 1.0) {
      float constant = 1.0;
      float linear = 0.014;
      float quadratic = 0.007;
      lightDir = normalize(viewPosition.xyz - FragPos);
      float theta = dot(lightDir, normalize(-lightVector.xyz));
      float epsilon = coefficients.x - coefficients.y;
      float intensity = clamp((theta - coefficients.y) / epsilon, 0.0, 1.0);
      attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
      attenuation = attenuation * intensity;
  }
  // Directional Light
  else if (coefficients.w == 1.0){
    lightDir = normalize(lightVector.xyz);
    attenuation = 0.65;
  }
  // Point light
  else{
    float constant = 1.0;
    float linear = 0.027;
    float quadratic = 0.0028;
  	lightDir = normalize(lightVector.xyz - FragPos);
    attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
  }

  vec3 viewDir = normalize(viewPosition.xyz - FragPos);
  
  float diffuse = kd * max(dot(lightDir, norm), 0.0);
  float specular;
  if(isBlinn == 1)
  {
      vec3 halfwayDir = normalize(lightDir + viewDir);  
      specular = ks * pow(max(dot(norm, halfwayDir), 0.0), shininess);
  }
  else
  {
      vec3 reflectDir = reflect(-lightDir, norm);
      specular = ks * pow(max(dot(viewDir, reflectDir), 0.0), shininess);
  }
  float shadow = 1.0;
  float lighting = ambient + attenuation * shadow * (diffuse + specular);
  color = lighting * color;
  FragColor = vec4(color, 1.0);
}
