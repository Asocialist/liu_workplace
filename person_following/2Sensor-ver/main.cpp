
//�K�v���C�u���� opencv_core249d.lib opencv_imgproc249d.lib opencv_legacy249d.lib opencv_highgui249d.lib

#include <windows.h>
#include <process.h>
#include <stdio.h>

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

#include "aurglib\aurglib.h"

#include "MyCondensation.h"
#include "MyEllipseNormalEvaluation.h"

#define M_PI 3.14159265
int URG_Distance1[1080],URG_Distance2[1080];
int URG_Intensity1[1080],URG_Intensity2[1080];

// �}�E�X�C�x���g�̎擾
bool lbPressed = false;
int lbX = 0;
int lbY = 0;

// �}�E�X�R�[���o�b�N�֐�
void mouseCallback(int event, int x, int y, int flag, void *param){
	if(event==CV_EVENT_LBUTTONDOWN){
		lbPressed = true;
		lbX = x;
		lbY = y;
	}
	return;
}

// �ǐՒ��t���O
int isTracked = 0;
int aveIntensity = 0;

int main(void)
{
	IplImage *baseImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ������₷�����邽�߂ɃZ���T�ʒu��ϑ��͈͂Ȃǂ�`�悵�Ă����x�[�X�摜
	IplImage *distanceImage     = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // �Z���T�Ŏ擾���������f�[�^��`�悷��摜
	IplImage *intensityImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ���ˋ��x�p�摜

	// �E�C���h�E�̏���
	cvNamedWindow("Distance Image", CV_WINDOW_AUTOSIZE );
	cvNamedWindow("Intensity Image", CV_WINDOW_AUTOSIZE );

	// urg1 �̊J�n
	aurglib urg1;
	//if(!urg1.start("COM7",qrk::Lidar::Serial,qrk::Lidar::Distance_intensity)){
	//	printf("Error on Urg_driver::open1\n");	exit(-1);
	//}
	// urg2 �̊J�n
	aurglib urg2;
	if(!urg2.start("COM8",qrk::Lidar::Serial,qrk::Lidar::Distance_intensity)){
		printf("Error on Urg_driver::open2\n");	exit(-1);
	}

	// �����f�[�^�\���p�ݒ�
	const int OffsetX = 320; // �摜���ł̃Z���T�ʒu��X�������I�t�Z�b�g
	const int OffsetY = 320; // �摜���ł̃Z���T�ʒu��Y�������I�t�Z�b�g
	float Scale  = 0.1f;     // �����̃X�P�[�����O�i0.1�̂Ƃ�1cm��1�s�N�Z���ɑΉ��j
	float Rotate1 = 180-45;  // �Z���T�̉�]�p�x
	float Rotate2 = -45;     // �Z���T�̉�]�p�x
	float ThresL = 3000;     // �������̂������l(3000=3m)
	float ThresM = 1000;     // �������̂������l(1000=1m)
	float ThresS = 200;      // �ߋ����̂������l(200=20cm)

	// sin cos�̃e�[�u�����쐬
	double cosVal1[1080];	double sinVal1[1080];
	for(int ii=0;ii<1080;ii++){
		cosVal1[ii] = cos((ii*0.25+Rotate1)*(M_PI/180));
		sinVal1[ii] = sin((ii*0.25+Rotate1)*(M_PI/180));
	}
	double cosVal2[1080];	double sinVal2[1080];
	for(int ii=0;ii<1080;ii++){
		cosVal2[ii] = cos((ii*0.25+Rotate2)*(M_PI/180));
		sinVal2[ii] = sin((ii*0.25+Rotate2)*(M_PI/180));
	}

	// �Z���T�ʒu�Ɗϑ��͈͂̕`��
	cvSetZero(baseImage);
	cvCircle( baseImage, cvPoint(cvRound(OffsetX), cvRound(OffsetY)), 4, CV_RGB(64,64,64), -1, 8); // �Z���T�̈ʒu
	for(int jj=0;jj<1080;jj++){
		cvCircle( baseImage, cvPoint(cvRound(ThresS*Scale*sinVal1[jj])+OffsetX, cvRound(ThresS*Scale*cosVal1[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �ߋ����̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresM*Scale*sinVal1[jj])+OffsetX, cvRound(ThresM*Scale*cosVal1[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �������̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresL*Scale*sinVal1[jj])+OffsetX, cvRound(ThresL*Scale*cosVal1[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �������̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresS*Scale*sinVal2[jj])+OffsetX, cvRound(ThresS*Scale*cosVal2[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �ߋ����̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresM*Scale*sinVal2[jj])+OffsetX, cvRound(ThresM*Scale*cosVal2[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �������̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresL*Scale*sinVal2[jj])+OffsetX, cvRound(ThresL*Scale*cosVal2[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �������̂������l�̉~��
	}
	
	/***************************************************************************************************************/

	//**************************************************************************************************
    // OpenCV�̏���
    //**************************************************************************************************
	IplImage *dispImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ���ʕ\���p�摜
	IplImage *distImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 1); // �Z���T�Ŏ擾���������f�[�^��`�悷��摜
	IplImage *distImageNot = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 1); // �Z���T�Ŏ擾���������f�[�^��`�悵���摜�𔽓]
	IplImage *transImage   = cvCreateImage(cvSize(640,640), IPL_DEPTH_32F,1); // �����摜�ɕϊ�

	// �E�C���h�E�̏���
	cvNamedWindow("Display Image", CV_WINDOW_AUTOSIZE );

	// �}�E�X�R�[���o�b�N�֐��̓o�^
	cvSetMouseCallback("Display Image", mouseCallback);

	// �����t�H���g�̐ݒ�
	CvFont myfont;
	cvInitFont( &myfont, CV_FONT_HERSHEY_COMPLEX_SMALL, 0.7, 0.7 ); // �傫������CV_FONT_HERSHEY_COMPLEX,����������CV_FONT_HERSHEY_COMPLEX_SMALL

	//**************************************************************************************************
    // �ȉ~�ǐՊ�(�p�[�e�B�N���t�B���^=Condensation)�̏���
    //**************************************************************************************************
	// �ǐՊ�\����ConDens�̍쐬
	CvConDensation *ConDens = myCreateConDensation(3, 300); // ����(��ԕϐ��x�N�g���̎���, �T���v����)
	// �T���v���̃p�����[�^��ݒ肷��, initValue - �����l, initMean - ����, initDeviation - �W���΍�
	CvMat *initValue     = cvCreateMat(3, 1, CV_32FC1);
	CvMat *initMean      = cvCreateMat(3, 1, CV_32FC1);
	CvMat *initDeviation = cvCreateMat(3, 1, CV_32FC1);
	initValue->data.fl[0]    = 0.0; initValue->data.fl[1]     = 0.0; initValue->data.fl[2]     =  0.0; // �����l(X���W�CY���W�C�p�x) �P�ʂ͉�f
	initMean->data.fl[0]     = 0.0; initMean->data.fl[1]      = 0.0; initMean->data.fl[2]      =  0.0; // ���ρ@(X���W�CY���W�C�p�x) �P�ʂ͉�f
	initDeviation->data.fl[0]= 5.0; initDeviation->data.fl[1] = 5.0; initDeviation->data.fl[2] = 20.0; // ���U�@(X���W�CY���W�C�p�x) �P�ʂ͉�f
	// �ǐՊ�\����Condens�̊e�T���v���̏�����
	myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);
	// �ǐՊ�\����Condens�̃T���v���̍X�V
	myConDensUpdateSample(ConDens);
	// �Z���T�̈ʒu���Z�b�g
	SetSensorPosition(cvPoint(OffsetX, OffsetY));
	// �ȉ~�̗֊s�]���_�e�[�u���̍쐬�i�������a�C�Z�����a�C���݊p�x�Ɓj
	StoreBodyContourPosition(24,12,10); // ���̑傫�� (�����@�c���@�p�x�̍���) �P�ʂ͉�f
	// �~�̗֊s�]���_�e�[�u���̍쐬�i���a�C���݊p�x�Ɓj�F�\���p
	StoreHeadContourPosition(12,10);    // ���̑傫�� (���@�p�x�̍���) �P�ʂ͉�f

	/***************************************************************************************************************/

	//**************************************************************************************************
	// Main Loop
	//**************************************************************************************************
	for(;;){
		// �x�[�X�摜�ŏ�����
		cvCopy(baseImage, dispImage);
		cvCopy(baseImage, distanceImage);
		cvZero(intensityImage);

		// �Z���T�Ŏ擾���������Ɣ��ˋ��x�f�[�^���擾
		urg1.getDistIntensity(URG_Distance1, URG_Intensity1);
		urg2.getDistIntensity(URG_Distance2, URG_Intensity2);
		
		// ���ˋ��x���K���p�ꎞ�ϐ�
		int x[1080],y[1080];
		double URG_Distance_I[1080], URG_Intensity_I[1080];
		double angle[1080], angle_I[1080], a_1[1080], a_2[1080], b_1[1080], b_2[1080], c[1080];
		// 1�ڂ̃��[�U�Z���T�̔��ˋ��x���K��
		for(int i=0;i<1080;i++){
			x[i] = cvRound(URG_Distance1[i]*Scale*sinVal1[i])+OffsetX;
			y[i] = cvRound(URG_Distance1[i]*Scale*cosVal1[i])+OffsetY;
			//���ˋ��x���K��(begin) ���K�����ˋ��x:URG_Intensity_I
			if(i!=0){
				a_1[i] = OffsetX - x[i];//�x�N�g��a��x����
				a_2[i] = OffsetY - y[i];//�x�N�g��a��y����
				b_1[i] = x[i-1] - x[i];//�x�N�g��b��x����
				b_2[i] = y[i-1] - y[i];//�x�N�g��b��y����
				c[i] = a_1[i] * b_1[i] + a_2[i] * b_2[i] / 
					sqrt((double)(a_1[i] * a_1[i] + a_2[i] * a_2[i])) * sqrt((double)(b_1[i] * b_1[i] + b_2[i] * b_2[i]));//2�x�N�g���̂Ȃ��p����cos(90-x)�����߂�
				angle[i] = sqrt( 1 - (c[i] * c[i]));//�\�ʂ̖@���x�N�g���ɑ΂�����ˊp sin^2 + cos^2 = 1���sin������ cosx = sin(90-x)��� cosx�����Ƃ߂�
			}
			URG_Distance_I[i] = pow((double)URG_Distance1[i],(double)0.287); //���K���̂���(����) r^0.287
			angle_I[i] = pow((double)angle[i],(double)0.196); //���K���̂���(���ˊp) cos^0.196(x)
			URG_Intensity_I[i] = URG_Intensity1[i] * URG_Distance_I[i] / angle_I[i];//���ˋ��x�̐��K�� ���K���l=�v���l*r^0.287/cos^0.196(x) (r:����,cosx:�\�ʖ@���ɑ΂�����ˊp)
			URG_Intensity1[i] = cvRound(URG_Intensity_I[i]/200000*255);	if(URG_Intensity1[i]>255) URG_Intensity1[i]=255; if(URG_Intensity1[i]<0) URG_Intensity1[i]=0;
			//���K��(end)
		}
		// 2�ڂ̃��[�U�Z���T�̔��ˋ��x���K��
		for(int i=0;i<1080;i++){
			x[i] = cvRound(URG_Distance2[i]*Scale*sinVal2[i])+OffsetX;
			y[i] = cvRound(URG_Distance2[i]*Scale*cosVal2[i])+OffsetY;
			//���ˋ��x���K��(begin) ���K�����ˋ��x:URG_Intensity_I
			if(i!=0){
				a_1[i] = OffsetX - x[i];//�x�N�g��a��x����
				a_2[i] = OffsetY - y[i];//�x�N�g��a��y����
				b_1[i] = x[i-1] - x[i];//�x�N�g��b��x����
				b_2[i] = y[i-1] - y[i];//�x�N�g��b��y����
				c[i] = a_1[i] * b_1[i] + a_2[i] * b_2[i] / 
					sqrt((double)(a_1[i] * a_1[i] + a_2[i] * a_2[i])) * sqrt((double)(b_1[i] * b_1[i] + b_2[i] * b_2[i]));//2�x�N�g���̂Ȃ��p����cos(90-x)�����߂�
				angle[i] = sqrt( 1 - (c[i] * c[i]));//�\�ʂ̖@���x�N�g���ɑ΂�����ˊp sin^2 + cos^2 = 1���sin������ cosx = sin(90-x)��� cosx�����Ƃ߂�
			}
			URG_Distance_I[i] = pow((double)URG_Distance2[i],(double)0.287); //���K���̂���(����) r^0.287
			angle_I[i] = pow((double)angle[i],(double)0.196); //���K���̂���(���ˊp) cos^0.196(x)
			URG_Intensity_I[i] = URG_Intensity2[i] * URG_Distance_I[i] / angle_I[i];//���ˋ��x�̐��K�� ���K���l=�v���l*r^0.287/cos^0.196(x) (r:����,cosx:�\�ʖ@���ɑ΂�����ˊp)
			URG_Intensity2[i] = cvRound(URG_Intensity_I[i]/200000*255);	if(URG_Intensity2[i]>255) URG_Intensity2[i]=255; if(URG_Intensity2[i]<0) URG_Intensity2[i]=0;
			//���K��(end)
		}

		//1�ڂ̃Z���T����`��
		for(int i=0;i<1080;i++){
			//printf("%f\n",URG_Intensity_I[540]);�ċA�����ˍގ���200000�O��
			if(URG_Distance1[i]>ThresS && URG_Distance1[i]<ThresL){
				cvCircle( distanceImage, cvPoint(cvRound(URG_Distance1[i]*Scale*sinVal1[i])+OffsetX-20, cvRound(URG_Distance1[i]*Scale*cosVal1[i])+OffsetY), 1, CV_RGB(0,255,255), -1, 8);
				cvCircle( intensityImage, cvPoint(cvRound(URG_Distance1[i]*Scale*sinVal1[i])+OffsetX-20, cvRound(URG_Distance1[i]*Scale*cosVal1[i])+OffsetY), 1, CV_RGB(URG_Intensity1[i],URG_Intensity1[i],URG_Intensity1[i]), -1, 8);
				int color1 = URG_Intensity1[i]*4+64;
				cvCircle( dispImage, cvPoint(cvRound(URG_Distance1[i]*Scale*sinVal1[i])+OffsetX-20, cvRound(URG_Distance1[i]*Scale*cosVal1[i])+OffsetY), 1, CV_RGB(64,color1,color1), -1, 8);

			}
		}
		//2�ڂ̃Z���T����`��
		for(int i=0;i<1080;i++){
			//printf("%f\n",URG_Intensity_I[540]);�ċA�����ˍގ���200000�O��
			if(URG_Distance2[i]>ThresS && URG_Distance2[i]<ThresL){
				cvCircle( distanceImage, cvPoint(cvRound(URG_Distance2[i]*Scale*sinVal2[i])+OffsetX+19, cvRound(URG_Distance2[i]*Scale*cosVal2[i])+OffsetY-1), 1, CV_RGB(255,255,0), -1, 8);
				cvCircle( intensityImage, cvPoint(cvRound(URG_Distance2[i]*Scale*sinVal2[i])+OffsetX+19, cvRound(URG_Distance2[i]*Scale*cosVal2[i])+OffsetY-1), 1, CV_RGB(URG_Intensity2[i],URG_Intensity2[i],URG_Intensity2[i]), -1, 8);
				int color2 = URG_Intensity2[i]*4+64;
				cvCircle( dispImage, cvPoint(cvRound(URG_Distance2[i]*Scale*sinVal2[i])+OffsetX+19, cvRound(URG_Distance2[i]*Scale*cosVal2[i])+OffsetY-1), 1, CV_RGB(color2,color2,64), -1, 8);
			}
		}
		
		// ���ʂ̕\��
		cvShowImage("Distance Image", distanceImage);
		cvShowImage("Intensity Image", intensityImage);

		/***************************************************************************************************************/

		cvCvtColor(distanceImage, distImage, CV_BGR2GRAY);
		cvThreshold(distImage,distImageNot,64.0,255.0,CV_THRESH_BINARY_INV); // ���ˋ��x���������̂���2�l�����Ĕ��]�`��(臒l153���炢�H)
		cvDistTransform(distImageNot, transImage); // �����摜�ϊ�
		//cvNormalize(transImage, distImage, 0.0, 255.0, CV_MINMAX, NULL); // �\���m�F�p

		// �T���v���̍X�V
		myConDensUpdateSample(ConDens);

		// �e�T���v���̖ޓx���v�Z
		for(int i=0;i<300;++i){
			ConDens->flConfidence[i] = CalculateBodyLikelihood(transImage, cvPoint(cvRound(ConDens->flSamples[i][0]),cvRound(ConDens->flSamples[i][1])),cvRound(ConDens->flSamples[i][2]));
		}

		// ��قǌv�Z�����ޓx�Ɋ�Â��Đ��肵����Ԋ��Ғl���v�Z
		myConDensUpdateByTime(ConDens);

		// �ǐՌ���(�l���ʒu)�𑼂Ŏg�����߂Ɏ��o��
		int usrU    = cvRound(ConDens->State[0]); // �l���ʒu��x���W
		int usrV    = cvRound(ConDens->State[1]); // �l���ʒu��y���W
		int usrAngl = cvRound(ConDens->State[2]); // �l�����̂̊p�x

		//�ǐՊ�̎���̕��ϔ��ˋ��x���v�Z
		//int usrU2 = usrU-30; int usrV2 = usrV-30;
		//if(usrU2<0) usrU2=0; if(usrU2>639-60) usrU2=639-60; if(usrV2<0) usrV2=0; if(usrV2>639-60) usrV2=639-60;
		cvCvtColor(intensityImage, distImage, CV_BGR2GRAY);
		cvSetImageROI(distImage, cvRect(usrU-30,usrV-30,60,60));
		aveIntensity = (int)(cvSum(distImage).val[0] / cvCountNonZero(distImage));
		cvResetImageROI(distImage);
		
		//�ǐՊ�̎���̕��ϔ��ˋ��x���Ⴂ�Ƃ��ɂ͒ǐՂ���߂�
		if(isTracked==1 && aveIntensity<48){
			initValue->data.fl[0] = NULL; initValue->data.fl[1] = NULL; // �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
			myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);	// �ǐՊ�\����Condens�̊e�T���v���̏�����
			myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
			myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���			
			isTracked = 0;
		}

		//�ǐՂ��Ă��Ȃ��Ƃ��ɁC�߂��ō����ˋ��x�����o���ꂽ�ꍇ�C�����l�ύX���čĒǐ�
		if(isTracked==0){ //�Ώۑ�����
			for(int i=0;i<1080;i++){
				if(URG_Distance1[i]<1200 && URG_Intensity1[i]>120){//1m�ȓ��Ő��K�����ˋ��x��120�ȏ�Ȃ�ǐՏ�����
					initValue->data.fl[0] = cvRound((URG_Distance1[i]*Scale*sinVal1[i])+OffsetX-20);// �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
					initValue->data.fl[1] = cvRound((URG_Distance1[i]*Scale*cosVal1[i])+OffsetY);
					myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);   // �ǐՊ�\����Condens�̊e�T���v���̏�����
					myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
					myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
					isTracked = 1; break;
				}
			}
		}
		if(isTracked==0){ //�Ώۑ�����
			for(int i=0;i<1080;i++){
				if(URG_Distance2[i]<1200 && URG_Intensity2[i]>120){//1m�ȓ��Ő��K�����ˋ��x��120�ȏ�Ȃ�ǐՏ�����
					initValue->data.fl[0] = cvRound((URG_Distance2[i]*Scale*sinVal2[i])+OffsetX+19);// �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
					initValue->data.fl[1] = cvRound((URG_Distance2[i]*Scale*cosVal2[i])+OffsetY-1);
					myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);   // �ǐՊ�\����Condens�̊e�T���v���̏�����
					myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
					myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
					isTracked = 1; break;
				}
			}
		}

		//�}�E�X�ŃN���b�N���ꂽ�炻�̈ʒu�������ʒu�Ƃ��ĒǐՂ��J�n
		if(lbPressed){
			lbPressed = false; // �}�E�X�N���b�N�̃t���O��߂�
			initValue->data.fl[0] = (float)lbX; initValue->data.fl[1] = (float)lbY; // �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
			myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);	// �ǐՊ�\����Condens�̊e�T���v���̏�����
			myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
			myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
			usrU = lbX;	usrV = lbY;
			isTracked = 1;
		}

		// �ǐՌ��ʂ̕`��
		if(	isTracked == 1){
			DrawBodyContour(dispImage,cvPoint(usrU,usrV),usrAngl);
			cvRectangle(dispImage,cvPoint(usrU-30,usrV-30),cvPoint(usrU+30,usrV+30),CV_RGB(255,255,255),1,8,0);
			if(aveIntensity<0) aveIntensity=0;
			char mytext[12]; sprintf(mytext,"%4d",aveIntensity);
			cvPutText(dispImage, mytext, cvPoint(usrU+30,usrV+30), &myfont, CV_RGB(255,255,255));
		}

		/***************************************************************************************************************/

		// ���ʂ̕\��
		cvShowImage("Display Image", dispImage);
		
		if(cvWaitKey(1) == 'q') {
			break;
		}
	}

	// �摜���������J������
	cvReleaseImage(&baseImage);
	cvReleaseImage(&distanceImage);
	cvReleaseImage(&intensityImage);
	cvReleaseImage(&dispImage);
	cvReleaseImage(&distImage);
	cvReleaseImage(&distImageNot);
	cvReleaseImage(&transImage);

	// �E�C���h�E�����
	cvDestroyAllWindows();

	// urg�̏I��
	urg1.end();
	urg2.end();
	
    return 1;
}
