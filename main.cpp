#include "src\Shader.h"
#include "src\SceneRenderer.h"
#include <GLFW\glfw3.h>
#include "src\MyImGuiPanel.h"

#include "src\ViewFrustumSceneObject.h"
#include "src\terrain\MyTerrain.h"
#include "src\MyCameraManager.h"

#pragma comment (lib, "lib-vc2015\\glfw3.lib")
#pragma comment(lib, "assimp-vc141-mt.lib")

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "src\MyPoissonSample.h"

#include <glm/gtx/quaternion.hpp>

using namespace std;
using namespace glm;

int FRAME_WIDTH = 1024;
int FRAME_HEIGHT = 512;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

bool initializeGL();
void resizeGL(GLFWwindow *window, int w, int h);
void paintGL();
void resize(const int w, const int h);

bool m_leftButtonPressed = false;
bool m_rightButtonPressed = false;
double cursorPos[2];



MyImGuiPanel* m_imguiPanel = nullptr;

void vsyncDisabled(GLFWwindow *window);

// ==============================================
SceneRenderer *defaultRenderer = nullptr;
ShaderProgram* defaultShaderProgram = new ShaderProgram();

ViewFrustumSceneObject* m_viewFrustumSO = nullptr;
MyTerrain* m_terrain = nullptr;
INANOA::MyCameraManager* m_myCameraManager = nullptr;
// ==============================================


GLuint			albedo_location;
GLuint          textureArrayHandle;



//airplane=======================================
GLuint airplane_vao;
GLuint airplane_vbo;
GLuint airplane_ebo;

GLuint airplane_texture;
//rock=======================================
GLuint rock_vao;
GLuint rock_vbo;
GLuint rock_ebo;

GLuint rock_texture;


//Shader Program=======================================
ShaderProgram* reset_cs_shader_program;
ShaderProgram* culling_cs_shader_program;
ShaderProgram* shaderProgram;
ShaderProgram* defShaderProgram;

GLuint          NUM_TOTAL_INSTANCE_LOCATION;
int             NUM_TOTAL_INSTANCE;


//grass and building=========================================
struct DrawElementsIndirectCommand {
	unsigned int count;
	unsigned int instanceCount;
	unsigned int firstIndex;
	unsigned int baseVertex;
	unsigned int baseInstance;
};
struct InstanceProperties {
	vec4 position;
	//vec4 radians;
	//int flag;
};
struct RawInstanceProperties {
	vec4 position;
	vec4 boundSphere; //前三個存圓心座標，第四個存半徑

	mat4 rotationMatrix;
};

GLuint grass_building_vao;
GLuint grass_building_vbo;
GLuint grass_building_ebo;

GLuint           model_location;
GLuint           view_location;
GLuint           proj_location;
GLuint           features_loc;

mat4 model_matrix;
mat4 view_matrix;
mat4 proj_matrix;

//===============================================

typedef struct _texture_data
{
	_texture_data() : width(0), height(0), data(0) {}
	int width;
	int height;
	unsigned char* data;
} texture_data;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
texture_data loadImg(const char* path)
{
	texture_data texture;
	int n;
	stbi_set_flip_vertically_on_load(true);
	stbi_uc* data = stbi_load(path, &texture.width, &texture.height, &n, 4);
	if (data != NULL)
	{
		texture.data = new unsigned char[texture.width * texture.height * 4 * sizeof(unsigned char)];
		memcpy(texture.data, data, texture.width * texture.height * 4 * sizeof(unsigned char));
		stbi_image_free(data);
	}
	return texture;
}

texture_data loadImg(const char* path, bool flip)
{
	texture_data texture;
	int n;
	stbi_set_flip_vertically_on_load(flip);
	stbi_uc* data = stbi_load(path, &texture.width, &texture.height, &n, 4);
	if (data != NULL)
	{
		texture.data = new unsigned char[texture.width * texture.height * 4 * sizeof(unsigned char)];
		memcpy(texture.data, data, texture.width * texture.height * 4 * sizeof(unsigned char));
		stbi_image_free(data);
	}
	return texture;
}

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth);
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner);

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 TexCoords;
};


struct Texture {
	GLuint id;
	string type;
	string path;
};


GLuint cmdBufferHandle;
class Mesh {
public:
	//C++寫法
	vector<Vertex> vertices; //儲存頂點的陣列
	vector<unsigned int> indices; //儲存index的陣列
	vector<Texture> textures; //儲存texture的陣列

	//constructor
	Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		//setupMesh();
		printf("Mesh set up\n");
	}
	void Draw()
	{

		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			/*
			string number;
			string name = textures[i].type;
			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++);
			*/
			//shader.setInt(("material." + name + number).c_str(), i);

			////這行好像沒差
			//glUniform1i(glGetUniformLocation(program, ("material." + name + number).c_str()) , i);

			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);
	}

};

float current_idx;
//model clas
class Model
{
public:
	//constructor
	Model(char* path)
	{
		loadModel(path);
		printf("Model loaded\n");
	}
	void Draw() {
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw();
			//printf("%d\n", i);
		}
	}

	vector<Mesh> getMesh() {
		return meshes;
	}
private:

	vector<Texture> textures_loaded;
	vector<Mesh> meshes;
	string directory;

	void loadModel(string path)
	{

		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

		//const aiScene* scene = aiImportFile(path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			auto e = std::string("ERROR ASSIMP ") + import.GetErrorString();
			throw std::runtime_error(e);
		}
		directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		vector<Texture> textures;

		//vertex
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vec3 vector;
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0])
			{
				vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec3(vec, current_idx); //TODO: 第三項要改成texture的id
			}
			else
				vertex.TexCoords = vec3(0.0f, 0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		//index
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			//indices.push_back(mesh->mNumFaces * 3);

			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
				//cout << "face.mIndices[j] = " << face.mIndices[j] << endl;

			}
			//printf("\n");

		}

		//material
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		return Mesh(vertices, indices, textures);
	}

	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
	{
		vector<Texture> textures;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			bool skip = false;
			for (unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{   // if texture hasn't been loaded already, load it
				/*
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory.c_str());
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);
				*/
			}
		}
		return textures;
	}
};

