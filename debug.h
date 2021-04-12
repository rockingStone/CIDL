
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
#define PRINT_DEBUG_FILE 0
#endif

#ifndef SPIN_ON_ERROR
#define SPIN_ON_ERROR 0
#endif

#define USE_PKEY 0

//#define ENV_GDB_VEC "NVP_GDB_VEC"
/*
#define fopen fopen_orig
#undef fopen
*/
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

// use stderr, until we dup it
#define NVP_PRINT_FD stderr

typedef char* charptr;
void xil_printf(FILE* f, const charptr c, ...);
//static inline void _nvp_debug_handoff(void)

/*
#define _nvp_debug_handoff(x)			\
{ \
	xil_printf(stderr, "Stopping thread and waiting for gdb...\ngdb --pid=%i\n", getpid()); \
	fflush(stderr);						\
	sleep(1); \
	volatile int asdf = 1; \
	sleep(1); \
	while(asdf) {}; \
}
*/

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
//#define PRINTFUNC fprintf 
#define PRINTFUNC xil_printf 


#if SHOW_MSG
	#define MSG(format, ...) do{PRINTFUNC(NVP_PRINT_FD, "\033[01;32mNVP_MSG(\e[m\033[01;36mF:%s L:%d\e[m\033[01;33m):\e[m" format, __func__ , __LINE__ , ##__VA_ARGS__); fflush(NVP_PRINT_FD); }while(0)
#else
	#define MSG(format, ...)
#endif
//#define MSG(format, ...) do{PRINTFUNC(NVP_PRINT_FD, "MSG: "); PRINTFUNC (NVP_PRINT_FD, format, ##__VA_ARGS__); fflush(NVP_PRINT_FD); }while(0)
#define ERROR(format, ...) do{PRINTFUNC(NVP_PRINT_FD, "\033[01;31mNVP_ERROR\e[m (F:%s L:%d): " format, __func__ , __LINE__ , ##__VA_ARGS__); PRINTFUNC(NVP_PRINT_FD, "ROHAN HERE\n"); _nv_error_count++; if(SPIN_ON_ERROR){ _nvp_debug_handoff(); } }while(0)

FILE *debug_fd;
//#define DEBUG_FD debug_fd
#define DEBUG_FD stderr

#if PRINT_DEBUG_FILE
#define DEBUG_FILE(format, ...) do {PRINTFUNC(DEBUG_FD, "\033[01;33mNVP_DEBUG (\e[m\033[01;35mF:%s L:%d\e[m\033[01;32m):\e[m" format, __func__ ,__LINE__, ##__VA_ARGS__); fflush(NVP_PRINT_FD); }while(0)
#else
#define DEBUG_FILE(format, ...) do{}while(0)
#endif

int _nv_error_count;

#if SHOW_DEBUG
	#define DEBUG(format, ...)   do{PRINTFUNC(NVP_PRINT_FD, "\033[01;33mNVP_DEBUG (\e[m\033[01;35mF:%s L:%d\e[m\033[01;32m):\e[m"   format, __func__ , __LINE__ , ##__VA_ARGS__); fflush(NVP_PRINT_FD); } while(0)
    #define WARNING(format, ...) do{PRINTFUNC(NVP_PRINT_FD, "\033[01;36mNVP_WARNING (\e[m\033[01;35mF:%s\e[m\033[01;32m):\e[m " format, __func__ , ##__VA_ARGS__); } while(0)
	#define DEBUG_P(format, ...) do{PRINTFUNC(NVP_PRINT_FD, format, ##__VA_ARGS__); } while(0)
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