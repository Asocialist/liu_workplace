/*
*  EllipseNormalEvaluation.cpp 
*  �K�v���C�u���� opencv_core249d.lib opencv_imgproc249d.lib opencv_legacy249d.lib opencv_highgui249d.lib
*/
#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

#include "MyEllipseNormalEvaluation.h"

#define IMAGEWIDTH    640	// ��ʃT�C�Y���FVIDEOMODE�Ƃ��킹��
#define IMAGEHIGHT    640	// ��ʃT�C�Y���FVIDEOMODE�Ƃ��킹��

#define EPS 1.0e-7			//�@�B�덷

// �v���g�^�C�v�錾
int CalculateBodyContourPosition(CvPoint center, int rotation, int bodyEnableFlg[], CvPoint *bodyPosData);
int CalculateHeadContourPosition(CvPoint center, CvPoint size, int headEnableFlg[], CvPoint *headPosData);

static const int    OutOfImageMargin = 10;                         // ��ʒ[�̃}�[�W��
static const double CONTOURPI = 3.1415926535897932384626433832795; // �~����

// �� ���[�U�Z���T�摜�ɂ�铷�̕]���̂��߂̐錾
static CvPoint m_sensorPos;       // �Z���T�̈ʒu
// ���̃��f��
static int m_NofBodyEdgePos = 0;  // ���̗֊s��̓_���̕ۑ��p
static int m_bodyMajorAxis  = 0;  // ����(�ȉ~)�̕�
static int m_bodyMinorAxis  = 0;  // ����(�ȉ~)�̌���
// ��{�ȉ~�̗֊s�ʒu�Ɩ@���x�N�g���̃e�[�u��
CvPoint m_bodyContourPositionTable[36]; // 36�p��(10�x���݂܂őΉ�)
CvPoint m_bodyNormalVectorTable[36];    // 36�p��(10�x���݂܂őΉ�)
// �������f��
static int m_NofHeadEdgePos = 0;  // �����֊s��̓_���̕ۑ��p
static int m_headAxis       = 0;  // ����(�~)�̔��a
// ��{�~�̗֊s�ʒu�Ɩ@���x�N�g���̃e�[�u��
CvPoint m_headContourPositionTable[36]; // 36�p��(10�x���݂܂őΉ�)
CvPoint m_headNormalVectorTable[36];    // 36�p��(10�x���݂܂őΉ�)


//************************************************************
// �� ���[�U�Z���T�摜�ɂ�铷�̕]���̂��߂̊֐�
//************************************************************
// ���[�U�Z���T�̈ʒu�̃Z�b�g
void SetSensorPosition(CvPoint sensorPos)
{
	m_sensorPos.x = sensorPos.x;
	m_sensorPos.y = sensorPos.y;
	return;
}

// ���̗̂֊s�ʒu�e�[�u���쐬�֐��i�����C�Z���C���݊p�x�Ɓj
void StoreBodyContourPosition(int majorAxis, int minorAxis, int stepTheta)
{
	//IplImage *image  = cvCreateImage(cvSize(IMAGEWIDTH,IMAGEHIGHT), IPL_DEPTH_8U, 1);
	//cvSetZero(image);

	m_bodyMajorAxis = majorAxis;
	m_bodyMinorAxis = minorAxis;

	// ���̑ȉ~�e�[�u���쐬
	for(int i=0; i<360/stepTheta; i++){
		// ���̗֊s�ʒu
		m_bodyContourPositionTable[i].x = cvRound(majorAxis * cos(i*stepTheta*CONTOURPI/180 + CONTOURPI));
		m_bodyContourPositionTable[i].y = cvRound(minorAxis * sin(i*stepTheta*CONTOURPI/180 + CONTOURPI));
		//printf("position :%d,%d --> ",m_bodyContourPositionTable[i].x,m_bodyContourPositionTable[i].y);
		// ���̖@���x�N�g��
		m_bodyNormalVectorTable[i].x = (minorAxis*minorAxis)*m_bodyContourPositionTable[i].x;
		m_bodyNormalVectorTable[i].y = (majorAxis*majorAxis)*m_bodyContourPositionTable[i].y;
		//printf("normalVec:%d,%d\n",m_bodyNormalVectorTable[i].x,m_bodyNormalVectorTable[i].y);
		//cvCircle( image, cvPoint(m_bodyContourPositionTable[i].x+IMAGEWIDTH/2, m_bodyContourPositionTable[i].y+IMAGEHIGHT/2), 2, CV_RGB(255,120,120), 1, 8);
		m_NofBodyEdgePos++;
	}

	//printf("%d\n",m_NofBodyEdgePos);
	//cvShowImage("Sample1", image); cvWaitKey(1000);
	//cvReleaseImage(&image);

	return;
}

