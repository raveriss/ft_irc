#ifndef IRCMESSAGEBUILDER_HPP
#define IRCMESSAGEBUILDER_HPP

#include "IrcNumericReplies.hpp"

#include <string>
#include <sstream>

class IrcMessageBuilder
{
    public:
        /* 461 ERR_NEEDMOREPARAMS */
        static std::string buildNeedMoreParamsError(const std::string& serverName, const std::string& command);

        
        static std::string buildErrorMessage(const std::string& serverName, const std::string& errorCode, const std::string& details);

        
        static std::string truncateAndAppend(const std::string& message);

        /* 001 RPL_WELCOME : "nickname :Welcome to the Internet Relay Network nickname!realName@host\r\n" */
        static std::string buildWelcomeMessage(const std::string& serverName, const std::string& nickname, const std::string& realName, const std::string& host);

        /* ERR_NOSUCHCHANNEL : "403 channelName :No such channel\r\n" */
        static std::string buildNoSuchChannelError(const std::string& serverName, const std::string& channelName);

        /* ERR_CHANOPRIVSNEEDED : "482 channelName :You're not channel operator\r\n" */
        static std::string buildChannelOperatorNeededError(const std::string& serverName, const std::string& nickname, const std::string& channelName);

        /* ERR_USERNOTINCHANNEL : "441 user channel :They aren't on that channel\r\n" */
        static std::string buildUserNotInChannelError(const std::string& serverName, const std::string& nickname, const std::string& channelName);

        /* ERR_UNKNOWNMODE : "472 modeChar :Unknown MODE flag\r\n" */
        static std::string buildUnknownModeError(const std::string& serverName, const std::string& nickname, char modeChar);

        /* MSG_MODE : "nickname MODE channelName modeString\r\n" */
        static std::string buildModeChangeMessage(const std::string& nickname, const std::string& channelName, const std::string& modeString);

        /* ERR_NOTONCHANNEL : "442 channelName :You're not on that channel\r\n" */
        static std::string buildNotOnChannelError(const std::string& serverName, const std::string& channelName);

        /* ERR_NOSUCHNICK : "401 targetNick :No such nick/channel\r\n" */
        static std::string buildNoSuchNickError(const std::string& serverName, const std::string& targetNick);

        /* ERR_USERONCHANNEL : "443 targetNick channelName :is already on channel\r\n" */
        static std::string buildUserOnChannelError(const std::string& serverName, const std::string& targetNick, const std::string& channelName);

        /* MSG_INVITE : "nickname INVITE targetNick :channelName\r\n" */
        static std::string buildInviteMessage(const std::string& nickname, const std::string& targetNick, const std::string& channelName);

        /* RPL_INVITING : "341 nickname targetNick :channelName\r\n" */
        static std::string buildInvitingReply(const std::string& serverName, const std::string& nickname, const std::string& targetNick, const std::string& channelName);

        /* RPL_TOPIC : "332 channelName :topic\r\n" */
        static std::string buildTopicReply(const std::string& serverName, const std::string& nickname, const std::string& channelName, const std::string& topic);

        /* RPL_NOTOPIC : "331 channelName :No topic is set\r\n" */
        static std::string buildNoTopicReply(const std::string& serverName, const std::string& nickname, const std::string& channelName);

        /* MSG_TOPIC : "nickname TOPIC channelName :topic\r\n" */
        static std::string buildTopicMessage(const std::string& nickname, const std::string& channelName, const std::string& topic);

        /* MSG_KICK : "nickname KICK channelName targetNick :comment\r\n" */
        static std::string buildKickMessage(const std::string& nickname, const std::string& channelName, const std::string& targetNick, const std::string& comment);

        /* MSG_CAP : "nickname CAP * LS :capabilities\r\n" */
        static std::string buildCapabilityListMessage(const std::string& serverName, const std::string& nick, const std::string& capabilities);

        /* ERR_INVALIDCAPCMD : "410 nickname :Invalid CAP subcommand\r\n" */
        static std::string buildInvalidCapSubcommandError(const std::string& serverName, const std::string& nick, const std::string& subCommand);

        /* ERR_NOTREGISTERED : "451 :You have not registered\r\n" */
        static std::string buildNotRegisteredError(const std::string& serverName);

        /* ERR_UNKNOWNCOMMAND : "421 serverName command :Unknown command\r\n" */
        static std::string buildUnknownCommandError(const std::string& serverName, const std::string& command);

        /* ERR_ALREADYREGISTRED : "You may not reregister\r\n" */
        static std::string buildAlreadyRegisteredError(const std::string& serverName);

        /* ERR_PASSWDMISMATCH : "464 :Password incorrect\r\n" */
        static std::string buildPasswordMismatchError(const std::string& serverName);

        /* ERR_NONICKNAMEGIVEN : "431 :No nickname given\r\n" */
        static std::string buildNoNicknameGivenError(const std::string& serverName);

        /* ERR_ERRONEUSNICKNAME : "432 nickname :Erroneous nickname\r\n" */
        static std::string buildErroneousNicknameError(const std::string& serverName, const std::string& newNickname);

        /* ERR_NICKNAMEINUSE : "433 nickname :Nickname is already in use\r\n" */
        static std::string buildNicknameInUseError(const std::string& serverName, const std::string& newNickname);

        /* MSG NICK : "nickname NICK :newNickname\r\n" */
        static std::string buildNickChangeMessage(const std::string& currentNickname, const std::string& newNickname);

        /* ERR_ERRONEUSUSERNAME : "491 username :Erroneous username\r\n" */
        static std::string buildErroneousUsernameError(const std::string& serverName, const std::string& username);

};

#endif
