#include "gui.h"
#include "config.h"
#include "ray.h"
#include <glm/gtx/io.hpp>
#include <jpegio.h>
#include "bone_geometry.h"
#include <iostream>
#include <algorithm>
#include <debuggl.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace {
	// FIXME: Implement a function that performs proper
	//        ray-cylinder intersection detection
	// TIPS: The implement is provided by the ray-tracer starter code.
}

GUI::GUI(GLFWwindow* window, int view_width, int view_height, int preview_height)
	:window_(window), preview_height_(preview_height)
{
	glfwSetWindowUserPointer(window_, this);
	glfwSetKeyCallback(window_, KeyCallback);
	glfwSetCursorPosCallback(window_, MousePosCallback);
	glfwSetMouseButtonCallback(window_, MouseButtonCallback);
	glfwSetScrollCallback(window_, MouseScrollCallback);

	glfwGetWindowSize(window_, &window_width_, &window_height_);
	if (view_width < 0 || view_height < 0) {
		view_width_ = window_width_;
		view_height_ = window_height_;
	} else {
		view_width_ = view_width;
		view_height_ = view_height;
	}
	float aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI()
{
}

void GUI::assignMesh(Mesh* mesh)
{
	mesh_ = mesh;
	center_ = mesh_->getCenter();
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
#if 0
	if (action != 2)
		std::cerr << "Key: " << key << " action: " << action << std::endl;
#endif
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window_, GL_TRUE);
		return ;
	}
	if (key == GLFW_KEY_J && action == GLFW_RELEASE) {
		//FIXME save out a screenshot using SaveJPEG
	}
	if (key == GLFW_KEY_S && (mods & GLFW_MOD_CONTROL)) {
		if (action == GLFW_RELEASE)
			mesh_->saveAnimationTo("animation.json");
		return ;
	}

	if (mods == 0 && captureWASDUPDOWN(key, action))
		return ;
	if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) {
		float roll_speed;
		if (key == GLFW_KEY_RIGHT)
			roll_speed = -roll_speed_;
		else
			roll_speed = roll_speed_;
		// FIXME: actually roll the bone here
		if(current_bone_ != -1){
			glm::vec3 tangentAxis = glm::vec3(mesh_->skeleton.bones[current_bone_].deformedOrientation[1][0], mesh_->skeleton.bones[current_bone_].deformedOrientation[1][1], mesh_->skeleton.bones[current_bone_].deformedOrientation[1][2]);

			mesh_->skeleton.transformChildren(current_bone_, roll_speed, tangentAxis);
			mesh_->updateAnimation(0.0f); // this is just to get the bones to update with new positions
		}

	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		fps_mode_ = !fps_mode_;
	} else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_RELEASE) {
		current_bone_--;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_RELEASE) {
		current_bone_++;
		current_bone_ += mesh_->getNumberOfBones();
		current_bone_ %= mesh_->getNumberOfBones();
	} else if (key == GLFW_KEY_T && action != GLFW_RELEASE) {
		transparent_ = !transparent_;
	}

	// FIXME: implement other controls here.
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
	last_x_ = current_x_;
	last_y_ = current_y_;
	current_x_ = mouse_x;
	current_y_ = window_height_ - mouse_y;
	float delta_x = current_x_ - last_x_;
	float delta_y = current_y_ - last_y_;
	if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
		return;
	if (mouse_x > view_width_)
		return ;
	glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));
	glm::vec2 mouse_start = glm::vec2(last_x_, last_y_);
	glm::vec2 mouse_end = glm::vec2(current_x_, current_y_);
	glm::uvec4 viewport = glm::uvec4(0, 0, view_width_, view_height_);

	bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_RIGHT;
	bool drag_bone = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;

	if (drag_camera) {
		glm::vec3 axis = glm::normalize(
				orientation_ *
				glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
				);
		orientation_ =
			glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));
		tangent_ = glm::column(orientation_, 0);
		up_ = glm::column(orientation_, 1);
		look_ = glm::column(orientation_, 2);
	} else if (drag_bone && current_bone_ != -1) {
		// FIXME: Handle bone rotation
		glm::vec3 mouseDirWorld = mouse_direction;
		mouseDirWorld[2] = 1.0f;
		mouseDirWorld = -1.0f * glm::unProject(mouseDirWorld, model_matrix_ * view_matrix_, projection_matrix_, viewport);
		mouseDirWorld += glm::unProject(glm::vec3(0.0, 0.0, 1.0), model_matrix_ * view_matrix_, projection_matrix_, viewport);
		mouseDirWorld = glm::normalize(mouseDirWorld);
		//std::cout << "mouse dir = " << mouseDirWorld << std::endl;

		glm::vec3 rotationAxis = glm::normalize(glm::cross(mouseDirWorld, mesh_->skeleton.bones[current_bone_].getTangent()));
		//rotationAxis = glm::normalize(glm::cross(rotationAxis, mouseDirWorld));
		//std::cout << "mouse dir = " << mouseDirWorld << " look = " << look_ << " cross = " << rotationAxis << std::endl;

		//glm::mat4 rotMat = glm::rotate(mesh_->skeleton.bones[current_bone_].deformedOrientation, glm::length(glm::vec2(delta_x, delta_y)) * rotation_speed_ * 0.5f, rotationAxis);
		
		int rotateDir = -delta_x;
		if (abs(delta_y) > abs(delta_x))
		{
			rotateDir = -delta_y;
		}
		
		//float magnitude = glm::length(glm::vec2(delta_x, delta_y)) * rotation_speed_ * 0.1f * rotateDir;
		float magnitude = glm::length(glm::vec2(delta_x, delta_y)) * rotation_speed_ * 0.1f;
		mesh_->skeleton.transformChildren(current_bone_, magnitude, rotationAxis);

		mesh_->updateAnimation(0.0f); // this is just to get the bones to update with new positions
		//animation scares me :( (joey)

		return ;
	}

	// FIXME: highlight bones that have been moused over
	//std::cout << "x = " << current_x_ << " y = " << current_y_ << std::endl;
	glm::vec3 clickPos1 = glm::vec3(current_x_, current_y_, 0.0);
	glm::vec3 clickPos2 = glm::vec3(current_x_, current_y_, 1.0);

	//clickPos1 -= eye_;
	//clickPos2 -= eye_;

	clickPos1 = glm::unProject(clickPos1, model_matrix_ * view_matrix_, projection_matrix_, viewport);
	clickPos2 = glm::unProject(clickPos2, model_matrix_ * view_matrix_, projection_matrix_, viewport);

	glm::vec3 dir = glm::normalize(clickPos2 - clickPos1);
	//dir[0] *= -1;
	

	float min_d = FLT_MAX;
	int bone_num = -1;

	glm::vec3 startPt;
	glm::vec3 endPt;
	glm::vec3 printc2;
	glm::vec3 printc1;
	
	for (int i = 0; i < mesh_->skeleton.bones.size(); ++i) {
		glm::vec3 bone_dir = glm::vec3(mesh_->skeleton.bones[i].deformedOrientation[1][0], mesh_->skeleton.bones[i].deformedOrientation[1][1], mesh_->skeleton.bones[i].deformedOrientation[1][2]);
		bone_dir = glm::normalize(bone_dir);
		glm::vec3 endBone = mesh_->skeleton.joints[mesh_->skeleton.bones[i].startJoint].position + mesh_->skeleton.bones[i].boneLength * bone_dir; 
		
		glm::vec3 n = glm::cross(dir, bone_dir);
		glm::vec3 p1 = clickPos1;
		glm::vec3 p2 = mesh_->skeleton.joints[mesh_->skeleton.bones[i].startJoint].position;
		
		glm::vec3 c2 = p2 + (glm::dot((p1 - p2), glm::cross(dir, n)) / glm::dot(bone_dir, glm::cross(dir, n))) * bone_dir;
		glm::vec3 c1 = p1 + (glm::dot((p2 - p1), glm::cross(bone_dir, n)) / glm::dot(dir, glm::cross(bone_dir, n))) * dir;
		float d = glm::length(c2 - c1);

		//std::cout<<clickPos1 << " " << eye_ <<std::endl;

		// set smallest distance bone
		if (d < min_d)
		{
			if(((c2[0] <= p2[0] && c2[0] >= endBone[0]) || (c2[0] >= p2[0] && c2[0] <= endBone[0]))
					&& ((c2[1] <= p2[1] && c2[1] >= endBone[1]) || (c2[1] >= p2[1] && c2[1] <= endBone[1]))
					&& ((c2[2] <= p2[2] && c2[2] >= endBone[2]) || (c2[2] >= p2[2] && c2[2] <= endBone[2]))){
				min_d = d;
				bone_num = i;		

				startPt = p2;
				endPt = endBone;
				printc2 = c2;
				printc1 = c1;		
			}
		}
	}

	// check if min_d is within the cylinder
	if (min_d < kCylinderRadius){
		current_bone_ = bone_num;

	} else {
		current_bone_ = -1;
	}

	//current_bone_ = bone_num;

	//std::cout << "x = " << current_x_ << " y = " << current_y_ << std::endl;
	//std::cout << "ray pos = " << clickPos1 << " ray dir " << dir << " ray intersect = " << printc1 << std::endl;
	//std::cout<< "bone = " << startPt << " " << printc2 << " " << endPt <<std::endl;
	//current_bone_ = 1;

}

