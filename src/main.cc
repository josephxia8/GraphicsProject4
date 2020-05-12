#include <GL/glew.h>

#include "bone_geometry.h"
#include "procedure_geometry.h"
#include "render_pass.h"
#include "config.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>
#include <time.h> 

int window_width = 800, window_height = 600;
const std::string window_title = "Skinning";

const char* vertex_shader =
#include "shaders/default.vert"
;

const char* blending_shader =
#include "shaders/blending.vert"
;

const char* geometry_shader =
#include "shaders/default.geom"
;

const char* fragment_shader =
#include "shaders/default.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* bone_vertex_shader =
#include "shaders/bone.vert"
;

const char* bone_fragment_shader =
#include "shaders/bone.frag"
;

const char* cylinder_vertex_shader =
#include "shaders/cylinder.vert"
;

const char* cylinder_fragment_shader =
#include "shaders/cylinder.frag"
;

const char* fur_vertex_shader =
#include "shaders/fur.vert"
;

const char* fur_geometry_shader =
#include "shaders/fur.geom"
;

const char* fur_fragment_shader =
#include "shaders/fur.frag"
;



// FIXME: Add more shaders here.

void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}


// Does the math for changing the shader_num flag with a button press
void shaderButton(int button_index, int &shaderNum){
	int index = pow(2, button_index);
	if ((shaderNum % (index * 2))/index != 0) {
				shaderNum -= index;
			} else {
				shaderNum += index;
			}
			std::cout << shaderNum << std::endl;
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Input model file is missing" << std::endl;
		std::cerr << "Usage: " << argv[0] << " <PMD file>" << std::endl;
		return -1;
	}
	GLFWwindow *window = init_glefw();
	GUI gui(window);

	clock_t start_time;
  	start_time = clock();
	float since_start = 0;

	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	create_floor(floor_vertices, floor_faces);

	LineMesh cylinder_mesh;
	LineMesh axes_mesh;
	LineMesh triangle_mesh;

	// FIXME: we already created meshes for cylinders. Use them to render
	//        the cylinder and axes if required by the assignment.
	create_cylinder_mesh(cylinder_mesh);
	create_axes_mesh(axes_mesh);

	std::vector<glm::vec4> triangle_vertices;
	std::vector<glm::uvec3> triangle_faces;
	create_triangle_mesh(triangle_vertices, triangle_faces);

	Mesh mesh;
	mesh.loadPmd(argv[1]);
	std::cout << "Loaded object  with  " << mesh.vertices.size()
		<< " vertices and " << mesh.faces.size() << " faces.\n";

	glm::vec4 mesh_center = glm::vec4(0.0f);
	for (size_t i = 0; i < mesh.vertices.size(); ++i) {
		mesh_center += mesh.vertices[i];
	}
	mesh_center /= mesh.vertices.size();

	/*
	 * GUI object needs the mesh object for bone manipulation.
	 */
	gui.assignMesh(&mesh);

	glm::vec4 light_position = glm::vec4(0.0f, 100.0f, 0.0f, 1.0f);
	MatrixPointers mats; // Define MatrixPointers here for lambda to capture

	/*
	 * In the following we are going to define several lambda functions as
	 * the data source of GLSL uniforms
	 *
	 * Introduction about lambda functions:
	 *      http://en.cppreference.com/w/cpp/language/lambda
	 *      http://www.stroustrup.com/C++11FAQ.html#lambda
	 *
	 * Note: lambda expressions cannot be converted to std::function directly
	 *       Hence we need to declare the data function explicitly.
	 *
	 * CAVEAT: DO NOT RETURN const T&, which compiles but causes
	 *         segfaults.
	 *
	 * Do not worry about the efficient issue, copy elision in C++ 17 will
	 * minimize the performance impact.
	 *
	 * More details about copy elision:
	 *      https://en.cppreference.com/w/cpp/language/copy_elision
	 */

	// FIXME: add more lambdas for data_source if you want to use RenderPass.
	//        Otherwise, do whatever you like here
	std::function<const glm::mat4*()> model_data = [&mats]() {
		return mats.model;
	};

	std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
	std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };
	std::function<glm::mat4()> identity_mat = [](){ return glm::mat4(1.0f); };
	std::function<glm::vec3()> cam_data = [&gui](){ return gui.getCamera(); };
	std::function<glm::vec4()> lp_data = [&light_position]() { return light_position; };

	// set transform of the bone (cylinder)
	std::function<glm::mat4()> b_transform = [&gui, &mesh]() { 
			glm::mat4 translate = glm::mat4(1.0f);
			glm::mat4 scale = glm::mat4(1.0f);
			scale[1][1] *= mesh.skeleton.bones[gui.getCurrentBone()].boneLength;
			//toRet[1][1] *= mesh.skeleton.bones[gui.getCurrentBone()].boneLength;
			int startJointId = mesh.skeleton.bones[gui.getCurrentBone()].startJoint;
			glm::mat4 rot = mesh.skeleton.bones[gui.getCurrentBone()].deformedOrientation;
			translate[3][0] = mesh.skeleton.joints[startJointId].position[0];
			translate[3][1] = mesh.skeleton.joints[startJointId].position[1];
			translate[3][2] = mesh.skeleton.joints[startJointId].position[2];
			//std::cout << toRet << std::endl;
			return translate * rot * scale;
	};

	// set transform of the fur
	std::function<glm::mat4()> f_transform = [&gui, &mesh]() {
			int pos = 1; 
			glm::mat4 translate = glm::mat4(1.0f);
			glm::mat4 scale = glm::mat4(1.0f);
			scale[1][1] *= mesh.skeleton.bones[pos].boneLength;
			
			int startJointId = mesh.skeleton.bones[pos].startJoint;
			glm::mat4 rot = mesh.skeleton.bones[pos].deformedOrientation;
			translate[3][0] = mesh.skeleton.joints[startJointId].position[0];
			translate[3][1] = mesh.skeleton.joints[startJointId].position[1];
			translate[3][2] = mesh.skeleton.joints[startJointId].position[2];
			//std::cout << toRet << std::endl;
			return translate * rot * scale;
	};

	std::function<std::vector<glm::mat4>()>  deformed_inv = [&mesh]() {
		std::vector<glm::mat4> toRet = std::vector<glm::mat4>();
		for (int i = 0; i < mesh.skeleton.joints.size(); ++i) {
			Joint cur = mesh.skeleton.joints[i];
			cur.childrenSum = glm::mat4(0.0);
			for (int j = 0; j < cur.boneChildren.size(); ++j) {
				cur.childrenSum += cur.boneChildren[j].deformedOrientation * cur.boneChildren[j].invRefPose;
			}
			if (cur.childrenSum[0][0] != 1 || cur.childrenSum[1][1] != 1 || cur.childrenSum[2][2] != 1 || cur.childrenSum[3][3] != 1) {
				//std::cout << cur.childrenSum << std::endl;
			}
			toRet.emplace_back(cur.childrenSum);
		}
		return toRet;
	};

	// setup for choosing different shaders
	int shaderNum = 0; // uses bit shifting as flags for different shaders
	std::function<int()> shader_num = [&shaderNum]() { return shaderNum; };

	std::function<float()> time_since_start = [&since_start]() {return since_start; };

	std::function<glm::fquat()> tri_rot = [&gui, &mesh]() {
		return mesh.skeleton.rotationBetweenVectors(glm::vec3(0,0,-1), gui.getCameraDirection());
	};

	auto std_model = std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
	auto floor_model = make_uniform("model", identity_mat);
	auto std_view = make_uniform("view", view_data);
	auto std_camera = make_uniform("camera_position", cam_data);
	auto std_proj = make_uniform("projection", proj_data);
	auto std_light = make_uniform("light_position", lp_data);

	auto bone_transform = make_uniform("bone_transform", b_transform);
	//auto fur_transform = make_uniform("bone_transform", f_transform);

	auto shaderNumUni = make_uniform("shader_num", shader_num);
	auto timeSinceStart = make_uniform("time_since_start", time_since_start);
	auto triRot = make_uniform("tri_rot", tri_rot);


	std::function<float()> alpha_data = [&gui]() {
		static const float transparent = 0.5; // Alpha constant goes here
		static const float non_transparent = 1.0;
		if (gui.isTransparent())
			return transparent;
		else
			return non_transparent;
	};
	auto object_alpha = make_uniform("alpha", alpha_data);

	std::function<std::vector<glm::vec3>()> trans_data = [&mesh](){ return mesh.getCurrentQ()->transData(); };
	std::function<std::vector<glm::fquat>()> rot_data = [&mesh](){ return mesh.getCurrentQ()->rotData(); };
	auto joint_trans = make_uniform("joint_trans", trans_data);
	auto joint_rot = make_uniform("joint_rot", rot_data);
	auto deform_inv = make_uniform("deform_inv", deformed_inv);
	// FIXME: define more ShaderUniforms for RenderPass if you want to use it.
	//        Otherwise, do whatever you like here

	// Floor render pass
	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);

	// PMD Model render pass
	// FIXME: initialize the input data at Mesh::loadPmd
	std::vector<glm::vec2>& uv_coordinates = mesh.uv_coordinates;
	RenderDataInput object_pass_input;
	object_pass_input.assign(0, "jid0", mesh.joint0.data(), mesh.joint0.size(), 1, GL_INT);
	object_pass_input.assign(1, "jid1", mesh.joint1.data(), mesh.joint1.size(), 1, GL_INT);
	object_pass_input.assign(2, "w0", mesh.weight_for_joint0.data(), mesh.weight_for_joint0.size(), 1, GL_FLOAT);
	object_pass_input.assign(3, "vector_from_joint0", mesh.vector_from_joint0.data(), mesh.vector_from_joint0.size(), 3, GL_FLOAT);
	object_pass_input.assign(4, "vector_from_joint1", mesh.vector_from_joint1.data(), mesh.vector_from_joint1.size(), 3, GL_FLOAT);
	object_pass_input.assign(5, "normal", mesh.vertex_normals.data(), mesh.vertex_normals.size(), 4, GL_FLOAT);
	object_pass_input.assign(6, "uv", uv_coordinates.data(), uv_coordinates.size(), 2, GL_FLOAT);
	// TIPS: You won't need vertex position in your solution.
	//       This only serves the stub shader.
	object_pass_input.assign(7, "vert", mesh.vertices.data(), mesh.vertices.size(), 4, GL_FLOAT);
	object_pass_input.assignIndex(mesh.faces.data(), mesh.faces.size(), 3);
	object_pass_input.useMaterials(mesh.materials);
	RenderPass object_pass(-1,
			object_pass_input,
			{
			  blending_shader,
			  geometry_shader,
			  fragment_shader
			},
			{ std_model, std_view, std_proj,
			  std_light,
			  std_camera, object_alpha,
			  joint_trans, joint_rot, deform_inv, shaderNumUni, timeSinceStart
			},
			{ "fragment_color" }
			);

	// Setup the render pass for drawing bones
	// FIXME: You won't see the bones until Skeleton::joints were properly
	//        initialized
	std::vector<int> bone_vertex_id;
	std::vector<glm::uvec2> bone_indices;
	for (int i = 0; i < (int)mesh.skeleton.joints.size(); i++) {
		bone_vertex_id.emplace_back(i);
	}
	/*
	for (const auto& joint: mesh.skeleton.joints) {
		if (joint.parent_index < 0)
			continue;
		bone_indices.emplace_back(joint.joint_index, joint.parent_index);
	}*/
	mesh.skeleton.initBoneIndicies(bone_indices);
	RenderDataInput bone_pass_input;
	bone_pass_input.assign(0, "jid", bone_vertex_id.data(), bone_vertex_id.size(), 1, GL_UNSIGNED_INT);
	bone_pass_input.assignIndex(bone_indices.data(), bone_indices.size(), 2);
	RenderPass bone_pass(-1, bone_pass_input,
			{ bone_vertex_shader, nullptr, bone_fragment_shader},
			{ std_model, std_view, std_proj, joint_trans },
			{ "fragment_color" }
			);

	// FIXME: Create the RenderPass objects for bones here.
	//        Otherwise do whatever you like.

	RenderDataInput cylinder_pass_input;
	cylinder_pass_input.assign(0, "vertex position", cylinder_mesh.vertices.data(), cylinder_mesh.vertices.size(), 4, GL_FLOAT);
	cylinder_pass_input.assignIndex(cylinder_mesh.indices.data(), cylinder_mesh.indices.size(), 2);
	RenderPass cylinder_pass(-1, cylinder_pass_input,
			{cylinder_vertex_shader , nullptr, cylinder_fragment_shader},
			{ std_model, std_view, std_proj, bone_transform },
			{ "fragment_color" }
			);


	float aspect = 0.0f;
	std::cout << "center = " << mesh.getCenter() << "\n";

	bool draw_floor = true;
	bool draw_skeleton = true;
	bool draw_object = true;
	bool draw_cylinder = true;

	// init undeformedVertices
	for (int i = 0; i < mesh.vertices.size(); i++){
		mesh.undeformedVertices.emplace_back(mesh.vertices[i]);
	}

	// imgui initalization
	// Application init: create a dear imgui context, setup some options, load fonts
     ImGui::CreateContext();
     ImGuiIO& io = ImGui::GetIO();
     // TODO: Set optional io.ConfigFlags values, e.g. 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard' to enable keyboard controls.
     // TODO: Fill optional fields of the io structure later.
     // TODO: Load TTF/OTF fonts if you don't want to use the default font.

     // Initialize helper Platform and Renderer bindings (here we are using imgui_impl_win32.cpp and imgui_impl_dx11.cpp)

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);

	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();
