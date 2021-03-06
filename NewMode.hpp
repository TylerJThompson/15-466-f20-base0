#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

/*
 * NewMode is a game mode that implements a single-player game of Not-Pong.
 */

struct NewMode : Mode {
	NewMode();
	virtual ~NewMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	glm::vec2 court_radius = glm::vec2(7.0f, 5.0f);
	glm::vec2 paddle_radius = glm::vec2(0.2f, 1.0f);
	glm::vec2 ball_radius = glm::vec2(0.2f, 0.2f);

	glm::vec2 arc_paddle = glm::vec2(1.0f, 0.0f);

	//multiple balls, with the first already in the game at the start
	glm::vec2 balls[7] = { glm::vec2(6.0f, 0.0f), glm::vec2(12.0f, 0.0f), glm::vec2(12.0f, 0.0f), glm::vec2(12.0f, 0.0f), glm::vec2(12.0f, 0.0f),
	                       glm::vec2(12.0f, 0.0f), glm::vec2(12.0f, 0.0f) };
	glm::vec2 ball_velocities[7] = { glm::vec2(-1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f),
	                                 glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) };

	uint32_t health = 5;
	uint32_t num_collisions = 11; //initially set to 11 so NewMode will spawn the second ball after the first wall collision

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4 * 3 + 1 * 4 + 4 * 2, "NewMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};