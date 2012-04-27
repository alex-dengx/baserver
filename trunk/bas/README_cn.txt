一、bas简介
bas为boost_asio_server(baserver)的简称，是采用Half-Sync/Half-Async模式的服务器框架，使用c++实现，能够大大简化tcp server的开发工作。
bas目前实现了以下功能：
1、底层基于boost及asio实现，支持ssl，跨越多种操作系统平台；
2、I/O部分使用非阻塞异步处理机制，业务逻辑处理部分采用同步线程池，便于更好的利用多处理器资源；
3、封装处理各种I/O操作及状态，采用无共享锁设计，控制逻辑清晰、简单，用户应用程序无须关心I/O操作细节，只需要关心业务逻辑的具体实现；
4、提供多级tcp server访问处理机制，非常容易实现各种代理服务器；
5、提供echo_server/echo_client、ssl_server/ssl_client、proxy_server、http_server(基于asio的http server示例)等示例供参考。

二、bas简单说明
bas代码中包含了详细的注解，几个示例程序演示了基本的用法，这里主要将两个模板参数的具体实现要求说明如下：
1、由Work_Allocator类负责新建Work_Handler类实例，必须实现如下函数：
   make_handler:  新建一个Work_Handler类实例；
   make_socket:   新建一个tcp::socket或ssl::stream类实例，看起来是比较奇怪的设计，不过为了适用于创建ssl::stream类，必须从Work_Allocator类中获得context参数；

2、Work_Handler类负责执行业务逻辑，必须实现如下函数：
   on_set_parent: 设置父连接指针时被调用，参数为负责I/O操作的service_handler对象和父连接指针；
   on_set_child:  设置子连接指针时被调用，参数为负责I/O操作的service_handler对象和子连接指针；
   on_clear:      连接开始前被调用，目的是将所有内部变量清除为初始状态；
   on_open:       当连接建立成功后将首先被调用，参数为负责I/O操作的service_handler对象；
   on_read:       当读操作正常完成后被调用，参数为负责I/O操作的service_handler对象和成功读数据长度；
   on_write:      当写操作正常完成后被调用，参数为负责I/O操作的service_handler对象和成功写数据长度；
   on_close:      当连接关闭时被调用，参数为负责I/O操作的service_handler对象和错误原因，错误原因主要包括：
                      0, 操作成功完成，连接正常关闭；
                      boost::asio::error::eof, 操作成功完成，连接被对端干净的关闭；
                      boost::asio::error::timed_out，操作超时未完成；
                      其他类型的I/O错误请参见<boost/asio/error.hpp>的定义；
   on_parent:     从父连接接收到事件时被调用，参数为负责I/O操作的service_handler对象和事件内容；
   on_child:      从子连接接收到事件时被调用，参数为负责I/O操作的service_handler对象和事件内容；
   在上述函数中，可通过调用service_handler对象的async_read_some、async_read、async_write等函数实现I/O操作，需要主动关闭连接时调用close即可。具体用法请参考示例；

三、版本历史
1、0.30.0，初始公开发布版本；
2、0.30.5，进一步完善接口，主要改变包括：
   *将模板函数的clear()改为与其它函数类似的on_clear()；
   *直接使用endpoint连接host，相应修改了对应函数的port参数类型；
   *增加connect接口，允许用作代理时临时指定连接host；
3、0.30.6，on_write接口调整为与on_read类似，增加成功写数据长度参数；
4、0.40.0，调整父/子连接的实现方式，由应用程序负责管理父、子连接之间的所有动作；
5、0.50.0，优化设计，主要改变包括：
   *优化io_service_pool设计，使用get_io_service(std::size_t load)可在重负载时增加工作线程；
   *调整service_handler_pool设计，可更好地适应连接池的动态调整；
   *强化service_handler异步操作缓冲区检查，缓冲区不足时直接关闭连接并返回错误信息boost::asio::error::no_buffer_space；
6、0.51.0，应用户要求，超时处理调整为整个连接会话超时和单个I/O操作超时分别进行处理，控制逻辑更灵活；
7、0.52.0，调整server接口，增加输入参数；
8、0.53.0，优化server设计，投递多个async_accept提高连接效率，优化连接中止处理。
9、0.55.0，调整部分接口，优化连接池控制，设置连接上限，超出上限时服务器延时accept。

四、其他说明
为了实现无锁设计，所有I/O操作及业务操作任务分别被分派到不同但固定的线程执行。如果业务处理任务负载非常小，将会发生不必要的额外线程切换。但是考虑到逻辑清晰而简单的需要(不是有个说法叫做科学就是追求简单嘛)，且实际的用户应用程序的业务处理任务负载应该比较繁重，所以最终保留了现有的设计。

五、版权声明
本软件的版权由Xu Ye Jun(moore.xu@gmail.com)所有，基于Boost Software License(Version 1.0)发布，具体规则参见内附的LICENSE_1_0.txt的内容(复制自http://www.boost.org/LICENSE_1_0.txt)。
