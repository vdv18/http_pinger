#ifndef __IFILE_HPP__
#define __IFILE_HPP__

class IFileDescriptor {
public:
  virtual int getfd(){return -1;};

  virtual bool getReadReady(){return false;};
  virtual bool getWriteReady(){return false;};
  virtual bool getExceptReady(){return false;};

  virtual bool isClose(){return false;};
};

class ISocket: public IFileDescriptor {
public:
  //virtual int connect(const struct sockaddr *addr, socklen_t addrlen) = 0;
  virtual int excp(){return 0;};
  virtual int recv(){return 0;};
  virtual int send(){return 0;};
};

class IFile: public IFileDescriptor {
public:
  //virtual int open( std::string filename, std::ios_base::openmode mode ) = 0;
  virtual int excp(){return 0;};
  virtual int read(){return 0;};
  virtual int write(){return 0;};
};

#endif//__IFILE_HPP__
