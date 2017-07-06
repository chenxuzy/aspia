//
// PROJECT:         Aspia Remote Desktop
// FILE:            protocol/file_request_sender_remote.cc
// LICENSE:         See top-level directory
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "client/file_request_sender_remote.h"
#include "protocol/message_serialization.h"
#include "base/logging.h"

namespace aspia {

FileRequestSenderRemote::FileRequestSenderRemote(ClientSession::Delegate* session) :
    session_(session)
{
    DCHECK(session_);
}

void FileRequestSenderRemote::SendDriveListRequest(FileReplyReceiver* receiver)
{
    receiver_ = receiver;

    proto::file_transfer::ClientToHost request;
    request.mutable_drive_list_request()->set_dummy(1);
    SendRequest(request);
}

void FileRequestSenderRemote::SendFileListRequest(FileReplyReceiver* receiver,
                                                  const FilePath& path)
{
    receiver_ = receiver;

    proto::file_transfer::ClientToHost request;
    request.mutable_file_list_request()->set_path(path.u8string());
    SendRequest(request);
}

void FileRequestSenderRemote::SendCreateDirectoryRequest(FileReplyReceiver* receiver,
                                                         const FilePath& path)
{
    receiver_ = receiver;

    proto::file_transfer::ClientToHost request;
    request.mutable_create_directory_request()->set_path(path.u8string());
    SendRequest(request);
}

void FileRequestSenderRemote::SendDirectorySizeRequest(FileReplyReceiver* receiver,
                                                       const FilePath& path)
{
    receiver_ = receiver;
    // TODO
}

void FileRequestSenderRemote::SendRemoveRequest(FileReplyReceiver* receiver,
                                                const FilePath& path)
{
    receiver_ = receiver;

    proto::file_transfer::ClientToHost request;
    request.mutable_remove_request()->set_path(path.u8string());
    SendRequest(request);
}

void FileRequestSenderRemote::SendRenameRequest(FileReplyReceiver* receiver,
                                                const FilePath& old_name,
                                                const FilePath& new_name)
{
    receiver_ = receiver;

    proto::file_transfer::ClientToHost request;

    request.mutable_rename_request()->set_old_name(old_name.u8string());
    request.mutable_rename_request()->set_new_name(new_name.u8string());

    SendRequest(request);
}

bool FileRequestSenderRemote::ReadIncommingMessage(const IOBuffer& buffer)
{
    if (!receiver_)
    {
        LOG(ERROR) << "Unexpected message received";
        session_->OnSessionTerminate();
        return false;
    }

    proto::file_transfer::HostToClient message;

    if (!ParseMessage(buffer, message))
    {
        session_->OnSessionTerminate();
        return false;
    }

    if (message.status() != proto::Status::STATUS_SUCCESS)
    {
        receiver_->OnLastRequestFailed(message.status());
    }
    else if (message.has_drive_list())
    {
        std::unique_ptr<proto::DriveList> drive_list(message.release_drive_list());
        receiver_->OnDriveListReply(std::move(drive_list));
    }
    else if (message.has_file_list())
    {
        std::unique_ptr<proto::FileList> file_list(message.release_file_list());
        receiver_->OnFileListReply(std::move(file_list));
    }
    else
    {
        DLOG(WARNING) << "Unhandled message from host";
    }

    receiver_ = nullptr;
    return true;
}

void FileRequestSenderRemote::SendRequest(const proto::file_transfer::ClientToHost& request)
{
    session_->OnSessionMessageAsync(SerializeMessage<IOBuffer>(request));
}

} // namespace aspia
