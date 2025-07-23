/*
*  MyCondensation.cpp
*  �K�v���C�u���� opencv_core249d.lib opencv_imgproc249d.lib opencv_legacy249d.lib opencv_highgui249d.lib
*/
#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/cvaux.h>
#include <opencv/cxcore.h>
#include "MyCondensation.h"

// �G���[�o�̓t�@�C��
extern FILE *fp;

//*************************************************************
// Condensation�\���̂�����������֐�
//*************************************************************
CvConDensation* myCreateConDensation( int DP, int SamplesNum )
{
    int i;
    CvConDensation *CD = 0;
    
    if( DP < 0 || SamplesNum < 0 ) { printf("Condensation Error.\n"); return NULL; }
    
    /* allocating memory for the structure */
    CD = (CvConDensation *) cvAlloc( sizeof( CvConDensation ));
    /* setting structure params */
    CD->SamplesNum = SamplesNum;
    CD->DP = DP;
	// �g���ĂȂ�MP��ݐϖޓx�̕ۑ��Ɏg��
    CD->MP = 0;
    /* allocating memory for structure fields */
    CD->flSamples       = (float **) cvAlloc( sizeof( float * ) * SamplesNum );
    CD->flNewSamples    = (float **) cvAlloc( sizeof( float * ) * SamplesNum );
    CD->flSamples[0]    = (float *)  cvAlloc( sizeof( float ) * SamplesNum * DP );
    CD->flNewSamples[0] = (float *)  cvAlloc( sizeof( float ) * SamplesNum * DP );

    /* setting pointers in pointer's arrays */
    for( i = 1; i < SamplesNum; i++ )
    {
        CD->flSamples[i]    = CD->flSamples[i - 1] + DP;
        CD->flNewSamples[i] = CD->flNewSamples[i - 1] + DP;
    }

    CD->State        = (float *) cvAlloc( sizeof( float ) * DP );
    CD->DynamMatr    = (float *) cvAlloc( sizeof( float ) * DP * DP );
    CD->flConfidence = (float *) cvAlloc( sizeof( float ) * SamplesNum );
    CD->flCumulative = (float *) cvAlloc( sizeof( float ) * SamplesNum );

    CD->RandS        = (CvRandState *) cvAlloc( sizeof( CvRandState ) * DP );
    CD->Temp         = (float *) cvAlloc( sizeof( float ) * DP );
    CD->RandomSample = (float *) cvAlloc( sizeof( float ) * DP );

    /* Returning created structure */
    return CD;
}

//*************************************************************
// Condensation�\���̂��J������֐�
//*************************************************************
void myReleaseConDensation( CvConDensation ** ConDensation )
{
    CvConDensation *CD = *ConDensation;
    
    if( !ConDensation ) { printf("Condensation Error.\n"); return; }

    /* freeing the memory */
	cvFree( (void**)&CD->State );
    cvFree( (void**)&CD->DynamMatr);
    cvFree( (void**)&CD->flConfidence );
    cvFree( (void**)&CD->flCumulative );
    cvFree( (void**)&CD->flSamples[0] );
    cvFree( (void**)&CD->flNewSamples[0] );
    cvFree( (void**)&CD->flSamples );
    cvFree( (void**)&CD->flNewSamples );
    cvFree( (void**)&CD->Temp );
    cvFree( (void**)&CD->RandS );
    cvFree( (void**)&CD->RandomSample );
    /* release structure */
    cvFree( (void**)ConDensation );
}

//*************************************************************
// ���Ғl�̌v�Z���s���֐�
//*************************************************************
void myConDensUpdateByTime( CvConDensation * ConDens )
{
    if( !ConDens ) { printf("Condensation Error.\n"); return; }

    /* Sets Temp to Zero */
    icvSetZero_32f( ConDens->Temp, ConDens->DP, 1 );

	// �ޓx�ŏd�ݕt�����ꂽ�T���v���̒l�̘a�FConDens->Temp
    /* Calculating the Mean */
    float Sum = 0.000001f;//float�̗L��������7�Ȃ̂ŁC0�ɋ߂��ŏ��l
    for(int i = 0; i < ConDens->SamplesNum; i++ ){
		//�T���v����ޓx�ŏd�ݕt�����Ĉꎞ�I��ConDens->State��
        icvScaleVector_32f( ConDens->flSamples[i], ConDens->State, ConDens->DP, ConDens->flConfidence[i] );
		//ConDens->Temp�ɗݐς���
        icvAddVector_32f( ConDens->Temp, ConDens->State, ConDens->Temp, ConDens->DP );
		//�ޓx��Sum�֗ݐς���
        Sum += ConDens->flConfidence[i];
		//�T���v�����O�̂��ߗݐϖޓx���e�T���v���ɃZ�b�g
        ConDens->flCumulative[i] = Sum;
    }
	// �ޓx�ŏd�ݕt�����ꂽ�T���v���̒l�̘a��ޓx�̘a�Ŋ����Đ��K���FConDens->State
    icvScaleVector_32f( ConDens->Temp, ConDens->State, ConDens->DP, 1.f / Sum );
	// ���v�ޓx��MP�ɕۑ�(�{��MP�͊ϑ���������\�����̂����A�g���Ă��Ȃ����߂����Ŏg�p)
	ConDens->MP = (int)Sum;

}

