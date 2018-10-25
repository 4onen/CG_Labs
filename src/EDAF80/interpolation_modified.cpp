#include "interpolation_modified.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	return p0*(1-x) + p1*x;
}

float evalCatmullRom1D(const float p0, const float p1, const float p2, const float p3, const float t, const float x){
	const glm::vec4 pointVector(p0,p1,p2,p3);
	const glm::mat4 geometryMatrix (0.0f,  -t,     2.0f*t,    -t,
									1.0f,0.0f,     t-3.0f,2.0f-t,
									0.0f,   t,3.0f-2.0f*t,t-2.0f,
									0.0f,0.0f,         -t,      t);
	const glm::vec4 progress(1.0f,x,x*x,x*x*x);
	return glm::dot(progress,geometryMatrix*pointVector);
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	return glm::vec3(evalCatmullRom1D(p0.x,p1.x,p2.x,p3.x,t,x)
					,evalCatmullRom1D(p0.y,p1.y,p2.y,p3.y,t,x)
					,evalCatmullRom1D(p0.z,p1.z,p2.z,p3.z,t,x)
					);
}

float evalCatmullRom1DDerivative(const float p0, const float p1, const float p2, const float p3, const float t, const float x){
	const glm::vec4 pointVector(p0,p1,p2,p3);
	const glm::mat4 geometryMatrix (0.0f,  -t,     2.0f*t,    -t,
									1.0f,0.0f,     t-3.0f,2.0f-t,
									0.0f,   t,3.0f-2.0f*t,t-2.0f,
									0.0f,0.0f,         -t,      t);
	const glm::vec4 progress(0.0f,1.0f,2.0f*x,3.0f*x*x);
	return glm::dot(progress,geometryMatrix*pointVector);
}

glm::vec3 
interpolation::evalCatmullRomDerivative(glm::vec3 const&p0, glm::vec3 const&p1,
										glm::vec3 const&p2, glm::vec3 const&p3,
										float const t, float const x)
{
	return glm::vec3(evalCatmullRom1DDerivative(p0.x,p1.x,p2.x,p3.x,t,x)
					,evalCatmullRom1DDerivative(p0.y,p1.y,p2.y,p3.y,t,x)
					,evalCatmullRom1DDerivative(p0.z,p1.z,p2.z,p3.z,t,x)
					);
}