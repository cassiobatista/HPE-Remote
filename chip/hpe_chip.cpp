/*
 * Head Pose Estimation with OpenCV
 *
 * Copyright 2017
 * Laboratório de Visualização, Interação e Sistemas Inteligentes (LabVIS)
 * Universidade Federal do Pará (UFPA)
 *
 * References:
 * [1] Euclides N. Arcoverde Neto, Rafael M. Duarte, Rafael M. Barreto, João 
 *     Paulo Magalhães, Carlos C. M. Bastos, Tsang Ing Ren, and George D. C. 
 *     Cavalcanti.  2014.
 *     Enhanced real-time head pose estimation system for mobile device.
 *     Integr. Comput.-Aided Eng. 21, 3 (July 2014), 281-293. 
 *
 * Authors: Jan 2017
 * Erick Modesto Campos    - erick.c.modesto@gmail.com
 * Cassio Trindade Batista - cassio.batista.13@gmail.com
 * Nelson C. Sampaio Neto  - dnelsonneto@gmail.com
 */

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <signal.h>

#include "bluetooth.h"
#include "bluetooth.c"

#include "simple_gpio.h"
#include "simple_gpio.c"

#define DEBUG 0
#define HAAR_PATH "../haarcascade_frontalface_default.xml"

using namespace cv;
using namespace std;

bool is_face = false;
char esc = 30;

//Keyboard interrupt routine
void turn_off(int dummy)
{
	esc = 27;
	is_face = true;
	is_face = false;
	is_face = true;
	is_face = false;
	sleep(1);

	/* Turn red light on */
	gpio_set_value(LED_RED,   HIGH);
	gpio_set_value(LED_AMBER, HIGH);
	gpio_set_value(LED_GREEN, HIGH);

	gpio_unexport(LED_RED);
	gpio_unexport(LED_AMBER);
	gpio_unexport(LED_GREEN);

	gpio_unexport(XT_SWITCH);
}

