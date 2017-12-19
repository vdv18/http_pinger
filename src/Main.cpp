#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include  <algorithm>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <cctype>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include<signal.h>
#include<unistd.h>

#include <queue>
#include <list>
#include <set>

#include <sys/time.h>
#include <sys/resource.h>

#include "IOPoll.hpp"
#include "IFile.hpp"
#include "ITimer.hpp"

#define DEFAULT_TIMEOUT (10)


class SelectPoll: public IOPoll {
  fd_set rdfds,wrfds,exfds;
  struct timeval tv,timeout;

  int run;
  int retval;

  int fd_max;

  std::list<ISocket*> sockets;
  std::list<IFile*>   files;
  std::list<ITimer*>  timers;
  
private:
  virtual void addTimer( ITimer *timer ){ timers.push_front(timer); }
  virtual void addSocket( ISocket *socket ){ sockets.push_front(socket); }
  void poll_prepare(){
    int tasks = 0;
    struct timeval curr, timeout, result, timemin;
    gettimeofday( &curr, NULL );

    tv.tv_sec  = 0;
    tv.tv_usec = 100*1000;

    fd_max     = 0;
    tasks = 0;
    FD_ZERO(&rdfds);
    FD_ZERO(&wrfds);
    FD_ZERO(&exfds);


    for(auto it=timers.begin(); it != timers.end(); ++it){
      //ITimer *timer = *it;
      tasks++;
      if((*it)->GetTimeout(&timeout) == 0){
        if((*it)->isComplete())
        {
          timers.remove(*it);
          it = timers.begin();
          continue;
        }
        // Timer is set
        timeval_subtract( &result, &timeout, &curr );
        if( (result.tv_sec < timemin.tv_sec) || ( result.tv_sec == timemin.tv_sec && result.tv_usec < timemin.tv_usec ) ){
          timemin.tv_sec = result.tv_sec;
          timemin.tv_usec = result.tv_sec;
        }
      }
    }

    //for(auto it=sockets.begin(); it != sockets.end(); ++it){
    for(auto it=sockets.begin(), end=sockets.end(); it != end; ++it){
      tasks++;
      if((*it)->isClose())
      {
        sockets.remove(*it);
        it = sockets.begin();
        continue;
      }
      if(fd_max < (*it)->getfd())
        fd_max = (*it)->getfd();

      if( (*it)->getReadReady() )
        FD_SET( (*it)->getfd(), &rdfds );
      if( (*it)->getWriteReady() )
        FD_SET( (*it)->getfd(), &wrfds );
      if( (*it)->getExceptReady() )
        FD_SET( (*it)->getfd(), &exfds );
    }
    if(tasks == 0)
    {
      //std::cout << "Complete IPoll Handler. " << std::endl;
      tv.tv_sec  = 0;
      tv.tv_usec = 1;
      run = 0;
    }
    /*
    for(auto it=files.begin(); it != files.end(); ++it){

      if(fd_max < (*it)->getfd())
        fd_max = (*it)->getfd();

      if( (*it)->getReadReady() )
        FD_SET( (*it)->getfd(), &rdfds );
      if( (*it)->getWriteReady() )
        FD_SET( (*it)->getfd(), &wrfds );
      if( (*it)->getExceptReady() )
        FD_SET( (*it)->getfd(), &exfds );

    }*/
  }
  int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
  {
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
      y->tv_usec -= 1000000 * nsec;
      y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
      int nsec = (x->tv_usec - y->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
  }

  void poll_process_timers(){
    struct timeval curr, result, value;
    gettimeofday( &curr, NULL );
    for(auto it=timers.begin(); it != timers.end(); ++it){
      if((*it)->GetTimeout(&value) == 0){
        // Timer is set
        if(timeval_subtract( &result, &value, &curr ) == 1){
          (*it)->ResetTimer();
          (*it)->TimerCallback( *it );
        }
      }
    }
  }

  void poll_process_fd(){
    for(auto it=sockets.begin(); it != sockets.end(); ++it){

      if( (*it)->getReadReady()   && FD_ISSET( (*it)->getfd(), &rdfds ) ){
        (*it)->recv();
      }
      if( (*it)->getWriteReady()  && FD_ISSET( (*it)->getfd(), &wrfds ) ){
        (*it)->send();
      }
      if( (*it)->getExceptReady() && FD_ISSET( (*it)->getfd(), &exfds ) ){
        (*it)->excp();
      }
    }
/*
    for(auto it=files.begin(); it != files.end(); ++it){
      if( (*it)->getReadReady()   && FD_ISSET( (*it)->getfd(), &rdfds ) ){
        (*it)->read();
      }
      if( (*it)->getWriteReady()  && FD_ISSET( (*it)->getfd(), &wrfds ) ){
        (*it)->write();
      }
      if( (*it)->getExceptReady() && FD_ISSET( (*it)->getfd(), &exfds ) ){
        (*it)->excp();
      }
    }
*/
  }

  void poll_process_errors( int retval ){
    switch( retval ){
      case ENOMEM: std::cout << "SelectPollError: Unable to allocate memory for internal tables." << std::endl; break;
      case EINVAL: std::cout << "SelectPollError: The value contained within timeout is invalid. OR nfds is negative or exceeds the RLIMIT_NOFILE resource limit." << std::endl; break;
      case EBADF:  std::cout << "SelectPollError: An invalid file descriptor was given in one of the sets." << std::endl;  break;
      case EINTR:
        {
          poll_process_timers();
        } break;
    };
    switch( retval ){
      case ENOMEM:
      case EINVAL:
      case EBADF:
        {
          run = 0;
        } break;
    };
  }
public:
  SelectPoll():run(1){
  }
  ~SelectPoll(){
  }
  int poll(){
    while(run){
      poll_prepare();
      retval = select( fd_max+1, &rdfds, &wrfds, &exfds, &tv );
      if( retval >= 0)
        poll_process_timers();
      if ( retval > 0 ) {
        poll_process_fd();
      } else if ( retval < 0) {
        poll_process_errors( retval );
      }
    }
  }
};


class HTTPPinger: public ISocket, public ITimer {
  typedef enum HTTPPingerState_e {
    HTTP_PINGER_START,
    HTTP_PINGER_PROCESS,
    HTTP_PINGER_WORK,
    HTTP_PINGER_COMPLETE,
  } HTTPPingerState_t;
  std::string 		url, hostname, path, protocol;
  HTTPPingerState_t 	state;
  unsigned int		delay_min, delay_max, delay_avr;
  unsigned int          requests,requests_complete,requests_err;
  unsigned int          delay;
  int port;
  int retval;
  int sock;
  int timeout;
  bool readReady, writeReady, exceptReady;
  bool is_close;