// �����̗֊s�ʒu�e�[�u���쐬�֐��i���a�C���݊p�x�Ɓj
void StoreHeadContourPosition(int headAxis, int stepTheta)
{
	//IplImage *image  = cvCreateImage(cvSize(IMAGEWIDTH,IMAGEHIGHT), IPL_DEPTH_8U, 1);
	//cvSetZero(image);

	m_headAxis = headAxis;

	// �����~�`�e�[�u���쐬
	for(int i=0; i<360/stepTheta; i++){
		// �����֊s�ʒu
		m_headContourPositionTable[i].x = cvRound(headAxis * cos(i*stepTheta*CONTOURPI/180 + CONTOURPI));
		m_headContourPositionTable[i].y = cvRound(headAxis * sin(i*stepTheta*CONTOURPI/180 + CONTOURPI));
		//printf("position :%d,%d --> ",m_headContourPositionTable[i].x,m_headContourPositionTable[i].y);
		// �����@���x�N�g��
		m_headNormalVectorTable[i].x = m_headContourPositionTable[i].x;
		m_headNormalVectorTable[i].y = m_headContourPositionTable[i].y;
		//printf("normalVec:%d,%d\n",m_headNormalVectorTable[i].x,m_headNormalVectorTable[i].y);
		//cvCircle( image, cvPoint(m_headNormalVectorTable[i].x+IMAGEWIDTH/2, m_headNormalVectorTable[i].y+IMAGEHIGHT/2), 2, CV_RGB(255,120,120), 1, 8);
		m_NofHeadEdgePos++;
	}

	//printf("%d\n",m_NofHeadEdgePos);
	//cvShowImage("Sample1", image); cvWaitKey(1000);
	//cvReleaseImage(&image);

	return;
}

// ���̗̂֊s�ʒu�v�Z
int CalculateBodyContourPosition(CvPoint center, int rotation, int bodyEnableFlg[], CvPoint *bodyPosData)
{
	CvPoint bodyContourPos;
	CvPoint bodyContourVec;
	int NofBodyEnablePos = 0;
	// �֊s�ʒu���v�Z���C�Z���T�ʒu�Ƃ̓��ς���enableFlg�𗧂Ă�
	for(int i=0; i<m_NofBodyEdgePos; i++){

		// ���̂̉�]�ƕ��s�ړ�
		bodyContourPos.x = cvRound(cos(rotation*CONTOURPI/180)*(double)m_bodyContourPositionTable[i].x - sin(rotation*CONTOURPI/180)*(double)m_bodyContourPositionTable[i].y) + center.x;
		bodyContourPos.y = cvRound(sin(rotation*CONTOURPI/180)*(double)m_bodyContourPositionTable[i].x + cos(rotation*CONTOURPI/180)*(double)m_bodyContourPositionTable[i].y) + center.y;
		bodyContourVec.x = cvRound(cos(rotation*CONTOURPI/180)*(double)m_bodyNormalVectorTable[i].x - sin(rotation*CONTOURPI/180)*(double)m_bodyNormalVectorTable[i].y);
		bodyContourVec.y = cvRound(sin(rotation*CONTOURPI/180)*(double)m_bodyNormalVectorTable[i].x + cos(rotation*CONTOURPI/180)*(double)m_bodyNormalVectorTable[i].y);

		// �@���x�N�g���Ƃ̓��ς��`�F�b�N
		int bodyInnerProduct = bodyContourVec.x * (m_sensorPos.x - bodyContourPos.x) + bodyContourVec.y * (m_sensorPos.y - bodyContourPos.y);
		double bodyMag = 
			cvSqrt((double)(m_sensorPos.x-bodyContourPos.x)*(m_sensorPos.x-bodyContourPos.x)+(m_sensorPos.y-bodyContourPos.y)*(m_sensorPos.y-bodyContourPos.y))
			*
			cvSqrt((double)(bodyContourVec.x*bodyContourVec.x)+(bodyContourVec.y*bodyContourVec.y));
		//printf("%4.2f, ",bodyInnerProduct/bodyMag);
		if(bodyInnerProduct/bodyMag > 0.2){
			bodyPosData[i].x = bodyContourPos.x;
			bodyPosData[i].y = bodyContourPos.y;
			bodyEnableFlg[i] = 1;
			NofBodyEnablePos++;
		}else{
			bodyPosData[i].x = bodyContourPos.x;
			bodyPosData[i].y = bodyContourPos.y;
			bodyEnableFlg[i] = 0;
		}
	}

	return NofBodyEnablePos;
}

