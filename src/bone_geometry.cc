#include "config.h"
#include "bone_geometry.h"
#include "texture_to_render.h"
#include <fstream>
#include <queue>
#include <iostream>
#include <stdexcept>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

/*
 * For debugging purpose.
 */
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	size_t count = std::min(v.size(), static_cast<size_t>(10));
	for (size_t i = 0; i < count; ++i) os << i << " " << v[i] << "\n";
	os << "size = " << v.size() << "\n";
	return os;
}

std::ostream& operator<<(std::ostream& os, const BoundingBox& bounds)
{
	os << "min = " << bounds.min << " max = " << bounds.max;
	return os;
}



const glm::vec3* Skeleton::collectJointTrans() const
{
	return cache.trans.data();
}

const glm::fquat* Skeleton::collectJointRot() const
{
	return cache.rot.data();
}

// FIXME: Implement bone animation.

void Skeleton::refreshCache(Configuration* target)
{
	if (target == nullptr)
		target = &cache;
	target->rot.resize(joints.size());
	target->trans.resize(joints.size());
	for (size_t i = 0; i < joints.size(); i++) {
		target->rot[i] = joints[i].orientation;
		target->trans[i] = joints[i].position;
	}
}


Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

void Mesh::loadPmd(const std::string& fn)
{
	MMDReader mr;
	mr.open(fn);
	mr.getMesh(vertices, faces, vertex_normals, uv_coordinates);
	computeBounds();
	mr.getMaterial(materials);

	// FIXME: load skeleton and blend weights from PMD file,
	//        initialize std::vectors for the vertex attributes,
	//        also initialize the skeleton as needed

	// init bones
	int index = 0;
	glm::vec3 wcoord;
	int parentID;
	while (mr.getJoint(index, wcoord, parentID)) {
		Joint toAdd = Joint(index, wcoord, parentID);
		skeleton.joints.push_back(toAdd);
		if (parentID >= 0){
			//Joint* startJoint = &skeleton.joints[parentID];
			//std::cout << parentID << " " << index << std::endl;
			Bone newBone = Bone(parentID, index, skeleton.joints[parentID].position, skeleton.joints[index].position, skeleton.bones.size());
			skeleton.joints[parentID].boneChildren.push_back(newBone);
			skeleton.bones.emplace_back(newBone);
		}
		++index;
	}

	// init weights
	std::vector<SparseTuple> tup;
	mr.getJointWeights(tup);


	int numBones = skeleton.bones.size();
	int numVerts = tup.size();
	skeleton.meshWeights.resize(numBones);
	for (int i = 0; i < numBones; ++i){
		skeleton.meshWeights[i].resize(numVerts);
		for(int j = 0; j < numVerts; ++j){
			skeleton.meshWeights[i][j] = 0;
		}
	}

	joint0 = std::vector<int32_t>(tup.size());
	joint1 = std::vector<int32_t>(tup.size());
	weight_for_joint0 = std::vector<float>(tup.size());

	vector_from_joint0 = std::vector<glm::vec3>(tup.size());
	vector_from_joint1 = std::vector<glm::vec3>(tup.size());
	
	for (int i = 0; i < tup.size(); ++i){
		SparseTuple cur = tup[i];
		Joint currentJoint = skeleton.joints[cur.jid0];

		joint0[i] = cur.jid0;
		joint1[i] = cur.jid1;
		weight_for_joint0[i] = cur.weight0;
		glm::vec3 vpos = glm::vec3(vertices[cur.vid][0], vertices[cur.vid][1], vertices[cur.vid][2]);
		vector_from_joint0[i] = vpos - skeleton.joints[cur.jid0].position;
		vector_from_joint1[i] = glm::vec3(0, 0, 0);

		for (int j = 0; j < currentJoint.boneChildren.size(); ++j){
			skeleton.meshWeights[currentJoint.boneChildren[j].boneIndex][i] = cur.weight0;
		}
		if(cur.jid1 >= 0){
			Joint currentJoint2 = skeleton.joints[cur.jid1];
			for (int j = 0; j < currentJoint2.boneChildren.size(); ++j){
				skeleton.meshWeights[currentJoint2.boneChildren[j].boneIndex][i] = cur.weight0;
			}

			vector_from_joint1[i] = vpos - skeleton.joints[cur.jid1].position;
		}
	}

	//std::cout << "numBones = " << numBones << "numVerts = " << numVerts << std::endl;
}

int Mesh::getNumberOfBones() const
{
	return skeleton.joints.size();
}

void Mesh::computeBounds()
{
	bounds.min = glm::vec3(std::numeric_limits<float>::max());
	bounds.max = glm::vec3(-std::numeric_limits<float>::max());
	for (const auto& vert : vertices) {
		bounds.min = glm::min(glm::vec3(vert), bounds.min);
		bounds.max = glm::max(glm::vec3(vert), bounds.max);
	}
}

void Mesh::updateAnimation(float t)
{
	skeleton.refreshCache(&currentQ_);
	// FIXME: Support Animation Here
}

const Configuration*
Mesh::getCurrentQ() const
{
	return &currentQ_;
}