  struct sockaddr_in addr;
  struct timeval timestamp;
  void parse(const std::string& url_s){
    const std::string prot_end("://");
    const std::string port_start(":");
    std::string::const_iterator prot_i = std::search(url_s.begin(), url_s.end(),
                                           prot_end.begin(), prot_end.end());
    protocol.reserve(distance(url_s.begin(), prot_i));
    std::transform(url_s.begin(), prot_i, back_inserter(protocol), ::tolower);
    std::advance(prot_i, prot_end.length());
    std::string::const_iterator path_i = std::find(prot_i, url_s.end(), '/');
    std::string::const_iterator port_i = std::find(prot_i, url_s.end(), ':');
    if(port_i != url_s.end())
    {
      std::string temp;
      temp.reserve(distance(port_i, path_i));
      std::transform(port_i, path_i, back_inserter(temp), ::tolower);
      temp = temp.substr(1);
      port = std::stoi(temp);
      hostname.reserve(distance(prot_i, port_i));
      std::transform(prot_i, port_i, back_inserter(hostname), ::tolower);
    }
    else
    {
      hostname.reserve(distance(prot_i, path_i));
      std::transform(prot_i, path_i, back_inserter(hostname), ::tolower);
    }
    if(path_i != url_s.end()){
      path = "/";
    }
  }
  int start_ping(){
    struct timeval tv;
    struct hostent *host;
    ResetTimer();
    host = gethostbyname(hostname.c_str());
    if(!host)
      return -1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ((in_addr*)host->h_addr_list[0])->s_addr;
    readReady = writeReady = exceptReady = false;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock <= 0)
    {
      return -1;
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    SetTimer(&tv);
  }
  int start_delay( int err ){
    struct timeval tv;
    readReady = writeReady = exceptReady = false;
    ResetTimer();
    requests_complete++;
    requests_err+=err;
    if(requests_complete >= requests){
      delay_avr /= requests_complete;
      state = HTTP_PINGER_COMPLETE;
    }
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    SetTimer(&tv);
  }
  int connect_to_port(){
    struct timeval tv;
    ResetTimer();
    gettimeofday(&timestamp,NULL);
    readReady = false;
    exceptReady = false;
    writeReady  = true;
    addr.sin_port = htons(port);
    retval = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if(retval < 0 && errno != EINPROGRESS){
      start_delay(1);
      return -1;
    }
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    SetTimer(&tv);
    return 0;
  }
  int send_http_head(){
    struct timeval tv;
    ResetTimer();
    std::string str;
    str.append("HEAD ");
    str.append(path);
    str.append(" HTTP/1.1\r\n");
    str.append("Host: ");
    str.append(hostname);
    str.append("\r\n");
    str.append("\r\n");
    ::send(sock, str.c_str(), str.length(), 0);
    readReady   = true;
    exceptReady = false;
    writeReady  = false;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    SetTimer(&tv);
  }
public:
  HTTPPinger(std::string url_, unsigned int requests_, unsigned int delay_):port(80),requests(requests_),delay(delay_),url(url_){
    timeout = DEFAULT_TIMEOUT;
    requests_complete =requests_err = 0;
    delay_min = delay_max = delay_avr = 0;
    parse(url_);
    if(protocol.compare("http") != 0)
      throw std::string("Unsupported protocol.");
    is_close = false;
    state = HTTP_PINGER_START;
    if(start_ping() == -1)
    {
      start_delay(1);
      return;
    }
  };
  HTTPPinger(std::string url_, unsigned int requests_, unsigned int delay_, int timeout_):port(80),requests(requests_),delay(delay_),url(url_){
    timeout = timeout_;
    requests_complete =requests_err = 0;
    delay_min = delay_max = delay_avr = 0;
    parse(url_);
    if(protocol.compare("http") != 0)
      throw std::string("Unsupported protocol.");
    is_close = false;
    state = HTTP_PINGER_START;
    if(start_ping() == -1)
    {
      start_delay(1);
      return;
    }
  };
  virtual ~HTTPPinger(){
    state = HTTP_PINGER_COMPLETE;
  };
  virtual int TimerCallback( ITimer *timer ){
    switch(state){
      case HTTP_PINGER_PROCESS:
        {
          ResetTimer();
          if(start_ping() == -1)
          {
            state = HTTP_PINGER_WORK;
            start_delay(1);
            break;
          }
          state = HTTP_PINGER_WORK;
          start_delay(1);
        } break;
      case HTTP_PINGER_WORK:
        {
          ResetTimer();
          if(start_ping() == -1)
          {
            start_delay(1);
            return 0;
          }
          state = HTTP_PINGER_PROCESS;
          connect_to_port();
        } break;
      default:break;
    }
    return 0;
  };
  virtual int excp(){
    return 0;
  };
  virtual int read(){ return 0; };
  virtual int write(){ return 0; };
  virtual bool isComplete() { return isClose(); };
  virtual bool isClose() {
    switch(state){
      case HTTP_PINGER_COMPLETE:
        {
          return true;
        }break;
    };
    return false;
  }
  virtual int recv(){
    switch(state){
      case HTTP_PINGER_WORK:break;
      case HTTP_PINGER_PROCESS:
        state = HTTP_PINGER_WORK;
        {
          int len = 0;
          unsigned int tv_delay;// = (tv.tv_sec - timestamp.tv_sec)*1000 + (tv.tv_usec - timestamp.tv_usec)/1000;
          struct timeval tv;
          char buffer[1024];
          char *p;
          memset(buffer,0,sizeof(buffer));
          len = ::recv(sock,buffer,sizeof(buffer),0);
          close(sock);
          gettimeofday(&tv,NULL);
          if(len){
            p = strstr(buffer," 200 ");
            if(p){
              tv_delay = (tv.tv_sec - timestamp.tv_sec)*1000;
              if(tv.tv_usec >=  timestamp.tv_usec)
                tv_delay += (tv.tv_usec - timestamp.tv_usec)/1000;
              else
                tv_delay -= (timestamp.tv_usec - tv.tv_usec)/1000;
              if(requests_complete == 0)
              {
                delay_avr = 0;
                delay_min = delay_max = tv_delay;
              }
              if(tv_delay < delay_min)
                delay_min = tv_delay;
              if(tv_delay > delay_max)
                delay_max = tv_delay;
              delay_avr += tv_delay;
            } else {
              len = 0;
            }
          }
          if(len == 0)
          {
            start_delay(1);
          }
          else
          {
            start_delay(0);
          }
        } break;
      default:break;
    }
    return 0;
  };
  virtual int send(){
    switch(state){
      case HTTP_PINGER_PROCESS:
        {
          send_http_head();
        } break;
      case HTTP_PINGER_WORK:break;
      default:break;
    }
    return 0;
  };
  virtual int getfd(){
    switch(state){
      case HTTP_PINGER_START:
        {
          state = HTTP_PINGER_WORK;
          if(start_ping() == -1)
          {
            start_delay(1);
            return sock;
          }
          state = HTTP_PINGER_PROCESS;
          connect_to_port();
        } break;
      default:break;
    }
    return sock;
  };

