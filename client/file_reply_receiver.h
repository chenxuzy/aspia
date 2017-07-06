//
// PROJECT:         Aspia Remote Desktop
// FILE:            protocol/file_reply_receiver.h
// LICENSE:         See top-level directory
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#ifndef _ASPIA_CLIENT__FILE_REPLY_RECEIVER_H
#define _ASPIA_CLIENT__FILE_REPLY_RECEIVER_H

#include <base/files/file_path.h>
#include "proto/file_transfer_session.pb.h"

#include <memory>

namespace aspia {

class FileReplyReceiver
{
public:
    virtual ~FileReplyReceiver() = default;

    virtual void OnLastRequestFailed(proto::Status status) = 0;

    virtual void OnDriveListReply(std::unique_ptr<proto::DriveList> drive_list) = 0;

    virtual void OnFileListReply(std::unique_ptr<proto::FileList> file_list) = 0;

    virtual void OnCreateDirectoryReply() = 0;

    virtual void OnRemoveReply() = 0;

    virtual void OnRenameReply() = 0;
};

} // namespace aspia

#endif // _ASPIA_CLIENT__FILE_REPLY_RECEIVER_H
