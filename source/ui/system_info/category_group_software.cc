//
// PROJECT:         Aspia Remote Desktop
// FILE:            ui/system_info/category_group_software.cc
// LICENSE:         Mozilla Public License Version 2.0
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "protocol/system_info_constants.h"
#include "proto/system_info_session_message.pb.h"
#include "ui/system_info/category_group_software.h"
#include "ui/system_info/category_info.h"
#include "ui/system_info/output_proxy.h"
#include "ui/resource.h"

namespace aspia {

class CategoryPrograms : public CategoryInfo
{
public:
    CategoryPrograms()
        : CategoryInfo(system_info::software::kPrograms, "Programs", IDI_APPLICATIONS)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Parameter", 200);
        column_list->emplace_back("Value", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        // TODO
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryPrograms);
};

class CategoryUpdates : public CategoryInfo
{
public:
    CategoryUpdates()
        : CategoryInfo(system_info::software::kUpdates, "Updates", IDI_APPLICATIONS)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Parameter", 200);
        column_list->emplace_back("Value", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        // TODO
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryUpdates);
};

class CategoryServices : public CategoryInfo
{
public:
    CategoryServices() : CategoryInfo(system_info::software::kServices, "Services", IDI_GEAR)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Display Name", 200);
        column_list->emplace_back("Name", 200);
        column_list->emplace_back("Description", 200);
        column_list->emplace_back("Status", 200);
        column_list->emplace_back("Startup Type", 200);
        column_list->emplace_back("Account", 200);
        column_list->emplace_back("Executable File", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        system_info::Services message;

        if (!message.ParseFromString(data))
            return;

        Output::Table table(output, Name(), column_list());

        for (int index = 0; index < message.item_size(); ++index)
        {
            const system_info::Services::Item& item = message.item(index);

            Output::Row row(output, Icon());

            output->AddValue(item.display_name());
            output->AddValue(item.name());
            output->AddValue(item.description());
            output->AddValue(StatusToString(item.status()));
            output->AddValue(StartupTypeToString(item.startup_type()));
            output->AddValue(item.start_name());
            output->AddValue(item.binary_path());
        }
    }

    static const char* StatusToString(system_info::Services::Item::Status status)
    {
        switch (status)
        {
            case system_info::Services::Item::STATUS_CONTINUE_PENDING:
                return "Continue Pending";

            case system_info::Services::Item::STATUS_PAUSE_PENDING:
                return "Pause Pending";

            case system_info::Services::Item::STATUS_PAUSED:
                return "Paused";

            case system_info::Services::Item::STATUS_RUNNING:
                return "Running";

            case system_info::Services::Item::STATUS_START_PENDING:
                return "Start Pending";

            case system_info::Services::Item::STATUS_STOP_PENDING:
                return "Stop Pending";

            case system_info::Services::Item::STATUS_STOPPED:
                return "Stopped";

            default:
                return "Unknown";
        }
    }

    static const char* StartupTypeToString(system_info::Services::Item::StartupType startup_type)
    {
        switch (startup_type)
        {
            case system_info::Services::Item::STARTUP_TYPE_AUTO_START:
                return "Auto Start";

            case system_info::Services::Item::STARTUP_TYPE_DEMAND_START:
                return "Demand Start";

            case system_info::Services::Item::STARTUP_TYPE_DISABLED:
                return "Disabled";

            case system_info::Services::Item::STARTUP_TYPE_BOOT_START:
                return "Boot Start";

            case system_info::Services::Item::STARTUP_TYPE_SYSTEM_START:
                return "System Start";

            default:
                return "Unknown";
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryServices);
};

class CategoryDrivers : public CategoryInfo
{
public:
    CategoryDrivers() : CategoryInfo(system_info::software::kDrivers, "Drivers", IDI_PCI)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Display Name", 200);
        column_list->emplace_back("Name", 200);
        column_list->emplace_back("Description", 200);
        column_list->emplace_back("Status", 200);
        column_list->emplace_back("Startup Type", 200);
        column_list->emplace_back("Executable File", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        system_info::Services message;

        if (!message.ParseFromString(data))
            return;

        Output::Table table(output, Name(), column_list());

        for (int index = 0; index < message.item_size(); ++index)
        {
            const system_info::Services::Item& item = message.item(index);

            Output::Row row(output, Icon());

            output->AddValue(item.display_name());
            output->AddValue(item.name());
            output->AddValue(item.description());
            output->AddValue(CategoryServices::StatusToString(item.status()));
            output->AddValue(CategoryServices::StartupTypeToString(item.startup_type()));
            output->AddValue(item.binary_path());
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryDrivers);
};

class CategoryProcesses : public CategoryInfo
{
public:
    CategoryProcesses()
        : CategoryInfo(system_info::software::kProcesses, "Processes", IDI_SYSTEM_MONITOR)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Parameter", 200);
        column_list->emplace_back("Value", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        // TODO
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryProcesses);
};

class CategoryLicenses : public CategoryInfo
{
public:
    CategoryLicenses()
        : CategoryInfo(system_info::software::kLicenses, "Licenses", IDI_LICENSE_KEY)
    {
        ColumnList* column_list = mutable_column_list();
        column_list->emplace_back("Parameter", 200);
        column_list->emplace_back("Value", 200);
    }

    void Parse(std::shared_ptr<OutputProxy> output, const std::string& data) final
    {
        // TODO
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CategoryLicenses);
};

CategoryGroupSoftware::CategoryGroupSoftware()
    : CategoryGroup("Software", IDI_SOFTWARE)
{
    CategoryList* child_list = mutable_child_list();
    child_list->emplace_back(std::make_unique<CategoryPrograms>());
    child_list->emplace_back(std::make_unique<CategoryUpdates>());
    child_list->emplace_back(std::make_unique<CategoryServices>());
    child_list->emplace_back(std::make_unique<CategoryDrivers>());
    child_list->emplace_back(std::make_unique<CategoryProcesses>());
    child_list->emplace_back(std::make_unique<CategoryLicenses>());
}

} // namespace aspia
