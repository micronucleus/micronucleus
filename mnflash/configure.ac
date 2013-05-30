#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([mnflash], [0.1], [andi@collax.com])
AC_CONFIG_SRCDIR([mnflash.c])
# AC_CONFIG_HEADERS([config.h])

AC_ARG_ENABLE([shared],
	[AS_HELP_STRING([--enable-shared],
                        [build a shared library (default is no)])],
        [test "x$withval" != "xno" && ENABLE_SHARED="1"])
AC_SUBST(ENABLE_SHARED)


# Checks for programs.
AC_PROG_CC
# AC_PROG_INSTALL
AC_PROG_LN_S
AC_CHECK_PROG(POD2MAN, pod2man, [pod2man])

# Checks for libraries.
AC_CHECK_LIB([elf], [elf_begin],      [], AC_MSG_ERROR(cannot find libelf))
AC_CHECK_LIB([usb], [usb_get_busses], [], AC_MSG_ERROR(cannot find libusb))

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h memory.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strdup strerror])

AC_CONFIG_FILES([Makefile mnflash.pc])
AC_OUTPUT
