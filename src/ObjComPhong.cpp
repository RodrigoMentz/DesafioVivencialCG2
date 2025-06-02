/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);
int loadSimpleOBJ(string filePATH, int &nVertices);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar* vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texc;
layout (location = 3) in vec3 normal;

uniform mat4 projection;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;
void main()
{
   	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
	fragPos = model * vec4(position.x, position.y, position.z, 1.0);
	texCoord = vec2(texc.s, 1 - texc.t);
	vNormal = normal;
	vColor = vec4(color,1.0);
})";

const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
uniform sampler2D texBuff;
uniform vec3 camPos;

uniform struct Light {
    vec3 position;
    vec3 color;
    bool enabled;
    float constant;
    float linear;
    float quadratic;
} lights[3];

uniform float ka;
uniform float kd;
uniform float ks;
uniform float q;
out vec4 color;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;
void main()
{
    vec4 textureColor = texture(texBuff, texCoord);
    vec3 totalLight = vec3(0.0);

    for(int i = 0; i < 3; i++)
    {
        if(lights[i].enabled)
        {
            vec3 ambient = ka * lights[i].color * vec3(textureColor);

            vec3 N = normalize(vNormal);
            vec3 L = normalize(lights[i].position - vec3(fragPos));
            float diff = max(dot(N, L), 0.0);
            vec3 diffuse = kd * diff * lights[i].color * vec3(textureColor);

            vec3 R = normalize(reflect(-L, N));
            vec3 V = normalize(camPos - vec3(fragPos));
            float spec = max(dot(R,V),0.0);
            spec = pow(spec,q);
            vec3 specular = ks * spec * lights[i].color;

            float distance = length(lights[i].position - vec3(fragPos));
            float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));

            totalLight += attenuation * (diffuse + specular) + ambient; // Atenuação se aplica ao difuso e especular, ambiente é constante
        }
    }

    color = vec4(totalLight, 1.0);
})";

struct Object
{
	GLuint VAO; //Índice do buffer de geometria
	GLuint texID; //Identificador da textura carregada
	int nVertices; //nro de vértices
};

struct Material {
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    std::string textureFile;
};

struct Light {
	vec3 position;
    vec3 color;
    bool enabled = true;
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
};

