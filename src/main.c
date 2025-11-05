#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shaders/shader.frag.h"
#include "shaders/shader.vert.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800

#define VERTEX_FLOATS 3

#define TRIANGLE_VERTICES 3
#define RECTANGLE_VERTICES 4
#define RECTANGLE_TRIANGLES 2

#define INDICES_PER_QUAD 6

struct Circle {
  float centerX;
  float centerY;
  float radius;

  struct {
    float r;
    float g;
    float b;
  } color;

  struct Circle* next;
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

GLfloat* lineVertices(struct Circle* c1, struct Circle* c2, float lineWidth) {
  GLfloat* vertices = (GLfloat*)malloc(RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat));

  float centerX1 = c1->centerX;
  float centerY1 = c1->centerY;
  float centerX2 = c2->centerX;
  float centerY2 = c2->centerY;

  float dx = centerX2 - centerX1;
  float dy = centerY2 - centerY1;

  double s = sqrt(dx * dx + dy * dy);

  float offsetX = -(lineWidth / 2.0f) * (dy / s);
  float offsetY = (lineWidth / 2.0f) * (dx / s);

  vertices[0] = centerX1 + offsetX; vertices[1] = centerY1 + offsetY; vertices[2] = 0.0f;
  vertices[3] = centerX1 - offsetX; vertices[4] = centerY1 - offsetY; vertices[5] = 0.0f;
  vertices[6] = centerX2 + offsetX; vertices[7] = centerY2 + offsetY; vertices[8] = 0.0f;
  vertices[9] = centerX2 - offsetX; vertices[10] = centerY2 - offsetY; vertices[11] = 0.0f;

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

  GLuint circleShaderProgram = glCreateProgram();
  glAttachShader(circleShaderProgram, vertexShader);
  glAttachShader(circleShaderProgram, fragmentShader);
  glLinkProgram(circleShaderProgram);

  GLuint lineFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar* lineFragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "}\n";
  glShaderSource(lineFragmentShader, 1, &lineFragmentShaderSource, NULL);
  glCompileShader(lineFragmentShader);

  GLuint lineVertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(lineVertexShader, 1, (const GLchar* const*)&vertexShaderSource, NULL);
  glCompileShader(lineVertexShader);

