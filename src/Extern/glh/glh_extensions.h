// glh_extensions.h: OpenGL extension loading (NVIDIA SDK, Extern/glh).
//
// Reconstructed from Tibia.exe 7.72: the client includes NVIDIA's helper
// header from its Extern folder (the binary keeps the path
// "Extern\glh/glh_extensions.h" in its memory error message) with
// GLH_EXT_SINGLE_FILE defined in SurfaceGL.cpp, so the header's code and the
// extension pointers are SurfaceGL.obj's (0x4f46f0-0x4f954f, .bss
// 0x71d318-0x71dbb0). This version of the header knows the core versions up
// to OpenGL 1.5. Only what the binary has is written here; a #line directive
// gives the two CHECK_MEMORY uses the original's __FILE__ and __LINE__ (173
// and 191), which the binary's messages print.
//
// glh_init_extensions takes a list of extension names separated by blanks,
// initialises the entry points of each (glh_genext.h) and returns false if
// one of them is missing or not supported; the names that failed are kept in
// unsupportedExts. The GL_VERSION_1_x names are supported when the version
// string does not start with an older version.

#ifndef GLH_EXTENSIONS
#define GLH_EXTENSIONS

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <GL/gl.h>

#define CHECK_MEMORY(ptr) \
	if (NULL == ptr) { \
		printf("Error allocating memory in file %s, line %d\n", __FILE__, __LINE__); \
		exit(-1); \
	}

#include "glh_genext.h"

#ifdef GLH_EXT_SINGLE_FILE

static char *unsupportedExts = NULL; // 0x71dbac
static char *sysExts = NULL;         // 0x71dbb0

// 0x4f9040. extName is in a register in the binary (the compiler chose the
// calling convention of this static function). Odd behaviour kept: the
// GL_VERSION_1_x answers return without freeing padExtName.
static int ExtensionExists(const char *extName, const char *sysExts)
{
	char *padExtName = (char *)malloc(strlen(extName) + 2);
	strcat(strcpy(padExtName, extName), " ");

	if (0 == strcmp(extName, "GL_VERSION_1_2")) {
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strstr(version, "1.0") == version || strstr(version, "1.1") == version) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if (0 == strcmp(extName, "GL_VERSION_1_3")) {
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strstr(version, "1.0") == version || strstr(version, "1.1") == version
				|| strstr(version, "1.2") == version) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if (0 == strcmp(extName, "GL_VERSION_1_4")) {
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strstr(version, "1.0") == version || strstr(version, "1.1") == version
				|| strstr(version, "1.2") == version || strstr(version, "1.3") == version) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if (0 == strcmp(extName, "GL_VERSION_1_5")) {
		const char *version = (const char *)glGetString(GL_VERSION);
		if (strstr(version, "1.0") == version || strstr(version, "1.1") == version
				|| strstr(version, "1.2") == version || strstr(version, "1.3") == version
				|| strstr(version, "1.4") == version) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if (strstr(sysExts, padExtName)) {
		free(padExtName);
		return TRUE;
	} else {
		free(padExtName);
		return FALSE;
	}
}

// Inlined into glh_init_extensions.
static const char *EatWhiteSpace(const char *str)
{
	for (; *str && (' ' == *str || '\t' == *str || '\n' == *str); str++)
		;
	return str;
}

static const char *EatNonWhiteSpace(const char *str)
{
	for (; *str && (' ' != *str && '\t' != *str && '\n' != *str); str++)
		;
	return str;
}

// 0x4f9260. The first call builds sysExts from the extension strings of
// OpenGL and WGL (each followed by a blank).
int glh_init_extensions(const char *origReqExts)
{
	// Length of requested extensions string
	unsigned reqExtsLen;
	char *reqExts;
	// Ptr for individual extensions within reqExts
	char *reqExt;
	int success = TRUE;

	// build space-padded extension string
	if (NULL == sysExts) {
		const char *extensions = (const char *)glGetString(GL_EXTENSIONS);
		int sysExtsLen = (int)strlen(extensions);
		const char *winsys_extensions = "";
		int winsysExtsLen = 0;
		{
			PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = 0;
			wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress(
				"wglGetExtensionsStringARB");
			if (wglGetExtensionsStringARB) {
				winsys_extensions = wglGetExtensionsStringARB(wglGetCurrentDC());
				winsysExtsLen = (int)strlen(winsys_extensions);
			}
		}
		// Add 2 bytes, one for padding space, one for terminating NULL
		sysExts = (char *)malloc(sysExtsLen + winsysExtsLen + 3);
		// The original's __FILE__ and __LINE__ (its messages say line 173 and 191).
#line 173 "Extern\\glh/glh_extensions.h"
		CHECK_MEMORY(sysExts);
		strcpy(sysExts, extensions);
		sysExts[sysExtsLen] = ' ';
		sysExts[sysExtsLen + 1] = 0;
		strcat(sysExts, winsys_extensions);
		sysExts[sysExtsLen + 1 + winsysExtsLen] = ' ';
		sysExts[sysExtsLen + 1 + winsysExtsLen + 1] = 0;
	}

	if (NULL == origReqExts)
		return TRUE;
	reqExts = _strdup(origReqExts); // strdup in the original (the same CRT function)
	reqExtsLen = (unsigned)strlen(reqExts);
	if (NULL == unsupportedExts) {
		unsupportedExts = (char *)malloc(reqExtsLen + 2);
	} else if (reqExtsLen > strlen(unsupportedExts)) {
		unsupportedExts = (char *)realloc(unsupportedExts, reqExtsLen + 2);
	}
	CHECK_MEMORY(unsupportedExts);
	*unsupportedExts = 0;

	// Parse requested extension list
	for (reqExt = reqExts; (reqExt = (char *)EatWhiteSpace(reqExt)) && *reqExt;
			reqExt = (char *)EatNonWhiteSpace(reqExt)) {
		char *extEnd = (char *)EatNonWhiteSpace(reqExt);
		char saveChar = *extEnd;
		*extEnd = (char)0;

		if (!ExtensionExists(reqExt, sysExts) || !glh_init_extension(reqExt)) {
			// add reqExt to end of unsupportedExts
			strcat(unsupportedExts, reqExt);
			strcat(unsupportedExts, " ");
			success = FALSE;
		}
		*extEnd = saveChar;
	}
	free(reqExts);
	return success;
}

// 0x4f9510 (inlined into TSurfaceGL::ExitSurfaces as well). Frees the
// extension strings.
void glh_shutdown_extensions()
{
	if (unsupportedExts) {
		free(unsupportedExts);
		unsupportedExts = NULL;
	}
	if (sysExts) {
		free(sysExts);
		sysExts = NULL;
	}
}

#endif // GLH_EXT_SINGLE_FILE

#endif // GLH_EXTENSIONS
