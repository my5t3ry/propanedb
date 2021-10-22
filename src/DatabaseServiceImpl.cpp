#include <filesystem>
namespace fs = std::filesystem;

#include "DatabaseServiceImpl.h"

DatabaseServiceImpl::DatabaseServiceImpl(string path)
{
  directory = path;
  implementation = new DatabaseImpl(path);
}

DatabaseServiceImpl::~DatabaseServiceImpl()
{
  delete implementation;
}

Metadata DatabaseServiceImpl::GetMetadata(ServerContext *context)
{
  Metadata metadata;
  LOG(INFO) << "GetMetadata from context:" << endl;
  std::multimap<grpc::string_ref, grpc::string_ref> data = context->client_metadata();
  LOG(INFO) << data.size();
  for (auto iter = data.begin(); iter != data.end(); ++iter)
  {
    LOG(INFO) << "Header key: " << iter->first << ", value: " << iter->second;
  }
  auto map = context->client_metadata();
  auto search = map.find("databaseName");
  if (search != map.end())
  {
    metadata.databaseName = (search->second).data();
    LOG(INFO) << "databaseName :" << metadata.databaseName;
  }
  return metadata;
}

grpc::Status DatabaseServiceImpl::CreateDatabase(ServerContext *context, const propane::PropaneDatabase *request,
                                                 propane::PropaneStatus *reply)
{
  Metadata meta = this->GetMetadata(context);
  return implementation->CreateDatabase(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Put(ServerContext *context, const propane::PropanePut *request,
                                      propane::PropaneId *reply)
{
  Metadata meta = this->GetMetadata(context);
  return implementation->Put(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Get(ServerContext *context, const propane::PropaneId *request,
                                      propane::PropaneEntity *reply)
{
  Metadata meta = this->GetMetadata(context);
  return implementation->Get(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Delete(ServerContext *context, const propane::PropaneId *request,
                                         propane::PropaneStatus *reply)
{
  Metadata meta = this->GetMetadata(context);
  return implementation->Delete(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Search(ServerContext *context, const propane::PropaneSearch *request,
                                         propane::PropaneEntities *reply)
{
  Metadata meta = this->GetMetadata(context);
  return implementation->Search(&meta, request, reply);
}
