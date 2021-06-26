#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <random>

class IIndividual
{
public:
  virtual double evaluate() = 0;
  virtual std::unique_ptr<IIndividual> selfReproduce() = 0;
};

template<typename RandomNumberGenerator>
class GeneticAlgorithm
{
public:
  template<typename IndividualGenerator>
  GeneticAlgorithm(double iInterGenerationalSurvivalRate, RandomNumberGenerator &iRng, double iSelectionStdDev, size_t iMaxPopulation, const IndividualGenerator &iIndividualGenerator)
    : mInterGenerationalSurvivalRate(iInterGenerationalSurvivalRate)
    , mRng(iRng)
    , mSelectionDistribution(0.0, iSelectionStdDev)
    , mMaxPopulationSize(iMaxPopulation)
  {
    while (mPopulation.size() < mMaxPopulationSize)
    {
      mPopulation.emplace_back(iIndividualGenerator(), 0.0);
    }
  }

  void doOneGeneration()
  {
    for (auto &&wElement : mPopulation)
    {
      wElement.mScore = wElement.mIndividual->evaluate();
    }

    std::sort(std::begin(mPopulation), std::end(mPopulation), [](auto &&iLeft, auto &&iRight)
    {
      return iLeft.mScore > iRight.mScore;
    });
    auto wNumberOfSurvivors = static_cast<size_t>(mPopulation.size() * mInterGenerationalSurvivalRate);
    mPopulation.resize(wNumberOfSurvivors);

    while (mPopulation.size() < mMaxPopulationSize)
    {
      auto wSelectionIndex = std::clamp(static_cast<size_t>(std::abs(mSelectionDistribution(mRng))), size_t(0), wNumberOfSurvivors - 1);
      mPopulation.emplace_back(mPopulation[wSelectionIndex].mIndividual->selfReproduce(), 0.0);
    }
  }

private:
  struct ScoredIndividual
  {
    std::unique_ptr<IIndividual> mIndividual;
    double mScore;
  };
  std::vector<ScoredIndividual> mPopulation;
  double mInterGenerationalSurvivalRate;
  RandomNumberGenerator &mRng;
  std::normal_distribution<> mSelectionDistribution;
  size_t mMaxPopulationSize;
};