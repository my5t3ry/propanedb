#include <filesystem>
#include <cstdio>
namespace fs = std::filesystem;

#include "DatabaseServiceImpl.hpp"
#include "BackupReader.hpp"
#include "FileWriter.hpp"
#include "SimpleAuth0MetadataAuthProcessor.hpp"

struct Const {
  static const std::string &TokenKeyName() {
    static std::string _("Authorization");
    return _;
  }
};
DatabaseServiceImpl::DatabaseServiceImpl(const string &databasePath,
                                         const string &backupPath, bool debug,
                                         SimpleAuth0MetadataAuthProcessor *auth)
    : databasePath(databasePath), backupPath(backupPath), auth(auth) {
  implementation = new DatabaseImpl(databasePath, backupPath, debug);
  this->debug = debug;
  this->auth = new SimpleAuth0MetadataAuthProcessor();
}

DatabaseServiceImpl::~DatabaseServiceImpl() { delete implementation; }

Metadata DatabaseServiceImpl::GetMetadata(ServerContext *context) {
  Metadata metadata;
  auto map = context->client_metadata();
  auto search = map.find("database-name");
  if (search != map.end()) {
    const char *data = (search->second).data();
    // LOG(INFO) << "length= " << (search->second).length();
    metadata.databaseName = std::string(data, (search->second).length());
    if (debug) {
      LOG(INFO) << "databaseName :" << metadata.databaseName;
    }
  }

  auto map2 = context->client_metadata();
  auto token = map2.find("authorization");
  if (token != map2.end()) {
    const char *data = (token->second).data();
    // LOG(INFO) << "length= " << (search->second).length();
    metadata.authToken = std::string(data, (token->second).length());
    if (debug) {
      LOG(INFO) << "databaseName :" << metadata.databaseName;
    }
  }
  return metadata;
}

grpc::Status DatabaseServiceImpl::CreateDatabase(
    ServerContext *context, const propane::PropaneDatabaseRequest *request,
    propane::PropaneStatus *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->CreateDatabase(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::UpdateDatabase(
    ServerContext *context, const propane::PropaneDatabaseRequest *request,
    propane::PropaneStatus *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->UpdateDatabase(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::DeleteDatabase(
    ServerContext *context, const propane::PropaneDatabaseRequest *request,
    propane::PropaneStatus *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->DeleteDatabase(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Put(ServerContext *context,
                                      const propane::PropanePut *request,
                                      propane::PropaneId *reply) {
  Metadata meta = this->GetMetadata(context);
  return implementation->Put(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Get(ServerContext *context,
                                      const propane::PropaneId *request,
                                      propane::PropaneEntity *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->Get(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Delete(ServerContext *context,
                                         const propane::PropaneId *request,
                                         propane::PropaneStatus *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->Delete(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Search(ServerContext *context,
                                         const propane::PropaneSearch *request,
                                         propane::PropaneEntities *reply) {
  Metadata meta = this->GetMetadata(context);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return implementation->Search(&meta, request, reply);
}

grpc::Status DatabaseServiceImpl::Backup(
    ServerContext *context, const ::propane::PropaneBackupRequest *request,
    ::grpc::ServerWriter<::propane::PropaneBackupReply> *writer) {
  Metadata meta = this->GetMetadata(context);
  string databaseName = request->databasename();
  string zipFilePath = "/tmp/backup.zip";
  implementation->Backup(&meta, databaseName, zipFilePath);
  BackupReader reader(zipFilePath, writer);

  const size_t chunk_size = 1UL << 20;  // Hardcoded to 1MB, which seems to be
                                        // recommended from experience.
  reader.Read(chunk_size);

  if (remove(zipFilePath.c_str()) != 0) {
    LOG(ERROR) << "Error deleting file" << endl;
  }
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return grpc::Status::OK;
}
grpc::Status DatabaseServiceImpl::Restore(
    ServerContext *context,
    ::grpc::ServerReader<::propane::PropaneRestoreRequest> *reader,
    ::propane::PropaneRestoreReply *response) {
  FileWriter writer;

  Metadata meta = this->GetMetadata(context);

  string databaseName = "restore";
  string zipFilePath = "/tmp/" + databaseName + ".zip";

  if (debug) {
    LOG(INFO) << "Restore: zipFilePath = " << zipFilePath << endl;
  }

  writer.OpenIfNecessary(zipFilePath);
  propane::PropaneRestoreRequest contentPart;
  reader->SendInitialMetadata();
  while (reader->Read(&contentPart)) {
    databaseName = contentPart.chunk().databasename();
    auto d = contentPart.chunk().data();
    writer.Write(d);
  }

  if (debug) {
    LOG(INFO) << "Restore: databaseName = " << databaseName << endl;
  }

  implementation->Restore(&meta, databasePath + "/" + databaseName,
                          zipFilePath);
  const grpc::Status &status = this->auth->ProcessMeta(meta.authToken);
  if (!status.ok()) {
    return status;
  }
  return grpc::Status::OK;
}
