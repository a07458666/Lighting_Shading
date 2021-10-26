#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <iostream>

#include <GLFW/glfw3.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION
#include "graphics.h"

// Unnamed namespace for global variables
namespace {
// Cameras
graphics::camera::Camera* currentCamera = nullptr;
// Control variables
bool isWindowSizeChanged = false;
bool isLightChanged = true;
int currentLight = 0;
int currentShader = 1;
int alignSize = 256;
// TODO (optional): Configs
// You should change line 32-35 if you add more shader / light / camera / mesh.
constexpr int LIGHT_COUNT = 3;
constexpr int CAMERA_COUNT = 1;
constexpr int MESH_COUNT = 3;
constexpr int SHADER_PROGRAM_COUNT = 3;
}  // namespace

int uboAlign(int i) { return ((i + 1 * (alignSize - 1)) / alignSize) * alignSize; }

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
  // There are three actions: press, release, hold
  if (action != GLFW_PRESS) return;
  // Press ESC to close the window.
  if (key == GLFW_KEY_ESCAPE) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  switch (key) {
    // TODO: Detect key-events, to:
    //       1. switch among directional light, point light, and spot light, or
    //       2. switch between phong shader and gouraurd shader
    // Hint: use currentLight, isLightChanged, currentShader
    // Note: 1 key for 1 variable change
    default: break;
  }
}

void resizeCallback(GLFWwindow* window, int width, int height) {
  OpenGLContext::framebufferResizeCallback(window, width, height);
  assert(currentCamera != nullptr);
  currentCamera->updateProjection(OpenGLContext::getAspectRatio());
  isWindowSizeChanged = true;
}

