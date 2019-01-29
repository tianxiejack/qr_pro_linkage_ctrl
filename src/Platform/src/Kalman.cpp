//#include "stdafx.h"
//#include <windows.h>
#include "Kalman.h"
#include <math.h>
#include <memory.h>

CKalman_PTZ::CKalman_PTZ()
{

}
CKalman_PTZ::~CKalman_PTZ()
	{

	}

static void MatrixMultiply(double* A, double* B, int m, int p, int n, double* C)
{
	// A = input matrix (m x p)
	// B = input matrix (p x n)
	// m = number of rows in A
	// p = number of columns in A = number of rows in B
	// n = number of columns in B
	// C = output matrix = A*B (m x n)
	int i, j, k;
	for (i=0;i<m;i++)
	{
		for(j=0;j<n;j++)
		{
			C[n*i+j]=0;
			for (k=0;k<p;k++)
			{
				C[n*i+j]= C[n*i+j]+A[p*i+k]*B[n*k+j];
			}
		}
	}
}

static void MatrixAddition(double* A, double* B, int m, int n, double* C)
{
	// A = input matrix (m x n)
	// B = input matrix (m x n)
	// m = number of rows in A = number of rows in B
	// n = number of columns in A = number of columns in B
	// C = output matrix = A+B (m x n)
	int i, j;
	for (i=0;i<m;i++)
	{
		for(j=0;j<n;j++)
		{
			C[n*i+j]=A[n*i+j]+B[n*i+j];
		}
	}
}

static void MatrixSubtraction(double* A, double* B, int m, int n, double* C)
{
	// A = input matrix (m x n)
	// B = input matrix (m x n)
	// m = number of rows in A = number of rows in B
	// n = number of columns in A = number of columns in B
	// C = output matrix = A-B (m x n)
	int i, j;
	for (i=0;i<m;i++)
	{
		for(j=0;j<n;j++)
		{
			C[n*i+j]=A[n*i+j]-B[n*i+j];
		}
	}
}

static void MatrixTranspose(double* A, int m, int n, double* C)
{
	// A = input matrix (m x n)
	// m = number of rows in A
	// n = number of columns in A
	// C = output matrix = the transpose of A (n x m)
	int i, j;
	for (i=0;i<m;i++)
	{
		for(j=0;j<n;j++)
		{
			C[m*j+i]=A[n*i+j];
		}
	}
}

static int MatrixInversion(double* A, int n, double* AInverse)
{
	// A = input matrix (n x n)
	// n = dimension of A 
	// AInverse = inverted matrix (n x n)
	// This function inverts a matrix based on the Gauss Jordan method.
	// The function returns 1 on success, 0 on failure.
	int i, j, iPass, imx, icol, irow;
	double det, temp, pivot, factor;
/*	double* ac = (double*)calloc(n*n, sizeof(double));*///modified by zhong
	det = 1;
	for (i = 0; i < n; i++)
	{
		for (j = 0; j < n; j++)
		{
			AInverse[n*i+j] = 0;
/*			ac[n*i+j] = A[n*i+j];*/
		}
		AInverse[n*i+i] = 1;
	}
	// The current pivot row is iPass.  
	// For each pass, first find the maximum element in the pivot column.
	for (iPass = 0; iPass < n; iPass++)
	{
		imx = iPass;
		for (irow = iPass; irow < n; irow++)
		{
			if (fabs(A[n*irow+iPass]) > fabs(A[n*imx+iPass]))
				imx = irow;
		}
		// Interchange the elements of row iPass and row imx in both A and AInverse.
		if (imx != iPass)
		{
			for (icol = 0; icol < n; icol++)
			{
				temp = AInverse[n*iPass+icol];
				AInverse[n*iPass+icol] = AInverse[n*imx+icol];
				AInverse[n*imx+icol] = temp;
				if (icol >= iPass)
				{
					temp = A[n*iPass+icol];
					A[n*iPass+icol] = A[n*imx+icol];
					A[n*imx+icol] = temp;
				}
			}
		}
		// The current pivot is now A[iPass][iPass].
		// The determinant is the product of the pivot elements.
		pivot = A[n*iPass+iPass];
		det = det * pivot;
		if (det == 0) 
		{
/*			free(ac);*/
			return 0;
		}
		for (icol = 0; icol < n; icol++)
		{	
			// Normalize the pivot row by dividing by the pivot element.
			AInverse[n*iPass+icol] = AInverse[n*iPass+icol] / pivot;
			if (icol >= iPass)
				A[n*iPass+icol] = A[n*iPass+icol] / pivot;
		}
		for (irow = 0; irow < n; irow++)
		{	
			// Add a multiple of the pivot row to each row.  The multiple factor 
			// is chosen so that the element of A on the pivot column is 0.
			if (irow != iPass)
				factor = A[n*irow+iPass];
			for (icol = 0; icol < n; icol++)
			{
				if (irow != iPass)
				{
					AInverse[n*irow+icol] -= factor * AInverse[n*iPass+icol];
					A[n*irow+icol] -= factor * A[n*iPass+icol];
				}
			}
		}
	}
/*	free(ac);*/
	return 1;
}

