#pragma config(Sensor, in1,    analog1,        sensorAnalog)
#pragma config(Sensor, dgtl1,  button,         sensorTouch)
#pragma config(Sensor, dgtl2,  limitSwitch,    sensorTouch)
#pragma config(Sensor, dgtl3,  ultrasonicInput, sensorSONAR_cm)
#pragma config(Sensor, dgtl10, digital10,      sensorDigitalOut)
#pragma config(Sensor, dgtl11, digital11,      sensorDigitalOut)
#pragma config(Sensor, dgtl12, digital12,      sensorDigitalOut)
#pragma config(Motor,  port3,  rightMotor,     tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port5,  robotArm,       tmotorServoStandard, openLoop) 
#pragma config(Motor,  port2,  leftMotor,      tmotorVex393_HBridge, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//


// Global Variables
int freq, ambient_level, slow_level, stop_level, expose_time, steer_sensitivity, forward_speed, slow_speed, spin_speed;

// initialize PD values for the global uses - will be modified at the appropriate timing in ReadPD function.
int PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD_sum;
// initialize max values
int max_val, max_no = 0;

/*The accumulator accumulates(or integrates) the rectified signal over a period of time (set by the expose and read)
The accumulated voltage read by SensorValue[analog1](an analog voltage) is read by the controller.
The expose_time period essentially set the ?gain? or ?sensitivity? of the overall circuit.*/
int Expose_and_read(){
	SensorValue[digital11] = 1; // close the shutter, clear the film, and increment the counter
	delay(5); // Wait for 5ms for things to settle
	SensorValue[digital11] = 0; // open shutter for exposure
	delay(expose_time); // expose time = 3ms to 8ms (can be adjusted)
	int intensity = SensorValue[analog1]; // get the IR intensity reading
	return intensity; // return the analog input 1
}

/*Read_PD calculates PD_sum which is the sum of all 8 photo diodes?outputs.
PD_sum is a measure of the distance between the robot and the beacon. The sensing
sensitivity is set by expose_time*/

void ReadPD(){
	SensorValue[digital11] = 1; // close the shutter, clear the film, and increment the counter
	SensorValue[digital12] = 1; // initialize counter value to '0'
	SensorValue[digital12] = 0; // allow counter to count
	delay(5); // 5ms wait for things settling down
	SensorValue[digital11] = 0; // open shutter and expose film
	delay(expose_time); // expose time = 3ms to 8ms (can be adjusted)
	PD0 = SensorValue[analog1];
	PD1 = Expose_and_read();
	PD2 = Expose_and_read();
	PD3 = Expose_and_read();
	PD4 = Expose_and_read();
	PD5 = Expose_and_read();
	PD6 = Expose_and_read();
	PD7 = Expose_and_read();
	PD_sum = PD0 + PD1 + PD2 + PD3 + PD4 + PD5 + PD6 + PD7;
}

/*This function compares the magnitude of the variables PD0~PD7 and stores the maximum value
in max_val and the number of the photo diode that has the maximum value in max_no.*/

void Find_max(){
	max_val = PD0; // default = PD0;
	max_no = 0;

	if(PD1 > max_val){
		max_val = PD1;
		max_no = 1;
	}
	if(PD2 > max_val){
		max_val = PD2;
		max_no = 2;
	}
	if(PD3 > max_val){
		max_val = PD3;
		max_no = 3;
	}
	if(PD4 > max_val){
		max_val = PD4;
		max_no = 4;
	}
	if(PD5 > max_val){
		max_val = PD5;
		max_no = 5;
	}
	if(PD6 > max_val){
		max_val = PD6;
		max_no = 6;
	}
	if(PD7 > max_val){
		max_val = PD7;
		max_no = 7;
	}
}

/*
Limits results of move function to max speed	and output to the motors*/
int limit_pwm(int temp){
	int limited = 0;

	if(temp > 127){
		limited = 127;
	} else if (temp < -127){
		limited = -127;
	} else{
		limited = temp;
	}

	return limited;

}

/*Move function helps to check if beacon is found or otherwise it will be search mode	 looking for the beacon.
This function also calculates the speed, steering sensitivity and the heading direction so the robot moves to beacon*/