#if 0
		std::cerr << model_data() << '\n';
		std::cerr << "call from outside: " << std_model->data_source() << "\n";
		std_model->bind(0);
#endif

		since_start = ((float)clock() - (float)start_time)/CLOCKS_PER_SEC;

		if (gui.isPoseDirty()) {
			if (draw_object) {
				/*
				for(int i = 0; i < mesh.vertices.size(); ++i){
					glm::vec4 newVertex = glm::vec4(0,0,0,0);
					for (int j = 0; j < mesh.skeleton.bones.size(); ++j) {
						newVertex += mesh.skeleton.meshWeights[j][i] * mesh.skeleton.bones[j].deformedOrientation * mesh.skeleton.bones[j].invRefPose * mesh.undeformedVertices[i];
					}
					mesh.vertices[i] = newVertex;
				}
				*/
			}
			mesh.updateAnimation();
			gui.clearPose();
		}

		int current_bone = gui.getCurrentBone();

		// Draw bones first.
		if (draw_skeleton && gui.isTransparent()) {
			bone_pass.setup();
			// Draw our lines.
			// FIXME: you need setup skeleton.joints properly in
			//        order to see the bones.
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
			                              bone_indices.size() * 2,
			                              GL_UNSIGNED_INT, 0));
		}
		draw_cylinder = (current_bone != -1 && gui.isTransparent());

		if (draw_cylinder) {

			cylinder_pass.setup();
			
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
			                              cylinder_mesh.indices.size() * 2,
			                              GL_UNSIGNED_INT, 0));
		}

		// setup for fur render pass
		if ((shaderNum % 8192)/4096 == 1) {
			int numtriangles = 0;
			for(int i = 1026; i < 6397; i+=10){
				//int i = 0;
				glm::vec4 face_normal = mesh.vertex_normals[mesh.faces[i][0]];
				face_normal += mesh.vertex_normals[mesh.faces[i][1]];
				face_normal += mesh.vertex_normals[mesh.faces[i][2]];
				face_normal = (1.0f/3.0f) * face_normal;
				glm::vec3 face_normal2 = glm::normalize(glm::vec3(face_normal[0], face_normal[1], face_normal[2]));

				if (glm::dot(glm::normalize(gui.getCameraDirection()), face_normal2) < 0.000000001)
				{
					numtriangles++;
					glm::vec4 pos = mesh.vertices[mesh.faces[i][0]];
					pos += mesh.vertices[mesh.faces[i][1]];
					pos += mesh.vertices[mesh.faces[i][2]];
					//std::cout<< "pos = " << pos << " averaged " << ((1.0f/3.0f) * pos) << std::endl;
					pos = (1.0f/3.0f) * pos;
					glm::vec3 color = glm::vec3(0,0,0);
					for (const auto& ma : mesh.materials) {
						//std::cerr << "Use Material from " << ma.offset << " size: " << ma.nfaces << std::endl;
						if (i >= ma.offset && i < ma.offset + ma.nfaces){
							color = ma.diffuse;
							break;
						}
					}
					std::vector<glm::vec4> facePos = std::vector<glm::vec4>();
					std::vector<glm::fquat> faceRot = std::vector<glm::fquat>();
					std::vector<glm::vec3> color_data = std::vector<glm::vec3>();
					for(int j = 0; j < triangle_vertices.size(); ++j){
						//triangle_vertices[j] += pos;
						facePos.emplace_back(pos);

						//v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz) qtransform
						glm::fquat rot1 = mesh.skeleton.rotationBetweenVectors(glm::vec3(0,0,-1), gui.getCameraDirection());
						glm::vec3 up = glm::vec3(0,1,0);
						up = up + 2.0f * glm::cross(glm::cross(up, glm::vec3(rot1[0], rot1[1], rot1[2])) - rot1[3] * up, glm::vec3(rot1[0], rot1[1], rot1[2]));
						glm::fquat rot2 = mesh.skeleton.rotationBetweenVectors(up, face_normal2);
						
						faceRot.emplace_back(rot2);
						color_data.emplace_back(color);
					}

					//glm::lookAt()
					RenderDataInput fur_pass_input;
					fur_pass_input.assign(0, "vertex position", triangle_vertices.data(), triangle_vertices.size(), 4, GL_FLOAT);
					fur_pass_input.assignIndex(triangle_faces.data(), triangle_faces.size(), 3);
					fur_pass_input.assign(3, "color", color_data.data(), color_data.size(), 3, GL_FLOAT);
					fur_pass_input.assign(4, "face_pos", facePos.data(), facePos.size(), 4, GL_FLOAT);
					fur_pass_input.assign(5, "face_rot", faceRot.data(), faceRot.size(), 4, GL_FLOAT);
					RenderPass fur_pass(-1, fur_pass_input,
							{ fur_vertex_shader, fur_geometry_shader, fur_fragment_shader},
							{ std_model, std_view, std_proj, std_light, triRot },
							{ "fragment_color" }
							);
					
					fur_pass.setup();
					
					CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
												triangle_faces.size() * 3,
												GL_UNSIGNED_INT, 0));
				}
			}
			//std::cout<<"Num triangles: " << numtriangles << std::endl;
		}

		// Then draw floor.
		if (draw_floor) {
			floor_pass.setup();
			// Draw our triangles.
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
			                              floor_faces.size() * 3,
			                              GL_UNSIGNED_INT, 0));
		}

		// Draw the model
		if (draw_object) {

			object_pass.setup();
			int mid = 0;
			while (object_pass.renderWithMaterial(mid))
				mid++;
#if 0
			// For debugging also
			if (mid == 0) // Fallback
				CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, mesh.faces.size() * 3, GL_UNSIGNED_INT, 0));