//*************************************************************
// �΂���̍Đݒ���s���֐�
//*************************************************************
void myConDensUpdateDeviation( CvConDensation * conDens, CvMat * mean, CvMat * deviation )
{
	if( !conDens || !deviation || !mean )                                             { printf("Condensation Error.\n"); return; }
    if( CV_MAT_TYPE(deviation->type) != CV_32FC1 || !CV_ARE_TYPES_EQ(deviation,mean) ){ printf("Condensation Error.\n"); return; }
    if( (deviation->cols != 1) || (mean->cols != 1) )                                 { printf("Condensation Error.\n"); return; }
    if( (deviation->rows != conDens->DP) || (mean->rows != conDens->DP) )             { printf("Condensation Error.\n"); return; }

	float *StdDev  = deviation->data.fl;
    float *MeanVal = mean->data.fl;
    for(int i = 0; i < conDens->DP; i++ ){
		cvRandInit( &(conDens->RandS[i]), MeanVal[i], StdDev[i], i, CV_RAND_NORMAL );
	}

}

//*************************************************************
// �T���v���̍X�V���s���֐�
//*************************************************************
void myConDensUpdateSample( CvConDensation * ConDens )
{
	///// �T���v�����O //////////////////////////////////////////////////////////////////////////////////
	// ���ϖޓx���v�Z(��l�ɖޓx�𕪊����ăT���v�����O���邽��)
    float Sum = 0;
    Sum = (float)ConDens->MP / ConDens->SamplesNum;
    /* Updating the set of random samples */
	// �ޓx�̔�ɏ]���ăT���v�������o��
    for(int i = 0; i < ConDens->SamplesNum; i++ ){
        int j = 1;
        while( (ConDens->flCumulative[j] <= (float) i * Sum + 0.000001f) && (j < (ConDens->SamplesNum-1)) )
		{
            j++;
        }
        icvCopyVector_32f( ConDens->flSamples[j], ConDens->DP, ConDens->flNewSamples[i] );
    }

	///// �T���v�����Ƀf�B�t���[�Y�������� //////////////////////////////////////////////////////////
    /* Adding the random-generated vector to every vector in sample set */
    for( int i = 0; i < ConDens->SamplesNum; i++ ){
		// �K�E�V�A���m�C�Y����
        for(int j = 0; j < ConDens->DP; j++ ){
            cvbRand( ConDens->RandS + j, ConDens->RandomSample + j, 1 );
        }
		// �K�E�V�A���m�C�Y��t�^
		icvAddVector_32f( ConDens->flNewSamples[i], ConDens->RandomSample, ConDens->flSamples[i], ConDens->DP );
    }

}

//*************************************************************
// �T���v���̏������֐�
//*************************************************************
void myConDensInitSampleSet( CvConDensation * conDens, CvMat * initvalue, CvMat * mean, CvMat * deviation )
{
	if( !conDens || !deviation || !mean || !initvalue ){
		printf("Condensation Error.\n"); return; }
    if( (deviation->cols != 1) || (mean->cols != 1) || (initvalue->cols != 1) ){
		printf("Condensation Error.\n"); return; }
    if( (deviation->rows != conDens->DP) || (mean->rows != conDens->DP) || (initvalue->rows != conDens->DP) ){
		printf("Condensation Error.\n"); return; }
    if( CV_MAT_TYPE(deviation->type) != CV_32FC1 || !CV_ARE_TYPES_EQ(deviation,mean) || !CV_ARE_TYPES_EQ(deviation,initvalue) ){
		printf("Condensation Error.\n"); return; }

    float *StdDev  = deviation->data.fl;
    float *MeanVal = mean->data.fl;
    float *InitVal = initvalue->data.fl;
	float Prob = 1.f / conDens->SamplesNum;

	/* Initializing the structures to create initial Sample set */
    for(int i = 0; i < conDens->DP; i++ ){
        cvRandInit( &(conDens->RandS[i]), MeanVal[i], StdDev[i], i, CV_RAND_NORMAL );
    }
    /* Generating the samples */
	for(int j = 0; j < conDens->SamplesNum; j++ ){
        for(int i = 0; i < conDens->DP; i++ ){
            cvbRand( conDens->RandS + i, conDens->flSamples[j] + i, 1 );
        }
        conDens->flConfidence[j] = Prob;
		icvAddVector_32f( conDens->flSamples[j], InitVal, conDens->flSamples[j], conDens->DP ); // �����l���v���X
    }

}
