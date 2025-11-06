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

#define ANCHOR_X 0.0f
#define ANCHOR_Y 0.5f

#define ROD_WIDTH 0.0075f
#define BOB_RADIUS 0.05f

#define GRAVITY -9.81f
#define PI 3.14159265358979323846f
#define TIME_STEP 0.0166f

#define DEFAULT_THETA (3 * PI / 4.0f)
#define DEFAULT_OMEGA 0.9f

struct Bob {
  float centerX;
  float centerY;
  float radius;

  struct {
    float r;
    float g;
    float b;
  } color;

  float theta;
  float omega;

  float mass;
};

GLfloat* bobVertices(struct Bob* bob) {
  GLfloat* vertices = (GLfloat*)malloc(RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat));

  float centerX = bob->centerX;
  float centerY = bob->centerY;
  float radius = bob->radius;

  vertices[0] = centerX + radius; vertices[1] = centerY + radius; vertices[2] = 0.0f;
  vertices[3] = centerX - radius; vertices[4] = centerY + radius; vertices[5] = 0.0f;
  vertices[6] = centerX + radius; vertices[7] = centerY - radius; vertices[8] = 0.0f;
  vertices[9] = centerX - radius; vertices[10] = centerY - radius; vertices[11] = 0.0f;

  return vertices;
}

GLfloat* rodVertices(float centerX1, float centerY1, float centerX2, float centerY2, float rodWidth) {
  GLfloat* vertices = (GLfloat*)malloc(RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat));

  float dx = centerX2 - centerX1;
  float dy = centerY2 - centerY1;

  double s = sqrt(dx * dx + dy * dy);

  float offsetX = -(rodWidth / 2.0f) * (dy / s);
  float offsetY = (rodWidth / 2.0f) * (dx / s);

  vertices[0] = centerX1 + offsetX; vertices[1] = centerY1 + offsetY; vertices[2] = 0.0f;
  vertices[3] = centerX1 - offsetX; vertices[4] = centerY1 - offsetY; vertices[5] = 0.0f;
  vertices[6] = centerX2 + offsetX; vertices[7] = centerY2 + offsetY; vertices[8] = 0.0f;
  vertices[9] = centerX2 - offsetX; vertices[10] = centerY2 - offsetY; vertices[11] = 0.0f;

  return vertices;
}

GLfloat* createMatrixA(size_t n, float* thetas) {
  GLfloat* matrixA = (GLfloat*)malloc(n * n * sizeof(GLfloat));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float value = (n - fmax(i, j)) * cos(thetas[i] - thetas[j]);
      matrixA[i * n + j] = value;
    }
  }

  return matrixA;
}

GLfloat* createVectorB(size_t n, float* thetas, float* omegas) {
  GLfloat* vectorB = (GLfloat*)malloc(n * sizeof(GLfloat));

  for (int i = 0; i < n; i++) {
    float b_i = 0;
    for (int j = 0; j < n; j++) {
      b_i -= (n - fmax(i, j)) * omegas[j] * omegas[j] * sin(thetas[i] - thetas[j]);
    }
    b_i -= (n - i) * GRAVITY * sin(thetas[i]);

    vectorB[i] = b_i;
  }

  return vectorB;
}

GLfloat** lu_decompose(size_t n, GLfloat* A) {
  float* L = (GLfloat*)malloc(n * n * sizeof(GLfloat));
  for (int i = 0; i < n * n; i++) {
    L[i] = 0;
  }

  float* U = (GLfloat*)malloc(n * n * sizeof(GLfloat));
  for (int i = 0; i < n * n; i++) {
    U[i] = 0;
  }

  for (int i = 0; i < n; i++) {
    for (int k = i; k < n; k++) {
      float sum = 0;
      for (int j = 0; j < i; j++) {
        sum += L[i * n + j] * U[j * n + k];
      }
      U[i * n + k] = A[i * n + k] - sum;
    }

    for (int k = i; k < n; k++) {
      if (i == k) {
        L[i * n + i] = 1;
      } else {
        float sum = 0;
        for (int j = 0; j < i; j++) {
          sum += L[k * n + j] * U[j * n + i];
        }
        L[k * n + i] = (A[k * n + i] - sum) / U[i * n + i];
      }
    }
  }

  GLfloat** LU = (GLfloat**)malloc(2 * sizeof(GLfloat*));
  LU[0] = L;
  LU[1] = U;

  return LU;
}

