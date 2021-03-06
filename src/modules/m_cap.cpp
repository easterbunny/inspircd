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
#include "m_cap.h"

/* $ModDesc: Provides the CAP negotiation mechanism seen in ratbox-derived ircds */

/*
CAP LS
:alfred.staticbox.net CAP * LS :multi-prefix sasl
CAP REQ :multi-prefix
:alfred.staticbox.net CAP * ACK :multi-prefix
CAP CLEAR
:alfred.staticbox.net CAP * ACK :-multi-prefix
CAP REQ :multi-prefix
:alfred.staticbox.net CAP * ACK :multi-prefix
CAP LIST
:alfred.staticbox.net CAP * LIST :multi-prefix
CAP END
*/

/** Handle /CAP
 */
class CommandCAP : public Command
{
 public:
	LocalIntExt reghold;
	CommandCAP (Module* mod) : Command(mod, "CAP", 1),
		reghold(EXTENSIBLE_USER, "CAP_REGHOLD", mod)
	{
		works_before_reg = true;
	}

	CmdResult Handle (const std::vector<std::string> &parameters, User *user)
	{
		irc::string subcommand = parameters[0].c_str();

		if (subcommand == "REQ")
		{
			CapEvent Data(creator, "cap_req");

			Data.type = subcommand;
			Data.user = user;
			Data.creator = this->creator;

			if (parameters.size() < 2)
				return CMD_FAILURE;

			// tokenize the input into a nice list of requested caps
			std::string param = parameters[1];
			std::string cap_;
			irc::spacesepstream cap_stream(param);

			while (cap_stream.GetToken(cap_))
			{
				Data.wanted.push_back(cap_);
			}

			reghold.set(user, 1);
			Data.Send();

			if (Data.ack.size() > 0)
			{
				std::string AckResult = irc::stringjoiner(" ", Data.ack, 0, Data.ack.size() - 1).GetJoined();
				user->WriteServ("CAP %s ACK :%s", user->nick.c_str(), AckResult.c_str());
			}

			if (Data.wanted.size() > 0)
			{
				std::string NakResult = irc::stringjoiner(" ", Data.wanted, 0, Data.wanted.size() - 1).GetJoined();
				user->WriteServ("CAP %s NAK :%s", user->nick.c_str(), NakResult.c_str());
			}
		}
		else if (subcommand == "END")
		{
			reghold.set(user, 0);
		}
		else if ((subcommand == "LS") || (subcommand == "LIST"))
		{
			CapEvent Data(creator, subcommand == "LS" ? "cap_ls" : "cap_list");

			Data.type = subcommand;
			Data.user = user;
			Data.creator = this->creator;

			reghold.set(user, 1);
			Data.Send();

			std::string Result;
			if (Data.wanted.size() > 0)
				Result = irc::stringjoiner(" ", Data.wanted, 0, Data.wanted.size() - 1).GetJoined();
			else
				Result = "";

			user->WriteServ("CAP %s %s :%s", user->nick.c_str(), subcommand.c_str(), Result.c_str());
		}
		else if (subcommand == "CLEAR")
		{
			CapEvent Data(creator, "cap_clear");

			Data.type = subcommand;
			Data.user = user;
			Data.creator = this->creator;

			reghold.set(user, 1);
			Data.Send();

			std::string Result = irc::stringjoiner(" ", Data.ack, 0, Data.ack.size() - 1).GetJoined();
			user->WriteServ("CAP %s ACK :%s", user->nick.c_str(), Result.c_str());
		}
		else
		{
			user->WriteNumeric(ERR_INVALIDCAPSUBCOMMAND, "%s %s :Invalid CAP subcommand", user->nick.c_str(), subcommand.c_str());
		}

		return CMD_FAILURE;
	}
};

class ModuleCAP : public Module
{
	CommandCAP cmd;
 public:
	ModuleCAP()
		: cmd(this)
	{
	}

	void init()
	{
		ServerInstance->AddCommand(&cmd);
		ServerInstance->Extensions.Register(&cmd.reghold);

		Implementation eventlist[] = { I_OnCheckReady };
		ServerInstance->Modules->Attach(eventlist, this, sizeof(eventlist)/sizeof(Implementation));
	}

	ModResult OnCheckReady(LocalUser* user)
	{
		/* Users in CAP state get held until CAP END */
		if (cmd.reghold.get(user))
			return MOD_RES_DENY;

		return MOD_RES_PASSTHRU;
	}

	~ModuleCAP()
	{
	}

	Version GetVersion()
	{
		return Version("Client CAP extension support", VF_VENDOR);
	}
};

MODULE_INIT(ModuleCAP)

