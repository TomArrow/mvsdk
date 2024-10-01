// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"


vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

vec4_t		colorLtBlue	= {0.367f, 0.261f, 0.722f, 1};
vec4_t		colorDkBlue	= {0.199f, 0.0f,   0.398f, 1};

vec4_t	g_color_table[8] =
	{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	};


vec4_t	g_color_table_nt[128] =
{
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x0 0: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1 1: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2 2: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x3 3: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x4 4: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x5 5: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x6 6: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x7 7: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x8 8: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x9 9: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xA 10: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xB 11: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xC 12: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xD 13: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xE 14: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0xF 15: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x10 16: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x11 17: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x12 18: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x13 19: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x14 20: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x15 21: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x16 22: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x17 23: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x18 24: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x19 25: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1A 26: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1B 27: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1C 28: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1D 29: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1E 30: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x1F 31: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x20 32: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x21 33: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x22 34: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x23 35: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x24 36: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x25 37: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x26 38: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x27 39: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x28 40: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x29 41: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2A 42: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2B 43: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2C 44: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2D 45: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2E 46: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x2F 47: 255 255 255
	{ 0.000, 0.000, 0.000, 1.0 }, // '0' 0x30 48:   0   0   0
	{ 1.000, 0.000, 0.000, 1.0 }, // '1' 0x31 49: 255   0   0
	{ 0.000, 1.000, 0.000, 1.0 }, // '2' 0x32 50:   0 255   0
	{ 1.000, 1.000, 0.000, 1.0 }, // '3' 0x33 51: 255 255   0
	{ 0.000, 0.000, 1.000, 1.0 }, // '4' 0x34 52:   0   0 255
	{ 0.000, 1.000, 1.000, 1.0 }, // '5' 0x35 53:   0 255 255
	{ 1.000, 0.000, 1.000, 1.0 }, // '6' 0x36 54: 255   0 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '7' 0x37 55: 255 255 255
	{ 0.949, 0.494, 0.263, 1.0 }, // '8' 0x38 56: 242 126  67
	{ 0.255, 0.000, 0.400, 1.0 }, // '9' 0x39 57:  65   0 102
	{ 1.000, 1.000, 1.000, 1.0 }, // ':' 0x3A 58: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // ';' 0x3B 59: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '<' 0x3C 60: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '=' 0x3D 61: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '>' 0x3E 62: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '?' 0x3F 63: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '@' 0x40 64: 255 255 255
	{ 0.404, 0.757, 0.608, 1.0 }, // 'A' 0x41 65: 103 193 155
	{ 0.486, 0.961, 0.761, 1.0 }, // 'B' 0x42 66: 124 245 194
	{ 0.039, 0.976, 0.608, 1.0 }, // 'C' 0x43 67:  10 249 155
	{ 0.055, 0.741, 0.478, 1.0 }, // 'D' 0x44 68:  14 189 122
	{ 0.000, 0.592, 0.369, 1.0 }, // 'E' 0x45 69:   0 151  94
	{ 0.000, 0.412, 0.255, 1.0 }, // 'F' 0x46 70:   0 105  65
	{ 0.016, 0.231, 0.243, 1.0 }, // 'G' 0x47 71:   4  59  62
	{ 0.024, 0.369, 0.384, 1.0 }, // 'H' 0x48 72:   6  94  98
	{ 0.278, 0.627, 0.620, 1.0 }, // 'I' 0x49 73:  71 160 158
	{ 0.008, 0.808, 0.812, 1.0 }, // 'J' 0x4A 74:   2 206 207
	{ 0.514, 0.941, 0.922, 1.0 }, // 'K' 0x4B 75: 131 240 235
	{ 0.388, 0.733, 0.741, 1.0 }, // 'L' 0x4C 76:  99 187 189
	{ 0.204, 0.376, 0.380, 1.0 }, // 'M' 0x4D 77:  52  96  97
	{ 0.396, 0.620, 0.737, 1.0 }, // 'N' 0x4E 78: 101 158 188
	{ 0.071, 0.667, 0.953, 1.0 }, // 'O' 0x4F 79:  18 170 243
	{ 0.008, 0.259, 0.396, 1.0 }, // 'P' 0x50 80:   2  66 101
	{ 0.020, 0.129, 0.376, 1.0 }, // 'Q' 0x51 81:   5  33  96
	{ 0.000, 0.263, 0.780, 1.0 }, // 'R' 0x52 82:   0  67 199
	{ 0.310, 0.537, 0.961, 1.0 }, // 'S' 0x53 83:  79 137 245
	{ 0.376, 0.506, 0.718, 1.0 }, // 'T' 0x54 84:  96 129 183
	{ 0.384, 0.384, 0.722, 1.0 }, // 'U' 0x55 85:  98  98 184
	{ 0.306, 0.302, 1.000, 1.0 }, // 'V' 0x56 86:  78  77 255
	{ 0.000, 0.004, 0.227, 1.0 }, // 'W' 0x57 87:   0   1  58
	{ 0.184, 0.051, 0.251, 1.0 }, // 'X' 0x58 88:  47  13  64
	{ 0.655, 0.000, 0.016, 1.0 }, // 'Y' 0x59 89: 167   0   4
	{ 0.400, 0.012, 0.612, 1.0 }, // 'Z' 0x5A 90: 102   3 156
	{ 1.000, 1.000, 1.000, 1.0 }, // '[' 0x5B 91: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '\' 0x5C 92: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // ']' 0x5D 93: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '^' 0x5E 94: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '_' 0x5F 95: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '`' 0x60 96: 255 255 255
	{ 0.349, 0.349, 0.349, 1.0 }, // 'a' 0x61 97:  89  89  89
	{ 0.596, 0.757, 0.388, 1.0 }, // 'b' 0x62 98: 152 193 99
	{ 0.757, 0.941, 0.494, 1.0 }, // 'c' 0x63 99: 193 240 126
	{ 0.231, 0.416, 0.016, 1.0 }, // 'd' 0x64 100:  59 106   4
	{ 0.043, 0.231, 0.043, 1.0 }, // 'e' 0x65 101:  11  59  11
	{ 0.498, 0.945, 0.498, 1.0 }, // 'f' 0x66 102: 127 241 127
	{ 0.373, 0.741, 0.365, 1.0 }, // 'g' 0x67 103:  95 189  93
	{ 0.714, 0.475, 0.933, 1.0 }, // 'h' 0x68 104: 182 121 238
	{ 0.565, 0.392, 0.741, 1.0 }, // 'i' 0x69 105: 144 100 189
	{ 1.000, 0.475, 1.000, 1.0 }, // 'j' 0x6A 106: 255 121 255
	{ 0.353, 0.020, 0.353, 1.0 }, // 'k' 0x6B 107:  90   5  90
	{ 0.863, 0.314, 0.498, 1.0 }, // 'l' 0x6C 108: 220 80 127
	{ 0.318, 0.122, 0.031, 1.0 }, // 'm' 0x6D 109:  81  31   8
	{ 0.137, 0.098, 0.000, 1.0 }, // 'n' 0x6E 110:  35  25   0
	{ 0.498, 0.333, 0.012, 1.0 }, // 'o' 0x6F 111: 127  85   3
	{ 0.459, 0.004, 0.016, 1.0 }, // 'p' 0x70 112: 117   1   4
	{ 0.325, 0.012, 0.008, 1.0 }, // 'q' 0x71 113:  83   3   2
	{ 0.463, 0.008, 0.012, 1.0 }, // 'r' 0x72 114: 118   2   3
	{ 0.957, 0.937, 0.537, 1.0 }, // 's' 0x73 115: 244 239 137
	{ 0.984, 0.965, 0.314, 1.0 }, // 't' 0x74 116: 251 246  80
	{ 0.988, 0.984, 0.039, 1.0 }, // 'u' 0x75 117: 252 251  10
	{ 0.816, 0.792, 0.024, 1.0 }, // 'v' 0x76 118: 208 202   6
	{ 0.984, 0.337, 0.024, 1.0 }, // 'w' 0x77 119: 255 255 255
	{ 0.973, 0.400, 0.114, 1.0 }, // 'x' 0x78 120: 255 255 255
	//{ 1.000, 1.000, 1.000, 1.0 }, // 'y' 0x79 121: 255 255 255
	//{ 1.000, 1.000, 1.000, 1.0 }, // 'z' 0x7A 122: 255 255 255
	{ 1.000, 0.271,	0.000, 1.0 }, // 'y' 0x79 121: 255 255 255
	{ 0.502, 0.000,	0.502, 1.0 }, // 'z' 0x7A 122: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '{' 0x7B 123: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '|' 0x7C 124: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '}' 0x7D 125: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x7E 126: 255 255 255
	{ 1.000, 1.000, 1.000, 1.0 }, // '###' 0x7F 127: 255 255 255
};

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