void Move(){
	int tempSpeed = 0;
	int error = 4 - max_no;//heading direction error , if PD4==max_no, then no error
	int steer = error * steer_sensitivity;//steering effort is proportioinal to heading error
	int speed = forward_speed;//forward speed (normal speed)

	if(PD_sum < ambient_level){ // looking for the beacon if background noise is
		speed = 0;//serach mode=>no forward motion
		steer = -spin_speed;//search mode =>spin
	}
	if(PD_sum > slow_level){//Beacon is near!
		speed = slow_speed;//slow down
	}
	// deleted the stop statement when the beacon is found, because we'll	 use the limit switch to stop the motors	which is more reliable.


	tempSpeed = limit_pwm(steer + speed);
	motor[leftMotor] = tempSpeed; // left, port 1
	tempSpeed = limit_pwm(steer - speed);
	motor[rightMotor] = tempSpeed; // right, port	 10
}

/*
The GOBEACON main program essentially sets up all the configuration variables and repeatedly
execute the three routines: Read_PD, find_max, and move.*/
task main(){
	freq = 0; // 0 = 1khz (red) 1 = 10khz (green)
	ambient_level = 200; // used in 'move'
	slow_level = 5000;// used in move
	stop_level = 6000;//used in move
	expose_time = 5; // expose time was changed from 3ms to 5ms (3ms in easyC -> 5ms in RobotC)
	steer_sensitivity = 20;//used in move
	forward_speed = 35;//forward speed , used in move
	slow_speed = 25;//slow speed , used in move
	spin_speed = 50;//spin speed (for searching mode),used in move
	SensorValue[digital10] = freq;// turn to 1KHz(red beacon)

	bool start = false;

	int state = 1;
	int armDown = -120;
	int armUp = 120;

	setServo(robotArm, servoDown);
while (true) {
        // Toggle start with button press
        if (SensorValue[button] == 0) {
            start = !start;
            //while(SensorValue[button] == 1); // Debounce to avoid double toggle
        }
        
        // Main operation loop if start is true
        if (start) {
            while (start) {
                // State 1: Locate and approach red beacon
                if (state == 1) {
                    
					while (SensorValue[limitSwitch] == 0){
					ReadPD();
                    Find_max();
                    Move();
					if (SensorValue[limitSwitch] == 1) { // Detect limit switch press
                        motor[leftMotor] = 0;
                        motor[rightMotor] = 0;
					}
					}
					state++;
                }

                // State 2: Turn off red beacon using arm
                if (state == 2) {
                    setServo(robotArm, armDown); // Lower arm to press red beacon

					ReadPD();
					Find_max();

					while(PD_sum > slow_level){
                    wait1Msec(5000);
					setServo(robotArm, 100); // Lower arm to press red beacon
                    wait1Msec(5000);
                    setServo(robotArm, armUp); // Retract arm
                    wait1Msec(5000);

					if(PD_sum > slow_level){
						ReadPD();
						Find_max()
						Move();
						}
					}
					motor[leftMotor] = -100;	//might need to be fixed
                    motor[rightMotor] = -100;
                    wait1Msec(2000);
                    motor[leftMotor] = 0;
                    motor[rightMotor] = 0;
					//state++;

				}
/*
                // State 3: Locate and approach green beacon
                if (state == 3) {
                    

                    freq = 1; // Change to 10 kHz for green beacon
                    SensorValue[digital10] = freq;
                    ReadPD();
                    Find_max();
                    Move();
                    state++;
                }

                // State 4: Grab green beacon with limit switch
                if (state == 4) {
                    if (SensorValue[limitSwitch] == 1) { // Detect limit switch press
                        motor[leftMotor] = 0;
                        motor[rightMotor] = 0;

                        // Lower arm to grab beacon
                        motor[robotArm] = 100;
                        wait1Msec(5000); // Adjust as needed for secure grip
                        motor[robotArm] = 0;

                        // Check for signal to verify contact with beacon
                        if (SensorValue[analog1] > thresholdValue) { 
                            // Hold arm down to grab beacon securely
                            setServo(robotArm, armDown);
                            state++;
                        }
                    }
                }

                // State 5: Navigate out of the arena using ultrasonic sensor
                if (state == 5) {
                    int distance = SensorValue[ultrasonicInput];

                    while (distance < maxDistance) {  // maxDistance is set based on arena layout
                        distance = SensorValue[ultrasonicInput];
                        if (distance >= maxDistance) {
                            // Move towards the furthest detected distance
                            motor[leftMotor] = forward_speed;
                            motor[rightMotor] = forward_speed;
                        } else {
                            motor[leftMotor] = 0;
                            motor[rightMotor] = 0;
                        }
                    }

                    // Stop the robot after reaching a clear area
                    motor[leftMotor] = 0;
                    motor[rightMotor] = 0;
                    state++;
                }

                // State 6: End program
                if (state == 6) {
                    break;
                }
				*/
            }
        }
    }
}
