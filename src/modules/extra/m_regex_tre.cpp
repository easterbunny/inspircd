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
#include "m_regex.h"
#include <sys/types.h>
#include <tre/regex.h>

/* $ModDesc: Regex Provider Module for TRE Regular Expressions */
/* $CompileFlags: pkgconfincludes("tre","tre/regex.h","") */
/* $LinkerFlags: pkgconflibs("tre","/libtre.so","-ltre") rpath("pkg-config --libs tre") */
/* $ModDep: m_regex.h */

class TRERegexException : public ModuleException
{
public:
	TRERegexException(const std::string& rx, const std::string& error)
		: ModuleException(std::string("Error in regex ") + rx + ": " + error)
	{
	}
};

class TRERegex : public Regex
{
private:
	regex_t regbuf;
	bool irc_lowercase, spaces_to_underscores;

public:
	TRERegex(const std::string& rx, RegexFlags reflags) : Regex(rx)
	{
		irc_lowercase = reflags & REGEX_IRC_LOWERCASE;
		spaces_to_underscores = reflags & REGEX_SPACES_TO_UNDERSCORES;
		int flags = REG_EXTENDED | REG_NOSUB | ((reflags & REGEX_CASE_INSENSITIVE) ? REG_ICASE : 0);
		int errcode;
		errcode = regcomp(&regbuf, rx.c_str(), flags);
		if (errcode)
		{
			// Get the error string into a std::string. YUCK this involves at least 2 string copies.
			std::string error;
			char* errbuf;
			size_t sz = regerror(errcode, &regbuf, NULL, 0);
			errbuf = new char[sz + 1];
			memset(errbuf, 0, sz + 1);
			regerror(errcode, &regbuf, errbuf, sz + 1);
			error = errbuf;
			delete[] errbuf;
			regfree(&regbuf);
			throw TRERegexException(rx, error);
		}
	}

	virtual ~TRERegex()
	{
		regfree(&regbuf);
	}

	virtual bool Matches(const std::string& text)
	{
		std::string matchtext(irc_lowercase ? irc::irc_char_traits::remap(text) : text);
		if(spaces_to_underscores)
			for(std::string::iterator i = matchtext.begin(); i != matchtext.end(); ++i)
				if(*i == ' ')
					*i = '_';
		return regexec(&regbuf, matchtext.c_str(), 0, NULL, 0) == 0;
	}
};

class TREFactory : public RegexFactory {
 public:
	TREFactory(Module* m) : RegexFactory(m, "regex/tre") {}
	Regex* Create(const std::string& expr, RegexFlags flags)
	{
		return new TRERegex(expr, flags);
	}
};

class ModuleRegexTRE : public Module
{
	TREFactory trf;
public:
	ModuleRegexTRE() : trf(this) {}

	void init()
	{
		ServerInstance->Modules->AddService(trf);
	}

	void Prioritize()
	{
		// we are a pure service provider, init us first
		ServerInstance->Modules->SetPriority(this, I_ModuleInit, PRIORITY_FIRST);
	}

	Version GetVersion()
	{
		return Version("Regex Provider Module for TRE Regular Expressions", VF_VENDOR);
	}

	~ModuleRegexTRE()
	{
	}
};

MODULE_INIT(ModuleRegexTRE)