int main(){
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(FRAME_WIDTH, FRAME_HEIGHT, "rendering", nullptr, nullptr);
	if (window == nullptr){
		std::cout << "failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// load OpenGL function pointer
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, mouseScrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetFramebufferSizeCallback(window, resizeGL);

	if (initializeGL() == false) {
		std::cout << "initialize GL failed\n";
		glfwTerminate();
		system("pause");
		return 0;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	// disable vsync
	//glfwSwapInterval(0);

	// start game-loop
	vsyncDisabled(window);
		

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void vsyncDisabled(GLFWwindow *window) {
	double previousTimeForFPS = glfwGetTime();
	int frameCount = 0;

	static int IMG_IDX = 0;

	while (!glfwWindowShouldClose(window)) {
		// measure speed
		const double currentTime = glfwGetTime();
		frameCount = frameCount + 1;
		const double deltaTime = currentTime - previousTimeForFPS;
		if (deltaTime >= 1.0) {
			m_imguiPanel->setAvgFPS(frameCount * 1.0);
			m_imguiPanel->setAvgFrameTime(deltaTime * 1000.0 / frameCount);

			// reset
			frameCount = 0;
			previousTimeForFPS = currentTime;
		}			

		glfwPollEvents();
		paintGL();
		glfwSwapBuffers(window);
	}
}

vector<Model> m_Models;

int airplane_idx = 0;
void initAirplane() {
	current_idx = 10.0;
	Model obj1("assets/airplane.obj");
	m_Models.push_back(obj1);

	vector<Mesh> air_plane_mesh = obj1.getMesh();
	vector<Vertex> airplane_vertex;
	vector<unsigned int> airplane_index;
	for (auto i : air_plane_mesh) {
		for (auto j : i.vertices) {
			airplane_vertex.push_back(j);
		}
		for (auto k : i.indices) {
			airplane_idx++;
			airplane_index.push_back(k);
		}
	}

	glGenVertexArrays(1, &airplane_vao);
	glBindVertexArray(airplane_vao);

	glGenBuffers(1, &airplane_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, airplane_vbo);
	glBufferData(GL_ARRAY_BUFFER, airplane_vertex.size() * sizeof(Vertex), airplane_vertex.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &airplane_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, airplane_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, airplane_index.size() * sizeof(unsigned int), airplane_index.data(), GL_STATIC_DRAW);

	//Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	//Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);
	//TexCoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);


	//texture
	texture_data tdata = loadImg("assets/Airplane_smooth_DefaultMaterial_BaseMap.jpg", false);
	glGenTextures(1, &airplane_texture);
	glBindTexture(GL_TEXTURE_2D, airplane_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	cout << "init airplane\n";
}

void renderAirplane(mat4 airplaneModelMat) {
	//mat4 airplaneModelMat = m_myCameraManager->airplaneModelMatrix();
	glUniformMatrix4fv(model_location, 1, GL_FALSE, value_ptr(airplaneModelMat));

	glUniform1i(SceneManager::Instance()->m_fs_pixelProcessIdHandle, 10);
	glUniform1i(SceneManager::Instance()->m_vs_vertexProcessIdHandle, 10);

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D, airplane_texture);

	glBindVertexArray(airplane_vao);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, slime_ebo);
	glDrawElements(GL_TRIANGLES, airplane_idx, GL_UNSIGNED_INT, 0);
}

int rock_idx;
void initRock() {
	current_idx = 11.0;
	Model obj1("assets/MagicRock/magicRock.obj");
	m_Models.push_back(obj1);

	vector<Mesh> rock_mesh = obj1.getMesh();
	vector<Vertex> rock_vertex;
	vector<unsigned int> rock_index;
	for (auto i : rock_mesh) {
		for (auto j : i.vertices) {
			rock_vertex.push_back(j);
		}
		for (auto k : i.indices) {
			rock_idx++;
			rock_index.push_back(k);
		}
	}

	glGenVertexArrays(1, &rock_vao);
	glBindVertexArray(rock_vao);

	glGenBuffers(1, &rock_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, rock_vbo);
	glBufferData(GL_ARRAY_BUFFER, rock_vertex.size() * sizeof(Vertex), rock_vertex.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &rock_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rock_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rock_index.size() * sizeof(unsigned int), rock_index.data(), GL_STATIC_DRAW);

	//Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	//Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);
	//TexCoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);


	//texture
	texture_data tdata = loadImg("assets/MagicRock/StylMagicRocks_AlbedoTransparency.png", false);
	glGenTextures(1, &rock_texture);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, rock_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tdata.width, tdata.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tdata.data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	cout << "init rock\n";
}

void renderRock() {
	//mat4 airplaneModelMat = m_myCameraManager->airplaneModelMatrix();
	mat4 identity = mat4(1.0);
	mat4 rockModelMat = identity;
	vec3 position = vec3(25.92, 18.27, 11.75);
	rockModelMat = translate(rockModelMat, position );
	
	glUniformMatrix4fv(model_location, 1, GL_FALSE, value_ptr(rockModelMat));

	glUniform1i(SceneManager::Instance()->m_fs_pixelProcessIdHandle, 11);
	glUniform1i(SceneManager::Instance()->m_vs_vertexProcessIdHandle, 11);

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, rock_texture);

	glBindVertexArray(rock_vao);
	glDrawElements(GL_TRIANGLES, rock_idx, GL_UNSIGNED_INT, 0);
}



void initTexture() {
	int NUM_TEXTURE = 5; //貼圖數量

	texture_data tex0 = loadImg("assets/grassB_albedo.png");
	texture_data tex1 = loadImg("assets/bush01.png");
	texture_data tex2 = loadImg("assets/bush05.png");
    texture_data tex3 = loadImg("assets/Medieval_Building_LowPoly/Medieval_Building_LowPoly_V2_Albedo_small.png");
    texture_data tex4 = loadImg("assets/Medieval_Building_LowPoly/Medieval_Building_LowPoly_V1_Albedo_small.png");

	// create texture array
	glGenTextures(1, &textureArrayHandle);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);
	// the internal format for glTexStorageXD must be "Sized Internal Formats"
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, tex0.width, tex0.height, NUM_TEXTURE);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, tex0.width, tex0.height, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, tex0.data);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, tex1.width, tex1.height, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, tex1.data);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 2, tex2.width, tex2.height, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, tex2.data);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 3, tex3.width, tex3.height, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, tex3.data);

	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 4, tex3.width, tex3.height, 1, GL_RGBA,
		GL_UNSIGNED_BYTE, tex4.data);

	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

