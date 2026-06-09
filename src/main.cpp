#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// SHADERS COM ILUMINACAO PHONG
// ─────────────────────────────────────────────────────────────────────────────
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos     = vec3(model * vec4(aPos, 1.0));
    Normal      = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

void main() {
    // Componente Ambiente
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Componente Difusa
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * lightColor;

    // Componente Especular
    float specularStrength = 0.5;
    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular   = specularStrength * spec * lightColor;

    // Cor final = (ambiente + difusa + especular) * cor do objecto
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor   = vec4(result, 1.0);
}
)";

// ─────────────────────────────────────────────────────────────────────────────
// GEOMETRIA — cubo com normais por face
// Cada face tem a sua normal (perpendicular a ela) para iluminacao correcta
// ─────────────────────────────────────────────────────────────────────────────
float vertices[] = {
    // posicao           // normal
    // Face TRAS (normal aponta para -Z)
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,
    // Face FRENTE (normal aponta para +Z)
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
    // Face ESQUERDA (normal aponta para -X)
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,
    -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,
    // Face DIREITA (normal aponta para +X)
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,
    // Face BASE (normal aponta para -Y)
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
     0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
    -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,
    // Face TOPO (normal aponta para +Y)
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,
};

unsigned int indices[] = {
     0, 1, 2,  2, 3, 0,   // tras
     4, 5, 6,  6, 7, 4,   // frente
     8, 9,10, 10,11, 8,   // esquerda
    12,13,14, 14,15,12,   // direita
    16,17,18, 18,19,16,   // base
    20,21,22, 22,23,20,   // topo
};

// ─────────────────────────────────────────────────────────────────────────────
// KEYFRAME SYSTEM
// ─────────────────────────────────────────────────────────────────────────────
struct Keyframe {
    float time, ombro, cotovelo, pulso;
};

float lerp(float a, float b, float t) { return a + t * (b - a); }

Keyframe interpolateKeyframes(const std::vector<Keyframe>& kfs, float ct) {
    if (ct <= kfs.front().time) return kfs.front();
    if (ct >= kfs.back().time)  return kfs.back();
    for (size_t i = 0; i < kfs.size()-1; i++) {
        if (ct >= kfs[i].time && ct <= kfs[i+1].time) {
            float t = (ct - kfs[i].time) / (kfs[i+1].time - kfs[i].time);
            Keyframe r;
            r.time     = ct;
            r.ombro    = lerp(kfs[i].ombro,    kfs[i+1].ombro,    t);
            r.cotovelo = lerp(kfs[i].cotovelo, kfs[i+1].cotovelo, t);
            r.pulso    = lerp(kfs[i].pulso,    kfs[i+1].pulso,    t);
            return r;
        }
    }
    return kfs.back();
}

// ─────────────────────────────────────────────────────────────────────────────
// FISICA
// ─────────────────────────────────────────────────────────────────────────────
struct Ball { float x,y,vx,vy,radius,restitution,gravity; };

void updateBall(Ball& b, float dt) {
    b.vy += b.gravity * dt;
    b.x  += b.vx * dt;
    b.y  += b.vy * dt;
    if (b.y <= b.radius) {
        b.y   = b.radius;
        b.vy  = -b.vy * b.restitution;
        if (fabsf(b.vy) < 0.1f) b.vy = 0.0f;
    }
    if (b.x > 3.0f || b.x < -3.0f) {
        b.vx = -b.vx;
        b.x  = glm::clamp(b.x, -3.0f, 3.0f);
    }
}

void resetBall(Ball& b) { b.x=0; b.y=4; b.vx=1.5f; b.vy=0; }

// ─────────────────────────────────────────────────────────────────────────────
// CAMERA COM RATO
// ─────────────────────────────────────────────────────────────────────────────
float camYaw   = -90.0f;
float camPitch =  -20.0f;
float lastMouseX, lastMouseY;
bool  firstMouse = true;
bool  mouseActive = false;

void mouseCallback(GLFWwindow* w, double xpos, double ypos) {
    if (!mouseActive) return;
    if (firstMouse) { lastMouseX=(float)xpos; lastMouseY=(float)ypos; firstMouse=false; }
    float dx = ((float)xpos - lastMouseX) * 0.2f;
    float dy = (lastMouseY - (float)ypos) * 0.2f;
    lastMouseX=(float)xpos; lastMouseY=(float)ypos;
    camYaw   += dx;
    camPitch  = glm::clamp(camPitch + dy, -89.0f, 89.0f);
}

