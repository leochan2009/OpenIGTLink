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
  igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
  for (int i = 0; i < 10; i++)
  {
    ts->GetTime();
    igtlUint64 nanoTimePre = ts->GetTimeStampInNanoseconds();
    igtl::Sleep(10);
    ts->GetTime();
    igtlUint64 nanoTimePost = ts->GetTimeStampInNanoseconds();
    std::cerr<<"Time Pre: "<<nanoTimePre<<"Time Post: "<<nanoTimePost<<std::endl;
    EXPECT_GT((nanoTimePost-1e7), nanoTimePre);
  }
  std::cerr<<"Test Passed"<<std::endl;
}


int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}




