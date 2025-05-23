#ifndef _ImgAIQ_h
#define _ImgAIQ_h

#include <Arduino.h>

#define ImgAIQ_spec { ImgAIQ_width, ImgAIQ_height, ImgAIQ_data }
#define ImgAIQ_rect(vx,vy) {.x = vx, .y = vy, .width = ImgAIQ_width, .height = ImgAIQ_height }
#define ImgAIQ_info(vx,vy) ImgAIQ_rect(vx,vy), ImgAIQ_data
const uint32_t ImgAIQ_width = 40;
const uint32_t ImgAIQ_height = 40;
const uint8_t ImgAIQ_data[(40*40)/2] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0xCC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0x79, 0x56, 0x44, 0x76, 0xD9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7C, 0x02, 0x00, 0x00, 0x00, 0x72, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0x02, 0x00, 0x31, 0x34, 0x00, 0x00, 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA4, 0xDC, 0xDD, 0xAC, 0x06, 0x20, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x04, 0x61, 0xFB, 0xFF, 0xFF, 0xFF, 0x7C, 0x01, 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6E, 0x01, 0xD5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x70, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1C, 0x30, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5C, 0x00, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x0A, 0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9F, 0x01, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0x06, 0x72, 0xDC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x02, 0xC6, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xEF, 0x8C, 0x02, 0x20, 0x63, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x04, 0x72, 0xEC, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0x4B, 0x01, 0x00, 0x00, 0x20, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x03, 0x00, 0x31, 0xF9, 0xFF, 0xFF, 
	0xFF, 0xFF, 0x8C, 0x04, 0x10, 0x43, 0x65, 0x86, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x4A, 0x13, 0x00, 0x72, 0xFC, 0xFF, 
	0xFF, 0xEF, 0x16, 0x10, 0x74, 0xAA, 0xDC, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0x89, 0x15, 0x00, 0xD6, 0xFF, 
	0xFF, 0x7E, 0x01, 0x82, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x8F, 0x02, 0x80, 0xFE, 
	0xFF, 0x1A, 0x40, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAF, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x4A, 0x10, 0xF8, 
	0xCF, 0x06, 0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x52, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xAE, 0x01, 0xD4, 
	0x8F, 0x01, 0xD6, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x10, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x05, 0x82, 
	0x6F, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2B, 0x40, 
	0x5F, 0x20, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xCF, 0x58, 0xF6, 0xFF, 0xFF, 0xFF, 0x3E, 0x10, 
	0x4D, 0x30, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xDC, 0x6A, 0x01, 0xE1, 0xFF, 0xFF, 0xFF, 0x4F, 0x00, 
	0x4C, 0x40, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0x58, 0x00, 0x10, 0xD4, 0xFF, 0xFF, 0xFF, 0x4F, 0x00, 
	0x4D, 0x30, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0x01, 0x00, 0xA5, 0xFE, 0xFF, 0xFF, 0xFF, 0x4F, 0x00, 
	0x5E, 0x10, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0x20, 0x96, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0x20, 
	0x6F, 0x00, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0x83, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2A, 0x51, 
	0x8F, 0x01, 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x05, 0x82, 
	0xDF, 0x07, 0x71, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9E, 0x01, 0xD4, 
	0xFF, 0x2B, 0x30, 0xE9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xDF, 0x39, 0x20, 0xF9, 
	0xFF, 0x8E, 0x01, 0x81, 0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xEF, 0x8E, 0x02, 0x80, 0xFE, 
	0xFF, 0xFF, 0x16, 0x00, 0x74, 0xCA, 0xCD, 0xCC, 0xDC, 0x4D, 0x00, 0xDA, 0xCC, 0xCC, 0xDC, 0x7A, 0x14, 0x00, 0xD7, 0xFF, 
	0xFF, 0xFF, 0x9D, 0x05, 0x10, 0x32, 0x33, 0x33, 0x33, 0x14, 0x00, 0x43, 0x33, 0x33, 0x33, 0x12, 0x00, 0x94, 0xFD, 0xFF, 
	0xFF, 0xFF, 0xFF, 0x5B, 0x12, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x52, 0xFA, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xEF, 0x9B, 0x47, 0x44, 0x44, 0x44, 0x14, 0x00, 0x43, 0x44, 0x44, 0x54, 0x97, 0xEB, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xEE, 0xDD, 0xDD, 0xDD, 0x4D, 0x00, 0xDB, 0xDD, 0xDD, 0xED, 0xEE, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x20, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x50, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	};

#endif