GLfloat* forward_substitution(size_t n, GLfloat* L, GLfloat* B) {
  GLfloat* y = (GLfloat*)malloc(n * sizeof(GLfloat));
  for (int i = 0; i < n; i++) {
    y[i] = 0;
  }

  for (int i = 0; i < n; i++) {
    float sum = 0;
    for (int j = 0; j < i; j++) {
      sum += L[i * n + j] * y[j];
    }
    y[i] = B[i] - sum;
  }

  return y;
}

GLfloat* backward_substitution(size_t n, GLfloat* U, GLfloat* y) {
  GLfloat* x = (GLfloat*)malloc(n * sizeof(GLfloat));
  for (int i = 0; i < n; i++) {
    x[i] = 0;
  }

  for (int i = n - 1; i >= 0; i--) {
    float sum = 0;
    for (int j = i + 1; j < n; j++) {
      sum += U[i * n + j] * x[j];
    }
    x[i] = (y[i] - sum) / U[i * n + i];
  }

  return x;
}

GLfloat* solveLinearSystem(size_t n, GLfloat* A, GLfloat* B) {
  GLfloat* result = (GLfloat*)malloc(n * sizeof(GLfloat));

  GLfloat** LU = lu_decompose(n, A);
  GLfloat* L = LU[0];
  GLfloat* U = LU[1];

  GLfloat* y = forward_substitution(n, L, B);
  GLfloat* x = backward_substitution(n, U, y);

  memcpy(result, x, n * sizeof(GLfloat));

  free(x);
  free(y);
  free(L);
  free(U);
  free(LU);

  return result;
}

GLfloat** f(size_t n, GLfloat* thetas, GLfloat* omegas) {
  GLfloat* A = createMatrixA(n, thetas);
  GLfloat* B = createVectorB(n, thetas, omegas);

  GLfloat* alphas = solveLinearSystem(n, A, B);

  free(A);
  free(B);

  GLfloat** result = (GLfloat**)malloc(2 * sizeof(GLfloat*));
  result[0] = (GLfloat*)malloc(n * sizeof(GLfloat));
  result[1] = (GLfloat*)malloc(n * sizeof(GLfloat));

  memcpy(result[0], omegas, n * sizeof(GLfloat));
  memcpy(result[1], alphas, n * sizeof(GLfloat));

  free(alphas);

  return result;
}

