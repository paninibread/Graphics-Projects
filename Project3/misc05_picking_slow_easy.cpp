#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

using namespace glm;
using namespace std;

const int window_width = 1024, window_height = 768;
GLFWwindow* window;
typedef struct Vertex {
    float Position[4];
    float Color[4];
    float Normal[3];

    void SetPosition(float* coords) {
        Position[0] = coords[0];
        Position[1] = coords[1];
        Position[2] = coords[2];
        Position[3] = 1.0;
    }

    void SetColor(float* color) {
        Color[0] = color[0];
        Color[1] = color[1];
        Color[2] = color[4];
        Color[3] = color[3];
    }

    void SetNormal(float* normal) {
        Normal[0] = normal[0];
        Normal[1] = normal[1];
        Normal[2] = normal[2];
    }
} Vertex;
glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;
GLuint programID;
GLuint tessProgramID;
const GLuint numObjects = 256;
GLuint vertexBufferID[numObjects] = { 0 };
GLuint vertexArrayID[numObjects] = { 0 };
GLuint indexBufferID[numObjects] = { 0 };
size_t numIndices[numObjects] = { 0 };
size_t vertexBufferSize[numObjects] = { 0 };
size_t indexBufferSize[numObjects] = { 0 };
Vertex* suzanne_verts;
GLushort* suzanne_idcs;
GLuint matrixID;
GLuint modelMatrixID;
GLuint viewMatrixID;
GLuint projectionMatrixID;
GLuint lightID;

