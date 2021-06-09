/************************************************************************/
/*																		*/
/*	video_demo.c	--	ZYBO Video demonstration 						*/
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

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "video_demo.h"
#include "display_ctrl/display_ctrl.h"
#include "intc/intc.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR 		XPAR_AXI_DYNCLK_0_BASEADDR
#define VDMA_ID 				XPAR_AXIVDMA_0_DEVICE_ID
#define HDMI_OUT_VTC_ID 		XPAR_V_TC_OUT_DEVICE_ID
#define SCU_TIMER_ID 			XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR 			XPAR_PS7_UART_1_BASEADDR

/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

/*
 * Display and Video Driver structs
 */
DisplayCtrl dispCtrl;
XAxiVdma vdma;
INTC intc;
char fRefresh; //flag used to trigger a refresh of the Menu on video detect

/*
 * Framebuffers for video data
 */
u8 frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; //array of pointers to the frame buffers

/*
 * grid for Connect Four
 */
struct Block grid[GRID_ROWS][GRID_COLUMNS];

/* ------------------------------------------------------------ */
/*				Procedure Definitions							*/
/* ------------------------------------------------------------ */

int main(void)
{
	DemoInitialize();

	DemoRun();

	return 0;
}


void DemoInitialize()
{
	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}

	/*
	 * Initialize a timer used for a simple delay
	 */
	TimerInitialize(SCU_TIMER_ID);

	/*
	 * Initialize VDMA driver
	 */
	vdmaConfig = XAxiVdma_LookupConfig(VDMA_ID);
	if (!vdmaConfig)
	{
		xil_printf("No video DMA found for ID %d\r\n", VDMA_ID);
		return;
	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		xil_printf("VDMA Configuration Initialization failed %d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Display controller and start it
	 */
	Status = DisplayInitialize(&dispCtrl, &vdma, HDMI_OUT_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}

	/*
	 * Initialize the Interrupt controller and start it.
	 */
	Status = fnInitInterruptController(&intc);
	if(Status != XST_SUCCESS) {
		xil_printf("Error initializing interrupts");
		return;
	}
	//fnEnableInterrupts(&intc, &ivt[0], sizeof(ivt)/sizeof(ivt[0]));

	/*
	 * Initialize the Video Capture device
	 */
	//Status = VideoInitialize(&videoCapt, &intc, &vdma, HDMI_IN_GPIO_ID, HDMI_IN_VTC_ID, HDMI_IN_VTC_IRPT_ID, pFrames, DEMO_STRIDE, DEMO_START_ON_DET);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Video Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}

	/*
	 * Initialize the grid
	 */
	int current_color = WHITE;
	for (int i = 0; i < GRID_ROWS; i++) {
		for (int j = 0; j < GRID_COLUMNS; j++) {
			current_color = (current_color == WHITE) ? BLACK : WHITE;

			grid[i][j].type = current_color;
			grid[i][j].width = (3 * dispCtrl.vMode.width) / GRID_COLUMNS;
			grid[i][j].height = dispCtrl.vMode.height / GRID_ROWS;

			grid[i][j].y = i * grid[i][j].height;
			grid[i][j].x = j * grid[i][j].width;
		}
	}

	// draw the connect four board
	u32 xcoi, ycoi;
	u32 iPixelAddr;
	u32 width = dispCtrl.vMode.width;
	u32 height = dispCtrl.vMode.height;
	u32 stride = DEMO_STRIDE;
	u8* frame = pFrames[dispCtrl.curFrame];

	// draw the board
	for (xcoi = 0; xcoi < (width * 3); xcoi += 3) {
		iPixelAddr = xcoi;

		for (ycoi = 0; ycoi < height; ycoi++) {
			for (int i = 0; i < GRID_ROWS; i++) {
				for (int j = 0; j < GRID_COLUMNS; j++) {
					if (xcoi > grid[i][j].x
							&& xcoi < grid[i][j].x + grid[i][j].width
							&& ycoi > grid[i][j].y
							&& ycoi < grid[i][j].y + grid[i][j].height) {
						if (grid[i][j].type == WHITE) {
							frame[iPixelAddr] = 255;			// blue
							frame[iPixelAddr + 1] = 255;		// green
							frame[iPixelAddr + 2] = 255;	// red
						} else if (grid[i][j].type == BLACK) {
							frame[iPixelAddr] = 0;			// blue
							frame[iPixelAddr + 1] = 0;		// green
							frame[iPixelAddr + 2] = 0;	// red
						} else if (grid[i][j].type == RED) {
							frame[iPixelAddr] = 0;			// blue
							frame[iPixelAddr + 1] = 0;		// green
							frame[iPixelAddr + 2] = 255;	// red
						} else {
							frame[iPixelAddr] = 255;			// blue
							frame[iPixelAddr + 1] = 0;		// green
							frame[iPixelAddr + 2] = 0;	// red
						}
					}
				}
			}

			/*
			 * This pattern is printed one vertical line at a time, so the address must be incremented
			 * by the stride instead of just 1.
			 */
			iPixelAddr += stride;
		}
	}

	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

	return;
}


void DemoRun()
{
	int nextFrame = 0;
	char userInput = 0;

	/* Flush UART FIFO */

	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (userInput != 'q')
	{
		fRefresh = 0;
		DemoPrintMenu();

		/*
		 *  Wait for data on UART
		 */
		while (!XUartPs_IsReceiveData(UART_BASEADDR) && !fRefresh)
		{}

		/*
		 * Store the first character in the UART receive FIFO and echo it *
		 */
		if (XUartPs_IsReceiveData(UART_BASEADDR))
		{
			userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
			xil_printf("%c", userInput);
		}
		else  //Refresh triggered by video detect interrupt
		{
			userInput = 'r';
		}

		switch (userInput) {
		case '1':
			// connect four
			xil_printf("\n\rPlaying Connect Four\r\n");

			// reset the board
			int current_color = WHITE;

			for (int i = 0; i < GRID_ROWS; i++) {
				for (int j = 0; j < GRID_COLUMNS; j++) {
					current_color = (current_color == WHITE) ? BLACK : WHITE;
					grid[i][j].type = current_color;
				}
			}

			// play the game
			play_connect_four(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
			break;
		case '2':
			DemoChangeRes();
		case 'q':
			break;
		case 'r':
			break;
		default:
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
	}


	return;
}

void DemoPrintMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*                ZYBO Video Demo                 *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Display Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Display Pixel Clock Freq. (MHz): %15.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("*Display Frame Index: %27d*\n\r", dispCtrl.curFrame);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");

	xil_printf("1 - Connect Four\n\r");
	xil_printf("2 - Change Resolution\n\r");
	xil_printf("q - Quit\n\r");
	xil_printf("\n\r");
	xil_printf("\n\r");
	xil_printf("Enter a selection:");
}

void DemoChangeRes()
{
	int fResSet = 0;
	int status;
	char userInput = 0;

	/* Flush UART FIFO */
	while (XUartPs_IsReceiveData(UART_BASEADDR))
	{
		XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
	}

	while (!fResSet)
	{
		DemoCRMenu();

		/* Wait for data on UART */
		while (!XUartPs_IsReceiveData(UART_BASEADDR))
		{}

		/* Store the first character in the UART recieve FIFO and echo it */
		userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
		xil_printf("%c", userInput);
		status = XST_SUCCESS;

		switch (userInput)
		{
		case '1':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_640x480);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '2':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_800x600);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '3':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x720);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '4':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x1024);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '5':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1600x900);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case '6':
			status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
			DisplayStart(&dispCtrl);
			fResSet = 1;
			break;
		case 'q':
			fResSet = 1;
			break;
		default :
			xil_printf("\n\rInvalid Selection");
			TimerDelay(500000);
		}
		if (status == XST_DMA_ERROR)
		{
			xil_printf("\n\rWARNING: AXI VDMA Error detected and cleared\n\r");
		}
	}
}

