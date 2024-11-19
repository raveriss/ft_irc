#ifndef IRC_CODES_HPP
#define IRC_CODES_HPP

#include <string>

namespace IRCCodes
{
    /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
    /*                                ERROR CODES                                */
    /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
    
    /* ERR_NOSUCHNICK
     * 401 <nickname> :No such nick/channel\r\n
     */
    const std::string ERR_NOSUCHNICK = "401";
    const std::string ERR_NOSUCHNICK_MSG = " <nickname> :No such nick/channel\r\n";

    /* ERR_NOSUCHSERVER
     * 402 <server> :No such server\r\n
     */
    const std::string ERR_NOSUCHSERVER = "402";
    const std::string ERR_NOSUCHSERVER_MSG = " <server> :No such server\r\n";

    /* ERR_NOSUCHCHANNEL
     * 403 <channel> :No such channel\r\n
     */
    const std::string ERR_NOSUCHCHANNEL = "403";
    const std::string ERR_NOSUCHCHANNEL_MSG = " channelName :No such channel\r\n";

    /* ERR_CANNOTSENDTOCHAN
     * 404 <channel> :Cannot send to channel\r\n
     */
    const std::string ERR_CANNOTSENDTOCHAN = "404";
    const std::string ERR_CANNOTSENDTOCHAN_MSG = " <channel> :Cannot send to channel\r\n";

    /* ERR_TOOMANYCHANNELS
     * 405 <channel> :You have joined too many channels\r\n
     */
    const std::string ERR_TOOMANYCHANNELS = "405";
    const std::string ERR_TOOMANYCHANNELS_MSG = " <channel> :You have joined too many channels\r\n";

    /* ERR_WASNOSUCHNICK
     * 406 <nickname> :There was no such nickname\r\n
     */
    const std::string ERR_WASNOSUCHNICK = "406";
    const std::string ERR_WASNOSUCHNICK_MSG = " <nickname> :There was no such nickname\r\n";

    /* ERR_TOOMANYTARGETS
     * 407 <target> :Duplicate recipients. No message delivered\r\n
     */
    const std::string ERR_TOOMANYTARGETS = "407";
    const std::string ERR_TOOMANYTARGETS_MSG = " <target> :Duplicate recipients. No message delivered\r\n";

    /* ERR_NOORIGIN
     * 409 :No origin specified\r\n
     */
    const std::string ERR_NOORIGIN = "409";
    const std::string ERR_NOORIGIN_MSG = " :No origin specified\r\n";

    /* ERR_NORECIPIENT
     * 411 :No recipient given (<command>)\r\n
     */
    const std::string ERR_NORECIPIENT = "411";
    const std::string ERR_NORECIPIENT_MSG = " :No recipient given (<command>)\r\n";

    /* ERR_NOTEXTTOSEND
     * 412 :No text to send\r\n
     */
    const std::string ERR_NOTEXTTOSEND = "412";
    const std::string ERR_NOTEXTTOSEND_MSG = " :No text to send\r\n";

    /* ERR_NOTOPLEVEL
     * 413 <mask> :No toplevel domain specified\r\n
     */
    const std::string ERR_NOTOPLEVEL = "413";
    const std::string ERR_NOTOPLEVEL_MSG = " <mask> :No toplevel domain specified\r\n";

    /* ERR_WILDTOPLEVEL
     * 414 <mask> :Wildcard in toplevel domain\r\n
     */
    const std::string ERR_WILDTOPLEVEL = "414";
    const std::string ERR_WILDTOPLEVEL_MSG = " <mask> :Wildcard in toplevel domain\r\n";

    /* ERR_UNKNOWNCOMMAND
     * 421 <command> :Unknown command\r\n
     */
    const std::string ERR_UNKNOWNCOMMAND = "421";
    const std::string ERR_UNKNOWNCOMMAND_MSG = " <command> :Unknown command\r\n";