/*
	��ȫѡ��ԪGauss-Jordan����n��ʵ����A�������A^{-1}
	�������
	double * A��     ԭ����Ϊһ������
	int n��          ����ά��
	�������
	double * A��     ��õ������
	����ֵ��
	���ر��Ϊ0����ʾ�������죻���򷵻ط�0ֵ
*/
static int MatrixBrinv( double * A, int n)
{
	int * is, * js, i, j, k, l, u, v;
	double d,p;

	is = (int *)malloc( n*sizeof(int) );
	js = (int *)malloc( n*sizeof(int) );

	for ( k = 0; k < n; k++ )
	{ 
		d = 0.0;
		for ( i = k; i < n; i++ )
		{
			for ( j = k; j < n; j++ )
			{ 
				l = i*n+j;
				p = fabs(A[l]);
				if ( p > d )
				{ 
					d = p; is[k] = i; js[k] = j;
				}
			}
		}
		if ( d+1.0 == 1.0 ) /* ����Ϊ������ */
		{ 
			free( is ); 
			free( js ); 
			return( 0 );
		}
		if ( is[k] != k )
		{
			for ( j = 0; j < n; j++ )
			{ 
				u = k*n+j;
				v = is[k]*n+j;
				p = A[u]; A[u] = A[v]; A[v] = p;
			}
		}
		if ( js[k] != k )
		{
			for ( i = 0; i < n; i++ )
			{ 
				u = i*n+k;
				v = i*n+js[k];
				p = A[u]; A[u] = A[v]; A[v] = p;
			}
		}
		l = k*n+k;
		A[l] = 1.0/A[l];
		for ( j = 0; j < n; j++ )
		{
			if ( j != k )
			{ 
				u = k*n+j;
				A[u] = A[u]*A[l];
			}
		}
		for ( i = 0; i < n; i++ )
		{
			if ( i != k )
			{
				for ( j = 0; j < n; j++ )
				{
					if ( j != k )
					{ 
						u = i*n+j;
						A[u] = A[u] - A[i*n+k]*A[k*n+j];
					}
				}
			}
		}
		for ( i = 0; i < n; i++ )
		{
			if ( i != k )
			{ 
				u = i*n+k;
				A[u] = -A[u]*A[l];
			}
		}
	}
	for ( k = n-1; k >= 0; k-- )
	{ 
		if ( js[k] != k )
		{
			for ( j = 0; j <= n-1; j++ )
			{ 
				u = k*n+j;
				v = js[k]*n+j;
				p = A[u]; A[u] = A[v]; A[v] = p;
			}
		}
		if ( is[k] != k )
		{
			for ( i = 0; i < n; i++ )
			{ 
				u = i*n+k;
				v = i*n+is[k];
				p = A[u]; A[u] = A[v]; A[v] = p;
			}
		}
	}
	free( is );
	free( js );

	return(1);
}

static void MatrixCopy(double *A, double *B, int m, int n)
{
	memcpy(A, B, sizeof(double)*m*n);
}

static CKalman g_Kalman;

