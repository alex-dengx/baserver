#ifndef PROXY_CONFIG_HPP
#define PROXY_CONFIG_HPP

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

namespace proxy {

/// Define error_code.
#define PROXY_ERR_NONE              0
#define PROXY_ERR_BASE              PROXY_ERR_NONE - 200  // Error codes offset of PROXY functions.
#define PROXY_ERR_FILE_NOT_FOUND    PROXY_ERR_BASE - 1
#define PROXY_ERR_ALLOC_FAILED      PROXY_ERR_BASE - 2

} // namespace proxy

#endif PROXY_CONFIG_HPP