    /* ERR_NOMOTD
     * 422 :MOTD File is missing\r\n
     */
    const std::string ERR_NOMOTD = "422";
    const std::string ERR_NOMOTD_MSG = " :MOTD File is missing\r\n";

    /* ERR_NOADMININFO
     * 423 <server> :No administrative info available\r\n
     */
    const std::string ERR_NOADMININFO = "423";
    const std::string ERR_NOADMININFO_MSG = " <server> :No administrative info available\r\n";

    /* ERR_FILEERROR
     * 424 :File error doing <operation> on <file>\r\n
     */
    const std::string ERR_FILEERROR = "424";
    const std::string ERR_FILEERROR_MSG = " :File error doing <operation> on <file>\r\n";

    /* ERR_NONICKNAMEGIVEN
     * 431 :No nickname given\r\n
     */
    const std::string ERR_NONICKNAMEGIVEN = "431";
    const std::string ERR_NONICKNAMEGIVEN_MSG = " :No nickname given\r\n";

    /* ERR_ERRONEUSNICKNAME
     * 432 <nickname> :Erroneous nickname\r\n
     */
    const std::string ERR_ERRONEUSNICKNAME = "432";
    const std::string ERR_ERRONEUSNICKNAME_MSG = " <nickname> :Erroneous nickname\r\n";

    /* ERR_NICKNAMEINUSE
     * 433 <nickname> :Nickname is already in use\r\n
     */
    const std::string ERR_NICKNAMEINUSE = "433";
    const std::string ERR_NICKNAMEINUSE_MSG = " <nickname> :Nickname is already in use\r\n";

    /* ERR_NICKCOLLISION
     * 436 <nickname> :Nickname collision KILL\r\n
     */
    const std::string ERR_NICKCOLLISION = "436";
    const std::string ERR_NICKCOLLISION_MSG = " <nickname> :Nickname collision KILL\r\n";

    /* ERR_USERNOTINCHANNEL
     * 441 <user> <channel> :They aren't on that channel\r\n
     */
    const std::string ERR_USERNOTINCHANNEL = "441";
    const std::string ERR_USERNOTINCHANNEL_MSG = " <user> <channel> :They aren't on that channel\r\n";

    /* ERR_NOTONCHANNEL
     * 442 <channel> :You're not on that channel\r\n
     */
    const std::string ERR_NOTONCHANNEL = "442";
    const std::string ERR_NOTONCHANNEL_MSG = " <channel> :You're not on that channel\r\n";

    /* ERR_USERONCHANNEL
     * 443 <user> <channel> :is already on channel\r\n
     */
    const std::string ERR_USERONCHANNEL = "443";
    const std::string ERR_USERONCHANNEL_MSG = " <user> <channel> :is already on channel\r\n";

    /* ERR_NOLOGIN
     * 444 <user> :User not logged in\r\n
     */
    const std::string ERR_NOLOGIN = "444";
    const std::string ERR_NOLOGIN_MSG = " <user> :User not logged in\r\n";

    /* ERR_SUMMONDISABLED
     * 445 :SUMMON has been disabled\r\n
     */
    const std::string ERR_SUMMONDISABLED = "445";
    const std::string ERR_SUMMONDISABLED_MSG = " :SUMMON has been disabled\r\n";

    /* ERR_USERSDISABLED
     * 446 :USERS has been disabled\r\n
     */
    const std::string ERR_USERSDISABLED = "446";
    const std::string ERR_USERSDISABLED_MSG = " :USERS has been disabled\r\n";

    /* ERR_NOTREGISTERED
     * 451 :You have not registered\r\n
     */
    const std::string ERR_NOTREGISTERED = "451";
    const std::string ERR_NOTREGISTERED_MSG = " :You have not registered\r\n";

    /* ERR_NEEDMOREPARAMS
     * 461 <command> :Not enough parameters\r\n
     */
    const std::string ERR_NEEDMOREPARAMS = "461";
    const std::string ERR_NEEDMOREPARAMS_MSG = " <command> :Not enough parameters\r\n";

