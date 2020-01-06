/* 
 * MDFourier
 * A Fourier Transform analysis tool to compare different 
 * Sega Genesis/Mega Drive audio hardware revisions, and
 * other hardware in the future
 *
 * Copyright (C)2019 Artemio Urbina
 *
 * This file is part of the 240p Test Suite
 *
 * You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 *
 * Requires the FFTW library: 
 *	  http://www.fftw.org/
 */

#include "mdfourier.h"
#include "windows.h"
#include "log.h"
#include "freq.h"

#define MAX_WINDOWS	100

int initWindows(windowManager *wm, int SamplesPerSec, char winType, parameters *config)
{
	if(!wm || !config)
		return 0;

	wm->MaxWindow = 0;
	wm->SamplesPerSec = 0;
	wm->winType = 'n';
	
	if(winType == 'n')
	{
		wm->windowArray = NULL;
		wm->windowCount = 0;
		return 1;
	}

	// Create the wm
	wm->windowArray = (windowUnit*)malloc(sizeof(windowUnit)*MAX_WINDOWS);
	if(!wm->windowArray)
	{
		logmsg("Not enough memory for window manager\n");
		return 0;
	}
	wm->windowCount = 0;
	wm->MaxWindow = MAX_WINDOWS-1;
	wm->SamplesPerSec = SamplesPerSec;
	wm->winType = winType;

	memset(wm->windowArray, 0, sizeof(windowUnit)*MAX_WINDOWS);

	return 1;
}

double *CreateWindow(windowManager *wm, long int frames, double framerate)
{
	double		seconds = 0;
	double		*window = NULL;
	long int	size = 0;

	if(!wm)
		return NULL;

	if(wm->winType == 'n')
		return NULL;

	if(wm->windowCount == wm->MaxWindow)
	{
		logmsg("ERROR: Reached Max window limit %d\n", wm->MaxWindow);
		return NULL;
	}

	seconds = FramesToSeconds(frames, framerate);
	size = floor(wm->SamplesPerSec*seconds);
	if(!size)
	{
		logmsg("ERROR: Asked for window with null size %ld Frames %g Framerate\n", frames, framerate);
		return NULL;
	}
	//logmsg("**** Creating window size %ld (%ld frames %g fr)\n", size, frames, framerate);
	if(wm->winType == 't')
	{
		window = tukeyWindow(size);
		if(!window)
		{
			logmsg ("Tukey window creation failed\n");
			return NULL;
		}
		wm->windowArray[wm->windowCount].window = window;
		wm->windowArray[wm->windowCount].seconds = seconds;
		wm->windowArray[wm->windowCount].size = size;
		wm->windowCount++;
		return window;
	}

	if(wm->winType == 'f')
	{
		window = flattopWindow(size);
		if(!window)
		{
			logmsg ("Flattop window creation failed\n");
			return NULL;
		}
		wm->windowArray[wm->windowCount].window = window;
		wm->windowArray[wm->windowCount].seconds = seconds;
		wm->windowArray[wm->windowCount].size = size;
		wm->windowCount++;
		return window;
	}

	if(wm->winType == 'h')
	{
		window = hannWindow(size);
		if(!window)
		{
			logmsg ("Hann window creation failed\n");
			return NULL;
		}
		wm->windowArray[wm->windowCount].window = window;
		wm->windowArray[wm->windowCount].seconds = seconds;
		wm->windowArray[wm->windowCount].size = size;
		wm->windowCount++;
		return window;
	}

	if(wm->winType == 'm')
	{
		window = hammingWindow(size);
		if(!window)
		{
			logmsg ("Hamming window creation failed\n");
			return NULL;
		}
		wm->windowArray[wm->windowCount].window = window;
		wm->windowArray[wm->windowCount].seconds = seconds;
		wm->windowArray[wm->windowCount].size = size;
		wm->windowCount++;
		return window;
	}

	logmsg("FAILED Creating window size %g (%ld frames %g fr)\n", frames*framerate, frames, framerate);
	return NULL;
}

double *getWindowByLength(windowManager *wm, long int frames, double framerate)
{
	double		seconds = 0;
	long int	size = 0;

	if(!wm)
		return 0;

	seconds = FramesToSeconds(frames, framerate);
	size = floor(wm->SamplesPerSec*seconds);
	for(int i = 0; i < wm->windowCount; i++)
	{
		//logmsg("Comparing pos %d: %g to %g from %d\n", i, seconds, wm->windowArray[i].seconds, wm->windowCount);
		if(size == wm->windowArray[i].size)
		{
			//logmsg("Served window size %ld (%ld frames %g fr)\n", size, frames, framerate);
			return wm->windowArray[i].window;
		}
	}

	return CreateWindow(wm, frames, framerate);
}

void freeWindows(windowManager *wm)
{
	if(!wm)
		return;

	for(int i = 0; i < wm->windowCount; i++)
	{
		if(wm->windowArray[i].window)
		{
			free(wm->windowArray[i].window);
			wm->windowArray[i].window = NULL;
		}
	}
	if(wm->windowCount)
	{
		free(wm->windowArray);
		wm->windowArray = NULL;
		wm->windowCount = 0;
	}

	wm->MaxWindow = 0;
	wm->SamplesPerSec = 0;
	wm->winType = 'n';
}

