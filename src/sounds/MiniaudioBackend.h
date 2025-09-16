//
// Created by bobini on 16.09.25.
//

#ifndef MINIAUDIO_BACKEND_H
#define MINIAUDIO_BACKEND_H
#ifdef MINIAUDIO_IMPLEMENTATION
#ifdef _WIN32
#include <windows.h>
#endif
#else
#define STB_VORBIS_HEADER_ONLY
#endif
extern "C" {
#include <stb_vorbis.c>
}
#include <miniaudio.h>
#undef R
#undef L
#undef C
#endif //MINIAUDIO_BACKEND_H
