#include <iostream>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <streambuf>
#include <gtest/gtest.h>
#include "test.pb.h"
#include "DatabaseImpl.hpp"
#include <boost/filesystem.hpp>
#include <chrono>
#include <thread>

using namespace std;

class UnitTest : public ::testing::Test
{
protected:
  UnitTest()
  {
    // You can do set-up work for each test here.
  }

  ~UnitTest() override
  {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  void SetUp() override
  {
   LOG(INFO) << "SetUp" << std::endl;
    std::string dir("/tmp/test1");
    try
    {
      if (boost::filesystem::exists(dir))
      {
        boost::filesystem::remove_all(dir);
      }
      boost::filesystem::create_directory(dir);
    }
    catch (boost::filesystem::filesystem_error const &e)
    {
      LOG(INFO) << "Error: " << e.what() << std::endl;
    }
    db = new DatabaseImpl(dir, dir, true);
  }

  void TearDown() override
  {
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
     LOG(INFO) << "Teardown" << std::endl;
    delete db;


    
  }

  DatabaseImpl *db;
};

TEST_F(UnitTest, PutGet)
{
  std::string id;

  test::TestEntity item;
  string description("Test PropaneDB");
  item.set_description(description);

  Metadata meta;
  meta.databaseName = "test";

  {
    std::ifstream t("descriptor.bin");
    std::string descriptor((std::istreambuf_iterator<char>(t)),
                           std::istreambuf_iterator<char>());

    google::protobuf::FileDescriptorSet *fd = new google::protobuf::FileDescriptorSet;
    fd->ParseFromString(descriptor);
    propane::PropaneDatabase request;
    request.set_allocated_descriptor_set(fd);
    request.set_databasename("test");
    propane::PropaneStatus reply;
    grpc::Status s = db->CreateDatabase(&meta, &request, &reply);
  }

  {
    propane::PropanePut request;
    propane::PropaneEntity *entity = new propane::PropaneEntity();
    Any *anyMessage = new Any();
    anyMessage->PackFrom(item);

    entity->set_allocated_data(anyMessage);
    request.set_allocated_entity(entity);
    request.set_databasename("test");
    propane::PropaneId reply;

    grpc::Status s = db->Put(&meta, &request, &reply);
    LOG(INFO) << "Put: Status=" << s.error_message() << std::endl;
    EXPECT_EQ(s.ok(), true);
    EXPECT_GT(reply.id().length(), 0);
    id = reply.id();
  }

  {
    propane::PropaneId request;
    request.set_id(id);
    request.set_databasename("test");

    propane::PropaneEntity reply;
    //Metadata meta;
    grpc::Status s = db->Get(&meta, &request, &reply);

    EXPECT_EQ(s.ok(), true);
    LOG(INFO) << "Get: any receive: " << reply.data().DebugString() << std::endl;
  }

 
}


int main(int argc, char **argv)
{
  FLAGS_logtostderr = 1;
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}