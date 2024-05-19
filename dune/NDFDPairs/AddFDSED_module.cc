////////////////////////////////////////////////////////////////////////
// Class:       AddFDSED
// Plugin Type: analyzer (Unknown Unknown)
// File:        AddFDSED.cc
//
// Crated on 16 May 24 Alex Wilkinson
// Dump SimEnergryDeposits to the ndfd pair HDF5 file,
// Also dump a dummy vertex object that contains the eventID for
// compatibility with later LoadFDDepos.
// This is so we do the ionisation and drift with newer dune software
// (using duneextrapolation) to replicate the way do FD simulation for
// the paired dataset.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/Simulation/SimEnergyDeposit.h"

#include "highfive/H5DataSet.hpp"
#include "highfive/H5File.hpp"
#include "highfive/H5Group.hpp"
#include "highfive/H5Object.hpp"
#include "highfive/H5DataType.hpp"
#include "highfive/H5DataSpace.hpp"

#include <string>
#include <vector>

typedef struct depo {
  int eventID;
  int uniqID;
  double x_end;
  double x_start;
  double y_end;
  double y_start;
  double z_end;
  double z_start;
  double x;
  double y;
  double z;
  double t0_end;
  double t0_start;
  double t0;
  double dx;
  double dEdx;
  double dE;
  int outsideNDLAr;
} depo;

HighFive::CompoundType make_depo() {
  return {
    {"eventID", HighFive::AtomicType<int>{}},
    {"uniqID", HighFive::AtomicType<int>{}},
    {"x_end", HighFive::AtomicType<double>{}},
    {"x_start", HighFive::AtomicType<double>{}},
    {"y_end", HighFive::AtomicType<double>{}},
    {"y_start", HighFive::AtomicType<double>{}},
    {"z_end", HighFive::AtomicType<double>{}},
    {"z_start", HighFive::AtomicType<double>{}},
    {"x", HighFive::AtomicType<double>{}},
    {"y", HighFive::AtomicType<double>{}},
    {"z", HighFive::AtomicType<double>{}},
    {"t0_end", HighFive::AtomicType<double>{}},
    {"t0_start", HighFive::AtomicType<double>{}},
    {"t0", HighFive::AtomicType<double>{}},
    {"dx", HighFive::AtomicType<double>{}},
    {"dEdx", HighFive::AtomicType<double>{}},
    {"dE", HighFive::AtomicType<double>{}},
    {"outsideNDLAr", HighFive::AtomicType<int>{}}
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

HIGHFIVE_REGISTER_TYPE(depo, make_depo)
HIGHFIVE_REGISTER_TYPE(vertex, make_vertex)

namespace extrapolation {
  class AddFDSED;
}

class extrapolation::AddFDSED : public art::EDAnalyzer {
public:
  explicit AddFDSED(fhicl::ParameterSet const& p);

  // Plugins should not be copied or assigned.
  AddFDSED(AddFDSED const&) = delete;
  AddFDSED(AddFDSED&&) = delete;
  AddFDSED& operator=(AddFDSED const&) = delete;
  AddFDSED& operator=(AddFDSED&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

  // Selected optional functions.
  void beginJob() override;
  void endJob() override;

private:
  // Methods

  // Members
  HighFive::File* fFile;

  // Data to write out to hdf5
  std::vector<depo> fSEDDepos;
  std::vector<vertex> fSEDEventIDs;

  // Product labels
  std::string fSEDActiveLabel;
  std::string fSEDOtherLabel;

  std::string fNDFDH5FileLoc;
};


extrapolation::AddFDSED::AddFDSED(fhicl::ParameterSet const& p)
  : EDAnalyzer(p)
{
  fNDFDH5FileLoc  = p.get<std::string>("NDFDH5FileLoc");
  fSEDActiveLabel = p.get<std::string>("SEDActiveLabel");
  fSEDOtherLabel  = p.get<std::string>("SEDOtherLabel");

  consumes<std::vector<sim::SimEnergyDeposit>>(fSEDActiveLabel);
  consumes<std::vector<sim::SimEnergyDeposit>>(fSEDOtherLabel);
}

void extrapolation::AddFDSED::analyze(art::Event const& e)
{
  const auto ActiveSEDs = e.getValidHandle<std::vector<sim::SimEnergyDeposit>>(fSEDActiveLabel);
  const auto OtherSEDs = e.getValidHandle<std::vector<sim::SimEnergyDeposit>>(fSEDOtherLabel);
  int eventID = (int)e.id().event();

  const vertex v = { eventID, -999.0, -999.0, -999.0 };
  fSEDEventIDs.push_back(v);

  for (const sim::SimEnergyDeposit& SED : *ActiveSEDs) {
    const depo d = {
      eventID,
      -999,
      SED.EndX(),
      SED.StartX(),
      SED.EndY(),
      SED.StartY(),
      SED.EndZ(),
      SED.MidPointZ(),
      SED.MidPointY(),
      SED.MidPointZ(),
      SED.T1(),
      SED.T0(),
      SED.T(),
      -999.0,
      -999.0,
      SED.E(),
      0
    };
    fSEDDepos.push_back(d);
  }
  for (const sim::SimEnergyDeposit& SED : *OtherSEDs) {
    const depo d = {
      eventID,
      -999,
      SED.EndX(),
      SED.StartX(),
      SED.EndY(),
      SED.StartY(),
      SED.EndZ(),
      SED.MidPointZ(),
      SED.MidPointY(),
      SED.MidPointZ(),
      SED.T1(),
      SED.T0(),
      SED.T(),
      -999.0,
      -999.0,
      SED.E(),
      0
    };
    fSEDDepos.push_back(d);
  }
}

void extrapolation::AddFDSED::beginJob()
{
  fFile = new HighFive::File(fNDFDH5FileLoc, HighFive::File::Create);
}

void extrapolation::AddFDSED::endJob()
{
  fFile->createDataSet("fd_deps", fSEDDepos);
  fFile->createDataSet("fd_vertices", fSEDEventIDs);
}

DEFINE_ART_MODULE(extrapolation::AddFDSED)
