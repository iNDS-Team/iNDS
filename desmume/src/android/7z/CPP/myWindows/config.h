
#if !defined(__DJGPP__)

#ifndef __CYGWIN__
  #define FILESYSTEM_IS_CASE_SENSITIVE 1
#endif

  #if !defined(ENV_MACOSX) && !defined(ENV_BEOS)

    /* <wchar.h> */
    /* HAVE_WCHAR__H and not HAVE_WCHAR_H to avoid warning with wxWidgets */
    #define HAVE_WCHAR__H

    /* <wctype.h> */
    #define HAVE_WCTYPE_H

    /* mbrtowc */
/* #ifndef __hpux */
/*    #define HAVE_MBRTOWC */
/* #endif */

    /* towupper */
    #define HAVE_TOWUPPER

  #endif /* !ENV_MACOSX && !ENV_BEOS */

  #if !defined(ENV_BEOS)
  #define HAVE_GETPASS
  #endif

  /* lstat, readlink and S_ISLNK */
  #define HAVE_LSTAT

  /* <locale.h> */
  #define HAVE_LOCALE

  /* mbstowcs */
  #define HAVE_MBSTOWCS

  /* wcstombs */
  #define HAVE_WCSTOMBS

#endif /* !__DJGPP__ */

#ifndef ENV_BEOS
#define HAVE_PTHREAD
#endif

#if defined(ENV_MACOSX)
#define LOCALE_IS_UTF8
#endif

#ifdef LOCALE_IS_UTF8
#undef HAVE_LOCALE
#undef HAVE_MBSTOWCS
#undef HAVE_WCSTOMBS
/* #undef HAVE_MBRTOWC */
#endif

#define MAX_PATHNAME_LEN   1024

