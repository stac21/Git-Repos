/************************************************************************/
/*																		*/
/*	video_demo.h	--	ZYBO Video demonstration 						*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2015, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This file contains code for running a demonstration of the		*/
/*		Video input and output capabilities on the ZYBO. It is a good	*/
/*		example of how to properly use the display_ctrl and				*/
/*		video_capture drivers.											*/
/*																		*/
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		11/25/2015(SamB): Created										*/
/*																		*/
/************************************************************************/

#ifndef VIDEO_DEMO_H_
#define VIDEO_DEMO_H_

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "xil_types.h"

/* ------------------------------------------------------------ */
/*					Miscellaneous Declarations					*/
/* ------------------------------------------------------------ */

#define DEMO_PATTERN_0 0
#define DEMO_PATTERN_1 1
#define DEMO_PATTERN_2 2
#define DEMO_PATTERN_3 3

#define DEMO_MAX_FRAME (1920*1080*3)
#define DEMO_STRIDE (1920 * 3)

#define GRID_COLUMNS 7
#define GRID_ROWS 6

#define WHITE 0
#define BLACK 1
#define RED 2
#define BLUE 3

/*
 * Configure the Video capture driver to start streaming on signal
 * detection
 */
#define DEMO_START_ON_DET 1

/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

struct Block {
	// 0 for white (neutral), 1 for black (neutral), 2 for red, 3 for blue
	int type;
	// top left x coord
	int x;
	// top left y coord
	int y;
	int width;
	int height;
};

void DemoInitialize();
void DemoRun();
void DemoPrintMenu();
void DemoChangeRes();
void DemoCRMenu();
void DemoInvertFrame(u8 *srcFrame, u8 *destFrame, u32 width, u32 height, u32 stride);
void DemoPrintTest(u8 *frame, u32 width, u32 height, u32 stride, int pattern);
void DemoScaleFrame(u8 *srcFrame, u8 *destFrame, u32 srcWidth, u32 srcHeight, u32 destWidth, u32 destHeight, u32 stride);
void DemoISR(void *callBackRef, void *pVideo);
/**
 * plays connect four until one of the players wins.
 * Returns 1 if red player won, 2 if blue player won, 0 if the game is still going
 */
void play_connect_four(u8 *frame, u32 width, u32 height, u32 stride);

/*
 * prints the connect four game menu
 */
void print_connect_four_menu();

/*
 * brings the block down to the desired spot and places it then calls check_winner
 * and returns its value
 */
int place_block(int x, int color);

/*
 * sets the block at the (x, y) coordinates in the grid to the specified color
 */
void set_at(int x, int y, int color);

/*
 * checks to see if anyone has won the game
 * returns 0 for no, 1 for yes, 2 for tie
 */
int check_winner();

/* ------------------------------------------------------------ */

/************************************************************************/

#endif /* VIDEO_DEMO_H_ */
