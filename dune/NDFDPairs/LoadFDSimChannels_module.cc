////////////////////////////////////////////////////////////////////////
// Class:       LoadFDSimChannels
// Plugin Type: producer (Unknown Unknown)
// File:        LoadFDSimChannels_module.cc
//
// Crated on 27 Feb 25 Alex Wilkinson
// Load FD SimChannel from hdf5 file.
// Ideally would like to load SimEnergyDeposit but old LArG4 expects to
// run on simb::MCTruth objects. So instead loading SimEnergyDeposits
// with new larsoft and doing ionisation and drift there to get
// SimChannel. Move SimChannel here to run old TDR signal simulation and
// reco.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"
#include "lardataobj/Simulation/SimChannel.h"

#include "highfive/H5DataSet.hpp"
#include "highfive/H5File.hpp"
#include "highfive/H5Group.hpp"
#include "highfive/H5Object.hpp"
#include "highfive/H5DataType.hpp"
#include "highfive/H5DataSpace.hpp"

#include <string>
#include <vector>
#include <map>
#include <numeric>

typedef struct simChannelIonisationFD {
  int trackID;
  unsigned int tdc;
  double numberElectrons;
  double x;
  double y;
  double z;
  double energy;
} simChannelIonisationFD;

HighFive::CompoundType make_simChannelIonisationFD() {
  return {
    {"trackID", HighFive::AtomicType<int>{}},
    {"tdc", HighFive::AtomicType<unsigned int>{}},
    {"numberElectrons", HighFive::AtomicType<double>{}},
    {"x", HighFive::AtomicType<double>{}},
    {"y", HighFive::AtomicType<double>{}},
    {"z", HighFive::AtomicType<double>{}},
    {"energy", HighFive::AtomicType<double>{}}
  };
}

typedef struct vertex {
  int eventID;
  double x_vert;
  double y_vert;
  double z_vert;
} depoVtx;

HighFive::CompoundType make_vertex() {
  return {
    {"eventID", HighFive::AtomicType<int>{}},
    {"x_vert", HighFive::AtomicType<double>{}},
    {"y_vert", HighFive::AtomicType<double>{}},
    {"z_vert", HighFive::AtomicType<double>{}},
  };
}

HIGHFIVE_REGISTER_TYPE(simChannelIonisationFD, make_simChannelIonisationFD)
HIGHFIVE_REGISTER_TYPE(vertex, make_vertex)

namespace extrapolation {
  class LoadFDSimChannels;
}

