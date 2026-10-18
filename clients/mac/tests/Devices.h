#include "Types.h"
typedef void *ParmBlkPtr;
OSErr OpenDriver(const unsigned char *name, short *reference);
OSErr PBControl(ParmBlkPtr block, Boolean async);
