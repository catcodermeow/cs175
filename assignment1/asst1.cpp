////////////////////////////////////////////////////////////////////////
//
//   Harvard Computer Science
//   CS 175: Computer Graphics
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

#include "glsupport.h"
#include "ppm.h"

using namespace std; // for string, vector, iostream and other standard C++
                     // stuff

// G L O B A L S ///////////////////////////////////////////////////


static int g_windowWidth = 512;
static int g_windowHeight = 512;
static float g_windowWidthScale = 1;
static float g_windowHeightScale = 1;
static double g_wScale = 1;
static double g_hScale = 1;
static bool g_leftClicked = false;     // is the left mouse button down?
static bool g_rightClicked = false;    // is the right mouse button down?
static float g_squareShift = 0.0; // scale factor for object
static float g_triangleShiftY = 0.0; //shift triangle y-axis
static float g_triangleShiftX = 0.0; //shift triangle x-axis
static float g_blender = 1.0;         // scale factor for object
static double g_leftClickX, g_leftClickY; // coordinates for mouse left click event
static double  g_rightClickX,
    g_rightClickY; // coordinates for mouse right click event

GLFWwindow* g_window;

// our global shader states
struct SquareShaderState {
    GlProgram program;

    // Handles to uniform variables
    GLint h_uSquareShift;
    GLint h_uWindowWidthScale;
    GLint h_uWindowHeightScale;
    GLint h_uBlender;
    GLint h_uTex0, h_uTex1;

    // Handles to vertex attributes
    GLint h_aPosition;
    GLint h_aTexCoord;
};

struct TriangleShaderState {
    GlProgram program;

    // Handles to uniform variables
    GLint h_uWindowWidthScale;
    GLint h_uWindowHeightScale;
    GLint h_uTriangleShiftX;
    GLint h_uTriangleShiftY;
    GLint h_uBlender;
    GLint h_uTex2;

    // Handles to vertex attributes
    GLint h_aPosition;
    GLint h_aTexCoord;
    GLint h_aColor;
};

static shared_ptr<SquareShaderState> g_squareShaderState;

static shared_ptr<TriangleShaderState> g_triangleShaderState;

// our global texture instance
static shared_ptr<GlTexture> g_tex0, g_tex1, g_tex2;


// our global geometries
struct GeometryPX {
    GlArrayObject vao;
    GlBufferObject posVbo, texVbo, colVbo;
};

static shared_ptr<GeometryPX> g_square, g_triangle;

// C A L L B A C K S ///////////////////////////////////////////////////

static void drawSquare() {
    // using a VAO is necessary to run on OS X.
    glBindVertexArray(g_square->vao);

    // activate the glsl program
    glUseProgram(g_squareShaderState->program);

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *g_tex0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, *g_tex1);

    // set glsl uniform variables
    safe_glUniform1i(g_squareShaderState->h_uTex0, 0); // 0 means GL_TEXTURE0
    safe_glUniform1i(g_squareShaderState->h_uTex1, 1); // 1 means GL_TEXTURE1
    safe_glUniform1f(g_squareShaderState->h_uBlender, g_blender);
    safe_glUniform1f(g_squareShaderState->h_uSquareShift, g_squareShift);
    safe_glUniform1f(g_squareShaderState->h_uWindowWidthScale, g_windowWidthScale);
    safe_glUniform1f(g_squareShaderState->h_uWindowHeightScale, g_windowHeightScale);


    // bind vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, g_square->posVbo);
    safe_glVertexAttribPointer(g_squareShaderState->h_aPosition, 2, GL_FLOAT,
                               GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, g_square->texVbo);
    safe_glVertexAttribPointer(g_squareShaderState->h_aTexCoord, 2, GL_FLOAT,
                               GL_FALSE, 0, 0);

    safe_glEnableVertexAttribArray(g_squareShaderState->h_aPosition);
    safe_glEnableVertexAttribArray(g_squareShaderState->h_aTexCoord);

    // draw using 6 vertices, forming two triangles
    glDrawArrays(GL_TRIANGLES, 0, 6);

    safe_glDisableVertexAttribArray(g_squareShaderState->h_aPosition);
    safe_glDisableVertexAttribArray(g_squareShaderState->h_aTexCoord);

    glBindVertexArray(0);

    // check for errors
    checkGlErrors();
}