void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        mouseActive = (action == GLFW_PRESS);
        firstMouse  = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS DE RENDER
// ─────────────────────────────────────────────────────────────────────────────
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(s,512,nullptr,l); std::cerr<<l<<"\n"; }
    return s;
}

void setUniforms(GLuint prog,
                 const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
                 const glm::vec3& color,
                 const glm::vec3& lightPos, const glm::vec3& viewPos) {
    glUniformMatrix4fv(glGetUniformLocation(prog,"model"),      1,GL_FALSE,glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(prog,"view"),       1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(prog,"projection"), 1,GL_FALSE,glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(prog,"objectColor"), 1,glm::value_ptr(color));
    glUniform3fv(glGetUniformLocation(prog,"lightPos"),    1,glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(prog,"viewPos"),     1,glm::value_ptr(viewPos));
    glUniform3f(glGetUniformLocation(prog,"lightColor"),   1.0f,1.0f,1.0f);
}

void drawBox(GLuint prog, GLuint VAO,
             const glm::mat4& view, const glm::mat4& proj,
             glm::vec3 pos, glm::vec3 scale, glm::vec3 color,
             const glm::vec3& lp, const glm::vec3& vp) {
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1.0f), pos), scale);
    setUniforms(prog, m, view, proj, color, lp, vp);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void drawSegment(GLuint prog, GLuint VAO,
                 const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
                 float sx, float sy, float sz, glm::vec3 color,
                 const glm::vec3& lp, const glm::vec3& vp) {
    glm::mat4 m = glm::scale(model, glm::vec3(sx,sy,sz));
    setUniforms(prog, m, view, proj, color, lp, vp);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    if (!glfwInit()) { std::cerr<<"GLFW falhou\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(900,600,"Animacao com Iluminacao Phong",nullptr,nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    glfwSetCursorPosCallback(window,   mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr<<"GLEW falhou\n"; return -1; }
    glGetError();
    glEnable(GL_DEPTH_TEST);

    std::cerr << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    // Shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);

    // VAO/VBO/EBO
    GLuint VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    // posicao (location 0)
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    // normal (location 1)
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Keyframes
    std::vector<Keyframe> kfs = {
        {0.0f,  0.0f,   0.0f,  0.0f},
        {2.0f, 45.0f, -60.0f, 30.0f},
        {4.0f, 90.0f,-120.0f, 60.0f},
        {6.0f, 45.0f, -60.0f,-30.0f},
        {8.0f,  0.0f,   0.0f,  0.0f},
    };
    float animTime = 0.0f;
    float animDur  = kfs.back().time;

    // Bola
    Ball ball; ball.radius=0.3f; ball.restitution=0.75f; ball.gravity=-9.8f;
    resetBall(ball);

    // Estado
    int  mode   = 1;
    bool paused = false;
    float procTime = 0.0f;

    // Luz
    glm::vec3 lightPos(3.0f, 5.0f, 3.0f);

    double lastTime = glfwGetTime();
    double fpsTimer = lastTime;
    int    frames   = 0;

    std::cerr << "Teclas: 1=Braco  2=Bola Fisica  3=Senoidal\n";
    std::cerr << "        ESPACO=pause  R=reset  ESC=sair\n";
    std::cerr << "Rato:   Botao direito arrastado = rodar camara\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now   = glfwGetTime();
        float  delta = (float)(now - lastTime);
        lastTime = now;

        // FPS
        frames++;
        if (now - fpsTimer >= 1.0) {
            const char* mn = mode==1?"Braco Robotico":mode==2?"Bola Fisica":"Senoidal";
            char t[128];
            snprintf(t,sizeof(t),"Animacao Phong [%d FPS] | %s | %s",
                frames, mn, paused?"PAUSADO":"A CORRER");
            glfwSetWindowTitle(window,t);
            frames=0; fpsTimer=now;
        }

        // Input
        if (glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(window,true);
        if (glfwGetKey(window,GLFW_KEY_1)==GLFW_PRESS) mode=1;
        if (glfwGetKey(window,GLFW_KEY_2)==GLFW_PRESS) { mode=2; resetBall(ball); }
        if (glfwGetKey(window,GLFW_KEY_3)==GLFW_PRESS) { mode=3; procTime=0.0f; }
        if (glfwGetKey(window,GLFW_KEY_R)==GLFW_PRESS) {
            animTime=0; resetBall(ball); procTime=0;
        }

        static bool spaceWas=false;
        bool spaceNow=glfwGetKey(window,GLFW_KEY_SPACE)==GLFW_PRESS;
        if (spaceNow&&!spaceWas) paused=!paused;
        spaceWas=spaceNow;

        // Camara a partir do yaw/pitch do rato
        glm::vec3 camDir;
        camDir.x = cosf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
        camDir.y = sinf(glm::radians(camPitch));
        camDir.z = sinf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
        camDir   = glm::normalize(camDir);

        glm::vec3 camPos = glm::vec3(0.0f, 3.0f, 0.0f) - camDir * 8.0f;
        glm::vec3 camTarget = glm::vec3(0.0f, 2.0f, 0.0f);

        glm::mat4 view = glm::lookAt(camPos, camTarget, glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f),900.0f/600.0f,0.1f,100.0f);

        // Actualizar animacao
        if (!paused) {
            if (mode==1) { animTime+=delta; if(animTime>animDur) animTime=0.0f; }
            if (mode==2) updateBall(ball,delta);
            if (mode==3) procTime+=delta;
        }

        // Render
        glClearColor(0.08f,0.08f,0.12f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(prog);

        // ── MODO 1: Braco Robotico ────────────────────────────────────────────
        if (mode==1) {
            Keyframe kf = interpolateKeyframes(kfs,animTime);
            float aO = glm::radians(kf.ombro);
            float aC = glm::radians(kf.cotovelo);
            float aP = glm::radians(kf.pulso);

            glm::mat4 base = glm::mat4(1.0f);
            drawSegment(prog,VAO,base,view,proj,1.0f,0.3f,1.0f,
                        glm::vec3(0.4f,0.4f,0.4f),lightPos,camPos);

            glm::mat4 s1 = glm::translate(glm::mat4(1.0f),glm::vec3(0,0.3f,0));
            s1 = glm::rotate(s1,aO,glm::vec3(0,0,1));
            drawSegment(prog,VAO,s1,view,proj,0.3f,1.2f,0.3f,
                        glm::vec3(0.8f,0.3f,0.1f),lightPos,camPos);

            glm::mat4 s2 = glm::translate(s1,glm::vec3(0,1.2f,0));
            s2 = glm::rotate(s2,aC,glm::vec3(0,0,1));
            drawSegment(prog,VAO,s2,view,proj,0.25f,1.0f,0.25f,
                        glm::vec3(0.2f,0.6f,0.9f),lightPos,camPos);

            glm::mat4 s3 = glm::translate(s2,glm::vec3(0,1.0f,0));
            s3 = glm::rotate(s3,aP,glm::vec3(0,0,1));
            drawSegment(prog,VAO,s3,view,proj,0.2f,0.7f,0.2f,
                        glm::vec3(0.2f,0.9f,0.3f),lightPos,camPos);
        }

        // ── MODO 2: Bola Fisica ───────────────────────────────────────────────
        else if (mode==2) {
            drawBox(prog,VAO,view,proj,
                    glm::vec3(0,-0.05f,0), glm::vec3(8,0.1f,1),
                    glm::vec3(0.3f,0.3f,0.3f),lightPos,camPos);

            drawBox(prog,VAO,view,proj,
                    glm::vec3(ball.x, ball.y, 0),
                    glm::vec3(ball.radius*2),
                    glm::vec3(0.9f,0.4f,0.1f),lightPos,camPos);
        }

        // ── MODO 3: Senoidal ──────────────────────────────────────────────────
        else if (mode==3) {
            drawBox(prog,VAO,view,proj,
                    glm::vec3(0,-0.05f,0), glm::vec3(8,0.1f,1),
                    glm::vec3(0.3f,0.3f,0.3f),lightPos,camPos);

            float freq   = 1.5f;
            float px     = 2.5f * sinf(procTime);
            float py     = 1.5f + 1.5f * fabsf(sinf(freq*procTime));
            float squash = 0.3f + 0.7f * fabsf(sinf(freq*procTime));

            drawBox(prog,VAO,view,proj,
                    glm::vec3(px,py,0),
                    glm::vec3(0.6f/squash, 0.6f*squash, 0.6f),
                    glm::vec3(0.2f,0.6f,0.9f),lightPos,camPos);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}