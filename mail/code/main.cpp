#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>

#include "TimerHeap.h"
#include "sendmail.h"


int main() {
  SendMail::getinstance()->initSocket("smtp.qq.com", 25);
  while (1) {
          std::cout << "Please input e-mail address: " ;
          std::string res;
          std::cin >> res;
          if (res.compare("quit") == 0) break;
          SendMail::getinstance()->SendVerificationMail(res);
  }
  return 0;
}
