#pragma config(Hubs,  S1, HTMotor,  HTMotor,  none,     none)
#pragma config(Hubs,  S3, HTMotor,  HTMotor,  none,     none)
#pragma config(Sensor, S1,     DCMotor1,       sensorI2CMuxController)
#pragma config(Sensor, S2,     IRSeeker,       sensorI2CCustom)
#pragma config(Sensor, S3,     DCMotor2,       sensorI2CMuxController)
#pragma config(Motor,  mtr_S1_C1_1,     motorA,        tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C1_2,     Lwheel,        tmotorTetrix, PIDControl, encoder)
#pragma config(Motor,  mtr_S1_C2_1,     motorF,        tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C2_2,     Ltread,        tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S3_C1_1,     Arm,           tmotorTetrix, openLoop, reversed)
#pragma config(Motor,  mtr_S3_C1_2,     motorI,        tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S3_C2_1,     Rwheel,        tmotorTetrix, PIDControl, reversed, encoder)
#pragma config(Motor,  mtr_S3_C2_2,     Rtread,        tmotorTetrix, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#include "JoystickDriver.c"  //Include file to "handle" the Bluetooth messages.
#include "drivers/hitechnic-irseeker-v2.h"

/* FUNCTION PROTOTYPES */
void mainAuto();
void searchDirection();
void searchStrength();
void rotateOne();
void rotateTwo();
void unRotate();
void zeroEncoder();
void stopRobot();

int encoderCount = 0; // holds the (total average wheel rotations * 1440)
const int PENDU_LENGTH = 7219; // wall-to-wall in encoder ticks: vital for getting on the ramp

void initializeRobot()
{
  // Place code here to initialize servos to starting positions.
  // Sensors are automatically configured and setup by ROBOTC. They may need a brief time to stabilize.

	// DSP mode is set to 1200 Hz
	tHTIRS2DSPMode _mode = DSP_1200;

	// attempt to initialize IR sensor
	if (HTIRS2setDSPMode(IRSeeker, _mode) == 0)
	{
		// if fail, display error and make noise
		eraseDisplay();
		nxtDisplayCenteredTextLine(0, "ERROR!");
		nxtDisplayCenteredTextLine(2, "DSP INIT FAILED");
		PlaySound(soundException);
		wait10Msec(300);
		return;
	}

  return;
}

task main()
{
  initializeRobot();

  waitForStart(); // Wait for the beginning of autonomous phase.
	wait10Msec(500);
 	mainAuto(); // Program is fully modular!
}

void mainAuto()
{
	searchDirection();
	eraseDisplay();
	searchStrength();

	// 3438 ticks = half way point
	if (encoderCount >= 3438) {
		rotateTwo();
	} else {
		rotateOne();
	}

	unRotate();
}

void searchDirection()
{
	int _dirAC = 0;
	int trueDir = 0;
	int maxSig = 0;
	int acS1, acS2, acS3, acS4, acS5;

	zeroEncoder();

	// IR direction search algorithm
	while(_dirAC != 5)
	{
		// Read the current IR signal directions
		_dirAC = HTIRS2readACDir(IRSeeker);
		if ((_dirAC < 0))
		{
			writeDebugStreamLine("Read dir ERROR!");
			break; // I2C read error occurred
		}

		// Read the individual signal strengths of the internal sensors
		if (!HTIRS2readAllACStrength(IRSeeker, acS1, acS2, acS3, acS4, acS5))
		{
			writeDebugStreamLine("Read dir ERROR!");
			break; // I2C read error occurred
		} else {
			// find the max sig strength of all detectors
			maxSig = (acS1 > acS2) ? acS1 : acS2;
			maxSig = (maxSig > acS3) ? maxSig : acS3;
			maxSig = (maxSig > acS4) ? maxSig : acS4;
			maxSig = (maxSig > acS5) ? maxSig : acS5;
		}

		// which way to go?
		// 0 = no signal found
		// 1 = far left (~8 o'clock)
		// 5 = straight ahead (~12 o'clock)
		// 9 = far right (~4 o'clock)

		encoderCount = (nMotorEncoder[Lwheel] + nMotorEncoder[Rwheel]) / 2;

		// raw, dirty direction -> nice, workable direction! :-)
		trueDir = abs(_dirAC - 5);

		// only concerned with moving back/forward
		motor[Lwheel] = 20 + 5 * trueDir;
		motor[Rwheel] = 20 + 5 * trueDir;
		wait1Msec(20);
	}
}

