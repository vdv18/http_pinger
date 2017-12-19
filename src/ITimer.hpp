#ifndef __ITIMER_HPP__
#define __ITIMER_HPP__

class ITimer {
  int is_time_set;
  struct timeval timestamp;
  struct timeval time, timeout;
public:
  ITimer():is_time_set(0){};
  virtual ~ITimer(){};
  virtual bool isComplete(){
    return false;
  };
  int SetTimer( struct timeval *tv ) {
    gettimeofday( &timestamp, NULL );
    time.tv_sec  = tv->tv_sec;
    time.tv_usec = tv->tv_usec;
    timeout.tv_sec  = time.tv_sec  + timestamp.tv_sec;
    timeout.tv_usec = time.tv_usec + timestamp.tv_usec;
    is_time_set = 1;
    return 0;
  }
  int GetTimer( struct timeval *tv ) {
    if(is_time_set != 1)
      return -1;
    memcpy( tv, &time, sizeof(struct timeval) );
    return 0;
  }
  int GetTimeout( struct timeval *tv ) {
    if(is_time_set != 1)
      return -1;
    memcpy( tv, &timeout, sizeof(struct timeval) );
    return 0;
  }
  int ResetTimer( ) {
    return is_time_set = 0;
  }
  virtual int TimerCallback( ITimer *timer ){ return 0;};
};

#endif//__ITIMER_HPP__
