/*       +------------------------------------+
 *       | Inspire Internet Relay Chat Daemon |
 *       +------------------------------------+
 *
 *  InspIRCd: (C) 2002-2011 InspIRCd Development Team
 * See: http://wiki.inspircd.org/Credits
 *
 * This program is free but copyrighted software; see
 *            the file COPYING for details.
 *
 * ---------------------------------------------------
 */

#include "inspircd.h"

/* $ModDesc: Provides support for the CHGHOST command */

/** Handle /CHGHOST
 */
class CommandChghost : public Command
{
 private:
	char* hostmap;
 public:
	CommandChghost(Module* Creator, char* hmap) : Command(Creator,"CHGHOST", 2), hostmap(hmap)
	{
		flags_needed = 'o';
		syntax = "<nick> <newhost>";
		TRANSLATE3(TR_NICK, TR_TEXT, TR_END);
	}

	CmdResult Handle(const std::vector<std::string> &parameters, User *user)
	{
		const char* x = parameters[1].c_str();

		if (parameters[1].length() > 63)
		{
			user->WriteServ("NOTICE %s :*** CHGHOST: Host too long", user->nick.c_str());
			return CMD_FAILURE;
		}

		for (; *x; x++)
		{
			if (!hostmap[(unsigned char)*x])
			{
				user->WriteServ("NOTICE %s :*** CHGHOST: Invalid characters in hostname", user->nick.c_str());
				return CMD_FAILURE;
			}
		}

		User* dest = ServerInstance->FindNick(parameters[0]);

		if (!dest)
		{
			user->WriteNumeric(ERR_NOSUCHNICK, "%s %s :No such nick/channel", user->nick.c_str(), parameters[0].c_str());
			return CMD_FAILURE;
		}

		if (IS_LOCAL(dest))
		{
			if ((dest->ChangeDisplayedHost(parameters[1].c_str())) && (!ServerInstance->ULine(user->server)))
			{
				// fix by brain - ulines set hosts silently
				ServerInstance->SNO->WriteGlobalSno('a', std::string(user->nick)+" used CHGHOST to make the displayed host of "+dest->nick+" become "+dest->dhost);
			}
		}

		return CMD_SUCCESS;
	}

	RouteDescriptor GetRouting(User* user, const std::vector<std::string>& parameters)
	{
		User* dest = ServerInstance->FindNick(parameters[0]);
		if (dest)
			return ROUTE_OPT_UCAST(dest->server);
		return ROUTE_LOCALONLY;
	}
};


class ModuleChgHost : public Module
{
	CommandChghost cmd;
	char hostmap[256];
 public:
	ModuleChgHost() : cmd(this, hostmap) {}

	void init()
	{
		ServerInstance->AddCommand(&cmd);
	}


	void ReadConfig(ConfigReadStatus&)
	{
		std::string hmap = ServerInstance->Config->GetTag("hostname")->getString("charmap");

		if (hmap.empty())
			hmap = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-_/0123456789";

		memset(hostmap, 0, sizeof(hostmap));
		for (std::string::iterator n = hmap.begin(); n != hmap.end(); n++)
			hostmap[(unsigned char)*n] = 1;
	}

	~ModuleChgHost()
	{
	}

	Version GetVersion()
	{
		return Version("Provides support for the CHGHOST command", VF_OPTCOMMON | VF_VENDOR);
	}

};

MODULE_INIT(ModuleChgHost)