static void drawTriangle() {
    // using a VAO is necessary to run on OS X.
    glBindVertexArray(g_triangle->vao);

    // activate the glsl program
    glUseProgram(g_triangleShaderState->program);

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *g_tex2);

    // set glsl uniform variables
    safe_glUniform1i(g_triangleShaderState->h_uTex2, 2); // 0 means GL_TEXTURE0
    safe_glUniform1f(g_triangleShaderState->h_uBlender, g_blender);
    safe_glUniform1f(g_triangleShaderState->h_uWindowWidthScale, g_windowWidthScale);
    safe_glUniform1f(g_triangleShaderState->h_uWindowHeightScale, g_windowHeightScale);
    safe_glUniform1f(g_triangleShaderState->h_uTriangleShiftX, g_triangleShiftX);
    safe_glUniform1f(g_triangleShaderState->h_uTriangleShiftY, g_triangleShiftY);

    // bind vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, g_triangle->posVbo);
    safe_glVertexAttribPointer(g_triangleShaderState->h_aPosition, 2, GL_FLOAT,
                               GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, g_triangle->texVbo);
    safe_glVertexAttribPointer(g_triangleShaderState->h_aTexCoord, 2, GL_FLOAT,
                               GL_FALSE, 0, 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, g_triangle->colVbo);
    safe_glVertexAttribPointer(g_triangleShaderState->h_aColor, 3, GL_FLOAT,
                               GL_FALSE, 0, 0);

    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aPosition);
    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
    safe_glEnableVertexAttribArray(g_triangleShaderState->h_aColor);

    // draw using 3 vertices, forming one triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aPosition);
    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
    safe_glDisableVertexAttribArray(g_triangleShaderState->h_aColor);

    glBindVertexArray(0);

    // check for errors
    checkGlErrors();
}

// _____________________________________________________
//|                                                     |
//|  display                                            |
//|_____________________________________________________|
///
///  Whenever OpenGL requires a screen refresh
///  we will call display() to draw the scene.

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawSquare();
    
    drawTriangle();

    glfwSwapBuffers(g_window);

    // check for errors
    checkGlErrors();
}

// _____________________________________________________
//|                                                     |
//|  reshape                                            |
//|_____________________________________________________|
///
///  Whenever a window is resized, a "resize" event is
///  generated and GLFW is told to call this reshape
///  callback function to handle it appropriately.


static void reshape(GLFWwindow * window, const int w, const int h) {
    int width, height;
    glfwGetFramebufferSize(g_window, &width, &height); 
    glViewport(0, 0, width, height);
    
    g_windowWidth = w;
    g_windowHeight = h;
    if (w > h) {
        g_windowWidthScale = (float) h / w;
        g_windowHeightScale = 1;
    } else {
        g_windowWidthScale = 1;
        g_windowHeightScale = (float) w / h;
    }
    cerr << "Size of window is now " << g_windowWidth << "x" << g_windowHeight << endl;
}

// _____________________________________________________
//|                                                     |
//|  mouse                                              |
//|_____________________________________________________|
///
///  Whenever a mouse button is clicked, a "mouse" event
///  is generated and this mouse callback function is
///  called to handle the user input.

static void mouse(GLFWwindow *window, int button, int state, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (state == GLFW_PRESS) {
            // right mouse button has been clicked
            g_leftClicked = true;
            g_leftClickX = x;
            g_leftClickY = g_windowHeight - y - 1;
        } else {
            // right mouse button has been released
            g_leftClicked = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (state == GLFW_PRESS) {
            // right mouse button has been clicked
            g_rightClicked = true;
            g_rightClickX = x;
            g_rightClickY = g_windowHeight - y - 1;
        } else {
            // right mouse button has been released
            g_rightClicked = false;
        }
    }
}

// _____________________________________________________
//|                                                     |
//|  motion                                             |
//|_____________________________________________________|
///
///  Whenever the mouse is moved while a button is pressed,
///  a "mouse move" event is triggered and this callback is
///  called to handle the event.

static void motion(GLFWwindow *window, double x, double y) {
    const double newx = x;
    const double newy = g_windowHeight - y - 1;
    if (g_leftClicked) {
        g_leftClickX = newx;
        g_leftClickY = newy;
    }
    if (g_rightClicked) {
        float deltax = (newx - g_rightClickX) * 0.02;
        g_squareShift += deltax;
        g_blender += deltax;

        g_rightClickX = newx;
        g_rightClickY = newy;
    }
}

static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_H:
            cout << " ============== H E L P ==============\n\n"
                 << "h\t\thelp menu\n"
                 << "s\t\tsave screenshot\n"
                 << "drag right mouse to change square size\n";
            break;
        case GLFW_KEY_Q:
            exit(0);
        case GLFW_KEY_S:
            glFinish();
            writePpmScreenshot(g_windowWidth, g_windowHeight, "out.ppm");
            break;
        case GLFW_KEY_I:
            g_triangleShiftY += .1;
            break;
        case GLFW_KEY_K:
            g_triangleShiftY -= .1;
            break;
        case GLFW_KEY_J:
            g_triangleShiftX -= .1;
            break;
        case GLFW_KEY_L:
            g_triangleShiftX += .1;
            break;
        }
    }
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

// H E L P E R    F U N C T I O N S ////////////////////////////////////
static void initGlfwState() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

    g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, "Assignment 1", NULL, NULL);
    if (!g_window) {
        fprintf(stderr, "Failed to create GLFW window or OpenGL context\n");
        exit(1);
    }
    glfwMakeContextCurrent(g_window);

    glewInit();



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
    glClearColor(128. / 255, 200. / 255, 1, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glEnable(GL_FRAMEBUFFER_SRGB);
}