    /* ERR_ALREADYREGISTRED
     * 462 :You may not reregister\r\n
     */
    const std::string ERR_ALREADYREGISTRED = "462";
    const std::string ERR_ALREADYREGISTRED_MSG = " :You may not reregister\r\n";

    /* ERR_NOPERMFORHOST
     * 463 :Your host isn't among the privileged\r\n
     */
    const std::string ERR_NOPERMFORHOST = "463";
    const std::string ERR_NOPERMFORHOST_MSG = " :Your host isn't among the privileged\r\n";

    /* ERR_PASSWDMISMATCH
     * 464 :Password incorrect\r\n
     */
    const std::string ERR_PASSWDMISMATCH = "464";
    const std::string ERR_PASSWDMISMATCH_MSG = " :Password incorrect\r\n";

    /* ERR_YOUREBANNEDCREEP
     * 465 :You are banned from this server\r\n
     */
    const std::string ERR_YOUREBANNEDCREEP = "465";
    const std::string ERR_YOUREBANNEDCREEP_MSG = " :You are banned from this server\r\n";

    /* ERR_YOUWILLBEBANNED
     * 466 :You will be banned\r\n
     */
    const std::string ERR_YOUWILLBEBANNED = "466";
    const std::string ERR_YOUWILLBEBANNED_MSG = " :You will be banned\r\n";

    /* ERR_KEYSET
     * 467 <channel> :Channel key already set\r\n
     */
    const std::string ERR_KEYSET = "467";
    const std::string ERR_KEYSET_MSG = " <channel> :Channel key already set\r\n";

    /* ERR_CHANNELISFULL
     * 471 <channel> :Cannot join channel (+l)\r\n
     */
    const std::string ERR_CHANNELISFULL = "471";
    const std::string ERR_CHANNELISFULL_MSG = " <channel> :Cannot join channel (+l)\r\n";

    /* ERR_UNKNOWNMODE
     * 472 <character> :is unknown mode char to me\r\n
     */
    const std::string ERR_UNKNOWNMODE = "472";
    const std::string ERR_UNKNOWNMODE_MSG = " <character> :is unknown mode char to me\r\n";

    /* ERR_INVITEONLYCHAN
     * 473 <channel> :Cannot join channel (+i)\r\n
     */
    const std::string ERR_INVITEONLYCHAN = "473";
    const std::string ERR_INVITEONLYCHAN_MSG = " <channel> :Cannot join channel (+i)\r\n";

    /* ERR_BANNEDFROMCHAN
     * 474 <channel> :Cannot join channel (+b)\r\n
     */
    const std::string ERR_BANNEDFROMCHAN = "474";
    const std::string ERR_BANNEDFROMCHAN_MSG = " <channel> :Cannot join channel (+b)\r\n";

    /* ERR_BADCHANNELKEY
     * 475 <channel> :Cannot join channel (+k)\r\n
     */
    const std::string ERR_BADCHANNELKEY = "475";
    const std::string ERR_BADCHANNELKEY_MSG = " <channel> :Cannot join channel (+k)\r\n";

    /* ERR_BADCHANMASK
     * 476 <channel> :Bad channel mask\r\n
     */
    const std::string ERR_BADCHANMASK = "476";
    const std::string ERR_BADCHANMASK_MSG = " <channel> :Bad channel mask\r\n";

    /* ERR_NOPRIVILEGES
     * 481 :Permission Denied- You're not an IRC operator\r\n
     */
    const std::string ERR_NOPRIVILEGES = "481";
    const std::string ERR_NOPRIVILEGES_MSG = " :Permission Denied- You're not an IRC operator\r\n";

    /* ERR_CHANOPRIVSNEEDED
     * 482 <channel> :You're not channel operator\r\n
     */
    const std::string ERR_CHANOPRIVSNEEDED = "482";
    const std::string ERR_CHANOPRIVSNEEDED_MSG = " <channel> :You're not channel operator\r\n";