void DemoCRMenu()
{
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("**************************************************\n\r");
	xil_printf("*                ZYBO Video Demo                 *\n\r");
	xil_printf("**************************************************\n\r");
	xil_printf("*Current Resolution: %28s*\n\r", dispCtrl.vMode.label);
	printf("*Pixel Clock Freq. (MHz): %23.3f*\n\r", dispCtrl.pxlFreq);
	xil_printf("**************************************************\n\r");
	xil_printf("\n\r");
	xil_printf("1 - %s\n\r", VMODE_640x480.label);
	xil_printf("2 - %s\n\r", VMODE_800x600.label);
	xil_printf("3 - %s\n\r", VMODE_1280x720.label);
	xil_printf("4 - %s\n\r", VMODE_1280x1024.label);
	xil_printf("5 - %s\n\r", VMODE_1600x900.label);
	xil_printf("6 - %s\n\r", VMODE_1920x1080.label);
	xil_printf("q - Quit (don't change resolution)\n\r");
	xil_printf("\n\r");
	xil_printf("Select a new resolution:");
}

/*
 * Bilinear interpolation algorithm. Assumes both frames have the same stride.
 */
void DemoScaleFrame(u8 *srcFrame, u8 *destFrame, u32 srcWidth, u32 srcHeight, u32 destWidth, u32 destHeight, u32 stride)
{
	float xInc, yInc; // Width/height of a destination frame pixel in the source frame coordinate system
	float xcoSrc, ycoSrc; // Location of the destination pixel being operated on in the source frame coordinate system
	float x1y1, x2y1, x1y2, x2y2; //Used to store the color data of the four nearest source pixels to the destination pixel
	int ix1y1, ix2y1, ix1y2, ix2y2; //indexes into the source frame for the four nearest source pixels to the destination pixel
	float xDist, yDist; //distances between destination pixel and x1y1 source pixels in source frame coordinate system

	int xcoDest, ycoDest; // Location of the destination pixel being operated on in the destination coordinate system
	int iy1; //Used to store the index of the first source pixel in the line with y1
	int iDest; //index of the pixel data in the destination frame being operated on

	int i;

	xInc = ((float) srcWidth - 1.0) / ((float) destWidth);
	yInc = ((float) srcHeight - 1.0) / ((float) destHeight);

	ycoSrc = 0.0;
	for (ycoDest = 0; ycoDest < destHeight; ycoDest++)
	{
		iy1 = ((int) ycoSrc) * stride;
		yDist = ycoSrc - ((float) ((int) ycoSrc));

		/*
		 * Save some cycles in the loop below by presetting the destination
		 * index to the first pixel in the current line
		 */
		iDest = ycoDest * stride;

		xcoSrc = 0.0;
		for (xcoDest = 0; xcoDest < destWidth; xcoDest++)
		{
			ix1y1 = iy1 + ((int) xcoSrc) * 3;
			ix2y1 = ix1y1 + 3;
			ix1y2 = ix1y1 + stride;
			ix2y2 = ix1y1 + stride + 3;

			xDist = xcoSrc - ((float) ((int) xcoSrc));

			/*
			 * For loop handles all three colors
			 */
			for (i = 0; i < 3; i++)
			{
				x1y1 = (float) srcFrame[ix1y1 + i];
				x2y1 = (float) srcFrame[ix2y1 + i];
				x1y2 = (float) srcFrame[ix1y2 + i];
				x2y2 = (float) srcFrame[ix2y2 + i];

				/*
				 * Bilinear interpolation function
				 */
				destFrame[iDest] = (u8) ((1.0-yDist)*((1.0-xDist)*x1y1+xDist*x2y1) + yDist*((1.0-xDist)*x1y2+xDist*x2y2));
				iDest++;
			}
			xcoSrc += xInc;
		}
		ycoSrc += yInc;
	}

	/*
	 * Flush the framebuffer memory range to ensure changes are written to the
	 * actual memory, and therefore accessible by the VDMA.
	 */
	Xil_DCacheFlushRange((unsigned int) destFrame, DEMO_MAX_FRAME);

	return;
}

