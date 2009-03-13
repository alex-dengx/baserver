一、bas简介
bas是采用Leader/Follower和Half-Sync/Half-Async混合模式的服务器框架，使用c++实现，能够大大简化tcp server的开发工作。
bas目前实现了以下功能：
1、底层基于boost及asio实现，跨越多种操作系统平台；
2、I/O部分使用非阻塞异步处理机制、采用Leader/Follower模式实现，业务逻辑处理部分采用同步线程池实现，便于更好的利用多处理器资源；
3、封装处理各种I/O操作及状态，采用无共享锁/无引用计数设计，控制逻辑清晰、简单，用户应用程序无须关心I/O操作细节，只需要关心业务逻辑的具体实现；
4、提供多级tcp server访问处理机制，非常容易实现各种代理服务器；
5、提供echo_server、echo_client、proxy_server、http_server(基于asio的http server示例)等示例供参考。

二、bas简单说明
bas代码中包含了详细的注解(英文，但不一定正确，请谅解)，几个示例程序演示了基本的用法，这里主要将用户需要处理的两个派生类简单说明如下：
1、由service_handler派生的子类负责处理具体业务，由service_handler_pool派生的子类负责新建和重用service_handler子类实例；
2、service_handler的6个纯虚方法需要由子类来重载：
   on_open:   当连接建立成功后将首先被调用；
   on_read:   当读操作正常完成后被调用，参数为成功读取数据长度；
   on_write:  当写操作正常完成后被调用；
   on_close:  当连接关闭时被调用，参数为错误原因，主要包括：
                  0, 操作成功完成，连接正常关闭；
                  boost::asio::error::eof, 操作成功完成，连接被对端干净的关闭；
                  boost::asio::error::timed_out，操作超时未完成；
                  其他类型的I/O错误请参见<boost/asio/error.hpp>的定义；
   on_parent: 从父连接接收到事件时被调用，参数为事件内容；
   on_child:  从子连接接收到事件时被调用，参数为事件内容；
3、需要主动关闭连接时，从上述任意函数中调用close()即可。

三、版本历史
1、当前版本是0.20.0，为初始公开发布版本；
2、修正内容：无。

四、其他说明
为了实现无锁设计，所有I/O操作及业务操作任务分别被分派到不同的、固定的线程执行。如果业务处理任务非常简单，将会发生不必要的额外线程切换。但是考虑到结构美观、逻辑清晰的需要，且实际的用户应用程序的业务处理任务应该比较繁重，不会像echo_server如此简单，所以最终保留了现有的设计。

五、版权声明
本软件的版权由Xu Ye Jun(moore.xu@gmail.com)所有，基于Boost Software License(Version 1.0)发布，具体规则参见内附的LICENSE_1_0.txt的内容(复制自http://www.boost.org/LICENSE_1_0.txt)。
