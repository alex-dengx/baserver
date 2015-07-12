bas(name as boost\_asio\_server) is a server framework implementation of the Half-Sync/Half-Async mode, can greatly simplify the development of tcp server. copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com), distributed under the Boost Software License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

bas为boost\_asio\_server(baserver)的简称，是采用Half-Sync/Half-Async模式的服务器框架，使用c++实现，能够大大简化tcp server的开发工作。bas目前实现了以下功能：

1、底层基于boost及asio实现，支持ssl，跨越多种操作系统平台；

2、I/O部分使用非阻塞异步处理机制、业务逻辑处理部分采用同步线程池实现，便于更好的利用多处理器资源；

3、封装处理各种I/O操作及状态，采用无共享锁设计，控制逻辑清晰、简单，用户应用程序无须关心I/O操作细节，只需要关心业务逻辑的具体实现；

4、提供多级tcp server访问处理机制，非常容易实现各种代理服务器；

5、提供echo\_server/echo\_client、ssl\_server/ssl\_client、proxy\_server、http\_server(基于asio的http server示例)等示例供参考。

请使用svn checkout最新的代码。

本软件的版权由Xu Ye Jun(moore.xu@gmail.com)所有，基于Boost Software License(Version 1.0)发布，具体规则参见http://www.boost.org/LICENSE\_1\_0.txt。