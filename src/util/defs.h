#pragma once

typedef signed char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef int b32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double d64;

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define DegreesToRadians(Degrees) Degrees*Pi32/180.f

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;} // Dereference a NULL pointer
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))