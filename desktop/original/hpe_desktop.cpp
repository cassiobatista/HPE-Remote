/*
 * Head Pose Estimation with OpenCV
 * Head Gesture Recognition
 *
 * Copyright 2017
 * LabVIS - Laboratório de Visualização, Interação e Sistemas Inteligentes
 * UFPA   - Universidade Federal do Pará
 *
 * References:
 * [1] Euclides N. Arcoverde Neto, Rafael M. Duarte, Rafael M. Barreto, João 
 *     Paulo Magalhães, Carlos C. M. Bastos, Tsang Ing Ren, and George D. C. 
 *     Cavalcanti. 2014.
 *     Enhanced real-time head pose estimation system for mobile device.
 *     Integr. Comput.-Aided Eng. 21, 3 (July 2014), 281-293. 
 *
 * Authors: Jan 2017
 * Erick Modesto Campos    - erick.c.modesto@gmail.com
 * Cassio Trindade Batista - cassio.batista.13@gmail.com
 */

#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include <stdio.h>

#include <iostream>
#include <fstream>

#define DEBUG 0
#define HAAR_PATH "../../haarcascade_frontalface_default.xml"

using namespace cv;
using namespace std;

int main(int argc, char** argv)  
{  
	bool is_face = false;
	int face_count = 0;

	/* variables viola-jones */
	CascadeClassifier face_model;
	vector<Rect> face(2);

	/* frames from webcam */
	VideoCapture camera(0); // 0: onboard; 1: USB

	/* variables OpticalFlow */
	Mat frame, Prev, err, Prev_Gray, Current, Current_Gray; 
	vector<Point2f> prev_points_2d(3), current_points_2d(3);
	vector<uchar> status(3);
	vector<Point2i> face_triang(3); // RE, LE and N
	int re_x, le_x, eye_y, face_dim; // for antropometric relations

	ifstream haar(HAAR_PATH);
	if(haar.good()) {
		face_model.load(HAAR_PATH);
	} else {
		cout << "error: unable to load file'" << HAAR_PATH << "'." << endl;
		exit(2);
	}

	int roll, yaw, pitch;
	int yaw_count, pitch_count, roll_count, noise_count;
	char esc;

	string action;

	/* infinite loop */
	for(;;) {

		/* reset face counter */
		face_count = 0;

		/* while a face is not found */
		while(!is_face) {

			/* get captured frame from device */
			camera >> frame;

			/* Viola-Jones face detector */
			face_model.detectMultiScale(frame, face, 2.1, 3, 
						0|CV_HAAR_SCALE_IMAGE, Size(100, 100));

			/* draw a green rectangle (square) around the detected face */
			for(int i=0; i < face.size(); i++){
				rectangle(frame, face[i], CV_RGB(0, 255, 0), 1);
				face_dim = frame(face[i]).size().height; // a = face_dimension
			}

			/* anthropometric initial coordinates of eyes and nose */
			re_x  = face[0].x + face_dim*0.3;  // right eye
			le_x  = face[0].x + face_dim*0.7;  // left eye
			eye_y = face[0].y + face_dim*0.38; // height of the eyes 

			/* define 3 face points in a 2D plan */
			prev_points_2d[0] = cvPoint(re_x, eye_y);  // right eye
			prev_points_2d[1] = cvPoint(le_x, eye_y);  // left eye
			prev_points_2d[2] = cvPoint((re_x+le_x)/2, // nose
						eye_y+int((le_x-re_x)*0.45));

			///* draw a red cicle (dtr) around eyes and nose coordinates */
			//for(int i=0; i<3; i++)
			//	circle(frame, prev_points_2d[i], 2, Scalar(10,10,255), 4, 8, 0);

			/* increase counter if a face is found according to the cascade */
			/* reset frame counter otherwise
			 * because we need 10 *consecutive* frames with a face */
			if((int) face.size() > 0) // frame
				face_count++;
			else
				face_count = 0;

			/* if a face is detected for 10 consecutives, face detection step
			 * stops and the head pose estimation step starts */
			if(face_count >= 10) {
				is_face = true; // ensure breaking Viola-Jones loop
				frame.copyTo(Prev); // current frame becomes 'previous frame'
				cvtColor(Prev, Prev_Gray, CV_BGR2GRAY); // jorah mormont
			}

			flip(frame, frame, 1);   // we've got some problems with orientation
			imshow("Camera", frame); // show frames

			/* wait for user to abort */
			esc = waitKey(30);
			if(esc == 27) // ESC to exit
				break;
		}// close while not face 

		/* (re)set pose counters (to zero) */
		yaw_count = 0;
		pitch_count = 0;
		roll_count = 0;
		noise_count = 0; // defines error

		/* while there's a face on the frame */
		while(is_face) {

			action = "ERROR";

			/* get captured frame from device */
			camera >> frame;

			/* convert current frame to gray scale */
			frame.copyTo(Current); // captured frame is the current frame
			cvtColor(Current, Current_Gray, CV_BGR2GRAY); // jorah mormont

			/* Lucas-Kanade algorithm calculates the optical flow */
			/* the points are stored in the variable 'current_points_2d' */
			cv::calcOpticalFlowPyrLK(Prev_Gray, Current_Gray,
						prev_points_2d, current_points_2d, 
						status, err, Size(15,15), 1);

			/* Get the three optical flow points 
			 * right eye = face_triang[0]
			 * left eye  = face_triang[1]
			 * nose      = face_triang[2] */
			for(int i=0; i<3; i++)
				face_triang[i] = current_points_2d[i];

			/* TODO: Geometry of a triangle
			 * 1) http://stackoverflow.com/questions/19835174/how-to-check-if-3-sides-form-a-triangle-in-c
			 * 2) http://math.stackexchange.com/questions/516219/finding-out-the-area-of-a-triangle-if-the-coordinates-of-the-three-vertices-are
			 * 3) Distance from camera to the user's face is inversely 
			 *    propotional to the distance between the eyes
			 */

			/* calculate the distance between eyes */
			float d_e2e = sqrt(
						pow((face_triang[0].x-face_triang[1].x),2) +
						pow((face_triang[0].y-face_triang[1].y),2));

			/* calculate the distance between right eye and nose */
			float d_re2n = sqrt(
						pow((face_triang[0].x-face_triang[2].x),2) +
						pow((face_triang[0].y-face_triang[2].y),2));

			/* calculate the distance between left eye and nose */
			float d_le2n = sqrt(
						pow((face_triang[1].x-face_triang[2].x),2) +
						pow((face_triang[1].y-face_triang[2].y),2));

			#if DEBUG
				printf("(%3.8f, %3.8f), %3.8f\n", d_re2n, d_le2n, d_e2e);
				printf("%3.8f, %3.8f\n", d_e2e/d_re2n, d_e2e/d_le2n);
			#endif
			
			/* Error conditions to opticalflow algorithm to stop */
			/* ratio: distance of the eyes / distance from right eye to nose */
			/* calculate the distance between left eye and nose */
			if(d_e2e/d_re2n < 0.5  || d_e2e/d_re2n > 2.5) {
				#if DEBUG
					printf("(%3.8f, %3.8f), %3.8f\n", d_re2n, d_le2n, d_e2e);
				#endif
				cout << "too much noise 0." << endl;
				is_face = false;
				break;
			}
			/* ratio: distance of the eyes / distance from left eye to nose */
			if(d_e2e/d_le2n < 0.5 || d_e2e/d_le2n > 2.5) {
				cout << "too much noise 1." << endl;
				is_face = false;
				break;
			}

			/* distance between the eyes */
			if(d_e2e > 160.0 || d_e2e < 20.0) {
				cout << "too much noise 2." << endl;
				is_face = false;
				break;
			}

			/* distance from the right eye to nose */
			if(d_re2n > 140.0 || d_re2n < 10.0) {
				cout << "too much noise 3." << endl;
				is_face = false;
				break;
			}
			/* distance from the left eye to nose */
			if(d_le2n > 140.0 || d_le2n < 10.0) {
				cout << "too much noise 4." << endl;
				is_face = false;
				break;
			}

			/* draw a cyan circle (dot) around the points calculated by optical flow */
			for(int i=0; i<3; i++)
				circle(frame, face_triang[i], 2, Scalar(255,255,5), 4, 8, 0);

			/* head rotation axes */
			float param = (face_triang[1].y-face_triang[0].y) / (float)(face_triang[1].x-face_triang[0].x);
			roll  = 180*atan(param)/M_PI;                    // eq. 1
			yaw   = prev_points_2d[2].x - face_triang[2].x;  // eq. 2
			pitch = face_triang[2].y - prev_points_2d[2].y;  // eq. 3

			#if DEBUG
				printf("%4d,%4d,%4d\t%4d\n", yaw, pitch, roll, noise_count);
			#endif

			/* Estimate yaw left and right intervals */
			if((yaw > -20 && yaw < 0) || (yaw > 0 && yaw < +20)) {
				yaw_count += yaw;
			} else {
				yaw_count = 0;
				if(yaw < -40 || yaw > +40) {
					noise_count++;
				}
			}

			/* Estimate pitch up and down intervals */
			if((pitch > -20 && pitch < 0) || (pitch > 0 && pitch < +20)) {
				pitch_count += pitch;
			} else {
				pitch_count = 0;
				if(pitch < -40 || pitch > +40) {
					noise_count++;
				}
			}

			/* Estimate roll left and right intervals */
			if((roll > -60 && roll < -15) || (roll > +15 && roll < +60)) {
				roll_count += roll;
			} else {
				roll_count = 0;
				if(roll < -60 || roll > +60) {
					noise_count++;
				}
			}

			/* check for noised signals. Stops if there were more than 2 */
			if(noise_count > 2) {
				cout << "too much noise." << endl;
				is_face = false;
				break;
			}

			/* too much noise between roll and yaw */
			/* to ensure a YAW did happen, make sure a ROLL did NOT occur */
			if(roll_count > -1 && roll_count < +1) {
				if(yaw_count <= -20) {
					action = "canal anterior";
					cout << "yaw left\tprevious channel" << endl;
					is_face = false;
					break;
				} else if(yaw_count >= +20) { 
					action = "proximo canal";
					cout << "yaw right\tnext channel" << endl;
					is_face = false;
					break;
				}

				/* Check if it is PITCH */
				if(pitch_count <= -10) {
					action = "aumentar volume";
					cout << "pitch up\tincrease volume" << endl;
					is_face = false;
					break;
				} else if(pitch_count >= +10) { 
					action = "diminuir volume";
					cout << "pitch down\tdecrease volume" << endl;
					is_face = false;
					break;
				}
			} else {
				/* Check if it is ROLL */
				if(roll_count < -150) {
					action = "ligar televisao";
					cout << "roll right\tturn TV on" << endl;
					is_face = false;
					break;
				} else if(roll_count > +150) { 
					action = "desligar televisao";
					cout << "roll left\tturn TV off" << endl;
					is_face = false;
					break;
				}
			}

			/* store the found points */
			for(int j=0; j<4; j++)
				prev_points_2d[j] = current_points_2d[j];

			/* current frame now becomes the previous frame */
			Current_Gray.copyTo(Prev_Gray);

			flip(frame, frame, 1);   // optional: orientation problems
			imshow("Camera", frame); //show frames from webcam

			/* wait for user to abort */
			esc = waitKey(30);
			if(esc == 27) // ESC to exit
				break;

		}//close while isface

		if(esc == 27)
			break;

		/* give time to arduino send the control command to the TV */
		for(int f=0; f<40; f++) {
			camera >> frame;
			flip(frame, frame, 1); // left is right, right is left

			putText(frame, action, Point2f(10,300),
						FONT_HERSHEY_SIMPLEX, 2, cvScalar(255,50,100), 5);
			imshow("Camera", frame);

			esc = waitKey(30);
			if(esc == 27)
				break;
		}

	}//close while true (for ;;)

	return 0;
}//fim main
