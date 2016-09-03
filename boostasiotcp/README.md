
针对开源中国悬赏项目 https://zb.oschina.net/reward/1788312_12646 编写
项目要求是：

> 实现服务端和客户端的TCP/IP通讯，其中服务端采用ubuntu操作系统，客户端支持ubuntu和windows。代码方面尽量使用标准C++，编译器用g++。拒绝使用 windows那一套什么visual studio及相关的库。
> 客户端连接服务端，并向服务端发送数据（比如发送了三个数值a, b, c）；
> 服务端接收客户端发送的数据，进行计算（a+b+c）并将结果返回给客户端。
> 这里计算部分是调用单独的一个可执行程序比如EXEU 它的argv可传入服务端所接收来自客户端发送的数值~ 
> 如此循环，直到客户端中断通讯，当然如果客户端有一段时间（比如2分钟）没有和服务端交互，服务端可以主动中断与客户端的连接。
> 封装尽量干净整洁简约。 会选取所有解决方案中最佳的解决方案！！所以务必对这块内容很熟练，甚至已经做过这部分工作的来参与。 谢谢：）

第一次参与开源中国的众包项目，很兴奋，经常做网络开发项目，这方面很有经验，boost库使用的也比较多，现在开源提供给大家学习借鉴下。
在CMakeList.txt中有一条语句 set(BOOST_ROOT "/opt/boost_1_60_0")，作用是指定boost库所在目录。在你的开发环境下，你可以用#号注释，让cmake自己搜索boost库目录，如果安装过boost开发库，cmake都能搜到。
cmake的使用方法是。
- 安装： apt-get install cmake
- 运行：cmake .  别忽略后面的‘，’
- 编译: make   编译成功，server和client执行文件全部生成