void GUI::mouseButtonCallback(int button, int action, int mods)
{
	if (current_x_ <= view_width_) {
		drag_state_ = (action == GLFW_PRESS);
		current_button_ = button;
		return ;
	}
	// FIXME: Key Frame Selection
}

void GUI::mouseScrollCallback(double dx, double dy)
{
	if (current_x_ < view_width_)
		return;
	// FIXME: Mouse Scrolling
}

void GUI::updateMatrices()
{
	// Compute our view, and projection matrices.
	if (fps_mode_)
		center_ = eye_ + camera_distance_ * look_;
	else
		eye_ = center_ - camera_distance_ * look_;

	view_matrix_ = glm::lookAt(eye_, center_, up_);
	light_position_ = glm::vec4(eye_, 1.0f);

	aspect_ = static_cast<float>(view_width_) / view_height_;
	projection_matrix_ =
		glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
	model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
	MatrixPointers ret;
	ret.projection = &projection_matrix_;
	ret.model= &model_matrix_;
	ret.view = &view_matrix_;
	return ret;
}

bool GUI::setCurrentBone(int i)
{
	if (i < 0 || i >= mesh_->getNumberOfBones())
		return false;
	current_bone_ = i;
	return true;
}

float GUI::getCurrentPlayTime() const
{
	return 0.0f;
}