// reduce scalloping loss 
double *flattopWindow(long int n)
{
	int half, i, idx;
	double *w;
 
	w = (double*) calloc(n, sizeof(double));
	if(!w)
	{
		logmsg("Not enough memory for window\n");
		return NULL;
	}
	memset(w, 0, n*sizeof(double));
 
	if(n%2==0)
	{
		half = n/2;
		for(i=0; i<half; i++)
		{
			double factor = 2*M_PI*i/(n-1);
			w[i] = 0.21557895 - 0.41663158*cos(factor) + 0.277263158*cos(2*factor) 
				  -0.083578947*cos(3*factor) + 0.006947368*cos(4*factor);
		}
 
		idx = half-1;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
	else
	{
		half = (n+1)/2;
		for(i=0; i<half; i++)
		{
			double factor = 2*M_PI*i/(n-1);
			w[i] = 0.21557895 - 0.41663158*cos(factor) + 0.277263158*cos(2*factor) 
				  -0.083578947*cos(3*factor) + 0.006947368*cos(4*factor);
		}
 
		idx = half-2;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
 
	return(w);
}


// Only attenuate the edges to reduce errors
double *tukeyWindow(long int n)
{
	long int i;
	double *w, M = 0, alpha = 0;
 
	w = (double*) calloc(n, sizeof(double));
	if(!w)
	{
		logmsg("Not enough memory for window\n");
		return NULL;
	}
	memset(w, 0, n*sizeof(double));
 
	alpha = 0.65;
	M = (n-1)/2;

	for(i=0; i<n; i++)
	{
		if(fabs(i-M) >= alpha*M)
			w[i] = 0.5*(1+cos(M_PI*(fabs(i-M)-alpha*M)/((1-alpha)*M)));
		else
			w[i] = 1;
	}
 
	return(w);
}

double *hannWindow(long int n)
{
	long int half, i, idx;
	double *w;
 
	w = (double*) calloc(n, sizeof(double));
	if(!w)
	{
		logmsg("Not enough memory for window\n");
		return NULL;
	}
	memset(w, 0, n*sizeof(double));

	if(n%2==0)
	{
		half = n/2;
		for(i=0; i<half; i++) //CALC_HANNING   Calculates Hanning window samples.
			w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));
 
		idx = half-1;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
	else
	{
		half = (n+1)/2;
		for(i=0; i<half; i++) //CALC_HANNING   Calculates Hanning window samples.
			w[i] = 0.5 * (1 - cos(2*M_PI*(i+1) / (n+1)));
 
		idx = half-2;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
 
	return(w);
}

double *hammingWindow(long int n)
{
	long int half, i, idx;
	double *w;
 
	w = (double*) calloc(n, sizeof(double));
	if(!w)
	{
		logmsg("Not enough memory for window\n");
		return NULL;
	}
	memset(w, 0, n*sizeof(double));

	if(n%2==0)
	{
		half = n/2;
		for(i=0; i<half; i++)
			w[i] = 0.54 -0.46*cos((2*M_PI*i)/(n-1));
 
		idx = half-1;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
	else
	{
		half = (n+1)/2;
		for(i=0; i<half; i++)
			w[i] = 0.54 -0.46*cos((2*M_PI*i)/(n-1));
 
		idx = half-2;
		for(i=half; i<n; i++) {
			w[i] = w[idx];
			idx--;
		}
	}
 
	return(w);
}

double cheby_poly(int n, double x)
{
	double res;

	if (fabs(x) <= 1)
		res = cos(n*acos(x));
	else
		res = cosh(n*acosh(x));
	return res;
}

/**************************************************************************
 Dolph-Chebyshev Window
 - Taken from http://practicalcryptography.com/miscellaneous/machine-learning/implementing-dolph-chebyshev-window/
-atten is the required sidelobe attenuation (e.g. if you want -60dB atten, use '60')
***************************************************************************/
double *cheby_win(long int N, float atten)
{
	int nn, i;
	double M, n, sum = 0, max=0;
	double tg = pow(10,atten/20);  /* 1/r term [2], 10^gamma [2] */
	double x0 = cosh((1.0/(N-1))*acosh(tg));
	double *w = NULL;

	w = (double*) calloc(N, sizeof(double));
	if(!w)
	{
		logmsg("Not enough memory for window\n");
		return NULL;
	}
	memset(w, 0, N*sizeof(double));

	M = (N-1)/2;
	if(N%2==0)
		M = M + 0.5; /* handle even length windows */
	for(nn=0; nn<(N/2+1); nn++)
	{
		n = nn-M;
		sum = 0;
		for(i=1; i<=M; i++)
		{
			sum += cheby_poly(N-1,x0*cos(M_PI*i/N))*cos(2.0*n*M_PI*i/N);
		}
		w[nn] = tg + 2*sum;
		w[N-nn-1] = w[nn];
		if(w[nn]>max)
			max = w[nn];
	}
	for(nn=0; nn<N; nn++)
		w[nn] /= max; /* normalise everything */
	return(w);
}

double CalculateCorrectionFactor(windowManager *wm, long int frames)
{
	double		*window = NULL;
	double		factor = 0, sum = 0;
	long int	size = 0;

	if(!wm)
		return 1;

	for(int i = 0; i < wm->windowCount; i++)
	{
		if(frames == wm->windowArray[i].frames)
		{
			window = wm->windowArray[i].window;
			size = wm->windowArray[i].size;
			break;
		}
	}

	if(!window)
		return 1;

	for(long int i = 0; i < size; i++)
		sum += window[i];
	
	factor = (double)size/sum;

	return factor;
}

double CompensateValueForWindow(double value, char winType)
{
	switch(winType)
	{
		case 'n':
			break;
		case 't':
			value *= 1.2122;
			break;
		case 'f':
			value *= 4.63899;
			break;
		case 'h':
			value *= 1.99986;
			break;
		case 'm':
			value *= 1.85196;
			break;
	}

	return value;
}
