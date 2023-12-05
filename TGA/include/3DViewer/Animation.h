#ifndef ANIMATION_H
#define ANIMATION_H

#include <glm/glm.hpp>
#include <vector>

using namespace std;

int n = 1;

enum Curves { Bezier, CatmullRom, Hermite };

class Animation
{
public:
	Animation() {
		this->active = false;
		this->loop = false;
		this->curve = Bezier;
		this->frame = 0;
	}

	Animation(bool loop, Curves curve, vector <glm::vec3> controlPoints)
	{
		this->active = true;
		this->loop = loop;
		this->curve = curve;
		this->controlPoints = controlPoints;
		this->frame = 0;
		computeCurve();
	}

	glm::vec3 animate()
	{
		if (!active) return glm::vec3{ 0.0,0.0,0.0 };

		glm::vec3 points = curvePoints[frame];
		if (loop) {
			this->frame = (frame + n) % curvePoints.size();
			if (frame == curvePoints.size()-1) {
				n = -1;
			}
			if (frame == 0) {
				n = 1;
			}
		}
		else {
			this->frame = (frame + 1) % curvePoints.size();
		}

		return points;
	}

private:
	bool active;
	bool loop;
	Curves curve;
	vector <glm::vec3> controlPoints;
	vector <glm::vec3> curvePoints;
	int frame;

	void computeCurve() {
		switch (this->curve)
		{
		case Bezier:
			computeBezier();
			break;
		case CatmullRom:
			computeCatmullRom();
			break;
		case Hermite:
			computeHermite();
			break;
		default:
			break;
		}
	}

	void computeBezier() {
		float step = 1.0 / 100.0;

		float t = 0;

		int nControlPoints = controlPoints.size();

		for (int i = 0; i < nControlPoints - 3; i += 3)
		{

			for (t; t <= 1.0; t += step)
			{
				glm::vec3 p;

				glm::vec4 T(t * t * t, t * t, t, 1);

				glm::vec3 P0 = controlPoints[i];
				glm::vec3 P1 = controlPoints[i + 1];
				glm::vec3 P2 = controlPoints[i + 2];
				glm::vec3 P3 = controlPoints[i + 3];

				glm::mat4x3 G(P0, P1, P2, P3);

				p = G * bezierM * T;  //---------

				curvePoints.push_back(p);
			}
		}
	}

	void computeCatmullRom() {
		float step = 1.0 / 100.0;

		float t = 0;

		int nControlPoints = controlPoints.size();

		for (int i = 0; i < nControlPoints - 3; i += 3)
		{

			for (float t = 0.0; t <= 1.0; t += step)
			{
				glm::vec3 p;

				glm::vec4 T(t * t * t, t * t, t, 1);

				glm::vec3 P0 = controlPoints[i];
				glm::vec3 P1 = controlPoints[i + 1];
				glm::vec3 P2 = controlPoints[i + 2];
				glm::vec3 P3 = controlPoints[i + 3];

				glm::mat4x3 G(P0, P1, P2, P3);

				p = G * catmullRomM * T;  //---------
				p = p * glm::vec3(0.5, 0.5, 0.5);

				curvePoints.push_back(p);
			}
		}
	}

	void computeHermite() {
		float step = 1.0 / 100.0;

		float t = 0;

		int nControlPoints = controlPoints.size();

		for (int i = 0; i < nControlPoints - 3; i += 3)
		{

			for (float t = 0.0; t <= 1.0; t += step)
			{
				glm::vec3 p;

				glm::vec4 T(t * t * t, t * t, t, 1);

				glm::vec3 P0 = controlPoints[i];
				glm::vec3 P1 = controlPoints[i + 3];
				glm::vec3 T0 = controlPoints[i + 1] - P0;
				glm::vec3 T1 = controlPoints[i + 2] - P1;

				glm::mat4x3 G(P0, P1, T0, T1);

				p = G * hermiteM * T;  //---------

				curvePoints.push_back(p);
			}
		}
	}

	const glm::mat4 bezierM = glm::mat4
	(
		-1, 3, -3, 1,
		3, -6, 3, 0,
		-3, 3, 0, 0,
		1, 0, 0, 0
	);

	const glm::mat4 hermiteM = glm::mat4
	(
		2, -2, 1, 1,
		-3, 3, -2, -1,
		0, 0, 1, 0,
		1, 0, 0, 0
	);

	const glm::mat4 catmullRomM = glm::mat4
	(
		-1, 3, -3, 1,
		2, -5, 4, -1,
		-1, 0, 1, 0,
		0, 2, 0, 0
	);
};

#endif