int main() {
  // Initialize OpenGL context, details are wrapped in class.
  OpenGLContext::createContext(43, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* window = OpenGLContext::getWindow();
  glfwSetWindowTitle(window, "HW2");
  glfwSetKeyCallback(window, keyCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetFramebufferSizeCallback(window, resizeCallback);
#ifndef NDEBUG
  OpenGLContext::printSystemInfo();
  // This is useful if you want to debug your OpenGL API calls.
  OpenGLContext::enableDebugCallback();
#endif
  // Initialize shader
  std::vector<graphics::shader::ShaderProgram> shaderPrograms(SHADER_PROGRAM_COUNT);
  std::string filenames[SHADER_PROGRAM_COUNT] = {"shadow", "phong", "gouraud"};
  for (int i = 0; i < SHADER_PROGRAM_COUNT; ++i) {
    graphics::shader::VertexShader vs;
    graphics::shader::FragmentShader fs;
    vs.fromFile("../assets/shader/" + filenames[i] + ".vert");
    fs.fromFile("../assets/shader/" + filenames[i] + ".frag");
    shaderPrograms[i].attach(&vs, &fs);
    shaderPrograms[i].link();
    shaderPrograms[i].detach(&vs, &fs);
    shaderPrograms[i].use();
    // TODO: bind the uniform variables
    // Hint:
    //       1. you can set other uniforms you want in this for-loop
    //       2. check ShaderProgram class to know how to bind more easily
    //       3. It's safe to find and bind a non-exist uniform, it will just become NOP
    //       4. For textures, you set a int for it, then bind that texture to the texture unit later
    // Note:
    //       1. glUniform
    //        https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniform.xhtml
    //       2. glGetUniformLocation
    //        https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml
    //       3. glUniformBlockBinding
    //        https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glUniformBlockBinding.xhtml
    //       4. glGetUniformBlockIndex
    //        https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetUniformBlockIndex.xhtml
    //       5. Check uniformBlockBinding and setUniform member function of ShaderProgram class
    //       We only set some variables here, you need more when you're lighting
    shaderPrograms[i].uniformBlockBinding("model", 0);
    shaderPrograms[i].uniformBlockBinding("camera", 1);
    // Maybe light here or other uniform you set :)

    shaderPrograms[i].setUniform("diffuseTexture", 0);
    shaderPrograms[i].setUniform("shadowMap", 1);
    shaderPrograms[i].setUniform("diffuseCubeTexture", 2);
    shaderPrograms[i].setUniform("isCube", 0);
  }
  graphics::buffer::UniformBuffer meshUBO, cameraUBO, lightUBO;
  // Calculate UBO alignment size
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignSize);
  constexpr int perMeshSize = 2 * sizeof(glm::mat4);
  constexpr int perCameraSize = sizeof(glm::mat4) + sizeof(glm::vec4);
  constexpr int perLightSize = sizeof(glm::mat4) + 2 * sizeof(glm::vec4);
  int perMeshOffset = uboAlign(perMeshSize);
  int perCameraOffset = uboAlign(perCameraSize);
  int perLightOffset = uboAlign(perLightSize);
  meshUBO.allocate(MESH_COUNT * perMeshOffset, GL_DYNAMIC_DRAW);
  cameraUBO.allocate(CAMERA_COUNT * perCameraOffset, GL_DYNAMIC_DRAW);
  lightUBO.allocate(LIGHT_COUNT * perLightOffset, GL_DYNAMIC_DRAW);
  // Default to first data
  meshUBO.bindUniformBlockIndex(0, 0, perMeshSize);
  cameraUBO.bindUniformBlockIndex(1, 0, perCameraSize);
  lightUBO.bindUniformBlockIndex(2, 0, perLightSize);
  // Get texture information
  int maxTextureSize = 1024;
  // Uncomment the following line if your GPU is very poor
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
  maxTextureSize = std::min(maxTextureSize, 4096);
  // Camera
  std::vector<graphics::camera::CameraPTR> cameras;
  cameras.emplace_back(graphics::camera::QuaternionCamera::make_unique(glm::vec3(0, 0, 15)));
  assert(cameras.size() == CAMERA_COUNT);
  // TODO (Just an example for you, no need to modify here): Bind camera object's uniform buffer
  // Hint:
  //       1. what should we bind -> what will be used in shader: camera's view-projection matrix's & camera
  //          position's pointer
  //       2. where to bind -> remind VBO figure: we have to know the offset, size of the obj wanted to bind
  //       3. how to bind -> check spec slide to know binding procedure & trace the obj/class in the template to
  //          call class methods
  for (int i = 0; i < CAMERA_COUNT; ++i) {
    int offset = i * perCameraOffset;
    cameras[i]->initialize(OpenGLContext::getAspectRatio());
    cameraUBO.load(offset, sizeof(glm::mat4), cameras[i]->getViewProjectionMatrixPTR());
    cameraUBO.load(offset + sizeof(glm::mat4), sizeof(glm::vec4), cameras[i]->getPositionPTR());
  }
  currentCamera = cameras[0].get();
  // Lights
  glm::vec2 cutoff = glm::vec2(cos(glm::radians(5.0f)), glm::cos(glm::radians(22.0f)));
  std::vector<graphics::light::LightPTR> lights;
  lights.emplace_back(graphics::light::DirectionalLight::make_unique(glm::vec3(8, 6, 6)));
  lights.emplace_back(graphics::light::PointLight::make_unique(glm::vec3(8, 6, 6)));
  lights.emplace_back(graphics::light::Spotlight::make_unique(currentCamera->getFront(), cutoff));
  assert(lights.size() == LIGHT_COUNT);
  // TODO: Bind light object's buffer
  // Hint: look what we did when binding other UBO
  for (int i = 0; i < LIGHT_COUNT; ++i) {
    int offset = i * perLightOffset;
  }
  // Texture
  graphics::texture::ShadowMap shadow(maxTextureSize);
  graphics::texture::Texture2D colorOrange, wood;
  graphics::texture::TextureCubeMap dice;
  colorOrange.fromColor(glm::vec4(1, 0.5, 0, 1));
  // TODO: Read texture(and set color) for objects respectively
  // Hint: check the calss of the variable(wood, colorOrange, dice) we've created for you.
  //       fromFile member function
  // We currently set everything to a color
  wood.fromColor(glm::vec4(0.5, 0, 0.5, 1));
  dice.fromColor(glm::vec4(1, 0, 0, 1), glm::vec4(1, 0.5, 0, 1), glm::vec4(0, 1, 0, 1), glm::vec4(0, 0, 1, 1),
                 glm::vec4(1, 1, 1, 1), glm::vec4(1, 1, 0, 1));
  // Meshes
  std::vector<graphics::shape::ShapePTR> meshes;
  std::vector<graphics::texture::Texture*> diffuseTextures;
  {
    std::vector<GLfloat> vertexData;
    std::vector<GLuint> indexData;
    graphics::shape::Plane::generateVertices(vertexData, indexData, 40, 20, 20, false);
    auto sphere = graphics::shape::Sphere::make_unique();
    auto cube = graphics::shape::Cube::make_unique();
    auto ground = graphics::shape::Plane::make_unique(vertexData, indexData);

    glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(3, 0, 4));
    model = glm::scale(model, glm::vec3(2));
    model = glm::rotate(model, glm::half_pi<float>(), glm::vec3(1, 0, 0));
    sphere->setModelMatrix(model);

    model = glm::translate(glm::mat4(1), glm::vec3(-3, -1, 0));
    model = glm::scale(model, glm::vec3(2));
    cube->setModelMatrix(model);

    model = glm::translate(glm::mat4(1), glm::vec3(0, -3, 0));
    ground->setModelMatrix(model);

    meshes.emplace_back(std::move(ground));
    diffuseTextures.emplace_back(&wood);
    meshes.emplace_back(std::move(sphere));
    diffuseTextures.emplace_back(&colorOrange);
    meshes.emplace_back(std::move(cube));
    diffuseTextures.emplace_back(&wood);
  }
  assert(meshes.size() == MESH_COUNT);
  assert(diffuseTextures.size() == MESH_COUNT);
  for (int i = 0; i < MESH_COUNT; ++i) {
    int offset = i * perMeshOffset;
    meshUBO.load(offset, sizeof(glm::mat4), meshes[i]->getModelMatrixPTR());
    meshUBO.load(offset + sizeof(glm::mat4), sizeof(glm::mat4), meshes[i]->getNormalMatrixPTR());
  }
  shadow.bind(1);
  dice.bind(2);
  // Main rendering loop
  while (!glfwWindowShouldClose(window)) {
    // Polling events.
    glfwPollEvents();
    // Update camera's uniforms if camera moves.
    bool isCameraMove = currentCamera->move(window);
    if (isCameraMove || isWindowSizeChanged) {
      isWindowSizeChanged = false;
      cameraUBO.load(0, sizeof(glm::mat4), currentCamera->getViewProjectionMatrixPTR());
      cameraUBO.load(sizeof(glm::mat4), sizeof(glm::vec4), currentCamera->getPositionPTR());
      if (lights[currentLight]->getType() == graphics::light::LightType::Spot) {
        lights[currentLight]->update(currentCamera->getViewMatrix());
        int offset = currentLight * perLightOffset;
        glm::vec4 front = currentCamera->getFront();
        lightUBO.load(offset, sizeof(glm::mat4), lights[currentLight]->getLightSpaceMatrixPTR());
        lightUBO.load(offset + sizeof(glm::mat4), sizeof(glm::vec4), glm::value_ptr(front));
      }
    }

    if (isLightChanged) {
      int offset = currentLight * perLightOffset;
      // TODO: Switch light uniforms if light changes
      // Hint:
      //       1. we've load all the lights' unifroms eariler, so here we just tell shader where to start binding
      //       the next light info
      //       2. you should not bind the same light every time, because we are in a while-loop
      // Note: You can do this by a single line of lightUBO.bindUniformBlockIndex call
      if (lights[currentLight]->getType() == graphics::light::LightType::Spot) {
        lights[currentLight]->update(currentCamera->getViewMatrix());
        glm::vec4 front = currentCamera->getFront();
        lightUBO.load(offset, sizeof(glm::mat4), lights[currentLight]->getLightSpaceMatrixPTR());
        lightUBO.load(offset + sizeof(glm::mat4), sizeof(glm::vec4), glm::value_ptr(front));
      }
      isLightChanged = false;
    }
    // TODO (If you want to implement shadow): Render shadow to texture first
    // Hint: You need to change glViewport, glCullFace and bind shadow's framebuffer to render

    // GL_XXX_BIT can simply "OR" together to use.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Render all objects
    shaderPrograms[currentShader].use();
    for (int i = 0; i < MESH_COUNT; ++i) {
      // Change uniform if it is a cube (We want to use cubemap texture)
      if (meshes[i]->getType() == graphics::shape::ShapeType::Cube) {
        shaderPrograms[currentShader].setUniform("isCube", 1);
      } else {
        shaderPrograms[currentShader].setUniform("isCube", 0);
      }
      // Bind current object's model matrix
      meshUBO.bindUniformBlockIndex(0, i * perMeshOffset, perMeshSize);
      // Bind current object's texture
      diffuseTextures[i]->bind(0);
      // Render current object
      meshes[i]->draw();
    }
#ifdef __APPLE__
    // Some platform need explicit glFlush
    glFlush();
#endif
    glfwSwapBuffers(window);
  }
  return 0;
}