HKalman CKalman_PTZ::KalmanInit()
{
	HKalman pKalamObj = (HKalman)&g_Kalman;
	pKalamObj->state_pre = NULL;
	pKalamObj->state_post = NULL;
	pKalamObj->measure_param = NULL;
	pKalamObj->transition_matrix = NULL;
	pKalamObj->process_noise_cov = NULL;
	pKalamObj->measurement_matrix = NULL;
	pKalamObj->measurement_noise_cov = NULL;
	pKalamObj->error_cov_pre = NULL;
	pKalamObj->error_cov_post = NULL;
	pKalamObj->gain = NULL;
	pKalamObj->control_matrix = NULL;

	pKalamObj->B_Uk = NULL;
	pKalamObj->A_Pk = NULL;
	pKalamObj->A_T = NULL;
	pKalamObj->APA_T = NULL;

	pKalamObj->H_T = NULL;
	pKalamObj->Pk_Ht = NULL;
	pKalamObj->Pk_Ht_R = NULL;
	pKalamObj->Pk_Ht_R_Inv = NULL;
	pKalamObj->H_Xk = NULL;
	pKalamObj->Kk_H_Xk = NULL;
	pKalamObj->H_Pk = NULL;
	pKalamObj->Kk_H_Pk = NULL;
	pKalamObj->deltat = 0.04; // ��Ƶ����ʱ����

	pKalamObj->m_bInited = false;

	return pKalamObj;
}

