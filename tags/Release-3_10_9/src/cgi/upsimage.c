/*
 * upsimage - cgi program to create graphical ups information reports
 *
 * Original Author: Russell Kroll <rkroll@exploits.org>
 *
 * When used together, upsstats and upsimage create interesting looking web
 * pages with graphical representations of the battery capacity, utility
 * voltage, and UPS load. 
 *
 * This program utilizes the gd graphics library for snappy IMG generation.
 * I highly recommend this package for anyone doing similar graphics
 * "on the fly" in C.
 *
 * This binary needs to be installed some place where upsstats can find it.
 *
 * Modified:  Jonathan Benson <jbenson@technologist.com>
 *	    19/6/98 to suit apcupsd
 *	    23/6/98 added more graphs and menu options
 *
 * Modified by Kern Sibbald to include additional graphs as well as
 *  to adapt the graphs to varing conditions (voltage, ...). Also,
 *  consolidate a lot of code into subroutines.
 *
 * Modified by Riccardo Facchetti to support both GIF and PNG formats.
 *
 */

#include "apc.h"

#if !defined(SYS_IMGFMT_PNG) && !defined(SYS_IMGFMT_GIF) && !defined(IMGFMT_GIF)
# error "A graphic file format must be defined to compile this program."
#endif

#include "cgiconfig.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

extern int extractcgiargs();

static char    monhost[128];
static char    cmd[16] = "";
static char    upsval[16] = "";
static char    upsval2[16] = "";
static char    upsval3[16] = "";

static int green, black, white, grey, darkgrey, red;

static void allocate_colors(gdImagePtr im)     
{
    black = gdImageColorAllocate (im, 0, 0, 0);
    green = gdImageColorAllocate (im, 0, 255, 0);
    white = gdImageColorAllocate (im, 255, 255, 255);
    grey = gdImageColorAllocate (im, 200, 200, 200);
    darkgrey = gdImageColorAllocate (im, 50, 50, 50);
    red = gdImageColorAllocate (im, 255, 0, 0);
}

static void DrawTickLines(gdImagePtr im)
{
    gdImageLine (im, 50,  60, 150,  60, darkgrey);
    gdImageLine (im, 50, 120, 150, 120, darkgrey);
    gdImageLine (im, 50, 180, 150, 180, darkgrey);
    gdImageLine (im, 50, 240, 150, 240, darkgrey);
    gdImageLine (im, 50, 300, 150, 300, darkgrey);
}

static void DrawText(gdImagePtr im, int min, int step)
{
    int next;
    char text[10];

    next = min;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 295, (unsigned char *)text, black);
    next += step;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 235, (unsigned char *)text, black);
    next += step;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 175, (unsigned char *)text, black);
    next += step;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 115, (unsigned char *)text, black);
    next += step;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 55, (unsigned char *)text, black);
    next += step;
    sprintf(text, "%d", next);
    gdImageString(im, gdFontLarge, 0, 0, (unsigned char *)text, black);
}

static gdImagePtr InitImage()
{
    gdImagePtr im;

    im = gdImageCreate(150, 350);
    allocate_colors(im);
    gdImageColorTransparent (im, grey);
    gdImageFilledRectangle (im, 0, 0, 150, 350, grey);
    gdImageFilledRectangle (im, 50, 0, 150, 300, green);

    return im;
}


void parsearg(char var[255], char value[255]) 
{
    if (!strcmp(var, "host"))
	  strncpy (monhost, value, sizeof(monhost));

    if (!strcmp(var, "display"))
	  strncpy (cmd, value, sizeof(cmd));

    if (!strcmp(var, "value"))
	  strncpy (upsval, value, sizeof(upsval));

    if (!strcmp(var, "value2"))
	  strncpy (upsval2, value, sizeof(upsval));

    if (!strcmp(var, "value3"))
	  strncpy (upsval3, value, sizeof(upsval));
}


void imgheader (void)
{
#ifdef SYS_IMGFMT_PNG
    printf ("Content-type: image/png\n");
#else
    printf ("Content-type: image/gif\n");
#endif
    printf ("Pragma: no-cache\n");
    printf ("\n");
}

static void TermImage(gdImagePtr im)
{
    DrawTickLines(im);
    imgheader();
#ifdef SYS_IMGFMT_PNG
    gdImagePng (im, stdout);
#else
    gdImageGif (im, stdout);
#endif
    gdImageDestroy (im);
}  