int createMesh(int n) {
    float tessilationFactor = 2.07;
    Vertex coordVerts[] =
    {
        { { 0.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 5.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 5.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 5.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
    };
    if (n > 50) {
        return 0;
    }
    if (n > 60) {
        return 1;
    }
    if (n > 70) {
        return 2;
    }
    if (n > 90) {
        return 3;
    }
    if (n > 100) {
        return 4;
    }
    if (n > 150) {
        return 5;
    }
    if (n > 250) {
        return 6;
    }

    float alpha = n * 3.14;
    float beta = alpha * tessilationFactor;
    int numberTriangles = 216;
    float meshes[216];
    float finalFactor = 0;
    for (int i = 0; i < 216; i++) {
        meshes[i] = beta * tessilationFactor / n;
    }
    for (int i = 0; i < 216; i++) {
        finalFactor = finalFactor + meshes[i];
    }
    matrixID = glGetUniformLocation(programID, "MVP");
    modelMatrixID = glGetUniformLocation(programID, "M");
    viewMatrixID = glGetUniformLocation(programID, "V");
    projectionMatrixID = glGetUniformLocation(programID, "P");
    lightID = glGetUniformLocation(programID, "lightPosition_worldspace");
    return finalFactor;
}




GLuint tessMatrixID;
GLuint tessModelMatrixID;
GLuint tessViewMatrixID;
GLuint tessProjectionMatrixID;
GLuint tessLightID;
GLfloat tessellationLevelInnerID;
GLfloat tessellationLevelOuterID;
GLfloat cameraAngleTheta = 3.142 / 4;
GLfloat cameraAnglePhi = asin(1 / sqrt(3));
GLfloat cameraSphereRadius = sqrt(300);
bool moveCameraLeft = false;
bool moveCameraRight = false;
bool moveCameraUp = false;
bool moveCameraDown = false;
bool shouldResetScene = false;
float tessellationLevel = 1.0f;
bool sTM = false;
bool sDW = false;
bool sTexureModel = false;
bool shiftPress = false;

int initWindow(void);
void initOpenGL(void);
void createObjects(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
GLuint loadStandardShaders(const char*, const char*);
GLuint loadTessShaders(const char*, const char*, const char*, const char*);
void loadObject(char*, glm::vec4, Vertex*&, GLushort*&, int);
void createVAOs(Vertex[], GLushort[], int);
void renderScene(void);
void cleanup(void);
void resetScene(void);
int createFace(int n);



void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    gViewMatrix = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f),
        glm::vec3(0.0f, 5.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    programID = loadStandardShaders("Standard.vert", "Standard.frag");
    tessProgramID = loadTessShaders("Tessellation.vs.glsl", "Tessellation.tc.glsl", "Tessellation.te.glsl",
        "Tessellation.fs.glsl");
    matrixID = glGetUniformLocation(programID, "MVP");
    modelMatrixID = glGetUniformLocation(programID, "M");
    viewMatrixID = glGetUniformLocation(programID, "V");
    projectionMatrixID = glGetUniformLocation(programID, "P");
    lightID = glGetUniformLocation(programID, "lightPosition_worldspace");
    tessModelMatrixID = glGetUniformLocation(tessProgramID, "M");
    tessViewMatrixID = glGetUniformLocation(tessProgramID, "V");
    tessProjectionMatrixID = glGetUniformLocation(tessProgramID, "P");
    tessLightID = glGetUniformLocation(tessProgramID, "lightPosition_worldspace");
    tessellationLevelInnerID = glGetUniformLocation(tessProgramID, "tessellationLevelInner");
    tessellationLevelOuterID = glGetUniformLocation(tessProgramID, "tessellationLevelOuter");
    createObjects();
}

void createObjects() {
    Vertex coordVerts[] =
    {
        { { 0.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 5.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 5.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 5.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
    };
    vertexBufferSize[0] = sizeof(coordVerts);	// ATTN: this needs to be done for each hand-made object with the ObjectID (subscript)
    createVAOs(coordVerts, NULL, 0);
    Vertex GridVerts[44];
    int k = 0;
    for (int i = -5; i <= 5; i++) {
        GridVerts[4 * k] = { {(float)i, 0, -5, 1.0}, {1.0, 1.0, 1.0, 1.0}, {0.0, 0.0, 1.0} };
        GridVerts[4 * k + 1] = { {(float)i, 0, 5, 1.0}, {1.0, 1.0, 1.0, 1.0}, {0.0, 0.0, 1.0} };
        GridVerts[4 * k + 2] = { {-5, 0, (float)i, 1.0}, {1.0, 1.0, 1.0, 1.0}, {0.0, 0.0, 1.0} };
        GridVerts[4 * k + 3] = { {5, 0, (float)i, 1.0}, {1.0, 1.0, 1.0, 1.0}, {0.0, 0.0, 1.0} };
        k++;
    }
    vertexBufferSize[2] = sizeof(GridVerts);
    numIndices[2] = 44;
    createVAOs(GridVerts, NULL, 2);
    loadObject("final.obj", glm::vec4(0.5, 0.4, 0.8, 1.0), suzanne_verts, suzanne_idcs, 1);
    createVAOs(suzanne_verts, suzanne_idcs, 1);
}

void loadObject(char* file, glm::vec4 color, Vertex*& out_vertices, GLushort*& out_indices, int objectID) {
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    bool res = loadOBJ(file, vertices, normals);
    vector<GLushort> indices;
    vector<glm::vec3> indexed_vertices;
    vector<glm::vec3> indexed_normals;
    indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);
    const size_t vert_count = indexed_vertices.size();
    const size_t idx_count = indices.size();
    out_vertices = new Vertex[vert_count];
    for (int i = 0; i < vert_count; i++) {
        out_vertices[i].SetPosition(&indexed_vertices[i].x);
        out_vertices[i].SetNormal(&indexed_normals[i].x);
        out_vertices[i].SetColor(&color[0]);
    }
    out_indices = new GLushort[idx_count];
    for (int i = 0; i < idx_count; i++) {
        out_indices[i] = indices[i];
    }
    numIndices[objectID] = idx_count;
    vertexBufferSize[objectID] = sizeof(out_vertices[0]) * vert_count;
    indexBufferSize[objectID] = sizeof(GLushort) * idx_count;
}

int createFace(int n) {
    float tessilationFactor = 2.07;
    float alpha = n * 3.14;
    float beta = alpha * tessilationFactor;
    int numberTriangles = 216;
    float meshes[216];
    float finalFactor = 0;
    for (int i = 0; i < 216; i++) {
        meshes[i] = beta * tessilationFactor / n;
    }
    for (int i = 0; i < 216; i++) {
        finalFactor = finalFactor + meshes[i];
    }
    return finalFactor;
}

void createVAOs(Vertex vertices[], GLushort indices[], int objectID) {
    GLenum errorCheckValue = glGetError();
    const size_t vertexSize = sizeof(vertices[0]);
    const size_t colorOffset = sizeof(vertices[0].Position);
    const size_t normalOffset = colorOffset + sizeof(vertices[0].Color);
    glGenVertexArrays(1, &vertexArrayID[objectID]);
    glBindVertexArray(vertexArrayID[objectID]);
    glGenBuffers(1, &vertexBufferID[objectID]);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID[objectID]);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize[objectID], vertices, GL_STATIC_DRAW);
    if (indices != NULL) {
        glGenBuffers(1, &indexBufferID[objectID]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID[objectID]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize[objectID], indices, GL_STATIC_DRAW);
    }
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, vertexSize, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)colorOffset);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)normalOffset);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    errorCheckValue = glGetError();
    if (errorCheckValue != GL_NO_ERROR) {
        fprintf(stderr, "Error: Could not create a VBO: %s\n", gluErrorString(errorCheckValue));
    }
}