int main(int argc, char** argv)  
{  
	//Variables Viola-Jones
	int face_count = 0;
	CascadeClassifier face_model;
	vector<Rect> face(2);
	
	int* pack;
	int status, sock;
	
	Point b;

	//Variables OpticalFlow
	Mat frame, Prev, Prev_Gray, Current, Current_Gray; 
	vector<Point2f> ponto(3);
	vector<Point2f> saida(3);
	Mat err;
	vector<uchar> STATUS(3);
	vector<Point2i> face_triang(3); // RE, LE n' N
	int re_x, le_x, eye_y, face_dim;

	ifstream haar(HAAR_PATH);
	if(haar.good()) {
		face_model.load(HAAR_PATH);
	} else {
		cout << "error: unable to load file'" << HAAR_PATH << "'." << endl;
		exit(2);
	}

	int roll, yaw, pitch;
	int yaw_count, pitch_count, roll_count, noise_count;

	/* Export tell kernel I'm gonna use GPIO pins */
	gpio_export(LED_RED);
	gpio_export(LED_AMBER);
	gpio_export(LED_GREEN);
	gpio_export(XT_SWITCH);

	/* Direction: define them as output pins */
	gpio_set_dir(LED_RED,   OUTPUT_PIN);
	gpio_set_dir(LED_AMBER, OUTPUT_PIN);
	gpio_set_dir(LED_GREEN, OUTPUT_PIN);
	gpio_set_dir(XT_SWITCH, INPUT_PIN);

	/* Make sure all LEDs start off */
	gpio_set_value(LED_RED,   HIGH);
	gpio_set_value(LED_AMBER, HIGH);
	gpio_set_value(LED_GREEN, HIGH);

	/* Init camera */
	VideoCapture camera(0);

	// http://blog.lemoneerlabs.com/3rdParty/Darling_BBB_30fps_DRAFT.html#x1-2000
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

	signal(SIGINT, turn_off);

	/* Open Bluetooth socket */
	pack = bt_open_conn();
	if(pack == NULL)
		return -1;

	status = pack[0];
	sock = pack[1];
	free(pack);

	for(;;) {
		face_count = 0;

		/* Turn red light on */
		gpio_set_value(LED_RED, LOW);
		gpio_set_value(LED_AMBER, HIGH);
		gpio_set_value(LED_GREEN, HIGH);

		/* wait for activity on external switch */
		cout << "waiting for external AT switch" << endl;
		if(gpio_get_value(XT_SWITCH) == '0')
			while(gpio_get_value(XT_SWITCH) == '0')
				if(esc == 27)
					break;

		/* TODO: What does it do? */
		printf("Trying to detect a face...\n");
		while(!is_face) {

			/* get captured frame from device */
			camera >> frame;

			//Face detector Viola-Jones
			face_model.detectMultiScale(frame, face, 2.1, 3, 0|CV_HAAR_SCALE_IMAGE, Size(100, 100));
	
			//Draw a rectangle around the detected face
			for(int i=0; i < face.size(); i++){
				rectangle(frame, face[i], CV_RGB(0, 255, 0), 1);
				face_dim = frame(face[i]).size().height; // a = face_dimension
			}

			//Initial Coordinates of eyes and nose
			re_x  = face[0].x + face_dim*0.3; //left eye
			le_x  = face[0].x + face_dim*0.7; //right eye
			eye_y = face[0].y + face_dim*0.38;//height of the eyes 

			/* TODO: What does it do? */
			ponto[0] = cvPoint(re_x, eye_y);//right eye coordinate
			ponto[1] = cvPoint(le_x, eye_y);//left eye coordinate
			ponto[2] = cvPoint((re_x+le_x)/2, eye_y+int((le_x-re_x)*0.45));//nose coordinate
 
			//Draw a cicle around the eyes and nose coordinates
			for(int i=0; i<3; i++)
				circle(frame, ponto[i], 2, Scalar(10,10,255), 4, 8, 0);

			//Turn the yellow led on if a face is detected. It count the frames
			//while the face is detected
			if((int) face.size() > 0) {// Frames
				face_count++;
				gpio_set_value(LED_AMBER, LOW); // yellow light: I'm getting it
			} else {
				face_count = 0;
				gpio_set_value(LED_AMBER, HIGH);
			}

			#if DEBUG
				printf(".");
			#endif
			/* TODO: What does it do? */
			
			//If a face is detected in 10 consecutives, the viola-jones
			//algorithm is terminated and OpticalFlow algorithm initiates
			if(face_count >= 10) {
				#if DEBUG
					printf("\n");
				#endif
				is_face = true;
				// Stores the last frame from Viola-jones
				frame.copyTo(Prev);
				cvtColor(Prev, Prev_Gray, CV_BGR2GRAY);// Gray Scale

				/* Turn green light on */
				gpio_set_value(LED_RED, HIGH);
				gpio_set_value(LED_AMBER, HIGH);
				gpio_set_value(LED_GREEN, LOW);
			}

			if(esc == 27)
				break;
		}//close while not face
	
		//Movementes counts
		yaw_count = 0;
		pitch_count = 0;
		roll_count = 0;
		noise_count = 0;

		//OpticalFlow
		while(is_face) {

			/* get captured frame from device */
			camera >> frame;

			/* TODO: What does it do? */
			//Current frame in gray scale
			frame.copyTo(Current);
			cvtColor(Current, Current_Gray, CV_BGR2GRAY);

			//Opticalflow is calculated
			cv::calcOpticalFlowPyrLK(Prev_Gray, Current_Gray, ponto,
					saida, STATUS, err, Size(15,15), 1);

			//Get the pticalflow points
			for(int i=0; i<3; i++)
				face_triang[i] = saida[i];
			//Distance between eyes and nose
			float d_e2e = sqrt(
						pow((face_triang[0].x-face_triang[1].x),2) +
						pow((face_triang[0].y-face_triang[1].y),2));

			float d_re2n = sqrt(
						pow((face_triang[0].x-face_triang[2].x),2) +
						pow((face_triang[0].y-face_triang[2].y),2));

			float d_le2n = sqrt(
						pow((face_triang[1].x-face_triang[2].x),2) +
						pow((face_triang[1].y-face_triang[2].y),2));

			//Error conditions to opticalflow algorithm to stop
			if(d_e2e/d_re2n < 0.5  || d_e2e/d_re2n > 3.5) {
				cout << "too much noise 0." << endl;
				is_face = false;
				break;
			}
			if(d_e2e/d_le2n < 0.5 || d_e2e/d_le2n > 3.5) {
				cout << "too much noise 1." << endl;
				is_face = false;
				break;
			}

			if(d_e2e > 160.0 || d_e2e < 20.0) {
				cout << "too much noise 2." << endl;
				is_face = false;
				break;
			}

			if(d_re2n > 140.0 || d_re2n < 10.0) {
				cout << "too much noise 3." << endl;
				is_face = false;
				break;
			}
			if(d_le2n > 140.0 || d_le2n < 10.0) {
				cout << "too much noise 4." << endl;
				is_face = false;
				break;
			}
		
			//Draw a circle around the calculated points by opticalflow 
			for(int i=0; i<3; i++)
				circle(frame, face_triang[i], 2, Scalar(255,255,5), 4, 8, 0);
			
			//Head rotation axes
			float param = (face_triang[1].y-face_triang[0].y) / (float)(face_triang[1].x-face_triang[0].x);
			roll  = 180*atan(param)/M_PI;
			yaw   = ponto[2].x - face_triang[2].x;
			pitch = face_triang[2].y - ponto[2].y;
			
			# if DEBUG
				printf("%4d,%4d,%4d\t%4d\n", yaw, pitch, roll, noise_count);
			#endif

			/* debug */
			printf("%4d,%4d,%4d\t%4d\n", yaw, pitch, roll, noise_count);

			/* Estimate yaw */
			if((yaw > -20 && yaw < 0) || (yaw > 0 && yaw < +20)) {
				yaw_count += yaw;
			} else {
				yaw_count = 0;
				if(yaw < -40 || yaw > +40) {
					noise_count++;
				}
			}

			/* Estimate pitch */
			if((pitch > -20 && pitch < 0) || (pitch > 0 && pitch < +20)) {
				pitch_count += pitch;
			} else {
				pitch_count = 0;
				if(pitch < -40 || pitch > +40) {
					noise_count++;
				}
			}

			/* Estimate roll */
			if((roll > -60 && roll < -15) || (roll > +15 && roll < +60)) {
				roll_count += roll;
			} else {
				roll_count = 0;
				if(roll < -60 || roll > +60) {
					noise_count++;
				}
			}

			/* Check for noised signals */
			if(noise_count > 2) {
				cout << "too much noise." << endl;
				is_face = false;
				break;
			}

			/* Too much noise between roll and yaw */
			if(roll_count > -1 && roll_count < +1) {

				/* Check if it is YAW */
				if(yaw_count <= -20) {
					cout << "yaw left\tcanal menos" << endl;
					status = bt_send(sock, status, "YL");
					is_face = false;
					break;
				} else if(yaw_count >= +20) { 
					cout << "yaw right\tcanal mais" << endl;
					status = bt_send(sock, status, "YR");
					is_face = false;
					break;
				}

				/* Check if it is PITCH */
				if(pitch_count <= -15) {
					cout << "pitch up\tvolume mais" << endl;
					status = bt_send(sock, status, "PU");
					is_face = false;
					break;
				} else if(pitch_count >= +15) { 
					cout << "pitch down\tvolume menos" << endl;
					status = bt_send(sock, status, "PD");
					is_face = false;
					break;
				}
			} else {

				/* Check if it is ROLL */
				if(roll_count < -120) {
					cout << "roll right\tligar televisao" << endl;
					status = bt_send(sock, status, "RR");
					is_face = false;
					break;
				} else if(roll_count > +120) { 
					cout << "roll left\tdesligar televisao" << endl;
					status = bt_send(sock, status, "RL");
					is_face = false;
					break;
				}
			}
			// check if C.H.I.P is connected with arduino's bluetooth
			if(status < 0)
				printf("vish maria");

			//It stores the found points.
			for(int j=0; j<4; j++)
				ponto[j] = saida[j];
			//Current frame become previous frame
			Current_Gray.copyTo(Prev_Gray);

			if(esc == 27)
				break;
	
		}//close while isface

		if(esc == 27)
			break;

	}//close infinity for

	/* Close Bluetooth connection */
	bt_close_conn(sock);

	return 0;
}//end main

