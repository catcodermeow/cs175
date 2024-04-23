////////////////////////////////////////////////////////////////////////
//
//   Harvard University
//   CS175 : Computer Graphics
//   Professor Steven Gortler
//
////////////////////////////////////////////////////////////////////////

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#define GLEW_STATIC


#include "GL/glew.h"
#include "GL/glfw3.h"

#include "cvec.h"
#include "geometrymaker.h"
#include "glsupport.h"
#include "matrix4.h"
#include "ppm.h"
#include "arcball.h"
#include "quat.h"
#include "rigtform.h"

using namespace std; // for string, vector, iostream, and other standard C++ stuff

// G L O B A L S ///////////////////////////////////////////////////

static const float g_frustMinFov = 60.0; // A minimal of 60 degree field of view
static float g_frustFovY = g_frustMinFov; // FOV in y direction (updated by updateFrustFovY)

static const float g_frustNear = -0.1;  // near plane
static const float g_frustFar = -50.0;  // far plane
static const float g_groundY = -2.0;    // y coordinate of the ground
static const float g_groundSize = 10.0; // half the ground length

static GLFWwindow* g_window;

static int g_windowWidth = 512;
static int g_windowHeight = 512;
static double g_wScale = 1;
static double g_hScale = 1;

static bool g_mouseClickDown = false; // is the mouse button pressed
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static bool g_spaceDown = false; // space state, for middle mouse emulation
static double g_mouseClickX, g_mouseClickY; // coordinates for mouse click event
static int g_activeShader = 0;

struct ShaderState {
    GlProgram program;

    // Handles to uniform variables
    GLint h_uLight, h_uLight2;
    GLint h_uProjMatrix;
    GLint h_uModelViewMatrix;
    GLint h_uNormalMatrix;
    GLint h_uColor;

    // Handles to vertex attributes
    GLint h_aPosition;
    GLint h_aNormal;

    ShaderState(const char *vsfn, const char *fsfn) {
        readAndCompileShader(program, vsfn, fsfn);

        const GLuint h = program; // short hand

        // Retrieve handles to uniform variables
        h_uLight = safe_glGetUniformLocation(h, "uLight");
        h_uLight2 = safe_glGetUniformLocation(h, "uLight2");
        h_uProjMatrix = safe_glGetUniformLocation(h, "uProjMatrix");
        h_uModelViewMatrix = safe_glGetUniformLocation(h, "uModelViewMatrix");
        h_uNormalMatrix = safe_glGetUniformLocation(h, "uNormalMatrix");
        h_uColor = safe_glGetUniformLocation(h, "uColor");

        // Retrieve handles to vertex attributes
        h_aPosition = safe_glGetAttribLocation(h, "aPosition");
        h_aNormal = safe_glGetAttribLocation(h, "aNormal");

	glBindFragDataLocation(h, 0, "fragColor");
        checkGlErrors();
    }
};

static const int g_numShaders = 2;
static const char *const g_shaderFiles[g_numShaders][2] = {
    {"./shaders/basic-gl3.vshader", "./shaders/diffuse-gl3.fshader"},
    {"./shaders/basic-gl3.vshader", "./shaders/solid-gl3.fshader"}};
static vector<shared_ptr<ShaderState>>
    g_shaderStates; // our global shader states

// --------- Geometry

// Macro used to obtain relative offset of a field within a struct
#define FIELD_OFFSET(StructType, field) &(((StructType *)0)->field)

// A vertex with floating point position and normal
struct VertexPN {
    Cvec3f p, n;

    VertexPN() {}
    VertexPN(float x, float y, float z, float nx, float ny, float nz)
        : p(x, y, z), n(nx, ny, nz) {}

    // Define copy constructor and assignment operator from GenericVertex so we
    // can use make* functions from geometrymaker.h
    VertexPN(const GenericVertex &v) { *this = v; }

    VertexPN &operator=(const GenericVertex &v) {
        p = v.pos;
        n = v.normal;
        return *this;
    }
};

struct Geometry {
    GlBufferObject vbo, ibo;
    GlArrayObject vao;
    int vboLen, iboLen;