void renderScene() {
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    int finalFactor = createFace(500);
    int finalMesh = createMesh(87);
    if (moveCameraLeft) {
        cameraAngleTheta -= 0.01f;
    }
    if (moveCameraRight) {
        cameraAngleTheta += 0.01f;
    }
    if (moveCameraUp) {
        cameraAnglePhi -= 0.01f;
    }
    if (moveCameraDown) {
        cameraAnglePhi += 0.01f;
    }
    if (moveCameraLeft || moveCameraRight || moveCameraDown || moveCameraUp || shouldResetScene) {
        float camX = cameraSphereRadius * cos(cameraAnglePhi) * sin(cameraAngleTheta);
        float camY = cameraSphereRadius * sin(cameraAnglePhi);
        float camZ = cameraSphereRadius * cos(cameraAnglePhi) * cos(cameraAngleTheta);
        gViewMatrix = glm::lookAt(glm::vec3(camX, camY, camZ),	// eye
            glm::vec3(0.0, 5.0, 0.0),	// center
            glm::vec3(0.0, 1.0, 0.0));	// up
    }
    if (sDW) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glm::vec3 lightPos = glm::vec3(20.0f, 20.0f, 0.0f);
    glm::mat4x4 modelMatrix = glm::mat4(1.0);
    glUseProgram(programID);
    {
        glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
        glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
        glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
        glBindVertexArray(vertexArrayID[0]);	// draw CoordAxes
        glDrawArrays(GL_LINES, 0, 6);
        glBindVertexArray(vertexArrayID[2]);
        glDrawArrays(GL_LINES, 0, numIndices[2]);
        if (!sTM) {
            glBindVertexArray(vertexArrayID[1]);
            glDrawElements(GL_TRIANGLES, numIndices[1], GL_UNSIGNED_SHORT, (void*)0);
        }
        glBindVertexArray(0);
    }
    if (sTM) {
        glUseProgram(tessProgramID);
        {
            glUniform3f(tessLightID, lightPos.x, lightPos.y, lightPos.z);
            glUniformMatrix4fv(tessViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
            glUniformMatrix4fv(tessProjectionMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
            glUniformMatrix4fv(tessModelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
            glUniform1f(tessellationLevelInnerID, tessellationLevel);
            glUniform1f(tessellationLevelOuterID, tessellationLevel);
            glPatchParameteri(GL_PATCH_VERTICES, 3);
            glBindVertexArray(vertexArrayID[1]);
            glDrawElements(GL_PATCHES, numIndices[1], GL_UNSIGNED_SHORT, (void*)0);
            glBindVertexArray(0);
        }
    }
    glUseProgram(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

GLuint loadStandardShaders(const char* vert_file_path, const char* frag_file_path) {
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    string vertexShaderCode;
    ifstream vertexShaderStream(vert_file_path, ios::in);
    if (vertexShaderStream.is_open()) {
        string line = "";
        while (std::getline(vertexShaderStream, line)) {
            vertexShaderCode += "\n" + line;
        }
        vertexShaderStream.close();
    }
    else {
        return 0;
    }
    string fragmentShaderCode;
    ifstream fragmentShaderStream(frag_file_path, ios::in);
    if (fragmentShaderStream.is_open()) {
        string line = "";
        while (std::getline(fragmentShaderStream, line)) {
            fragmentShaderCode += "\n" + line;
        }
        fragmentShaderStream.close();
    }
    else {
        return 0;
    }
    GLint result = GL_FALSE;
    int infoLogLength;
    char const* vertexSource = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSource, NULL);
    glCompileShader(vertexShaderID);

    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
        cout << &vertexShaderErrorMessage[0] << endl;
    }
    char const* fragmentSource = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
        cout << &fragmentShaderErrorMessage[0] << endl;
    }
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glGetShaderiv(programID, GL_LINK_STATUS, &result);
    glGetShaderiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        vector<char> programErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(programID, infoLogLength, NULL, &programErrorMessage[0]);
        cout << programErrorMessage[0] << endl;
    }
    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    return programID;
}

GLuint loadTessShaders(const char* tess_vert_file_path, const char* tess_ctrl_file_path, const char* tess_eval_file_path,
    const char* tess_frag_file_path) {
    GLuint tessVertShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint tessCtrlShaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
    GLuint tessEvalShaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
    GLuint tessFragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    string tessVertexShaderCode;
    ifstream tessVertexShaderStream(tess_vert_file_path, std::ios::in);
    if (tessVertexShaderStream.is_open()) {
        string line = "";
        while (std::getline(tessVertexShaderStream, line)) {
            tessVertexShaderCode += "\n" + line;
        }
        tessVertexShaderStream.close();
    }
    else {
        printf("Impossible to open %s.\n", tess_vert_file_path);
        getchar();
        return 0;
    }

    string tessCtrlShaderCode;
    ifstream tessCtrlShaderStream(tess_ctrl_file_path, std::ios::in);
    if (tessCtrlShaderStream.is_open()) {
        string line = "";
        while (std::getline(tessCtrlShaderStream, line)) {
            tessCtrlShaderCode += "\n" + line;
        }
        tessCtrlShaderStream.close();
    }
    else {
        printf("Impossible to open %s\n", tess_ctrl_file_path);
        getchar();
        return 0;
    }

    string tessEvalShaderCode;
    ifstream tessEvalShaderStream(tess_eval_file_path, std::ios::in);
    if (tessEvalShaderStream.is_open()) {
        string line = "";
        while (std::getline(tessEvalShaderStream, line)) {
            tessEvalShaderCode += "\n" + line;
        }
        tessEvalShaderStream.close();
    }
    else {
        printf("Impossible to open %s.\n", tess_eval_file_path);
        getchar();
        return 0;
    }

    string tessFragShaderCode;
    ifstream tessFragShaderStream(tess_frag_file_path, std::ios::in);
    if (tessFragShaderStream.is_open()) {
        string line = "";
        while (std::getline(tessFragShaderStream, line)) {
            tessFragShaderCode += "\n" + line;
        }
        tessFragShaderStream.close();
    }
    else {
        printf("Impossible to open %s.\n", tess_frag_file_path);
        getchar();
        return 0;
    }

    GLint result = false;
    int infoLogLength;

    printf("Compiling shader: %s\n", tess_vert_file_path);
    char const* tessVertSourcePointer = tessVertexShaderCode.c_str();
    glShaderSource(tessVertShaderID, 1, &tessVertSourcePointer, NULL);
    glCompileShader(tessVertShaderID);
    glGetShaderiv(tessVertShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessVertShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> tessVertShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessVertShaderID, infoLogLength, NULL, &tessVertShaderErrMsg[0]);
        printf("%s\n", &tessVertShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_ctrl_file_path);
    char const* tessCtrlSourcePointer = tessCtrlShaderCode.c_str();
    glShaderSource(tessCtrlShaderID, 1, &tessCtrlSourcePointer, NULL);
    glCompileShader(tessCtrlShaderID);
    glGetShaderiv(tessCtrlShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessCtrlShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> tessCtrlShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessCtrlShaderID, infoLogLength, NULL, &tessCtrlShaderErrMsg[0]);
        printf("%s\n", &tessCtrlShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_eval_file_path);
    char const* tessEvalSourcePointer = tessEvalShaderCode.c_str();
    glShaderSource(tessEvalShaderID, 1, &tessEvalSourcePointer, NULL);
    glCompileShader(tessEvalShaderID);
    glGetShaderiv(tessEvalShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessEvalShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> tessEvalShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessEvalShaderID, infoLogLength, NULL, &tessEvalShaderErrMsg[0]);
        printf("%s\n", &tessEvalShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_frag_file_path);
    char const* tessFragSourcePointer = tessFragShaderCode.c_str();
    glShaderSource(tessFragShaderID, 1, &tessFragSourcePointer, NULL);
    glCompileShader(tessFragShaderID);
    glGetShaderiv(tessFragShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessFragShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> tessFragShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessFragShaderID, infoLogLength, NULL, &tessFragShaderErrMsg[0]);
        printf("%s\n", &tessFragShaderErrMsg[0]);
    }

    printf("Linking Shader\n");
    GLuint tessProgramID = glCreateProgram();
    glAttachShader(tessProgramID, tessVertShaderID);
    glAttachShader(tessProgramID, tessCtrlShaderID);
    glAttachShader(tessProgramID, tessEvalShaderID);
    glAttachShader(tessProgramID, tessFragShaderID);
    glLinkProgram(tessProgramID);

    glGetProgramiv(tessProgramID, GL_LINK_STATUS, &result);
    glGetProgramiv(tessProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> tessProgramErrMsg(infoLogLength + 1);
        glGetProgramInfoLog(tessProgramID, infoLogLength, NULL, &tessProgramErrMsg[0]);
        printf("%s\n", &tessProgramErrMsg[0]);
    }

    glDetachShader(tessProgramID, tessVertShaderID);
    glDetachShader(tessProgramID, tessCtrlShaderID);
    glDetachShader(tessProgramID, tessEvalShaderID);
    glDetachShader(tessProgramID, tessFragShaderID);

    glDeleteShader(tessVertShaderID);
    glDeleteShader(tessCtrlShaderID);
    glDeleteShader(tessEvalShaderID);
    glDeleteShader(tessFragShaderID);

    return tessProgramID;
}

