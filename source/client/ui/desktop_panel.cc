//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "client/ui/desktop_panel.h"
#include "base/logging.h"
#include "client/ui/desktop_settings.h"
#include "client/ui/select_screen_action.h"

#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

namespace client {

DesktopPanel::DesktopPanel(proto::SessionType session_type, QWidget* parent)
    : QFrame(parent),
      session_type_(session_type)
{
    ui.setupUi(this);

    ui.action_autoscroll->setChecked(settings_.autoScrolling());

    scale_ = settings_.scale();

    // Sending key combinations is available only in desktop management.
    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
        ui.action_send_key_combinations->setChecked(settings_.sendKeyCombinations());
    else
        ui.action_send_key_combinations->setChecked(false);

    connect(ui.action_settings, &QAction::triggered, this, &DesktopPanel::settingsButton);
    connect(ui.action_autosize, &QAction::triggered, this, &DesktopPanel::onAutosizeButton);
    connect(ui.action_fullscreen, &QAction::triggered, this, &DesktopPanel::onFullscreenButton);
    connect(ui.action_autoscroll, &QAction::triggered, this, &DesktopPanel::autoScrollChanged);
    connect(ui.action_update, &QAction::triggered, this, &DesktopPanel::startRemoteUpdate);
    connect(ui.action_system_info, &QAction::triggered, this, &DesktopPanel::startSystemInfo);
    connect(ui.action_close, &QAction::triggered, this, &DesktopPanel::closeSession);

    createAdditionalMenu(session_type);

    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
    {
        connect(ui.action_cad, &QAction::triggered, this, &DesktopPanel::onCtrlAltDel);
    }
    else
    {
        DCHECK(session_type == proto::SESSION_TYPE_DESKTOP_VIEW);
        ui.action_cad->setVisible(false);
    }

    connect(ui.action_file_transfer, &QAction::triggered, [this]()
    {
        emit startSession(proto::SESSION_TYPE_FILE_TRANSFER);
    });

    ui.frame->hide();
    showCloseButton(false);

    hide_timer_id_ = startTimer(std::chrono::seconds(1));
}

DesktopPanel::~DesktopPanel()
{
    settings_.setScale(scale_);
    settings_.setAutoScrolling(ui.action_autoscroll->isChecked());

    // Save the parameter only for desktop management.
    if (session_type_ == proto::SESSION_TYPE_DESKTOP_MANAGE)
        settings_.setSendKeyCombinations(ui.action_send_key_combinations->isChecked());
}

void DesktopPanel::enableScreenSelect(bool /* enable */)
{
    // By default, we disable the monitor selection menu. Selection will be enabled when receiving
    // a list of monitors.
    ui.action_monitors->setVisible(false);
    ui.action_monitors->setEnabled(false);
    screens_menu_.reset();
    updateSize();
}

void DesktopPanel::enablePowerControl(bool enable)
{
    ui.action_power_control->setVisible(enable);
    ui.action_power_control->setEnabled(enable);

    if (!enable)
    {
        power_menu_.reset();
    }
    else
    {
        power_menu_.reset(new QMenu());
        power_menu_->addAction(ui.action_shutdown);
        power_menu_->addAction(ui.action_reboot);
        power_menu_->addAction(ui.action_logoff);
        power_menu_->addAction(ui.action_lock);

        ui.action_power_control->setMenu(power_menu_.get());

        QToolButton* button = qobject_cast<QToolButton*>(
            ui.toolbar->widgetForAction(ui.action_power_control));
        button->setPopupMode(QToolButton::InstantPopup);

        connect(power_menu_.get(), &QMenu::triggered, this, &DesktopPanel::onPowerControl);
        connect(power_menu_.get(), &QMenu::aboutToShow, [this]() { allow_hide_ = false; });
        connect(power_menu_.get(), &QMenu::aboutToHide, [this]()
        {
            allow_hide_ = true;

            if (leaved_)
                delayedHide();
        });
    }

    updateSize();
}

void DesktopPanel::enableSystemInfo(bool enable)
{
    ui.action_system_info->setVisible(enable);
    ui.action_system_info->setEnabled(enable);
    updateSize();
}

void DesktopPanel::enableRemoteUpdate(bool enable)
{
    ui.action_update->setVisible(enable);
    ui.action_update->setEnabled(enable);
    updateSize();
}

void DesktopPanel::setScreenList(const proto::desktop::ScreenList& screen_list)
{
    screens_menu_.reset();

    // If it has only one screen or an empty list is received.
    if (screen_list.screen_size() <= 1)
    {
        // Monitor selection not available.
        ui.action_monitors->setVisible(false);
        return;
    }

    screens_menu_.reset(new QMenu());
    screens_group_ = new QActionGroup(screens_menu_.get());

    SelectScreenAction* full_screen_action = new SelectScreenAction(screens_group_);
    screens_group_->addAction(full_screen_action);
    screens_menu_->addAction(full_screen_action);

    ui.action_monitors->setMenu(screens_menu_.get());

    QToolButton* button = qobject_cast<QToolButton*>(
        ui.toolbar->widgetForAction(ui.action_monitors));
    button->setPopupMode(QToolButton::InstantPopup);

    for (int i = 0; i < screen_list.screen_size(); ++i)
    {
        SelectScreenAction* action = new SelectScreenAction(
            screen_list.screen(i), tr("Monitor %1").arg(i + 1), screens_group_);

        screens_group_->addAction(action);
        screens_menu_->addAction(action);
    }

    connect(screens_menu_.get(), &QMenu::aboutToShow, [this]() { allow_hide_ = false; });
    connect(screens_menu_.get(), &QMenu::aboutToHide, [this]()
    {
        allow_hide_ = true;

        if (leaved_)
            delayedHide();
    });

    connect(screens_group_, &QActionGroup::triggered, [this](QAction* action)
    {
        emit screenSelected(static_cast<SelectScreenAction*>(action)->screen());
    });

    ui.action_monitors->setVisible(true);
    ui.action_monitors->setEnabled(true);

    updateSize();
}

bool DesktopPanel::autoScrolling() const
{
    return ui.action_autoscroll->isChecked();
}

bool DesktopPanel::sendKeyCombinations() const
{
    return ui.action_send_key_combinations->isChecked();
}

void DesktopPanel::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == hide_timer_id_)
    {
        killTimer(hide_timer_id_);
        hide_timer_id_ = 0;

        ui.frame->setFixedWidth(ui.toolbar->width());

        ui.toolbar->hide();
        ui.frame->show();

        adjustSize();
        return;
    }

    QFrame::timerEvent(event);
}

