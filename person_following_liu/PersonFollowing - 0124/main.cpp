#include "FollowTrackingServe.h"
#include <iostream>
#include <memory>


//�K�v���C�u���� opencv_core249d.lib opencv_imgproc249d.lib opencv_legacy249d.lib opencv_highgui249d.lib

#include <opencv2\video\tracking.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc_c.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "MyUrgCapture.h"
#include "MyEllipseNormalEvaluation.h"
#include "MyCondensation.h"
#include "jwvehicle_wrapper.h"
#include "aurglib\aurglib.h"
#include <Windows.h>
#include <process.h>
#include <opencv2\opencv.hpp>
#include <opencv2\core\core.hpp>



#define M_PI 3.14159265
int URG_Distance[1080];
int URG_Intensity[1080];
double URG_Distance_I[1080],URG_Intensity_I[1080],angle[1080],angle_I[1080],a_1[1080],a_2[1080],b_1[1080],b_2[1080],c[1080];//���ˋ��x���K���p

//�Ԉ֎q��COM�|�[�g�ԍ�(��)
char ComJW[16] = "\\\\.\\COM4";
char ComURG[16] = "COM3";

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

int main(int argc, char **argv)
{
	// For Follow Tracking
	std::unique_ptr<FollowTrackingServer> ftServer;
	//FollowTrackingServer* ftServer = nullptr;
	if (argc >= 2) {
		ftServer = std::make_unique<FollowTrackingServer>(argv[1]);
		//ftServer = new FollowTrackingServer(argv[1]);
	}
	else {
		std::cerr << "invalid argument." << std::endl;
		exit(1);
	}



	IplImage *dispImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ���ʕ\���p�摜
	IplImage *baseImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ������₷�����邽�߂ɃZ���T�ʒu��ϑ��͈͂Ȃǂ�`�悵�Ă����x�[�X�摜
	IplImage *distImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 1); // �Z���T�Ŏ擾���������f�[�^��`�悷��摜
	IplImage *distImageNot = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 1); // �Z���T�Ŏ擾���������f�[�^��`�悵���摜�𔽓]
	IplImage *transImage   = cvCreateImage(cvSize(640,640), IPL_DEPTH_32F,1); // �����摜�ɕϊ�
	IplImage *intensityImage    = cvCreateImage(cvSize(640,640), IPL_DEPTH_8U, 3); // ���ˋ��x�p�摜

	// �E�C���h�E�̏���
	cvNamedWindow("Display Image", CV_WINDOW_AUTOSIZE );
	cvNamedWindow("Intensity Image", CV_WINDOW_AUTOSIZE );

	// �}�E�X�R�[���o�b�N�֐��̓o�^
	cvSetMouseCallback("Display Image",mouseCallback);
	// �����t�H���g�̐ݒ�
	CvFont myfont;
	cvInitFont( &myfont, CV_FONT_HERSHEY_COMPLEX_SMALL, 1, 1 ); // �傫������CV_FONT_HERSHEY_COMPLEX,����������CV_FONT_HERSHEY_COMPLEX_SMALL
	// URG �̊J�n
	
	aurglib urg;
	strcpy(ComURG, ftServer->GetURGPort().c_str());
	if(!urg.start(ComURG, qrk::Lidar::Serial,qrk::Lidar::Distance_intensity)){
		fprintf(stderr,"Error on Urg_driver::open\n");
		exit(-1);
	}

	// �����f�[�^�\���p�ݒ�
	const int OffsetX = 320; // �摜���ł̃Z���T�ʒu��X�������I�t�Z�b�g
	const int OffsetY = 320; // �摜���ł̃Z���T�ʒu��Y�������I�t�Z�b�g
	float Scale  = 0.1f;     // �����̃X�P�[�����O�i0.1�̂Ƃ�1cm��1�s�N�Z���ɑΉ��j
	float Rotate = 45;       // �Z���T�̉�]�p�x
	float ThresL = 6000;     // �������̂������l(3000=3m)
	float ThresM = 500;      // �������̂������l(500=50cm)
	float ThresS = 200;      // �ߋ����̂������l(200=20cm)
	int INTENS_S = 120000; //���K�����ˋ��x�̉���
	int INTENS_L = 200000; //���K�����ˋ��x�̏��
	int usrU=0,usrV=0,usrAngl=0;// �l���ʒu��x���W,y���W,�l�����̂̊p�x
	// sin cos�̃e�[�u�����쐬
	double cosVal[1080];	double sinVal[1080];
	for(int ii=0;ii<1080;ii++){
		cosVal[ii] = cos((ii*0.25+Rotate)*(M_PI/180));
		sinVal[ii] = sin((ii*0.25+Rotate)*(M_PI/180));
	}

	// �Z���T�ʒu�Ɗϑ��͈͂̕`��
	cvSetZero(baseImage);
	cvCircle( baseImage, cvPoint(cvRound(OffsetX), cvRound(OffsetY)), 4, CV_RGB(64,64,64), -1, 8); // �Z���T�̈ʒu
	for(int jj=0;jj<1080;jj++){
		cvCircle( baseImage, cvPoint(cvRound(ThresS*Scale*sinVal[jj])+OffsetX, cvRound(ThresS*Scale*cosVal[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �ߋ����̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresM*Scale*sinVal[jj])+OffsetX, cvRound(ThresM*Scale*cosVal[jj])+OffsetY), 1, CV_RGB(32,32,32), -1, 8); // �������̂������l�̉~��
		cvCircle( baseImage, cvPoint(cvRound(ThresL*Scale*sinVal[jj])+OffsetX, cvRound(ThresL*Scale*cosVal[jj])+OffsetY), 1, CV_RGB(64,64,64), -1, 8); // �������̂������l�̉~��
	}

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
	//�Ԉ֎q�̏���
	//JWVehicle vehicle(ComJW);
	//Sleep(100);
	//vehicle.stop();
	//Sleep(10);

	int x[1080],y[1080];
	int k=0;//���,�}�E�X�N���b�N
	int count_1=0; //�L���T���v����
	int sum[3];//�L���T���v�����U�Z�o�̂��߂̗݌v�i�[�ϐ�
	double dis[3];//�L���T���v���̕��U(x,y,�p�x)

	while(true){
	//for(int r=0;r<100000;r++){//r:�T�C�N����
		// �x�[�X�摜�ŏ�����
		cvCopy(baseImage, dispImage);
		cvZero(distImage);
		cvCopy(baseImage, intensityImage);

		//�T�C�N�����ƂɗL���T���v���̕��U�����߂邽�߂̏�����
		for(int lp=0;lp<3;lp++){//(0:x,1:y,2:�p�x)
		sum[lp]=0;//�݌v�l������
		dis[lp]=0.0;//���U�l������
		}
		// �Z���T�Ŏ擾���������f�[�^��`��
		urg.getDistIntensity(URG_Distance,URG_Intensity); // �����f�[�^�̎擾
		for(int i=0;i<1080;i++){
			x[i] = cvRound(URG_Distance[i]*Scale*sinVal[i])+OffsetX;
			y[i] = cvRound(URG_Distance[i]*Scale*cosVal[i])+OffsetY;

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
			URG_Distance_I[i] = pow((double)URG_Distance[i],(double)0.287); //���K���̂���(����) r^0.287
			angle_I[i] = pow((double)angle[i],(double)0.196); //���K���̂���(���ˊp) cos^0.196(x)
			URG_Intensity_I[i] = URG_Intensity[i] * URG_Distance_I[i] / angle_I[i];//���ˋ��x�̐��K�� ���K���l=�v���l*r^0.287/cos^0.196(x) (r:����,cosx:�\�ʖ@���ɑ΂�����ˊp)
			//���K��(end)

			//�Z���T����̋�������`��
			//printf("%f\n",URG_Intensity_I[540]);�ċK�����ˍގ���200000�O��
			if(URG_Distance[i]>ThresS && URG_Distance[i]<ThresL){
				cvCircle( dispImage, cvPoint(cvRound(URG_Distance[i]*Scale*sinVal[i])+OffsetX, cvRound(URG_Distance[i]*Scale*cosVal[i])+OffsetY), 1, CV_RGB(255,255,255), -1, 8);
				cvCircle( distImage, cvPoint(cvRound(URG_Distance[i]*Scale*sinVal[i])+OffsetX, cvRound(URG_Distance[i]*Scale*cosVal[i])+OffsetY), 1, CV_RGB(255,255,255), -1, 8);
				if(URG_Intensity_I[i]>INTENS_S && URG_Intensity_I[i]<INTENS_L){
					cvCircle( intensityImage,cvPoint(x[i],y[i]), 1, CV_RGB(255,255,255), -1, 8);
				}
			}
		}
		cvNot(distImage,distImageNot); // ���]
		cvDistTransform(distImageNot, transImage); // �����摜�ϊ�
		// �T���v���̍X�V
		myConDensUpdateSample(ConDens);
		// �e�T���v���̖ޓx���v�Z
		for(int i=0;i<300;++i){
			ConDens->flConfidence[i] = CalculateBodyLikelihood(transImage, cvPoint(cvRound(ConDens->flSamples[i][0]),cvRound(ConDens->flSamples[i][1])),cvRound(ConDens->flSamples[i][2]));
		}

		//�L���T���v���Z�o
		count_1=0; //count_1:�X�V��(�󂯌p�����T���v���̐�)
		int j_before = -1; //�O��̍X�V��j�̒l���i�[
		if(k==1){ //k=1:�}�E�X�N���b�N��
			 for(int o=0;o<ConDens->SamplesNum;o++){
				int j = 1;
				while((ConDens->flCumulative[j]<=(float)o*(float)ConDens->MP/300.0+0.000001f)&&(j<(ConDens->SamplesNum-1))){
					j++;
				}
				if(j_before != j){ //�X�V��
					count_1++;
					for(int lp=0;lp<3;lp++){
						sum[lp] +=  ConDens->flSamples[o][lp];//�L���T���v���̒l��݌v(���U�����߂邽��)0:x,1:y,2:�p�x
					}
					j_before = j;
				}
			}
		}

		//���U�l�v�Z
		for(int lp=0;lp<3;lp++){//�L���T���v���̕��U���v�Z(lp-�����@0:x,1:y,2:�p�x)
			dis[lp] =  Dispersion_EffectivenessSample(ConDens,sum[lp],count_1,lp);//Dispersion_EffectivenessSample(ConDens:�T���v�����,sum:�L���T���v���l�̗݌v�l,count_1:�L���T���v����,lp:����)
		}

		// ��قǌv�Z�����ޓx�Ɋ�Â��Đ��肵����Ԋ��Ғl���v�Z
		myConDensUpdateByTime(ConDens);
		 // �ǐՌ���(�l���ʒu)�𑼂Ŏg�����߂Ɏ��o��
		usrU    = cvRound(ConDens->State[0]); // �l���ʒu��x���W
		usrV    = cvRound(ConDens->State[1]); // �l���ʒu��y���W
		usrAngl = cvRound(ConDens->State[2]); // �l�����̂̊p�x

		// �ǐՌ��ʂ̕`��
		DrawBodyContour(dispImage,cvPoint(usrU,usrV),usrAngl);


		//�}�E�X�ŃN���b�N���ꂽ�炻�̈ʒu�������ʒu�Ƃ��ĒǐՂ��J�n
		if(lbPressed){
			lbPressed = false; // �}�E�X�N���b�N�̃t���O��߂�
			initValue->data.fl[0] = (float)lbX; initValue->data.fl[1] = (float)lbY; // �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
			myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);	// �ǐՊ�\����Condens�̊e�T���v���̏�����
			myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
			myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
			k=1;//�}�E�X�N���b�N��
		}


		//�o�͒l�̍쐬
		//vehicle.move(speedY,speedX)
		//speedX:���E�����̐��񑬓x(-100~100)
		//speedY:�O������̒��i���x(-100~100)
		int orgPosX = 320;//�j���[�g�����ʒuX//320
		int orgPosY = 240;//�j���[�g�����ʒuY//240
		int diffX = orgPosX - usrU;//�Ԉ֎q����݂����ΓI�ȑΏێ҂�x�ʒu
		int diffY = orgPosY - usrV;//�Ԉ֎q����݂����ΓI�ȑΏێ҂�y�ʒu
		//���߂̏o��
		//if(-120<diffX && diffX<120 && -40<diffY && diffY<120){
		//	vehicle.move((diffY*2),(diffX*2));//*2
		//	Sleep(1);
		//}else{
		//	vehicle.stop();
		//	Sleep(1);
		//}

		//�Ώۑ����� �����ˋ��x�����o���ꂽ�ꍇ�A�����l�ύX���čĒǐ�
		int DiffX,DiffY;//�Z���T�ʒu����̑��΋���
		for(int i=0;i<1080;i++){
		DiffX = OffsetX - x[i];
		DiffY = OffsetY - y[i];
			if(k!=1){ //�Ώۑ�����:k=0
				if(-120<DiffX && DiffX<120 && -40<DiffY && DiffY<120){//�˒��g����
					if(URG_Intensity_I[i]>INTENS_S && URG_Intensity_I[i]<INTENS_L){//���K�����ˋ��x��臒l�ȏ�Ȃ珉���l�ύX
						initValue->data.fl[0] = (float)x[i]; initValue->data.fl[1] = (float)y[i]; // �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
						myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);	// �ǐՊ�\����Condens�̊e�T���v���̏�����
						myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
						myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
						k=1;
					}
				}
			}
		}

		//�Ԉ֎q����݂����ΓI�ȑΏێ҂�x�ʒu,y�ʒu,��Ԃ�`��
		char posText[12];
		sprintf(posText, "%3d,%3d,%3d",diffX,diffY,k);
		cvPutText(dispImage,posText,cvPoint(20,20),&myfont,CV_RGB(255,255,255));

		// For Follow Tracking
		if (usrU != 0 && usrV != 0 && k != 0)
			ftServer->SendTracking(diffX, diffY, k);
		
		// ����͈͂̕`��
		cvRectangle(dispImage, cvPoint(orgPosX-120, orgPosY-120), cvPoint(orgPosX+120, orgPosY+40), CV_RGB(255, 255, 255) );


		//�L���T���v���̕��U����Ń}�[�L���O���O��
		if((dis[0] > 2500.0) || (dis[1] >2500.0)){//�L���T���v�����U��臒l�𒴂���Ȃ�
			initValue->data.fl[0] = NULL; initValue->data.fl[1] = NULL; // �T���v���̏������p�����[�^�̂��������l���}�E�X�N���b�N���W�ɐݒ肷��
			myConDensInitSampleSet(ConDens, initValue, initMean, initDeviation);	// �ǐՊ�\����Condens�̊e�T���v���̏�����
			myConDensUpdateSample(ConDens); // �ǐՊ�\����Condens�̃T���v���̍X�V
			myConDensUpdateByTime(ConDens); // ��Ԋ��Ғl���v�Z���Ă���
			k=0;
			printf("�O������\n");
			
		}
		// ���ʂ̕\��
		cvShowImage("Display Image", dispImage);
		cvShowImage("Intensity Image", intensityImage);
		if(cvWaitKey(1) == 'q') {
			break; // exit main roop
			//vehicle.stop();
			Sleep(1);
			// URG �̏I��
			urg.end();
		}
	}

	// �ǐՊ�\����ConDens�̃��������J������
	myReleaseConDensation(&ConDens);
	// �s��C�x�N�g���̃��������J������
	cvReleaseMat(&initValue);
	cvReleaseMat(&initMean);
	cvReleaseMat(&initDeviation);
	// �摜���������J������
	cvReleaseImage(&dispImage);
	cvReleaseImage(&baseImage);
	cvReleaseImage(&distImage);
	cvReleaseImage(&distImageNot);
	cvReleaseImage(&transImage);
	cvReleaseImage(&intensityImage);
	// �E�C���h�E�����
	cvDestroyWindow("Display Image");
	cvDestroyWindow("Intensity Image");

	//vehicle.stop();
	//Sleep(1);
	// URG �̏I��
	urg.end();
    return 1;
}
