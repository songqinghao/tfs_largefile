#ifndef _COMMON_H_INCLUDED_
#define _COMMON_H_INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <error.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <assert.h>


namespace myProject
{
    namespace largefile
    {
        //磁盘操作未完成代号
        const int32_t EXIT_DISK_OPER_INCOMPLETE = -8012;
        const int32_t TFS_SUCCESS = 0;
    }
}

#endif /*_COMMON_H_INCLUDED_*/