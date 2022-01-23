#include <arpa/inet.h>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unistd.h>
#include <regex>
#include "netdb.h"
#include "sendmail.h"

// public
SendMail* SendMail::getinstance() {
    static SendMail instance;
    return &instance;
}

SendMail::~SendMail() {
    if (ser_sock == -1) {
        close(ser_sock);
    }
}

bool SendMail::initSocket(const std::string &Host, const int &Port) {
    const std::string Ip = HostNameToIp(Host);
    ser_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (ser_sock  < 0) {
        std::cout << "Server: create socket error!" << std::endl;
        return false;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(Port);
    inet_pton(AF_INET, Ip.c_str(), &server_address.sin_addr);

    int ret = 0;

    ret = connect(ser_sock, (struct sockaddr*)&server_address, sizeof(server_address));
    if (ret < 0) {
        std::cout << "Server: connect failed! " << std::endl;
        return false;
    }

    return true;
}

bool SendMail::SendVerificationMail(const std::string& mail) {
    std::regex mail_compare("\\w+\\@+(?:(\\w)+(?:(\\.){1}+\\w+){1,})");
    if (!std::regex_match(mail, mail_compare)) {
        std::cout << "Please check e-mail address format!" << std::endl;
        return false;
    }
    if (initState == false) {
        initconfig();
    }

    // 发送方邮箱配置
    sendMsg("mail from: <");
    sendMsg("1508498108@qq.com");
    sendMsg(">");
    sendMsg("\r\n");
    recvMsg();

    // 接收方邮箱配置
    sendMsg("rcpt to: <");
    sendMsg(mail.c_str());
    sendMsg(">");
    sendMsg("\r\n");
    recvMsg();

    bool ret = GenerateVerificationCode(mail);
    if (!ret) {
        return false;
    }

    // 开始发送邮件内容
    sendMsg("data\r\n");
    sendMsg("subject: Verfication Code\r\n\r");

    // 发送的生成的六位验证码
    sendMsg(VerficationCode.c_str());
    sendMsg("\r\n.\r\n");
    recvMsg();

    return true;
}

bool SendMail::CompareVerificationCode(const std::string& mail, const std::string& code)
{
    // 使用后删除
    for (auto it = VerificationMap.begin(); it != VerificationMap.end(); ++it) {
        if (it->first == mail && it->second.compare(code) == 0) {
            VerificationMap.erase(it);
            return true;
        }
    }
    // 验证码未过期，且比对失败 ==> 验证码保留
    return false;
}


// private
SendMail::SendMail() :
    ser_sock(-1),
    initState(false),
    message(),
    VerficationCode(),
    VerificationMap() {
}

bool SendMail::sendMsg(const char* msg) {
    int ret = 0;
    ret = send(ser_sock, msg, strlen(msg), 0);
    if (ret == -1) {
        std::cout << "Server: send message error !" << std::endl;
        return false;
    }
    memset(message, 0x00, BUFSIZE);
    return true;
}


void SendMail::startTimer(const std::string& code)
{
    initTimer(code);
    signal(SIGALRM, handler);
}

void SendMail::initTimer(const std::string& code)
{
    tempans.code = code;
    timer = new heap_timer(SECONDTIME);
    timer->user_data = &tempans;
    timer->callBackFunc = &terminalBack;
    m_timeHeap.AddTimer(timer);
    alarm(SECONDTIME);
}

void SendMail::terminalBack()
{
    std::cout << "I has been invoke!" << std::endl;
}


void SendMail::tick()
{
    while (!m_timeHeap.isEmpty()) {
        m_timeHeap.Tick();
    }
    if (!m_timeHeap.TimeOutArray().empty()) {
        auto&& res = m_timeHeap.TimeOutArray();
        TimeoutVerifyCode = res;
    }
}

void SendMail::DelTimeoutVerifyCode()
{
    if (!TimeoutVerifyCode.empty()) {
        for (auto iter = TimeoutVerifyCode.begin(); iter != TimeoutVerifyCode.end(); ++iter) {
            if (VerificationMap.empty()) {
                break;
            }
            for (auto it = VerificationMap.begin(); it != VerificationMap.end(); ++it) {
                if (it->second.compare( (*iter)->user_data->code) == 0) {
                    VerificationMap.erase(it);
                }
                else {
                    break;
                }
            }
        }
    }
    TimeoutVerifyCode.clear();
    return;
}

void SendMail::myhandler(int signum)
{
//    std::thread t1(&SendMail::tick, this);
//    t1.join();
    SendMail::getinstance()->tick();
}

void SendMail::handler(int signum)
{
    SendMail::getinstance()->myhandler(signum);
    return;
}

bool SendMail::recvMsg() {
    int ret = 0;
    memset(message, 0x00, BUFSIZE);
    ret = recv(ser_sock, message, BUFSIZE, 0);
    if (ret == -1) {
        std::cout << "Server: recv failed!" << std::endl;
        return false;
    }
    return true;
}

bool SendMail::initconfig()
{
    sendMsg("HELO qq.com\r\n");
    recvMsg();
    // 验证登陆
    sendMsg("auth login\r\n");
    recvMsg();
    // 用户名(bash64编码)
    sendMsg("MTUwODQ5ODEwOEBxcS5jb20=");
    sendMsg("\r\n");
    recvMsg();
    // 密码(bash64编码)
    sendMsg("aHFzbGJyb2V2bm5rYmFhYQ==");
    sendMsg("\r\n");
    recvMsg();

    initState = true;
    return true;
}

const std::string SendMail::HostNameToIp(const std::string& hostname) {
    struct hostent* host;
    std::string resIp = "";

    std::regex ip_match("^(?:[01]?\\d{1,2}|2(?:[0-4][0-9]|5[0-5]))(?:\\.(?:[01]?\\d{1,2}|2(?:[0-4][0-9]|5[0-5]))){3}$");
    if (std::regex_match(hostname, ip_match)) {
        std::cout << "successfully";
        return hostname;
    }
    host = gethostbyname(hostname.c_str());

    if (host->h_addrtype != AF_INET) {
        std::cout << "Resolved domain name is not IPV4!" << std::endl;
        return resIp;
    }

    for (int i = 0; host->h_addr_list[i]; i++) {
        resIp = inet_ntoa(*(struct in_addr*) host->h_addr_list[i]);
        if (resIp != "") return resIp;
    }
    return resIp;
}


bool SendMail::GenerateVerificationCode(const std::string& mail) {
    srand(time(0));
    long num = (rand()%(MAXNUM - MINNUM + 1)) + MINNUM;
    VerficationCode = std::to_string(num);
    DelTimeoutVerifyCode();
    for (auto it = VerificationMap.begin(); it != VerificationMap.end(); ++it) {
        if (it->first == mail) {
            return false;
        }
    }
    startTimer(VerficationCode);
    VerificationMap.insert(std::pair<std::string, std::string>(mail,VerficationCode));
    return true;
}
