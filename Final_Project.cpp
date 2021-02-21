/*
 * Software Engineering/ Design Enhancement
 * CS 499
 * February 9, 2021
 * William Valdes
*/

#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "SOIL2/SOIL2.h"


// GLM Math Header inclusions
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;

#define WINDOW_TITLE "Final Project - Knife"

// Vertex and Fragment Shader Source Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

//Global variable declarations
int view_state = 1;

// Variables declarations for shader, window width and height, buffer and array objects
GLint KnifeShaderProgram, lampShaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint texture, VBO, KnifeVAO, LightVAO;
GLfloat cameraSpeed = 0.0005f; // Camera movement speed
GLchar currentKey; // Storing pressed key
GLfloat lastMouseX = 400, lastMouseY = 300; // Sets mouse cursor to the center
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // Mouse yaw and pitch
GLfloat sensitivity = 0.5f;
bool mouseDetected = true;

// Subject position and scale
glm::vec3 KnifePosition(0.0f, 0.0f, 0.0f);
glm::vec3 KnifeScale(1.0f);

//Global vector declarations
glm::vec3 cameraPosition(0.0f, 0.0f, 0.0f);
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, 0.0f); // Temporary z unit vector
glm::vec3 front; //Temporary z unit vector for mouse

// Knife and light color
glm::vec3 objectColor(0.6f, 0.5f, 0.75f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

// Light position and scale
glm::vec3 lightPosition(0.5f, 0.5f, -3.0f);
glm::vec3 lightScale(0.4f);



// Camera rotation
float cameraRotation = glm::radians(-25.0f);

// Function prototypes
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void pressSpecialKey(int key, int xx, int yy);
void UMouseMove(int x, int y);

/*Vertex Shader Program Source Code*/
const GLchar * KnifeVertexShaderSource = GLSL(330,

  layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
  layout(location = 1) in vec3 normal; // VAP position 1 for normals
  layout(location = 2) in vec2 textureCoordinate;

  out vec3 FragmentPos; // For outgoing color / pixels to fragment shader
  out vec3 Normal; // For outgoing normals to fragment shader
  out vec2 mobileTextureCoordinate; // For outgoing texture coordinates

    // Uniform Global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

void main() {

		gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms into clip coordinates
		FragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
		Normal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1 - textureCoordinate.y);

	}
);

