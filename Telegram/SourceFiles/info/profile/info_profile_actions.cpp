/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#include "info/profile/info_profile_actions.h"

#include <rpl/flatten_latest.h>
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/shadow.h"
#include "boxes/abstract_box.h"
#include "boxes/confirm_box.h"
#include "boxes/peer_list_box.h"
#include "boxes/peer_list_controllers.h"
#include "boxes/add_contact_box.h"
#include "boxes/report_box.h"
#include "lang/lang_keys.h"
#include "info/info_controller.h"
#include "info/info_top_bar_override.h"
#include "info/profile/info_profile_icon.h"
#include "info/profile/info_profile_values.h"
#include "info/profile/info_profile_button.h"
#include "info/profile/info_profile_text.h"
#include "window/window_controller.h"
#include "window/window_peer_menu.h"
#include "mainwidget.h"
#include "auth_session.h"
#include "apiwrap.h"
#include "styles/style_info.h"
#include "styles/style_boxes.h"

namespace Info {
namespace Profile {
namespace {

object_ptr<Ui::RpWidget> CreateSkipWidget(
		not_null<Ui::RpWidget*> parent) {
	return Ui::CreateSkipWidget(parent, st::infoProfileSkip);
}

object_ptr<Ui::SlideWrap<>> CreateSlideSkipWidget(
		not_null<Ui::RpWidget*> parent) {
	return Ui::CreateSlideSkipWidget(parent, st::infoProfileSkip);
}

template <typename Text, typename ToggleOn, typename Callback>
auto AddActionButton(
		not_null<Ui::VerticalLayout*> parent,
		Text &&text,
		ToggleOn &&toggleOn,
		Callback &&callback,
		const style::InfoProfileButton &st
			= st::infoSharedMediaButton) {
	auto result = parent->add(object_ptr<Ui::SlideWrap<Button>>(
		parent,
		object_ptr<Button>(
			parent,
			std::move(text),
			st))
	);
	result->toggleOn(
		std::move(toggleOn)
	)->entity()->addClickHandler(std::move(callback));
	return result;
};

template <typename Text, typename ToggleOn, typename Callback>
auto AddMainButton(
		not_null<Ui::VerticalLayout*> parent,
		Text &&text,
		ToggleOn &&toggleOn,
		Callback &&callback,
		Ui::MultiSlideTracker &tracker) {
	tracker.track(AddActionButton(
		parent,
		std::move(text) | ToUpperValue(),
		std::move(toggleOn),
		std::move(callback),
		st::infoMainButton));
}

class DetailsFiller {
public:
	DetailsFiller(
		not_null<Controller*> controller,
		not_null<Ui::RpWidget*> parent,
		not_null<PeerData*> peer);

	object_ptr<Ui::RpWidget> fill();

private:
	object_ptr<Ui::RpWidget> setupInfo();
	object_ptr<Ui::RpWidget> setupMuteToggle();
	void setupMainButtons();
	Ui::MultiSlideTracker fillUserButtons(
		not_null<UserData*> user);
	Ui::MultiSlideTracker fillChannelButtons(
		not_null<ChannelData*> channel);

	template <
		typename Widget,
		typename = std::enable_if_t<
		std::is_base_of_v<Ui::RpWidget, Widget>>>
	Widget *add(
			object_ptr<Widget> &&child,
			const style::margins &margin = style::margins()) {
		return _wrap->add(
			std::move(child),
			margin);
	}

	not_null<Controller*> _controller;
	not_null<Ui::RpWidget*> _parent;
	not_null<PeerData*> _peer;
	object_ptr<Ui::VerticalLayout> _wrap;

};

class ActionsFiller {
public:
	ActionsFiller(
		not_null<Controller*> controller,
		not_null<Ui::RpWidget*> parent,
		not_null<PeerData*> peer);

	object_ptr<Ui::RpWidget> fill();

private:
	void addInviteToGroupAction(not_null<UserData*> user);
	void addShareContactAction(not_null<UserData*> user);
	void addEditContactAction(not_null<UserData*> user);
	void addDeleteContactAction(not_null<UserData*> user);
	void addClearHistoryAction(not_null<UserData*> user);
	void addDeleteConversationAction(not_null<UserData*> user);
	void addBotCommandActions(not_null<UserData*> user);
	void addReportAction();
	void addBlockAction(not_null<UserData*> user);
	void addLeaveChannelAction(not_null<ChannelData*> channel);
	void addJoinChannelAction(not_null<ChannelData*> channel);
	void fillUserActions(not_null<UserData*> user);
	void fillChannelActions(not_null<ChannelData*> channel);

