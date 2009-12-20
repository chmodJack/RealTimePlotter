#ifndef STHREAD_H
#define STHREAD_H

#include <iostream>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "Lock.h"

using std::cerr;
using std::endl;

namespace ST{
  enum{ us, ms, sec};
};

class SThread
{
  
 public:

    SThread() {  
	pthread_mutex_init(&mutex_,NULL);
	pthread_attr_init(&attr_);
	pthread_cond_init(&condition_, NULL);
    }

  virtual ~SThread(){
  }

  virtual void Run()=0;
 
  static void* Init(void* _this){
   SThread* p_object = reinterpret_cast<SThread*>(_this);
   p_object->Run();
   return NULL;
  }

  void Start(){
    int status=-1;
    status = pthread_create(&p_sthread_, NULL, Init, this);
    if(status < 0)
      cerr << "STHREAD: thread creation failed" << endl;
  }    
 
  void Wait(){ pthread_join(p_sthread_, NULL);}

  void Detach(){pthread_detach(p_sthread_);}

  void Destroy(){
    int status;
    pthread_exit(reinterpret_cast<void*>(&status));
  }

  void Lock(pthread_mutex_t& mutex){
    pthread_mutex_lock(&mutex);
  }

  void Unlock(pthread_mutex_t& mutex){
    pthread_mutex_unlock(&mutex);
  }
  
  void Pause(){
    ScopedPThreadLock Lock(mutex_);
    pthread_cond_wait(&condition_, &mutex_);
  }

  void Wake(){ pthread_cond_signal(&condition_);}
  
  void Sleep(int _type = ST::us, long _value = 1000L){
    int status=0;
    
    clock_gettime(CLOCK_REALTIME , &cTime_);

    switch(_type){
    case ST::us:
      fTime_ = cTime_;
      fTime_.tv_nsec += 1000L*_value;
      break;
    case ST::ms:
      fTime_ = cTime_;
      fTime_.tv_nsec += _value*1000000L;

      //hack to fix overflow problem
      if(fTime_.tv_nsec > 1000000000L){
	fTime_.tv_nsec -= 1000000000L;
	fTime_.tv_sec++;
      }
      break;
    case ST::sec:
      fTime_ = cTime_;
      fTime_.tv_sec = cTime_.tv_sec + _value;
       break;
    default:
      cerr << "STHREAD: invalid sleep value. default to 1 sec" << endl;
      fTime_ = cTime_;
      fTime_.tv_sec += 1;
    }

    ScopedPThreadLock Lock(mutex_);
    while(status != ETIMEDOUT){
      status  = pthread_cond_timedwait(&condition_, &mutex_, &fTime_);
    }
    
  }  

  void Priority(int _value);
  void SetCondition(int _value);

 protected:
  pthread_t p_sthread_;
  pthread_mutex_t mutex_;
  pthread_attr_t attr_;
  pthread_cond_t condition_;
  timeval time_now_;
  timespec timeout_;
  timespec newTime_;
  timespec cTime_;
  timespec fTime_;
};

#endif