// �����̗֊s�ʒu�v�Z
int CalculateHeadContourPosition(CvPoint center, CvPoint size, int headEnableFlg[], CvPoint *headPosData)
{
	CvPoint headContourPos;
	CvPoint headContourVec;
	int NofHeadEnablePos = 0;
	
	// �֊s�ʒu���v�Z���C�Z���T�ʒu�Ƃ̓��ς���enableFlg�𗧂Ă�
	for(int i=0; i<m_NofBodyEdgePos; i++){
		// �T�C�Y�ύX�ƕ��s�ړ�
		headContourPos.x = cvRound( (double)size.x/(double)m_headAxis * (double)m_headContourPositionTable[i].x) + center.x;
		headContourPos.y = cvRound( (double)size.y/(double)m_headAxis * (double)m_headContourPositionTable[i].y) + center.y;
		headContourVec.x = cvRound( (double)size.x/(double)m_headAxis * (double)m_headNormalVectorTable[i].x);
		headContourVec.y = cvRound( (double)size.y/(double)m_headAxis * (double)m_headNormalVectorTable[i].y);

		// �@���x�N�g���Ƃ̓��ς��`�F�b�N
		int headInnerProduct = headContourVec.x * (m_sensorPos.x - headContourPos.x) + headContourVec.y * (m_sensorPos.y - headContourPos.y);
		double Mag = cvSqrt((double)(m_sensorPos.x-headContourPos.x)*(m_sensorPos.x-headContourPos.x)+(m_sensorPos.y-headContourPos.y)*(m_sensorPos.y-headContourPos.y))*cvSqrt((double)(headContourVec.x*headContourVec.x)+(headContourVec.y*headContourVec.y));
		//printf("%4.2f, ",innerProduct/Mag);
		if(headInnerProduct/Mag > -0.2){
			headPosData[i].x = headContourPos.x;
			headPosData[i].y = headContourPos.y;
			headEnableFlg[i] = 1;
			NofHeadEnablePos++;
		}else{
			headPosData[i].x = headContourPos.x;
			headPosData[i].y = headContourPos.y;
			headEnableFlg[i] = 0;
		}

	}

	return NofHeadEnablePos;
}