vector<Vertex> mergeVertices;
vector<unsigned int> mergeIndices;
void initGrassBuilding() {
	current_idx = 0.0;
	Model obj0("assets/grassB.obj");

	current_idx = 1.0;
	Model obj1("assets/bush01_lod2.obj");

	current_idx = 2.0;
	Model obj2("assets/bush05_lod2.obj");

	current_idx = 3.0;
	Model obj3("assets/Medieval_Building_LowPoly/medieval_building_lowpoly_2.obj");

	current_idx = 4.0;
	Model obj4("assets/Medieval_Building_LowPoly/medieval_building_lowpoly_1.obj");

	unsigned int vtx0 = 0;
	unsigned int vtx1 = 0;
	unsigned int vtx2 = 0;
	unsigned int vtx3 = 0;
	unsigned int vtx4 = 0;

	unsigned int idx0 = 0;
	unsigned int idx1 = 0;
	unsigned int idx2 = 0;
	unsigned int idx3 = 0;
	unsigned int idx4 = 0;

	vector<Mesh> mesh0 = obj0.getMesh();
	for (auto i : mesh0) {
		for (auto j : i.vertices) {
			vtx0++;
			mergeVertices.push_back(j);
		}
		for (auto k : i.indices) {
			idx0++;
			mergeIndices.push_back(k);
		}
	}

	vector<Mesh> mesh1 = obj1.getMesh();
	for (auto i : mesh1) {
		for (auto j : i.vertices) {
			vtx1++;
			mergeVertices.push_back(j);
		}
		for (auto k : i.indices) {
			idx1++;
			mergeIndices.push_back(k);
		}
	}

	vector<Mesh> mesh2 = obj2.getMesh();
	for (auto i : mesh2) {
		for (auto j : i.vertices) {
			vtx2++;
			mergeVertices.push_back(j);
		}
		for (auto k : i.indices) {
			idx2++;
			mergeIndices.push_back(k);
		}
	}

	vector<Mesh> mesh3 = obj3.getMesh();
	for (auto i : mesh3) {
		for (auto j : i.vertices) {
			vtx3++;
			mergeVertices.push_back(j);
		}
		for (auto k : i.indices) {
			idx3++;
			mergeIndices.push_back(k);
		}
	}

	vector<Mesh> mesh4 = obj4.getMesh();
	for (auto i : mesh4) {
		for (auto j : i.vertices) {
			vtx4++;
			mergeVertices.push_back(j);
		}
		for (auto k : i.indices) {
			idx4++;
			mergeIndices.push_back(k);
		}
	}


	// initialize the poisson samples
	MyPoissonSample* sample0 = MyPoissonSample::fromFile("assets/poissonPoints_621043_after.ppd2");
	MyPoissonSample* sample1 = MyPoissonSample::fromFile("assets/poissonPoints_1010.ppd2");
	MyPoissonSample* sample2 = MyPoissonSample::fromFile("assets/poissonPoints_1010.ppd2");
	MyPoissonSample* sample3 = MyPoissonSample::fromFile("assets/cityLots_sub_0.ppd2");
	MyPoissonSample* sample4 = MyPoissonSample::fromFile("assets/cityLots_sub_1.ppd2");
	
	// get number of sample
	const int NUM_SAMPLE0 = sample0->m_numSample;
	const int NUM_SAMPLE1 = sample1->m_numSample;
	const int NUM_SAMPLE2 = sample2->m_numSample;
	const int NUM_SAMPLE3 = sample3->m_numSample;
	const int NUM_SAMPLE4 = sample4->m_numSample;

	NUM_TOTAL_INSTANCE = NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2 + NUM_SAMPLE3 + NUM_SAMPLE4;
	printf("NUM_SAMPLE0 = %d\n", NUM_SAMPLE0);
	printf("NUM_SAMPLE1 = %d\n", NUM_SAMPLE1);
	printf("NUM_SAMPLE2 = %d\n", NUM_SAMPLE2);
	printf("NUM_SAMPLE1 = %d\n", NUM_SAMPLE3);
	printf("NUM_SAMPLE2 = %d\n", NUM_SAMPLE4);
	printf("NUM_TOTAL_INSTANCE = %d\n", NUM_TOTAL_INSTANCE);
	// get the sample position buffer (size = num-sample * 3)
	const float* POSITION_BUFFER0 = sample0->m_positions;
	const float* POSITION_BUFFER1 = sample1->m_positions;
	const float* POSITION_BUFFER2 = sample2->m_positions;
	const float* POSITION_BUFFER3 = sample3->m_positions;
	const float* POSITION_BUFFER4 = sample4->m_positions;

	const float* RADIANS_BUFFER0 = sample0->m_radians;
	const float* RADIANS_BUFFER1 = sample1->m_radians;
	const float* RADIANS_BUFFER2 = sample2->m_radians;
	const float* RADIANS_BUFFER3 = sample3->m_radians;
	const float* RADIANS_BUFFER4 = sample4->m_radians;
	//======================================================================
	// initialize VBO and IBO of grass
	//VBO
	//不先把資料放進vbo buffer中

	glGenBuffers(1, &grass_building_vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	//EBO
	glGenBuffers(1, &grass_building_ebo);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	///std::vector<float> rawData;
	// initialize the RAW instance data
	RawInstanceProperties* rawInsData = new RawInstanceProperties[NUM_TOTAL_INSTANCE];
	int i, j, k;
	for (i = 0; i < NUM_SAMPLE0; i++) {
		rawInsData[i].position = vec4(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2],
			0.0);
		rawInsData[i].boundSphere = vec4(0.0, 0.66, 0.0, 1.4);
		
		//計算好transform model，再把transform model存進rawInsData
		vec3 rad = vec3(RADIANS_BUFFER0[i * 3 + 0], RADIANS_BUFFER0[i * 3 + 1], RADIANS_BUFFER0[i * 3 + 2]);
		quat q = quat(rad);
		mat4 rotationMatrix = toMat4(q);
		//vec3 pos = vec3(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2]);
		//transformMatrix = glm::translate(transformMatrix, pos);
		
		rawInsData[i].rotationMatrix = rotationMatrix;
		
		
	}
	for (j = 0; j < NUM_SAMPLE1; j++) {
		rawInsData[j + NUM_SAMPLE0].position = vec4(POSITION_BUFFER1[j * 3 + 0], POSITION_BUFFER1[j * 3 + 1], POSITION_BUFFER1[j * 3 + 2],
			1.0);
		
		rawInsData[j + NUM_SAMPLE0].boundSphere = vec4(0.0, 2.55, 0.0, 3.4);

		vec3 rad = vec3(RADIANS_BUFFER1[j * 3 + 0], RADIANS_BUFFER1[j * 3 + 1], RADIANS_BUFFER1[j * 3 + 2]);
		quat q = quat(rad);
		mat4 rotationMatrix = toMat4(q);
		//vec3 pos = vec3(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2]);
		//transformMatrix = glm::translate(transformMatrix, pos);

		rawInsData[j + NUM_SAMPLE0].rotationMatrix = rotationMatrix;
	}
	for (k = 0; k < NUM_SAMPLE2; k++) {
		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1].position = vec4(POSITION_BUFFER2[k * 3 + 0], POSITION_BUFFER2[k * 3 + 1], POSITION_BUFFER2[k * 3 + 2],
			2.0);

		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1].boundSphere = vec4(0.0, 1.76, 0.0, 2.6);

		vec3 rad = vec3(RADIANS_BUFFER2[k * 3 + 0], RADIANS_BUFFER2[k * 3 + 1], RADIANS_BUFFER2[k * 3 + 2]);
		quat q = quat(rad);
		mat4 rotationMatrix = toMat4(q);
		//vec3 pos = vec3(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2]);
		//transformMatrix = glm::translate(transformMatrix, pos);

		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1].rotationMatrix = rotationMatrix;
	}
	for (k = 0; k < NUM_SAMPLE3; k++) {
		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2].position = vec4(POSITION_BUFFER3[k * 3 + 0], POSITION_BUFFER3[k * 3 + 1], POSITION_BUFFER3[k * 3 + 2],
			3.0);
		
		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2].boundSphere = vec4(0.0, 4.57, 0.0, 8.5);

		vec3 rad = vec3(RADIANS_BUFFER3[k * 3 + 0], RADIANS_BUFFER3[k * 3 + 1], RADIANS_BUFFER3[k * 3 + 2]);
		quat q = quat(rad);
		mat4 rotationMatrix = toMat4(q);
		//vec3 pos = vec3(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2]);
		//transformMatrix = glm::translate(transformMatrix, pos);

		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2].rotationMatrix = rotationMatrix;
		
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				cout << rotationMatrix[i][j] << ' ';
			}
			cout << "\n";
		}
		cout << "\n";
	}
	for (k = 0; k < NUM_SAMPLE4; k++) {
		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2 + NUM_SAMPLE3].position = vec4(POSITION_BUFFER4[k * 3 + 0], POSITION_BUFFER4[k * 3 + 1], POSITION_BUFFER4[k * 3 + 2],
			4.0);

		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2 + NUM_SAMPLE3].boundSphere = vec4(0.0, 4.57, 0.0, 10.2);
	
		vec3 rad = vec3(RADIANS_BUFFER4[k * 3 + 0], RADIANS_BUFFER4[k * 3 + 1], RADIANS_BUFFER4[k * 3 + 2]);
		quat q = quat(rad);
		mat4 rotationMatrix = toMat4(q);
		//vec3 pos = vec3(POSITION_BUFFER0[i * 3 + 0], POSITION_BUFFER0[i * 3 + 1], POSITION_BUFFER0[i * 3 + 2]);
		//transformMatrix = glm::translate(transformMatrix, pos);

		rawInsData[k + NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2 + NUM_SAMPLE3].rotationMatrix = rotationMatrix;
	}
	// prepare a SSBO for storing raw instance data
	GLuint rawInstanceDataBufferHandle;
	glGenBuffers(1, &rawInstanceDataBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, rawInstanceDataBufferHandle);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, NUM_TOTAL_INSTANCE * sizeof(RawInstanceProperties), rawInsData, GL_MAP_READ_BIT);
	//glBufferStorage(GL_SHADER_STORAGE_BUFFER, 10 * sizeof(InstanceProperties), testData, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rawInstanceDataBufferHandle);

	// prepare a SSBO for storing VALID instance data
	GLuint validInstanceDataBufferHandle;
	glGenBuffers(1, &validInstanceDataBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, validInstanceDataBufferHandle);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, NUM_TOTAL_INSTANCE * sizeof(InstanceProperties), nullptr, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, validInstanceDataBufferHandle);

	// prepare a SSBO for storing draw commands
	DrawElementsIndirectCommand drawCommands[5];
	drawCommands[0].count = idx0; //要畫幾個index
	drawCommands[0].instanceCount = 0; //要畫幾個instance。先從0開始，compute shader算好後會加在這裡
	drawCommands[0].firstIndex = 0; //起點index要跳過幾個byte(單位用index數量就好，不用再乘sizeof(float))
	drawCommands[0].baseVertex = 0; //起點vertex要跳過幾個位移量
	drawCommands[0].baseInstance = 0; //第一個instance要從哪裡算 (其面項目的instanceCount)

	drawCommands[1].count = idx1; //要畫幾個index
	drawCommands[1].instanceCount = 0; //要畫幾個instance。先從0開始，compute shader算好後會加在這裡
	drawCommands[1].firstIndex = idx0; //起點index要跳過幾個byte(單位用index數量就好，不用再乘sizeof(float))
	drawCommands[1].baseVertex = vtx0; //起點vertex要跳過幾個位移量
	drawCommands[1].baseInstance = NUM_SAMPLE0; //第一個instance要從哪裡算

	drawCommands[2].count = idx2; //要畫幾個index
	drawCommands[2].instanceCount = 0; //要畫幾個instance。先從0開始，compute shader算好後會加在這裡
	drawCommands[2].firstIndex = idx0 + idx1; //起點index要跳過幾個byte(單位用index數量就好，不用再乘sizeof(float))
	drawCommands[2].baseVertex = vtx0 + vtx1; //起點vertex要跳過幾個位移量
	drawCommands[2].baseInstance = NUM_SAMPLE0 + NUM_SAMPLE1; //第一個instance要從哪裡算

	drawCommands[3].count = idx3; //要畫幾個index
	drawCommands[3].instanceCount = 0; //要畫幾個instance。先從0開始，compute shader算好後會加在這裡
	drawCommands[3].firstIndex = idx0 + idx1 + idx2; //起點index要跳過幾個byte(單位用index數量就好，不用再乘sizeof(float))
	drawCommands[3].baseVertex = vtx0 + vtx1 + vtx2; //起點vertex要跳過幾個位移量
	drawCommands[3].baseInstance = NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2; //第一個instance要從哪裡算

	drawCommands[4].count = idx4; //要畫幾個index
	drawCommands[4].instanceCount = 0; //要畫幾個instance。先從0開始，compute shader算好後會加在這裡
	drawCommands[4].firstIndex = idx0 + idx1 + idx2 + idx3; //起點index要跳過幾個byte(單位用index數量就好，不用再乘sizeof(float))
	drawCommands[4].baseVertex = vtx0 + vtx1 + vtx2 + vtx3; //起點vertex要跳過幾個位移量
	drawCommands[4].baseInstance = NUM_SAMPLE0 + NUM_SAMPLE1 + NUM_SAMPLE2 + NUM_SAMPLE3; //第一個instance要從哪裡算

	//GLuint cmdBufferHandle;
	glGenBuffers(1, &cmdBufferHandle);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cmdBufferHandle);
	//glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(DrawElementsIndirectCommand), drawCommands, GL_MAP_READ_BIT);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, sizeof(drawCommands), drawCommands, GL_MAP_READ_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cmdBufferHandle);

	// initialize VAO
	//glGenVertexArrays(1, &vaoHandle);
	//glBindVertexArray(vaoHandle);
	// bind the VBO and IBO
		//TODO
	glGenVertexArrays(1, &grass_building_vao);
	glBindVertexArray(grass_building_vao);

	glBindBuffer(GL_ARRAY_BUFFER, grass_building_vbo);
	glBufferData(GL_ARRAY_BUFFER, mergeVertices.size() * sizeof(Vertex), &mergeVertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grass_building_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mergeIndices.size() * sizeof(unsigned int), &mergeIndices[0], GL_STATIC_DRAW);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	//Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);
	//TexCoords
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);




	// SSBO as vertex shader attribute
	//glBindBuffer(GL_ARRAY_BUFFER, rawInstanceDataBufferHandle);
	glBindBuffer(GL_ARRAY_BUFFER, validInstanceDataBufferHandle);
	glVertexAttribPointer(3, 4, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);
	// SSBO as draw-indirect-buffer
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);
	//glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCommands), drawCommands, GL_DYNAMIC_DRAW);
	//

	// done
	glBindVertexArray(0);
}

