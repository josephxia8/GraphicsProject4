#include "ray.h"

ray::ray(const glm::dvec3& pp,
	 const glm::dvec3& dd,
	 const glm::dvec3& w,
         RayType tt)
        : p(pp), d(dd), atten(w), t(tt)
{
}

ray::~ray()
{
}

ray& ray::operator=(const ray& other)
{
	p     = other.p;
	d     = other.d;
	atten = other.atten;
	t     = other.t;
	return *this;
}

glm::dvec3 ray::at(const isect& i) const
{
	return at(i.getT());
}