void DemoISR(void *callBackRef, void *pVideo)
{
	char *data = (char *) callBackRef;
	*data = 1; //set fRefresh to 1
}

void play_connect_four(u8 *frame, u32 width, u32 height, u32 stride) {
	u32 xcoi, ycoi;
	u32 iPixelAddr;
	char userInput;
	int fRefresh;
	int current_type = RED;
	int winner = 0;

	while (userInput != 'q' && !winner) {
		print_connect_four_menu();

		fRefresh = 0;
		/*
		 *  Wait for data on UART
		 */
		while (!XUartPs_IsReceiveData(UART_BASEADDR) && !fRefresh) {
		}

		/*
		 * Store the first character in the UART receive FIFO and echo it *
		 */
		if (XUartPs_IsReceiveData(UART_BASEADDR)) {
			userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
			xil_printf("%c", userInput);
		} else  //Refresh triggered by video detect interrupt
		{
			userInput = 'r';
		}

		// switch between RED and BLUE players every turn
		current_type = (current_type == RED) ? BLUE : RED;

		switch (userInput) {
		case '1':
			winner = place_block(1, current_type);
			break;
		case '2':
			winner = place_block(2, current_type);
			break;
		case '3':
			winner = place_block(3, current_type);
			break;
		case '4':
			winner = place_block(4, current_type);
			break;
		case '5':
			winner = place_block(5, current_type);
			break;
		case '6':
			winner = place_block(6, current_type);
			break;
		case '7':
			winner = place_block(7, current_type);
			break;
		case 'q':
			break;
		default:
			xil_printf("\n\rPlease choose a valid column or quit\n\r");
			break;
		}

		// draw the board
		for (xcoi = 0; xcoi < (width * 3); xcoi += 3) {
			iPixelAddr = xcoi;

			for (ycoi = 0; ycoi < height; ycoi++) {
				for (int i = 0; i < GRID_ROWS; i++) {
					for (int j = 0; j < GRID_COLUMNS; j++) {
						if (xcoi > grid[i][j].x
								&& xcoi < grid[i][j].x + grid[i][j].width
								&& ycoi > grid[i][j].y
								&& ycoi < grid[i][j].y + grid[i][j].height) {
							if (grid[i][j].type == WHITE) {
								frame[iPixelAddr] = 255;			// blue
								frame[iPixelAddr + 1] = 255;		// green
								frame[iPixelAddr + 2] = 255;		// red
							} else if (grid[i][j].type == BLACK) {
								frame[iPixelAddr] = 0;			// blue
								frame[iPixelAddr + 1] = 0;		// green
								frame[iPixelAddr + 2] = 0;	// red
							} else if (grid[i][j].type == RED) {
								frame[iPixelAddr] = 0;			// blue
								frame[iPixelAddr + 1] = 0;		// green
								frame[iPixelAddr + 2] = 255;	// red
							} else {
								frame[iPixelAddr] = 255;			// blue
								frame[iPixelAddr + 1] = 0;		// green
								frame[iPixelAddr + 2] = 0;	// red
							}
						}
					}
				}

				/*
				 * This pattern is printed one vertical line at a time, so the address must be incremented
				 * by the stride instead of just 1.
				 */
				iPixelAddr += stride;
			}
		}

		/*
		 * Flush the framebuffer memory range to ensure changes are written to the
		 * actual memory, and therefore accessible by the VDMA.
		 */
		Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	}

	if (winner) {
		// if a player has won
		if (winner == 1) {
			// print out a victory message then wait for the user to return to menu
			char* victor = (current_type == RED) ? "Red" : "Blue";

			xil_printf("\n\rCongratulations %s player, you have won!\n\r",
					victor);
		}
		// if the game ended in a tie
		else if (winner == 2) {
			xil_printf("\n\rThe game ended in a tie, no more places remained to place blocks\n\r");
		}

		xil_printf("\n\rPress any key to return to menu...\n\r");

		/*
		 *  Wait for data on UART
		 */
		while (!XUartPs_IsReceiveData(UART_BASEADDR) && !fRefresh) {
		}

		/*
		 * Store the first character in the UART receive FIFO and echo it *
		 */
		if (XUartPs_IsReceiveData(UART_BASEADDR)) {
			userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);
			xil_printf("%c", userInput);
		} else  //Refresh triggered by video detect interrupt
		{
			userInput = 'r';
		}
	}
}

