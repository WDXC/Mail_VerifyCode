#ifndef SENDMAIL_H
#define SENDMAIL_H

#include <cstdlib>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <signal.h>
#include "TimerHeap.h"

#define BUFSIZE 256
#define MAXNUM 999999
#define MINNUM 100000
#define SECONDTIME 60

class SendMail {
    public:
        static SendMail *getinstance();
        ~SendMail();
        // 初始化socket连接
        bool initSocket(const std::string &HostOrIp, const int &Port);
        // 发送验证码
        bool SendVerificationMail(const std::string &mail);
        // 比对验证码(使用map进行存储,{mail:VerificaitonCode})
        bool CompareVerificationCode(const std::string& mail, const std::string& code);


    private:
        SendMail();
        // 发送消息
        bool sendMsg(const char* msg);
        // 接收返回的消息
        bool recvMsg();
        // 发送方邮箱配置
        bool initconfig();
        // 域名与IP之间的转化
        const std::string HostNameToIp(const std::string &hostname);
        // 生成六位验证码
        bool GenerateVerificationCode(const std::string &mail);


        /**
         * 定时器
         */
        //  初始化定时器
        void initTimer(const std::string& code);
        //  启动定时器
        void startTimer(const std::string& code);
        // 定时器的回调函数
        static void terminalBack();
        // 心跳处理
        void tick();
        // 删除过期验证码
        void DelTimeoutVerifyCode();
       // Alarm信号处理
        void myhandler(int signum);
        static void handler(int signum);

    private:
        TimerHeap m_timeHeap;
        heap_timer* timer;
        client_data tempans;
        std::vector<heap_timer*> TimeoutVerifyCode;
        int ser_sock;
        bool initState;
        char message[BUFSIZE];
        std::string VerficationCode;
        struct sockaddr_in server_address;
        std::unordered_map<std::string, std::string> VerificationMap;
};


#endif // SENDMAIL_H