void DesktopPanel::enterEvent(QEvent* event)
{
    leaved_ = false;

    if (allow_hide_)
    {
        if (hide_timer_id_)
        {
            killTimer(hide_timer_id_);
            hide_timer_id_ = 0;
        }

        ui.toolbar->show();
        ui.frame->hide();

        adjustSize();
    }

    QFrame::enterEvent(event);
}

void DesktopPanel::leaveEvent(QEvent* event)
{
    leaved_ = true;

    if (allow_hide_)
        delayedHide();

    QFrame::leaveEvent(event);
}

void DesktopPanel::onFullscreenButton(bool checked)
{
    if (checked)
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/img/application-resize-actual.png")));
    }
    else
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/img/application-resize-full.png")));
    }

    showCloseButton(checked);

    emit switchToFullscreen(checked);
}

void DesktopPanel::onAutosizeButton()
{
    if (ui.action_fullscreen->isChecked())
    {
        ui.action_fullscreen->setIcon(
            QIcon(QStringLiteral(":/img/application-resize-full.png")));
        ui.action_fullscreen->setChecked(false);

        showCloseButton(false);
    }

    emit switchToAutosize();
}

void DesktopPanel::onCtrlAltDel()
{
    emit keyCombination(Qt::ControlModifier | Qt::AltModifier | Qt::Key_Delete);
}