    Geometry(VertexPN *vtx, unsigned short *idx, int vboLen, int iboLen) {
        this->vboLen = vboLen;
        this->iboLen = iboLen;

        // Now create the VBO and IBO
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPN) * vboLen, vtx,
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * iboLen,
                     idx, GL_STATIC_DRAW);
    }

    void draw(const ShaderState &curSS) {
        // bind the object's VAO
        glBindVertexArray(vao);

        // Enable the attributes used by our shader
        safe_glEnableVertexAttribArray(curSS.h_aPosition);
        safe_glEnableVertexAttribArray(curSS.h_aNormal);

        // bind vbo
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        safe_glVertexAttribPointer(curSS.h_aPosition, 3, GL_FLOAT, GL_FALSE,
                                   sizeof(VertexPN), FIELD_OFFSET(VertexPN, p));
        safe_glVertexAttribPointer(curSS.h_aNormal, 3, GL_FLOAT, GL_FALSE,
                                   sizeof(VertexPN), FIELD_OFFSET(VertexPN, n));

        // bind ibo
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

        // draw!
        glDrawElements(GL_TRIANGLES, iboLen, GL_UNSIGNED_SHORT, 0);

        // Disable the attributes used by our shader
        safe_glDisableVertexAttribArray(curSS.h_aPosition);
        safe_glDisableVertexAttribArray(curSS.h_aNormal);

        // disable VAO
        glBindVertexArray((GLuint) NULL);
    }
};

// Vertex buffer and index buffer associated with the ground and cube geometry
static shared_ptr<Geometry> g_ground, g_cube_red, g_cube_blue, g_sphere;

// --------- Scene

static const Cvec3 g_light1(2.0, 3.0, 14.0),
    g_light2(-2, -3.0, -5.0); // define two lights positions in world space
static RigTForm g_skyRbt = RigTForm(Cvec3(0.0, 0.25, 4.0));
static RigTForm g_objectRbt[2] = { RigTForm(Cvec3(-1, 0, 0)),
                                RigTForm(Cvec3(1, 0, 0)) };
static Cvec3f g_objectColors[2] = {Cvec3f(1, 0, 0), Cvec3f(0, 0, 1)};

static int g_activeEyeRbt = 0; // 0 is sky, 1+ is objectRbt[g_activeEyeRbt-1]
static const int g_numEyeRbts = 3;
static bool isOrbit = true;
const char* viewModeNames[] = {"Sky", "Red Cube", "Blue Cube"};

static int g_activeManipulator = 0; // 0 is sky, 1+ is objectRbt[g_activeManipulator-1]
static const int g_numManipulators = 3;
const char* objectNames[] = {"Sky", "Red Cube", "Blue Cube", };
static Cvec2 g_arcballCenter;
static float g_arcballScreenRadius = 0.25 * min(g_windowWidth, g_windowHeight);
static float g_arcballScale = 1.0;

///////////////// END OF G L O B A L S
/////////////////////////////////////////////////////