	not_null<Controller*> _controller;
	not_null<Ui::RpWidget*> _parent;
	not_null<PeerData*> _peer;
	object_ptr<Ui::VerticalLayout> _wrap = { nullptr };

};

DetailsFiller::DetailsFiller(
	not_null<Controller*> controller,
	not_null<Ui::RpWidget*> parent,
	not_null<PeerData*> peer)
: _controller(controller)
, _parent(parent)
, _peer(peer)
, _wrap(_parent) {
}

object_ptr<Ui::RpWidget> DetailsFiller::setupInfo() {
	auto result = object_ptr<Ui::VerticalLayout>(_wrap);
	auto tracker = Ui::MultiSlideTracker();
	auto addInfoLine = [&](
			LangKey label,
			rpl::producer<TextWithEntities> &&text,
			bool selectByDoubleClick = false,
			const style::FlatLabel &textSt = st::infoLabeled) {
		auto line = result->add(CreateTextWithLabel(
			result,
			Lang::Viewer(label) | WithEmptyEntities(),
			std::move(text),
			textSt,
			st::infoProfileLabeledPadding,
			selectByDoubleClick));
		tracker.track(line);
		return line;
	};
	auto addInfoOneLine = [&](
			LangKey label,
			rpl::producer<TextWithEntities> &&text) {
		addInfoLine(
			label,
			std::move(text),
			true,
			st::infoLabeledOneLine);
	};
	if (auto user = _peer->asUser()) {
		addInfoOneLine(lng_info_mobile_label, PhoneValue(user));
		if (user->botInfo) {
			addInfoLine(lng_info_about_label, AboutValue(user));
		} else {
			addInfoLine(lng_info_bio_label, BioValue(user));
		}
		addInfoOneLine(lng_info_username_label, UsernameValue(user));
	} else {
		addInfoOneLine(lng_info_link_label, LinkValue(_peer));
		addInfoLine(lng_info_about_label, AboutValue(_peer));
	}
	result->add(object_ptr<Ui::SlideWrap<>>(
		result,
		object_ptr<Ui::PlainShadow>(result),
		st::infoProfileSeparatorPadding)
	)->toggleOn(std::move(tracker).atLeastOneShownValue());
	object_ptr<FloatingIcon>(
		result,
		st::infoIconInformation,
		st::infoInformationIconPosition);
	return std::move(result);
}

object_ptr<Ui::RpWidget> DetailsFiller::setupMuteToggle() {
	auto peer = _peer;
	auto result = object_ptr<Button>(
		_wrap,
		Lang::Viewer(lng_profile_enable_notifications),
		st::infoNotificationsButton);
	result->toggleOn(
		NotificationsEnabledValue(peer)
	)->addClickHandler([=] {
		App::main()->updateNotifySetting(
			_peer,
			_peer->isMuted()
				? NotifySettingSetNotify
				: NotifySettingSetMuted);
	});
	object_ptr<FloatingIcon>(
		result,
		st::infoIconNotifications,
		st::infoNotificationsIconPosition);
	return std::move(result);
}

void DetailsFiller::setupMainButtons() {
	auto wrapButtons = [=](auto &&callback) {
		auto topSkip = _wrap->add(CreateSlideSkipWidget(_wrap));
		auto tracker = callback();
		topSkip->toggleOn(std::move(tracker).atLeastOneShownValue());
	};
	if (auto user = _peer->asUser()) {
		wrapButtons([=] {
			return fillUserButtons(user);
		});
	} else if (auto channel = _peer->asChannel()) {
		if (!channel->isMegagroup()) {
			wrapButtons([=] {
				return fillChannelButtons(channel);
			});
		}
	}
}

Ui::MultiSlideTracker DetailsFiller::fillUserButtons(
		not_null<UserData*> user) {
	using namespace rpl::mappers;

	Ui::MultiSlideTracker tracker;
	auto window = _controller->window();
	auto sendMessageVisible = window->historyPeer.value()
		| rpl::map($1 != user);
	auto sendMessage = [window, user] {
		window->showPeerHistory(
			user,
			Window::SectionShow::Way::Forward);
	};
	AddMainButton(
		_wrap,
		Lang::Viewer(lng_profile_send_message),
		std::move(sendMessageVisible),
		std::move(sendMessage),
		tracker);
	AddMainButton(
		_wrap,
		Lang::Viewer(lng_info_add_as_contact),
		CanAddContactValue(user),
		[user] { Window::PeerMenuAddContact(user); },
		tracker);

	return tracker;
}

Ui::MultiSlideTracker DetailsFiller::fillChannelButtons(
		not_null<ChannelData*> channel) {
	using namespace rpl::mappers;

	Ui::MultiSlideTracker tracker;
	auto window = _controller->window();
	auto viewChannelVisible = window->historyPeer.value()
		| rpl::map($1 != channel);
	auto viewChannel = [=] {
		window->showPeerHistory(
			channel,
			Window::SectionShow::Way::Forward);
	};
	AddMainButton(
		_wrap,
		Lang::Viewer(lng_profile_view_channel),
		std::move(viewChannelVisible),
		std::move(viewChannel),
		tracker);

	return tracker;
}

object_ptr<Ui::RpWidget> DetailsFiller::fill() {
	add(object_ptr<BoxContentDivider>(_wrap));
	add(CreateSkipWidget(_wrap));
	add(setupInfo());
	add(setupMuteToggle());
	setupMainButtons();
	add(CreateSkipWidget(_wrap));
	return std::move(_wrap);
}

ActionsFiller::ActionsFiller(
	not_null<Controller*> controller,
	not_null<Ui::RpWidget*> parent,
	not_null<PeerData*> peer)
: _controller(controller)
, _parent(parent)
, _peer(peer) {
}

void ActionsFiller::addInviteToGroupAction(
		not_null<UserData*> user) {
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_invite_to_group),
		CanInviteBotToGroupValue(user),
		[user] { AddBotToGroupBoxController::Start(user); });
}

void ActionsFiller::addShareContactAction(not_null<UserData*> user) {
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_info_share_contact),
		CanShareContactValue(user),
		[user] { Window::PeerMenuShareContactBox(user); });
}

