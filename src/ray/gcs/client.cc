#include "ray/gcs/client.h"

#include "ray/gcs/redis_context.h"

namespace ray {

namespace gcs {

AsyncGcsClient::AsyncGcsClient(const ClientID &client_id, CommandType command_type) {
  context_ = std::make_shared<RedisContext>();
  client_table_.reset(new ClientTable(context_, this, client_id));
  object_table_.reset(new ObjectTable(context_, this));
  actor_table_.reset(new ActorTable(context_, this));
  task_table_.reset(new TaskTable(context_, this, command_type));
  raylet_task_table_.reset(new raylet::TaskTable(context_, this, command_type));
  task_reconstruction_log_.reset(new TaskReconstructionLog(context_, this));
  heartbeat_table_.reset(new HeartbeatTable(context_, this));
  error_table_.reset(new ErrorTable(context_, this));
  command_type_ = command_type;
}

#if RAY_USE_NEW_GCS
// Use of kChain currently only applies to Table::Add which affects only the
// task table, and when RAY_USE_NEW_GCS is set at compile time.
AsyncGcsClient::AsyncGcsClient(const ClientID &client_id)
    : AsyncGcsClient(client_id, CommandType::kChain) {}
#else
AsyncGcsClient::AsyncGcsClient(const ClientID &client_id)
    : AsyncGcsClient(client_id, CommandType::kRegular) {}
#endif  // RAY_USE_NEW_GCS

AsyncGcsClient::AsyncGcsClient(CommandType command_type)
    : AsyncGcsClient(ClientID::from_random(), command_type) {}

AsyncGcsClient::AsyncGcsClient() : AsyncGcsClient(ClientID::from_random()) {}

Status AsyncGcsClient::Connect(const std::string &address, int port) {
  RAY_RETURN_NOT_OK(context_->Connect(address, port));
  // TODO(swang): Call the client table's Connect() method here. To do this,
  // we need to make sure that we are attached to an event loop first. This
  // currently isn't possible because the aeEventLoop, which we use for
  // testing, requires us to connect to Redis first.
  return Status::OK();
}

Status Attach(plasma::EventLoop &event_loop) {
  // TODO(pcm): Implement this via
  // context()->AttachToEventLoop(event loop)
  return Status::OK();
}

Status AsyncGcsClient::Attach(boost::asio::io_service &io_service) {
  asio_async_client_.reset(new RedisAsioClient(io_service, context_->async_context()));
  asio_subscribe_client_.reset(
      new RedisAsioClient(io_service, context_->subscribe_context()));
  return Status::OK();
}

ObjectTable &AsyncGcsClient::object_table() { return *object_table_; }

TaskTable &AsyncGcsClient::task_table() { return *task_table_; }

raylet::TaskTable &AsyncGcsClient::raylet_task_table() { return *raylet_task_table_; }

ActorTable &AsyncGcsClient::actor_table() { return *actor_table_; }

TaskReconstructionLog &AsyncGcsClient::task_reconstruction_log() {
  return *task_reconstruction_log_;
}

ClientTable &AsyncGcsClient::client_table() { return *client_table_; }

FunctionTable &AsyncGcsClient::function_table() { return *function_table_; }

ClassTable &AsyncGcsClient::class_table() { return *class_table_; }

HeartbeatTable &AsyncGcsClient::heartbeat_table() { return *heartbeat_table_; }

ErrorTable &AsyncGcsClient::error_table() { return *error_table_; }

}  // namespace gcs

}  // namespace ray