void DesktopPanel::onPowerControl(QAction* action)
{
    if (action == ui.action_shutdown)
    {
        if (QMessageBox::question(this,
                                  tr("Confirmation"),
                                  tr("Are you sure you want to shutdown the remote computer?"),
                                  QMessageBox::Yes,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            emit powerControl(proto::desktop::PowerControl::ACTION_SHUTDOWN);
        }
    }
    else if (action == ui.action_reboot)
    {
        if (QMessageBox::question(this,
                                  tr("Confirmation"),
                                  tr("Are you sure you want to reboot the remote computer?"),
                                  QMessageBox::Yes,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            emit powerControl(proto::desktop::PowerControl::ACTION_REBOOT);
        }
    }
    else if (action == ui.action_logoff)
    {
        if (QMessageBox::question(this,
                                  tr("Confirmation"),
                                  tr("Are you sure you want to end the user session on the remote computer?"),
                                  QMessageBox::Yes,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            emit powerControl(proto::desktop::PowerControl::ACTION_LOGOFF);
        }
    }
    else if (action == ui.action_lock)
    {
        if (QMessageBox::question(this,
                                  tr("Confirmation"),
                                  tr("Are you sure you want to lock the user session on the remote computer?"),
                                  QMessageBox::Yes,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            emit powerControl(proto::desktop::PowerControl::ACTION_LOCK);
        }
    }
}

void DesktopPanel::createAdditionalMenu(proto::SessionType session_type)
{
    // Create a menu and add actions to it.
    additional_menu_ = new QMenu(this);

    scale_group_ = new QActionGroup(additional_menu_);
    scale_group_->addAction(ui.action_scale100);
    scale_group_->addAction(ui.action_scale90);
    scale_group_->addAction(ui.action_scale80);
    scale_group_->addAction(ui.action_scale70);
    scale_group_->addAction(ui.action_scale60);
    scale_group_->addAction(ui.action_scale50);

    scale_menu_ = additional_menu_->addMenu(tr("Scale"));
    scale_menu_->addAction(ui.action_fit_window);
    scale_menu_->addSeparator();
    scale_menu_->addActions(scale_group_->actions());

    updateScaleMenu();

    additional_menu_->addAction(ui.action_autoscroll);

    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
        additional_menu_->addAction(ui.action_send_key_combinations);

    additional_menu_->addSeparator();
    additional_menu_->addAction(ui.action_screenshot);

    // Set the menu for the button on the toolbar.
    ui.action_menu->setMenu(additional_menu_);

    QToolButton* button = qobject_cast<QToolButton*>(ui.toolbar->widgetForAction(ui.action_menu));
    button->setPopupMode(QToolButton::InstantPopup);

    // Now we connect all the necessary signals and slots.
    if (session_type == proto::SESSION_TYPE_DESKTOP_MANAGE)
    {
        connect(ui.action_send_key_combinations, &QAction::triggered,
                this, &DesktopPanel::keyCombinationsChanged);
    }

    connect(scale_group_, &QActionGroup::triggered, [this](QAction* action)
    {
        if (action == ui.action_scale100)
            scale_ = 100;
        else if (action == ui.action_scale90)
            scale_ = 90;
        else if (action == ui.action_scale80)
            scale_ = 80;
        else if (action == ui.action_scale70)
            scale_ = 70;
        else if (action == ui.action_scale60)
            scale_ = 60;
        else if (action == ui.action_scale50)
            scale_ = 50;
        else
            return;

        emit scaleChanged();
    });

    connect(ui.action_fit_window, &QAction::toggled, [this](bool checked)
    {
        ui.action_autoscroll->setEnabled(!checked);
        scale_group_->setEnabled(!checked);

        if (checked)
        {
            scale_ = -1;
        }
        else
        {
            if (ui.action_scale90->isChecked())
                scale_ = 90;
            else if (ui.action_scale80->isChecked())
                scale_ = 80;
            else if (ui.action_scale70->isChecked())
                scale_ = 70;
            else if (ui.action_scale60->isChecked())
                scale_ = 60;
            else if (ui.action_scale50->isChecked())
                scale_ = 50;
            else
                scale_ = 100;
        }

        emit scaleChanged();
    });

    connect(ui.action_screenshot, &QAction::triggered, this, &DesktopPanel::takeScreenshot);
    connect(additional_menu_, &QMenu::aboutToShow, [this]() { allow_hide_ = false; });
    connect(additional_menu_, &QMenu::aboutToHide, [this]()
    {
        allow_hide_ = true;

        if (leaved_)
            delayedHide();
    });
}

void DesktopPanel::showCloseButton(bool show)
{
    ui.action_close->setVisible(show);
    ui.action_close->setEnabled(show);

    QList<QAction*> actions = ui.toolbar->actions();

    for (auto it = actions.crbegin(); it != actions.crend(); ++it)
    {
        QAction* action = *it;

        if (action->isSeparator())
        {
            action->setVisible(show);
            break;
        }
    }

    updateSize();
}

void DesktopPanel::updateScaleMenu()
{
    if (scale_ == -1)
    {
        ui.action_fit_window->setChecked(true);
        scale_group_->setEnabled(false);
    }
    else
    {
        ui.action_fit_window->setChecked(false);
        scale_group_->setEnabled(true);

        if (scale_ == 90)
            ui.action_scale90->setChecked(true);
        else if (scale_ == 80)
            ui.action_scale80->setChecked(true);
        else if (scale_ == 70)
            ui.action_scale70->setChecked(true);
        else if (scale_ == 60)
            ui.action_scale60->setChecked(true);
        else if (scale_ == 50)
            ui.action_scale50->setChecked(true);
        else
        {
            ui.action_scale100->setChecked(true);
            scale_ = 100;
        }
    }
}

void DesktopPanel::updateSize()
{
    ui.toolbar->adjustSize();
    adjustSize();
}

void DesktopPanel::delayedHide()
{
    if (!ui.action_pin->isChecked() && !hide_timer_id_)
        hide_timer_id_ = startTimer(std::chrono::seconds(1));
}

} // namespace client
