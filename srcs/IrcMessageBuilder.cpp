#include "IrcMessageBuilder.hpp"

std::string IrcMessageBuilder::truncateAndAppend(const std::string& message) {
    const size_t maxLength = 510;
    if (message.length() > maxLength) {
        return message.substr(0, maxLength) + "\r\n";
    }
    return message + "\r\n";
}

std::string IrcMessageBuilder::buildNeedMoreParamsError(const std::string& serverName, const std::string& command)
{
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NEEDMOREPARAMS << " " << command << " :Not enough parameters";
    return truncateAndAppend(oss.str());
}

std::string IrcMessageBuilder::buildErrorMessage(const std::string& serverName, const std::string& errorCode, const std::string& details) {
    std::ostringstream oss;
    oss << ":" << serverName << " " << errorCode << " " << details;
    return truncateAndAppend(oss.str());
}

std::string IrcMessageBuilder::buildWelcomeMessage(const std::string& serverName, const std::string& nickname, const std::string& realName, const std::string& host) {
    std::ostringstream oss;
    oss << ":" << serverName << " 001 " << nickname 
        << " :Welcome to the Internet Relay Network " 
        << nickname << "!" << realName << "@" << host;
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NOSUCHCHANNEL : "403 channelName :No such channel\r\n"
 */
std::string IrcMessageBuilder::buildNoSuchChannelError(const std::string& serverName, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NOSUCHCHANNEL << channelName << " :No such channel";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_CHANOPRIVSNEEDED : "482 channelName :You're not channel operator\r\n"
 */
std::string IrcMessageBuilder::buildChannelOperatorNeededError(const std::string& serverName, const std::string& nickname, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_CHANOPRIVSNEEDED << nickname << " " << channelName << " :You're not channel operator";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_USERNOTINCHANNEL : " 441 user channel :They aren't on that channel\r\n"
 */
std::string IrcMessageBuilder::buildUserNotInChannelError(const std::string& serverName, const std::string& nickname, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_USERNOTINCHANNEL << nickname << " " << channelName << " :They aren't on that channel";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_UNKNOWNMODE : "472 modeChar :Unknown MODE flag\r\n"
 */
std::string IrcMessageBuilder::buildUnknownModeError(const std::string& serverName, const std::string& nickname, char modeChar) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_UNKNOWNMODE << nickname << " " << modeChar << " :is unknown mode char to me";
    return truncateAndAppend(oss.str());
}

std::string IrcMessageBuilder::buildModeChangeMessage(const std::string& nickname, const std::string& channelName, const std::string& modeString) {
    std::ostringstream oss;
    oss << ":" << nickname << " MODE " << channelName << " " << modeString;
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NOTONCHANNEL : "442 channelName :You're not on that channel\r\n"
 */
std::string IrcMessageBuilder::buildNotOnChannelError(const std::string& serverName, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NOTONCHANNEL << channelName << " :You're not on that channel";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NOSUCHNICK : "401 targetNick :No such nick/channel\r\n"
 */
std::string IrcMessageBuilder::buildNoSuchNickError(const std::string& serverName, const std::string& targetNick) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NOSUCHNICK << targetNick << " :No such nick/channel";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_USERONCHANNEL : "443 targetNick channelName :is already on channel\r\n"
 */
std::string IrcMessageBuilder::buildUserOnChannelError(const std::string& serverName, const std::string& targetNick, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_USERONCHANNEL << targetNick << " " << channelName << " :is already on channel";
    return truncateAndAppend(oss.str());
}

/**
 * MSG_INVITE : "nickname INVITE targetNick :channelName\r\n"
 */
std::string IrcMessageBuilder::buildInviteMessage(const std::string& nickname, const std::string& targetNick, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << nickname << " INVITE " << targetNick << " :" << channelName;
    return truncateAndAppend(oss.str());
}

/**
 * RPL_INVITING : "341 nickname targetNick :channelName\r\n"
 */
std::string IrcMessageBuilder::buildInvitingReply(const std::string& serverName, const std::string& nickname, const std::string& targetNick, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << RPL_INVITING << nickname << " " << targetNick << " :" << channelName;
    return truncateAndAppend(oss.str());
}

/**
 * RPL_TOPIC : "332 channelName :topic\r\n"
 */
std::string IrcMessageBuilder::buildTopicReply(const std::string& serverName, const std::string& nickname, const std::string& channelName, const std::string& topic) {
    std::ostringstream oss;
    oss << ":" << serverName << RPL_TOPIC << nickname << " " << channelName << " :" << topic;
    return truncateAndAppend(oss.str());
}

/**
 * RPL_NOTOPIC : "331 channelName :No topic is set\r\n"
 */
std::string IrcMessageBuilder::buildNoTopicReply(const std::string& serverName, const std::string& nickname, const std::string& channelName) {
    std::ostringstream oss;
    oss << ":" << serverName << RPL_NOTOPIC << nickname << " " << channelName << " :No topic is set";
    return truncateAndAppend(oss.str());
}

/**
 * MSG_TOPIC : "nickname TOPIC channelName :topic\r\n"
 */
std::string IrcMessageBuilder::buildTopicMessage(const std::string& nickname, const std::string& channelName, const std::string& topic) {
    std::ostringstream oss;
    oss << ":" << nickname << " TOPIC " << channelName << " :" << topic;
    return truncateAndAppend(oss.str());
}

/**
 * MSG_KICK : "nickname KICK channelName targetNick :comment\r\n"
 */
std::string IrcMessageBuilder::buildKickMessage(const std::string& nickname, const std::string& channelName, const std::string& targetNick, const std::string& comment) {
    std::ostringstream oss;
    oss << ":" << nickname << " KICK " << channelName << " " << targetNick << " :" << comment;
    return truncateAndAppend(oss.str());
}

/**
 * MSG CAP : "nickname CAP * LS :capabilities\r\n"
 */
std::string IrcMessageBuilder::buildCapabilityListMessage(const std::string& serverName, const std::string& nick, const std::string& capabilities) {
    std::ostringstream oss;
    oss << ":" << serverName << " CAP " << nick << " LS :" << capabilities;
    return truncateAndAppend(oss.str());
}

/**
 * ERR_INVALIDCAPCMD : "410 nick subCommand :Invalid CAP subcommand\r\n"
 */
std::string IrcMessageBuilder::buildInvalidCapSubcommandError(const std::string& serverName, const std::string& nick, const std::string& subCommand) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_INVALIDCAPCMD << nick << " " << subCommand << " :Invalid CAP subcommand";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NOTREGISTERED : "451 :You have not registered\r\n"
 */
std::string IrcMessageBuilder::buildNotRegisteredError(const std::string& serverName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NOTREGISTERED << ":You have not registered";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_UNKNOWNCOMMAND : "421 serverName command :Unknown command\r\n"
 */
std::string IrcMessageBuilder::buildUnknownCommandError(const std::string& serverName, const std::string& command) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_UNKNOWNCOMMAND << command << " :Unknown command";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_ALREADYREGISTRED : "You may not reregister\r\n"
 */
std::string IrcMessageBuilder::buildAlreadyRegisteredError(const std::string& serverName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_ALREADYREGISTRED << ":You may not reregister";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_PASSWDMISMATCH : "464 :Password incorrect\r\n"
 */
std::string IrcMessageBuilder::buildPasswordMismatchError(const std::string& serverName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_PASSWDMISMATCH << ":Password incorrect";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NONICKNAMEGIVEN : "431 :No nickname given\r\n"
 */
std::string IrcMessageBuilder::buildNoNicknameGivenError(const std::string& serverName) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NONICKNAMEGIVEN << ":No nickname given";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_ERRONEUSNICKNAME : "432 nickname :Erroneous nickname\r\n"
 */
std::string IrcMessageBuilder::buildErroneousNicknameError(const std::string& serverName, const std::string& newNickname) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_ERRONEUSNICKNAME << newNickname << " :Erroneous nickname";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_NICKNAMEINUSE : "433 nickname :Nickname is already in use\r\n"
 */
std::string IrcMessageBuilder::buildNicknameInUseError(const std::string& serverName, const std::string& newNickname) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_NICKNAMEINUSE << newNickname << " :Nickname is already in use";
    return truncateAndAppend(oss.str());
}

/**
 * MSG NICK : "nickname NICK :newNickname\r\n"
 */
std::string IrcMessageBuilder::buildNickChangeMessage(const std::string& currentNickname, const std::string& newNickname) {
    std::ostringstream oss;
    oss << ":" << currentNickname << " NICK :" << newNickname << ".\033[0m";
    return truncateAndAppend(oss.str());
}

/**
 * ERR_ERRONEUSUSERNAME : "491 username :Erroneous username\r\n"
 */
std::string IrcMessageBuilder::buildErroneousUsernameError(const std::string& serverName, const std::string& username) {
    std::ostringstream oss;
    oss << ":" << serverName << ERR_ERRONEUSUSERNAME << username << " :Erroneous username";
    return truncateAndAppend(oss.str());
}