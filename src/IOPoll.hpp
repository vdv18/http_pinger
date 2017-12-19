#ifndef __IOPOLL_HPP__
#define __IOPOLL_HPP__

#include "ITimer.hpp"
#include "IFile.hpp"

class IOPoll {
public:
  virtual ~IOPoll(){};
  virtual int poll( void ) = 0;
  virtual void addTimer( ITimer *timer ) = 0;
  virtual void addSocket( ISocket *socket ) = 0;
};

#endif//__IOPOLL_HPP__

