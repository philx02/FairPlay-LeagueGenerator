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

template<typename RandomNumberGenerator>
class LeagueIndividual : public IIndividual
{
public:
  LeagueIndividual(League &&iLeague, double iLeagueMean, double iNumberOfMuationStdDev, RandomNumberGenerator &iRng)
    : mLeague(std::move(iLeague))
    , mLeagueMean(iLeagueMean)
    , mTeamSelectionDistro1(0, mLeague.mTeams.size() - 1)
    , mTeamSelectionDistro2(0, mLeague.mTeams.size() - 2)
    , mNumberOfMutationDistro(0, iNumberOfMuationStdDev)
    , mRng(iRng)
  {
  }

  double evaluate() final
  {
    std::vector<double> wTeamGradeMeans;
    std::vector<double> wTeamFieldDistanceMeans;

    for (auto &&wTeam : mLeague.mTeams)
    {
      wTeamGradeMeans.emplace_back(std::accumulate(std::begin(wTeam.mPlayers), std::end(wTeam.mPlayers), 0.0, [](auto &&iSum, auto &&iPlayer) { return std::move(iSum) + iPlayer->mGrade; }) / wTeam.mPlayers.size());
      wTeamFieldDistanceMeans.emplace_back(std::accumulate(std::begin(wTeam.mPlayers), std::end(wTeam.mPlayers), 0.0, [&](auto &&iSum, auto &&iPlayer)
      {
        auto &&wFieldDistanceIter = std::find_if(std::begin(iPlayer->mFieldDistances), std::end(iPlayer->mFieldDistances), [&](auto &&iFieldCandidate)
        {
          return iFieldCandidate.mField == wTeam.mField;
        });
        return std::move(iSum) + wFieldDistanceIter->mDistance;
      }) / wTeam.mPlayers.size());
    }

    auto wLeagueVariance = std::accumulate(std::begin(wTeamGradeMeans), std::end(wTeamGradeMeans), 0.0, [&](auto &&iAcc, auto &&iValue)
    {
      auto wTerm = iValue - mLeagueMean;
      return std::move(iAcc) + (wTerm * wTerm / (wTeamGradeMeans.size()));
    });
    auto wLeagueMeanDistances = std::accumulate(std::begin(wTeamFieldDistanceMeans), std::end(wTeamFieldDistanceMeans), 0.0) / wTeamFieldDistanceMeans.size();

    return -(wLeagueVariance + wLeagueMeanDistances);
  }

  std::unique_ptr<IIndividual> selfReproduce() final
  {
    auto wLeague = League(mLeague);
    {
      auto wNumberOfMutations = static_cast<size_t>(std::abs(mNumberOfMutationDistro(mRng))) + 1;
      for (size_t wMutationIter = 0; wMutationIter < wNumberOfMutations; ++wMutationIter)
      {
        auto wTeam1Index = mTeamSelectionDistro1(mRng);
        auto wTeam2Index = mTeamSelectionDistro2(mRng);
        if (wTeam1Index == wTeam2Index)
        {
          wTeam1Index = mLeague.mTeams.size() - 1;
        }
        Team &wTeam1 = wLeague.mTeams[wTeam1Index];
        Team &wTeam2 = wLeague.mTeams[wTeam2Index];
        auto wPlayer1Index = std::uniform_int_distribution<size_t>(0, wTeam1.mPlayers.size() - 1)(mRng);
        auto wPlayer2Index = std::uniform_int_distribution<size_t>(0, wTeam2.mPlayers.size() - 1)(mRng);
        std::swap(wTeam1.mPlayers[wPlayer1Index], wTeam2.mPlayers[wPlayer2Index]);
      }
    }
    {
      auto wNumberOfMutations = static_cast<size_t>(std::abs(mNumberOfMutationDistro(mRng) / 10.0));
      for (size_t wMutationIter = 0; wMutationIter < wNumberOfMutations; ++wMutationIter)
      {
        auto wOddsOfPickingFromRemainingFields = wLeague.mRemainingFields.size() / (wLeague.mRemainingFields.size() + wLeague.mTeams.size());
        if (std::uniform_real_distribution<>(0.0, 1.0)(mRng) <= wOddsOfPickingFromRemainingFields)
        {
          auto wTeamIndex = mTeamSelectionDistro1(mRng);
          auto wRemainingFieldIndex = std::uniform_int_distribution<size_t>(0, wLeague.mRemainingFields.size() - 1)(mRng);
          std::swap(wLeague.mTeams[wTeamIndex].mField, wLeague.mRemainingFields[wRemainingFieldIndex]);
        }
        else
        {
          auto wTeam1Index = mTeamSelectionDistro1(mRng);
          auto wTeam2Index = mTeamSelectionDistro2(mRng);
          std::swap(wLeague.mTeams[wTeam1Index].mField, wLeague.mTeams[wTeam2Index].mField);
        }
      }
    }
    return std::make_unique<LeagueIndividual>(std::move(wLeague), mLeagueMean, mNumberOfMutationDistro.stddev(), mRng);
  }

private:
  League mLeague;
  double mLeagueMean;
  std::uniform_int_distribution<size_t> mTeamSelectionDistro1;
  std::uniform_int_distribution<size_t> mTeamSelectionDistro2;
  std::normal_distribution<> mNumberOfMutationDistro;
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
std::unique_ptr<LeagueIndividual<RandomNumberGenerator>> generateLeagueIndividual(const std::vector<Player> &iPlayers, const std::vector<Field> &iFields, size_t iNumberOfTeams, double iLeagueMean, double iNumberOfMuationStdDev, RandomNumberGenerator &iRng)
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

  return std::make_unique<LeagueIndividual<RandomNumberGenerator>>(std::move(wLeague), iLeagueMean, iNumberOfMuationStdDev, iRng);
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
    return generateLeagueIndividual(wPlayers, wFields, wNumberOfTeams, wLeagueMean, 15.0, wRng);
  });

  for (size_t wIter = 0; wIter < 1000; ++wIter)
  {
    wGenAlgo.doOneGeneration();
  }
  wGenAlgo.doOneGeneration();

  return 0;
}