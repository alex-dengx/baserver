#ifndef ECHO_CONFIG_HPP
#define ECHO_CONFIG_HPP

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

namespace echo {

/// Define error_code.
#define ECHO_ERROR_NONE                    0
#define ECHO_ERROR_BASE                    ECHO_ERROR_NONE - 100  // Error codes offset of ECHOGATE functions.
#define ECHO_ERROR_FILE_NOT_FOUND          ECHO_ERROR_BASE - 1
#define ECHO_ERROR_ALLOC_FAILED            ECHO_ERROR_BASE - 2

} // namespace echo

#endif ECHO_CONFIG_HPP