HKalman  CKalman_PTZ::KalmanOpen(int D, int M, int C )
{
	HKalman hKalman = KalmanInit();
	if(hKalman == NULL)
		return NULL;
	if( D <= 0 || M <= 0 ){
		//AfxMessageBox("state and measurement vectors must have positive number of dimensions! ");	
		hKalman->m_bInited = false;
		return NULL;
	}
	if( C < 0 ){
        //AfxMessageBox("No Control!");
		hKalman->m_bInited = false;
		return NULL;
	}
    hKalman->DP = D;
    hKalman->MP = M;
    hKalman->CP = C;

	if(hKalman->state_pre == NULL){
		hKalman->state_pre =  (double*)malloc(hKalman->DP * 1*sizeof(double));
		memset( hKalman->state_pre ,  0 , sizeof (*hKalman->state_pre) );
	}
	if(hKalman->state_post == NULL){
		hKalman->state_post = (double*)malloc(hKalman->DP * 1*sizeof(double));
		memset( hKalman->state_post , 0 , sizeof (*hKalman->state_post) );
	}
	if (hKalman->measure_param == NULL){
		hKalman->measure_param =  (double*)malloc(hKalman->MP * 1*sizeof(double));
		memset( hKalman->measure_param , 0 , sizeof (*hKalman->measure_param) );
	}
	if(hKalman->transition_matrix == NULL){
		hKalman->transition_matrix =  (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->transition_matrix , 0 , sizeof (*hKalman->transition_matrix) );
	}
	if(hKalman->process_noise_cov == NULL){
		hKalman->process_noise_cov = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->process_noise_cov , 0 , sizeof (*hKalman->process_noise_cov) );
	}
	if(hKalman->measurement_matrix == NULL){
		hKalman->measurement_matrix = (double*)malloc(hKalman->MP * hKalman->DP*sizeof(double));
		memset( hKalman->measurement_matrix , 0 , sizeof (*hKalman->measurement_matrix) );
	}
	if(hKalman->measurement_noise_cov == NULL){
		hKalman->measurement_noise_cov =  (double*)malloc(hKalman->MP * hKalman->MP*sizeof(double));
		memset( hKalman->measurement_noise_cov , 0 , sizeof (*hKalman->measurement_noise_cov) );
	}
	if(hKalman->error_cov_pre == NULL){
		hKalman->error_cov_pre = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->error_cov_pre , 0 , sizeof (*hKalman->error_cov_pre) );
	}
	if(hKalman->error_cov_post == NULL){
		hKalman->error_cov_post = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->error_cov_post , 0 , sizeof (*hKalman->error_cov_post) );
	}
	if(hKalman->gain == NULL){
		hKalman->gain = (double*)malloc(hKalman->DP * hKalman->MP*sizeof(double));
		memset( hKalman->gain , 0 , sizeof (*hKalman->gain) );
	}
	if( hKalman->CP > 0 )
    {
		if(hKalman->control_matrix == NULL){
			hKalman->control_matrix = (double*)malloc(hKalman->DP * hKalman->CP*sizeof(double));
			memset( hKalman->control_matrix , 0 , sizeof (*hKalman->control_matrix) );
		}
    }

	if(hKalman->B_Uk == NULL){
		hKalman->B_Uk  = (double*)malloc(hKalman->DP * hKalman->MP*sizeof(double)); 
		memset( hKalman->B_Uk ,  0 , sizeof (*hKalman->B_Uk) );
	}
	if(hKalman->A_Pk == NULL){
		hKalman->A_Pk  = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->A_Pk ,  0 , sizeof (*hKalman->A_Pk) );
	}
	if(hKalman->A_T == NULL){
		hKalman->A_T   = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->A_T ,  0 , sizeof (*hKalman->A_T) );
	}
	if(hKalman->APA_T == NULL){
		hKalman->APA_T = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->APA_T ,  0 , sizeof (*hKalman->APA_T) );
	}
	
	if(hKalman->H_T == NULL){
		hKalman->H_T       = (double*)malloc(hKalman->DP * hKalman->MP*sizeof(double));
		memset( hKalman->H_T ,  0 , sizeof (*hKalman->H_T) );
	}
	if(hKalman->Pk_Ht == NULL){
		hKalman->Pk_Ht     = (double*)malloc(hKalman->DP * hKalman->MP*sizeof(double));
		memset( hKalman->Pk_Ht ,  0 , sizeof (*hKalman->Pk_Ht) );
	}
	if(hKalman->Pk_Ht_R == NULL){
		hKalman->Pk_Ht_R   = (double*)malloc(hKalman->MP * hKalman->MP*sizeof(double));
		memset( hKalman->Pk_Ht_R ,  0 , sizeof (*hKalman->Pk_Ht_R) );
	}
	if(hKalman->Pk_Ht_R_Inv == NULL){
		hKalman->Pk_Ht_R_Inv = (double*)malloc(hKalman->MP * hKalman->MP*sizeof(double));
		memset( hKalman->Pk_Ht_R_Inv ,  0 , sizeof (*hKalman->Pk_Ht_R_Inv) );
	}
	if(hKalman->H_Xk == NULL){
		hKalman->H_Xk      = (double*)malloc(hKalman->MP *  1*sizeof(double));
		memset( hKalman->H_Xk ,  0 , sizeof (*hKalman->H_Xk) );
	}
	if(hKalman->Kk_H_Xk == NULL){
		hKalman->Kk_H_Xk   = (double*)malloc(hKalman->DP *  1*sizeof(double));
		memset( hKalman->Kk_H_Xk ,  0 , sizeof (*hKalman->Kk_H_Xk) );
	}
	if(hKalman->H_Pk == NULL){
		hKalman->H_Pk      = (double*)malloc(hKalman->MP * hKalman->DP *sizeof(double));
		memset( hKalman->H_Pk ,  0 , sizeof (*hKalman->H_Pk) );
	}
	if(hKalman->Kk_H_Pk == NULL){
		hKalman->Kk_H_Pk   = (double*)malloc(hKalman->DP * hKalman->DP*sizeof(double));
		memset( hKalman->Kk_H_Pk ,  0 , sizeof (*hKalman->Kk_H_Pk) );
	}
	hKalman->m_bInited = true;
	return hKalman;
}

