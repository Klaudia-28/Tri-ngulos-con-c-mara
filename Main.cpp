#include <iostream> //Permite mostrar mensajes en la consola
#include <glad/glad.h> //Funciones de OpenGL, siempre va antes de GLFW
#include <GLFW/glfw3.h> //Crea la ventana y maneja el contexto de OpenGL con los eventos
#include <glm/glm.hpp> //Permite usar vectores y matrices
#include <glm/gtc/matrix_transform.hpp> //Permite crear matrices de posición, rotación y perspectiva
#include <glm/gtc/type_ptr.hpp> //Permite pasar matrices de GLM a OpenGL

//Se llama cuando el usuario cambia el tamaño de la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height); //Ajusta el área de dibujo al nuevo tamaño
}

//-----------VERTEX SHADER----------
//La posición final se calcula usando las matrices de modelo, vista y proyección
//Esto permite ubicar los triángulos en un espacio 3D y mover la cámara
const char* vertexShaderSource =
"#version 330 core\n" //Indica que usamos OpenGL 3.3
"layout (location=0) in vec3 apos;\n" //Recibe la posición del vértice
"layout (location=1) in vec3 aColor;\n" //Recibe el color del vértice
"out vec3 vertexColor;\n" //Envía el color al Fragment Shader
"uniform mat4 model;\n" //Guarda la posición y escala del triángulo
"uniform mat4 view;\n" //Guarda la posición y dirección de la cámara
"uniform mat4 projection;\n" //Define cómo se muestra el mundo 3D en la pantalla
"void main()\n" //Función principal del Vertex Shader
"{\n"
"gl_Position= projection*view*model*vec4(apos, 1.0);\n" //Calcula la posición final del vértice
"vertexColor= aColor;\n" //Envía el color del vértice
"}\0";

//----------FRAGMENT SHADER----------
const char* fragmentShaderSource =
"#version 330 core\n" //Indica que usamos OpenGL 3.3
"in vec3 vertexColor;\n"  //Recibe el color que le pasó el Vertex Shader
"out vec4 FragColor;\n" //Guarda el color final
"void main()\n" //Función principal del Fragment Shader
"{\n"
"FragColor = vec4(vertexColor, 1.0);\n" //Establece el color final
"}\n\0";