// Fragment Shader Source Code
const GLchar * KnifeFragmentShaderSource = GLSL(330,

		in vec3 FragmentPos; // For incoming fragment position
		in vec3 Normal; // For incoming normals
		in vec2 mobileTextureCoordinate;

		out vec4 KnifeColor; // For outgoing geometry color to the GPU

		// Uniform / Global variables for object color, light color, light position, and camera/view position

		uniform vec3 lightColor;
		uniform vec3 lightPos;
		uniform vec3 viewPosition;
		uniform sampler2D uTexture;

	void main() {

		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		// Calculate Ambient lighting
		float ambientStrength = 1; // Set ambient or global lighting strength
		vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

		// Calculate Diffuse lighting
		vec3 norm = normalize(Normal); // Normalize vectors to 1 unit
		vec3 lightDirection = normalize(lightPos - FragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on
		float impact = max(dot(norm, lightDirection), 0.0); // Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor; // Generate diffuse light color

		// Calculate Specular lighting
		float specularIntensity = 0.8f; // Set specular light strength
		float highlightSize = 32.0f; // Set specular highlight size
		vec3 viewDir = normalize(viewPosition - FragmentPos); // Calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm); // Calculate reflection vector

		// Calculate Specular Component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		// Calculate phong result
		vec3 objectColor = texture(uTexture, mobileTextureCoordinate).xyz;
		vec3 phong = (ambient + diffuse + specular) * objectColor;

		KnifeColor = vec4(phong, 1.0f); // Send lighting results to GPU
  }
);

/* Lamp Shader Source Code */
const GLchar * lampVertexShaderSource = GLSL(330,

		layout (location = 0) in vec3 position; // VAP position 0 for vertex position data

		// Uniform / Global variables for the transform matrices
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		void main()
		{
			gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
		}
);

/* Fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(330,

		out vec4 color; // For outgoing lamp color (smaller Knife) to the GPU

		void main()
		{
			color = vec4(1.0f); // Set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0
		}
);

// main function. Entry point to the OpenGL program
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);
	glutReshapeFunc(UResizeWindow);
	glewExperimental = GL_TRUE;
			if (glewInit() != GLEW_OK)
			{
				std::cout << "Failed to initialize GLEW" << std::endl;
				return -1;
			}

	UCreateShader();
	UCreateBuffers();
	UGenerateTexture();

	// Set background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glutDisplayFunc(URenderGraphics);
    glutSpecialFunc(pressSpecialKey);
    glutPassiveMotionFunc(UMouseMove);

	// Create and run OpenGL Loop
	glutMainLoop();

	// Deletes buffer objects once used
	glDeleteVertexArrays(1, &KnifeVAO);
	glDeleteVertexArrays(1, &LightVAO);
	glDeleteBuffers(1, &VBO);
	return 0;
}

// Resize windiw to fit primitives
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

// Graphics rendering
void URenderGraphics(void)
{
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLint uTextureLoc, lightColorLoc, lightPositionLoc, viewPositionLoc, modelLoc, viewLoc, projLoc, objectColorLoc;

	// Declare a 4x4 identity matrix uniform variable to handle transformations
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;

	// Use the Knife Shader and activate the Knife Vertex Array Object for rendering and transforming
	glUseProgram(KnifeShaderProgram);

	// Activate the vertex array object before rendering and transforming them
	glBindVertexArray(KnifeVAO);
    CameraForwardZ = front;

    //Transforms the object
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Place the object at the center of the viewport
    model = glm::rotate(model, 45.0f, glm:: vec3(0.0, 1.0f, 0.0f)); // Rotate the object 45 degrees on the X
    model = glm::scale(model, KnifeScale);

	// Transform the camera
    view = glm::lookAt(cameraPosition - CameraForwardZ, cameraPosition, CameraUpY);

	// Perspective projection
	projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

    if(view_state == 1){
        projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
    }else if(view_state == 0){
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

	// Retrieves and passes transform to the shader program
	modelLoc = glGetUniformLocation(KnifeShaderProgram, "model");
	viewLoc = glGetUniformLocation(KnifeShaderProgram, "view");
	projLoc = glGetUniformLocation(KnifeShaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms from the Knife Shader program for the Knife color, light color, light position, and camera position
	uTextureLoc = glGetUniformLocation(KnifeShaderProgram, "uTexture");
	objectColorLoc = glGetUniformLocation(KnifeShaderProgram, "objectColor");
	lightColorLoc = glGetUniformLocation(KnifeShaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(KnifeShaderProgram, "lightPos");
	viewPositionLoc = glGetUniformLocation(KnifeShaderProgram, "viewPosition");

	// Pass color, light, and camera data to the Knife Shader program's corresponding uniforms
	glUniform1i(uTextureLoc, 0); // texture unit 0
	glUniform3f(objectColorLoc, objectColor.r, objectColor.g, objectColor.b);
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLES, 0, 198); // Draw the primitives / Knife

	glBindVertexArray(0); // Deactivate the Knife Vertex Array Object

	// Use the Lamp Shader and activate the Lamp Vertex Array Object for rendering and transforming
	glUseProgram(lampShaderProgram);
	glBindVertexArray(LightVAO);

	// Transform the smaller Knife used as a visual cue for the light source
	model = glm::translate(model, lightPosition);
	model = glm::scale(model, lightScale);

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(lampShaderProgram, "model");
	viewLoc = glGetUniformLocation(lampShaderProgram, "view");
	projLoc = glGetUniformLocation(lampShaderProgram, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 198);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glutPostRedisplay();
	glutSwapBuffers();

}

// Creates the shader program
void UCreateShader()
{
	// Knife Vertex shader
	GLint KnifeVertexShader = glCreateShader(GL_VERTEX_SHADER); // Creates the Vertex shader
	glShaderSource(KnifeVertexShader, 1, &KnifeVertexShaderSource, NULL); // Attaches the Vertex shader to the source code
	glCompileShader(KnifeVertexShader); // Compiles the Vertex shader

	// Knife Fragment shader
	GLint KnifeFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Creates the Fragment shader
	glShaderSource(KnifeFragmentShader, 1, &KnifeFragmentShaderSource, NULL); // Attaches the Fragment shader to the source code
	glCompileShader(KnifeFragmentShader); // Compiles the Fragment shader

	// Knife Shader program
	KnifeShaderProgram = glCreateProgram(); // Creates the Shader program and returns an id
	glAttachShader(KnifeShaderProgram, KnifeVertexShader); // Attach Vertex shader to the Shader program
	glAttachShader(KnifeShaderProgram, KnifeFragmentShader);; // Attach Fragment shader to the Shader program
	glLinkProgram(KnifeShaderProgram); // Link Vertex and Fragment shaders to Shader program

	// Delete the Knife shaders once linked
	glDeleteShader(KnifeVertexShader);
	glDeleteShader(KnifeFragmentShader);

	// Lamp Vertex shader
	GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER); // Creates the Vertex shader
	glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL); // Attaches the Vertex shader to the source code
	glCompileShader(lampVertexShader); // Compiles the Vertex shader

	// Lamp Fragment shader
	GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); // Creates the Fragment shader
	glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL); // Attaches the Fragment shader to the source code
	glCompileShader(lampFragmentShader); // Compiles the Fragment shader

	// Lamp Shader program
	lampShaderProgram = glCreateProgram(); // Creates the Shader program and returns an id
	glAttachShader(lampShaderProgram, lampVertexShader); // Attach Vertex shader to the Shader program
	glAttachShader(lampShaderProgram, lampFragmentShader);; // Attach Fragment shader to the Shader program
	glLinkProgram(lampShaderProgram); // Link Vertex and Fragment shaders to Shader program

	// Delete the vertex and fragment shaders once linked
	glDeleteShader(lampVertexShader);
	glDeleteShader(lampFragmentShader);

}

// creates the buffer and array objects
void UCreateBuffers()
{

	// Position and Texture coordinate data for 66 triangles
	GLfloat vertices[] = {
									// Positions

		    // Knife handle

		    // Vertex Positions  	// Normals				// Texture

		    // Handle Base Back
		   -3.0f,  -1.0f,  -0.2f,	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.4f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40,   0.0f,  // 1
		   -3.4f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.4f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -3.0f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Base Front
		   -3.0f,  -1.0f,   0.0f, 	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.4f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,	    0.40f,  0.0f,  // 1
		   -3.4f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.4f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -3.0f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Base Blade Side
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.0f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.0f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -3.0f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Base End Side
		   -3.4f,   0.1f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 0
		   -3.4f,   0.1f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  0.0f,  // 1
		   -3.4f,  -1.1f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 2
		   -3.4f,  -1.1f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 3
		   -3.4f,  -1.1f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   1.0f,  // 4
		   -3.4f,   0.1f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

		    // Handle Base Bottom
		   -3.0f,  -1.0f,  -0.2f,   0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.4f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.4f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.4f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -3.0f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -3.0f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Base Top
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.4f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.4f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.4f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

		    // Handle Main Back
		   -0.6f,  -1.0f,  -0.2f,	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.0f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -0.6f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Main Front
		   -0.6f,  -1.0f,   0.0f, 	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.0f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,	    0.40f,  0.0f,  // 1
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -0.6f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Main Blade Side
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -0.6f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -0.6f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -0.6f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Main End Side
		   -3.0f,   0.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 0
		   -3.0f,   0.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  0.0f,  // 1
		   -3.0f,  -1.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 2
		   -3.0f,  -1.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 3
		   -3.0f,  -1.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   1.0f,  // 4
		   -3.0f,   0.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

		    // Handle Main Bottom
		   -0.6f,  -1.0f,  -0.2f,   0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.0f,  -1.0f,  -0.2f,   0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.0f,  -1.0f,   0.0f,   0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.0f,  -1.0f,   0.0f,   0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -0.6f,  -1.0f,   0.0f,   0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -0.6f,  -1.0f,  -0.2f,   0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Main Top
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -3.0f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -3.0f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

		    // Handle Hilt Back
		    0.0f,  -1.1f,  -0.2f,	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -0.6f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
		    0.0f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
		    0.0f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

		    // Handle Hilt Front
		    0.0f,  -1.1f,   0.0f, 	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -0.6f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,	    0.40f,  0.0f,  // 1
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
			0.0f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
			0.0f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

			// Handle Hilt Blade Side
			0.0f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
			0.0f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
			0.0f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
			0.0f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
			0.0f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
			0.0f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

			// Handle Hilt End Side
		   -0.6f,   0.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 0
		   -0.6f,   0.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  0.0f,  // 1
		   -0.6f,  -1.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 2
		   -0.6f,  -1.0f,  -0.2f,  -0.4f,  0.2f,  0.0f, 	0.40f,  1.0f,  // 3
		   -0.6f,  -1.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   1.0f,  // 4
		   -0.6f,   0.0f,   0.0f,  -0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

			// Handle Hilt Bottom
			0.0f,  -1.1f,  -0.2f,   0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -0.6f,  -1.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -0.6f,  -1.0f,   0.0f,   0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -0.6f,  -1.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
			0.0f,  -1.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
			0.0f,  -1.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 5

			// Handle Hilt Top
			0.0f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.0f,   0.0f,  // 0
		   -0.6f,   0.0f,  -0.2f,  	0.4f,  0.2f,  0.0f,		0.40f,  0.0f,  // 1
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 2
		   -0.6f,   0.0f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.40f,  1.0f,  // 3
			0.0f,   0.1f,   0.0f,  	0.4f,  0.2f,  0.0f,		0.0f,   1.0f,  // 4
			0.0f,   0.1f,  -0.2f,  	0.4f,  0.2f,  0.0f, 	0.0f,   0.0f,  // 5

		    // Knife blade

		    // Vertex Positions     // Normals			    // Textures
		    // Front
		    0.8f,   0.0f,  -0.09f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    4.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    1.0f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    1.0f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    4.0f,   0.0f,  -0.09f,  0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    2.8f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Back
		    0.8f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    4.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    1.0f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    1.0f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    4.0f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    2.8f,  -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Base Front
		    0.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    1.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    1.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    1.0f,  -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Base Back
		    0.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    1.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    1.0f,   0.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    1.0f,  -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Front 1
		    1.0f,  -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.95f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.85f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.95f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.85f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.75f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Front 2
		    0.85f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.75f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.65f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.75f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.65f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.55f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Front 3
		    0.65f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.55f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.45f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.55f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.45f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.35f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Front 4
		    0.45f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.35f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.25f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.35f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.25f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.15f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Front 5
		    0.25f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.15f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.05f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.15f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.05f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Back 1
		    1.0f,  -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.95f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.85f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.95f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.85f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.75f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Back 2
		    0.85f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.75f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.65f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.75f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.65f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.55f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Back 3
		    0.65f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.55f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.45f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.55f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.45f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.35f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Back 4
		    0.45f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.35f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.25f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.35f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.25f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.15f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		    // Serrated Teeth Back 5
		    0.25f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    0.15f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.05f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 2
		    0.15f, -0.8f,  -0.1f,   0.4f,  0.4f,  0.4f,		1.0f,   1.0f,  // 3
		    0.05f, -1.0f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  1.0f,  // 4
		    0.0f,  -0.9f,  -0.1f,   0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5


		    // Top Blunt Edge
		    0.0f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 0
		    4.0f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 1
		    0.0f,   0.0f,  -0.09f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 2
		    0.0f,   0.0f,  -0.09f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 3
		    4.0f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		1.0f,   0.0f,  // 4
		    0.0f,   0.0f,  -0.11f,  0.4f,  0.4f,  0.4f,		0.52f,  0.0f,  // 5

		};


			// Generate buffer ids
			glGenVertexArrays(1, &KnifeVAO); // Vertex Array Object for Knife vertices
			glGenBuffers(1, &VBO);

			// Activates the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers.
			glBindVertexArray(KnifeVAO);

			// Activate the VBO
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Copy vertices to VBO

			// set attribute pointer 0 to hold position data
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Set attribute pointer 1 to hold Normal data
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
			glEnableVertexAttribArray(1);

	        //Set attribute pointer 2 to hold Texture coordinate data
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);

			// Generate buffer ids for lamp (smaller Knife)
			glGenVertexArrays(1, &LightVAO); // Vertex Array Object for Knife vertex copies to serve as light source

			// Activate the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers
			glBindVertexArray(LightVAO);

			// Referencing the same VBO for its vertices
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

}


void pressSpecialKey(int key, int xx, int yy)
{
    switch(key){

    // Zoom in
    case GLUT_KEY_UP:
    	cameraPosition += front.x * 0.1f;
    	cameraPosition += front.y * 0.1f;
    	cameraPosition += front.z * 0.1f;
        break;

    // Zoom out
    case GLUT_KEY_DOWN:
    	cameraPosition -= front.x * 0.1f;
    	cameraPosition -= front.y * 0.1f;
    	cameraPosition -= front.z * 0.1f;
        break;

    // Change view state: Orthogonal
    case GLUT_KEY_LEFT:
        view_state = 0;
        break;

    // Change view state: Perspective
    case GLUT_KEY_RIGHT:
        view_state = 1;
        break;
    }
}
/*Implements the UMouseMove function*/
void UMouseMove(int x, int y)
{
    // Immediately replaces center locked coordinated with new mouse coordinates
    if(mouseDetected)
    {
        lastMouseX = x;
        lastMouseY = y;
        mouseDetected = false;
    }

    // Gets the direction the mouse was moved in x and y
    mouseXOffset = x - lastMouseX;
    mouseYOffset = lastMouseY - y; // Inverted Y

    // Updates with new mouse coordinates
    lastMouseX = x;
    lastMouseY = y;

    // Applies sensitivity to mouse direction
    mouseXOffset *= sensitivity;
    mouseYOffset *= sensitivity;

    // Accumulates the yaw and pitch variables
    yaw += mouseXOffset;
    pitch += mouseYOffset;

    // Maintains a 90 degree pitch
    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    // Converts mouse coordinates
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    cameraPosition = - front * glm::length(cameraPosition);
}

void UGenerateTexture(){

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			int width, height;

			unsigned char* image = SOIL_load_image("knife.jpg", &width, &height, 0, SOIL_LOAD_RGB); //loads texture

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			glGenerateMipmap(GL_TEXTURE_2D);
			SOIL_free_image_data(image);

			// Deactivates the bind texture
			glBindTexture(GL_TEXTURE_2D, 0);

}



