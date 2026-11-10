// ==========================================================================
//  PROYECTO FINAL - ARCADE (adelanto)
//  Contiene: habitación con texturas, tres máquinas arcade .glb
//  y cámara en primera persona.
//
//  Crédito del modelo 3D: "arcade machine" por Logotyp (Sketchfab),
//  licencia CC-BY-4.0 -> https://sketchfab.com/3d-models/arcade-machine-a914bf3651e3496ea708dc0782eca212
// ==========================================================================

#include <iostream>              // Permite mostrar mensajes en la consola
#include <vector>                // Listas de tamaño variable (std::vector)
#include <string>
#include <glad/glad.h>           // Funciones de OpenGL (siempre antes de GLFW)
#include <GLFW/glfw3.h>          // Crea la ventana, y maneja el teclado y el mouse

#define GLM_ENABLE_EXPERIMENTAL  // debe ir ANTES de cualquier include de GLM
#include <glm/glm.hpp>                       // Vectores y matrices (vec3, mat4)
#include <glm/gtc/matrix_transform.hpp>      // translate, rotate, scale, lookAt, perspective
#include <glm/gtc/type_ptr.hpp>              // value_ptr: pasar matrices de glm a OpenGL
#include <glm/gtc/quaternion.hpp>            // quat y mat4_cast (rotaciones de glTF)

// ---- Librería para cargar el modelo (tinygltf, versión 2.9.x) ----
// Estas líneas se escriben UNA sola vez en todo el proyecto, justo antes del include:
#define TINYGLTF_IMPLEMENTATION          // activa el código real del cargador
#define STB_IMAGE_IMPLEMENTATION         // activa el código que decodifica las imágenes (png, jpg)
#define TINYGLTF_NO_STB_IMAGE_WRITE      // no necesitamos guardar imágenes
#include "tiny_gltf.h"

// ==========================================================================
//  VARIABLES GLOBALES DE LA CÁMARA (las usan el render loop y el mouse)
// ==========================================================================
float anchoCuarto = 10.0f;   // tamaño de la habitación: TODO se calcula a partir de estos 3 números
float largoCuarto = 10.0f;
float altoCuarto = 3.6f;
float alturaOjos = 1.6f;     // la cámara siempre se queda a esta altura (como si caminaras)

glm::vec3 cameraPos = glm::vec3(0.0f, 1.6f, 4.0f);     // dónde está la cámara
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // hacia dónde mira
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);      // vector "arriba"

float yaw = -90.0f;     // giro horizontal (-90 = mirando hacia -Z)
float pitch = 0.0f;     // giro vertical
float ultimoX = 500.0f;
float ultimoY = 350.0f;
bool primeraVezMouse = true;

// ==========================================================================
//  FUNCIONES DE APOYO
// ==========================================================================

// GLFW la llama cuando se cambia el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// GLFW la llama cada vez que se mueve el mouse (o el touchpad)
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (primeraVezMouse)
    {
        ultimoX = (float)xpos;
        ultimoY = (float)ypos;
        primeraVezMouse = false;
    }

    float xoffset = (float)xpos - ultimoX;
    float yoffset = ultimoY - (float)ypos;   // invertido: en pantalla Y crece hacia abajo
    ultimoX = (float)xpos;
    ultimoY = (float)ypos;

    float sensibilidad = 0.1f;
    yaw += xoffset * sensibilidad;
    pitch += yoffset * sensibilidad;

    if (pitch > 89.0f) pitch = 89.0f;        // para que la cámara no se voltee de cabeza
    if (pitch < -89.0f) pitch = -89.0f;

    // Con los dos ángulos se calcula hacia dónde mira la cámara (trigonometría)
    glm::vec3 direccion;
    direccion.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direccion.y = sin(glm::radians(pitch));
    direccion.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direccion);
}

// Revisa si un shader compiló bien; si no, imprime el error en la consola.
void revisarShader(GLuint shader, const char* nombre)
{
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, NULL, log);
        std::cout << "Error compilando " << nombre << ":\n" << log << std::endl;
    }
}

