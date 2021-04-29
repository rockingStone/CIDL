
#ifndef __DEBUG_INCLUDED
#define __DEBUG_INCLUDED

// Turns on debugging messages
#undef SHOW_DEBUG
#ifndef SHOW_DEBUG
	#define SHOW_DEBUG 0
#endif

#undef SHOW_MSG
#ifndef SHOW_MSG
	#define SHOW_MSG 1
#endif

#undef PRINT_DEBUG_FILE
#ifndef PRINT_DEBUG_FILE
#define PRINT_DEBUG_FILE 1
#endif

#ifndef SPIN_ON_ERROR
#define SPIN_ON_ERROR 0
#endif

#define USE_PKEY 0

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

// use stderr, until we dup it
#define PRINT_FD stderr

typedef char* charptr;
void xil_printf(FILE* f, const charptr c, ...);

#define _nvp_debug_handoff(x) \
{ \
	sleep(1); \
	volatile int asdf = 1; \
	sleep(1); \
	while(asdf) {}; \
}


//void outbyte(char c);

//#define ERROR_NAMES (EPERM) (ENOENT) (ESRCH) (EINTR) (EIO) (ENXIO) (E2BIG) (ENOEXEC) (EBADF) (ECHILD) (EAGAIN) (ENOMEM) (EACCES) (EFAULT) (ENOTBLK) (EBUSY) (EEXIST) (EXDEV) (ENODEV) (ENOTDIR) (EISDIR) (EINVAL) (ENFILE) (EMFILE) (ENOTTY) (ETXTBSY) (EFBIG) (ENOSPC) (ESPIPE) (EROFS) (EMLINK) (EPIPE) (EDOM) (ERANGE) (EDEADLK)
#define ERROR_NAMES_LIST (EPERM, (ENOENT, (ESRCH, (EINTR, (EIO, (ENXIO, (E2BIG, (ENOEXEC, (EBADF, (ECHILD, (EAGAIN, (ENOMEM, (EACCES, (EFAULT, (ENOTBLK, (EBUSY, (EEXIST, (EXDEV, (ENODEV, (ENOTDIR, (EISDIR, (EINVAL, (ENFILE, (EMFILE, (ENOTTY, (ETXTBSY, (EFBIG, (ENOSPC, (ESPIPE, (EROFS, (EMLINK, (EPIPE, (EDOM, (ERANGE, (EDEADLK, BOOST_PP_NIL)))))))))))))))))))))))))))))))))))

#define ERROR_IF_PRINT(r, data, elem) if(data == elem) { DEBUG("errno == %s (%i): %s\n", MK_STR(elem), elem, strerror(elem)); }

// also used in fileops_wrap
#define PRINTFUNC fprintf 
//#define PRINTFUNC xil_printf 


#if SHOW_MSG
	#define MSG(format, ...) do{PRINTFUNC(PRINT_FD, "\033[01;32mMSG(\e[m\033[01;36mF:%s L:%d\e[m\033[01;33m):\e[m" , __func__ , __LINE__); PRINTFUNC(PRINT_FD,  format, ##__VA_ARGS__); fflush(PRINT_FD); }while(0)
#else
	#define MSG(format, ...)
#endif
#define ERROR(format, ...) do{PRINTFUNC(PRINT_FD, "\033[01;31mERROR\e[m (F:%s L:%d): " , __func__ , __LINE__); PRINTFUNC(PRINT_FD,  format, ##__VA_ARGS__); _nv_error_count++; if(SPIN_ON_ERROR){ _nvp_debug_handoff(); } }while(0)

FILE *debug_fd;
#define DEBUG_FD debug_fd
//#define DEBUG_FD stderr

#if PRINT_DEBUG_FILE
#define DEBUG_FILE(format, ...) do {PRINTFUNC(DEBUG_FD, "DEBUG (F:%s L:%d):" , __func__ ,__LINE__); PRINTFUNC(DEBUG_FD, format, ##__VA_ARGS__); fflush(DEBUG_FD); }while(0)
#else
#define DEBUG_FILE(format, ...) do{}while(0)
#endif

int _nv_error_count;

#if SHOW_DEBUG
	#define DEBUG(format, ...)   do{PRINTFUNC(PRINT_FD, "\033[01;33mDEBUG (\e[m\033[01;35mF:%s L:%d\e[m\033[01;32m):\e[m", __func__ , __LINE__); PRINTFUNC(PRINT_FD, format, ##__VA_ARGS__); fflush(PRINT_FD); } while(0)
    #define WARNING(format, ...) do{PRINTFUNC(PRINT_FD, "\033[01;36mWARNING (\e[m\033[01;35mF:%s\e[m\033[01;32m):\e[m ", __func__); PRINTFUNC(PRINT_FD, format, ##__VA_ARGS__); } while(0)
	#define DEBUG_P(format, ...) do{PRINTFUNC(PRINT_FD, format, ##__VA_ARGS__); } while(0)
#else
	#define DEBUG(format, ...) do{}while(0)
	#define WARNING(format, ...) do{}while(0)
	#define DEBUG_P(format, ...) do{}while(0)
#endif

#define FAIL \
"FFFFFFFFFFFFFFFFFFFFFF      AAA               IIIIIIIIII LLLLLLLLLLL             \n"\
"F::::::::::::::::::::F     A:::A              I::::::::I L:::::::::L             \n"\
"F::::::::::::::::::::F    A:::::A             I::::::::I L:::::::::L             \n"\
"FF::::::FFFFFFFFF::::F   A:::::::A            II::::::II LL:::::::LL             \n"\
"  F:::::F       FFFFFF  A:::::::::A             I::::I     L:::::L               \n"\
"  F:::::F              A:::::A:::::A            I::::I     L:::::L               \n"\
"  F::::::FFFFFFFFFF   A:::::A A:::::A           I::::I     L:::::L               \n"\
"  F:::::::::::::::F  A:::::A   A:::::A          I::::I     L:::::L               \n"\
"  F:::::::::::::::F A:::::A     A:::::A         I::::I     L:::::L               \n"\
"  F::::::FFFFFFFFFFA:::::AAAAAAAAA:::::A        I::::I     L:::::L               \n"\
"  F:::::F         A:::::::::::::::::::::A       I::::I     L:::::L               \n"\
"  F:::::F        A:::::AAAAAAAAAAAAA:::::A      I::::I     L:::::L         LLLLLL\n"\
"FF:::::::FF     A:::::A             A:::::A   II::::::II LL:::::::LLLLLLLLL:::::L\n"\
"F::::::::FF    A:::::A               A:::::A  I::::::::I L::::::::::::::::::::::L\n"\
"F::::::::FF   A:::::A                 A:::::A I::::::::I L::::::::::::::::::::::L\n"\
"FFFFFFFFFFF  AAAAAAA                   AAAAAAAIIIIIIIIII LLLLLLLLLLLLLLLLLLLLLLLL\n"

#endif