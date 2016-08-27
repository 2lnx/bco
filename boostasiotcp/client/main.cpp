#include <iostream>
#include <vector>
#include <string>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;
const string suffix("end");
const string serverip("127.0.0.1");
const int port(6636);
class Client{//客户端所有业务逻辑封装于Client类中
    typedef ip::tcp::endpoint endpoint_type;
    typedef ip::address address_type;
    typedef ip::tcp::socket socket_type;
    typedef std::shared_ptr<socket_type> socket_ptr;
    typedef vector<char> buffer_type;
    typedef Client this_type;
private:
    io_service m_io;
    buffer_type m_buf;
    endpoint_type m_endpoint;
    boost::asio::io_service::work* work;
public:
    Client() :
        m_buf(512,0),
        m_endpoint(address_type::from_string(serverip),port)
    {
        start();
    }
    void run(){
        cout << "client run()"<<endl;
        work = new boost::asio::io_service::work( m_io );
        m_io.run();
    }
    void start(){
        cout << "client start()"<<endl;
        socket_ptr socket(new socket_type(m_io));
        //连接服务器
        socket->async_connect(m_endpoint,bind(&this_type::conn_handler,this,boost::asio::placeholders::error,socket));
    }
    void conn_handler(const system::error_code& ec,socket_ptr socket){
        if(ec){
            return;
        }
        cout << "recive from " << socket->remote_endpoint().address().to_string()<<endl;
        socket->async_read_some(buffer(m_buf),bind(&this_type::read_handler,this,socket,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    }

    void read_handler(socket_ptr socket,const system::error_code& ec,std::size_t bytes_transferred){
        cout<<socket->remote_endpoint().address().to_string()<<endl;
        if(ec){
            cout << ec.message() <<endl;
            return;
        }
        cout << "recieve msg: " <<string(&m_buf.front(),bytes_transferred)<< endl;
        cout << "please input number or type ‘exit’ quit:";
        string str;
        getline(cin,str);
        if(str=="exit"){//exit 是退出命令
            socket->close();
            work->~work();
        }else{
            socket->async_write_some(buffer(str+suffix),boost::bind(&this_type::write_handler,this,socket,boost::asio::placeholders::error));
        }
    }
    void write_handler(socket_ptr socket,const system::error_code& ec){
        if(ec){
            cout<<"send fail"<<ec.message()<<endl;
            return;
        }
        cout << "send msg ok" << endl;
        socket->async_read_some(buffer(m_buf),bind(&this_type::read_handler,this,socket,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    }

};

int main(int argc, char *argv[])
{

    try{
        cout << "client start ."<<endl;
        Client cli;
        cli.run();
    }catch (std::exception& e){
        cout << e.what() <<endl;
    }
    cout << "client end." << endl;
    return 0;
}
