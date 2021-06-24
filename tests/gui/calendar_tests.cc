#include <gtest/gtest.h>
#include <windows.h>
#include <ngl_os.h>
#include <sys/time.h>
#include <calendar.h>
#include <inputeventlabels.h>

class CALENDAR:public testing::Test{

   public :
   virtual void SetUp(){
   }
   virtual void TearDown(){
   }
};

TEST_F(CALENDAR,1970){
   Calendar cal;
   cal.set(1970,0,1,0,0,0);
   ASSERT_EQ(cal.getTime(),0);
   cal.setTimeZone(3600*8);
   cal.set(1970,0,1,0,0,0);
   ASSERT_EQ(cal.getTime(),0);
}