void ActionsFiller::addEditContactAction(not_null<UserData*> user) {
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_info_edit_contact),
		IsContactValue(user),
		[user] { Ui::show(Box<AddContactBox>(user)); });
}

void ActionsFiller::addDeleteContactAction(
		not_null<UserData*> user) {
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_info_delete_contact),
		IsContactValue(user),
		[user] { Window::PeerMenuDeleteContact(user); });
}

void ActionsFiller::addClearHistoryAction(not_null<UserData*> user) {
	auto callback = [user] {
		auto confirmation = lng_sure_delete_history(
			lt_contact,
			App::peerName(user));
		auto confirmCallback = [user] {
			Ui::hideLayer();
			App::main()->clearHistory(user);
			Ui::showPeerHistory(user, ShowAtUnreadMsgId);
		};
		auto box = Box<ConfirmBox>(
			confirmation,
			lang(lng_box_delete),
			st::attentionBoxButton,
			std::move(confirmCallback));
		Ui::show(std::move(box));
	};
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_clear_history),
		rpl::single(true),
		std::move(callback));
}

void ActionsFiller::addDeleteConversationAction(
		not_null<UserData*> user) {
	auto callback = [user] {
		auto confirmation = lng_sure_delete_history(
			lt_contact,
			App::peerName(user));
		auto confirmButton = lang(lng_box_delete);
		auto confirmCallback = [user] {
			Ui::hideLayer();
			Ui::showChatsList();
			App::main()->deleteConversation(user);
		};
		auto box = Box<ConfirmBox>(
			confirmation,
			confirmButton,
			st::attentionBoxButton,
			std::move(confirmCallback));
		Ui::show(std::move(box));
	};
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_delete_conversation),
		rpl::single(true),
		std::move(callback));
}

void ActionsFiller::addBotCommandActions(not_null<UserData*> user) {
	auto findBotCommand = [user](const QString &command) {
		if (!user->botInfo) {
			return QString();
		}
		for_const (auto &data, user->botInfo->commands) {
			auto isSame = data.command.compare(
				command,
				Qt::CaseInsensitive) == 0;
			if (isSame) {
				return data.command;
			}
		}
		return QString();
	};
	auto hasBotCommandValue = [=](const QString &command) {
		return Notify::PeerUpdateValue(
			user,
			Notify::PeerUpdate::Flag::BotCommandsChanged)
			| rpl::map([=] {
				return !findBotCommand(command).isEmpty();
			});
	};
	auto sendBotCommand = [=](const QString &command) {
		auto original = findBotCommand(command);
		if (!original.isEmpty()) {
			Ui::showPeerHistory(user, ShowAtTheEndMsgId);
			App::sendBotCommand(user, user, '/' + original);
		}
	};
	auto addBotCommand = [=](LangKey key, const QString &command) {
		AddActionButton(
			_wrap,
			Lang::Viewer(key),
			hasBotCommandValue(command),
			[=] { sendBotCommand(command); });
	};
	addBotCommand(lng_profile_bot_help, qsl("help"));
	addBotCommand(lng_profile_bot_settings, qsl("settings"));
}