#endif
		}

		// Feed inputs to dear imgui, start new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Any application code here
		ImGui::Text("Choose a shader:");
		if (ImGui::Button("Sphericalize")){
			shaderButton(0, shaderNum);
		}
		if (ImGui::Button("Color Blind Mode")){
			shaderButton(1, shaderNum);
		}
		if (ImGui::Button("Outline")){
			shaderButton(2, shaderNum);
		}
		if (ImGui::Button("Metalify")){
			shaderButton(3, shaderNum);
		}
		if (ImGui::Button("Miku Miku Rave!")){
			shaderButton(4, shaderNum);
		}
		if (ImGui::Button("Miku Miku Bounce!")){
			shaderButton(5, shaderNum);
		}
		if (ImGui::Button("walk cycle (no animation required)")){
			shaderButton(6, shaderNum);
		}
		if (ImGui::Button("Toonify")){
			shaderButton(7, shaderNum);
		}
		/*
		if (ImGui::Button("Cubify")){
			shaderButton(8, shaderNum);
		}
		*/
		if (ImGui::Button("Miku says gay rights!")){
			shaderButton(9, shaderNum);
		}
		if (ImGui::Button("Flat")){
			shaderButton(10, shaderNum);
		}
		if (ImGui::Button("Loooong")){
			shaderButton(11, shaderNum);
		}
		if (ImGui::Button("Fur")){
			shaderButton(12, shaderNum);
		}

    

		// Render dear imgui into screen
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//g_pSwapChain->Present(1, 0);

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	// Shutdown
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