Object obj;
std::unordered_map<std::string, Material> materiais;
std::string nomeMaterial;
Light lights[3];
GLuint currentShaderID;

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio Vivencial 2 - Rodrigo Korte Mentz", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();
	currentShaderID = shaderID;

	obj.VAO = loadSimpleOBJ("../assets/Modelos3D/SuzanneSubdiv1.obj", obj.nVertices);

    Material mat = materiais["Material.001"];
    cout << "Conferir material.textureFile: " << mat.textureFile << endl;
	// Carregando uma textura e armazenando seu id
	int imgWidth, imgHeight;
	obj.texID = loadTexture("../assets/Modelos3D/" + mat.textureFile,imgWidth,imgHeight);

    float q = 10.0;
	vec3 camPos = vec3(0.0,0.0,-2.0);

	// Luz Principal
	lights[0].position = vec3(2.0, 2.0, -3.0);
    lights[0].color = vec3(0.4, 0.4, 0.4);

    // Luz de Preenchimento
    lights[1].position = vec3(-1.8, 1.5, 1.5);
    lights[1].color = vec3(0.3, 0.3, 0.3);

    // Luz de Fundo
    lights[2].position = vec3(4.0, -4.0, 4.0);
    lights[2].color = vec3(0.2, 0.2, 0.2);

	glUseProgram(shaderID);

	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    glUniform1f(glGetUniformLocation(shaderID, "ka"), mat.ka.r);
	glUniform1f(glGetUniformLocation(shaderID, "kd"), mat.kd.r);
	glUniform1f(glGetUniformLocation(shaderID, "ks"), mat.ks.r);
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);
	glUniform3f(glGetUniformLocation(shaderID, "camPos"), camPos.x,camPos.y,camPos.z);

	for (int i = 0; i < 3; i++) {
        string lightPosUniform = "lights[" + to_string(i) + "].position";
        string lightColorUniform = "lights[" + to_string(i) + "].color";
        string lightEnabledUniform = "lights[" + to_string(i) + "].enabled";
        string lightConstantUniform = "lights[" + to_string(i) + "].constant";
        string lightLinearUniform = "lights[" + to_string(i) + "].linear";
        string lightQuadraticUniform = "lights[" + to_string(i) + "].quadratic";

        glUniform3f(glGetUniformLocation(shaderID, lightPosUniform.c_str()), lights[i].position.x, lights[i].position.y, lights[i].position.z);
        glUniform3f(glGetUniformLocation(shaderID, lightColorUniform.c_str()), lights[i].color.x, lights[i].color.y, lights[i].color.z);
        glUniform1i(glGetUniformLocation(shaderID, lightEnabledUniform.c_str()), lights[i].enabled);
        glUniform1f(glGetUniformLocation(shaderID, lightConstantUniform.c_str()), lights[i].constant);
        glUniform1f(glGetUniformLocation(shaderID, lightLinearUniform.c_str()), lights[i].linear);
        glUniform1f(glGetUniformLocation(shaderID, lightQuadraticUniform.c_str()), lights[i].quadratic);
    }

	//Ativando o primeiro buffer de textura da OpenGL
	glActiveTexture(GL_TEXTURE0);

	// Matriz de projeção paralela ortográfica
	mat4 projection = ortho(-4.0, 4.0, -4.0, 4.0, -4.0, 4.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	// Matriz de modelo: transformações na geometria (objeto)
	mat4 model = mat4(1); // matriz identidade
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

	glEnable(GL_DEPTH_TEST);


	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(obj.VAO); // Conectando ao buffer de geometria
		glBindTexture(GL_TEXTURE_2D, obj.texID); //conectando com o buffer de textura que será usado no draw
		glDrawArrays(GL_TRIANGLES, 0, obj.nVertices);

		glBindVertexArray(0); // Desconectando o buffer de geometria

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &obj.VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
        glfwSetWindowShouldClose(window, GL_TRUE);
	}

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        lights[0].enabled = !lights[0].enabled;
		cout << "Status luz 1: " << lights[0].enabled << endl;
        glUniform1i(glGetUniformLocation(currentShaderID, "lights[0].enabled"), lights[0].enabled);
    }

    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        lights[1].enabled = !lights[1].enabled;
		cout << "Status luz 2: " << lights[1].enabled << endl;
        glUniform1i(glGetUniformLocation(currentShaderID, "lights[1].enabled"), lights[1].enabled);
    }

    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        lights[2].enabled = !lights[2].enabled;
		cout << "Status luz 3: " << lights[2].enabled << endl;
        glUniform1i(glGetUniformLocation(currentShaderID, "lights[2].enabled"), lights[2].enabled);
    }
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint loadTexture(string filePath, int &width, int &height)
{
	GLuint texID; // id da textura a ser carregada

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// Ajuste dos parâmetros de wrapping e filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Carregamento da imagem usando a função stbi_load da biblioteca stb_image
	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // assume que é 4 canais png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture " << filePath << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

int loadSimpleOBJ(string filePATH, int &nVertices)
 {
    std::string nomeArquivoMtl;
    float ka, ks, ke;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0, 1.0, 1.0);
    GLuint texturaId = 0;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
	{
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
	{
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib")
		{
            ssline >> nomeArquivoMtl;
        }
        if (word == "v")
		{
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
		{
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
		{
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
		 {
            while (ssline >> word)
			{
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);

                if (ti >= 0 && ti < texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].x);
                    vBuffer.push_back(texCoords[ti].y);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }

                // Adicionando normais (nx, ny, nz)
                if (ni >= 0 && ni < normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }

    arqEntrada.close();

    if (!nomeArquivoMtl.empty()) {
        std::string diretorioObj = filePATH.substr(0, filePATH.find_last_of("/\\") + 1);
        std::string caminhoMTL = diretorioObj + nomeArquivoMtl;
        std::ifstream arqMTL(caminhoMTL.c_str());
        if (arqMTL.is_open()) {
            std::string mtlLine;
            while (std::getline(arqMTL, mtlLine)) {
                std::istringstream ssmtl(mtlLine);
                std::string mtlWord;
                ssmtl >> mtlWord;

                if (mtlWord == "newmtl") {
                    ssmtl >> nomeMaterial;
                    materiais[nomeMaterial] = Material();
                    cout << "Nome material: " << nomeMaterial << endl;
                }

                if (mtlWord == "Ka") {
                    ssmtl >> materiais[nomeMaterial].ka.r >> materiais[nomeMaterial].ka.g >> materiais[nomeMaterial].ka.b;
                }

                if (mtlWord == "Kd") {
                    ssmtl >> materiais[nomeMaterial].kd.r >> materiais[nomeMaterial].kd.g >> materiais[nomeMaterial].kd.b;
                }

                if (mtlWord == "Ks") {
                    ssmtl >> materiais[nomeMaterial].ks.r >> materiais[nomeMaterial].ks.g >> materiais[nomeMaterial].ks.b;
                }

                if (mtlWord == "map_Kd") {
                    ssmtl >> materiais[nomeMaterial].textureFile;
                    cout << "Nome arquivo textura: " << materiais[nomeMaterial].textureFile << endl;
                }
            }
            arqMTL.close();
        }
    }

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

	GLsizei stride = 11 * sizeof(GLfloat);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);                    // posição
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat))); // cor
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat))); // texcoord
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat))); // normal

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	nVertices = vBuffer.size() / 11;  // x, y, z, r, g, b, s, t, nx, ny, nz (valores atualmente armazenados por vértice)
	cout << "nVertices: " << nVertices << endl;
    return VAO;
}