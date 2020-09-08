#include "NewMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

//for math operations related to the arc
#include <cmath>
#define _USE_MATH_DEFINES

NewMode::NewMode() {
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of NewMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4 * 3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4 * 3 + 4 * 1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1, 1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

NewMode::~NewMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool NewMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (health > 0 && evt.type == SDL_MOUSEMOTION) {
		//convert mouse from window pixels (top-left origin, +y is down) to clip space ([-1,1]x[-1,1], +y is up):
		glm::vec2 clip_mouse = glm::vec2(
			(evt.motion.x + 0.5f) / window_size.x * 2.0f - 1.0f,
			(evt.motion.y + 0.5f) / window_size.y *-2.0f + 1.0f
		);

		//calculate the distance vector from the mouse to the center of the screen
		glm::vec2 distance = glm::vec2(
			(clip_to_court * glm::vec3(clip_mouse, 1.0f)).x,
			(clip_to_court * glm::vec3(clip_mouse, 1.0f)).y
		);

		arc_paddle.x = distance.x;
		arc_paddle.y = distance.y;
	}

	return false;
}

void NewMode::update(float elapsed) {
	//end the game if health reaches zero
	if (health > 0)
	{
		//----- ball update -----

		//add a new ball every 12 wall collisions, up to a max of 7 balls in play at once
		if (num_collisions != 0 && num_collisions <= 72 && num_collisions % 12 == 0) {
			int index = num_collisions / 12;
			if (index % 2 == 0)
			{
				balls[index] = glm::vec2(6.0f, 0.0f);
				ball_velocities[index] = glm::vec2(-1.0f, 0.0f);
			}
			else {
				balls[index] = glm::vec2(-6.0f, 0.0f);
				ball_velocities[index] = glm::vec2(1.0f, 0.0f);
			}
		}

		//increase the speed multiplier based on the number of wall collisions
		float speed_multiplier = glm::min((float)num_collisions / 12.0f + 2.0f, 7.5f);

		//loop through all the balls and move them if necessary
		for (int i = 0; i < 7; i++) {
			if (balls[i].x != 12.0f) {
				balls[i] += elapsed * ball_velocities[i] * speed_multiplier;
			}
		}

		//---- collision handling ----

		//arc paddle:
		auto arc_vs_ball = [this](glm::vec2 const &paddle, glm::vec2 &ball, glm::vec2 &ball_velocity) {
			//find the corners of the ball:
			glm::vec2 top_left = glm::vec2(ball.x - ball_radius.x, ball.y - ball_radius.y);
			glm::vec2 top_right = glm::vec2(ball.x + ball_radius.x, ball.y - ball_radius.y);
			glm::vec2 bottom_left = glm::vec2(ball.x - ball_radius.x, ball.y + ball_radius.y);
			glm::vec2 bottom_right = glm::vec2(ball.x + ball_radius.x, ball.y + ball_radius.y);

			//compute the magnitude of the distance from the center of the window to the ball's corners:
			float top_left_distance = std::sqrt(top_left.x * top_left.x + top_left.y * top_left.y);
			float top_right_distance = std::sqrt(top_right.x * top_right.x + top_right.y * top_right.y);
			float bottom_left_distance = std::sqrt(bottom_left.x * bottom_left.x + bottom_left.y * bottom_left.y);
			float bottom_right_distance = std::sqrt(bottom_right.x * bottom_right.x + bottom_right.y * bottom_right.y);

			//check if ball close enough to center to possibly collide with arc:
			if ((top_left_distance <= 1.35f     && top_left_distance >= 1.0f) ||
				(top_right_distance <= 1.35f    && top_right_distance >= 1.0f) ||
				(bottom_left_distance <= 1.35f  && bottom_left_distance >= 1.0f) ||
				(bottom_right_distance <= 1.35f && bottom_right_distance >= 1.0f)) {
				//compute the "collision points" from the arc that will be compared against the ball
				double theta = atan(paddle.y / paddle.x);
				if (paddle.x < 0) theta += M_PI;
				glm::vec2 arc_left_corner = glm::vec2(1.35f * cos(theta - 0.5f), 1.35f * sin(theta - 0.5f));
				glm::vec2 arc_center = glm::vec2(1.35f * cos(theta), 1.35f * sin(theta));
				glm::vec2 arc_right_corner = glm::vec2(1.35f * cos(theta + 0.25f), 1.35f * sin(theta + 0.25f));
				//get the min and max in x and y of the arc's collision points
				float x_min = glm::min(arc_center.x, glm::min(arc_left_corner.x, arc_right_corner.x));
				float x_max = glm::max(arc_center.x, glm::max(arc_left_corner.x, arc_right_corner.x));
				float y_min = glm::min(arc_center.y, glm::min(arc_left_corner.y, arc_right_corner.y));
				float y_max = glm::max(arc_center.y, glm::max(arc_left_corner.y, arc_right_corner.y));
				//compare corners of the ball against the arc's collision points
				if ((top_left.x     >= x_min && top_left.x     <= x_max && top_left.y     >= y_min && top_left.y     <= y_max) ||
					(top_right.x    >= x_min && top_right.x    <= x_max && top_right.y    >= y_min && top_right.y    <= y_max) ||
					(bottom_left.x  >= x_min && bottom_left.x  <= x_max && bottom_left.y  >= y_min && bottom_left.y  <= y_max) ||
					(bottom_right.x >= x_min && bottom_right.x <= x_max && bottom_right.y >= y_min && bottom_right.y <= y_max)) {
					//change ball x velocity:
					if (ball.x > 0.0f) {
						ball.x += ball_radius.x;
						ball_velocity.x = std::abs(ball_velocity.x);
					}
					else if (ball.x < 0.0f) {
						ball.x -= ball_radius.x;
						ball_velocity.x = -std::abs(ball_velocity.x);
					}
					//change ball y velocity:
					if (ball.y > 0.0f) {
						ball.y += ball_radius.y;
						ball_velocity.y = std::abs(ball_velocity.y);
					}
					else if (ball.y < 0.0f) {
						ball.y -= ball_radius.y;
						ball_velocity.y = -std::abs(ball_velocity.y);
					}
					//warp y velocity based on offset from window center:
					float vel = (ball.y - paddle.y) / (1.35f + ball_radius.y);
					ball_velocity.y = glm::min(glm::mix(ball_velocity.y, vel, 0.75f), 1.0f);
					ball_velocity.y = glm::max(ball_velocity.y, -1.0f);
				}
			}
		};

		//for each ball, do collisions
		for (int i = 0; i < 7; i++) {
			if (balls[i].x != 12.0f) {
				arc_vs_ball(arc_paddle, balls[i], ball_velocities[i]);

				//court walls:
				if (balls[i].y > court_radius.y - ball_radius.y) {
					balls[i].y = court_radius.y - ball_radius.y;
					if (ball_velocities[i].y > 0.0f) {
						ball_velocities[i].y = -ball_velocities[i].y;
					}
					num_collisions += 1;
				}
				if (balls[i].y < -court_radius.y + ball_radius.y) {
					balls[i].y = -court_radius.y + ball_radius.y;
					if (ball_velocities[i].y < 0.0f) {
						ball_velocities[i].y = -ball_velocities[i].y;
					}
					num_collisions += 1;
				}

				if (balls[i].x > court_radius.x - ball_radius.x) {
					balls[i].x = court_radius.x - ball_radius.x;
					if (ball_velocities[i].x > 0.0f) {
						ball_velocities[i].x = -ball_velocities[i].x;
					}
					num_collisions += 1;
				}
				if (balls[i].x < -court_radius.x + ball_radius.x) {
					balls[i].x = -court_radius.x + ball_radius.x;
					if (ball_velocities[i].x < 0.0f) {
						ball_velocities[i].x = -ball_velocities[i].x;
					}
					num_collisions += 1;
				}

				//center square:
				if (balls[i].x >= -2.0f * ball_radius.x && balls[i].x <= 2.0f * ball_radius.x &&
					balls[i].y >= -2.0f * ball_radius.y && balls[i].y <= 2.0f * ball_radius.y) {
					if (num_collisions % 2 == 0) {
						balls[i] = glm::vec2(6.0f, 0.0f);
						ball_velocities[i] = glm::vec2(-1.0f, 0.0f);
					}
					else {
						balls[i] = glm::vec2(-6.0f, 0.0f);
						ball_velocities[i] = glm::vec2(1.0f, 0.0f);
					}
					health -= 1;
					num_collisions += 1;
				}

				//other balls:
				for (int j = 0; j < 7; j++) {
					if (i != j && balls[j].x != 12.0f &&
						balls[i].x - balls[j].x >= -2.0f * ball_radius.x && balls[i].x - balls[j].x <= 2.0f * ball_radius.x &&
						balls[i].y - balls[j].y >= -2.0f * ball_radius.y && balls[i].y - balls[j].y <= 2.0f * ball_radius.y) {
						if (std::abs(balls[i].x - balls[j].x) > std::abs(balls[i].y - balls[j].y)) {
							if (balls[i].x > balls[j].x) {
								balls[i].x += ball_radius.x;
								balls[j].x -= ball_radius.x;
							}
							else {
								balls[i].x -= ball_radius.x;
								balls[j].x += ball_radius.x;
							}
							ball_velocities[i].x = -ball_velocities[i].x;
							ball_velocities[j].x = -ball_velocities[j].x;
						}
						else if (std::abs(balls[i].x - balls[j].x) == std::abs(balls[i].y - balls[j].y)) {
							if (balls[i].x > balls[j].x) {
								balls[i].x += ball_radius.x;
								balls[j].x -= ball_radius.x;
							}
							else {
								balls[i].x -= ball_radius.x;
								balls[j].x += ball_radius.x;
							}
							if (balls[i].y > balls[j].y) {
								balls[i].y += ball_radius.y;
								balls[j].y -= ball_radius.y;
							}
							else {
								balls[i].y -= ball_radius.y;
								balls[j].y += ball_radius.y;
							}
							ball_velocities[i].x = -ball_velocities[i].x;
							ball_velocities[i].y = -ball_velocities[i].y;
						}
						else {
							if (balls[i].y > balls[j].y) {
								balls[i].y += ball_radius.y;
								balls[j].y -= ball_radius.y;
							}
							else {
								balls[i].y -= ball_radius.y;
								balls[j].y += ball_radius.y;
							}
							ball_velocities[i].y = -ball_velocities[i].y;
						}
					}
				}
			}
		}
	}
	//reset the game if health reaches zero
	else {
		health = 5;
		num_collisions = 11;
		balls[0] = glm::vec2(6.0f, 0.0f);
		ball_velocities[0] = glm::vec2(-1.0f, 0.0f);
		for (int i = 1; i < 7; i++) {
			balls[i] = glm::vec2(12.0f, 0.0f);
			ball_velocities[i] = glm::vec2(0.0f, 0.0f);
		}
	}
}

void NewMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x171714ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0xd1bb54ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0x604d29ff);
	const std::vector< glm::u8vec4 > rainbow_colors = {
		HEX_TO_U8VEC4(0x604d29ff), HEX_TO_U8VEC4(0x624f29fc), HEX_TO_U8VEC4(0x69542df2),
		HEX_TO_U8VEC4(0x6a552df1), HEX_TO_U8VEC4(0x6b562ef0), HEX_TO_U8VEC4(0x6b562ef0),
		HEX_TO_U8VEC4(0x6d572eed), HEX_TO_U8VEC4(0x6f592feb), HEX_TO_U8VEC4(0x725b31e7),
		HEX_TO_U8VEC4(0x745d31e3), HEX_TO_U8VEC4(0x755e32e0), HEX_TO_U8VEC4(0x765f33de),
		HEX_TO_U8VEC4(0x7a6234d8), HEX_TO_U8VEC4(0x826838ca), HEX_TO_U8VEC4(0x977840a4),
		HEX_TO_U8VEC4(0x96773fa5), HEX_TO_U8VEC4(0xa07f4493), HEX_TO_U8VEC4(0xa1814590),
		HEX_TO_U8VEC4(0x9e7e4496), HEX_TO_U8VEC4(0xa6844887), HEX_TO_U8VEC4(0xa9864884),
		HEX_TO_U8VEC4(0xad8a4a7c),
	};