class extrapolation::LoadFDSimChannels : public art::EDProducer {
public:
  explicit LoadFDSimChannels(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.
  // The line "hep_hpc::hdf5::File fFile;" somehow causes the destructor to have noexcept(false)
  // then there is a build error "looser throw specifier ..." coming from the destructor being
  // noexcept in ED{Analyzer/Producer}. Defining the destructor as below is the only way I found
  // to remove  build error. duhduhduhhhhh code????

  // Plugins should not be copied or assigned.
  LoadFDSimChannels(LoadFDSimChannels const&) = delete;
  LoadFDSimChannels(LoadFDSimChannels&&) = delete;
  LoadFDSimChannels& operator=(LoadFDSimChannels const&) = delete;
  LoadFDSimChannels& operator=(LoadFDSimChannels&&) = delete;

  // Required functions.
  void produce(art::Event& e) override;

  // Selected optional functions.
  void beginJob() override;

  // My function.
  void reset();

private:
  HighFive::File* fFile;
  std::vector<int> fEventIDs;
  std::map<int, std::map<raw::ChannelID_t, std::vector<simChannelIonisationFD>>> fSCs;

  std::string fNDFDH5FileLoc;
};

extrapolation::LoadFDSimChannels::LoadFDSimChannels(fhicl::ParameterSet const& p)
{
  fNDFDH5FileLoc = p.get<std::string>("NDFDH5FileLoc");

  produces<std::vector<sim::SimChannel>>();
  produces<std::vector<sim::SimEnergyDeposit>>("eventID");
}

void extrapolation::LoadFDSimChannels::produce(art::Event& e)
{
  if (!fEventIDs.size()) {
    throw cet::exception("LoadFDSimChannels")
      << "Number of events exceed unique eventIDs in " << fNDFDH5FileLoc
      << " - Line " << __LINE__ << " in file " << __FILE__ << "\n";
  }
  const int currEventID = fEventIDs.back();
  fEventIDs.pop_back();
  const std::map<raw::ChannelID_t, std::vector<simChannelIonisationFD>> eventSCs =
    fSCs[currEventID];

  std::cout << "eventID = " << currEventID << ": "
            << "Loading SimChannels from " << eventSCs.size() << " channels, "
            << "totalling " << std::accumulate(
              eventSCs.begin(), eventSCs.end(), 0, [](int a, auto b){ return a + b.second.size(); }
            ) << " IDEs\n";

  // SED vector with single SED that stores the eventID for matching with ND data later
  // eventID is being stored in the trackID member
  auto evNum = std::make_unique<std::vector<sim::SimEnergyDeposit>>();
  geo::Point_t posStart = geo::Point_t(0, 0, 0);
  geo::Point_t posEnd = geo::Point_t(0, 0, 0);
  sim::SimEnergyDeposit ID = sim::SimEnergyDeposit(
    0, 0, 0.0, posStart, posEnd, 0, 0, currEventID, 0
  );
  evNum->push_back(ID);

  auto SCs = std::make_unique<std::vector<sim::SimChannel>>();
  for (auto chIDEs : eventSCs) {
    sim::SimChannel SC(chIDEs.first);

    for (const simChannelIonisationFD IDE : chIDEs.second) {
      const double xyz[3] = { IDE.x, IDE.y, IDE.z };
      SC.AddIonizationElectrons(IDE.trackID, IDE.tdc, IDE.numberElectrons, xyz, IDE.energy);
    }

    SCs->push_back(SC);
  }

  e.put(std::move(SCs));
  e.put(std::move(evNum), "eventID");
}

void extrapolation::LoadFDSimChannels::beginJob()
{
  std::cout << "Readng data from " << fNDFDH5FileLoc << "\n";
  fFile = new HighFive::File(fNDFDH5FileLoc, HighFive::File::ReadOnly);

  // Read in data
  for (const std::string eventIDStr : fFile->getGroup("fd_simchannels").listObjectNames()) {
    const int eventID = std::stoi(eventIDStr);
    const std::string eventGroupPath = "fd_simchannels/" + eventIDStr;

    for (const std::string chStr : fFile->getGroup(eventGroupPath).listObjectNames()) {
      const raw::ChannelID_t ch = (raw::ChannelID_t)std::stol(chStr);
      const std::string chGroupPath = eventGroupPath + "/" + chStr;

      std::vector<simChannelIonisationFD> readSimChannelIonisationFD;
      HighFive::DataSet datasetSimChannelIonisationFD = fFile->getDataSet(chGroupPath);
      datasetSimChannelIonisationFD.read(readSimChannelIonisationFD);

      fSCs[eventID][ch] = readSimChannelIonisationFD;
    }
  }

  for (const auto evSCs : fSCs) {
    fEventIDs.push_back(evSCs.first);
    }
  if (!fEventIDs.size()) {
    throw cet::exception("LoadFDSimChannels")
      << "No data to read in " << fNDFDH5FileLoc
      << " - Line " << __LINE__ << " in file " << __FILE__ << "\n";
  }
  std::sort(fEventIDs.begin(), fEventIDs.end(), std::greater<>()); // sort desc so to pop from back
  std::cout << fEventIDs.size() << " unique eventIDs in input\n";
}

DEFINE_ART_MODULE(extrapolation::LoadFDSimChannels)