// Sube una imagen (ya cargada en memoria) a la GPU y devuelve el número (ID) de la textura
GLuint subirTextura(const unsigned char* pixeles, int ancho, int alto, int canales)
{
    GLenum formato = GL_RGB;
    if (canales == 1) formato = GL_RED;
    else if (canales == 2) formato = GL_RG;
    else if (canales == 3) formato = GL_RGB;
    else if (canales == 4) formato = GL_RGBA;

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // evita errores con imágenes cuyo ancho no es múltiplo de 4
    glTexImage2D(GL_TEXTURE_2D, 0, formato, ancho, alto, 0, formato, GL_UNSIGNED_BYTE, pixeles);
    glGenerateMipmap(GL_TEXTURE_2D);         // versiones más pequeñas, para cuando se ve de lejos

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // si u,v pasan de 1, la imagen se repite
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return id;
}

// Carga un archivo de imagen del disco (jpg/png) y lo sube a la GPU
GLuint cargarTextura(const char* ruta)
{
    int ancho, alto, canales;
    unsigned char* datos = stbi_load(ruta, &ancho, &alto, &canales, 0);
    if (!datos)
    {
        std::cout << "No se pudo cargar la imagen: " << ruta << std::endl;
        return 0;
    }
    GLuint id = subirTextura(datos, ancho, alto, canales);
    stbi_image_free(datos);   // ya está en la GPU, se libera de la RAM
    return id;
}

// ==========================================================================
//  CARGA DEL MODELO .GLB
//  Un modelo glTF está hecho de "nodos" (que tienen una matriz de posición/rotación/escala)
//  y cada nodo puede tener una malla (mesh) formada por una o más "primitivas" (triángulos).
//  Aquí cada primitiva se convierte en una "Pieza" con su propio VAO/VBO/EBO y su material.
// ==========================================================================
struct Pieza
{
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei numIndices = 0;
    glm::mat4 matriz = glm::mat4(1.0f);       // posición de esta pieza dentro del modelo
    GLuint texColor = 0;                      // textura de color (0 = no tiene)
    GLuint texEmisiva = 0;                    // textura que "brilla" (pantalla, neón) (0 = no tiene)
    glm::vec4 colorBase = glm::vec4(1.0f);    // color base del material (se multiplica con la textura)
    bool transparente = false;                // true para vidrios
};

struct ModeloCargado
{
    std::vector<Pieza> piezas;
    glm::mat4 normalizacion = glm::mat4(1.0f);
};

std::vector<GLuint> cacheTexturas;   // evita subir dos veces la misma imagen del modelo
std::vector<GLuint> texturasCargadas; // guarda las texturas de todos los modelos para liberarlas al final

// Devuelve la textura de GPU para la textura número "idxTextura" del glTF (la crea si hace falta)
GLuint texturaDelModelo(const tinygltf::Model& model, int idxTextura)
{
    if (idxTextura < 0) return 0;
    int idxImagen = model.textures[idxTextura].source;
    if (idxImagen < 0) return 0;
    if (cacheTexturas.size() < model.images.size())
        cacheTexturas.resize(model.images.size(), 0);
    if (cacheTexturas[idxImagen] == 0)
    {
        const tinygltf::Image& img = model.images[idxImagen];
        cacheTexturas[idxImagen] = subirTextura(img.image.data(), img.width, img.height, img.component);
        texturasCargadas.push_back(cacheTexturas[idxImagen]);
    }
    return cacheTexturas[idxImagen];
}

// Matriz de un nodo: glTF la entrega como "matrix" o como traslación + rotación + escala
glm::mat4 matrizDelNodo(const tinygltf::Node& nodo)
{
    if (nodo.matrix.size() == 16)
    {
        glm::mat4 m(1.0f);
        for (int i = 0; i < 16; i++) m[i / 4][i % 4] = (float)nodo.matrix[i];
        return m;
    }
    glm::mat4 m(1.0f);
    if (nodo.translation.size() == 3)
        m = glm::translate(m, glm::vec3((float)nodo.translation[0], (float)nodo.translation[1], (float)nodo.translation[2]));
    if (nodo.rotation.size() == 4)
        m = m * glm::mat4_cast(glm::quat((float)nodo.rotation[3], (float)nodo.rotation[0], (float)nodo.rotation[1], (float)nodo.rotation[2]));
    if (nodo.scale.size() == 3)
        m = glm::scale(m, glm::vec3((float)nodo.scale[0], (float)nodo.scale[1], (float)nodo.scale[2]));
    return m;
}

