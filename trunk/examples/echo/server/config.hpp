#ifndef ECHO_CONFIG_HPP
#define ECHO_CONFIG_HPP

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

namespace echo {

/// Define error_code.
#define ECHO_ERR_NONE              0
#define ECHO_ERR_BASE              ECHO_ERR_NONE - 100  // Error codes offset of ECHO functions.
#define ECHO_ERR_FILE_NOT_FOUND    ECHO_ERR_BASE - 1
#define ECHO_ERR_ALLOC_FAILED      ECHO_ERR_BASE - 2

} // namespace echo

#endif ECHO_CONFIG_HPP
