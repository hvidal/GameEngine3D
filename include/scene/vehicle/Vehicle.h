#ifndef VEHICLE_H_
#define VEHICLE_H_

class Vehicle
{
public:
	Vehicle() {}
	virtual ~Vehicle() {}

	virtual void turnLeft() = 0;
	virtual void turnRight() = 0;
	virtual void moveForward() = 0;
	virtual void brake() = 0;

	virtual void resetSteering() = 0;
	virtual void resetEngineAndBrake() = 0;
	virtual void jump() = 0;
};


#endif