void renderGrassBuilding() {
	mat4 Identy_Init(1.0);
	model_matrix = Identy_Init;
	glUniformMatrix4fv(model_location, 1, GL_FALSE, &model_matrix[0][0]);

    glActiveTexture(GL_TEXTURE0 + 24);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayHandle);

	glUniform1i(SceneManager::Instance()->m_fs_pixelProcessIdHandle, 12);
	glUniform1i(SceneManager::Instance()->m_vs_vertexProcessIdHandle, 12);
	shaderProgram->useProgram();

	glBindVertexArray(grass_building_vao);
	//glBindBuffer(GL_DRAW_INDIRECT_BUFFER, cmdBufferHandle);

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, 5, 0);

	//glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, 10);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glDrawElements(GL_TRIANGLES, (indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
}

void activeComputeShader(mat4 view_projection_matrix) {
	reset_cs_shader_program->useProgram();
	glDispatchCompute(5, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// shader program for collecting visible instances
	culling_cs_shader_program->useProgram();

	// send the necessary information to compute shader (must after useProgram)
	glUniform1i(NUM_TOTAL_INSTANCE_LOCATION, NUM_TOTAL_INSTANCE);
	//mat4 view_projection_matrix = playerProjMat * playerViewMat;
	glUniformMatrix4fv(7, 1, false, &view_projection_matrix[0][0]);

	// start GPU process
	glDispatchCompute((NUM_TOTAL_INSTANCE / 1024) + 1, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

GLuint gbuf, gbuf_tex[4], final_vao;
void reinitFB() {
	glBindFramebuffer(GL_FRAMEBUFFER, gbuf);

	glGenTextures(4, gbuf_tex);
	glBindTexture(GL_TEXTURE_2D, gbuf_tex[0]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, FRAME_WIDTH, FRAME_HEIGHT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuf_tex[1]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, FRAME_WIDTH, FRAME_HEIGHT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuf_tex[2]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, FRAME_WIDTH, FRAME_HEIGHT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, gbuf_tex[3]);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, FRAME_WIDTH, FRAME_HEIGHT);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gbuf_tex[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gbuf_tex[1], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gbuf_tex[2], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbuf_tex[3], 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initFB() {
	glGenFramebuffers(1, &gbuf);

	reinitFB();

	glGenVertexArrays(1, &final_vao);
	glBindVertexArray(final_vao);

	const GLfloat wvtx[16] = {
		-1.0f, -1.0f,
		  0.0f, 0.0f,
		-1.0f, 1.0f,
		  0.0f, 1.0f,
		1.0f, -1.0f,
		  1.0f, 0.0f,
		1.0f, 1.0f,
		  1.0f, 1.0f,
	};

	GLuint buf;
	glGenBuffers(1, &buf);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), wvtx, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4, (const GLvoid*)(sizeof(GLfloat) * 2));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

unsigned int features = 1;

bool initializeGL(){

	initAirplane();
	initRock();
	initGrassBuilding();
	initTexture();

	initFB();

	// initialize shader program
	// vertex shader
	Shader* vsShader = new Shader(GL_VERTEX_SHADER);
	vsShader->createShaderFromFile("src\\shader\\oglVertexShader.glsl");
	std::cout << vsShader->shaderInfoLog() << "\n";

	// fragment shader
	Shader* fsShader = new Shader(GL_FRAGMENT_SHADER);
	fsShader->createShaderFromFile("src\\shader\\oglFragmentShader.glsl");
	std::cout << fsShader->shaderInfoLog() << "\n";

	// shader program
	shaderProgram = new ShaderProgram();
	shaderProgram->init();
	shaderProgram->attachShader(vsShader);
	shaderProgram->attachShader(fsShader);
	shaderProgram->checkStatus();
	if (shaderProgram->status() != ShaderProgramStatus::READY) {
		return false;
	}
	shaderProgram->linkProgram();

	vsShader->releaseShader();
	fsShader->releaseShader();
	
	delete vsShader;
	delete fsShader;

	auto* defVsShader = new Shader(GL_VERTEX_SHADER);
	defVsShader->createShaderFromFile("src\\shader\\def_vert.glsl");
	std::cout << defVsShader->shaderInfoLog() << std::endl;

	auto* defFsShader = new Shader(GL_FRAGMENT_SHADER);
	defFsShader->createShaderFromFile("src\\shader\\def_frag.glsl");
	std::cout << defFsShader->shaderInfoLog() << std::endl;

	defShaderProgram = new ShaderProgram();
	defShaderProgram->init();
	defShaderProgram->attachShader(defVsShader);
	defShaderProgram->attachShader(defFsShader);
	defShaderProgram->checkStatus();
	if (defShaderProgram->status() != ShaderProgramStatus::READY) {
		return false;
	}
	defShaderProgram->linkProgram();

	defVsShader->releaseShader();
	defFsShader->releaseShader();

	delete defVsShader;
	delete defFsShader;

	//reset Compute shader

	Shader* reset_cs_shader = new Shader(GL_COMPUTE_SHADER);
	reset_cs_shader->createShaderFromFile("src\\shader\\reset_cs_shader.glsl");
	std::cout << reset_cs_shader->shaderInfoLog() << "\n";

	reset_cs_shader_program = new ShaderProgram();
	reset_cs_shader_program->init();
	reset_cs_shader_program->attachShader(reset_cs_shader);
	reset_cs_shader_program->checkStatus();
	if (reset_cs_shader_program->status() != ShaderProgramStatus::READY) {
		return false;
	}
	reset_cs_shader_program->linkProgram();

	reset_cs_shader->releaseShader();

	delete reset_cs_shader;

	//culling Compute shader
	Shader* culling_cs_shader = new Shader(GL_COMPUTE_SHADER);
	culling_cs_shader->createShaderFromFile("src\\shader\\culling_cs_shader.glsl");
	std::cout << culling_cs_shader->shaderInfoLog() << "\n";

	culling_cs_shader_program = new ShaderProgram();
	culling_cs_shader_program->init();
	culling_cs_shader_program->attachShader(culling_cs_shader);
	culling_cs_shader_program->checkStatus();
	if (culling_cs_shader_program->status() != ShaderProgramStatus::READY) {
		return false;
	}
	culling_cs_shader_program->linkProgram();

	NUM_TOTAL_INSTANCE_LOCATION = glGetUniformLocation(culling_cs_shader_program->programId(), "numMaxInstance");
	culling_cs_shader->releaseShader();

	delete culling_cs_shader;


	//uniform location setup============================================
	albedo_location = glGetUniformLocation(shaderProgram->programId(), "albedoTextureArray");
	glUniform1i(albedo_location, 1);

	model_location = glGetUniformLocation(shaderProgram->programId(), "modelMat");
	view_location = glGetUniformLocation(shaderProgram->programId(), "viewMat");
	proj_location = glGetUniformLocation(shaderProgram->programId(), "projMat");
	features_loc = glGetUniformLocation(defShaderProgram->programId(), "features");
	// =================================================================
	// init renderer
	defaultRenderer = new SceneRenderer();
	if (!defaultRenderer->initialize(FRAME_WIDTH, FRAME_HEIGHT, shaderProgram)) {
		return false;
	}

	// =================================================================
	// initialize camera
	m_myCameraManager = new INANOA::MyCameraManager();
	m_myCameraManager->init(FRAME_WIDTH, FRAME_HEIGHT);
	
	// initialize view frustum
	m_viewFrustumSO = new ViewFrustumSceneObject(2, SceneManager::Instance()->m_fs_pixelProcessIdHandle, SceneManager::Instance()->m_fs_pureColor);
	defaultRenderer->appendDynamicSceneObject(m_viewFrustumSO->sceneObject());

	// initialize terrain
	m_terrain = new MyTerrain();
	m_terrain->init(-1); 
	defaultRenderer->appendTerrainSceneObject(m_terrain->sceneObject());
	// =================================================================	
	
	resize(FRAME_WIDTH, FRAME_HEIGHT);
	
	m_imguiPanel = new MyImGuiPanel();		
	
	return true;
}
void resizeGL(GLFWwindow *window, int w, int h){
	resize(w, h);
}

void featureUI(const char* name, int idx)
{
	bool b = (features & (1 << idx)) != 0;
	if (ImGui::Button(name)) {
		if (!b)
			features |= (1 << idx);
		else
			features &= ~(1 << idx);
	}
	ImGui::SameLine(0.0f, 1.0f);
	if (b)
		ImGui::TextUnformatted("ON");
	else
		ImGui::TextUnformatted("OFF");
}

void paintGL(){
	// update cameras and airplane
	// god camera
	m_myCameraManager->updateGodCamera();
	// player camera
	m_myCameraManager->updatePlayerCamera();
	const glm::vec3 PLAYER_CAMERA_POSITION = m_myCameraManager->playerViewOrig();
	m_myCameraManager->adjustPlayerCameraHeight(m_terrain->terrainData()->height(PLAYER_CAMERA_POSITION.x, PLAYER_CAMERA_POSITION.z));
	// airplane
	m_myCameraManager->updateAirplane();
	const glm::vec3 AIRPLANE_POSTION = m_myCameraManager->airplanePosition();
	m_myCameraManager->adjustAirplaneHeight(m_terrain->terrainData()->height(AIRPLANE_POSTION.x, AIRPLANE_POSTION.z));

	// prepare parameters
	const glm::mat4 playerVM = m_myCameraManager->playerViewMatrix();
	const glm::mat4 playerProjMat = m_myCameraManager->playerProjectionMatrix();
	const glm::vec3 playerViewOrg = m_myCameraManager->playerViewOrig();

	const glm::mat4 godVM = m_myCameraManager->godViewMatrix();
	const glm::mat4 godProjMat = m_myCameraManager->godProjectionMatrix();

	const glm::mat4 airplaneModelMat = m_myCameraManager->airplaneModelMatrix();

	// (x, y, w, h)
	const glm::ivec4 playerViewport = m_myCameraManager->playerViewport();

	// (x, y, w, h)
	const glm::ivec4 godViewport = m_myCameraManager->godViewport();

	// ====================================================================================
	// update player camera view frustum
	m_viewFrustumSO->updateState(playerVM, playerViewOrg);

	// update geography
	m_terrain->updateState(playerVM, playerViewOrg, playerProjMat, nullptr);
	// =============================================
	// compute shader	
	mat4 view_projection_matrix = playerProjMat * playerVM;
	activeComputeShader(view_projection_matrix);

	// =============================================
	// start rendering
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// start new frame
	defaultRenderer->setViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	defaultRenderer->startNewFrame();

	const GLenum dbufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gbuf);
	glDrawBuffers(3, dbufs);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	const GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glClearBufferfv(GL_COLOR, 0, white);
	glClearBufferfv(GL_COLOR, 1, white);
	glClearBufferfv(GL_COLOR, 2, white);
	glClearBufferfv(GL_DEPTH, 0, white);

	// rendering with player view		
	defaultRenderer->setViewport(playerViewport[0], playerViewport[1], playerViewport[2], playerViewport[3]);
	defaultRenderer->setView(playerVM);
	defaultRenderer->setProjection(playerProjMat);
	defaultRenderer->renderPass();

	renderAirplane(airplaneModelMat);
	renderRock();

	renderGrassBuilding();

	// rendering with god view
	defaultRenderer->setViewport(godViewport[0], godViewport[1], godViewport[2], godViewport[3]);
	defaultRenderer->setView(godVM);
	defaultRenderer->setProjection(godProjMat);
	defaultRenderer->renderPass();

	renderAirplane(airplaneModelMat);
	renderRock();

	renderGrassBuilding();
	// ===============================
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	defShaderProgram->useProgram();
	glUniform1ui(features_loc, features);
	glViewport(0, 0, FRAME_WIDTH, FRAME_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glBindVertexArray(final_vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuf_tex[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuf_tex[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gbuf_tex[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ImGui::Begin("My name is window");
	m_imguiPanel->update();

	if (ImGui::Button("SHADED"))
		features = (1 << 0);
	ImGui::SameLine(0, 1.0);
	if (ImGui::Button("WORLDSPACE"))
		features = (1 << 13);
	ImGui::SameLine(0, 1.0);
	if (ImGui::Button("NORMAL"))
		features = (1 << 14);
	ImGui::SameLine(0, 1.0);
	if (ImGui::Button("SPECULAR"))
		features = (1 << 15);

	featureUI("Blinn-Phong", 0);

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

////////////////////////////////////////////////
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_LEFT, cursorPos[0], cursorPos[1]);
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		m_myCameraManager->mousePress(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		m_myCameraManager->mouseRelease(RenderWidgetMouseButton::M_RIGHT, cursorPos[0], cursorPos[1]);
	}
}
void cursorPosCallback(GLFWwindow* window, double x, double y){
	cursorPos[0] = x;
	cursorPos[1] = y;

	m_myCameraManager->mouseMove(x, y);
}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
	auto setKeyStatus = [](const RenderWidgetKeyCode code, const int action) {
		if (action == GLFW_PRESS) {
			m_myCameraManager->keyPress(code);
		}
		else if (action == GLFW_RELEASE) {
			m_myCameraManager->keyRelease(code);
		}
	};

	// =======================================
	if (key == GLFW_KEY_W) { setKeyStatus(RenderWidgetKeyCode::KEY_W, action); }
	else if (key == GLFW_KEY_A) { setKeyStatus(RenderWidgetKeyCode::KEY_A, action); }
	else if (key == GLFW_KEY_S) { setKeyStatus(RenderWidgetKeyCode::KEY_S, action); }
	else if (key == GLFW_KEY_D) { setKeyStatus(RenderWidgetKeyCode::KEY_D, action); }
	else if (key == GLFW_KEY_T) { setKeyStatus(RenderWidgetKeyCode::KEY_T, action); }
	else if (key == GLFW_KEY_Z) { setKeyStatus(RenderWidgetKeyCode::KEY_Z, action); }
	else if (key == GLFW_KEY_X) { setKeyStatus(RenderWidgetKeyCode::KEY_X, action); }


	else if (key == GLFW_KEY_0) { m_myCameraManager->teleport(0); }
	else if (key == GLFW_KEY_1) { m_myCameraManager->teleport(1); }
	else if (key == GLFW_KEY_2) { m_myCameraManager->teleport(2); }

	else if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); }
}
void mouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {}

void updateWhenPlayerProjectionChanged(const float nearDepth, const float farDepth) {
	// get view frustum corner
	const int NUM_CASCADE = 2;
	const float HY = 0.0;

	float dOffset = (farDepth - nearDepth) / NUM_CASCADE;
	float *corners = new float[(NUM_CASCADE + 1) * 12];
	std::vector<float> depths(NUM_CASCADE + 1);
	for (int i = 0; i < NUM_CASCADE; i++) {
		depths[i] = nearDepth + dOffset * i;
	}
	depths[NUM_CASCADE] = farDepth;
	// get viewspace corners
	glm::mat4 tView = glm::lookAt(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	// calculate corners of view frustum cascade
	viewFrustumMultiClipCorner(depths, tView, m_myCameraManager->playerProjectionMatrix(), corners);
	
	// update view frustum scene object
	for (int i = 0; i < NUM_CASCADE + 1; i++) {
		float *layerBuffer = m_viewFrustumSO->cascadeDataBuffer(i);
		for (int j = 0; j < 12; j++) {
			layerBuffer[j] = corners[i * 12 + j];
		}
	}
	m_viewFrustumSO->updateDataBuffer();

	delete corners;
}
void resize(const int w, const int h) {
	FRAME_WIDTH = w;
	FRAME_HEIGHT = h;

	reinitFB();

	m_myCameraManager->resize(w, h);
	defaultRenderer->resize(w, h);
	updateWhenPlayerProjectionChanged(0.1, m_myCameraManager->playerCameraFar());
}
void viewFrustumMultiClipCorner(const std::vector<float> &depths, const glm::mat4 &viewMat, const glm::mat4 &projMat, float *clipCorner) {
	const int NUM_CLIP = depths.size();

	// Calculate Inverse
	glm::mat4 viewProjInv = glm::inverse(projMat * viewMat);

	// Calculate Clip Plane Corners
	int clipOffset = 0;
	for (const float depth : depths) {
		// Get Depth in NDC, the depth in viewSpace is negative
		glm::vec4 v = glm::vec4(0, 0, -1 * depth, 1);
		glm::vec4 vInNDC = projMat * v;
		if (fabs(vInNDC.w) > 0.00001) {
			vInNDC.z = vInNDC.z / vInNDC.w;
		}
		// Get 4 corner of clip plane
		float cornerXY[] = {
			-1, 1,
			-1, -1,
			1, -1,
			1, 1
		};
		for (int j = 0; j < 4; j++) {
			glm::vec4 wcc = {
				cornerXY[j * 2 + 0], cornerXY[j * 2 + 1], vInNDC.z, 1
			};
			wcc = viewProjInv * wcc;
			wcc = wcc / wcc.w;

			clipCorner[clipOffset * 12 + j * 3 + 0] = wcc[0];
			clipCorner[clipOffset * 12 + j * 3 + 1] = wcc[1];
			clipCorner[clipOffset * 12 + j * 3 + 2] = wcc[2];
		}
		clipOffset = clipOffset + 1;
	}
}
