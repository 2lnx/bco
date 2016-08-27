#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio/steady_timer.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;
typedef ip::tcp::acceptor acceptor_type;
typedef ip::tcp::endpoint endpoint_type;
typedef ip::tcp::socket socket_type;
typedef std::shared_ptr<socket_type> socket_ptr;
typedef vector<char> buffer_type;
const string suffix("end");//数据包后缀，完整数据结束标志，因为tcp数据包是粘连的。
const string delims(" ,\t;");//数字之间的分隔符，可以空格、逗号、tab或者分号
const string ipaddress("0.0.0.0");//服务侦听ip
const int port(6636);
const std::chrono::seconds deadline(120);//超时设定，120秒没数据交互，服务器主动踢出客户端

class Peer{//客户端建立连接，对应一个Peer类实例
    typedef Peer this_type;
private:
    socket_ptr socket;
    buffer_type m_buf;//数据接收缓冲区
    string recvData;//接收到的数据，客户端发来的
    string respData;//返回的数据，反馈回客户端的
    string peerName;//本节点的名字
    std::chrono::system_clock::time_point lasttime;//最近一次活动时间，实现客户端静默超时，服务端主动关闭连接
    string state;//状态标记，“close”表示本节点已经关闭

public:
    Peer(socket_ptr& sp):m_buf(512,0),socket(sp){
        lasttime=std::chrono::system_clock::now();
        peerName=socket->remote_endpoint().address().to_string()+":"+std::to_string(socket->remote_endpoint().port());
        socket->async_write_some(buffer("Hello "+peerName),boost::bind(&this_type::write_handler,this,boost::asio::placeholders::error));
    }
    ~Peer(){socket->close();}//符合RAII，资源自动回收
    string getPeerName(){
        return peerName;
    }
    void write_handler(const system::error_code& ec){
        if(isclose()) return;
        if(ec){
            //cout<<"send fail"<<ec.message()<<" receiver:"<<peerName<<endl;
            //cout<<"close client:"<<peerName<<endl;
            state="close";
            return;
        }
        //cout << "response msg ok，receiver:"<<peerName<<endl;
        socket->async_read_some(buffer(m_buf),bind(&this_type::read_handler,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    }
    void read_handler(const system::error_code& ec,std::size_t bytes_transferred){
        if(isclose()) return;
        if(ec){
            //cout<<"close client:"<<peerName<<endl;
            state="close";
            return;
        }
        lasttime=std::chrono::system_clock::now();

        recvData+=string(&m_buf.front(),bytes_transferred);
        std::string::size_type idx=recvData.find(suffix);
        if(idx!=std::string::npos){
            string package=recvData.substr(0,idx);
            recvData=recvData.substr(idx+suffix.size());//删除接收到的数据包部分
            boost::trim(package);
            respData=processPackage(std::move(package));
            socket->async_write_some(buffer(respData),boost::bind(&this_type::write_handler,this,boost::asio::placeholders::error));
        }else{//如果客户端不发送数据包结束后缀，会造成接收的数据包无限增大，加入数据自动废弃功能
            if(recvData.size()>255){
                recvData.clear();
                socket->async_write_some(buffer("data is too long,auto delete."),boost::bind(&this_type::write_handler,this,boost::asio::placeholders::error));
            }else{//为收到完整数据包，继续接收数据
                socket->async_read_some(buffer(m_buf),bind(&this_type::read_handler,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
            }
        }
    }
    //处里客户端发来的请求，其中数值计算逻辑可执行程序比如EXEU 通过argv把客户数据传递过去，获得计算结果返回客户端
    string processPackage(const string package){
        cout<<"Recive data from:"<<peerName<<" data:"<<package<<endl;
        vector<string> vec;
        split(vec,package,is_any_of(delims),token_compress_on);
        string result;
        try{
            double sum=0;
            for(int i=0;i<vec.size();i++){
                //cout<<vec[i]<<":"<<std::stod(vec[i])<<endl;
                sum+=std::stod(vec[i]);
            }
            result=boost::join(vec,"+")+"="+boost::lexical_cast<string>(sum);
        }catch(std::exception e){
            result="发送的数据包内包含非法数字";
        }
        return std::move(result);
    }
    bool isclose(){
        return state=="close";
    }
    bool isexpire(){
        return (std::chrono::system_clock::now()-lasttime)>deadline;
    }
};
class Server{//网络服务业务逻辑全部在Server类中，客户端连接后自动创建一个peer
    typedef Server this_type;
private:
    io_service m_io;
    acceptor_type m_acceptor;
    boost::asio::steady_timer timer;
    std::map<string,std::shared_ptr<Peer>> m_peers;
    void startTimer(){
        timer.expires_from_now(std::chrono::seconds(1));
        timer.async_wait(boost::bind(&this_type::onTimer,this,boost::asio::placeholders::error));
    }
    void onTimer(const system::error_code&){
        for(auto iter=m_peers.begin();iter!=m_peers.end();){
            if(iter->second->isclose()||iter->second->isexpire()){
                cout<<iter->second->getPeerName()<<" is closed."<<endl;
                m_peers.erase(iter++);
            }else{
               iter++;
            }
        }
        startTimer();
    }

public:
    Server() :timer(m_io), m_acceptor(m_io,endpoint_type(ip::address_v4::from_string(ipaddress),port))
    {
        startTimer();
        accept();
    }
    ~Server(){
        m_acceptor.close();
    }

    void run(){
        m_io.run();
    }
    void accept(){
        socket_ptr socket(new socket_type(m_io));
        m_acceptor.async_accept(*socket,boost::bind(&this_type::accept_handler,this,boost::asio::placeholders::error,socket));
    }
    void accept_handler(const system::error_code& ec,socket_ptr socket){
        if(ec){
            return;
        }
        auto peer=make_shared<Peer>(socket);//客户端新连接，创建一个对等peer实例，提供服务
        cout << "new client:" << peer->getPeerName()<<endl;
        m_peers.emplace(make_pair(peer->getPeerName(),peer));
        accept();//继续接收新的网络连接
    }
};


int main(int argc, char *argv[])
{
    try{
        cout << "Server start ."<<endl;
        Server srv;
        srv.run();
    }catch (std::exception& e){
        cout << e.what() <<endl;
    }
    cout << "Server end." << endl;
    return 0;
}