// Devuelve un puntero al inicio de los datos de un "accessor" (posiciones, normales, uv...)
const unsigned char* datosAccessor(const tinygltf::Model& model, int idxAccessor, int& stride)
{
    const tinygltf::Accessor& acc = model.accessors[idxAccessor];
    const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = model.buffers[bv.buffer];
    stride = acc.ByteStride(bv);   // cuántos bytes hay de un vértice al siguiente
    return &buf.data[bv.byteOffset + acc.byteOffset];
}

// Recorre un nodo (y sus hijos) creando una Pieza por cada primitiva de triángulos que encuentre
void procesarNodo(const tinygltf::Model& model, int idxNodo, const glm::mat4& matrizPadre,
    std::vector<Pieza>& piezas, glm::vec3& bbMin, glm::vec3& bbMax)
{
    const tinygltf::Node& nodo = model.nodes[idxNodo];
    glm::mat4 matrizMundo = matrizPadre * matrizDelNodo(nodo);   // la del padre * la propia

    if (nodo.mesh >= 0)
    {
        const tinygltf::Mesh& malla = model.meshes[nodo.mesh];
        for (const tinygltf::Primitive& prim : malla.primitives)
        {
            if (prim.mode != TINYGLTF_MODE_TRIANGLES) continue;
            if (prim.attributes.find("POSITION") == prim.attributes.end()) continue;

            // ---- 1) Leer posiciones, normales y coordenadas de textura ----
            int stridePos, strideNor = 0, strideUv = 0;
            int accPos = prim.attributes.at("POSITION");
            const unsigned char* pPos = datosAccessor(model, accPos, stridePos);
            size_t numVertices = model.accessors[accPos].count;

            const unsigned char* pNor = nullptr;
            if (prim.attributes.count("NORMAL"))
                pNor = datosAccessor(model, prim.attributes.at("NORMAL"), strideNor);
            const unsigned char* pUv = nullptr;
            if (prim.attributes.count("TEXCOORD_0"))
                pUv = datosAccessor(model, prim.attributes.at("TEXCOORD_0"), strideUv);

            // Cada vértice queda como 8 floats: posición(3) + normal(3) + uv(2)
            std::vector<float> vertices;
            vertices.reserve(numVertices * 8);
            for (size_t i = 0; i < numVertices; i++)
            {
                const float* p = (const float*)(pPos + i * stridePos);
                vertices.push_back(p[0]); vertices.push_back(p[1]); vertices.push_back(p[2]);

                if (pNor) {
                    const float* n = (const float*)(pNor + i * strideNor);
                    vertices.push_back(n[0]); vertices.push_back(n[1]); vertices.push_back(n[2]);
                }
                else { vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f); }

                if (pUv) {
                    const float* t = (const float*)(pUv + i * strideUv);
                    vertices.push_back(t[0]); vertices.push_back(t[1]);
                }
                else { vertices.push_back(0.0f); vertices.push_back(0.0f); }

                // Para calcular el tamaño total del modelo (caja que lo envuelve)
                glm::vec3 enMundo = glm::vec3(matrizMundo * glm::vec4(p[0], p[1], p[2], 1.0f));
                bbMin = glm::min(bbMin, enMundo);
                bbMax = glm::max(bbMax, enMundo);
            }

            // ---- 2) Leer los índices (qué vértices forman cada triángulo) ----
            std::vector<unsigned int> indices;
            if (prim.indices >= 0)
            {
                const tinygltf::Accessor& accIdx = model.accessors[prim.indices];
                int strideIdx;
                const unsigned char* pIdx = datosAccessor(model, prim.indices, strideIdx);
                for (size_t i = 0; i < accIdx.count; i++)
                {
                    if (accIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                        indices.push_back(*(const unsigned int*)(pIdx + i * strideIdx));
                    else if (accIdx.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                        indices.push_back(*(const unsigned short*)(pIdx + i * strideIdx));
                    else
                        indices.push_back(*(pIdx + i * strideIdx));
                }
            }
            else
            {
                for (unsigned int i = 0; i < numVertices; i++) indices.push_back(i);
            }

            // ---- 3) Crear VAO / VBO / EBO de esta pieza ----
            Pieza pieza;
            pieza.matriz = matrizMundo;
            pieza.numIndices = (GLsizei)indices.size();

            glGenVertexArrays(1, &pieza.VAO);
            glGenBuffers(1, &pieza.VBO);
            glGenBuffers(1, &pieza.EBO);

            glBindVertexArray(pieza.VAO);
            glBindBuffer(GL_ARRAY_BUFFER, pieza.VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pieza.EBO);   // el EBO se queda guardado dentro del VAO
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                    // posición
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));  // normal
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));  // uv
            glEnableVertexAttribArray(2);
            glBindVertexArray(0);

            // ---- 4) Material de esta pieza (colores y texturas) ----
            if (prim.material >= 0)
            {
                const tinygltf::Material& mat = model.materials[prim.material];
                const std::vector<double>& c = mat.pbrMetallicRoughness.baseColorFactor;
                if (c.size() == 4) pieza.colorBase = glm::vec4((float)c[0], (float)c[1], (float)c[2], (float)c[3]);
                pieza.texColor = texturaDelModelo(model, mat.pbrMetallicRoughness.baseColorTexture.index);
                pieza.texEmisiva = texturaDelModelo(model, mat.emissiveTexture.index);
                pieza.transparente = (mat.alphaMode == "BLEND");
            }

            piezas.push_back(pieza);
        }
    }

    for (int hijo : nodo.children)
        procesarNodo(model, hijo, matrizMundo, piezas, bbMin, bbMax);
}

