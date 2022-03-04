#include <cmath>
#include <algorithm>
#include <math.h>
#include <random>
#include <unordered_set>
#include <map>
#include "shader.h"
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm\glm.hpp>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>

const size_t numVerts{ 6 };
const size_t numVAOs{ 1 };
const size_t numVBOs{ 2 };

float cameraX, cameraY, cameraZ;
float cubeLocX, cubeLocY, cubeLocZ;
glm::vec3 origin(0.0f, 0.0f, 0.0f);
glm::vec3 cameraLoc(0.0f, 0.0f, 0.74f);
GLuint renderingProgram;
GLuint vao[numVAOs];
GLuint vbo[numVBOs];
GLuint mazeTex;

//Allocate variables used in draw() function, so that they won’t need to be allocated during rendering
GLuint mvLoc, projLoc;
int width, height;
float aspect;
glm::mat4 pMat, vMat, mMat, mvMat;

//Grid size
const size_t texW{ 65 };
const size_t texH{ 65 };

//Holds color data
static GLubyte mazeTexData[texH][texW][4];
//Assigns a cel's groupid
static size_t mazeMetaData[texH][texW]{ 0 };

//Random number generator
inline size_t RNG(size_t lower, size_t upper) {
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<size_t> d(lower, upper);
	return d(rng);
}

//Sets Cel color given col & row
inline void setColor(size_t col, size_t row, size_t r, size_t g, size_t b) {
	mazeTexData[col][row][0] = (GLubyte)r;
	mazeTexData[col][row][1] = (GLubyte)g;
	mazeTexData[col][row][2] = (GLubyte)b;
	mazeTexData[col][row][3] = (GLubyte)255;
}

//load the sequential byte data array into a texture object
void setupVertices(void) {
	//basic plane
	float vertexPositions[numVerts*3] = {
	 -1.0f, 1.0f, -1.0f,	-1.0f, -1.0f, -1.0f,	1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,	 1.0f, 1.0f, -1.0f,		-1.0f, 1.0f, -1.0f,
	};
	float texCoordinates[12] = {
	 0.0f, 0.0f,	0.0f, 1.0f,		1.0f, 1.0f,
	 1.0f, 1.0f,	1.0f, 0.0f,		0.0f, 0.0f
	};
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);
	glGenBuffers(numVBOs, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoordinates), texCoordinates, GL_STATIC_DRAW);
}


/*
key: group id
value: set of group ids that key group has already merged with
purpose: Avoid merging of groups that have already been merged
*/
std::map<size_t, std::unordered_set<size_t>> merged{};
/*
key: group id 
value: list of group's cels at cur row
purpose: Making bridges from previous row
*/
std::map<size_t, std::vector<size_t>> rGroupsPrev{}; 

