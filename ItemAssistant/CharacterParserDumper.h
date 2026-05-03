#ifndef CHARACTERPARSERDUMPER_H
#define CHARACTERPARSERDUMPER_H

#include <Parsers/AOMessageBase.h>

class CharacterParserDumper
{
public:
    CharacterParserDumper(std::tstring const& folder_name);
    ~CharacterParserDumper();

    void OnAOServerMessage(Parsers::AOMessageBase &msg);

private:
	void DumpMessageToFile(std::tstring const& file_name, Parser &msg);
	void DumpMessageToStream(std::ostream &out, Parser &msg);

	std::tstring m_folder; // Now uses the safe, standard string

};

#endif // CHARACTERPARSERDUMPER_H