GLfloat** rk4(float dt, size_t n, GLfloat* thetas, GLfloat* omegas) {
  GLfloat** k1 = f(n, thetas, omegas);

  GLfloat* k2_A = (GLfloat*)malloc(n * sizeof(GLfloat));
  GLfloat* k2_B = (GLfloat*)malloc(n * sizeof(GLfloat));
  for (int i = 0; i < n; i++) {
    k2_A[i] = thetas[i] + (dt / 2.0f) * k1[0][i];
    k2_B[i] = omegas[i] + (dt / 2.0f) * k1[1][i];
  }

  GLfloat** k2 = f(n, k2_A, k2_B);
  free(k2_A);
  free(k2_B);

  GLfloat* k3_A = (GLfloat*)malloc(n * sizeof(GLfloat));
  GLfloat* k3_B = (GLfloat*)malloc(n * sizeof(GLfloat));
  for (int i = 0; i < n; i++) {
    k3_A[i] = thetas[i] + (dt / 2.0f) * k2[0][i];
    k3_B[i] = omegas[i] + (dt / 2.0f) * k2[1][i];
  }

  GLfloat** k3 = f(n, k3_A, k3_B);
  free(k3_A);
  free(k3_B);

  GLfloat* k4_A = (GLfloat*)malloc(n * sizeof(GLfloat));
  GLfloat* k4_B = (GLfloat*)malloc(n * sizeof(GLfloat));
  for (int i = 0; i < n; i++) {
    k4_A[i] = thetas[i] + dt * k3[0][i];
    k4_B[i] = omegas[i] + dt * k3[1][i];
  }

  GLfloat** k4 = f(n, k4_A, k4_B);
  free(k4_A);
  free(k4_B);

  GLfloat* thetaDeltas = (GLfloat*)malloc(n * sizeof(GLfloat));
  GLfloat* omegaDeltas = (GLfloat*)malloc(n * sizeof(GLfloat));

  for (int i = 0; i < n; i++) {
    thetaDeltas[i] = (k1[0][i] + 2.0f * k2[0][i] + 2.0f * k3[0][i] + k4[0][i]) * (dt / 6.0f);
    omegaDeltas[i] = (k1[1][i] + 2.0f * k2[1][i] + 2.0f * k3[1][i] + k4[1][i]) * (dt / 6.0f);
  }

  free(k1[0]);
  free(k1[1]);
  free(k1);
  free(k2[0]);
  free(k2[1]);
  free(k2);
  free(k3[0]);
  free(k3[1]);
  free(k3);
  free(k4[0]);
  free(k4[1]);
  free(k4);

  GLfloat* newThetas = (GLfloat*)malloc(n * sizeof(GLfloat));
  GLfloat* newOmegas = (GLfloat*)malloc(n * sizeof(GLfloat));

  for (int i = 0; i < n; i++) {
    newThetas[i] = thetas[i] + thetaDeltas[i];
    newOmegas[i] = omegas[i] + omegaDeltas[i];
  }

  free(thetaDeltas);
  free(omegaDeltas);

  GLfloat** result = (GLfloat**)malloc(2 * sizeof(GLfloat*));
  result[0] = newThetas;
  result[1] = newOmegas;

  return result;
}

