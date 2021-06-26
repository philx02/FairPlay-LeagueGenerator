#include "LeagueGenerator.h"
#include <iostream>
#include <string>

#include <numeric>

using namespace std;

struct Field
{
	size_t mId;
	std::string mName;
};

struct FieldDistance
{
	const Field *mField;
	double mDistance;
};

struct Player
{
	std::string mName;
	double mGrade;
	std::vector<FieldDistance> mFieldDistances;
};

struct Team
{
	std::vector<const Player *> mPlayers;
	const Field *mField;
};

struct League
{
	std::vector<const Field *> mRemainingFields;
	std::vector<Team> mTeams;
};

template<typename	RandomNumberGenerator>
class LeagueIndividual : public IIndividual
{
public:
	LeagueIndividual(League &&iLeague, double iLeagueMean, RandomNumberGenerator &iRng)
		: mLeague(std::move(iLeague))
		, mLeagueMean(iLeagueMean)
		, mTeamSelectionDistro(0, mLeague.mTeams.size() - 1)
		, mRng(iRng)
	{
	}

	double evaluate() final
	{
		std::vector<double> wTeamMeans;
		for (auto &&wTeam : mLeague.mTeams)
		{
			wTeamMeans.emplace_back(std::accumulate(std::begin(wTeam.mPlayers), std::end(wTeam.mPlayers), 0.0, [](auto &&iSum, auto &&iPlayer) { return std::move(iSum) + iPlayer->mGrade; }) / wTeam.mPlayers.size());
		}
		auto wLeagueVariance = std::accumulate(std::begin(wTeamMeans), std::end(wTeamMeans), 0.0, [&](auto &&iAcc, auto &&iValue)
		{
			auto wTerm = iValue - mLeagueMean;
			return std::move(iAcc) + (wTerm * wTerm / (wTeamMeans.size()));
		});
		return -wLeagueVariance;
	}

	std::unique_ptr<IIndividual> selfReproduce() final
	{
		auto wLeague = League(mLeague);
		return std::make_unique<LeagueIndividual>(std::move(wLeague), mLeagueMean, mRng);
	}

private:
	League mLeague;
	double mLeagueMean;
	std::uniform_int_distribution<size_t> mTeamSelectionDistro;
	RandomNumberGenerator &mRng;
};

std::vector<Field> generateFields(size_t iQuantity)
{
	std::vector<Field> wFields;
	for (size_t wIter = 0; wIter < iQuantity; ++wIter)
	{
		wFields.emplace_back(wIter, "Field " + std::to_string(wIter));
	}
	return wFields;
}

template<typename RandomNumberGenerator>
std::vector<Player> generatePlayers(const std::vector<Field> &iFields, RandomNumberGenerator &iRng, size_t iQuantity)
{
	static std::uniform_real_distribution<> wGradeDistro(0, 100);
	static std::uniform_real_distribution<> wDistanceDistro(0, 50);
	std::vector<Player> wPlayers;
	for (size_t wIter = 0; wIter < iQuantity; ++wIter)
	{
		std::vector<FieldDistance> wFieldDistances;
		for (auto &&wField : iFields)
		{
			wFieldDistances.emplace_back(&wField, wDistanceDistro(iRng));
		}
		wPlayers.emplace_back("Gaetan", wGradeDistro(iRng), std::move(wFieldDistances));
	}
	return wPlayers;
}

template<typename RandomNumberGenerator>
std::unique_ptr<LeagueIndividual<RandomNumberGenerator>> generateLeagueIndividual(const std::vector<Player> &iPlayers, const std::vector<Field> &iFields, size_t iNumberOfTeams, double iLeagueMean, RandomNumberGenerator &iRng)
{
	std::vector<const Player *> wPlayersPtr;
	std::transform(std::begin(iPlayers), std::end(iPlayers), std::back_inserter(wPlayersPtr), [&](const Player &iPlayer)
	{
		return &iPlayer;
	});
	std::shuffle(std::begin(wPlayersPtr), std::end(wPlayersPtr), iRng);

	auto wLeague = League();
	wLeague.mTeams.resize(iNumberOfTeams);
	auto wTeamIter = wLeague.mTeams.begin();
	for (auto &&wPlayer : wPlayersPtr)
	{
		wTeamIter->mPlayers.emplace_back(wPlayer);
		if (++wTeamIter == std::end(wLeague.mTeams))
		{
			wTeamIter = wLeague.mTeams.begin();
		}
	}

	std::vector<const Field *> wFieldsPtr;
	std::transform(std::begin(iFields), std::end(iFields), std::back_inserter(wFieldsPtr), [](auto &&iField) { return &iField; });
	std::shuffle(std::begin(wFieldsPtr), std::end(wFieldsPtr), iRng);
	for (auto &&wTeam : wLeague.mTeams)
	{
		wTeam.mField = wFieldsPtr.back();
		wFieldsPtr.pop_back();
	}
	wLeague.mRemainingFields = wFieldsPtr;

	return std::make_unique<LeagueIndividual<RandomNumberGenerator>>(std::move(wLeague), iLeagueMean, iRng);
}

int main()
{
	auto wRng = std::mt19937_64(0);

	auto wFields = generateFields(20);
	auto wPlayers = generatePlayers(wFields, wRng, 200);

	auto wLeagueMean = std::accumulate(std::begin(wPlayers), std::end(wPlayers), 0.0, [](auto &&iSum, auto &&iPlayer) { return std::move(iSum) + iPlayer.mGrade; }) / wPlayers.size();

	auto wTargetTeamSize = size_t(12);

	auto wNumberOfTeams = wPlayers.size() / wTargetTeamSize;

	if (wNumberOfTeams <= 1)
	{
		throw std::invalid_argument("Insufficient players.");
	}
	if (wNumberOfTeams > wFields.size())
	{
		throw std::invalid_argument("Number of fields smaller than the number of teams.");
	}

	auto wGenAlgo = GeneticAlgorithm(0.25, wRng, 10.0, 100, [&]()
	{
		return generateLeagueIndividual(wPlayers, wFields, wNumberOfTeams, wLeagueMean, wRng);
	});

	for (size_t wIter = 0; wIter < 100; ++wIter)
	{
		wGenAlgo.doOneGeneration();
	}

	return 0;
}