// ���̗֊s�̕]���֐�(�֊s�����摜�ɂ��]��)
float CalculateBodyLikelihood(IplImage *urgDstImg, CvPoint center, int rotation)
{
	CvPoint edgePositionForCompare; //�ۂ߂��ē����ʒu�ɂ����ꍇ�͕]�����珜������
	bool    flg  = true;			//�����ʒu����Ȃ�������true
	double  temp = 0;				//�ޓx�i�[�p
	int		EvalCnt = 0;

	// �ȉ~�̗֊s�ʒu�v�Z
	CvPoint bodyPositionData[36];
	int     bodyEnableFlg[36];
	CalculateBodyContourPosition(center, rotation, bodyEnableFlg, bodyPositionData);

	temp = 0;
	for(int i=0; i<m_NofBodyEdgePos; i++){
		//******* �����ʒu�����x���]�����邱�Ƃ�h��
		if( i == 0 ){ // �ŏ��̓_�͖������ŕ]�����邽�߂Ƀt���O�𗧂Ă�
			edgePositionForCompare.x = bodyPositionData[i].x;
			edgePositionForCompare.y = bodyPositionData[i].y;
			flg = true;
		}else{ // 2�_�ڈȍ~�������s�N�Z�����H�𒲂ׂăt���O�𗧂Ă�
			if(  ( edgePositionForCompare.x == bodyPositionData[i].x ) 
			   &&( edgePositionForCompare.y == bodyPositionData[i].y ) ){
				flg = false;
			}
			else
				flg = true;
		}

		//***** ��ʓ��œ����ʒu�����Ă��Ȃ��Ȃ�΁A�]������
		if ( flg && (bodyPositionData[i].x>OutOfImageMargin) && (bodyPositionData[i].x<IMAGEWIDTH-OutOfImageMargin) 
			     && (bodyPositionData[i].y>OutOfImageMargin) && (bodyPositionData[i].y<IMAGEHIGHT-OutOfImageMargin) ){
			if(bodyEnableFlg[i]==1){
				CvScalar PixVal = cvGet2D(urgDstImg, bodyPositionData[i].y, bodyPositionData[i].x);
				double tmp = PixVal.val[0];
				if(tmp > temp) temp = tmp;
				EvalCnt++;
			}else{
			}
		}
		//***** �����ʒu�����Ă���Ƃ��͕]�����Ȃ�
		else{
		}

		//**** ��r�̂��߂ɍ��̗֊s�̈ʒu���i�[
		edgePositionForCompare.x = bodyPositionData[i].x;
		edgePositionForCompare.y = bodyPositionData[i].y;
	}

	//static FILE *fp=fopen("tmp.txt","w");
	//fprintf(fp,"%lf\t%d\n",temp,EvalCnt);
	double ret = exp(-(double)(temp)*(temp)/(5))*exp((double)EvalCnt);
	return temp>EPS?(float)ret:0;
}
   
// ���[�U�Z���T�摜�ւ̗֊s�̕`��
void DrawBodyContour(IplImage *image, CvPoint center, int rotation)
{
	// ���̗̂֊s�ʒu�̌v�Z�ƕ`��
	CvPoint bodyPositionData[36];
	int     bodyEnableFlg[36];
	CalculateBodyContourPosition(center, rotation, bodyEnableFlg, bodyPositionData);

	for(int i=0; i<m_NofBodyEdgePos; i++){
		if(bodyEnableFlg[i]==1){
			cvCircle( image, cvPoint(bodyPositionData[i].x, bodyPositionData[i].y), 2, CV_RGB(255,0,0), 1, 8);
		}else{
			cvCircle( image, cvPoint(bodyPositionData[i].x, bodyPositionData[i].y), 2, CV_RGB(128,128,128), 1, 8);
		}
	}
	
	// �����̗֊s�ʒu�̌v�Z�ƕ`��
	CvPoint headPositionData[36];
	int     headEnableFlg[36];
	CalculateHeadContourPosition(center, cvPoint(m_headAxis,m_headAxis), headEnableFlg, headPositionData);
	
	for(int j=0; j<m_NofHeadEdgePos; j++){
		if(headEnableFlg[j]==1){
			cvCircle( image, cvPoint(headPositionData[j].x, headPositionData[j].y), 2, CV_RGB(255,0,0), 1, 8);
		}else{
			cvCircle( image, cvPoint(headPositionData[j].x, headPositionData[j].y), 2, CV_RGB(128,128,128), 1, 8);
		}
	}

	return;
}

