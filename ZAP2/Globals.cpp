#include "Globals.h"
#include "File.h"

Globals* Globals::Instance;

Globals::Globals()
{
	Instance = this;

	symbolMap = std::map <uint32_t, std::string>();
	genSourceFile = true;
	lastScene = nullptr;

	game = ZGame::OOT;
}

void Globals::GenSymbolMap(std::string symbolMapPath)
{
	auto symbolLines = File::ReadAllLines(symbolMapPath);

	for (std::string symbolLine : symbolLines)
	{
		auto split = StringHelper::Split(symbolLine, " ");
		uint32_t addr = strtoul(split[0].c_str(), NULL, 16);
		std::string symbolName = split[1];

		symbolMap[addr] = symbolName;
	}
}