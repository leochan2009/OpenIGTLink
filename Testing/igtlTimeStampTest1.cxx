/*=========================================================================

  Program:   OpenIGTLink Library
  Language:  C++

  Copyright (c) Insight Software Consortium. All rights reserved.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include <igtlTimeStamp.h>
#include "igtlOSUtil.h"
#include "igtlTestConfig.h"

#include <iostream>

TEST(TimeStampTest, SetTime){
  // Simply testing Setter/Getter
  igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
  ts->GetTime();

  igtlUint64 totalNanosSinceEpochBefore = ts->GetTimeStampInNanoseconds();
  ts->SetTime(0.0);
  ts->SetTimeInNanoseconds(totalNanosSinceEpochBefore);

  igtlUint64 totalNanosSinceEpochAfter = ts->GetTimeStampInNanoseconds();
  EXPECT_EQ(totalNanosSinceEpochAfter, totalNanosSinceEpochBefore);
  if (totalNanosSinceEpochAfter != totalNanosSinceEpochBefore)
  {
    std::cerr << "Expected totalNanosSinceEpochBefore=" << totalNanosSinceEpochBefore << " to equal totalNanosSinceEpochAfter=" << totalNanosSinceEpochAfter << std::endl;
  }
}
TEST(TimeStampTest, GetTimeInterval)
{
  for (int i = 0; i < 10; i++)
  {
    igtl::TimeStamp::Pointer ts_pre = igtl::TimeStamp::New();
    ts_pre->GetTime();
    igtlUint64 nanoTimePre = ts_pre->GetTimeStampInNanoseconds();
    igtl::Sleep(20);
    igtl::TimeStamp::Pointer ts_post = igtl::TimeStamp::New();
    ts_post->GetTime();
    igtlUint64 nanoTimePost = ts_post->GetTimeStampInNanoseconds();
    std::cerr << "Time Pre: " << nanoTimePre << "Time Post: " << nanoTimePost << std::endl;
    EXPECT_NE(nanoTimePost, nanoTimePre);
  }
}


int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}




