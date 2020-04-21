#ifndef BONE_GEOMETRY_H
#define BONE_GEOMETRY_H

#include "ray.h"
#include <ostream>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/quaternion.hpp>
#include <mmdadapter.h>
#include <cstdlib>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

class TextureToRender;
struct Bone;

struct BoundingBox {
	BoundingBox()
		: min(glm::vec3(-std::numeric_limits<float>::max())),
		max(glm::vec3(std::numeric_limits<float>::max())) {}
	glm::vec3 min;
	glm::vec3 max;

	// this was taken from Project 1
	bool intersect(const ray& r, double& tMin, double& tMax) const
	{
		/*
		* Kay/Kajiya algorithm.
		*/
		glm::dvec3 R0 = r.getPosition();
		glm::dvec3 Rd = r.getDirection();
		tMin = -1.0e308; // 1.0e308 is close to infinity... close enough
						// for us!
		tMax = 1.0e308;
		double ttemp;

		for (int currentaxis = 0; currentaxis < 3; currentaxis++) {
			double vd = Rd[currentaxis];
			// if the ray is parallel to the face's plane (=0.0)
			if (vd == 0.0)
				continue;
			double v1 = min[currentaxis] - R0[currentaxis];
			double v2 = max[currentaxis] - R0[currentaxis];
			// two slab intersections
			double t1 = v1 / vd;
			double t2 = v2 / vd;
			if (t1 > t2) { // swap t1 & t2
				ttemp = t1;
				t1    = t2;
				t2    = ttemp;
			}
			if (t1 > tMin)
				tMin = t1;
			if (t2 < tMax)
				tMax = t2;
			if (tMin > tMax)
				return false; // box is missed
			if (tMax < RAY_EPSILON)
				return false; // box is behind ray
		}
		return true; // it made it past all 3 axes.
	}
};

struct Joint {
	Joint()
		: joint_index(-1),
		  parent_index(-1),
		  position(glm::vec3(0.0f)),
		  init_position(glm::vec3(0.0f))
	{
	}
	Joint(int id, glm::vec3 wcoord, int parent)
		: joint_index(id),
		  parent_index(parent),
		  position(wcoord),
		  init_position(wcoord),
		  init_rel_position(init_position)
	{
	}
	
	int joint_index;
	int parent_index;
	glm::vec3 position;             // position of the joint
	glm::fquat orientation;         // rotation w.r.t. initial configuration
	glm::fquat rel_orientation;     // rotation w.r.t. it's parent. Used for animation.
	glm::vec3 init_position;        // initial position of this joint
	glm::vec3 init_rel_position;    // initial relative position to its parent
	std::vector<int> children;

	std::vector<Bone> boneChildren;
};

struct Configuration {
	std::vector<glm::vec3> trans;
	std::vector<glm::fquat> rot;

	const auto& transData() const { return trans; }
	const auto& rotData() const { return rot; }
};

struct KeyFrame {
	std::vector<glm::fquat> rel_rot;

	static void interpolate(const KeyFrame& from,
	                        const KeyFrame& to,
	                        float tau,
	                        KeyFrame& target);
};

struct LineMesh {
	std::vector<glm::vec4> vertices;
	std::vector<glm::uvec2> indices;
};

struct Bone {

	Bone(int sJoint, int eJoint, glm::vec3 pos1, glm::vec3 pos2, int index){
		startJoint = sJoint;
		endJoint = eJoint;
		boneIndex = index;
		boneLength = glm::length(pos1-pos2);

		glm::vec3 tangent = glm::normalize(pos2-pos1);

		glm::vec3 check = pos1 + (boneLength * tangent);
		
		updateOrientation(tangent);
		orientation = deformedOrientation;
	}

	void updateOrientation(glm::vec3 tangent)
	{
		glm::vec3 normal = tangent;

		int lowest = abs(normal[0]);
		int which = 0;

		for(int i = 1; i < 3; i++){
			if (abs(normal[i]) < lowest) {
				lowest = abs(normal[i]);
				which = i;
			}
		}

		normal[which] = 1;
		normal = glm::normalize(normal);

		normal = glm::cross(tangent, normal)/glm::length(glm::cross(tangent, normal));
		
		glm::vec3 binormal = glm::normalize(glm::cross(tangent, normal));

		// trying to make the orientation matrix
		deformedOrientation = glm::mat4(1.0);
		deformedOrientation[2][0] = normal[0];
		deformedOrientation[2][1] = normal[1];
		deformedOrientation[2][2] = normal[2];
		deformedOrientation[0][0] = binormal[0];
		deformedOrientation[0][1] = binormal[1];
		deformedOrientation[0][2] = binormal[2];
		deformedOrientation[1][0] = tangent[0];
		deformedOrientation[1][1] = tangent[1];
		deformedOrientation[1][2] = tangent[2];
	}

	glm::vec3 getTangent()
	{
		return glm::vec3(deformedOrientation[1][0], deformedOrientation[1][1], deformedOrientation[1][2]);
	}

	int boneIndex;
	int startJoint;
	int endJoint;

	float boneLength;

	glm::vec3 localTranslation;
	glm::fquat localRotation;

	glm::vec3 globalTranslation;
	glm::fquat globalRotation;

	glm::mat4 orientation;
	glm::mat4 deformedOrientation;

	LineMesh* boneLine;
};

struct Skeleton {
	std::vector<Joint> joints;
	std::vector<Bone> bones;

	Configuration cache;