int initWindow() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW!\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(window_width, window_height, "Sharma, Prashast(29732270)", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW!\n");
        return -1;
    }

    glfwSetCursorPos(window, window_width / 2, window_height / 2);
    glfwSetKeyCallback(window, keyCallback);

    return 0;
}

void cleanup() {
    for (int i = 0; i < numObjects; i++) {
        glDeleteBuffers(1, &vertexBufferID[i]);
        glDeleteBuffers(1, &indexBufferID[i]);
        glDeleteVertexArrays(1, &vertexArrayID[i]);
    }
    glDeleteProgram(programID);
    glfwTerminate();
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = true;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = true;
            break;
        case GLFW_KEY_UP:
            moveCameraDown = true;
            break;
        case GLFW_KEY_DOWN:
            moveCameraUp = true;
            break;
        case GLFW_KEY_R:
            shouldResetScene = true;
            resetScene();
            break;
        case GLFW_KEY_LEFT_SHIFT:
            shiftPress = true;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            shiftPress = true;
            break;
        default:
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = false;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = false;
            break;
        case GLFW_KEY_UP:
            moveCameraDown = false;
            break;
        case GLFW_KEY_DOWN:
            moveCameraUp = false;
            break;
        case GLFW_KEY_R:
            shouldResetScene = false;
            break;
        case GLFW_KEY_P:
            sTM = !sTM;
            tessellationLevel = 4.0f;
            break;
        case GLFW_KEY_F:
            if (shiftPress) {
                sTexureModel = !sTexureModel;
            }
            else {
                sDW = !sDW;
            }
            break;
        case GLFW_KEY_LEFT_SHIFT:
            shiftPress = false;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            shiftPress = false;
            break;
        default:
            break;
        }
    }
}

void resetScene(void) {
    cameraAngleTheta = 3.142 / 4;
    cameraAnglePhi = asin(1 / sqrt(3));
    // loadControlPoints(2);
}



int main(void) {
    int errorCode = initWindow();
    if (errorCode != 0) {
        return errorCode;
    }

    initOpenGL();

    do {
        renderScene();
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    cleanup();

    return 0;
}