//Purpose: Performs Eller's maze algo
void mazeGen()
{
	size_t row, col;//row & column counter
	size_t groupid{};//id counter

	//loop only odd columns
	for (col = 1; col < texH - 1; col = col + 2) {
		std::map<size_t, std::vector<size_t>> rGroupsCur{}; //map used for current row's groups and associated cels

		//if we're starting on anything but the 1st row then time to make bridges
		if (col > 1) {
			//loop through the groups from previous row
			for (auto const&[id, cels] : rGroupsPrev)
			{
				//if they have cels then make at least 1 bridge for them
				if (!cels.empty()) {
					size_t numConnections = 1;
					//25 percent chance the group will only have 1 or 2 connections
					if (RNG(1, 100) < 25)
						numConnections = RNG(1, 2);
					
					//Create the bridges
					for (size_t i = 0; i < numConnections; i++)
					{
						//pick a random spot within the group
						size_t bridge = cels.at(RNG(0, cels.size() - 1));

						//change their and the cel below color to white
						setColor(col - 1, bridge, 255, 255, 255);
						setColor(col, bridge, 255, 255, 255);

						//update their cel groupid in array
						mazeMetaData[col][bridge] = id;
						mazeMetaData[col - 1][bridge] = id;

						//check if groupid is already a key 
						if (rGroupsCur.find(id) == rGroupsCur.end())
							rGroupsCur.insert(std::pair<size_t, std::vector<size_t>>(id, std::vector<size_t>{bridge})); // not found
						else
							rGroupsCur[id].push_back(bridge); //found: push it into it's vector value
					}
				}
			}
		}
		//Loop through the row
		for (row = 1; row < texW - 1; row++) {
			//if row is even & cur cel & it's adjacent cels are black then we make a new group
			if (row % 2 && mazeMetaData[col][row] == 0 && mazeMetaData[col][row - 1] == 0 && mazeMetaData[col][row + 1] == 0) {
				//update the groupid counter
				groupid++;

				setColor(col, row, 255, 255, 255); //make cel white
				mazeMetaData[col][row] = groupid; //update array

				//Create entry for grouip in rGroups map
				rGroupsCur.insert(std::pair<size_t, std::vector<size_t>>(groupid, std::vector<size_t>{row}));
				merged[groupid].insert(groupid); //add the new group to merged map
			}

		}

		//Expand the groups in the cur row
		for (size_t row = 1; row < texW - 1; row++) {
			size_t id{ mazeMetaData[col][row] };//id of group being expanded

			//70 percent chance that it will get expanded
			if (RNG(1, 100) <= 70 && id != 0) {
				size_t amt = RNG(1, (texW - 1)/16);//how many groups to merge into this id

				//check if we can satisfy the demand
				if (amt * 2 < ((texW - 1) - row)) {
					row++; // go to the next cel of the group we are expanding

					//keep looping until we've expanded the groups
					while (amt != 0)
					{
						size_t celID{ mazeMetaData[col][row] };//id of current cel
						size_t nxtCelID{ mazeMetaData[col][row + 1] };//id of next cel

						if (nxtCelID != 0 && celID == 0) {//if the next cel is already in a group & the prev is not
							bool collide{};
							//compare the groups already linked to the expanding id w/ the next groups linked ids.
							for (const auto& elem : merged[nxtCelID]) {
								if (merged[id].find(elem) != merged[id].end()) {
									collide = true;
									break;
								}
							}
							//if they are already linked then break
							if (collide) { row++; break; }
						}

						setColor(col, row, 255, 255, 255); //change the cel to white
						
						//Remove the cels from merged group
						if (rGroupsCur.find(celID) != rGroupsCur.end())
							rGroupsCur[celID].erase(std::remove(rGroupsCur[celID].begin(), rGroupsCur[celID].end(), row), rGroupsCur[celID].end());

						mazeMetaData[col][row] = id;
						
						//we merged a group do bookkeeping
						if (celID != 0 && celID != id) {
							rGroupsCur[id].push_back(row);
							merged[id].insert(merged[celID].begin(), merged[celID].end());
							merged[celID].insert(merged[id].begin(), merged[id].end());
							amt--;
						}

						row++;
					}
				}
			}
		}
		rGroupsPrev = rGroupsCur;
	}
}

void init(GLFWwindow* window) {
	shader mazeShader{ "vShader.glsl", "fShader.glsl" };
	renderingProgram = mazeShader.getProg();

	setupVertices();

	mazeGen();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &mazeTex);
	glBindTexture(GL_TEXTURE_2D, mazeTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW,
		texH, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		mazeTexData);

}

void draw(GLFWwindow* window, double currentTime) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	//glClearColor(255, 0, 0, 255.0f);
	glUseProgram(renderingProgram);
	// get the uniform variables for the MV and projection matrices
	mvLoc = glGetUniformLocation(renderingProgram, "mv_matrix");
	projLoc = glGetUniformLocation(renderingProgram, "proj_matrix");
	// build perspective matrix
	glfwGetFramebufferSize(window, &width, &height);
	aspect = (float)width / (float)height;
	pMat = glm::perspective(1.0472f, aspect, 0.1f, 1000.0f); // 1.0472 radians = 60 degrees
	// build view matrix, model matrix, and model-view matrix
	//glm::mat4 projection = glm::ortho(0.0f, 4.0f, 4.0f, 0.0f, -5.0f, 5.0f);
	vMat = glm::translate(glm::mat4(1.0f), -cameraLoc);
	mMat = glm::translate(glm::mat4(1.0f), origin);
	mvMat = vMat * mMat;

	// copy perspective and MV matrices to corresponding uniform variables
	glUniformMatrix4fv(mvLoc, 1, GL_FALSE, glm::value_ptr(mvMat));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pMat));

	// associate VBO with the corresponding vertex attribute in the vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	// adjust OpenGL settings and draw model
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mazeTex);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDrawArrays(GL_TRIANGLES, 0, numVerts);
}

int main(void) {
	if (!glfwInit()) { exit(EXIT_FAILURE); }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow* window = glfwCreateWindow(512, 512, "Maze Generation", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	init(window);
	while (!glfwWindowShouldClose(window)) {
		draw(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}