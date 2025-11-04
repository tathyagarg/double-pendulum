#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <math.h>

#include "shaders/shader.frag.h"
#include "shaders/shader.vert.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define VERTEX_FLOATS 3

#define TRIANGLE_VERTICES 3
#define RECTANGLE_VERTICES 4
#define RECTANGLE_TRIANGLES 2

GLfloat* circleVertices(GLfloat centerX, GLfloat centerY, GLfloat radius) {
  GLfloat* vertices = (GLfloat*)malloc(RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat));
  vertices[0] = centerX + radius; vertices[1] = centerY + radius; vertices[2] = 0.0f;
  vertices[3] = centerX - radius; vertices[4] = centerY + radius; vertices[5] = 0.0f;
  vertices[6] = centerX + radius; vertices[7] = centerY - radius; vertices[8] = 0.0f;
  vertices[9] = centerX - radius; vertices[10] = centerY - radius; vertices[11] = 0.0f;

  return vertices;
}

int main(void) {
  const GLchar* _vertexShaderSource = (const GLchar*)shaders_shader_vert;
  const GLchar* _fragmentShaderSource = (const GLchar*)shaders_shader_frag;

  GLchar* vertexShaderSource = (GLchar*)malloc(shaders_shader_vert_len + 1);
  GLchar* fragmentShaderSource = (GLchar*)malloc(shaders_shader_frag_len + 1);

  memcpy(vertexShaderSource, _vertexShaderSource, shaders_shader_vert_len);
  memcpy(fragmentShaderSource, _fragmentShaderSource, shaders_shader_frag_len);

  fragmentShaderSource[ shaders_shader_frag_len ] = '\0';
  vertexShaderSource[ shaders_shader_vert_len ] = '\0';

  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Double Pendulum", NULL, NULL);
  if (window == NULL) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  gladLoadGL();
  glViewport(0, 0, 2 * WINDOW_WIDTH, 2 * WINDOW_HEIGHT);

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, (const GLchar* const*)&vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, (const GLchar* const*)&fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLuint VBO, VAO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  float centerX = 0.0f;
  float centerY = 0.0f;
  float radius = 1.0f;

  GLfloat* vertices = circleVertices(centerX, centerY, radius);

  for (int i = 0; i < RECTANGLE_VERTICES * VERTEX_FLOATS; i++) {
    printf("%f ", vertices[i]);
  }

  GLuint indices[] = {
    0, 1, 2,
    1, 2, 3,
  };

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, RECTANGLE_VERTICES, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(1.0f, 1.0f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    GLint iCenterLocation = glGetUniformLocation(shaderProgram, "iCenter");
    glUniform3f(iCenterLocation, centerX, centerY, 0.0f);

    GLint iColorLocation = glGetUniformLocation(shaderProgram, "iColor");
    glUniform3f(iColorLocation, 0.0f, 1.0f, 0.0f);

    GLint iResolutionLocation = glGetUniformLocation(shaderProgram, "iResolution");
    glUniform2f(iResolutionLocation, WINDOW_WIDTH, WINDOW_HEIGHT);

    GLint iRadiusLocation = glGetUniformLocation(shaderProgram, "iRadius");
    glUniform1f(iRadiusLocation, radius);

    vertices = circleVertices(centerX, centerY, radius);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, RECTANGLE_TRIANGLES * TRIANGLE_VERTICES, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shaderProgram);

  free(vertices);
  free(vertexShaderSource);
  free(fragmentShaderSource);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