void print_connect_four_menu() {
	xil_printf("\x1B[H"); //Set cursor to top left of terminal
	xil_printf("\x1B[2J"); //Clear terminal
	xil_printf("\n\rPlace block at column # (1-7)\n\r");
	xil_printf("q - Quit\n\r");
	xil_printf("\n\r");
	xil_printf("\n\r");
	xil_printf("Enter a selection: ");
}

int place_block(int x, int type) {
	int i = 0;

	// if the column is not full
	if (grid[0][x - 1].type == WHITE || grid[0][x - 1].type == BLACK) {
		// check that the next block down is neutral and, if so, move down to it
		while ((grid[i + 1][x - 1].type == WHITE || grid[i + 1][x - 1].type == BLACK) && i != GRID_ROWS - 1)  {
			i++;
		}

		set_at(x - 1, i, type);
	} else {
		xil_printf("\n\rPlease choose a valid location\n\r");
	}

	return check_winner(x - 1, i, type);
}

void set_at(int x, int y, int type) {
	grid[y][x].type = type;
}

int check_winner(int x, int drop_coord, int type) {
	// initialize winner to a tie
	int winner = 2;

	// check for tie
	for (int i = 0; i < GRID_ROWS; i++) {
		for (int j = 0; j < GRID_COLUMNS; j++) {
			if (grid[i][j].type == WHITE || grid[i][j].type == BLACK) {
				winner = 0;
				break;
			}
		}
	}

	// check for horizontal win
	if (!winner) {
		for (int i = 0; i < GRID_ROWS; i++) {
			for (int j = 0; j < GRID_COLUMNS; j++) {
				if ((grid[i][j].type != WHITE && grid[i][j].type != BLACK)
						&& grid[i][j].type == grid[i][j + 1].type
						&& grid[i][j + 1].type == grid[i][j + 2].type
						&& grid[i][j + 2].type == grid[i][j + 3].type) {
					winner = 1;
					break;
				}
			}
		}
	}

	// check for vertical win
	if (!winner) {
		for (int i = 0; i < GRID_ROWS; i++) {
			for (int j = 0; j < GRID_COLUMNS; j++) {
				if ((grid[i][j].type != WHITE && grid[i][j].type != BLACK)
						&& grid[i][j].type == grid[i + 1][j].type
						&& grid[i + 1][j].type == grid[i + 2][j].type
						&& grid[i + 2][j].type == grid[i + 3][j].type) {
					winner = 1;
					break;
				}
			}
		}
	}

	// check for diagonal win
	if (!winner) {
		int x1 = x;
		int y1 = drop_coord;
		int count = 0;
		// denotes the stage of the process it is on. 1-4 stages, 5 to denote done
		int stage = 1;

		while (stage != 5) {
			if (count == 4) {
				stage = 5;
				winner = 1;
			} else if (stage == 1 && x1 != -1 && y1 != -1
					&& grid[y1][x1].type == type) {
				count++;
				x1--;
				y1--;
			} else if (stage == 1) {
				stage = 2;
				x1 = x + 1;
				y1 = drop_coord + 1;
			} else if (stage == 2 && x1 != GRID_COLUMNS
					&& y1 != GRID_ROWS && grid[y1][x1].type == type) {
				count++;
				x1++;
				y1++;
			} else if (stage == 2) {
				stage = 3;
				count = 0;
				x1 = x;
				y1 = drop_coord;
			} else if (stage == 3 && x1 != -1 && y1 != GRID_ROWS
					&& grid[y1][x1].type == type) {
				count++;
				x1--;
				y1++;
			} else if (stage == 3) {
				stage = 4;
				x1--;
				y1++;
			} else if (stage == 4 && x1 != GRID_COLUMNS && y1 != -1
					&& grid[y1][x1].type == type) {
				count++;
				x1++;
				y1--;
			} else if (stage == 4) {
				stage = 5;
			}
		}
	}

	return winner;
}
