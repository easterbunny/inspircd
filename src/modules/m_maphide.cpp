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

/* $ModDesc: Hide /MAP and /LINKS in the same form as ircu (mostly useless) */

class ModuleMapHide : public Module
{
	std::string url;
 public:
	void init()
	{
		Implementation eventlist[] = { I_OnPreCommand };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
	}

	void ReadConfig(ConfigReadStatus&)
	{
		url = ServerInstance->Config->GetTag("security")->getString("maphide");
	}

	ModResult OnPreCommand(std::string &command, std::vector<std::string> &parameters, LocalUser *user, bool validated, const std::string &original_line)
	{
		if (!IS_OPER(user) && !url.empty() && (command == "MAP" || command == "LINKS"))
		{
			user->WriteServ("NOTICE %s :/%s has been disabled; visit %s", user->nick.c_str(), command.c_str(), url.c_str());
			return MOD_RES_DENY;
		}
		else
			return MOD_RES_PASSTHRU;
	}

	virtual ~ModuleMapHide()
	{
	}

	virtual Version GetVersion()
	{
		return Version("Hide /MAP and /LINKS in the same form as ircu (mostly useless)", VF_VENDOR);
	}
};

MODULE_INIT(ModuleMapHide)

