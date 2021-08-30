#include <gtest/gtest.h>
#include <windows.h>
#include <ngl_os.h>
#include <sys/time.h>
#include <core/systemclock.h>
#include <cdlog.h>
#include <functional>
#include <core/callbackbase.h>

class CALLBACK:public testing::Test{

   public :
   virtual void SetUp(){
   }
   virtual void TearDown(){
   }
};

TEST_F(CALLBACK,loop){
   CallbackBase<void> c0([](){});
   CallbackBase<void,int> c1([](int a){printf("a=%d\r\n",a);});
   CallbackBase<void,int,int>cb2([](int a,int b){printf("a=%d,b=%d\r\n",a,b);});
   CallbackBase<int,int,int>cbr2([](int a,int b){return a+b;});
   c1(123);
   cb2(123,456);
   printf("cbr2=%d\r\n",cbr2(234,345));
   cbr2=[](int a,int b){return a*b;};
   printf("cbr2=%d\r\n",cbr2(234,345));
}

TEST_F(CALLBACK,eq){
   CallbackBase<void>c0([](){});
   CallbackBase<void>c1(c0);
   CallbackBase<void>c2([](){});
   ASSERT_TRUE(c0==c1);

   ASSERT_FALSE(c0==c2);
   ASSERT_FALSE(c1==c2);

   c2=c1;
   ASSERT_TRUE(c2==c1);
   ASSERT_TRUE(c2==c0);
}