GLfloat** coordinates(size_t n, GLfloat* thetas) {
  float x = ANCHOR_X;
  float y = ANCHOR_Y;

  GLfloat** coords = (GLfloat**)malloc(n * sizeof(GLfloat*));
  for (int i = 0; i < n; i++) {
    coords[i] = (GLfloat*)malloc(2 * sizeof(GLfloat));
    x += sin(thetas[i]);
    y += cos(thetas[i]);

    coords[i][0] = x;
    coords[i][1] = y;
  }
  
  return coords;
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

  GLuint bobShaderProgram = glCreateProgram();
  glAttachShader(bobShaderProgram, vertexShader);
  glAttachShader(bobShaderProgram, fragmentShader);
  glLinkProgram(bobShaderProgram);

  GLuint rodFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar* rodFragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "    FragColor = vec4(0.5, 0.5, 0.5, 1.0);\n"
    "}\n";
  glShaderSource(rodFragmentShader, 1, &rodFragmentShaderSource, NULL);
  glCompileShader(rodFragmentShader);

  GLuint rodVertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(rodVertexShader, 1, (const GLchar* const*)&vertexShaderSource, NULL);
  glCompileShader(rodVertexShader);

  GLuint rodShaderProgram = glCreateProgram();
  glAttachShader(rodShaderProgram, rodVertexShader);
  glAttachShader(rodShaderProgram, rodFragmentShader);
  glLinkProgram(rodShaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  glDeleteShader(rodVertexShader);
  glDeleteShader(rodFragmentShader);

  GLuint bobVBO, bobVAO, bobEBO;

  glGenVertexArrays(1, &bobVAO);
  glGenBuffers(1, &bobVBO);
  glGenBuffers(1, &bobEBO);

  glBindVertexArray(bobVAO);

  struct Bob bob1 = {0.0f, 0.0f, BOB_RADIUS, {1.0f, 0.0f, 0.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};
  struct Bob bob2 = {0.0f, 0.0f, BOB_RADIUS, {0.0f, 1.0f, 0.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};
  struct Bob bob3 = {0.0f, 0.0f, BOB_RADIUS, {0.0f, 0.0f, 1.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};
  struct Bob bob4 = {0.0f, 0.0f, BOB_RADIUS, {1.0f, 1.0f, 0.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};
  struct Bob bob5 = {0.0f, 0.0f, BOB_RADIUS, {1.0f, 0.0f, 1.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};
  struct Bob bob6 = {0.0f, 0.0f, BOB_RADIUS, {0.0f, 1.0f, 1.0f}, DEFAULT_THETA, DEFAULT_OMEGA, 1.0f};

  struct Bob* bobs[] = {&bob1, &bob2, &bob3, &bob4, &bob5, &bob6};

  size_t numBobs = sizeof(bobs) / sizeof(bobs[0]);
  // size_t numBobs = 2;
  printf("Number of Bobs: %zu\n", numBobs);

  glBindBuffer(GL_ARRAY_BUFFER, bobVBO);
  glBufferData(GL_ARRAY_BUFFER, numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  GLuint *allIndices = (GLuint*)malloc(numBobs * INDICES_PER_QUAD * sizeof(GLuint));
  for (int i = 0; i < numBobs; ++i) {
    GLuint base = (GLuint)(i * RECTANGLE_VERTICES);
    int off = i * INDICES_PER_QUAD;
    allIndices[off + 0] = base + 0;
    allIndices[off + 1] = base + 1;
    allIndices[off + 2] = base + 2;
    allIndices[off + 3] = base + 1;
    allIndices[off + 4] = base + 2;
    allIndices[off + 5] = base + 3;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bobEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBobs * INDICES_PER_QUAD * sizeof(GLuint), allIndices, GL_STATIC_DRAW);
  free(allIndices);

  glVertexAttribPointer(0, VERTEX_FLOATS, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  GLuint rodVAO, rodVBO, rodEBO;
  glGenVertexArrays(1, &rodVAO);
  glGenBuffers(1, &rodVBO);
  glGenBuffers(1, &rodEBO);

  glBindVertexArray(rodVAO);
  glBindBuffer(GL_ARRAY_BUFFER, rodVBO);
  glBufferData(GL_ARRAY_BUFFER, numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  GLuint *rodAllIndices = (GLuint*)malloc(numBobs * INDICES_PER_QUAD * sizeof(GLuint));
  for (int i = 0; i < numBobs; ++i) {
    GLuint base = (GLuint)(i * RECTANGLE_VERTICES);
    int off = i * INDICES_PER_QUAD;
    rodAllIndices[off + 0] = base + 0;
    rodAllIndices[off + 1] = base + 1;
    rodAllIndices[off + 2] = base + 2;
    rodAllIndices[off + 3] = base + 1;
    rodAllIndices[off + 4] = base + 2;
    rodAllIndices[off + 5] = base + 3;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rodEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBobs * INDICES_PER_QUAD * sizeof(GLuint), rodAllIndices, GL_STATIC_DRAW);
  free(rodAllIndices);

  glVertexAttribPointer(0, VERTEX_FLOATS, GL_FLOAT, GL_FALSE, VERTEX_FLOATS * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  float* batch = (float*)malloc(numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
  float* rodBatch = (float*)malloc(numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));

  static double previousSeconds = 0.0;
  while (!glfwWindowShouldClose(window)) {
    GLfloat* thetas = (GLfloat*)malloc(numBobs * sizeof(GLfloat));
    GLfloat* omegas = (GLfloat*)malloc(numBobs * sizeof(GLfloat));

    for (int i = 0; i < numBobs; i++) {
      thetas[i] = bobs[i]->theta;
      omegas[i] = bobs[i]->omega;
    }

    GLfloat** rk4Result = rk4(TIME_STEP, numBobs, thetas, omegas);
    GLfloat* newThetas = rk4Result[0];
    GLfloat* newOmegas = rk4Result[1];

    for (int i = 0; i < numBobs; i++) {
      bobs[i]->theta = newThetas[i];
      bobs[i]->omega = newOmegas[i];

      thetas[i] = bobs[i]->theta;
    }

    free(omegas);
    free(newThetas);
    free(newOmegas);
    free(rk4Result);

    GLfloat** coords = coordinates(numBobs, thetas);
    for (int i = 0; i < numBobs; i++) {
      bobs[i]->centerX = ANCHOR_X + coords[i][0] * 1.5f / numBobs;
      bobs[i]->centerY = ANCHOR_Y + coords[i][1] * 1.5f / numBobs;

      free(coords[i]);
    }

    free(thetas);
    free(coords);

    for (int i = 0; i < numBobs; i++) {
      float* verts = bobVertices(bobs[i]);
      memcpy(batch + i * RECTANGLE_VERTICES * VERTEX_FLOATS, verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
      free(verts);
    }

    glBindBuffer(GL_ARRAY_BUFFER, bobVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float), batch);

    float* verts = rodVertices(ANCHOR_X, ANCHOR_Y, bobs[0]->centerX, bobs[0]->centerY, ROD_WIDTH);
    memcpy(rodBatch, verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));

    for (int i = 0; i < numBobs - 1; i++) {
      float* verts = rodVertices(bobs[i]->centerX, bobs[i]->centerY, bobs[i+1]->centerX, bobs[i+1]->centerY, ROD_WIDTH);
      memcpy(rodBatch + ((i + 1) * RECTANGLE_VERTICES * VERTEX_FLOATS), verts, RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float));
      free(verts);
    }

    glBindBuffer(GL_ARRAY_BUFFER, rodVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, numBobs * RECTANGLE_VERTICES * VERTEX_FLOATS * sizeof(float), rodBatch);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(rodShaderProgram);

    glBindVertexArray(rodVAO);

    for (int i = 0; i < numBobs; i++) {
      glDrawElements(GL_TRIANGLES, INDICES_PER_QUAD, GL_UNSIGNED_INT, (const void*)(i * INDICES_PER_QUAD * sizeof(GLuint)));
    }
    glBindVertexArray(0);


    glUseProgram(bobShaderProgram);

    GLint iCenterLocation = glGetUniformLocation(bobShaderProgram, "iCenter");
    GLint iColorLocation = glGetUniformLocation(bobShaderProgram, "iColor");
    GLint iResolutionLocation = glGetUniformLocation(bobShaderProgram, "iResolution");
    GLint iRadiusLocation = glGetUniformLocation(bobShaderProgram, "iRadius");

    glUniform2f(iResolutionLocation, WINDOW_WIDTH, WINDOW_HEIGHT);

    glBindVertexArray(bobVAO);

    for (int i = 0; i < numBobs; i++) {
      void* color = &bobs[i]->color;

      glUniform1f(iRadiusLocation, bobs[i]->radius);
      glUniform3f(iCenterLocation, bobs[i]->centerX, bobs[i]->centerY, 0.0f);
      glUniform3f(iColorLocation, ((float*)color)[0], ((float*)color)[1], ((float*)color)[2]);

      glDrawElements(GL_TRIANGLES, INDICES_PER_QUAD, GL_UNSIGNED_INT, (const void*)(i * INDICES_PER_QUAD * sizeof(GLuint)));
    }
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();

    // Calculate the FPS, and set the window title accordingly. Limit FPS to 60.
    double currentSeconds = glfwGetTime();
    double elapsedSeconds = currentSeconds - previousSeconds;
    if (elapsedSeconds > 0.016) {
      previousSeconds = currentSeconds;

      double fps = 1.0 / elapsedSeconds;
      char tmp[128];
      sprintf(tmp, "Multi-Pendulum Simulation - %.0f FPS", fps);
      glfwSetWindowTitle(window, tmp);
    } else {
      continue;
    }
  }

  free(rodBatch);
  free(batch);

  glDeleteVertexArrays(1, &bobVAO);
  glDeleteBuffers(1, &bobVBO);
  glDeleteBuffers(1, &bobEBO);

  glDeleteVertexArrays(1, &rodVAO);
  glDeleteBuffers(1, &rodVBO);
  glDeleteBuffers(1, &rodEBO);

  glDeleteProgram(bobShaderProgram);
  glDeleteProgram(rodShaderProgram);

  free(vertexShaderSource);
  free(fragmentShaderSource);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