  virtual bool getReadReady(){return readReady;};
  virtual bool getWriteReady(){return writeReady;};
  virtual bool getExceptReady(){return exceptReady;};

  void printResult(){
    std::cout << " RESULT: err:" << requests_err;
    std::cout << "\tmin: " << delay_min << "\tmax: " << delay_max << "\tavr: " << delay_avr << "\tURL:\t"<< url <<std::endl;
  }
};

void print_help() {
  std::cout << "-h  : print list options." << std::endl;
  std::cout << "-f  : file with list of URLs." << std::endl;
  std::cout << "-n  : number of HTTP HEAD method pings." << std::endl;
  std::cout << "-i  : (seconds) interval time betwen HTTP HEAD method pings." << std::endl;
}

static std::string file_name;
static int ping_count = 3;
static int sec_interval_time = 5;

int parse(int argc, char **argv) {
  int index = 0;
  if(argc < 3)
    return -1;
  while(++index < argc){
    if(argv[index][0] = "-" && strlen(argv[index]) == 2){
      switch(argv[index][1]){
        case 'h':
          return -1;
        case 'f':
          index++;
          file_name.append(argv[index]);
          break;
        case 'n':
          index++;
          ping_count = atoi(argv[index]);
          break;
        case 'i':
          index++;
          sec_interval_time = atoi(argv[index]);
          break;
      }
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  if(parse(argc,argv) != 0)
  {
    print_help();
    return 0;
  }

  std::list<HTTPPinger*> pingers;
  int number = 0;
  std::ifstream file;
  std::string url;
  file.open(file_name, std::ifstream::in);
  while((++number < (int)(1000)) && (file >> url)){
    try {
      HTTPPinger *p = new HTTPPinger(url,ping_count,sec_interval_time);
      pingers.push_back(p);
    } catch ( std::string *err ) {
      std::cout<< "Invalid URL: " << url << " give error " << err << std::endl;
    }
  }
  file.close();

  IOPoll *poll = new SelectPoll();
  for(auto it=pingers.begin();it!=pingers.end();++it)
  {
    poll->addTimer(*it);
    poll->addSocket(*it);
  }

  try{
    poll->poll();
  } catch (std::string str) {
    std::cout << "Exception: " << str << std::endl;
  }

  for(auto it=pingers.begin();it!=pingers.end();++it)
  {
    (*it)->printResult();
  }

  while(! pingers.empty()){
    HTTPPinger *p = pingers.front();
    pingers.pop_front();
    delete p;
  }
  delete poll;
  return 0;
}