    /* ERR_CANTKILLSERVER
     * 483 :You can't kill a server!\r\n
     */
    const std::string ERR_CANTKILLSERVER = "483";
    const std::string ERR_CANTKILLSERVER_MSG = " :You can't kill a server!\r\n";

    /* ERR_NOOPERHOST
     * 491 :No O-lines for your host\r\n
     */
    const std::string ERR_NOOPERHOST = "491";
    const std::string ERR_NOOPERHOST_MSG = " :No O-lines for your host\r\n";

    /* ERR_UMODEUNKNOWNFLAG
     * 501 :Unknown MODE flag\r\n
     */
    const std::string ERR_UMODEUNKNOWNFLAG = "501";
    const std::string ERR_UMODEUNKNOWNFLAG_MSG = " :Unknown MODE flag\r\n";

    /* ERR_USERSDONTMATCH
     * 502 :Can't change mode for other users\r\n
     */
    const std::string ERR_USERSDONTMATCH = "502";
    const std::string ERR_USERSDONTMATCH_MSG = " :Can't change mode for other users\r\n";


    /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */
    /*                                REPLY CODES                                */
    /*   -'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-'-,-',-'   */

    /* RPL_NONE
     * 300 :No text\r\n
     */
    const std::string RPL_NONE = "300";
    const std::string RPL_NONE_MSG = " :No text\r\n";

    /* RPL_USERHOST
     * 302 :[<reply>{<space><reply>}]\r\n
     */
    const std::string RPL_USERHOST = "302";
    const std::string RPL_USERHOST_MSG = " :[<reply>{<space><reply>}]\r\n";

    /* RPL_ISON
     * 303 :[<nick> {<space><nick>}]\r\n
     */
    const std::string RPL_ISON = "303";
    const std::string RPL_ISON_MSG = " :[<nick> {<space><nick>}]\r\n";

    /* RPL_AWAY
     * 301 <nick> :<away message>\r\n
     */
    const std::string RPL_AWAY = "301";
    const std::string RPL_AWAY_MSG = " <nick> :<away message>\r\n";

    /* RPL_UNAWAY
     * 305 :You are no longer marked as being away\r\n
     */
    const std::string RPL_UNAWAY = "305";
    const std::string RPL_UNAWAY_MSG = " :You are no longer marked as being away\r\n";

    /* RPL_NOWAWAY
     * 306 :You have been marked as being away\r\n
     */
    const std::string RPL_NOWAWAY = "306";
    const std::string RPL_NOWAWAY_MSG = " :You have been marked as being away\r\n";

    /* RPL_WHOISUSER
     * 311 <nick> <user> <host> * :<real name>\r\n
     */
    const std::string RPL_WHOISUSER = "311";
    const std::string RPL_WHOISUSER_MSG = " <nick> <user> <host> * :<real name>\r\n";

    /* RPL_WHOISSERVER
     * 312 <nick> <server> :<server info>\r\n
     */
    const std::string RPL_WHOISSERVER = "312";
    const std::string RPL_WHOISSERVER_MSG = " <nick> <server> :<server info>\r\n";

    /* RPL_WHOISOPERATOR
     * 313 <nick> :is an IRC operator\r\n
     */
    const std::string RPL_WHOISOPERATOR = "313";
    const std::string RPL_WHOISOPERATOR_MSG = " <nick> :is an IRC operator\r\n";

    /* RPL_WHOISIDLE
     * 317 <nick> <integer> :seconds idle\r\n
     */
    const std::string RPL_WHOISIDLE = "317";
    const std::string RPL_WHOISIDLE_MSG = " <nick> <integer> :seconds idle\r\n";

    /* RPL_ENDOFWHOIS
     * 318 <nick> :End of /WHOIS list\r\n
     */
    const std::string RPL_ENDOFWHOIS = "318";
    const std::string RPL_ENDOFWHOIS_MSG = " <nick> :End of /WHOIS list\r\n";