void drawbattcap(char *battcaps, char *minbchgs)
{
    gdImagePtr	    im;
    char	   batttxt[16];
    int 	   battpos;
    double	   battcap;
    int 	   minbchgpos;
    double	   minbchg;

    battcap = strtod(battcaps, NULL);
    minbchg = strtod(minbchgs, NULL);

    im = InitImage();

    DrawText(im, 0, 20);

    minbchgpos = (int)(300 - (minbchg * 3));
    gdImageFilledRectangle(im, 50, minbchgpos, 150, 300, red);

    battpos = (int)(300 - (battcap * 3));
    gdImageFilledRectangle(im, 75, battpos, 125, 300, black);

    sprintf(batttxt, "%.1f %%", battcap);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)batttxt, black);

    TermImage(im);
}

void drawbattvolt(char *battvolts, char *nombattvs) 
{
    gdImagePtr	    im;
    char	   batttxt[16];
    int 	   battpos;
    int 	   hipos, lowpos;
    double	   battvolt;
    double	   nombattv;
    double	   hip, lowp;	   /* hi and low red line conditions */
    int 	   minv, maxv, deltav;

    im = InitImage();

    battvolt = strtod(battvolts, NULL);
    nombattv = strtod(nombattvs, NULL);

    /* NOTE, if you tweek minv and maxv, ensure that the difference
     * is evenly divisible by 5 or the scales will be wrong!!!	 
     */
    switch ((int)nombattv) {
       case 12:
	  minv = 3;
	  maxv = 18;
	  hip = 12 + 3; 	   /* high redline -- guess */
	  lowp = 12 - 3;	   /* low redline -- guess */
	  break;
       case 24: 
	  minv = 15;
	  maxv = 30;
	  hip = 24 + 5;
	  lowp = 24 - 5;
	  break;
       case 48:
	  minv = 30;
	  maxv = 60;
	  hip = 48 + 7;
	  lowp = 48 - 7;
	  break;
       default:
	  minv = 0;
	  maxv = (int)(battvolt/10 + 1) * 10;
	  hip = battvolt + 5;
	  lowp = battvolt - 5;
	  break;
    }
    deltav = maxv - minv;

    DrawText(im, minv, (deltav)/5);


    /* Do proper scaling of battery voltage and redline positions */
    battpos = (int)(300 - (((battvolt - minv) / deltav ) * 300));
    hipos = (int)( 300 - (((hip - minv) / deltav) * 300) );
    lowpos = (int)( 300 - (((lowp - minv) / deltav) * 300) );

    gdImageFilledRectangle (im, 50, 0, 150, hipos, red);
    gdImageFilledRectangle (im, 50, lowpos, 150, 300, red);


    gdImageFilledRectangle (im, 75, battpos, 125, 300, black);

    sprintf (batttxt, "%.1f VDC", battvolt);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)batttxt, black);

    TermImage(im);
}

void noimage (void)
{
    gdImagePtr	    im;

    im = gdImageCreate (150, 350);

    allocate_colors(im);

    gdImageColorTransparent (im, grey);

    gdImageFilledRectangle (im, 0, 0, 150, 300, grey);

    gdImageString (im, gdFontLarge, 0, 0, (unsigned char *)"Data not available", black);

    imgheader();
#ifdef SYS_IMGFMT_PNG
    gdImagePng (im, stdout);
#else
    gdImageGif (im, stdout);
#endif
    gdImageDestroy (im);
}

void drawupsload(char *upsloads) 
{
    gdImagePtr	    im;
    char	   loadtxt[16];
    int 	   loadpos;
    double	   upsload;

    upsload = strtod(upsloads, NULL);

    im = InitImage();

    DrawText(im, 0, 25);

    gdImageFilledRectangle (im, 50, 0, 150, 60, red);
    gdImageFilledRectangle (im, 50, 60, 150, 300, green); 

    loadpos = (int)(300 - ((upsload / 125) * 300));
    gdImageFilledRectangle(im, 75, loadpos, 125, 300, black);

    sprintf(loadtxt, "%.1f %%", upsload);
    gdImageString(im, gdFontLarge, 70, 320, (unsigned char *)loadtxt, black);

    TermImage(im);
}

/*
 * Input Voltage */
