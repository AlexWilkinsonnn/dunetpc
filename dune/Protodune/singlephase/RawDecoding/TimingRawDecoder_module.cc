////////////////////////////////////////////////////////////////////////
// Class:       TimingRawDecoder
// Module Type: producer
// File:        TimingRawDecoder_module.cc
//
// Generated at Thu Jul  6 18:31:48 2017 by Antonino Sergi,32 1-A14,+41227678738, using artmod
// from cetpkgsupport v1_11_00.
////////////////////////////////////////////////////////////////////////

// art includes
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"

// artdaq and dune-raw-data includes
#include "dune-raw-data/Overlays/TimingFragment.hh"

// larsoft includes
#include "lardataobj/RawData/RDTimeStamp.h"

// ROOT includes
#include "TH1.h"

// C++ Includes
#include <memory>
#include <iostream>
#include <fstream>

namespace dune {
  class TimingRawDecoder;
}

using std::string;
using std::cout;
using std::endl;
using std::ofstream;

class dune::TimingRawDecoder : public art::EDProducer {
public:
  explicit TimingRawDecoder(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TimingRawDecoder(TimingRawDecoder const &) = delete;
  TimingRawDecoder(TimingRawDecoder &&) = delete;
  TimingRawDecoder & operator = (TimingRawDecoder const &) = delete;
  TimingRawDecoder & operator = (TimingRawDecoder &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;
  void reconfigure(const fhicl::ParameterSet &pset);
  void printParameterSet();
  void beginJob() override;

  void setRootObjects();

private:

  // Declare member data here.
  std::string fRawDataLabel;
  std::string fOutputDataLabel;
  bool fUseChannelMap;
  bool fDebug;
  bool fMakeTree;
  bool fMakeEventTimeFile = false;

  TH1I * fHTimestamp;
  TH1I * fHTrigType;
  TH1I * fHTimestampDelta;

  ULong64_t fPrevTimestamp;
};


dune::TimingRawDecoder::TimingRawDecoder(fhicl::ParameterSet const & pset)
// :
// Initialize member data here.
{
  art::ServiceHandle<art::TFileService> fs;
  //fs->registerFileSwitchCallback(this, &TimingRawDecoder::setRootObjects);
  setRootObjects();

  reconfigure(pset);
  // Call appropriate produces<>() functions here.
  produces< std::vector<raw::RDTimeStamp> > (fOutputDataLabel);  
}

void dune::TimingRawDecoder::reconfigure(fhicl::ParameterSet const& pset) {

  fRawDataLabel = pset.get<std::string>("RawDataLabel");
  fOutputDataLabel = pset.get<std::string>("OutputDataLabel");
  fUseChannelMap = pset.get<bool>("UseChannelMap");
  fDebug = pset.get<bool>("Debug");
  fMakeTree = pset.get<bool>("MakeTree");
  pset.get_if_present<bool>("MakeEventTimeFile", fMakeEventTimeFile);
  fPrevTimestamp=0;
  if(fDebug) printParameterSet();

}

void dune::TimingRawDecoder::printParameterSet(){

  for(int i=0;i<20;i++) std::cout << "=";
  std::cout << std::endl;
  std::cout << "Parameter Set" << std::endl;
  for(int i=0;i<20;i++) std::cout << "=";
  std::cout << std::endl;

  std::cout << "fRawDataLabel: " << fRawDataLabel << std::endl;
  std::cout << "fOutputDataLabel: " << fOutputDataLabel << std::endl;
  std::cout << "fDebug: ";
  if(fDebug) std::cout << "true" << std::endl;
  else std::cout << "false" << std::endl;

  for(int i=0;i<20;i++) std::cout << "=";
  std::cout << std::endl;    
}

void dune::TimingRawDecoder::setRootObjects(){
  art::ServiceHandle<art::TFileService> tFileService;

  fHTimestamp = tFileService->make<TH1I>("Timestamp","Timing Timestamp",  1e3, 0, 1e9);
  fHTimestamp->GetXaxis()->SetTitle("Timestamp (ms)");

  fHTimestampDelta = tFileService->make<TH1I>("TimestampDelta","Timing Timestamp Delta", 100, 0, 1e3);
  fHTimestampDelta->GetXaxis()->SetTitle("Delta Timestamp (ms)");

  fHTrigType = tFileService->make<TH1I>("TrigType","Timing trigger type", 10, 0, 10);
  fHTrigType->GetXaxis()->SetTitle("Trigger type");


}
void dune::TimingRawDecoder::beginJob(){
}

void dune::TimingRawDecoder::produce(art::Event & evt){
  //std::cout<<"-------------------- Timing RawDecoder -------------------"<<std::endl;
  // Implementation of required member function here.
  art::Handle<artdaq::Fragments> rawFragments;
  evt.getByLabel(fRawDataLabel, "TIMING", rawFragments);

  art::EventNumber_t eventNumber = evt.event();
  art::RunNumber_t runNumber = evt.run();

  std::vector<raw::RDTimeStamp> rdtimestamps;

  if(rawFragments.isValid()){

    ULong64_t evtTimestamp = 0;
    for(auto const& rawFrag : *rawFragments){
      dune::TimingFragment frag(rawFrag);
      //std::cout << "  Run " << runNumber << ", event " << eventNumber << ": ArtDaq Fragment Timestamp: "  << std::dec << rawFrag.timestamp() << std::endl;
      ULong64_t currentTimestamp=frag.get_tstamp();
      uint16_t scmd = (frag.get_scmd() & 0xFFFF);  // mask this just to make sure.  Though scmd only has four relevant bits, the method is declared uint32_t.
      rdtimestamps.emplace_back(currentTimestamp,scmd);

      fHTimestamp->Fill(currentTimestamp/1e6);
      fHTrigType->Fill(frag.get_scmd());

      if(fPrevTimestamp!=0) fHTimestampDelta->Fill((currentTimestamp-fPrevTimestamp)/1e6);

      fPrevTimestamp=currentTimestamp;
      //      fHTimestamp->Fill(rawFrag.timestamp());

      if ( fMakeEventTimeFile ) {
	if ( evtTimestamp == 0 ) {
	  evtTimestamp = rawFrag.timestamp();
	  string foutName = "artdaqTimestamp-Run" + std::to_string(runNumber);
	  int subrun = evt.subRun();
	  if ( subrun != 1 ) {
	    cout << "TimingRawDecoder::produce: WARNING: Unexpected subrun number: " << subrun << endl;
	    foutName += "-Sub" + std::to_string(subrun);
	  }
	  foutName += "-Event" + std::to_string(eventNumber) + ".dat";
	  ofstream fout(foutName);
	  fout << evtTimestamp << endl;
	} 
	else 
	  {
	    if ( rawFrag.timestamp() != evtTimestamp ) {
	      cout << "TimingRawDecoder::produce: WARNING: Fragments have inconsistent timestamps." << endl;
	    }
	  }
      }
    }
  }
  evt.put(std::make_unique<decltype(rdtimestamps)>(std::move(rdtimestamps)), fOutputDataLabel);
}

DEFINE_ART_MODULE(dune::TimingRawDecoder)