bool GUI::captureWASDUPDOWN(int key, int action)
{
	if (key == GLFW_KEY_W) {
		if (fps_mode_)
			eye_ += zoom_speed_ * look_;
		else
			camera_distance_ -= zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_S) {
		if (fps_mode_)
			eye_ -= zoom_speed_ * look_;
		else
			camera_distance_ += zoom_speed_;
		return true;
	} else if (key == GLFW_KEY_A) {
		if (fps_mode_)
			eye_ -= pan_speed_ * tangent_;
		else
			center_ -= pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_D) {
		if (fps_mode_)
			eye_ += pan_speed_ * tangent_;
		else
			center_ += pan_speed_ * tangent_;
		return true;
	} else if (key == GLFW_KEY_DOWN) {
		if (fps_mode_)
			eye_ -= pan_speed_ * up_;
		else
			center_ -= pan_speed_ * up_;
		return true;
	} else if (key == GLFW_KEY_UP) {
		if (fps_mode_)
			eye_ += pan_speed_ * up_;
		else
			center_ += pan_speed_ * up_;
		return true;
	}
	return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->keyCallback(key, scancode, action, mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseButtonCallback(button, action, mods);
}

void GUI::MouseScrollCallback(GLFWwindow* window, double dx, double dy)
{
	GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
	gui->mouseScrollCallback(dx, dy);
}
