/*
 * Head Pose Estimation with OpenCV
 *
 * Copyright 2017
 * Grupo FalaBrasil
 * Universidade Federal do Pará
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

#define DEBUG 0

using namespace cv;
using namespace std;

bool is_face = false;
char esc;

int main(int argc, char** argv)  
{  
	int face_count = 0;
	vector<Point2f> ponto(3);
	vector<Point2f> saida(3);
	Mat err;
	vector<uchar> STATUS(3);
	vector<Point2i> face_triang(3); // RE, LE n' N
	int re_x, le_x, eye_y, face_dim;

	int* pack;
	int status, sock;
	
	Point b;
	VideoCapture camera(0); // onboard

	/* TODO: Do we need all these variables? */
	/* TODO: variables do not start with capital letters */
	Mat frame, Prev, Prev_Gray, Current, Current_Gray; 
	CascadeClassifier rosto;
	vector<Rect> face(2);

	/*
	 * Criar um arquivo chamado "cv_path.txt" cujo conteúdo seja o caminho para
	 * o diretório da OpenCV. OBS.: NÃO COMMITAR ESTE ARQUIV TXT!!!
	 * Ex.: 
	 * cassio@ppgcc ~/github/coruja_remote/HPE $ cat ../cv_path.txt
	 * /home/cassio/Downloads/opencv/
	 *
	 */

	// http://www.cplusplus.com/reference/string/string/operator+/
	string cv_path = "../../haarcascade_frontalface_default.xml";
	ifstream haar(cv_path.c_str());
	if(haar.good()) {
		rosto.load(cv_path);
	} else {
		cout << "error: unable to load \"" << cv_path << "\"." << endl;
		exit(2);
	}

	int roll, yaw, pitch;
	int yaw_count, pitch_count, roll_count, noise_count;

	// http://blog.lemoneerlabs.com/3rdParty/Darling_BBB_30fps_DRAFT.html#x1-2000
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 320);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 240);

	pack = bt_open_conn();
	if(pack == NULL)
		return -1;

	status = pack[0];
	sock = pack[1];
	free(pack);

	string action;

	for(;;) {

		face_count = 0;

		/* TODO: What does it do? */
		printf("Trying to detect a face...\n");
		while(!is_face) {

			/* get captured frame from device */
			camera >> frame;

			/* TODO: What does it do? */
			rosto.detectMultiScale(frame, face, 2.1, 3, 0|CV_HAAR_SCALE_IMAGE, Size(100, 100));
	
			/* TODO: What does it do? */
			for(int i=0; i < face.size(); i++){
				rectangle(frame, face[i], CV_RGB(0, 255, 0), 1);
				face_dim = frame(face[i]).size().height; // a = face_dimension
			}

			/* TODO: What does it do? */
			re_x  = face[0].x + face_dim*0.3;
			le_x  = face[0].x + face_dim*0.7;
			eye_y = face[0].y + face_dim*0.38;

			/* TODO: What does it do? */
			ponto[0] = cvPoint(re_x, eye_y);//olho direito
			ponto[1] = cvPoint(le_x, eye_y);//olho esquerdo
			ponto[2] = cvPoint((re_x+le_x)/2, eye_y+int((le_x-re_x)*0.45));//nariz

			/* TODO: What does it do? */
			for(int i=0; i<3; i++)
				circle(frame, ponto[i], 2, Scalar(10,10,255), 4, 8, 0);

			/* TODO: What does it do? */
			if((int) face.size() > 0) {// quadros
				face_count++;
			} else {
				face_count = 0;
			}

			#if DEBUG
				printf(".");
			#endif
			/* TODO: What does it do? */
			if(face_count >= 10) {
				#if DEBUG
					printf("\n");
				#endif
				is_face = true;
				frame.copyTo(Prev);
				cvtColor(Prev, Prev_Gray, CV_BGR2GRAY);
			}

			/* TODO: What does it do? */
			flip(frame,frame,1);
			imshow("Camera", frame);
			esc = waitKey(30);
			if(esc == 27)
				break;
		}//close while not face

		yaw_count = 0;
		pitch_count = 0;
		roll_count = 0;
		noise_count = 0;

		/* TODO: What does it do? */
		while(is_face) {

			action = "erro";
			/* get captured frame from device */
			camera >> frame;

			/* TODO: What does it do? */
			frame.copyTo(Current);
			cvtColor(Current, Current_Gray, CV_BGR2GRAY);

			/* TODO: What does it do? */
			cv::calcOpticalFlowPyrLK(Prev_Gray, Current_Gray, ponto,
					saida, STATUS, err, Size(15,15), 1);

			/* TODO: What does it do? */
			for(int i=0; i<3; i++)
				face_triang[i] = saida[i];

			float d_e2e = sqrt(
						pow((face_triang[0].x-face_triang[1].x),2) +
						pow((face_triang[0].y-face_triang[1].y),2));

			float d_re2n = sqrt(
						pow((face_triang[0].x-face_triang[2].x),2) +
						pow((face_triang[0].y-face_triang[2].y),2));

			float d_le2n = sqrt(
						pow((face_triang[1].x-face_triang[2].x),2) +
						pow((face_triang[1].y-face_triang[2].y),2));

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
		
			/* TODO: What does it do? */
			for(int i=0; i<3; i++)
				circle(frame, face_triang[i], 2, Scalar(255,255,5), 4, 8, 0);
			
			/* TODO: What does it do? */
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
					action = "canal menos";
					cout << "yaw left\tcanal menos" << endl;
					status = bt_send(sock, status, "YL");
					is_face = false;
					break;
				} else if(yaw_count >= +20) { 
					action = "canal mais";
					cout << "yaw right\tcanal mais" << endl;
					status = bt_send(sock, status, "YR");
					is_face = false;
					break;
				}

				/* Check if it is PITCH */
				if(pitch_count <= -15) {
					action = "aumentar volume";
					cout << "pitch up\tvolume mais" << endl;
					status = bt_send(sock, status, "PU");
					is_face = false;
					break;
				} else if(pitch_count >= +15) { 
					action = "diminuir volume";
					cout << "pitch down\tvolume menos" << endl;
					status = bt_send(sock, status, "PD");
					is_face = false;
					break;
				}
			} else {

				/* Check if it is ROLL */
				if(roll_count < -120) {
					action = "ligar televisao";
					cout << "roll right\tligar televisao" << endl;
					status = bt_send(sock, status, "RR");
					is_face = false;
					break;
				} else if(roll_count > +120) { 
					action = "desligar televisao";
					cout << "roll left\tdesligar televisao" << endl;
					status = bt_send(sock, status, "RL");
					is_face = false;
					break;
				}
			}

			if(status < 0)
				printf("vish maria");

			/* TODO: What does it do? */
			for(int j=0; j<4; j++)
				ponto[j] = saida[j];

			/* TODO: What does it do? */
			Current_Gray.copyTo(Prev_Gray);

			flip(frame,frame,1);
			imshow("Camera", frame);
			esc = waitKey(30);
			if(esc == 27)
				break;
	
		}//close while isface

		for(int f=0; f<40; f++) {
			camera >> frame;
			flip(frame, frame, 1);

			putText(frame, action, Point2f(10,300),
						FONT_HERSHEY_SIMPLEX, 2, cvScalar(255,50,100), 5);
			imshow("Camera", frame);

			esc = waitKey(30);
			if(esc == 27)
				break;
		}

		if(esc == 27)
			break;

	}//close while true

	bt_close_conn(sock);

	return 0;
}//fim main

