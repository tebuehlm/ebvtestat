/* Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty. This file is offered as-is,
 * without any warranty.
 */
/*! @file process_frame.c
 * @brief Contains the actual algorithm and calculations.
 */
/* Definitions specific to this application. Also includes the Oscar main header file. */
#include "template.h"

#include <string.h>
#include <stdlib.h>
#define IMG_SIZE NUM_COLORS*(OSC_CAM_MAX_IMAGE_WIDTH/2)*(OSC_CAM_MAX_IMAGE_HEIGHT/2)
const int nc = OSC_CAM_MAX_IMAGE_WIDTH / 2;
const int nr = OSC_CAM_MAX_IMAGE_HEIGHT / 2;
int avgDxy[5][IMG_SIZE];
int helpBuf[IMG_SIZE];
int Border = 6;
int k = 5;
int TextColor;
void ResetProcess() {
//called when "reset" button is pressed
	if (TextColor == CYAN)
		TextColor = MAGENTA;
	else
		TextColor = CYAN;
}
void ProcessFrame() {
	uint32 t1, t2;
	char Text[] = "hallo world";

	if (data.ipc.state.nStepCounter == 1) {

		memset(data.u8TempImage[THRESHOLD], 0, IMG_SIZE);
		TextColor = CYAN;
	} else {
		/* //example for time measurement
		 t1 = OscSupCycGet();
		 //example for copying sensor image to background image
		 memcpy(data.u8TempImage[BACKGROUND], data.u8TempImage[SENSORIMG], IMG_SIZE);
		 //example for time measurement
		 t2 = OscSupCycGet();
		 //example for log output to console
		 OscLog(INFO, "required = %d us\n", OscSupCycToMicroSecs(t2-t1));
		 //example for drawing output
		 //draw line
		 DrawLine(10, 100, 200, 20, RED);
		 //draw open rectangle
		 DrawBoundingBox(20, 10, 50, 40, false, GREEN);
		 //draw filled rectangle
		 DrawBoundingBox(80, 100, 110, 120, true, BLUE);
		 DrawString(200, 200, strlen(Text), TINY, TextColor, Text);*/
		CalcDeriv();
		AvgDeriv(0);
		AvgDeriv(1);
		AvgDeriv(2);
		EdgeStrength();
		MaxFinder();
	}
}
void CalcDeriv() {
	int c, r;
	for (r = nc; r < nr * nc; r += nc) {
		for (c = 1; c < nc - 1; c++) {
			unsigned char* p = &data.u8TempImage[SENSORIMG][r + c];

			int dx = -(int) *(p - nc - 1) + (int) *(p - nc + 1)
					- 2 * (int) *(p - 1) + 2 * (int) *(p + 1)
					- (int) *(p + nc - 1) + (int) *(p + nc + 1);
			int dy = -(int) *(p - nc - 1) - 2 * (int) *(p - nc)
					- (int) *(p - nc + 1) + (int) *(p + nc - 1)
					+ 2 * (int) *(p - nc) + (int) *(p + nc + 1);
			avgDxy[0][r + c] = dx * dx;
			avgDxy[1][r + c] = dy * dy;
			avgDxy[2][r + c] = dx * dy;
//data.u8TempImage[BACKGROUND][r+c]=(uint8)MIN(255,MAX(0,128+(dx >> 2)));
//data.u8TempImage[THRESHOLD][r+c]=(uint8)MIN(255,MAX(0,128+(dy >> 2)));
			data.u8TempImage[BACKGROUND][r + c] = (uint8) MIN(255,
					MAX(0,((avgDxy[0][r+c]) >> 10)));

		}
	}
}
void AvgDeriv(int Index) {
//int helpBuff[3][IMG_SIZE];

	int c, r;
	for (r = nc; r < nr * nc - nc; r += nc) {
		for (c = Border + 1; c < nc - (Border + 1); c++) {

			int* p = &avgDxy[Index][r + c];
//
			int sx = (*(p - 6) + *(p + 6)) + ((*(p - 5) + *(p + 5)) << 2)
					+ ((*(p - 4) + *(p + 4)) << 3)
					+ ((*(p - 3) + *(p + 3)) << 5)
					+ ((*(p - 2) + *(p + 2)) << 6)
					+ ((*(p - 1) + *(p + 1)) << 6) + (*p << 7);
//now averaged
			helpBuf[r + c] = (sx >> 8);
		}
	}

	for (r = nc * (Border + 1); r < nr * nc - nc * (Border + 1); r += nc) {
		for (c = Border + 1; c < nc - (Border + 1); c++) {

			int* p = &helpBuf[r + c];
//
			int sy = (*(p - 6 * nc) + *(p + 6 * nc))
					+ ((*(p - 5 * nc) + *(p + 5 * nc)) << 2)
					+ ((*(p - 4 * nc) + *(p + 4 * nc)) << 3)
					+ ((*(p - 3 * nc) + *(p + 3 * nc)) << 5)
					+ ((*(p - 2 * nc) + *(p + 2 * nc)) << 6)
					+ ((*(p - 1 * nc) + *(p + 1 * nc)) << 6) + (*p << 7);

			avgDxy[Index][r + c] = (sy >> 8);

		}
	}
}
void EdgeStrength() {
	int c, r;
	for (r = nc * (Border + 1); r < nr * nc - nc * (Border + 1); r += nc) {
		for (c = Border + 1; c < nc - (Border + 1); c++) {
			int dIx2 = (avgDxy[0][r + c] >> 7);
			int dIy2 = (avgDxy[1][r + c] >> 7);
			int dIxy = (avgDxy[2][r + c] >> 7);
			avgDxy[3][r + c] = (dIx2 * dIy2 - dIxy * dIxy)
					- ((k * (dIx2 + dIy2) * (dIx2 + dIy2)) >> 7);
			data.u8TempImage[THRESHOLD][r + c] = (uint8) MIN(255,
					MAX(0,avgDxy[3][r+c]>>7));
		}
	}
}
void MaxFinder() {
	memset(helpBuf, 0, IMG_SIZE * sizeof(int));
	memset(avgDxy[4], 0, IMG_SIZE * sizeof(int));
	int c, r;
	int maxGlobal = 0;
	int threshold = 0;
	int SizeBox = 4;
	for (int i = 0; i < IMG_SIZE; i++) {
		maxGlobal = MAX(maxGlobal, avgDxy[3][i]);
	}
	threshold = (maxGlobal * data.ipc.state.nThreshold) / 100;

	for (r = nc * (Border + 1); r < nr * nc - nc * (Border + 1); r += nc) {
		for (c = Border + 1; c < nc - (Border + 1); c++) {

			int* p = &avgDxy[3][r + c];

			for (int xi = -6; xi <= 6; xi++) {
				helpBuf[r + c] = MAX(*(p + xi), helpBuf[r + c]);
			}
		}
	}

	for (r = nc * (Border + 1); r < nr * nc - nc * (Border + 1); r += nc) {
		for (c = Border + 1; c < nc - (Border + 1); c++) {

			int* p = &helpBuf[r + c];
			for (int i = -6 * nc; i <= 6 * nc; i += nc) {
				avgDxy[4][r + c] = MAX(*(p + i), avgDxy[4][r + c]);
			}
			if (avgDxy[4][r + c] == avgDxy[3][r + c]
					&& avgDxy[3][r + c] > threshold) {
				DrawBoundingBox(c - SizeBox, r / nc + SizeBox, c + SizeBox,
						r / nc - SizeBox, 0, GREEN);
			}
			data.u8TempImage[BACKGROUND][r + c] = (uint8) MIN(255,
					MAX(0,((avgDxy[4][r+c])>>7)));
		}
	}
}