	void refreshCache(Configuration* cache = nullptr);
	const glm::vec3* collectJointTrans() const;
	const glm::fquat* collectJointRot() const;

	// FIXME: create skeleton and bone data structures
	void initBoneIndicies(std::vector<glm::uvec2>& boneIndicies) {
		//bone_indices.emplace_back(joint.joint_index, joint.parent_index);
		boneIndiciesRecursion(boneIndicies, 0);
		int nextIndex = joints[0].boneChildren[0].endJoint;
		//std::cout<< "First one " << nextIndex << std::endl;
		//std::cout << "why " << boneIndicies.size() << std::endl;
	}

	void boneIndiciesRecursion(std::vector<glm::uvec2>& boneIndicies, int index) {
		for(int i = 0; i < (int)joints[index].boneChildren.size(); i++) {
			
			int nextIndex = joints[index].boneChildren[i].endJoint;
			boneIndicies.emplace_back(nextIndex, index);
			//std::cout << "index =  " << index << " i = " << i << " nextIndex= " << nextIndex << std::endl;
			boneIndiciesRecursion(boneIndicies, nextIndex);
		}
	}

	void transformChildren(int boneIndex, float magnitude, glm::vec3 axis){

		/*glm::mat4 rotateMatrix = glm::mat4(1.0f);
		rotateMatrix[0][0] = bones[boneIndex].deformedOrientation[1][0];
		rotateMatrix[0][1] = bones[boneIndex].deformedOrientation[1][1];
		rotateMatrix[0][2] = bones[boneIndex].deformedOrientation[1][2];
		rotateMatrix[1][0] = bones[boneIndex].deformedOrientation[2][0];
		rotateMatrix[1][1] = bones[boneIndex].deformedOrientation[2][1];
		rotateMatrix[1][2] = bones[boneIndex].deformedOrientation[2][2];
		rotateMatrix[2][0] = bones[boneIndex].deformedOrientation[0][0];
		rotateMatrix[2][1] = bones[boneIndex].deformedOrientation[0][1];
		rotateMatrix[2][2] = bones[boneIndex].deformedOrientation[0][2];
		// trying to make the orientation matrix but it's not quite right yet
		/*orientation = glm::mat4(1.0);
		orientation[2][0] = normal[0];
		orientation[2][1] = normal[1];
		orientation[2][2] = normal[2];
		orientation[0][0] = binormal[0];
		orientation[0][1] = binormal[1];
		orientation[0][2] = binormal[2];
		orientation[1][0] = tangent[0];
		orientation[1][1] = tangent[1];
		orientation[1][2] = tangent[2];

		rotateMatrix = glm::rotate(rotateMatrix, magnitude, axis);
		bones[boneIndex].deformedOrientation[0][0] = rotateMatrix[2][0];
		bones[boneIndex].deformedOrientation[0][1] = rotateMatrix[2][1];
		bones[boneIndex].deformedOrientation[0][2] = rotateMatrix[2][2];
		bones[boneIndex].deformedOrientation[1][0] = rotateMatrix[0][0];
		bones[boneIndex].deformedOrientation[1][1] = rotateMatrix[0][1];
		bones[boneIndex].deformedOrientation[1][2] = rotateMatrix[0][2];
		bones[boneIndex].deformedOrientation[2][0] = rotateMatrix[1][0];
		bones[boneIndex].deformedOrientation[2][1] = rotateMatrix[1][1];
		bones[boneIndex].deformedOrientation[2][2] = rotateMatrix[1][2];*/

		
		//glm::vec3 newTangent = glm::vec3(rotateMatrix[0][0], rotateMatrix[0][1], rotateMatrix[0][2]);

		glm::vec3 newTangent = glm::rotate(bones[boneIndex].getTangent(), magnitude, axis);
		bones[boneIndex].updateOrientation(newTangent);

		glm::vec3 newEndPos = joints[bones[boneIndex].startJoint].position + (newTangent * bones[boneIndex].boneLength);
		joints[bones[boneIndex].endJoint].position = newEndPos;

		for (int i = 0; i < joints[bones[boneIndex].endJoint].boneChildren.size(); ++i){
			transformChildren(joints[bones[boneIndex].endJoint].boneChildren[i].boneIndex, magnitude, axis);
		}
	}
};

struct Mesh {
	Mesh();
	~Mesh();
	std::vector<glm::vec4> vertices;
	/*
	 * Static per-vertex attrributes for Shaders
	 */
	std::vector<int32_t> joint0;
	std::vector<int32_t> joint1;
	std::vector<float> weight_for_joint0; // weight_for_joint1 can be calculated
	std::vector<glm::vec3> vector_from_joint0;
	std::vector<glm::vec3> vector_from_joint1;
	std::vector<glm::vec4> vertex_normals;
	std::vector<glm::vec4> face_normals;
	std::vector<glm::vec2> uv_coordinates;
	std::vector<glm::uvec3> faces;

	std::vector<Material> materials;
	BoundingBox bounds;
	Skeleton skeleton;

	void loadPmd(const std::string& fn);
	int getNumberOfBones() const;
	glm::vec3 getCenter() const { return 0.5f * glm::vec3(bounds.min + bounds.max); }
	const Configuration* getCurrentQ() const; // Configuration is abbreviated as Q
	void updateAnimation(float t = -1.0);

	void saveAnimationTo(const std::string& fn);
	void loadAnimationFrom(const std::string& fn);

private:
	void computeBounds();
	void computeNormals();
	Configuration currentQ_;
};


#endif