  GLuint lineShaderProgram = glCreateProgram();
  glAttachShader(lineShaderProgram, lineVertexShader);
  glAttachShader(lineShaderProgram, lineFragmentShader);
  glLinkProgram(lineShaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  glDeleteShader(lineVertexShader);
  glDeleteShader(lineFragmentShader);

  GLuint VBO, VAO, EBO;

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  struct Circle circle1 = {0.4f, -0.6f, 0.1f, {0.0f, 0.0f, 1.0f}};
  struct Circle circle2 = {0.0f, 0.0f, 0.1f, {1.0f, 0.0f, 0.0f}};
  struct Circle circle3 = {-0.5f, 0.5f, 0.1f, {0.0f, 1.0f, 0.0f}};

  circle1.next = &circle2;
  circle2.next = &circle3;
  circle3.next = NULL;

  int numCircles = 3;

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, numCircles * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  GLuint *allIndices = (GLuint*)malloc(numCircles * INDICES_PER_QUAD * sizeof(GLuint));
  for (int i = 0; i < numCircles; ++i) {
    GLuint base = (GLuint)(i * RECTANGLE_VERTICES);
    int off = i * INDICES_PER_QUAD;
    allIndices[off + 0] = base + 0;
    allIndices[off + 1] = base + 1;
    allIndices[off + 2] = base + 2;
    allIndices[off + 3] = base + 1;
    allIndices[off + 4] = base + 2;
    allIndices[off + 5] = base + 3;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numCircles * INDICES_PER_QUAD * sizeof(GLuint), allIndices, GL_STATIC_DRAW);
  free(allIndices);

  glVertexAttribPointer(0, VERTEX_FLOATS, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLuint lineVAO, lineVBO, lineEBO;
  glGenVertexArrays(1, &lineVAO);
  glGenBuffers(1, &lineVBO);
  glGenBuffers(1, &lineEBO);

  glBindVertexArray(lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
  glBufferData(GL_ARRAY_BUFFER, (numCircles - 1) * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  GLuint *lineAllIndices = (GLuint*)malloc((numCircles - 1) * INDICES_PER_QUAD * sizeof(GLuint));
  for (int i = 0; i < numCircles - 1; ++i) {
    GLuint base = (GLuint)(i * RECTANGLE_VERTICES);
    int off = i * INDICES_PER_QUAD;
    lineAllIndices[off + 0] = base + 0;
    lineAllIndices[off + 1] = base + 1;
    lineAllIndices[off + 2] = base + 2;
    lineAllIndices[off + 3] = base + 1;
    lineAllIndices[off + 4] = base + 2;
    lineAllIndices[off + 5] = base + 3;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, (numCircles - 1) * INDICES_PER_QUAD * sizeof(GLuint), lineAllIndices, GL_STATIC_DRAW);
  free(lineAllIndices);

  glVertexAttribPointer(0, VERTEX_FLOATS, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  float* batch = (float*)malloc(numCircles * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
  float* lineBatch = (float*)malloc((numCircles - 1) * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));

  while (!glfwWindowShouldClose(window)) {
    struct Circle* current = &circle1;
    for (int i = 0; i < numCircles; current = current->next, i++) {
      float* verts = circleVertices(*current);
      memcpy(batch + i * RECTANGLE_VERTICES * VERTEX_FLOATS, verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
      free(verts);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numCircles * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float), batch);

    current = &circle1;

    for (int i = 0; i < numCircles - 1; current = current->next, i++) {
      float* verts = lineVertices(current, current->next, 0.0075f);

      // print verts for debugging in groups of 3
      for (int j = 0; j < RECTANGLE_VERTICES; j++) {
        printf("(%f, %f, %f)\n", verts[j * 3 + 0], verts[j * 3 + 1], verts[j * 3 + 2]);
      }
      printf("\n");

      memcpy(lineBatch + (i * RECTANGLE_VERTICES * VERTEX_FLOATS), verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
      free(verts);
    }

    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (numCircles - 1) * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float), lineBatch);

    glClearColor(1.0f, 1.0f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(circleShaderProgram);

    GLint iCenterLocation = glGetUniformLocation(circleShaderProgram, "iCenter");
    GLint iColorLocation = glGetUniformLocation(circleShaderProgram, "iColor");
    GLint iResolutionLocation = glGetUniformLocation(circleShaderProgram, "iResolution");
    GLint iRadiusLocation = glGetUniformLocation(circleShaderProgram, "iRadius");

    glUniform2f(iResolutionLocation, WINDOW_WIDTH, WINDOW_HEIGHT);

    glBindVertexArray(VAO);

    current = &circle1;

    for (int i = 0; i < numCircles; current = current->next, i++) {
      void* color = &current->color;

      glUniform1f(iRadiusLocation, current->radius);
      glUniform3f(iCenterLocation, current->centerX, current->centerY, 0.0f);
      glUniform3f(iColorLocation, ((float*)color)[0], ((float*)color)[1], ((float*)color)[2]);

      glDrawElements(GL_TRIANGLES, INDICES_PER_QUAD, GL_UNSIGNED_INT, (const void*)(i * INDICES_PER_QUAD * sizeof(GLuint)));
    }
    glBindVertexArray(0);

    glUseProgram(lineShaderProgram);

    glBindVertexArray(lineVAO);

    for (int i = 0; i < numCircles - 1; i++) {
      glDrawElements(GL_TRIANGLES, INDICES_PER_QUAD, GL_UNSIGNED_INT, (const void*)(i * INDICES_PER_QUAD * sizeof(GLuint)));
    }
    glBindVertexArray(0);

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  free(lineBatch);
  free(batch);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);

  glDeleteVertexArrays(1, &lineVAO);
  glDeleteBuffers(1, &lineVBO);
  glDeleteBuffers(1, &lineEBO);

  glDeleteProgram(circleShaderProgram);
  glDeleteProgram(lineShaderProgram);

  free(vertexShaderSource);
  free(fragmentShaderSource);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
