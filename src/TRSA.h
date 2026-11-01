// TRSA: RSA encryption with a public key, used for the login packet
// (0x508970-0x508c64). The original file name is unknown; "TRSA" fits the
// alphabetical link order (after TMyException, before TXTEA). Errors are
// thrown as const char*.

#pragma once

#include "vlong.h"

class TRSA {
public:
	TRSA();                                // 0x5089a0
	virtual ~TRSA();                       // 0x5089c0; scalar deleting destructor 0x5089d0
	void SetPublicKey(const char *ModulusM); // 0x5089f0, decimal text
	void Encrypt(uint8 *Data);             // 0x508aa0, 128 bytes in place, exponent 65537

private:
	vlong m_ModulusM;                      // +0x4
};