// ==========================================================================
//  SHADERS
// ==========================================================================
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location=0) in vec3 apos;\n"
"layout (location=1) in vec3 aNormal;\n"
"layout (location=2) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"out vec3 FragPos;\n"
"out vec3 Normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform vec2 uvScale;\n"
"void main()\n"
"{\n"
"vec4 posicionMundo = model * vec4(apos, 1.0);\n"
"FragPos = posicionMundo.xyz;\n"
"Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"gl_Position = projection * view * posicionMundo;\n"
"TexCoord = aTexCoord * uvScale;\n"
"}\0";

const char* fragmentShaderSource =
"#version 330 core\n"
"in vec2 TexCoord;\n"
"in vec3 FragPos;\n"
"in vec3 Normal;\n"
"out vec4 FragColor;\n"
"uniform sampler2D texColor;\n"
"uniform sampler2D texEmisiva;\n"
"uniform bool tieneEmisiva;\n"
"uniform vec4 colorBase;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 lightPos[3];\n"
"uniform vec3 lightColor[3];\n"
"void main()\n"
"{\n"
"vec4 base = texture(texColor, TexCoord) * colorBase;\n"
"vec3 normal = normalize(Normal);\n"
"vec3 iluminacion = base.rgb * 0.28;\n"
"for (int i = 0; i < 3; i++)\n"
"{\n"
"vec3 haciaLuz = lightPos[i] - FragPos;\n"
"float distancia = length(haciaLuz);\n"
"vec3 direccionLuz = normalize(haciaLuz);\n"
"float difusa = max(dot(normal, direccionLuz), 0.0);\n"
"vec3 vista = normalize(viewPos - FragPos);\n"
"vec3 medio = normalize(direccionLuz + vista);\n"
"float especular = pow(max(dot(normal, medio), 0.0), 32.0);\n"
"float atenuacion = 1.0 / (1.0 + 0.12 * distancia + 0.035 * distancia * distancia);\n"
"iluminacion += base.rgb * lightColor[i] * difusa * atenuacion * 1.8;\n"
"iluminacion += lightColor[i] * especular * atenuacion * 0.22;\n"
"}\n"
"vec3 color = iluminacion;\n"
"if (tieneEmisiva) color += texture(texEmisiva, TexCoord).rgb;\n"
"FragColor = vec4(color, base.a);\n"
"}\n\0";