void ActionsFiller::addReportAction() {
	auto peer = _peer;
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_report),
		rpl::single(true),
		[peer] { Ui::show(Box<ReportBox>(peer)); },
		st::infoBlockButton);
}

void ActionsFiller::addBlockAction(not_null<UserData*> user) {
	auto text = Notify::PeerUpdateValue(
		user,
		Notify::PeerUpdate::Flag::UserIsBlocked)
		| rpl::map([user]() -> rpl::producer<QString> {
			switch (user->blockStatus()) {
			case UserData::BlockStatus::Blocked:
				return Lang::Viewer(user->botInfo
					? lng_profile_unblock_bot
					: lng_profile_unblock_user);
			case UserData::BlockStatus::NotBlocked:
				return Lang::Viewer(user->botInfo
					? lng_profile_block_bot
					: lng_profile_block_user);
			default:
				return rpl::single(QString());
			}
		})
		| rpl::flatten_latest()
		| rpl::start_spawning(_wrap->lifetime());

	auto toggleOn = rpl::duplicate(text)
		| rpl::map([](const QString &text) {
			return !text.isEmpty();
		});
	auto callback = [user] {
		if (user->isBlocked()) {
			Auth().api().unblockUser(user);
		} else {
			Auth().api().blockUser(user);
		}
	};
	AddActionButton(
		_wrap,
		rpl::duplicate(text),
		std::move(toggleOn),
		std::move(callback),
		st::infoBlockButton);
}

void ActionsFiller::addLeaveChannelAction(
		not_null<ChannelData*> channel) {
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_leave_channel),
		AmInChannelValue(channel),
		[channel] { Auth().api().leaveChannel(channel); },
		st::infoBlockButton);
}

void ActionsFiller::addJoinChannelAction(
		not_null<ChannelData*> channel) {
	using namespace rpl::mappers;
	auto joinVisible = AmInChannelValue(channel)
		| rpl::map(!$1)
		| rpl::start_spawning(_wrap->lifetime());
	AddActionButton(
		_wrap,
		Lang::Viewer(lng_profile_join_channel),
		rpl::duplicate(joinVisible),
		[channel] { Auth().api().joinChannel(channel); });
	_wrap->add(object_ptr<Ui::SlideWrap<Ui::FixedHeightWidget>>(
		_wrap,
		CreateSkipWidget(
			_wrap,
			st::infoBlockButtonSkip))
	)->toggleOn(rpl::duplicate(joinVisible));
}

void ActionsFiller::fillUserActions(not_null<UserData*> user) {
	if (user->botInfo) {
		addInviteToGroupAction(user);
	}
	addShareContactAction(user);
	addEditContactAction(user);
	addDeleteContactAction(user);
	addClearHistoryAction(user);
	addDeleteConversationAction(user);
	if (!user->isSelf()) {
		if (user->botInfo) {
			addBotCommandActions(user);
		}
		_wrap->add(CreateSkipWidget(
			_wrap,
			st::infoBlockButtonSkip));
		if (user->botInfo) {
			addReportAction();
		}
		addBlockAction(user);
	}
}

void ActionsFiller::fillChannelActions(
		not_null<ChannelData*> channel) {
	using namespace rpl::mappers;

	addJoinChannelAction(channel);
	if (!channel->amCreator()) {
		addReportAction();
	}
	addLeaveChannelAction(channel);
}

object_ptr<Ui::RpWidget> ActionsFiller::fill() {
	auto wrapResult = [=](auto &&callback) {
		_wrap = object_ptr<Ui::VerticalLayout>(_parent);
		_wrap->add(CreateSkipWidget(_wrap));
		callback();
		_wrap->add(CreateSkipWidget(_wrap));
		object_ptr<FloatingIcon>(
			_wrap,
			st::infoIconActions,
			st::infoIconPosition);
		return std::move(_wrap);
	};
	if (auto user = _peer->asUser()) {
		return wrapResult([=] {
			fillUserActions(user);
		});
	} else if (auto channel = _peer->asChannel()) {
		if (!channel->isMegagroup()) {
			return wrapResult([=] {
				fillChannelActions(channel);
			});
		}
	}
	return { nullptr };
}

} // namespace

object_ptr<Ui::RpWidget> SetupDetails(
		not_null<Controller*> controller,
		not_null<Ui::RpWidget*> parent,
		not_null<PeerData*> peer) {
	DetailsFiller filler(controller, parent, peer);
	return filler.fill();
}

object_ptr<Ui::RpWidget> SetupActions(
		not_null<Controller*> controller,
		not_null<Ui::RpWidget*> parent,
		not_null<PeerData*> peer) {
	ActionsFiller filler(controller, parent, peer);
	return filler.fill();
}

} // namespace Profile
} // namespace Info