void searchStrength()
{
	int acS1, acS2, acS3, acS4, acS5;
	int acTemp[3];
	bool offTarget = true;

	// IR max strength search
	while(offTarget)
	{
		// get input
		for (int i = 0; i <= 2; ++i)
		{
			motor[Lwheel] = 15;
			motor[Rwheel] = 15;
			wait10Msec(50);
			// Read the individual signal strengths of the internal sensors
			if (!HTIRS2readAllACStrength(IRSeeker, acS1, acS2, acS3, acS4, acS5))
			{
				writeDebugStreamLine("Read dir ERROR!");
				break; // I2C read error occurred
			} else {
				nxtDisplayCenteredBigTextLine(3, "Sig=%d", acS3);
				acS3 = acTemp[i];
			}
		}

		// compare 3 IR strength readings
		if ((acTemp[2] > acTemp[1]) && (acTemp[2] > acTemp[0]))
		{
			motor[Lwheel] = 15;
			motor[Rwheel] = 15;
			wait10Msec(20);
		} else if ((acTemp[2] < acTemp[1]) && (acTemp[2] > acTemp[0])) {
			motor[Lwheel] = -13;
			motor[Rwheel] = -13;
			wait10Msec(20);
		} else {
			offTarget = false;
			stopRobot();
		}
		wait1Msec(500);
	}
}

void rotateOne()
{
	motor[Arm] = 50;
	wait1Msec(1250);

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Rwheel] <= 800) {
		motor[Lwheel] = -20;
		motor[Rwheel] = 20;
	}
	stopRobot();

	motor[Ltread] = 100;
	motor[Rtread] = 100;
	wait1Msec(1000);

	stopRobot();
}

void rotateTwo()
{
	motor[Lwheel] = -10;
	motor[Rwheel] = -10;
	wait1Msec(750);

	stopRobot();

	motor[Arm] = 50;
	wait1Msec(1500);

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Rwheel] <= 850) {
		motor[Lwheel] = -20;
		motor[Rwheel] = 20;
	}
	stopRobot();

	motor[Ltread] = 100;
	motor[Rtread] = 100;
	wait1Msec(1000);

	stopRobot();
}

void unRotate()
{
	zeroEncoder();
	while(nMotorEncoder[Lwheel] <= 800) {
		motor[Lwheel] = 20;
		motor[Rwheel] = -20;
	}
	stopRobot();
	motor[Arm] = -2;
	wait1Msec(1000);
	motor[Arm] = 0;
	wait1Msec(20);

	/* TO THE RAMP! */
	zeroEncoder();

	/* EXPERIMENTAL MATH */
	while(nMotorEncoder[Lwheel] <= (PENDU_LENGTH - encoderCount - 800)) {
		motor[Lwheel] = 60;
		motor[Rwheel] = 60;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Rwheel] <= 800) {
		motor[Lwheel] = -40;
		motor[Rwheel] = 40;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Lwheel] <= 4000) { // -1440
		motor[Lwheel] = 60;
		motor[Rwheel] = 60;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Rwheel] <= 800) {
		motor[Lwheel] = -40;
		motor[Rwheel] = 40;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Lwheel] <= 1000) {
		motor[Lwheel] = 60;
		motor[Rwheel] = 60;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Rwheel] <= 1540) {
		motor[Lwheel] = -40;
		motor[Rwheel] = 40;
	}

	stopRobot();
	zeroEncoder();
	while(nMotorEncoder[Lwheel] <= 4560) {
		motor[Lwheel] = 70;
		motor[Rwheel] = 70;
	}
}

/* REPETITIVE PROCESSES GO HERE */
void zeroEncoder()
{
	nMotorEncoder[Lwheel] = 0;
	nMotorEncoder[Rwheel] = 0;
}

void stopRobot()
{
	motor[Lwheel] = 0;
	motor[Rwheel] = 0;
	motor[Arm] = 0;
	motor[Ltread] = 0;
	motor[Rtread] = 0;
	wait1Msec(20);
}
