#ifndef _ImgGaugeFrame2_h
#define _ImgGaugeFrame2_h

#include <Arduino.h>

#define ImgGaugeFrame2_spec { ImgGaugeFrame2_width, ImgGaugeFrame2_height, ImgGaugeFrame2_data }
#define ImgGaugeFrame2_rect(vx,vy) {.x = vx, .y = vy, .width = ImgGaugeFrame2_width, .height = ImgGaugeFrame2_height }
#define ImgGaugeFrame2_info(vx,vy) ImgGaugeFrame2_rect(vx,vy), ImgGaugeFrame2_data
const uint32_t ImgGaugeFrame2_width = 55;
const uint32_t ImgGaugeFrame2_height = 29;
const uint8_t ImgGaugeFrame2_data[(56*29)/2] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCE, 0x9A, 0x88, 0x98, 0xCA, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAE, 0x47, 0x43, 0x44, 0x34, 0x63, 0x45, 0x43, 0x96, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x8B, 0x56, 0x34, 0x43, 0x54, 0x26, 0xA1, 0x89, 0x67, 0x66, 0x86, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9C, 0x25, 0x42, 0x86, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0x9C, 0x34, 0x95, 0xEC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAE, 0x23, 0x74, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xDF, 0x28, 0xA3, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x25, 0x74, 0x88, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x29, 0xB4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x58, 0x54, 0x87, 0x88, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAE, 0x46, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xFF, 0x9E, 0x44, 0x86, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6C, 0x94, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0xEF, 0x19, 0x86, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x37, 0xE9, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0x9F, 0x01, 0x87, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x39, 0xF3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8F, 0x60, 0xFF, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xFF, 0x49, 0x96, 0x60, 0x89, 0x88, 0x88, 0x88, 0x88, 0x88, 0x56, 0x15, 0xA1, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0x52, 0xF8, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0xCF, 0x54, 0xBA, 0x27, 0x95, 0x88, 0x88, 0x88, 0x47, 0x22, 0x43, 0x45, 0x64, 0x45, 0x96, 0xFD, 0xFF, 0xFF, 0xFF, 0xDF, 0x94, 0x7B, 0xC4, 0xFF, 0xFF, 0x0F, 
	0xFF, 0xFF, 0x5E, 0x84, 0xBC, 0x6C, 0x53, 0x89, 0x88, 0x57, 0x44, 0x65, 0x98, 0xBA, 0x9B, 0x58, 0x55, 0xA7, 0xFD, 0xFF, 0xFF, 0x7C, 0xF5, 0xCF, 0x55, 0xFE, 0xFF, 0x0F, 
	0xFF, 0xFF, 0x1A, 0xB8, 0xBB, 0xDB, 0x16, 0x87, 0x68, 0x34, 0x94, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0x9E, 0x24, 0xC7, 0xFF, 0xDF, 0x46, 0xFF, 0xFF, 0x2B, 0xFA, 0xFF, 0x0F, 
	0xFF, 0xCF, 0x63, 0xBA, 0xBB, 0xBB, 0x8C, 0x80, 0x26, 0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x33, 0xFA, 0x3F, 0xF6, 0xFF, 0xFF, 0x8E, 0xC4, 0xFF, 0x0F, 
	0xFF, 0x9E, 0xA2, 0xBB, 0xBB, 0xBB, 0xBB, 0x19, 0x60, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6C, 0x83, 0x92, 0xFE, 0xFF, 0xFF, 0xDF, 0x92, 0xFE, 0x0F, 
	0xFF, 0x5B, 0xB5, 0xBB, 0xBB, 0xBB, 0xBB, 0x08, 0xC6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x16, 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0x67, 0xFB, 0x0F, 
	0xFF, 0x38, 0xB9, 0xBB, 0xBB, 0xBB, 0x9B, 0x53, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3D, 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0x4D, 0xF7, 0x0F, 
	0xEF, 0x36, 0xBC, 0xBB, 0xBB, 0xBB, 0x5A, 0xD2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0x72, 0xFD, 0xFF, 0xFF, 0xFF, 0x4F, 0xE6, 0x0F, 
	0xAF, 0x65, 0xBB, 0xBB, 0xBB, 0xCB, 0x47, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3A, 0xF8, 0xFF, 0xFF, 0xFF, 0x8F, 0xA6, 0x0F, 
	0x7F, 0x85, 0xBB, 0xBB, 0xBB, 0xAB, 0x54, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0xD6, 0xFF, 0xFF, 0xFF, 0xCF, 0x76, 0x0F, 
	0x4F, 0xB4, 0xBB, 0xBB, 0xBB, 0x7B, 0x95, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0x95, 0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0x0F, 
	0x3F, 0xC5, 0xBB, 0xBB, 0xBB, 0x4B, 0xD6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0x56, 0xFF, 0xFF, 0xFF, 0xFF, 0x37, 0x0F, 
	0x4D, 0xC6, 0xBB, 0xBB, 0xBB, 0x2B, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x37, 0xFF, 0xFF, 0xFF, 0xFF, 0x49, 0x0C, 
	0x4A, 0xC7, 0xBB, 0xBB, 0xBB, 0x39, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x49, 0xFC, 0xFF, 0xFF, 0xFF, 0x5A, 0x0A, 
	0x59, 0xC7, 0xBB, 0xBB, 0xBB, 0x57, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5A, 0xFA, 0xFF, 0xFF, 0xFF, 0x6B, 0x09, 
	0x69, 0xC8, 0xCC, 0xCC, 0xCC, 0x67, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7B, 0xF9, 0xFF, 0xFF, 0xFF, 0x7C, 0x09, 
	0x46, 0x96, 0x88, 0x88, 0x88, 0x44, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5B, 0xB6, 0xBB, 0xBB, 0xBB, 0x58, 0x06, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	};

#endif