#ifdef __LCC__

int VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}			
	return 1;
}

vec_t VectorLength( const vec3_t v ) {
	return (vec_t)sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t VectorLengthSquared( const vec3_t v ) {
	return (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract (p2, p1, v);
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

void VectorInverse( vec3_t v ){
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}
#endif

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


// this isn't a real cheap function to call!
int DirToByte( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;
	float	rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	zrot[0][0] = cos( rad );
	zrot[0][1] = sin( rad );
	zrot[1][0] = -sin( rad );
	zrot[1][1] = cos( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
#ifndef Q3_VM
	assert( Q_fabs(inv_denom) != 0.0f ); // bk010122 - zero vectors get here
#endif
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//============================================================================

/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	floatint_t fi;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	fi.f  = y;									// evil floating point bit level hacking
	fi.i  = 0x5f3759df - ( fi.i >> 1 );			// what the fuck?
	y  = fi.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

float Q_fabs( float f ) {
	floatint_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	assert(fabs(a) < 3600);
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float	AngleMod(float a) {
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/

int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}


vec_t VectorNormalize( vec3_t v ) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if ( length ) {
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);

	if (length)
	{
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])!=0.0f) || (Q_fabs(v[1])!=0.0f) || (Q_fabs(v[2])!=0.0f)) );
#endif
		ilength = 1/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
#ifndef Q3_VM // bk0101022 - FPE related
//	  assert( ((Q_fabs(v[0])==0.0f) && (Q_fabs(v[1])==0.0f) && (Q_fabs(v[2])==0.0f)) );
#endif
		VectorClear( out );
	}
		
	return length;

}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/