int main()
{
	//----------INICIALIZACIÓN DE GLFW----------
	glfwInit(); //Inicia GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Establece la versión principal de OpenGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //Establece la versión secundaria de OpenGL
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Usa el perfil moderno de OpenGL

	//----------DATOS DEL TRIÁNGULO----------
		//Un solo triángulo base con un color por vértice
		//El mismo triángulo se dibuja 3 veces con diferente posición y escala
	GLfloat vertices[] = {
		//posiciones              //colores
		-0.3f, -0.3f, 0.0f,       0.773f, 0.200f, 0.392f, //Posición y color del primer vértice
		0.3f, -0.3f, 0.0f,        0.992f, 0.510f, 0.388f, //Posición y color del segundo vértice
		0.0f, 0.3f, 0.0f,         0.455f, 0.153f, 0.424f //Posición y color del tercer vértice
	};

	//----------CREACIÓN DE LA VENTANA---------
	GLFWwindow* window = glfwCreateWindow(800, 800, "Triángulos con cámara", NULL, NULL); //Crea una ventana de 800x800
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl; //Muestra un mensaje si falla la ventana
		glfwTerminate(); //Cierra GLFW
		return -1; //Termina el programa con error
	}
	glfwMakeContextCurrent(window); //Hace que esta ventana sea la que use OpenGL

	//----------INICIALIZACIÓN DE GLAD----------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl; //Muestra un mensaje si falla GLAD
		return -1; //Termina el programa con error
	}
	int fbWidth, fbHeight; //Guarda el ancho y alto del área de dibujo
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight); //Obtiene el tamaño del área de dibujo
	glViewport(0, 0, fbWidth, fbHeight); //Establece el área donde se va a dibujar
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //Actualiza el área de dibujo al cambiar la ventana

	//Buffer de profundidad, es indispensable para que los triángulos que están detrás
	//no se dibujen por encima de los que están delante
	glEnable(GL_DEPTH_TEST); //Activa la comprobación de profundidad

	//----------COMPILACIÓN DE SHADERS----------
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); //Crea el Vertex Shader
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //Le entrega su código
	glCompileShader(vertexShader); //Compila el Vertex Shader

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Crea el Fragment Shader
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); //Le entrega su código
	glCompileShader(fragmentShader); //Compila el Fragment Shader

	GLuint ShaderProgram = glCreateProgram(); //Crea el programa para unir los shaders
	glAttachShader(ShaderProgram, vertexShader); //Agrega el Vertex Shader
	glAttachShader(ShaderProgram, fragmentShader); //Agrega el Fragment Shader
	glLinkProgram(ShaderProgram); //Une los dos shaders

	glDeleteShader(vertexShader); //Elimina el Vertex Shader individual
	glDeleteShader(fragmentShader); //Elimina el Fragment Shader individual

	//Ubicación de las 3 matrices dentro del shader
	GLint modelLoc = glGetUniformLocation(ShaderProgram, "model"); //Obtiene la ubicación de la matriz model
	GLint viewLoc = glGetUniformLocation(ShaderProgram, "view"); //Obtiene la ubicación de la matriz view
	GLint projLoc = glGetUniformLocation(ShaderProgram, "projection"); //Obtiene la ubicación de la matriz projection

	//----------VAO/VBO----------
	GLuint VAO, VBO; //VAO guarda la configuración y VBO guarda los datos
	glGenVertexArrays(1, &VAO); //Crea el VAO
	glGenBuffers(1, &VBO); //Crea el VBO

	glBindVertexArray(VAO); //Activa el VAO
	glBindBuffer(GL_ARRAY_BUFFER, VBO); //Activa el VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Pasa los datos de los vértices al VBO

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); //Indica cómo leer la posición
	glEnableVertexAttribArray(0); //Activa el atributo de posición

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); //Indica cómo leer el color
	glEnableVertexAttribArray(1); //Activa el atributo de color

	glBindBuffer(GL_ARRAY_BUFFER, 0); //Desactiva el VBO
	glBindVertexArray(0); //Desactiva el VAO

	//Los tres triángulos quedan quietos alineados en el eje Z, con espacio entre ellos 
	//el de adelante es el más pequeño, el del medio es de tamaño normal
	//el de atrás es un poco más grande
	float posicionesZ[3] = { -3.0f, -4.0f, -5.0f }; //Adelante, medio y atrás
	float escalas[3] = { 0.65f, 0.85f, 1.15f }; //Pequeño, mediano y grande

	//Punto alrededor del cual va a girar la cámara, o sea el centro de los tres triángulos
	glm::vec3 centro = glm::vec3(0.0f, 0.0f, -4.0f); //Define el centro de los triángulos
	float radioOrbita = 6.0f; //Indica qué tan lejos gira la cámara del centro
	float alturaCamara = 0.0f; //Mantiene la cámara a la misma altura del centro

	//----------RENDER LOOP----------
	while (!glfwWindowShouldClose(window)) //Se repite mientras la ventana esté abierta
	{
		glClearColor(0.05f, 0.07f, 0.14f, 1.0f); //Establece el fondo azul oscuro
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Limpia el color y la profundidad
		glUseProgram(ShaderProgram); //Activa el programa de shaders

		//Se declaran las matrices al revés, projection, view y model
		//Esto porque en el shader se multiplican de derecha a izquierda

		//PROYECCIÓN
		//Se recalcula el tamaño actual de la ventana para que no se deforme

		glfwGetFramebufferSize(window, &fbWidth, &fbHeight); //Obtiene el tamaño actual de la ventana
		float aspect = (fbHeight != 0) ? (float)fbWidth / (float)fbHeight : 1.0f; //Calcula la proporción de la ventana
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f); //Crea la perspectiva de la cámara

		//VISTA
		//En vez de una cámara fija, esta cámara orbita alrededor de los triángulos
		//El ángulo avanza con el tiempo y con sen/cos se calcula un punto que gira en círculo sobre el eje X-Z
		//Siempre mirando hacia el centro
		float angulo = (float)glfwGetTime() * 0.5f; //Calcula el ángulo de la cámara y lo hace más lento
		float camX = centro.x + sin(angulo) * radioOrbita; //Calcula la posición X de la cámara
		float camZ = centro.z + cos(angulo) * radioOrbita; //Calcula la posición Z de la cámara
		glm::mat4 view = glm::lookAt(
			glm::vec3(camX, alturaCamara, camZ),  //Posición de la cámara
			centro,                               //Punto hacia donde mira la cámara
			glm::vec3(0.0f, 1.0f, 0.0f)           //Indica cuál es la dirección de arriba
		);

		//Los uniforms de view y projection son los mismos para los 3 triángulos
		//Así que se actualizan una sola vez por frame
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view)); //Envía la matriz view al shader
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection)); //Envía la matriz projection al shader

		glBindVertexArray(VAO); //Activa la configuración de los vértices

		for (int i = 0; i < 3; i++) //Repite el dibujo para los 3 triángulos
		{
			//MODELO
			//La posición y el tamaño propios de este triángulo, no rota y se queda quieto
			//Lo único que cambia entre frames es la cámara

				//El triángulo base tiene su vértice inferior en y = -0.3
				//Al escalar ese punto se mueve hacia -0.3*escala, o sea que entre más pequeño
				//La base sube hacia el centro, para que los 3 se vean alineados abajo
				//Se calcula cuánto hay que subir/bajar cada uno para igualar la base
				//Con la del triángulo más grande que se deja como referencia
			float baseLocalY = -0.3f; //Guarda la posición Y de la base original
			float baseReferencia = baseLocalY * escalas[2]; //Calcula la base del triángulo grande
			float offsetY = baseReferencia - (baseLocalY * escalas[i]); //Calcula cuánto mover cada triángulo

			glm::mat4 model = glm::mat4(1.0f); //Crea la matriz inicial del triángulo
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, posicionesZ[i])); //Mueve el triángulo en Z
			model = glm::translate(model, glm::vec3(0.0f, offsetY, 0.0f)); //Alinea la base en Y
			model = glm::scale(model, glm::vec3(escalas[i])); //Cambia el tamaño del triángulo

			//---Actualizar uniforms---
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //Envía la matriz model al shader

			//---Poner los valores en el shader---
			glDrawArrays(GL_TRIANGLES, 0, 3); //Dibuja el triángulo usando sus 3 vértices
		}

		glBindVertexArray(0); //Desactiva el VAO

		glfwSwapBuffers(window); //Muestra el dibujo en la pantalla
		glfwPollEvents(); //Revisa los eventos de la ventana
	}

	//-----------LIMPIEZA FINAL----------
	glDeleteVertexArrays(1, &VAO); //Elimina el VAO
	glDeleteBuffers(1, &VBO); //Elimina el VBO
	glDeleteProgram(ShaderProgram); //Elimina el programa de shaders

	glfwDestroyWindow(window); //Destruye la ventana
	glfwTerminate(); //Cierra GLFW
	return 0; //Indica que el programa terminó correctamente
}