//  MAIN  -  siguiendo los 6 momentos del profe
// ==========================================================================
int main()
{
    // ================= 1) CONSTRUCT: contexto (GLFW + GLAD) =================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "Arcade - adelanto", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);   // oculta y "atrapa" el mouse
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_DEPTH_TEST);                                  // buffer de profundidad
    glEnable(GL_BLEND);                                       // permite transparencias (vidrio)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ================= 2) GEOMETRY: declaración de vértices =================
    // Un solo cuadrado (1 x 1) sirve para el piso, el techo y las 4 paredes:
    // cada cara es este mismo cuadrado movido, girado y estirado con una matriz "model".
    // Cada vértice: posición (x,y,z) + normal (x,y,z) + coordenada de textura (u,v)
    float verticesCuadrado[] = {
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // abajo izquierda
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,   // abajo derecha
         0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,   // arriba derecha
        -0.5f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f    // arriba izquierda
    };
    // Con índices (EBO) el cuadrado necesita solo 4 vértices en vez de 6
    unsigned int indicesCuadrado[] = { 0, 1, 2,   2, 3, 0 };

    // ================= 3) SHADER COMPILATION =================
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    revisarShader(vertexShader, "vertex shader");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    revisarShader(fragmentShader, "fragment shader");

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, vertexShader);
    glAttachShader(ShaderProgram, fragmentShader);
    glLinkProgram(ShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint modelLoc = glGetUniformLocation(ShaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(ShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(ShaderProgram, "projection");
    GLint uvScaleLoc = glGetUniformLocation(ShaderProgram, "uvScale");
    GLint colorBaseLoc = glGetUniformLocation(ShaderProgram, "colorBase");
    GLint tieneEmisivaLoc = glGetUniformLocation(ShaderProgram, "tieneEmisiva");
    GLint viewPosLoc = glGetUniformLocation(ShaderProgram, "viewPos");
    GLint lightPosLoc[3] = {
        glGetUniformLocation(ShaderProgram, "lightPos[0]"),
        glGetUniformLocation(ShaderProgram, "lightPos[1]"),
        glGetUniformLocation(ShaderProgram, "lightPos[2]")
    };
    GLint lightColorLoc[3] = {
        glGetUniformLocation(ShaderProgram, "lightColor[0]"),
        glGetUniformLocation(ShaderProgram, "lightColor[1]"),
        glGetUniformLocation(ShaderProgram, "lightColor[2]")
    };

    glUseProgram(ShaderProgram);
    glUniform1i(glGetUniformLocation(ShaderProgram, "texColor"), 0);     // texColor usa la unidad 0
    glUniform1i(glGetUniformLocation(ShaderProgram, "texEmisiva"), 1);   // texEmisiva usa la unidad 1

    // ================= 4) BUFFERS: VAO, VBO, EBO =================
    // ---- del cuadrado (habitación) ----
    GLuint VAOcuadrado, VBOcuadrado, EBOcuadrado;
    glGenVertexArrays(1, &VAOcuadrado);
    glGenBuffers(1, &VBOcuadrado);
    glGenBuffers(1, &EBOcuadrado);

    glBindVertexArray(VAOcuadrado);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcuadrado);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCuadrado), verticesCuadrado, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOcuadrado);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesCuadrado), indicesCuadrado, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // ---- Carga de los tres modelos .glb ----
    // Cada modelo conserva sus propias piezas y su propia escala.
    auto cargarModelo = [&](const char* ruta, float alturaModelo)
        {
            ModeloCargado resultado;
            cacheTexturas.clear(); // cada archivo tiene sus propias imágenes internas

            tinygltf::Model modeloGltf;
            tinygltf::TinyGLTF cargador;
            std::string errores, avisos;
            bool cargado = cargador.LoadBinaryFromFile(&modeloGltf, &errores, &avisos, ruta);

            if (!avisos.empty()) std::cout << "Aviso en " << ruta << ": " << avisos << std::endl;
            if (!cargado)
            {
                std::cout << "No se pudo cargar el modelo " << ruta << ": " << errores << std::endl;
                return resultado;
            }

            glm::vec3 bbMin(1e9f), bbMax(-1e9f);
            int escena = modeloGltf.defaultScene >= 0 ? modeloGltf.defaultScene : 0;

            for (int nodoRaiz : modeloGltf.scenes[escena].nodes)
                procesarNodo(modeloGltf, nodoRaiz, glm::mat4(1.0f), resultado.piezas, bbMin, bbMax);

            if (!resultado.piezas.empty())
            {
                float escala = alturaModelo / (bbMax.y - bbMin.y);
                glm::vec3 centro = (bbMin + bbMax) * 0.5f;
                resultado.normalizacion = glm::scale(glm::mat4(1.0f), glm::vec3(escala));
                resultado.normalizacion = glm::translate(resultado.normalizacion,
                    glm::vec3(-centro.x, -bbMin.y, -centro.z));
            }

            std::cout << "Modelo cargado: " << ruta << " (" << resultado.piezas.size() << " piezas)." << std::endl;
            return resultado;
        };

    std::vector<ModeloCargado> modelos;
    modelos.push_back(cargarModelo("arcade_machine.glb", 2.45f));
    modelos.push_back(cargarModelo("abandondedvandalised_arcade_machine.glb", 2.45f));
    modelos.push_back(cargarModelo("dance_dance_revolution.glb", 2.15f));

    // ---- Texturas de la habitación ----
    // stb_image voltea la imagen al cargarla para que coincida con las coordenadas (u,v) de OpenGL.
    // Se activa DESPUÉS de cargar el .glb porque las imágenes de glTF ya vienen acomodadas.
    stbi_set_flip_vertically_on_load(true);
    GLuint texParedA = cargarTextura("pared1.jpeg");
    GLuint texParedB = cargarTextura("pared2.jpeg");
    GLuint texPiso = cargarTextura("piso.jpeg");

    // Textura blanca de 1 píxel: se usa cuando una pieza no tiene textura propia
    unsigned char pixelBlanco[3] = { 255, 255, 255 };
    GLuint texBlanca = subirTextura(pixelBlanco, 1, 1, 3);

    // ---- Dónde va cada máquina dentro de la habitación ----
    // modelo: 0 = arcade original, 1 = arcade abandonada, 2 = Dance Dance Revolution
    struct ColocacionMaquina { int modelo; glm::vec3 posicion; float anguloY; };
    std::vector<ColocacionMaquina> maquinas = {
        { 0, glm::vec3(-2.7f, 0.0f, -largoCuarto / 2.0f + 1.0f), 35.0f },
        { 1, glm::vec3(-anchoCuarto / 2.0f + 1.0f, 0.0f, 0.0f), 90.0f },
        { 2, glm::vec3(2.7f, 0.0f, -largoCuarto / 2.0f + 1.5f), 0.0f }
    };

    float tiempoAnterior = (float)glfwGetTime();

    // ================= 5) CONTINUUM: render loop =================
    while (!glfwWindowShouldClose(window))
    {
        float tiempoActual = (float)glfwGetTime();
        float deltaTime = tiempoActual - tiempoAnterior;     // tiempo entre frames
        tiempoAnterior = tiempoActual;

        // ---- Teclado ----
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);          // ESC cierra (el mouse está atrapado)

        float velocidad = 2.5f * deltaTime;
        // W/S avanzan solo en el plano horizontal
        glm::vec3 adelante = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 derecha = glm::normalize(glm::cross(cameraFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += adelante * velocidad;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= adelante * velocidad;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= derecha * velocidad;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += derecha * velocidad;

        // La cámara no puede salirse de la habitación
        float margen = 0.5f;
        float limiteX = anchoCuarto / 2.0f - margen;
        float limiteZ = largoCuarto / 2.0f - margen;
        if (cameraPos.x > limiteX) cameraPos.x = limiteX;
        if (cameraPos.x < -limiteX) cameraPos.x = -limiteX;
        if (cameraPos.z > limiteZ) cameraPos.z = limiteZ;
        if (cameraPos.z < -limiteZ) cameraPos.z = -limiteZ;
        cameraPos.y = alturaOjos;

        glClearColor(0.05f, 0.07f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(ShaderProgram);

        // ---- Matrices ----
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = (fbHeight != 0) ? (float)fbWidth / (float)fbHeight : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // Luces neon fijas: azul, magenta y cian.
        glm::vec3 posicionesLuces[3] = {
            glm::vec3(-3.5f, 3.0f, -2.5f),
            glm::vec3(3.5f, 3.0f, -2.5f),
            glm::vec3(0.0f, 3.0f,  2.5f)
        };
        glm::vec3 coloresLuces[3] = {
            glm::vec3(0.15f, 0.25f, 1.0f),
            glm::vec3(1.0f, 0.08f, 0.65f),
            glm::vec3(0.05f, 0.85f, 1.0f)
        };
        for (int i = 0; i < 3; i++)
        {
            glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(posicionesLuces[i]));
            glUniform3fv(lightColorLoc[i], 1, glm::value_ptr(coloresLuces[i]));
        }

        // ============ HABITACIÓN ============
        glUniform4f(colorBaseLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform1i(tieneEmisivaLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAOcuadrado);

        float tamanoBaldosa = 2.0f;   // una repetición de la imagen del piso mide 2 x 2 unidades

        // ---- Piso ----
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, largoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform2f(uvScaleLoc, 1.0f, 1.0f); // una imagen completa cubre todo el piso
        glBindTexture(GL_TEXTURE_2D, texPiso ? texPiso : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Techo ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, largoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform2f(uvScaleLoc, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared del fondo (z = -largo/2) ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto / 2.0f, -largoCuarto / 2.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared del frente (z = +largo/2), girada 180° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, altoCuarto / 2.0f, largoCuarto / 2.0f));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(anchoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedA ? texParedA : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared izquierda (x = -ancho/2), girada 90° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-anchoCuarto / 2.0f, altoCuarto / 2.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(largoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedB ? texParedB : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ---- Pared derecha (x = +ancho/2), girada -90° ----
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(anchoCuarto / 2.0f, altoCuarto / 2.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(largoCuarto, altoCuarto, 1.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, texParedB ? texParedB : texBlanca);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // ============ MÁQUINAS ARCADE ============
        // Dos vueltas: primero las piezas sólidas y al final las transparentes (vidrio)
        glUniform2f(uvScaleLoc, 1.0f, 1.0f);
        for (int vuelta = 0; vuelta < 2; vuelta++)
        {
            bool dibujarTransparentes = (vuelta == 1);
            for (const ColocacionMaquina& maq : maquinas)
            {
                glm::mat4 colocacion = glm::translate(glm::mat4(1.0f), maq.posicion);
                colocacion = glm::rotate(colocacion, glm::radians(maq.anguloY), glm::vec3(0.0f, 1.0f, 0.0f));

                const ModeloCargado& modeloActual = modelos[maq.modelo];
                for (const Pieza& pieza : modeloActual.piezas)
                {
                    if (pieza.transparente != dibujarTransparentes) continue;

                    glm::mat4 modelPieza = colocacion * modeloActual.normalizacion * pieza.matriz;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPieza));
                    glUniform4fv(colorBaseLoc, 1, glm::value_ptr(pieza.colorBase));
                    glUniform1i(tieneEmisivaLoc, pieza.texEmisiva != 0 ? 1 : 0);

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, pieza.texColor ? pieza.texColor : texBlanca);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, pieza.texEmisiva ? pieza.texEmisiva : texBlanca);
                    glActiveTexture(GL_TEXTURE0);

                    glBindVertexArray(pieza.VAO);
                    glDrawElements(GL_TRIANGLES, pieza.numIndices, GL_UNSIGNED_INT, 0);
                }
            }
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ================= 6) FREE MEMORY SPACE =================
    glDeleteVertexArrays(1, &VAOcuadrado);
    glDeleteBuffers(1, &VBOcuadrado);
    glDeleteBuffers(1, &EBOcuadrado);
    for (ModeloCargado& modelo : modelos)
    {
        for (Pieza& pieza : modelo.piezas)
        {
            glDeleteVertexArrays(1, &pieza.VAO);
            glDeleteBuffers(1, &pieza.VBO);
            glDeleteBuffers(1, &pieza.EBO);
        }
    }
    for (GLuint tex : texturasCargadas)
        if (tex != 0) glDeleteTextures(1, &tex);
    glDeleteTextures(1, &texParedA);
    glDeleteTextures(1, &texParedB);
    glDeleteTextures(1, &texPiso);
    glDeleteTextures(1, &texBlanca);
    glDeleteProgram(ShaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}