void CKalman_PTZ::KalmanClose(HKalman hKalman)
{
	if(hKalman == NULL)
		return ;
	if(hKalman->m_bInited == false)
		return;
	if(hKalman->state_pre != NULL){
		free (hKalman->state_pre) ;        hKalman->state_pre = NULL;
	}
	if(hKalman->state_post != NULL){
		free (hKalman->state_post) ;        hKalman->state_post = NULL;
	}
	if (hKalman->measure_param != NULL){
		free (hKalman->measure_param) ;        hKalman->measure_param = NULL;
	}
	if(hKalman->transition_matrix != NULL){
		free (hKalman->transition_matrix) ;  hKalman->transition_matrix = NULL;
	}
	if(hKalman->CP >0)
	{
		if(hKalman->control_matrix != NULL){
			free (hKalman->control_matrix) ;  hKalman->control_matrix = NULL;
		}
	}
	if(hKalman->measurement_matrix != NULL){
		free (hKalman->measurement_matrix) ;  hKalman->measurement_matrix = NULL;
	}
	if(hKalman->process_noise_cov != NULL){
		free (hKalman->process_noise_cov) ;   hKalman->process_noise_cov = NULL;
	}
	if(hKalman->measurement_noise_cov != NULL){
		free (hKalman->measurement_noise_cov) ;  hKalman->measurement_noise_cov = NULL;
	}
	if(hKalman->error_cov_pre != NULL){
		free (hKalman->error_cov_pre) ;    hKalman->error_cov_pre = NULL;
	}
	if(hKalman->gain != NULL){
		free (hKalman->gain) ;  hKalman->gain = NULL;
	}
	if(hKalman->error_cov_post != NULL){
		free (hKalman->error_cov_post) ;      hKalman->error_cov_post = NULL;
	}
	
	if(hKalman->B_Uk != NULL){
		free (hKalman->B_Uk) ;   hKalman->B_Uk = NULL;
	}
	if(hKalman->A_Pk != NULL){
		free (hKalman->A_Pk) ;   hKalman->A_Pk = NULL;
	}
	if(hKalman->A_T != NULL){
		free (hKalman->A_T) ;   hKalman->A_T = NULL;
	}
	if(hKalman->APA_T != NULL){
		free (hKalman->APA_T) ;  hKalman->APA_T = NULL;
	}
	
	if(hKalman->H_T != NULL){
		free (hKalman->H_T) ;     hKalman->H_T = NULL;
	}
	if(hKalman->Pk_Ht != NULL){
		free (hKalman->Pk_Ht) ;    hKalman->Pk_Ht = NULL;
	}
	if(hKalman->Pk_Ht_R != NULL){
		free (hKalman->Pk_Ht_R) ;   hKalman->Pk_Ht_R = NULL;
	}
	if(hKalman->Pk_Ht_R_Inv != NULL){
		free (hKalman->Pk_Ht_R_Inv) ;  hKalman->Pk_Ht_R_Inv = NULL;
	}
	if(hKalman->H_Xk != NULL){
		free (hKalman->H_Xk) ;     hKalman->H_Xk = NULL;
	}
	if(hKalman->Kk_H_Xk != NULL){
		free (hKalman->Kk_H_Xk) ;  hKalman->Kk_H_Xk = NULL;
	}
	if(hKalman->H_Pk != NULL){
		free (hKalman->H_Pk) ;     hKalman->H_Pk = NULL;
	}
	if(hKalman->Kk_H_Pk != NULL){
		free (hKalman->Kk_H_Pk) ;   hKalman->Kk_H_Pk = NULL;
	}
	hKalman->m_bInited = false;
}


