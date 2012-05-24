//
// app_param.hpp
// ~~~~~~~~~~~~

#ifndef PROXY_APP_PARAM_HPP
#define PROXY_APP_PARAM_HPP

#include <boost/program_options.hpp>
#include <string>
#include <fstream>

#include "config.hpp"

namespace proxy {

/// Define the param of application.
struct app_param
{
  std::string    ip;
  unsigned short port;
  std::size_t    accept_queue_size;

  std::size_t    io_thread_size;
  std::size_t    work_thread_init;
  std::size_t    work_thread_high;
  std::size_t    work_thread_load;

  std::size_t    handler_pool_init;
  std::size_t    handler_pool_low;
  std::size_t    handler_pool_high;
  std::size_t    handler_pool_inc;
  std::size_t    handler_pool_max;

  std::size_t    read_buffer_size;
  std::size_t    write_buffer_size;
  unsigned int   session_timeout;
  unsigned int   io_timeout;

  std::string    local_ip;
  std::string    proxy_ip;
  unsigned short proxy_port;
};

/// Read parameters from config file.
static int get_param(const std::string& config_file,
                     app_param& param)
{
  namespace bpo = boost::program_options;
  bpo::options_description opt_desc;
  bpo::variables_map var_map;
  std::ifstream fin(config_file.c_str());

  if (!fin.is_open())
    return PROXY_ERR_FILE_NOT_FOUND;

  opt_desc.add_options()
    ("server.ip"                    , bpo::value<std::string   >()->default_value(""  ), "")
    ("server.port"                  , bpo::value<unsigned short>()->default_value(2012), "")
    ("server.accept_queue_size"     , bpo::value<std::size_t   >()->default_value( 250), "")

    ("server.io_thread_size"        , bpo::value<std::size_t   >()->default_value(   4), "")
    ("server.work_thread_init"      , bpo::value<std::size_t   >()->default_value(   4), "")
    ("server.work_thread_high"      , bpo::value<std::size_t   >()->default_value(  32), "")
    ("server.work_thread_load"      , bpo::value<std::size_t   >()->default_value( 100), "")

    ("server.handler_pool_init"     , bpo::value<std::size_t   >()->default_value(1000), "")
    ("server.handler_pool_low"      , bpo::value<std::size_t   >()->default_value(   0), "")
    ("server.handler_pool_high"     , bpo::value<std::size_t   >()->default_value(5000), "")
    ("server.handler_pool_inc"      , bpo::value<std::size_t   >()->default_value(  50), "")
    ("server.handler_pool_max"      , bpo::value<std::size_t   >()->default_value(9999), "")

    ("server.read_buffer_size"      , bpo::value<std::size_t   >()->default_value( 256), "")
    ("server.write_buffer_size"     , bpo::value<std::size_t   >()->default_value(   0), "")
    ("server.session_timeout"       , bpo::value<unsigned int  >()->default_value(  30), "")
    ("server.io_timeout"            , bpo::value<unsigned int  >()->default_value(   0), "")

    ("proxy.local_ip"               , bpo::value<std::string   >()->default_value(""  ), "")
    ("proxy.peer_ip"                , bpo::value<std::string   >()->default_value(""  ), "")
    ("proxy.peer_port"              , bpo::value<unsigned short>()->default_value(2012), "")
    ;

  bpo::store(bpo::parse_config_file(fin, opt_desc, true), var_map);
  fin.close();

  param.ip                    = var_map["server.ip"                   ].as<std::string>();
  param.port                  = var_map["server.port"                 ].as<unsigned short>();
  param.accept_queue_size     = var_map["server.accept_queue_size"    ].as<std::size_t>();

  param.io_thread_size        = var_map["server.io_thread_size"       ].as<std::size_t>();
  param.work_thread_init      = var_map["server.work_thread_init"     ].as<std::size_t>();
  param.work_thread_high      = var_map["server.work_thread_high"     ].as<std::size_t>();
  param.work_thread_load      = var_map["server.work_thread_load"     ].as<std::size_t>();

  param.handler_pool_init     = var_map["server.handler_pool_init"    ].as<std::size_t>();
  param.handler_pool_low      = var_map["server.handler_pool_low"     ].as<std::size_t>();
  param.handler_pool_high     = var_map["server.handler_pool_high"    ].as<std::size_t>();
  param.handler_pool_inc      = var_map["server.handler_pool_inc"     ].as<std::size_t>();
  param.handler_pool_max      = var_map["server.handler_pool_max"     ].as<std::size_t>();

  param.read_buffer_size      = var_map["server.read_buffer_size"     ].as<std::size_t>();
  param.write_buffer_size     = var_map["server.write_buffer_size"    ].as<std::size_t>();
  param.session_timeout       = var_map["server.session_timeout"      ].as<unsigned int>();
  param.io_timeout            = var_map["server.io_timeout"           ].as<unsigned int>();

  param.local_ip              = var_map["proxy.local_ip"              ].as<std::string>();
  param.proxy_ip              = var_map["proxy.peer_ip"               ].as<std::string>();
  param.proxy_port            = var_map["proxy.peer_port"             ].as<unsigned short>();

  return PROXY_ERR_NONE;
}

} // namespace proxy

#endif PROXY_APP_PARAM_HPP