/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

// This is the VC libc version of rand() without multiple seeds per thread or 12 levels
// of subroutine calls.
// Both calls have been designed to minimise the inherent number of float <--> int 
// conversions and the additional math required to get the desired value.
// eg the typical tint = (rand() * 255) / 32768
// becomes tint = irand(0, 255)

static unsigned int	holdrand = 0x89abcdef;

void Rand_Init(int seed)
{
	holdrand = seed;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)

float flrand(float min, float max)
{
	float	result;

	assert((max - min) < 32768);

	holdrand = (holdrand * 214013) + 2531011;
	result = (float)(holdrand >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) / 32768.0F) + min;

	return(result);
}

// Returns an integer min <= x <= max (ie inclusive)

int irand(int min, int max)
{
	int		result;

	assert((max - min) < 32768);

	max++;
	holdrand = (holdrand * 214013) + 2531011;
	result = holdrand >> 17;
	result = ((result * (max - min)) >> 15) + min;
	return(result);
}

float JK2_powf ( float x, int y )
{
	float r = x;
	for ( y--; y>0; y-- )
		r = r * r;
	return r;
}

// Returns `base` raised to the power of `exp`
float Q_pown(float base, int exp)
{
	float		result = 1.0f;
	qboolean	invert = qfalse;

	if (exp < 0) {
		invert = qtrue;
		exp = -exp;
	}

	while (exp > 0) {
		if (exp & 1) {
			result *= base;
		}

		base *= base;
		exp >>= 1;
	}

	if (invert)
		result = 1.0f / result;

	return result;
}