void CKalman_PTZ::KalmanInitParam(HKalman hKalman, float theta, float delta_x, float delta_y, double DeltaT)
{
	int x, y;
	if(hKalman == NULL)
		return ;
	if (!hKalman->m_bInited){
		return;
	}
	hKalman->deltat = DeltaT;
	/* ��̼�������Э������� */
	for ( y = 0; y < hKalman->DP; y++ ){
		for ( x = 0; x < hKalman->DP; x++ ){
			hKalman->process_noise_cov[y*hKalman->DP+x] = 0;//1E-5;
		}
	}
	hKalman->process_noise_cov[1*hKalman->DP+1] = 1E-5;//0.00000001;//100.0;  /* Э���Ϊ100.0�����໥���� */
	hKalman->process_noise_cov[3*hKalman->DP+3] = 1E-5;//0.00000001;//100.0;  /* Э���Ϊ100.0�����໥���� */
	hKalman->process_noise_cov[5*hKalman->DP+5] = 1E-5;//0.00000001;//100.0;  /* Э���Ϊ100.0�����໥���� */

	/* �۲�����Э������� */
	for ( y = 0; y < hKalman->MP; y++ ){
		for ( x = 0; x < hKalman->MP; x++ ){
			hKalman->measurement_noise_cov[y*hKalman->MP+x] = 0;
		}
	}
	hKalman->measurement_noise_cov[0*hKalman->MP+0] = 0.25;  /* Э���Ϊ0.001�����໥���� */
	hKalman->measurement_noise_cov[1*hKalman->MP+1] = 0.25;  /* Э���Ϊ0.001�����໥���� */
	hKalman->measurement_noise_cov[2*hKalman->MP+2] = 0.25;  /* Э���Ϊ0.001�����໥���� */

	/* ״̬�������Э���� */
	for ( y = 0; y < hKalman->DP; y++ ){
		for ( x = 0; x < hKalman->DP; x++ ){
			hKalman->error_cov_post[y*hKalman->DP+x] = 0.0;
		}
	}
	for ( y = 0; y < hKalman->DP; y++ ){
		hKalman->error_cov_post[y*hKalman->DP+y] = 1.0;  /* �Խǳ�ʼЭ���Ϊ1�����໥���� */
	}

	/* ״̬ת���� */
	for ( y = 0; y < hKalman->DP; y++ ){
		for ( x = 0; x < hKalman->DP; x++ ){
			hKalman->transition_matrix[y*hKalman->DP+x] = 0.0;
		}
	}
	for ( y = 0; y < hKalman->DP; y++ ){
		hKalman->transition_matrix[y*hKalman->DP+y] = 1.0;  /* �Խ�Ϊ1 */
	}
	hKalman->transition_matrix[0*hKalman->DP+1] = 1;
	hKalman->transition_matrix[2*hKalman->DP+3] = 1;
	hKalman->transition_matrix[4*hKalman->DP+5] = 1;

	/* �۲���״̬�����۲�����ת�ƾ��� */
	for ( y = 0; y < hKalman->MP; y++ ){
		for ( x = 0; x < hKalman->DP; x++ ){
			hKalman->measurement_matrix[y*hKalman->DP+x] = 0.0; 
		}
	}
	hKalman->measurement_matrix[0*hKalman->DP+0] = 1.0;
	hKalman->measurement_matrix[1*hKalman->DP+2] = 1.0;
	hKalman->measurement_matrix[2*hKalman->DP+4] = 1.0;

	// �۲���������˳��theta, x, y
	hKalman->measure_param[0] = (float)theta;
	hKalman->measure_param[1] = (float)delta_x;
	hKalman->measure_param[2] = (float)delta_y;
	/* ��ʼ��thelta, thelta_v, x, x_v, y, y_v��״̬�� */
    // ״̬��������˳��thelta, thelta_v,  x, y, x_v, y_v
	hKalman->state_post[0] = theta;
	hKalman->state_post[1] = 0.0;
	hKalman->state_post[2] = delta_x;
	hKalman->state_post[3] = 0.0;
	hKalman->state_post[4] = delta_y;
	hKalman->state_post[5] = 0.0;
}



void CKalman_PTZ::KalmanPredict(HKalman hKalman,  double * control )
{
	if(hKalman == NULL)
		return;
	if (!hKalman->m_bInited){
		return;
	}
	/* update the state */
	/* x'(k) = A*x(k) */
	MatrixMultiply( hKalman->transition_matrix, hKalman->state_post, hKalman->DP , hKalman->DP , 1 , hKalman->state_pre );

	if( control!=NULL && hKalman->CP > 0 ){
		/* x'(k) = x'(k) + B*u(k) */
		MatrixMultiply( hKalman->control_matrix, control, hKalman->DP , hKalman->CP , 1 , hKalman->B_Uk);
		MatrixAddition( hKalman->state_pre, hKalman->B_Uk, hKalman->DP, 1, hKalman->state_pre);
	}
	
	/* update error covariance matrices */
	/* A_Pk = A*P(k) */
	MatrixMultiply( hKalman->transition_matrix, hKalman->error_cov_post, hKalman->DP, hKalman->DP, hKalman->DP, hKalman->A_Pk);
	
	/* P'(k) = A_Pk*At + Q */
	MatrixTranspose(hKalman->transition_matrix, hKalman->DP, hKalman->DP, hKalman->A_T);
	MatrixMultiply(hKalman->A_Pk, hKalman->A_T, hKalman->DP, hKalman->DP, hKalman->DP, hKalman->APA_T);
	MatrixAddition(hKalman->APA_T, hKalman->process_noise_cov, hKalman->DP, hKalman->DP, hKalman->error_cov_pre);
}