static void initGround() {
    // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
    VertexPN vtx[4] = {
        VertexPN(-g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
        VertexPN(-g_groundSize, g_groundY, g_groundSize, 0, 1, 0),
        VertexPN(g_groundSize, g_groundY, g_groundSize, 0, 1, 0),
        VertexPN(g_groundSize, g_groundY, -g_groundSize, 0, 1, 0),
    };
    unsigned short idx[] = {0, 1, 2, 0, 2, 3};
    g_ground.reset(new Geometry(&vtx[0], &idx[0], 4, 6));
}

static void initCubes() {
    // cube 1
    int ibLen, vbLen;
    getCubeVbIbLen(vbLen, ibLen);

    // Temporary storage for cube geometry
    vector<VertexPN> vtx1(vbLen);
    vector<unsigned short> idx1(ibLen);

    makeCube(1, vtx1.begin(), idx1.begin());
    g_cube_red.reset(new Geometry(&vtx1[0], &idx1[0], vbLen, ibLen));
    
    // cube 2

    // Temporary storage for cube geometry
    vector<VertexPN> vtx2(vbLen);
    vector<unsigned short> idx2(ibLen);

    makeCube(1, vtx2.begin(), idx2.begin());
    g_cube_blue.reset(new Geometry(&vtx2[0], &idx2[0], vbLen, ibLen));
}
                                    
static void initSpheres() {
    int slices = 12;
    int stacks = 12;
    int radius = 1;
    
    // sphere
    int ibLen, vbLen;
    getSphereVbIbLen(slices, stacks, vbLen, ibLen);

    // Temporary storage for sphere geometry
    vector<VertexPN> vtx1(vbLen);
    vector<unsigned short> idx1(ibLen);

    makeSphere(radius, slices, stacks, vtx1.begin(), idx1.begin());
    g_sphere.reset(new Geometry(&vtx1[0], &idx1[0], vbLen, ibLen));
}

// takes a projection matrix and send to the the shaders
static void sendProjectionMatrix(const ShaderState &curSS,
                                 const Matrix4 &projMatrix) {
    GLfloat glmatrix[16];
    projMatrix.writeToColumnMajorMatrix(glmatrix); // send projection matrix
    safe_glUniformMatrix4fv(curSS.h_uProjMatrix, glmatrix);
}

// takes MVM and its normal matrix to the shaders
static void sendModelViewNormalMatrix(const ShaderState &curSS,
                                      const Matrix4 &MVM, const Matrix4 &NMVM) {
    GLfloat glmatrix[16];
    MVM.writeToColumnMajorMatrix(glmatrix); // send MVM
    safe_glUniformMatrix4fv(curSS.h_uModelViewMatrix, glmatrix);

    NMVM.writeToColumnMajorMatrix(glmatrix); // send NMVM
    safe_glUniformMatrix4fv(curSS.h_uNormalMatrix, glmatrix);
}

// update g_frustFovY from g_frustMinFov, g_windowWidth, and g_windowHeight
static void updateFrustFovY() {
    if (g_windowWidth >= g_windowHeight)
        g_frustFovY = g_frustMinFov;
    else {
        const double RAD_PER_DEG = 0.5 * CS175_PI / 180;
        g_frustFovY = atan2(sin(g_frustMinFov * RAD_PER_DEG) * g_windowHeight /
                                g_windowWidth,
                            cos(g_frustMinFov * RAD_PER_DEG)) /
                      RAD_PER_DEG;
    }
}

static Matrix4 makeProjectionMatrix() {
    return Matrix4::makeProjection(
        g_frustFovY, g_windowWidth / static_cast<double>(g_windowHeight),
        g_frustNear, g_frustFar);
}

static void drawStuff() {
    // short hand for current shader state
    const ShaderState &curSS = *g_shaderStates[g_activeShader];

    // build & send proj. matrix to vshader
    const Matrix4 projmat = makeProjectionMatrix();
    sendProjectionMatrix(curSS, projmat);

    // g_activeEyeRbt cycles, so 0 is sky and 1+ is object
    const RigTForm eyeRbt = g_activeEyeRbt == 0 ? g_skyRbt : g_objectRbt[g_activeEyeRbt - 1];
    const RigTForm invEyeRbt = inv(eyeRbt);

    const Cvec3 eyeLight1 = Cvec3(
        invEyeRbt * Cvec4(g_light1, 1)); // g_light1 position in eye coordinates
    const Cvec3 eyeLight2 = Cvec3(
        invEyeRbt * Cvec4(g_light2, 1)); // g_light2 position in eye coordinates
    safe_glUniform3f(curSS.h_uLight, eyeLight1[0], eyeLight1[1], eyeLight1[2]);
    safe_glUniform3f(curSS.h_uLight2, eyeLight2[0], eyeLight2[1], eyeLight2[2]);

    // draw ground
    // ===========
    //
    const RigTForm groundRbt = RigTForm(); // identity
    Matrix4 MVM = rigTFormToMatrix(invEyeRbt * groundRbt);
    Matrix4 NMVM = normalMatrix(MVM);
    sendModelViewNormalMatrix(curSS, MVM, NMVM);
    safe_glUniform3f(curSS.h_uColor, 0.1, 0.95, 0.1); // set color
    g_ground->draw(curSS);

    // draw cubes
    // ==========
    MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[0]);
    NMVM = normalMatrix(MVM);
    sendModelViewNormalMatrix(curSS, MVM, NMVM);
    safe_glUniform3f(curSS.h_uColor, g_objectColors[0][0], g_objectColors[0][1],
                     g_objectColors[0][2]);
    g_cube_red->draw(curSS);
    
    MVM = rigTFormToMatrix(invEyeRbt * g_objectRbt[1]);
    NMVM = normalMatrix(MVM);
    sendModelViewNormalMatrix(curSS, MVM, NMVM);
    safe_glUniform3f(curSS.h_uColor, g_objectColors[1][0], g_objectColors[1][1],
                     g_objectColors[1][2]);
    g_cube_blue->draw(curSS);
    
    // draw spheres
    // ==========
    if (g_activeEyeRbt == 0 && g_activeManipulator != 0) {
        RigTForm eyeCoords = invEyeRbt * g_objectRbt[g_activeManipulator-1];
        // calculate/update arcball stuff
        g_arcballCenter = getScreenSpaceCoord(eyeCoords.getTranslation(), makeProjectionMatrix(), g_frustNear, g_frustFovY, g_windowWidth, g_windowHeight);
        if (!(g_mouseMClickButton ||
           (g_mouseLClickButton && g_mouseRClickButton) ||
           (g_mouseLClickButton && !g_mouseRClickButton &&
            g_spaceDown))) { // middle or (left and right, or left + space)
                                    // button down?
            g_arcballScale = getScreenToEyeScale(eyeCoords.getTranslation()[2], g_frustFovY, g_windowHeight);
        }
        
        // actual drawing
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        Matrix4 scale = Matrix4() * g_arcballScale * g_arcballScreenRadius;
        scale(3,3) = 1.0;
        MVM = rigTFormToMatrix(eyeCoords) * scale;
        NMVM = normalMatrix(MVM);
        sendModelViewNormalMatrix(curSS, MVM, NMVM);
        safe_glUniform3f(curSS.h_uColor, 0,1,1); // simple color
        // draw it
        g_sphere->draw(curSS);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

static void display() {
    glUseProgram(g_shaderStates[g_activeShader]->program);
    // clear framebuffer color&depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawStuff();

    glfwSwapBuffers(g_window); // show the back buffer (where we rendered stuff)

    checkGlErrors();
}

static void reshape(GLFWwindow * window, const int w, const int h) {
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height); 
    glViewport(0, 0, width, height);
    
    g_windowWidth = w;
    g_windowHeight = h;
    cerr << "Size of window is now " << g_windowWidth << "x" << g_windowHeight << endl;
    updateFrustFovY();
    g_arcballScreenRadius = 0.25 * min(g_windowWidth, g_windowHeight);
}

static Cvec3 getRadiusVector(double x, double y) {
    // calculate z coord of click on sphere
    // (x - cx)^2 + (y - cy)^2 + (z - 0)^2 - r^2 = 0
    // so z^2 = r^2 - (x - cx)^2 - (y - cy)^2
    double zsquared = pow(g_arcballScreenRadius,2) - pow( (x-g_arcballCenter(0)) , 2) - pow( (y-g_arcballCenter(1)) , 2);
    // if click was off the sphere, assume on the sphere edge (aka z=0)
    double z = zsquared > 0 ? sqrt(zsquared) : 0.0;
    // make vector pointing from center of sphere to click-on-sphere
    Cvec3 vec = Cvec3( x-g_arcballCenter(0) , y-g_arcballCenter(1), z );
    return normalize(vec);
}

static void motion(GLFWwindow *window, double x, double y) {
    // only allow manipulation from the skycam
    if (g_activeEyeRbt != 0) return;
    
    const double dx = x - g_mouseClickX;
    const double dy = g_windowHeight - y - 1 - g_mouseClickY;
    
    // if active manip is sky, then fallback to 0.01, otherwise use arcball scale
    double translateScale = g_activeManipulator == 0 ? 0.01 : g_arcballScale;

    RigTForm translation;
    
    // ROTATIONS
    if (g_mouseLClickButton && !g_mouseRClickButton &&
        !g_spaceDown) { // left button down?
        switch (g_activeManipulator) {
            case 0:
                if (isOrbit) {
                    RigTForm worldsky = linFact(g_skyRbt);
                    // o = ama^-1o
                    // y rotate using worldworld aka identity
                    g_skyRbt = RigTForm(Quat::makeYRotation(-dx)) * g_skyRbt;
                    // x rotate using worldsky
                    g_skyRbt = worldsky * RigTForm(Quat::makeXRotation(dy)) * inv(worldsky) * g_skyRbt;
                } else {
                    RigTForm skyworld = transFact(g_skyRbt);
                    // y rotate using skyworld. negate dx for direction fix
                    g_skyRbt = skyworld * RigTForm(Quat::makeYRotation(-dx)) * inv(skyworld) * g_skyRbt;
                    // x rotate using skysky, inv(sky)*sky cancels
                    g_skyRbt = g_skyRbt * RigTForm(Quat::makeXRotation(dy));
                }
                break;
            default:
                // all cubes
                Cvec3 v1 = getRadiusVector(g_mouseClickX, g_mouseClickY);
                Cvec3 v2 = getRadiusVector(x, g_windowHeight - y - 1);
                double theta = acos(dot(v1, v2));
                Cvec3 crossprod = cross(v1,v2);

                // nan shouldn't happen but occasionally does with really fast mouse movements
                // we think it's precision issues
                // CS175_EPS2 is very very small; if there's (basically) no movement, be done
                if (isnan(theta) || theta < CS175_EPS2 || dot(crossprod, crossprod) < CS175_EPS2) break;
                Cvec3 k = normalize(cross(v1, v2));
                
                RigTForm rot = RigTForm( Quat(cos(theta), k * sin(theta)) );
                RigTForm cubesky = transFact(g_objectRbt[g_activeManipulator - 1]) * linFact(g_skyRbt);
                g_objectRbt[g_activeManipulator - 1] = cubesky * rot * inv(cubesky) * g_objectRbt[g_activeManipulator - 1];
                
                break;
        }
    // TRANSLATIONS
    } else if (g_mouseRClickButton &&
               !g_mouseLClickButton) { // right button down?
        translation = RigTForm(Cvec3(dx, dy, 0) * translateScale);
    } else if (g_mouseMClickButton ||
               (g_mouseLClickButton && g_mouseRClickButton) ||
               (g_mouseLClickButton && !g_mouseRClickButton &&
                g_spaceDown)) { // middle or (left and right, or left + space)
                                // button down?
        translation = RigTForm(Cvec3(0, 0, -dy) * translateScale);
    }
    
    // perform the translation
    if (g_activeManipulator == 0) {
        // flip bc we're looking down -z
        translation.setTranslation(translation.getTranslation() * -1);
        g_skyRbt = g_skyRbt * translation;
    } else {
        g_objectRbt[g_activeManipulator - 1] = g_skyRbt * translation * inv(g_skyRbt) * g_objectRbt[g_activeManipulator - 1];
    }
    
    g_mouseClickX = x;
    g_mouseClickY = g_windowHeight - y - 1;
}

static void mouse(GLFWwindow *window, int button, int state, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    g_mouseClickX = x;
    // conversion from window-coordinate-system to OpenGL window-coordinate-system
    g_mouseClickY = g_windowHeight - y - 1;

    g_mouseLClickButton |= (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS);
    g_mouseRClickButton |= (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_PRESS);
    g_mouseMClickButton |= (button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_PRESS);

    g_mouseLClickButton &= !(button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_RELEASE);
    g_mouseRClickButton &= !(button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_RELEASE);
    g_mouseMClickButton &= !(button == GLFW_MOUSE_BUTTON_MIDDLE && state == GLFW_RELEASE);

    g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;
}

static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            exit(0);
        case GLFW_KEY_H:
            cout << " ============== H E L P ==============\n\n"
                 << "h\t\thelp menu\n"
                 << "s\t\tsave screenshot\n"
                 << "f\t\tToggle flat shading on/off.\n"
                 << "o\t\tCycle object to edit\n"
                 << "v\t\tCycle view\n"
                 << "drag left mouse to rotate\n"
                 << endl;
            break;
        case GLFW_KEY_S:
            glFlush();
            writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
            break;
        case GLFW_KEY_F:
            g_activeShader = (g_activeShader + 1) % g_numShaders;
            break;
        case GLFW_KEY_SPACE:
            g_spaceDown = true;
            break;
        case GLFW_KEY_V:
            g_activeEyeRbt = (g_activeEyeRbt + 1) % g_numEyeRbts;
            cout << "View Mode: " << viewModeNames[g_activeEyeRbt] << endl;
            break;
        case GLFW_KEY_O:
            g_activeManipulator = (g_activeManipulator + 1) % g_numManipulators;
            cout << "Manipulated Object: " << objectNames[g_activeManipulator] << endl;
            break;
        case GLFW_KEY_M:
            // toggles between ego and orbit motion
            // only allowed when looking + manipulating the skycam
            if (g_activeEyeRbt == 0 && g_activeManipulator == 0) {
                isOrbit = !isOrbit;
                
                if (isOrbit) {
                    cout << "Manipulation Mode: Ego-Motion Mode" << endl;
                } else {
                    cout << "Manipulation Mode: Orbit Mode" << endl;
                }
            } else {
                cout << "Manipulation mode only alterable when Object and View Mode selected is Sky" << endl;

            }
                
            break;
        }
    } else {
        switch (key) {
        case GLFW_KEY_SPACE:
            g_spaceDown = false;
            break;
        }
    }
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void initGlfwState() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

    g_window = glfwCreateWindow(g_windowWidth, g_windowHeight,
                                "Assignment 4", NULL, NULL);
    if (!g_window) {
        fprintf(stderr, "Failed to create GLFW window or OpenGL context\n");
        exit(1);
    }
    glfwMakeContextCurrent(g_window);

    glfwSwapInterval(1);

    glfwSetErrorCallback(error_callback);
    glfwSetMouseButtonCallback(g_window, mouse);
    glfwSetCursorPosCallback(g_window, motion);
    glfwSetWindowSizeCallback(g_window, reshape);
    glfwSetKeyCallback(g_window, keyboard);

    int screen_width, screen_height;
    glfwGetWindowSize(g_window, &screen_width, &screen_height);
    int pixel_width, pixel_height;
    glfwGetFramebufferSize(g_window, &pixel_width, &pixel_height);

    cout << screen_width << " " << screen_height << endl;
    cout << pixel_width << " " << pixel_width << endl;

    g_wScale = pixel_width / screen_width;
    g_hScale = pixel_height / screen_height;
    
}

static void initGLState() {
    glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);
    glClearDepth(0.);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glReadBuffer(GL_BACK);
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void initShaders() {
    g_shaderStates.resize(g_numShaders);
    for (int i = 0; i < g_numShaders; ++i) {
      g_shaderStates[i].reset(
			      new ShaderState(g_shaderFiles[i][0], g_shaderFiles[i][1]));
    }
}

static void initGeometry() {
    initGround();
    initCubes();
    initSpheres();
}

void glfwLoop() {
    while (!glfwWindowShouldClose(g_window)) {
        display();
        glfwWaitEvents();
    }
    printf("end loop\n");
}

int main(int argc, char *argv[]) {
    try {
        initGlfwState();

        glewInit(); // load the OpenGL extensions
#ifndef __MAC__

        if (!GLEW_VERSION_3_0)
            throw runtime_error("Error: card/driver does not support OpenGL "
                                "Shading Language v1.3");
#endif


        initGLState();
        initShaders();
        initGeometry();

        glfwLoop();
        return 0;
    } catch (const runtime_error &e) {
        cout << "Exception caught: " << e.what() << endl;
        return -1;
    }
}