void drawutility (char *utilitys, char *translos, char *transhis) 
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   utilpos, translopos, transhipos;
    double	   utility, translo, transhi;
    int 	   minv, deltav;

    utility = strtod(utilitys, NULL);
    translo = strtod(translos, NULL);
    transhi = strtod(transhis, NULL);

    im = InitImage();

    if (utility > 180) {	      /* Europe 230V */
       minv = 200;
       deltav = 75;
    } else if (utility > 110) {       /* US 110-120 V */
       minv = 90;
       deltav = 50;
    } else if (utility > 95) {	      /* Japan 100V */
       minv = 80;
       deltav = 50;
    } else {			      /* No voltage */
       minv = 0;
       deltav = 50;
    }

    DrawText(im, minv, deltav/5);
    utilpos = (int)(300 - (((utility - minv) / deltav) * 300) );
    translopos = (int)(300 - (((translo - minv) / deltav) * 300) );
    transhipos = (int)(300 - (((transhi - minv) / deltav) * 300) );

    gdImageFilledRectangle(im, 50, 0, 150, transhipos, red);
    gdImageFilledRectangle(im, 50, translopos, 150, 300, red);

    gdImageFilledRectangle (im, 75, utilpos, 125, 300, black);

    sprintf (utiltxt, "%.1f VAC", utility);
    gdImageString (im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 

    TermImage(im);
}

/*
 * Output Voltage
 */
void drawupsout (char *upsouts) 
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   uoutpos;
    double	   upsout;
    int 	   minv, deltav;

    upsout = strtod(upsouts, NULL);

    im = InitImage();

    if (upsout > 180) {
       minv = 200;
       deltav = 75;
    } else if (upsout > 110) {
       minv = 90;
       deltav = 50;
    } else if (upsout > 95) {
       minv = 80;
       deltav = 50;
    } else {
       minv = 0;
       deltav = 50;
    }
    
    DrawText(im, minv, deltav/5);
    uoutpos = (int)(300 - (((upsout - minv) / deltav) * 300) );

    gdImageFilledRectangle(im, 75, uoutpos, 125, 300, black);

    sprintf(utiltxt, "%.1f VAC", upsout);
    gdImageString(im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 

    TermImage(im);
}

void drawruntime (char *upsrunts, char *lowbatts)
{
    gdImagePtr	    im;
    char	   utiltxt[16];
    int 	   uoutpos, lowbattpos;
    double	   upsrunt;
    double	   lowbatt;
    int step, maxt;

    upsrunt = strtod(upsrunts, NULL);
    lowbatt = strtod(lowbatts, NULL);

    im = InitImage();

    step = (int)(upsrunt + 4) / 5;
    if (step <= 0)
       step = 1;		   /* make sure we have a positive step */
    DrawText(im, 0, step);

    maxt = step * 5;
    uoutpos = 300 - (int)(upsrunt * 300 ) / maxt;
    lowbattpos = 300 - (int)(lowbatt * 300) / maxt;

    gdImageFilledRectangle(im, 50, lowbattpos, 150, 300, red);

    gdImageFilledRectangle(im, 75, uoutpos, 125, 300, black);

    sprintf(utiltxt, "%.1f mins", upsrunt);
    gdImageString(im, gdFontLarge, 65, 320, (unsigned char *)utiltxt, black); 
 
    TermImage(im);
}


int main ()
{
    if (!extractcgiargs()) {
        printf("Content-type: text/plain\n\n");
        printf("Unable to extract cgi arguments!\n");
	exit(0);
    }


    if (!strcmp(cmd, "upsload")) {
	drawupsload(upsval);
	exit(0);
    }

    if (!strcmp(cmd, "battcap")) {
	drawbattcap(upsval, upsval2);
	exit(0);
    }

    if (!strcmp(cmd, "battvolt")) {
	drawbattvolt(upsval, upsval2);
	exit(0);
    }

    if (!strcmp(cmd, "utility")) {
	drawutility(upsval,upsval2,upsval3);
	exit(0);
    }

    if (!strcmp(cmd, "outputv")) {
	drawupsout(upsval);
	exit(0);
    }

    if (!strcmp(cmd, "runtime")) {
	drawruntime(upsval, upsval2);
	exit(0);
    }

    printf ("Content-type: text/plain\n\n");
    printf ("Invalid request!\n");

    exit(0);
}