    /* RPL_WHOISCHANNELS
     * 319 <nick> :{[@|+]<channel><space>}\r\n
     */
    const std::string RPL_WHOISCHANNELS = "319";
    const std::string RPL_WHOISCHANNELS_MSG = " <nick> :{[@|+]<channel><space>}\r\n";

    /* RPL_WHOWASUSER
     * 314 <nick> <user> <host> * :<real name>\r\n
     */
    const std::string RPL_WHOWASUSER = "314";
    const std::string RPL_WHOWASUSER_MSG = " <nick> <user> <host> * :<real name>\r\n";

    /* RPL_ENDOFWHOWAS
     * 369 <nick> :End of WHOWAS\r\n
     */
    const std::string RPL_ENDOFWHOWAS = "369";
    const std::string RPL_ENDOFWHOWAS_MSG = " <nick> :End of WHOWAS\r\n";

    /* RPL_LISTSTART
     * 321 Channel :Users Name\r\n
     */
    const std::string RPL_LISTSTART = "321";
    const std::string RPL_LISTSTART_MSG = " Channel :Users Name\r\n";

    /* RPL_LIST
     * 322 <channel> <# visible> :<topic>\r\n
     */
    const std::string RPL_LIST = "322";
    const std::string RPL_LIST_MSG = " <channel> <# visible> :<topic>\r\n";

    /* RPL_LISTEND
     * 323 :End of /LIST\r\n
     */
    const std::string RPL_LISTEND = "323";
    const std::string RPL_LISTEND_MSG = " :End of /LIST\r\n";

    /* RPL_CHANNELMODEIS
     * 324 <channel> <mode> <mode params>\r\n
     */
    const std::string RPL_CHANNELMODEIS = "324";
    const std::string RPL_CHANNELMODEIS_MSG = " <channel> <mode> <mode params>\r\n";

    /* RPL_NOTOPIC
     * 331 <channel> :No topic is set\r\n
     */
    const std::string RPL_NOTOPIC = "331";
    const std::string RPL_NOTOPIC_MSG = " <channel> :No topic is set\r\n";

    /* RPL_TOPIC
     * 332 <channel> :<topic>\r\n
     */
    const std::string RPL_TOPIC = "332";
    const std::string RPL_TOPIC_MSG = " <channel> :<topic>\r\n";

    /* RPL_INVITING
     * 341 <channel> <nick>\r\n
     */
    const std::string RPL_INVITING = "341";
    const std::string RPL_INVITING_MSG = " <channel> <nick>\r\n";

    /* RPL_SUMMONING
     * 342 <user> :Summoning user to IRC\r\n
     */
    const std::string RPL_SUMMONING = "342";
    const std::string RPL_SUMMONING_MSG = " <user> :Summoning user to IRC\r\n";

    /* RPL_VERSION
     * 351 <version>.<debuglevel> <server> :<comments>\r\n
     */
    const std::string RPL_VERSION = "351";
    const std::string RPL_VERSION_MSG = " <version>.<debuglevel> <server> :<comments>\r\n";

    /* RPL_WHOREPLY
     * 352 <channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>\r\n
     */
    const std::string RPL_WHOREPLY = "352";
    const std::string RPL_WHOREPLY_MSG = " <channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>\r\n";

    /* RPL_ENDOFWHO
     * 315 <name> :End of /WHO list\r\n
     */
    const std::string RPL_ENDOFWHO = "315";
    const std::string RPL_ENDOFWHO_MSG = " <name> :End of /WHO list\r\n";

    /* RPL_NAMREPLY
     * 353 <channel> :[[@|+]<nick> [[@|+]<nick> [...]]]\r\n
     */
    const std::string RPL_NAMREPLY = "353";
    const std::string RPL_NAMREPLY_MSG = " <channel> :[[@|+]<nick> [[@|+]<nick> [...]]]\r\n";