static void loadSquareShader(SquareShaderState &ss) {
    const GLuint h = ss.program; // short hand

    readAndCompileShader(ss.program, "shaders/asst1-sq-gl3.vshader",
			 "shaders/asst1-sq-gl3.fshader");
        
    // Retrieve handles to uniform variables
    ss.h_uSquareShift = safe_glGetUniformLocation(h, "uSquareShift");
    ss.h_uWindowWidthScale = safe_glGetUniformLocation(h, "uWindowWidthScale");
    ss.h_uWindowHeightScale = safe_glGetUniformLocation(h, "uWindowHeightScale");
    ss.h_uBlender = safe_glGetUniformLocation(h, "uBlender");
    ss.h_uTex0 = safe_glGetUniformLocation(h, "uTex0");
    ss.h_uTex1 = safe_glGetUniformLocation(h, "uTex1");

    // Retrieve handles to vertex attributes
    ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");

    glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
}

static void loadTriangleShader(TriangleShaderState &ss) {
    const GLuint h = ss.program; // short hand

    readAndCompileShader(ss.program, "shaders/asst1-tr-gl3.vshader",
             "shaders/asst1-tr-gl3.fshader");
        
    // Retrieve handles to uniform variables
    ss.h_uTriangleShiftX = safe_glGetUniformLocation(h, "uTriangleShiftX");
    ss.h_uTriangleShiftY = safe_glGetUniformLocation(h, "uTriangleShiftY");

    ss.h_uWindowWidthScale = safe_glGetUniformLocation(h, "uWindowWidthScale");
    ss.h_uWindowHeightScale = safe_glGetUniformLocation(h, "uWindowHeightScale");
    ss.h_uBlender = safe_glGetUniformLocation(h, "uBlender");
    ss.h_uTex2 = safe_glGetUniformLocation(h, "uTex2");

    // Retrieve handles to vertex attributes
    ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
    ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");
    ss.h_aColor = safe_glGetAttribLocation(h, "aColor");

    glBindFragDataLocation(h, 0, "fragColor");
    checkGlErrors();
}

static void initShaders() {
    g_squareShaderState.reset(new SquareShaderState);
    g_triangleShaderState.reset(new TriangleShaderState);
    
    loadSquareShader(*g_squareShaderState);
    loadTriangleShader(*g_triangleShaderState);
}

static void loadSquareGeometry(const GeometryPX &g) {
    GLfloat pos[12] = {-.5, -.5, .5,  .5, .5, -.5,
                       -.5, -.5, -.5, .5, .5, .5};

    GLfloat tex[12] = {0, 0, 1, 1, 1, 0,
                       0, 0, 0, 1, 1, 1};

    glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), pos, GL_STATIC_DRAW);
    checkGlErrors();

    glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), tex, GL_STATIC_DRAW);
    checkGlErrors();
}

static void loadTriangleGeometry(const GeometryPX &g) {
    GLfloat pos[6] = {-.5, 0.25, .5, .25, 0, -.5};
    GLfloat tex[6] = {-0.3, 1, 1.3, 1., 0.5, -0.2};
    GLfloat col[9] = {0.5, 0.5, 1.,   0.5, 1., 0.5,   1., 0.5, 0.5};

    glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), pos, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(GLfloat), tex, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, g.colVbo);
    glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), col, GL_STATIC_DRAW);
    checkGlErrors();
}

static void initGeometry() {
    g_square.reset(new GeometryPX());
    loadSquareGeometry(*g_square);
    
    g_triangle.reset(new GeometryPX());
    loadTriangleGeometry(*g_triangle);
}

static void loadTexture(GLuint texHandle, const char *ppmFilename) {
    int texWidth, texHeight;
    vector<PackedPixel> pixData;

    ppmRead(ppmFilename, texWidth, texHeight, pixData);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0,  GL_SRGB, texWidth,
                 texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    checkGlErrors();
}

static void initTextures() {
    g_tex0.reset(new GlTexture());
    g_tex1.reset(new GlTexture());
    g_tex2.reset(new GlTexture());

    loadTexture(*g_tex0, "smiley.ppm");
    loadTexture(*g_tex1, "reachup.ppm");
    loadTexture(*g_tex2, "shield.ppm");

}

// Main loop for our program
static void glfwLoop() {
    while (!glfwWindowShouldClose(g_window)) {
        // call our draw code
        display();
        // wait until the user does something
        glfwWaitEvents();
    }
}

// M A I N /////////////////////////////////////////////////////////////

// _____________________________________________________
//|                                                     |
//|  main                                               |
//|_____________________________________________________|
///
///  The main entry-point for the HelloWorld example
///  application.

int main(int argc, char **argv) {
    try {
        initGlfwState();



        initGLState();
        initShaders();
        initGeometry();
        initTextures();

        glfwLoop();
        return 0;
    } catch (const runtime_error &e) {
        cout << "Exception caught: " << e.what() << endl;
        return -1;
    }
}