void CKalman_PTZ::KalmanCorrect(HKalman hKalman,  double * measurement )
{
	int i;
	if(hKalman == NULL)
		return;
	if (!hKalman->m_bInited){
		return;
	}
	if( measurement == NULL)
	{
		;//AfxMessageBox("Measurement is Null!!!");
	}
	if(measurement != NULL){
		for (i=0; i<hKalman->MP; i++){
			hKalman->measure_param[i] = measurement[i];
		}
	}
	/* H_T = Ht*/
	MatrixTranspose( hKalman->measurement_matrix , hKalman->MP , hKalman->DP , hKalman->H_T);
	/* Pk_Ht = P'(k) * H_T */
    MatrixMultiply( hKalman->error_cov_pre, hKalman->H_T, hKalman->DP , hKalman->DP , hKalman->MP , hKalman->Pk_Ht);
	
    /* Pk_Ht_R = H*Pk_Ht + R */
    MatrixMultiply( hKalman->measurement_matrix ,hKalman->Pk_Ht , hKalman->MP , hKalman->DP , hKalman->MP , hKalman->Pk_Ht_R);
	MatrixAddition( hKalman->Pk_Ht_R , hKalman->measurement_noise_cov , hKalman->MP , hKalman->MP , hKalman->Pk_Ht_R);
	
    /* Pk_Ht_R_Inv = inv(Pk_Ht_R) */
#if 0
    MatrixInversion( hKalman->Pk_Ht_R , hKalman->MP, hKalman->Pk_Ht_R_Inv);   
#else
    MatrixCopy(hKalman->Pk_Ht_R_Inv, hKalman->Pk_Ht_R, hKalman->MP, hKalman->MP);
	MatrixBrinv(hKalman->Pk_Ht_R_Inv, hKalman->MP);
#endif
    
    /* K(k) = Pk_Ht * Pk_Ht_R_Inv  */
    MatrixMultiply( hKalman->Pk_Ht , hKalman->Pk_Ht_R_Inv, hKalman->DP , hKalman->MP ,hKalman->MP , hKalman->gain);

    //update state_post
    /* H_Xk = z(k) - H*x'(k) */
	MatrixMultiply( hKalman->measurement_matrix , hKalman->state_pre , hKalman->MP , hKalman->DP , 1, hKalman->H_Xk);
	MatrixSubtraction( hKalman->measure_param , hKalman->H_Xk , hKalman->MP , 1, hKalman->H_Xk);    
    /* x(k) = x'(k) + K(k)*H_Xk */
	MatrixMultiply( hKalman->gain , hKalman->H_Xk, hKalman->DP , hKalman->MP, 1, hKalman->Kk_H_Xk );
    MatrixAddition( hKalman->state_pre , hKalman->Kk_H_Xk , hKalman->DP ,1 , hKalman->state_post);
 
	//update error_cov_post
    /* P(k) = P'(k) - K(k)*H* P'(k) */
    MatrixMultiply( hKalman->measurement_matrix , hKalman->error_cov_pre , hKalman->MP , hKalman->DP , hKalman->DP , hKalman->H_Pk );
	MatrixMultiply( hKalman->gain , hKalman->H_Pk , hKalman->DP , hKalman->MP, hKalman->DP , hKalman->Kk_H_Pk );
	MatrixSubtraction( hKalman->error_cov_pre , hKalman->Kk_H_Pk , hKalman->DP ,hKalman->DP , hKalman->error_cov_post);
}

int CKalman_PTZ::Kalman(HKalman hKalman, double *measure, double *control)
{
	if(hKalman == NULL)
		return -1;
	KalmanPredict(hKalman, control);
	KalmanCorrect(hKalman, measure);
	return 0;
}