    /* RPL_ENDOFNAMES
     * 366 <channel> :End of /NAMES list\r\n
     */
    const std::string RPL_ENDOFNAMES = "366";
    const std::string RPL_ENDOFNAMES_MSG = " <channel> :End of /NAMES list\r\n";

    /* RPL_LINKS
     * 364 <mask> <server> :<hopcount> <server info>\r\n
     */
    const std::string RPL_LINKS = "364";
    const std::string RPL_LINKS_MSG = " <mask> <server> :<hopcount> <server info>\r\n";

    /* RPL_ENDOFLINKS
     * 365 <mask> :End of /LINKS list\r\n
     */
    const std::string RPL_ENDOFLINKS = "365";
    const std::string RPL_ENDOFLINKS_MSG = " <mask> :End of /LINKS list\r\n";

    /* RPL_BANLIST
     * 367 <channel> <banid>\r\n
     */
    const std::string RPL_BANLIST = "367";
    const std::string RPL_BANLIST_MSG = " <channel> <banid>\r\n";

    /* RPL_ENDOFBANLIST
     * 368 <channel> :End of channel ban list\r\n
     */
    const std::string RPL_ENDOFBANLIST = "368";
    const std::string RPL_ENDOFBANLIST_MSG = " <channel> :End of channel ban list\r\n";

    /* RPL_INFO
     * 371 :<string>\r\n
     */
    const std::string RPL_INFO = "371";
    const std::string RPL_INFO_MSG = " :<string>\r\n";

    /* RPL_ENDOFINFO
     * 374 :End of /INFO list\r\n
     */
    const std::string RPL_ENDOFINFO = "374";
    const std::string RPL_ENDOFINFO_MSG = " :End of /INFO list\r\n";

    /* RPL_MOTDSTART
     * 375 :- <server> Message of the day -\r\n
     */
    const std::string RPL_MOTDSTART = "375";
    const std::string RPL_MOTDSTART_MSG = " :- <server> Message of the day -\r\n";

    /* RPL_MOTD
     * 372 :- <text>\r\n
     */
    const std::string RPL_MOTD = "372";
    const std::string RPL_MOTD_MSG = " :- <text>\r\n";

    /* RPL_ENDOFMOTD
     * 376 :End of /MOTD command\r\n
     */
    const std::string RPL_ENDOFMOTD = "376";
    const std::string RPL_ENDOFMOTD_MSG = " :End of /MOTD command\r\n";

    /* RPL_YOUREOPER
     * 381 :You are now an IRC operator\r\n
     */
    const std::string RPL_YOUREOPER = "381";
    const std::string RPL_YOUREOPER_MSG = " :You are now an IRC operator\r\n";

    /* RPL_REHASHING
     * 382 <config file> :Rehashing\r\n
     */
    const std::string RPL_REHASHING = "382";
    const std::string RPL_REHASHING_MSG = " <config file> :Rehashing\r\n";

    /* RPL_TIME
     * 391 <server> :<time string>\r\n
     */
    const std::string RPL_TIME = "391";
    const std::string RPL_TIME_MSG = " <server> :<time string>\r\n";

    /* RPL_USERSSTART
     * 392 :UserID Terminal Host\r\n
     */
    const std::string RPL_USERSSTART = "392";
    const std::string RPL_USERSSTART_MSG = " :UserID Terminal Host\r\n";

    /* RPL_USERS
     * 393 :%-8s %-9s %-8s\r\n
     */
    const std::string RPL_USERS = "393";
    const std::string RPL_USERS_MSG = " :%-8s %-9s %-8s\r\n";

    /* RPL_ENDOFUSERS
     * 394 :End of users\r\n
     */
    const std::string RPL_ENDOFUSERS = "394";
    const std::string RPL_ENDOFUSERS_MSG = " :End of users\r\n";

    /* RPL_NOUSERS
     * 395 :Nobody logged in\r\n
     */
    const std::string RPL_NOUSERS = "395";
    const std::string RPL_NOUSERS_MSG = " :Nobody logged in\r\n";
}

#endif // IRC_CODES_HPP