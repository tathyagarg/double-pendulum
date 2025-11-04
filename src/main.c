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

GLuint RECTANGLE_INDICES[] = {
  0, 1, 2,
  1, 2, 3,
};

struct Circle {
  float centerX;
  float centerY;
  float radius;
};

GLfloat* circleVertices(struct Circle circle) {
  GLfloat* vertices = (GLfloat*)malloc(RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat));

  float centerX = circle.centerX;
  float centerY = circle.centerY;
  float radius = circle.radius;

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

  struct Circle circle1 = {0.0f, 0.0f, 0.2f};
  struct Circle circle2 = {0.4f, -0.6f, 0.2f};
  struct Circle circle3 = {-0.5f, 0.5f, 0.2f};

  struct Circle circles[] = {circle1, circle2, circle3};
  int numCircles = sizeof(circles) / sizeof(circles[0]);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, numCircles * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(RECTANGLE_INDICES), RECTANGLE_INDICES, GL_STATIC_DRAW);

  glVertexAttribPointer(0, RECTANGLE_VERTICES, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  float* batch = (float*)malloc(numCircles * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
  int p = 0;

  while (!glfwWindowShouldClose(window)) {
    p = 0;

    glClearColor(1.0f, 1.0f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);

    GLint iCenterLocation = glGetUniformLocation(shaderProgram, "iCenter");
    GLint iColorLocation = glGetUniformLocation(shaderProgram, "iColor");
    GLint iResolutionLocation = glGetUniformLocation(shaderProgram, "iResolution");
    GLint iRadiusLocation = glGetUniformLocation(shaderProgram, "iRadius");

    glUniform2f(iResolutionLocation, WINDOW_WIDTH, WINDOW_HEIGHT);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    for (int i = 0; i < numCircles; i++) {
      glUniform1f(iRadiusLocation, circles[i].radius);
      glUniform3f(iCenterLocation, circles[i].centerX, circles[i].centerY, 0.0f);
      glUniform3f(iColorLocation, 0.0f, 1.0f, 0.0f);

      float* verts = circleVertices(circles[i]);
      memcpy(batch + p, verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
      p += RECTANGLE_VERTICES * VERTEX_FLOATS;

      glBufferSubData(GL_ARRAY_BUFFER, i * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), verts);

      free(verts);

      glBindVertexArray(VAO);
      glDrawElements(GL_TRIANGLES, RECTANGLE_TRIANGLES * TRIANGLE_VERTICES, GL_UNSIGNED_INT, (const void*)20);
    }


    // vertices = circleVertices(circle1);

    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    // glUniform1f(iRadiusLocation, circle1.radius);
    // glUniform3f(iCenterLocation, circle1.centerX, circle1.centerY, 0.0f);
    // glUniform3f(iColorLocation, 0.0f, 1.0f, 0.0f);

    // glBindVertexArray(VAO);
    // glDrawElements(GL_TRIANGLES, RECTANGLE_TRIANGLES * TRIANGLE_VERTICES, GL_UNSIGNED_INT, 0);
    // free(vertices);

    // vertices2 = circleVertices(circle2);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), vertices2, GL_STATIC_DRAW);

    // glUniform1f(iRadiusLocation, circle2.radius);
    // glUniform3f(iCenterLocation, circle2.centerX, circle2.centerY, 0.0f);
    // glUniform3f(iColorLocation, 1.0f, 0.0f, 0.0f);

    // glBindVertexArray(VAO);
    // glDrawElements(GL_TRIANGLES, RECTANGLE_TRIANGLES * TRIANGLE_VERTICES, GL_UNSIGNED_INT, 0);
    // free(vertices2);

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shaderProgram);

  free(batch);
  free(vertexShaderSource);
  free(fragmentShaderSource);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
