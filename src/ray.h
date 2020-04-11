#ifndef __RAY_H__
#define __RAY_H__

// This was taken from ray.h in Project 1

#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class isect;

class ray {
public:
	enum RayType { VISIBILITY, REFLECTION, REFRACTION, SHADOW };

	ray(const glm::dvec3& pp, const glm::dvec3& dd, const glm::dvec3& w,
	    RayType tt = VISIBILITY);
	ray(const ray& other);
	~ray();

	ray& operator=(const ray& other);

	glm::dvec3 at(double t) const { return p + (t * d); }
	glm::dvec3 at(const isect& i) const;

	glm::dvec3 getPosition() const { return p; }
	glm::dvec3 getDirection() const { return d; }
	glm::dvec3 getAtten() const { return atten; }
	RayType type() const { return t; }

	void setPosition(const glm::dvec3& pp) { p = pp; }
	void setDirection(const glm::dvec3& dd) { d = dd; }

private:
	glm::dvec3 p;
	glm::dvec3 d;
	glm::dvec3 atten;
	RayType t;
};

class isect {
public:
	isect() : t(0.0), N() {}
	isect(const isect& other)
	{
		copyFromOther(other);
	}

	~isect() { }

	isect& operator=(const isect& other)
	{
		copyFromOther(other);
		return *this;
	}

	// Get/Set Time of flight
	void setT(double tt) { t = tt; }
	double getT() const { return t; }
	// Get/Set surface normal at this intersection.
	void setN(const glm::dvec3& n) { N = n; }
	glm::dvec3 getN() const { return N; }

	void setUVCoordinates(const glm::dvec2& coords)
	{
		uvCoordinates = coords;
	}
	glm::dvec2 getUVCoordinates() const { return uvCoordinates; }
	void setBary(const glm::dvec3& weights) { bary = weights; }
	void setBary(const double alpha, const double beta, const double gamma)
	{
		setBary(glm::dvec3(alpha, beta, gamma));
	}

private:
	void copyFromOther(const isect& other)
	{
		if (this == &other)
			return ;
		t             = other.t;
		N             = other.N;
		bary          = other.bary;
		uvCoordinates = other.uvCoordinates;
	}

	double t;
	glm::dvec3 N;
	glm::dvec2 uvCoordinates;
	glm::dvec3 bary;
};

const double RAY_EPSILON = 0.00000001;

#endif // __RAY_H__