#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//inline helper function for arc drawing:
	auto draw_arc = [&vertices](glm::vec2 const &center, glm::u8vec4 const &color) {
		//get the angle that the mouse is at from the center of the window in radians
		double theta = atan(center.y / center.x);
		if (center.x < 0) theta += M_PI;
		
		//draw arc as eight triangles along an arc:
		for (int i = -2; i < 2; i++) {
			vertices.emplace_back(glm::vec3(1.0f * cos(theta + 0.25f * i),       1.0f * sin(theta + 0.25f * i),       0.0f), color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(glm::vec3(1.0f * cos(theta + 0.25f * (i + 1)), 1.0f * sin(theta + 0.25f * (i + 1)), 0.0f), color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(glm::vec3(1.35f * cos(theta + 0.25f * i),       1.35f * sin(theta + 0.25f * i),       0.0f), color, glm::vec2(0.5f, 0.5f));
			
			vertices.emplace_back(glm::vec3(1.0f * cos(theta + 0.25f * (i + 1)), 1.0f * sin(theta + 0.25f * (i + 1)), 0.0f), color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(glm::vec3(1.35f * cos(theta + 0.25f * i),       1.35f * sin(theta + 0.25f * i),       0.0f), color, glm::vec2(0.5f, 0.5f));
			vertices.emplace_back(glm::vec3(1.35f * cos(theta + 0.25f * (i + 1)), 1.35f * sin(theta + 0.25f * (i + 1)), 0.0f), color, glm::vec2(0.5f, 0.5f));
		}
	};

	glm::vec2 s = glm::vec2(0.0f, -shadow_offset);
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);

	//things that should only be drawn if health is greater than zero
	if (health > 0) {
		//shadow for the paddle
		draw_arc(arc_paddle + s, shadow_color);

		//draw center point to be protected
		draw_rectangle(glm::vec2(0.0f, 0.0f), ball_radius, fg_color);

		//paddle:
		draw_arc(arc_paddle, fg_color);

		//ball:
		for (int i = 0; i < 7; i++) {
			if (balls[i].x != 12.0f) {
				draw_rectangle(balls[i], ball_radius, fg_color);
			}
		}

		//health:
		uint32_t max_i = health;
		for (uint32_t i = 0; i < max_i; ++i) {
			draw_rectangle(glm::vec2(-court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, fg_color);
		}
	}

	//shadows for the walls
	draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f) + s, glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), shadow_color);
	draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);
	draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius) + s, glm::vec2(court_radius.x, wall_radius), shadow_color);

	//solid objects:

	//walls:
	draw_rectangle(glm::vec2(-court_radius.x - wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(court_radius.x + wall_radius, 0.0f), glm::vec2(wall_radius, court_radius.y + 2.0f * wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, -court_radius.y - wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);
	draw_rectangle(glm::vec2(0.0f, court_radius.y + wall_radius), glm::vec2(court_radius.x, wall_radius), fg_color);

	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);


	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}