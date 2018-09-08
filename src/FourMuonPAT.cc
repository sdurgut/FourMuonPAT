// -*- C++ -*-
//
// Package:    FourMuonPAT
// Class:      FourMuonPAT
// 

/**\class FourMuonPAT FourMuonPAT.cc myAnalyzers/FourMuonPAT/src/FourMuonPAT.cc
*/
//
// Original Author:  
//
//

// system include files
#include <memory>
#include "TLorentzVector.h"
// user include files
#include "../interface/FourMuonPAT.h"
#include "../interface/VertexReProducer.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Common/interface/TriggerNames.h"

#include "CommonTools/Statistics/interface/ChiSquaredProbability.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"

#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutRecord.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/L1GlobalTrigger/interface/L1GtFdlWord.h"
#include "DataFormats/Candidate/interface/VertexCompositeCandidate.h"
#include "DataFormats/Common/interface/RefToBase.h"
#include "DataFormats/Candidate/interface/ShallowCloneCandidate.h"
#include "DataFormats/Candidate/interface/CandMatchMap.h"
#include "DataFormats/Math/interface/Error.h"
#include "DataFormats/Math/interface/LorentzVector.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/Math/interface/Vector3D.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/PatCandidates/interface/GenericParticle.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/TrackReco/interface/DeDxData.h"
#include "DataFormats/Math/interface/Point3D.h"
#include "DataFormats/GeometryCommonDetAlgo/interface/DeepCopyPointer.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"
#include "DataFormats/CLHEP/interface/AlgebraicObjects.h"
#include "DataFormats/CLHEP/interface/Migration.h"
 
#include "RecoVertex/KinematicFitPrimitives/interface/MultiTrackKinematicConstraint.h"
#include "RecoVertex/KinematicFit/interface/KinematicConstrainedVertexFitter.h"
#include "RecoVertex/AdaptiveVertexFit/interface/AdaptiveVertexFitter.h"
#include "RecoVertex/KinematicFit/interface/TwoTrackMassKinematicConstraint.h"
#include "RecoVertex/VertexTools/interface/VertexDistanceXY.h"
//#include "RecoVertex/PrimaryVertexProducer/interface/PrimaryVertexProducerAlgorithm.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"

#include "MagneticField/Engine/interface/MagneticField.h"

//////////This is necessary for lumicalc///////
/*
#include "DataFormats/Luminosity/interface/LumiSummary.h"
#include "DataFormats/Luminosity/interface/LumiDetails.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "RecoLuminosity/LumiProducer/interface/LumiCorrectionParam.h"
#include "RecoLuminosity/LumiProducer/interface/LumiCorrectionParamRcd.h"
*/
#include "PhysicsTools/RecoUtils/interface/CheckHitPattern.h"
 
#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"

#include <vector>
#include <utility>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "DataFormats/MuonReco/interface/MuonSelectors.h"

#include "CLHEP/Matrix/Vector.h"
#include "CLHEP/Matrix/Matrix.h"
#include "CLHEP/Matrix/SymMatrix.h"


#include "TrackingTools/IPTools/interface/IPTools.h"

//added by zhenhu for MuonID
#include "../data/TMVAClassification_BDT.class.C"

//
// constants, enums and typedefs
//

typedef math::Error < 3 >::type CovarianceMatrix;
typedef ROOT::Math::SVector<double, 3> SVector3;
typedef ROOT::Math::SMatrix<double, 3, 3, ROOT::Math::MatRepSym<double, 3> > SMatrixSym3D;

// constructors and destructor
FourMuonPAT::FourMuonPAT(const edm::ParameterSet & iConfig)
:hlTriggerResults_(iConfig.getUntrackedParameter < edm::InputTag > ("HLTriggerResults",edm::InputTag("TriggerResults::HLT"))),
inputGEN_(iConfig.getUntrackedParameter < edm::InputTag > ("inputGEN", edm::InputTag("genParticles"))),
vtxSample(iConfig.getUntrackedParameter < std::string > ("VtxSample", std::string("offlinePrimaryVertices"))),
doMC(iConfig.getUntrackedParameter < bool > ("DoMonteCarloTree", false)),
MCParticle(iConfig.getUntrackedParameter < int >("MonteCarloParticleId", 20443)),   // 20443 X, 100443 Psi(2S), 9120443  // X from B
doJPsiMassCost(iConfig.getUntrackedParameter < bool > ("DoJPsiMassConstraint")),
MuPixHits_c(iConfig.getUntrackedParameter < int >("MinNumMuPixHits", 0)),
MuSiHits_c(iConfig.getUntrackedParameter < int >("MinNumMuSiHits", 0)),
MuNormChi_c(iConfig.getUntrackedParameter < double >("MaxMuNormChi2", 1000)),
MuD0_c(iConfig.getUntrackedParameter < double >("MaxMuD0", 1000)),
JMaxM_c(iConfig.getUntrackedParameter < double >("MaxJPsiMass", 4)),
JMinM_c(iConfig.getUntrackedParameter < double >("MinJPsiMass", 2.2)),
PiSiHits_c(iConfig.getUntrackedParameter < int >("MinNumTrSiHits", 0)),
PiPt_c(iConfig.getUntrackedParameter < double >("MinTrPt", 0)),
JPiPiDR_c(iConfig.getUntrackedParameter < double >("JPsiKKKMaxDR", 1)),
XPiPiDR_c(iConfig.getUntrackedParameter < double >("XCandPiPiMaxDR", 1.1)),
UseXDr_c(iConfig.getUntrackedParameter < bool > ("UseXDr", false)),
JPiPiMax_c(iConfig.getUntrackedParameter < double >("JPsiKKKMaxMass", 50)),
JPiPiMin_c(iConfig.getUntrackedParameter < double >("JPsiKKKMinMass", 0)),
resolveAmbiguity_(iConfig.getUntrackedParameter < bool > ("resolvePileUpAmbiguity", true)),
addXlessPrimaryVertex_(iConfig.getUntrackedParameter < bool > ("addXlessPrimaryVertex", true)),
TriggersForJpsi_(iConfig.getUntrackedParameter < std::vector < std::string > >("TriggersForJpsi")),
FiltersForJpsi_(iConfig.getUntrackedParameter < std::vector < std::string > >("FiltersForJpsi")),
TriggersForUpsilon_(iConfig.getUntrackedParameter < std::vector < std::string > >("TriggersForUpsilon")),
FiltersForUpsilon_(iConfig.getUntrackedParameter < std::vector < std::string > >("FiltersForUpsilon")),
Debug_(iConfig.getUntrackedParameter < bool > ("Debug_Output", false)),
Chi_Track_(iConfig.getUntrackedParameter < double >("Chi2NDF_Track", 10)),
X_One_Tree_(0),

runNum(0), evtNum(0), lumiNum(0),   
trigRes(0), trigNames(0), L1TT(0), MatchTriggerNames(0), 

priVtxX(0), priVtxY(0), priVtxZ(0), priVtxXE(0), priVtxYE(0), priVtxZE(0), priVtxChiNorm(0), priVtxChi(0), priVtxCL(0), 
PriVtxXCorrX(0), PriVtxXCorrY(0), PriVtxXCorrZ(0), 
PriVtxXCorrEX(0), PriVtxXCorrEY(0), PriVtxXCorrEZ(0), PriVtxXCorrC2(0), PriVtxXCorrCL(0), 

nMu(0), 
muPx(0), muPy(0), muPz(0), muD0(0), muD0E(0), muDz(0), muChi2(0), muGlChi2(0), mufHits(0),  
muFirstBarrel(0), muFirstEndCap(0), muDzVtx(0), muDxyVtx(0),  
muNDF(0), muGlNDF(0), muPhits(0), muShits(0), muGlMuHits(0), muType(0), muQual(0), 
muTrack(0), muCharge(0), muIsoratio(0), muIsGoodLooseMuon(0), muIsGoodLooseMuonNew(0), 
muIsGoodSoftMuonNewIlse(0), muIsGoodSoftMuonNewIlseMod(0), muIsGoodTightMuon(0), muIsJpsiTrigMatch(0), muIsUpsTrigMatch(0), munMatchedSeg(0), 
  //added by zhenhu for MuonID
muMVAMuonID(0), musegmentCompatibility(0), 
 
nMyFourMuon(0), MyFourMuonTrigMatch(0), MyFourMuonMass(0), MyFourMuonMassErr(0), MyFourMuonVtxCL(0), 
MyFourMuonVtxC2(0), MyFourMuonPx(0), MyFourMuonPy(0), MyFourMuonPz(0), MyFourMuonCTau(0), MyFourMuonCTauErr(0), 
  //////////New vars//////, triger match
 MyMu1TrigMatchVtxSD(0), MyMu2TrigMatchVtxSD(0), MyMu3TrigMatchVtxSD(0), MyMu4TrigMatchVtxSD(0),
 MyMu1TrigMatchL3FilterSD(0), MyMu2TrigMatchL3FilterSD(0), MyMu3TrigMatchL3FilterSD(0), MyMu4TrigMatchL3FilterSD(0),
////
My1234Mass(0), My1324Mass(0), My1423Mass(0), 
My1234JpsiDisXY(0), My1324JpsiDisXY(0), My1423JpsiDisXY(0), My1234JpsiDisZ(0), My1324JpsiDisZ(0), My1423JpsiDisZ(0), 
MyHitBeforeVrtx(0), MyMissAfterVrtx(0), MyJpsi1HitsBeforeMu12(0), MyJpsi1MissAfterMu12(0), MyJpsi2HitsBeforeMu34(0), 
MyJpsi2MissAfterMu34(0), MyJpsi3HitsBeforeMu13(0), MyJpsi3MissAfterMu13(0), MyJpsi4HitsBeforeMu24(0), MyJpsi4MissAfterMu24(0), 
MyJpsi5HitsBeforeMu14(0), MyJpsi5MissAfterMu14(0), MyJpsi6HitsBeforeMu23(0), MyJpsi6MissAfterMu23(0), MyFourMuonChg(0), 
MyJpsi1CTau_Mu12(0), MyJpsi1CTauErr_Mu12(0), MyJpsi1ChiProb_Mu12(0), MyJpsi2CTau_Mu34(0), MyJpsi2CTauErr_Mu34(0), 
MyJpsi2ChiProb_Mu34(0), MyJpsi3CTau_Mu13(0), MyJpsi3CTauErr_Mu13(0), MyJpsi3ChiProb_Mu13(0), MyJpsi4CTau_Mu24(0), 
MyJpsi4CTauErr_Mu24(0), MyJpsi4ChiProb_Mu24(0), MyJpsi5CTau_Mu14(0), MyJpsi5CTauErr_Mu14(0), MyJpsi5ChiProb_Mu14(0), 
MyJpsi6CTau_Mu23(0), MyJpsi6CTauErr_Mu23(0), MyJpsi6ChiProb_Mu23(0), 

MyJpsi1Mass_Mu12(0), MyJpsi2Mass_Mu34(0), MyJpsi3Mass_Mu13(0), MyJpsi4Mass_Mu24(0), MyJpsi5Mass_Mu14(0), MyJpsi6Mass_Mu23(0), 
MyJpsi1MassErr_Mu12(0), MyJpsi2MassErr_Mu34(0), MyJpsi3MassErr_Mu13(0), MyJpsi4MassErr_Mu24(0), MyJpsi5MassErr_Mu14(0), 
MyJpsi6MassErr_Mu23(0), 
  
//added by yik, for a mupmum pair only
nmumuonly(0), 
mumuonlyMass(0), mumuonlyMassErr(0), mumuonlyVtxCL(0), mumuonlyPx(0), mumuonlyPy(0), mumuonlyPz(0), 
mumuonlymu1Idx(0), mumuonlymu2Idx(0), mumuonlyctau(0), mumuonlyctauerr(0), mumuonlymuoverlapped(0), mumuonlyChg(0), 
mumuonlynsharedSeg(0), 
//added by yik end
  
  
mybxlumicorr(0), myrawbxlumi(0), 
  ////////////////////////  
MyFourMuonMu1Idx(0), MyFourMuonMu2Idx(0), MyFourMuonMu3Idx(0), MyFourMuonMu4Idx(0), 
MyFourMuonMu4Px(0), MyFourMuonMu4Py(0), MyFourMuonMu4Pz(0), MyFourMuonMu4fChi2(0), 
MyFourMuonMu3Px(0), MyFourMuonMu3Py(0), MyFourMuonMu3Pz(0), MyFourMuonMu3fChi2(0), 
MyFourMuonMu2Px(0), MyFourMuonMu2Py(0), MyFourMuonMu2Pz(0), MyFourMuonMu2fChi2(0), 
MyFourMuonMu1Px(0), MyFourMuonMu1Py(0), MyFourMuonMu1Pz(0), MyFourMuonMu1fChi2(0), 
MyFourMuonMu4fNDF(0), MyFourMuonMu3fNDF(0), MyFourMuonMu2fNDF(0), MyFourMuonMu1fNDF(0), 
MyFourMuonDecayVtxX(0), MyFourMuonDecayVtxY(0), MyFourMuonDecayVtxZ(0), 
MyFourMuonDecayVtxXE(0), MyFourMuonDecayVtxYE(0), MyFourMuonDecayVtxZE(0), 
MyFourMuoncosAlpha(0), MyFourMuonFLSig(0), MyFourMuonrVtxMag2D(0), MyFourMuonsigmaRvtxMag2D(0), 
MyFourMuonTrkIsoOverFourMuSumpt(0), MyFourMuonTrkIsoOverFourMuVectpt(0), 
MyFourMuonOverlap12(0), MyFourMuonOverlap13(0), MyFourMuonOverlap14(0), MyFourMuonOverlap23(0), MyFourMuonOverlap24(0), 
MyFourMuonOverlap34(0), MyFourMuonSharedSeg12(0), MyFourMuonSharedSeg13(0), MyFourMuonSharedSeg14(0), 
 MyFourMuonSharedSeg23(0), MyFourMuonSharedSeg24(0), MyFourMuonSharedSeg34(0),
 //MC
nxMC(0),MC_xPdgId(0),
MC_YmuPx(0), MC_YmuPy(0), MC_YmuPz(0), MC_YmuM(0), MC_YmuChg(0), MC_YmuPdgId(0),
MC_HmuPx(0), MC_HmuPy(0), MC_HmuPz(0), MC_HmuM(0), MC_HmuChg(0), MC_HmuPdgId(0),
MC_xPx(0), MC_xPy(0), MC_xPz(0), MC_xM(0), MC_xChg(0)
 //MC
{
}


FourMuonPAT::~FourMuonPAT() {
    // do anything here that needs to be done at desctruction time
    // (e.g. close files, deallocate resources etc.)

}
// member functions

//    ofstream myfile("comparison.txt");
// ------------ method called to for each event  ------------
void FourMuonPAT::analyze(const edm::Event & iEvent, const edm::EventSetup & iSetup) {

  
  if(doMC)
    {
      Handle < GenParticleCollection > genParticles;
      iEvent.getByLabel(inputGEN_, genParticles);	  
      //nxMC = 0;
      for(GenParticleCollection::const_iterator mygen = genParticles->begin(); mygen !=genParticles->end(); mygen++)
	{
	  if(fabs(mygen->pdgId()) == 35 && mygen->numberOfDaughters()>2 )
	    {
	      //cout<<"number of daughters="<<mygen->numberOfDaughters()<<endl;
	      MC_xPdgId->push_back(mygen->pdgId());
	      MC_xPx->push_back(mygen->px()); 
	      MC_xPy->push_back(mygen->py()); 
	      MC_xPz->push_back(mygen->pz());
	      MC_xM->push_back(mygen->mass());
	      MC_xChg->push_back(mygen->charge());
	      ++nxMC;	
	    }
	  if(fabs(mygen->pdgId()) == 13 && fabs(mygen->mother()->pdgId()) == 553  )
	    {
	      MC_YmuPx->push_back(mygen->px()); 
	      MC_YmuPy->push_back(mygen->py()); 
	      MC_YmuPz->push_back(mygen->pz());
	      MC_YmuM->push_back(mygen->mass());
	      MC_YmuChg->push_back(mygen->charge());
	      MC_YmuPdgId->push_back(mygen->pdgId());		      
	    }
	  
	  if(fabs(mygen->pdgId()) == 13 && fabs(mygen->mother()->pdgId()) == 35 )
	    {
	      MC_HmuPx->push_back(mygen->px()); 
	      MC_HmuPy->push_back(mygen->py()); 
	      MC_HmuPz->push_back(mygen->pz());
	      MC_HmuM->push_back(mygen->mass());
	      MC_HmuChg->push_back(mygen->charge());
	      MC_HmuPdgId->push_back(mygen->pdgId());		      
	    }		  
	}  //end loop for genParticles
    }
  //if doMC 
  

    // get event content information	
    using std::vector;
    using namespace edm;
    using namespace reco;
    using namespace std;

    runNum = iEvent.id().run();
    evtNum = iEvent.id().event();
    lumiNum = iEvent.id().luminosityBlock();

    ESHandle < MagneticField > bFieldHandle;
    iSetup.get < IdealMagneticFieldRecord > ().get(bFieldHandle);

    // first get HLT results
    edm::Handle < edm::TriggerResults > hltresults;
    try {
        iEvent.getByLabel(hlTriggerResults_, hltresults);
    }
    catch(...) {
        cout << "Couldn't get handle on HLT Trigger!" << endl;
    }
    if (!hltresults.isValid()) {
        cout << "No Trigger Results!" << endl;
    } else {
        int ntrigs = hltresults->size();
        if (ntrigs == 0) {
            cout << "No trigger name given in TriggerResults of the input " << endl;
        }
        
	// get hold of trigger names - based on TriggerResults object!
        edm::TriggerNames triggerNames_;
        triggerNames_ = iEvent.triggerNames(*hltresults);

	int nUpstrigger = TriggersForUpsilon_.size();
        int nJpsitrigger = TriggersForJpsi_.size();

        for (int JpsiTrig = 0; JpsiTrig < nJpsitrigger; JpsiTrig++) {
            JpsiMatchTrig[JpsiTrig] = 0;
        }//jpsi trigger

        for (int UpsTrig = 0; UpsTrig < nUpstrigger; UpsTrig++) {
            UpsilonMatchTrig[UpsTrig] = 0;
        }//upsilon trig




        for (int itrig = 0; itrig < ntrigs; itrig++) {
            string trigName = triggerNames_.triggerName(itrig);
            int hltflag = (*hltresults)[itrig].accept();
            // cout << trigName << " " <<hltflag <<endl;
            trigRes->push_back(hltflag);
            trigNames->push_back(trigName);

	for (unsigned int JpsiTrig = 0; JpsiTrig < TriggersForJpsi_.size(); JpsiTrig++) {
                    if (TriggersForJpsi_[JpsiTrig] == triggerNames_.triggerName(itrig)) {
                            JpsiMatchTrig[JpsiTrig] = hltflag;
                            break;
                        }

                }//Jpsi Trigger

	for (unsigned int UpsTrig = 0; UpsTrig < TriggersForUpsilon_.size(); UpsTrig++) {
                    if (TriggersForUpsilon_[UpsTrig] == triggerNames_.triggerName(itrig)) {
                            UpsilonMatchTrig[UpsTrig] = hltflag;
                            break;
                        }
                }//Upsilon Trigger


	        }

        for (int MatchTrig = 0; MatchTrig < nJpsitrigger; MatchTrig++) {
            MatchTriggerNames->push_back(TriggersForJpsi_[MatchTrig]);

        }



    } // end of HLT trigger info


    // get L1 trigger info
    edm::ESHandle < L1GtTriggerMenu > menuRcd;
    iSetup.get < L1GtTriggerMenuRcd > ().get(menuRcd);

    edm::ESHandle<TransientTrackBuilder> theB;
    iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder",theB);

    edm::Handle < L1GlobalTriggerReadoutRecord > gtRecord;
    iEvent.getByLabel(edm::InputTag("gtDigis"), gtRecord);
    const DecisionWord dWord = gtRecord->decisionWord();

    const TechnicalTriggerWord ttWord = gtRecord->technicalTriggerWord();
    for (unsigned int l1i = 0; l1i != ttWord.size(); ++l1i) {
        L1TT->push_back(ttWord.at(l1i));
    }

    
    /*
	////////////////////////////////////////////////////////////////////////////
	/////////////// Keep following information for further use//////////////////
	/////////////// It has delivered and recorded lumi information//////////////
	////////////////////////////////////////////////////////////////////////////
	edm::Handle<LumiSummary> lumisummary;   //get the raw lumi data from edm::LumiSummary
        iEvent.getLuminosityBlock().getByLabel("lumiProducer", lumisummary);
    //     float instlumi=lumisummary->avgInsDelLumi(); //Delivered Lumi
     //    float correctedinstlumi=instlumi;
      //   float recinstlumi=lumisummary->avgInsRecLumi(); //Recorded Lumi
         float corrfac2=1.;
	myrawbxlumi =0; mybxlumicorr=0;
	// Get instantaneous Lumi
           edm::Handle<LumiDetails> d;
           iEvent.getLuminosityBlock().getByLabel("lumiProducer",d);

	//cout<<"Lumi is="<<d->isValid()<<endl;

	if(d->isValid()){
        int bx=iEvent.bunchCrossing();
        myrawbxlumi=d->lumiValue(LumiDetails::kOCC1,bx);

	//cout<<"Banch Crossing="<<bx<<endl;
	// get LumiCorrectionParam object from event setup
         edm::ESHandle<LumiCorrectionParam> datahandle; 
         iSetup.get<LumiCorrectionParamRcd>().get(datahandle);

         if(datahandle.isValid()){
           const LumiCorrectionParam* mydata=datahandle.product();
           //std::cout<<"correctionparams "<<*mydata<<std::endl; 
  //         corrfac=mydata->getCorrection(instlumi);//get lumi correction factor. Note: the corrfac is dependent of the average inst lumi value of the LS
	   corrfac2=mydata->getCorrection(myrawbxlumi);
         }else{
           std::cout<<"no valid record found"<<std::endl;
         }

        mybxlumicorr=myrawbxlumi*corrfac2; // unit in Hz/nb
    
//         cout<<"This corrfac1="<<corrfac<<"; and corrfac2="<<corrfac2<<endl;
//         cout<<"This is lumi="<<myrawbx<<endl;
// 	cout<<"This is corrected Recorded lumi="<<correctedinstRecLumi<<endl; // units are in Hz/nb
// 	cout<<"This is corrected Delivered lumi="<<correctedinstlumi<<endl;
// 	cout<<"This is corrected Inst lumi="<<mybxlumicorr<<endl;		
	} //check if lumidetail is valid
    */
	///////////////////////////////////////////////////////////////////////////////////
	////////////////////// End Lumi Calculation////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////


    Vertex thePrimaryV;
    Vertex theRecoVtx;
    Vertex theBeamSpotV;
    BeamSpot beamSpot;
    math::XYZPoint RefVtx;

    // get BeamSplot
    edm::Handle < reco::BeamSpot > beamSpotHandle;
    iEvent.getByLabel("offlineBeamSpot", beamSpotHandle);
    if (beamSpotHandle.isValid()) {
        beamSpot = *beamSpotHandle;
        theBeamSpotV = Vertex(beamSpot.position(), beamSpot.covariance3D());
    } else cout << "No beam spot available from EventSetup" << endl;


    Handle < VertexCollection > recVtxs;
    iEvent.getByLabel(vtxSample, recVtxs);
    unsigned int nVtxTrks = 0;


///////////////////////////////////////////////////////////////////////
////////////////Check Lines below for Primary Vertex///////////////////
///////////////////////////////////////////////////////////////////////
 /*
 int nGoodPrimVtx=0;
  int mygoodprimvtxz[100];
  for(unsigned myi=0;myi<recVtxs->size();myi++) {
    if( (*recVtxs)[myi].ndof()>=5
        && fabs((*recVtxs)[myi].z())<=24
        && fabs( (*recVtxs)[myi].position().rho())<=2.0
        ) {
      mygoodprimvtxz[nGoodPrimVtx]=(*recVtxs)[myi].z();
      nGoodPrimVtx++;

	//std::cout<<"Vertex Z ="<<(*recVtxs)[myi].z()<<std::endl;
    }//if( (*vertices_h)[myi].ndof()>=5
  }
  */
  //	std::cout<<"N Good Vertex="<<nGoodPrimVtx<<std::endl;    
//	std::cout<<"N Good Vertex2="<<recVtxs->size()<<std::endl; 
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

    if (recVtxs->begin() != recVtxs->end()) {
	if (addXlessPrimaryVertex_ || resolveAmbiguity_) {
            thePrimaryV = Vertex(*(recVtxs->begin()));
        } else {
            for (reco::VertexCollection::const_iterator vtx = recVtxs->begin(); vtx != recVtxs->end(); ++vtx) {
		if (nVtxTrks < vtx->tracksSize()) {
                    nVtxTrks = vtx->tracksSize();
                    thePrimaryV = Vertex(*vtx);
                }
            }
        }
    } else {
        thePrimaryV = Vertex(beamSpot.position(), beamSpot.covariance3D());
    }
    
    RefVtx = thePrimaryV.position();
    priVtxX = (thePrimaryV.position().x());
    priVtxY = (thePrimaryV.position().y());
    priVtxZ = (thePrimaryV.position().z());
    priVtxXE = (thePrimaryV.xError());
    priVtxYE = (thePrimaryV.yError());
    priVtxZE = (thePrimaryV.zError());
    priVtxChiNorm = (thePrimaryV.normalizedChi2());
    priVtxChi = thePrimaryV.chi2();
    priVtxCL = ChiSquaredProbability((double) (thePrimaryV.chi2()), (double) (thePrimaryV.ndof()));
    
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // try reconstruction without fitting modules
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Handle < vector < pat::GenericParticle > >thePATTrackHandle;
    iEvent.getByLabel("cleanPatTrackCands", thePATTrackHandle);

    Handle < vector < pat::Muon > >thePATMuonHandle;
    iEvent.getByLabel("patMuonsWithTrigger", thePATMuonHandle);
     
    if (Debug_) {
        cout << "starting event with " << thePATTrackHandle->size() << " tracks, and " << thePATMuonHandle->
          size() << " muons" << endl;
    }


    if ( thePATMuonHandle->size()<2 ) return;

    if ( thePATMuonHandle->size()>=2 ) {

      //added by zhenhu for MuonID
      vector<std::string> theInputVariables;
      theInputVariables.push_back("validFrac");
      theInputVariables.push_back("globalChi2");
      theInputVariables.push_back("pt");
      theInputVariables.push_back("eta");
      theInputVariables.push_back("segComp");
      theInputVariables.push_back("chi2LocMom");
      theInputVariables.push_back("chi2LocPos");
      theInputVariables.push_back("glbTrackProb");
      theInputVariables.push_back("NTrkVHits");
      theInputVariables.push_back("NTrkEHitsOut");
      ReadBDT muonID(theInputVariables);
      vector<double> inputValues;
      inputValues.resize(10, 0.);

      //fill muon track block
      for (std::vector < pat::Muon >::const_iterator iMuonP = thePATMuonHandle->begin();
	   iMuonP != thePATMuonHandle->end(); ++iMuonP) {
	// push back all muon information
	++nMu;
	const reco::Muon * rmu = dynamic_cast < const reco::Muon * >(iMuonP->originalObject());
	muPx->push_back(rmu->px());
	muPy->push_back(rmu->py());
	muPz->push_back(rmu->pz());
	muCharge->push_back(rmu->charge());
       
        int goodSoftMuonNewIlseMod=0;
        int goodSoftMuonNewIlse=0;
        int goodLooseMuonNew=0;
        int goodLooseMuon=0;
        int goodTightMuon=0;

	float mymuMVABs=-1;

	bool JpsiTrigger = false;
        for (unsigned int JpsiTrig = 0; JpsiTrig < TriggersForJpsi_.size(); JpsiTrig++) {
                if (JpsiMatchTrig[JpsiTrig] != 0) {
                const pat::TriggerObjectStandAloneCollection muJpsiHLTMatches =
                  iMuonP->triggerObjectMatchesByFilter(FiltersForJpsi_[JpsiTrig]);
                bool pass1 = muJpsiHLTMatches.size() > 0;
                if(pass1) JpsiTrigger = true;
        //      std::cout<<pass1<<"; Jpsi"<<endl;
                }
        }

        muIsJpsiTrigMatch->push_back(JpsiTrigger);

        bool UpsTrigger = false;
        for (unsigned int UpsTrig = 0; UpsTrig < TriggersForUpsilon_.size(); UpsTrig++) {
                if (UpsilonMatchTrig[UpsTrig] != 0) {
                const pat::TriggerObjectStandAloneCollection muUpsHLTMatches =
                  iMuonP->triggerObjectMatchesByFilter(FiltersForUpsilon_[UpsTrig]);
                bool pass1 = muUpsHLTMatches.size() > 0;
                if(pass1) UpsTrigger = true;
        //      std::cout<<pass1<<"; Ups"<<endl;
                }
        }

        muIsUpsTrigMatch->push_back(UpsTrigger);

	/*
	int munMatchedSegtmp=0;
	for(std::vector<reco::MuonChamberMatch>::const_iterator chamberMatch = rmu->matches().begin();
	    chamberMatch != rmu->matches().end(); ++chamberMatch) {
	  if (chamberMatch->segmentMatches.empty()) continue;       
	  munMatchedSegtmp+=chamberMatch->segmentMatches.size();
	}
	cout<<"munMatchedSegtmp="<<munMatchedSegtmp<<", another nseg="<<rmu->numberOfMatches(reco::Muon::SegmentArbitration)<<endl;
	//munMatchedSeg->push_back( munMatchedSegtmp );
	*/
	munMatchedSeg->push_back( rmu->numberOfMatches(reco::Muon::SegmentArbitration) );
	//munMatchedSeg->push_back( -1);


	if (rmu->track().isNull()) {    // rmu->track() returns innerTrack();
	  // cout << "no track for " << std::distance(thePATMuonHandle->begin(), iMuonP) << " filling defaults"
	  // << endl;
	  muD0->push_back(0);
	  muD0E->push_back(0);
	  muDz->push_back(0);
	  muChi2->push_back(0);
	  muNDF->push_back(-1);
	  muPhits->push_back(0);
	  muShits->push_back(0);
	  
	  muDzVtx->push_back(0);
	  muDxyVtx->push_back(0);
	  mufHits->push_back(0);
	  muFirstBarrel->push_back(0);
	  muFirstEndCap->push_back(0);
	  	  
	} else {

	//doing muon isolation here
	//find the closest primary vertex first
	  double  minDist = 99999.;
	  reco::Vertex closestPV;
	  for (reco::VertexCollection::const_iterator iVtx = recVtxs->begin(); iVtx != recVtxs->end(); ++iVtx) {
	    reco::TransientTrack myTranMutrk (rmu->track(),  &(*bFieldHandle) );
	    pair<bool,Measurement1D> the3DIpPairX = IPTools::absoluteImpactParameter3D(myTranMutrk, Vertex(*iVtx));
	    if( iVtx->ndof()>=5 && fabs(iVtx->z())<=24
		&& fabs(iVtx->position().rho())<=2.0
		&& the3DIpPairX.first && minDist > the3DIpPairX.second.value()){
	      minDist = the3DIpPairX.second.value();
	      //cout<<"minDist="<<minDist<<endl;
	      closestPV = Vertex(*iVtx);
	    }//if( iVtx->ndof()>=5 && fabs(iVtx->z())<=24
	  }//for (reco::VertexCollection::const_iterator iVtx 
	  //	  
	  float myextratrkptsum=0.0;
	  float myextratrkptsumratio=0.0;
	  for(reco::Vertex::trackRef_iterator tv = closestPV.tracks_begin(); tv!=closestPV.tracks_end(); tv++){
	    const reco::Track & iextraTrk=*(tv->get());
	    reco::TrackBase::TrackQuality myquality=reco::TrackBase::qualityByName("highPurity");  //"loose","tight","highPurity"
	    // cout<<"highPurity trck="<<iextraTrk.quality(myquality)<<",pt"<<iextraTrk.pt()<<endl;
	    math::XYZTLorentzVectorD mymup4, mytrkp4;
	    mymup4.SetXYZT(rmu->track()->px(),rmu->track()->py(),rmu->track()->pz(),0.1057);
	    mytrkp4.SetXYZT(iextraTrk.px(),iextraTrk.py(),iextraTrk.pz(),0.13957);
	    if(iextraTrk.pt() > 0.9
	       && iextraTrk.quality(myquality)
	       && tv->key() != rmu->track().key()
	       && deltaR( mymup4.eta(), mymup4.phi(), mytrkp4.eta(), mytrkp4.phi() ) <0.3
	       ){
	      //cout<<"delta R="<<deltaR( mymup4.eta(), mymup4.phi(), mytrkp4.eta(), mytrkp4.phi() )<<endl;
	      //check if this track is a muon
	      bool thetrkismuon=false;
	      for (std::vector < pat::Muon >::const_iterator mymuon = thePATMuonHandle->begin();
		   mymuon != thePATMuonHandle->end(); ++mymuon) {				
		const reco::Muon * mymuobj = dynamic_cast < const reco::Muon * >(mymuon->originalObject());
		if( (!(mymuobj->track().isNull()))  
		    && mymuobj->track().key()==tv->key()  
		    //loose muon id
		    && muon::isGoodMuon(*mymuobj, muon::SelectionType(11))
		    && mymuobj->track()->hitPattern().trackerLayersWithMeasurement()>5
		    && mymuobj->innerTrack()->hitPattern().pixelLayersWithMeasurement()>1
		    && mymuobj->innerTrack()->normalizedChi2() <1.8
		    && fabs(mymuobj->innerTrack()->dxy(RefVtx))<3.0
		    && fabs(mymuobj->innerTrack()->dz(RefVtx)) <30.0
		    //loose muon id end
		    )  
		  {thetrkismuon=true; }
	      }//for (std::vector < pat::Muon >::const_iterator mymuon = thePATMuonHandle->begin();
	      if( !thetrkismuon ) myextratrkptsum+=iextraTrk.pt();
	    }//if(iextraTrk.pt() > 0.9 ){
	  }// for(reco::Vertex::trackRef_iterator tv
	  myextratrkptsumratio=myextratrkptsum/rmu->track()->pt();
	  //cout<<"myextratrkptsum="<<myextratrkptsum<<", mu pt="<<rmu->track()->pt()<<", myextratrkptsumratio="<<myextratrkptsumratio<<endl;
	  muIsoratio->push_back( myextratrkptsumratio );
	//end doing muon isolation here



/*
cout<<"number of tracker layers="<<rmu->track()->hitPattern().trackerLayersWithMeasurement()<<endl;
cout<<"number of tracker pixel layers="<<rmu->innerTrack()->hitPattern().pixelLayersWithMeasurement()<<endl;
cout<<"normalizedChi2()="<<rmu->innerTrack()->normalizedChi2() <<endl;
cout<<"dxy="<<fabs(rmu->innerTrack()->dxy(RefVtx))<<endl; 
cout<<"dz="<<fabs(rmu->innerTrack()->dz(RefVtx)) <<endl;
*/

if(muon::isGoodMuon(*rmu, muon::SelectionType(11))
&& rmu->track()->hitPattern().trackerLayersWithMeasurement()>5
&& rmu->innerTrack()->hitPattern().pixelLayersWithMeasurement()>1
&& rmu->innerTrack()->normalizedChi2() <1.8
&& fabs(rmu->innerTrack()->dxy(RefVtx))<3.0
&& fabs(rmu->innerTrack()->dz(RefVtx)) <30.0
)
goodLooseMuon=1;
//cout<<"goodLooseMuon="<<goodLooseMuon<<endl<<endl;
if(muon::isGoodMuon(*rmu, muon::SelectionType(12))
&& rmu->track()->hitPattern().trackerLayersWithMeasurement()>5
&& rmu->innerTrack()->hitPattern().pixelLayersWithMeasurement()>1
&& rmu->innerTrack()->normalizedChi2() <1.8
&& fabs(rmu->innerTrack()->dxy(RefVtx))<3.0
&& fabs(rmu->innerTrack()->dz(RefVtx)) <30.0
)
goodLooseMuonNew=1;
//cout<<"goodLooseMuonNew="<<goodLooseMuonNew<<endl<<endl;

 
if(muon::isGoodMuon(*rmu, muon::SelectionType(12))
    && rmu->track()->hitPattern().trackerLayersWithMeasurement()>5
    && rmu->innerTrack()->hitPattern().pixelLayersWithMeasurement()>0
    && rmu->innerTrack()->quality(reco::TrackBase::highPurity)
    //    && rmu->innerTrack()->normalizedChi2() <1.8
    && fabs(rmu->innerTrack()->dxy(RefVtx))<0.3
    && fabs(rmu->innerTrack()->dz(RefVtx)) <20.0
    )
   goodSoftMuonNewIlse=1;
 //cout<<"goodSoftMuonNewIlse="<<goodSoftMuonNewIlse<<endl<<endl;


if(muon::isGoodMuon(*rmu, muon::SelectionType(11))
    && rmu->track()->hitPattern().trackerLayersWithMeasurement()>5
    && rmu->innerTrack()->hitPattern().pixelLayersWithMeasurement()>0
    && rmu->innerTrack()->quality(reco::TrackBase::highPurity)
    //    && rmu->innerTrack()->normalizedChi2() <1.8
    && fabs(rmu->innerTrack()->dxy(RefVtx))<0.3
    && fabs(rmu->innerTrack()->dz(RefVtx)) <20.0
    )
   goodSoftMuonNewIlseMod=1;
// cout<<"goodSoftMuonNewIlseMod="<<goodSoftMuonNewIlseMod<<endl<<endl;


 
if(rmu->isGlobalMuon() ){
/*
cout<<"Global muon="<<rmu->isGlobalMuon()<<endl;
cout<<"PF muon="<<rmu->isPFMuon()<<endl;
cout<<"global track normalizedChi2()="<<rmu->globalTrack()->normalizedChi2() <<endl;
cout<<"muon hits="<<rmu->globalTrack()->hitPattern().numberOfValidMuonHits()<<endl;
cout<<"matched station="<<rmu->numberOfMatchedStations()<<endl;
cout<<"best track dxy="<<fabs(rmu->muonBestTrack()->dxy( RefVtx ))<<endl;
cout<<"best track dz="<<fabs(rmu->muonBestTrack()->dz( RefVtx ))<<endl;
cout<<"number of tracker pixel hits="<<rmu->innerTrack()->hitPattern().numberOfValidPixelHits()<<endl;
*/
if( rmu->isGlobalMuon()
&& rmu->isPFMuon() 
&&rmu->globalTrack()->normalizedChi2() <10.0
&&rmu->globalTrack()->hitPattern().numberOfValidMuonHits()>0
&&rmu->numberOfMatchedStations()>1
&& fabs(rmu->muonBestTrack()->dxy( RefVtx ))<0.2
&& fabs(rmu->muonBestTrack()->dz( RefVtx )) < 0.5
&& rmu->innerTrack()->hitPattern().numberOfValidPixelHits()>0
&& rmu->track()->hitPattern().trackerLayersWithMeasurement()>5
)
goodTightMuon=1;
}
//cout<<"goodTightMuon="<<goodTightMuon<<endl<<endl;

	  muD0->push_back(rmu->track()->d0());
	  muD0E->push_back(rmu->track()->d0Error());
	  muDz->push_back(rmu->track()->dz());
	  muChi2->push_back(rmu->track()->chi2());
	  muNDF->push_back(rmu->track()->ndof());
	  muPhits->push_back(rmu->track()->hitPattern().numberOfValidPixelHits());
	  muShits->push_back(rmu->track()->hitPattern().numberOfValidStripHits());
	  
	  muDzVtx->push_back(rmu->track()->dz(RefVtx));
	  muDxyVtx->push_back(rmu->track()->dxy(RefVtx));
	  
	  mufHits->push_back((1.0 * rmu->track()->found()) / (rmu->track()->found() + rmu->track()->lost() +
							      rmu->track()->trackerExpectedHitsInner().numberOfHits() +
							      rmu->track()->trackerExpectedHitsOuter().numberOfHits()));
	  muFirstBarrel->push_back(rmu->track()->hitPattern().hasValidHitInFirstPixelBarrel());
	  muFirstEndCap->push_back(rmu->track()->hitPattern().hasValidHitInFirstPixelEndcap());		
	}
	
	if (rmu->globalTrack().isNull()) {  // rmu->globalTrack() returns globalTrack();
	  // cout << "no track for " << std::distance(thePATMuonHandle->begin(), iMuonP) << " filling defaults"
	  // << endl;
	  muGlChi2->push_back(0);
	  muGlNDF->push_back(-1);
	  muGlMuHits->push_back(0);
	} else {
	  muGlChi2->push_back(rmu->globalTrack()->chi2());
	  muGlNDF->push_back(rmu->globalTrack()->ndof());
	  muGlMuHits->push_back(rmu->globalTrack()->hitPattern().numberOfValidMuonHits());
	}
	muType->push_back(rmu->type());
	int qm = 0;
	for (int qi = 1; qi != 24; ++qi) {
	  if (muon::isGoodMuon(*rmu, muon::SelectionType(qi))) {
	    qm += 1 << qi;
	  }
	}
	muQual->push_back(qm);
	muTrack->push_back(-1); // not implemented yet

	muIsGoodSoftMuonNewIlseMod->push_back( goodSoftMuonNewIlseMod );
	muIsGoodSoftMuonNewIlse->push_back( goodSoftMuonNewIlse );
        muIsGoodLooseMuonNew->push_back( goodLooseMuonNew );
	muIsGoodLooseMuon->push_back( goodLooseMuon );
	muIsGoodTightMuon->push_back( goodTightMuon );
	
	//added by zhenhu for MuonID
	if (!rmu->globalTrack().isNull() && !rmu->track().isNull()) {
	  inputValues[0] = rmu->track()->validFraction();
	  inputValues[1] = rmu->globalTrack()->normalizedChi2();
	  inputValues[2] = sqrt(rmu->px()*rmu->px()+rmu->py()*rmu->py());
	  inputValues[3] = rmu->eta();
	  inputValues[4] = muon::segmentCompatibility(*rmu);
	  inputValues[5] = rmu->combinedQuality().chi2LocalMomentum;
	  inputValues[6] = rmu->combinedQuality().chi2LocalPosition;
	  inputValues[7] = rmu->combinedQuality().glbTrackProbability;
	  inputValues[8] = rmu->track()->hitPattern().numberOfValidTrackerHits();
	  inputValues[9] = rmu->track()->trackerExpectedHitsOuter().numberOfLostTrackerHits();
	  mymuMVABs=muonID.GetMvaValue( inputValues );
	  //std::cout<<muonID.GetMvaValue( inputValues )<<",segment compatibility="<<muon::segmentCompatibility(*rmu)<<std::endl;
	  //muMVAMuonID->push_back( mymuMVABs );
	}
	//cout<<"segment compatibility="<<muon::segmentCompatibility(*rmu)<<endl;
	muMVAMuonID->push_back( mymuMVABs );	
	musegmentCompatibility->push_back( muon::segmentCompatibility(*rmu) );
	//added for muon id studies

      }
    }

    //fill muon track block end
      

    
    //get the four moun fit, but first also fit dimuon
    if ( thePATTrackHandle->size()>=2 && thePATMuonHandle->size()>=2 ) {
        // get X and MyFourMuon cands
        for (std::vector < pat::Muon >::const_iterator iMuon1 = thePATMuonHandle->begin();
          iMuon1 != thePATMuonHandle->end(); ++iMuon1) {

	  const reco::Muon * rmu1 = dynamic_cast < const reco::Muon * >(iMuon1->originalObject());
	  // check for mu1
	  // if (iMuon1->charge() != 1) continue;
	  TrackRef muTrack1 = iMuon1->track();
	  if (muTrack1.isNull()) {
	    // cout << "continue due to no track ref" << endl;
	    continue;
	  }
	  if (rmu1->track()->hitPattern().numberOfValidPixelHits() < MuPixHits_c
              || rmu1->track()->hitPattern().numberOfValidStripHits() < MuSiHits_c
              || rmu1->track()->chi2() / rmu1->track()->ndof() > MuNormChi_c
              || fabs(rmu1->track()->dxy(RefVtx)) > MuD0_c) {
	    continue;
	  }

	  reco::Track recoMu1 = *iMuon1->track();
	  // next check for mu2
	  for (std::vector < pat::Muon >::const_iterator iMuon2 = iMuon1 + 1;
	       iMuon2 != thePATMuonHandle->end(); ++iMuon2) {
	    // if (iMuon2->charge() != -1) continue;
	    //if (iMuon2->charge() * iMuon1->charge() > 0) continue;
	    TrackRef muTrack2 = iMuon2->track();
	    if (muTrack2.isNull()) {
	      // cout << "continue from no track ref" << endl;
	      continue;
	    }
	    const reco::Muon * rmu2 = dynamic_cast < const reco::Muon * >(iMuon2->originalObject());
	    /*
	    if (muon::overlap(*rmu1, *rmu2)) {
	       cout << "continued from overlapped muons" << endl;
	      continue;
	    }
	    */
	    if (rmu2->track()->hitPattern().numberOfValidPixelHits() < MuPixHits_c
		|| rmu2->track()->hitPattern().numberOfValidStripHits() < MuSiHits_c
		|| rmu2->track()->chi2() / rmu2->track()->ndof() > MuNormChi_c
		|| fabs(rmu2->track()->dxy(RefVtx)) > MuD0_c) {
	      continue;
	    }

	    /*
	    int mynseg1=0, mynseg2=0;
     for(std::vector<reco::MuonChamberMatch>::const_iterator chamberMatch = rmu1->matches().begin();
         chamberMatch != rmu1->matches().end(); ++chamberMatch) {
       if (chamberMatch->segmentMatches.empty()) continue;       
       mynseg1+=chamberMatch->segmentMatches.size();
     }     
	    cout<<"rmu1 nseg="<< mynseg1<<", rmu2 nseg="<< rmu2->matches()[0].segmentMatches.size()<<", nshare="<<muon::sharedSegments(*rmu1, *rmu2)<<endl; 
	    */
	    mumuonlynsharedSeg->push_back(  muon::sharedSegments(*rmu1, *rmu2) );

	   reco::Track recoMu2 = *iMuon2->track();	   
	   //added by yik for mumuonly block
	   //requested by ARC to remove charge requirement
	   //if ( (iMuon1->charge()+iMuon2->charge() ) == 0 ) 
	     {
	     // Get The mumu information 
	     TransientTrack muonPTT(muTrack1, &(*bFieldHandle));
	     TransientTrack muonMTT(muTrack2, &(*bFieldHandle));	    
	     KinematicParticleFactoryFromTransientTrack pmumuFactory;
	     // The mass of a muon and the insignificant mass sigma 
	     // to avoid singularities in the covariance matrix.
	     ParticleMass muon_mass = 0.10565837;    // pdg mass
	     float muon_sigma = muon_mass * 1.e-6;
	     // initial chi2 and ndf before kinematic fits.
	     float chi = 0.;
	     float ndf = 0.;
	     vector < RefCountedKinematicParticle > muonParticles;
	     muonParticles.push_back(pmumuFactory.particle(muonPTT, muon_mass, chi, ndf, muon_sigma));
	     muonParticles.push_back(pmumuFactory.particle(muonMTT, muon_mass, chi, ndf, muon_sigma));
	     KinematicParticleVertexFitter fitter;
	     RefCountedKinematicTree psiVertexFitTree;
	     psiVertexFitTree = fitter.fit(muonParticles);

	     float mymumuonlyctau=-999;
	     float mymumuonlyctauerr=-999;
	     float mydimumass=-99.0;
	     float mydimumasserr=-99.0;
	     float mychisqprob=-9.0;
	     float mypx=-9999;
	     float mypy=-9999;
	     float mypz=-9999;

	     if (psiVertexFitTree->isValid()) {   
	       psiVertexFitTree->movePointerToTheTop();	    
	       RefCountedKinematicParticle psi_vFit_noMC = psiVertexFitTree->currentParticle();		
	       RefCountedKinematicVertex psi_vFit_vertex_noMC = psiVertexFitTree->currentDecayVertex();
	       KinematicParameters mymumupara=  psi_vFit_noMC->currentState().kinematicParameters();
	       //cout<<"mymumupara px="<<mymumupara.momentum().x()<<",py="<<mymumupara.momentum().y()<<", m="<<psi_vFit_noMC->currentState().mass()<<endl;	      
	       mymumuonlyctau=GetcTau(psi_vFit_vertex_noMC,psi_vFit_noMC,theBeamSpotV);
	       mymumuonlyctauerr=GetcTauErr(psi_vFit_vertex_noMC,psi_vFit_noMC,theBeamSpotV);
	       mydimumass=psi_vFit_noMC->currentState().mass();
	       mydimumasserr=sqrt( psi_vFit_noMC->currentState().kinematicParametersError().matrix()(6,6) );
	       mychisqprob= ChiSquaredProbability((double)(psi_vFit_vertex_noMC->chiSquared()),(double)(psi_vFit_vertex_noMC->degreesOfFreedom()));
	       mypx= mymumupara.momentum().x();
	       mypy=mymumupara.momentum().y();
	       mypz=mymumupara.momentum().z();
	     }
	     {
	       mumuonlyChg->push_back( (iMuon1->charge() + iMuon2->charge() ) );
	       mumuonlymuoverlapped->push_back( muon::overlap(*rmu1, *rmu2) );
	       mumuonlyctau->push_back(mymumuonlyctau);
	       mumuonlyctauerr->push_back(mymumuonlyctauerr);
	       mumuonlyVtxCL->push_back( mychisqprob );
	       mumuonlyMass->push_back( mydimumass );
	       mumuonlyMassErr->push_back( mydimumasserr );
	       mumuonlyPx->push_back( mypx );
	       mumuonlyPy->push_back( mypy );
	       mumuonlyPz->push_back( mypz );
	       mumuonlymu1Idx->push_back( std::distance(thePATMuonHandle->begin(), iMuon1));
	       mumuonlymu2Idx->push_back( std::distance(thePATMuonHandle->begin(), iMuon2));
	       ++nmumuonly;
	     }


	   }
	   //added by yik end
	   

	   if ( thePATTrackHandle->size()>=4 && thePATMuonHandle->size()>=4 ) {
	  // next check for mu3
	  for (std::vector < pat::Muon >::const_iterator iMuon3 = iMuon2 + 1;
	       iMuon3 != thePATMuonHandle->end(); ++iMuon3) { 
	    // if (iMuon2->charge() != -1) continue;
	    //if (iMuon2->charge() * iMuon1->charge() > 0) continue;
	    TrackRef muTrack3 = iMuon3->track();
	    if (muTrack3.isNull()) {
	      // cout << "continue from no track ref" << endl;
	      continue;
	    }
	    
	    const reco::Muon * rmu3 = dynamic_cast < const reco::Muon * >(iMuon3->originalObject());
	    /*
	    if (muon::overlap(*rmu1, *rmu3) ||muon::overlap(*rmu2, *rmu3)  ) {
	      // cout << "continued from overlapped muons" << endl;
	      continue;
	    }
	    */
	    if (rmu3->track()->hitPattern().numberOfValidPixelHits() < MuPixHits_c
		|| rmu3->track()->hitPattern().numberOfValidStripHits() < MuSiHits_c
		|| rmu3->track()->chi2() / rmu3->track()->ndof() > MuNormChi_c
		|| fabs(rmu3->track()->dxy(RefVtx)) > MuD0_c) {
	      continue;
	    }

		reco::Track recoMu3 = *iMuon3->track();
	  // next check for mu4
	  for (std::vector < pat::Muon >::const_iterator iMuon4 = iMuon3 + 1;
	       iMuon4 != thePATMuonHandle->end(); ++iMuon4) { 
	    // if (iMuon2->charge() != -1) continue;
	    //if (iMuon2->charge() * iMuon1->charge() > 0) continue;
	    TrackRef muTrack4 = iMuon4->track();
	    if (muTrack4.isNull()) {
	      // cout << "continue from no track ref" << endl;
	      continue;
	    }
	    
	    const reco::Muon * rmu4 = dynamic_cast < const reco::Muon * >(iMuon4->originalObject());
	    /*
	    if (muon::overlap(*rmu1, *rmu4) ||muon::overlap(*rmu2, *rmu4) || muon::overlap(*rmu3, *rmu4)  ) {
	      // cout << "continued from overlapped muons" << endl;
	      continue;
	    }
	    */
	    if (rmu4->track()->hitPattern().numberOfValidPixelHits() < MuPixHits_c
		|| rmu4->track()->hitPattern().numberOfValidStripHits() < MuSiHits_c
		|| rmu4->track()->chi2() / rmu4->track()->ndof() > MuNormChi_c
		|| fabs(rmu4->track()->dxy(RefVtx)) > MuD0_c) {
	      continue;
	    }
	    reco::Track recoMu4 = *iMuon4->track();
	    //check that the charge of the four muons to be zero
	    //requested by ARC to remove charge requirement
	    //if ( (iMuon1->charge()+iMuon2->charge()+iMuon3->charge()+iMuon4->charge() ) != 0 )  continue;
	    
	    
	    /////////////////////////////////////////////////////////////////////
	    // Trigger Matching start, added by Suleyman
	    bool mu1PassVrtx=false, mu1PassTripleL3=false, mu2PassVrtx=false, mu2PassTripleL3=false, mu3PassVrtx=false, mu3PassTripleL3=false, mu4PassVrtx=false, mu4PassTripleL3=false;
	    std::string vrtxMuMuFilter("hltVertexmumuFilterUpsilonMuon");
	    std::string tripleMuFilter("hltTripleMuL3PreFiltered0");	    
	    // Exact Match of Trigger legs, fitted leg match hltMuMuVrtx and Third one match to L3
	    // cout<<"NEW EVENT"<<endl;
	    for (unsigned int UpsTrig = 0; UpsTrig < TriggersForUpsilon_.size(); UpsTrig++)
	      {
		if(UpsilonMatchTrig[UpsTrig] != 0)
		  {
		    const pat::TriggerObjectStandAloneCollection mu1MatchVrxtFilter = iMuon1->triggerObjectMatchesByFilter(vrtxMuMuFilter);
		    const pat::TriggerObjectStandAloneCollection mu1MatchTripleL3Filter = iMuon1->triggerObjectMatchesByFilter(tripleMuFilter);		    
		    const pat::TriggerObjectStandAloneCollection mu2MatchVrxtFilter = iMuon2->triggerObjectMatchesByFilter(vrtxMuMuFilter);
		    const pat::TriggerObjectStandAloneCollection mu2MatchTripleL3Filter = iMuon2->triggerObjectMatchesByFilter(tripleMuFilter);		    
		    const pat::TriggerObjectStandAloneCollection mu3MatchVrxtFilter = iMuon3->triggerObjectMatchesByFilter(vrtxMuMuFilter);
		    const pat::TriggerObjectStandAloneCollection mu3MatchTripleL3Filter = iMuon3->triggerObjectMatchesByFilter(tripleMuFilter);		    
		    const pat::TriggerObjectStandAloneCollection mu4MatchVrxtFilter = iMuon4->triggerObjectMatchesByFilter(vrtxMuMuFilter);
		    const pat::TriggerObjectStandAloneCollection mu4MatchTripleL3Filter = iMuon4->triggerObjectMatchesByFilter(tripleMuFilter);
		    //cout<<"Hello, I am in Trigger match block"<<endl;		    

		    mu1PassVrtx = mu1MatchVrxtFilter.size() > 0;
		    mu1PassTripleL3 = mu1MatchTripleL3Filter.size() > 0;		    
		    mu2PassVrtx = mu2MatchVrxtFilter.size() > 0;
		    mu2PassTripleL3 = mu2MatchTripleL3Filter.size() > 0;		    
		    mu3PassVrtx = mu3MatchVrxtFilter.size() > 0;
		    mu3PassTripleL3 = mu3MatchTripleL3Filter.size() > 0;		    
		    mu4PassVrtx = mu4MatchVrxtFilter.size() > 0;
		    mu4PassTripleL3 = mu4MatchTripleL3Filter.size() > 0;

		    MyMu1TrigMatchVtxSD->push_back( mu1PassVrtx );
		    MyMu2TrigMatchVtxSD->push_back( mu2PassVrtx );
		    MyMu3TrigMatchVtxSD->push_back( mu3PassVrtx );
		    MyMu4TrigMatchVtxSD->push_back( mu4PassVrtx );
		    MyMu1TrigMatchL3FilterSD->push_back( mu1PassTripleL3 );
		    MyMu2TrigMatchL3FilterSD->push_back( mu2PassTripleL3 );
		    MyMu3TrigMatchL3FilterSD->push_back( mu3PassTripleL3 );
		    MyMu4TrigMatchL3FilterSD->push_back( mu4PassTripleL3 );
		  }	
	      }	    
	    // Trigger Matching end 

	    // Get The four-muon information 
	    TransientTrack muon1TT(muTrack1, &(*bFieldHandle));
	    TransientTrack muon2TT(muTrack2, &(*bFieldHandle));
	    TransientTrack muon3TT(muTrack3, &(*bFieldHandle));
	    TransientTrack muon4TT(muTrack4, &(*bFieldHandle));

	    
	    KinematicParticleFactoryFromTransientTrack pFactory;
	    
	    // The mass of a muon and the insignificant mass sigma 
	    // to avoid singularities in the covariance matrix.
	    ParticleMass muon_mass = 0.10565837;    // pdg mass
	    float muon_sigma = muon_mass * 1.e-6;
	    
	    // initial chi2 and ndf before kinematic fits.
	    float chi = 0.;
	    float ndf = 0.;
	    vector < RefCountedKinematicParticle > muonParticles;
	    muonParticles.push_back(pFactory.particle(muon1TT, muon_mass, chi, ndf, muon_sigma));
	    muonParticles.push_back(pFactory.particle(muon2TT, muon_mass, chi, ndf, muon_sigma));
	    muonParticles.push_back(pFactory.particle(muon3TT, muon_mass, chi, ndf, muon_sigma));
	    muonParticles.push_back(pFactory.particle(muon4TT, muon_mass, chi, ndf, muon_sigma));
	    
	    KinematicParticleVertexFitter fitter;
	    RefCountedKinematicTree myFourMuonVertexFitTree;
	    myFourMuonVertexFitTree = fitter.fit(muonParticles);

	/////// assign tmp values/////
	double MyFourMuonrVtxMag2DD = -999., MyFourMuonsigmaRvtxMag2DDtmp = -999.;
	float cosAlphatmp = -9, MyFourMuonFLSigtmp = -999;

	float myFourMuontmp = -9., myFourMuontmpErr = -9., myFourMuonVtxXtmp = -999., myFourMuonVtxYtmp = -999., myFourMuonVtxZtmp = -999., myFourMuonVtxXEtmp = -9., myFourMuonVtxYEtmp = -9., myFourMuonVtxZEtmp = -9.,myFourMuonVtxCLtmp = -999., myFourMuonVtxCL2tmp = -999., myFourMuonPxtmp = -999., myFourMuonPytmp = -999, myFourMuonPztmp = -999., tmpCTauFourMuon =-999, tmpCTauErrFourMuon =-9;

	int myMu4NDFtmp = -99, myMu3NDFtmp = -99, myMu2NDFtmp = -99, myMu1NDFtmp = -99, 
	myHitBeforetmp = -9,myMissAftertmp=-9;
	float myMu4Pxtmp = -999., myMu4Pytmp = -999.,myMu4Pztmp = -999., myMu4Chi2tmp = -999., myMu3Pxtmp = -999., myMu3Pytmp = -999., myMu3Pztmp = -999., myMu3Chi2tmp = -999., myMu2Pxtmp = -999.,  myMu2Pytmp = -999., myMu2Pztmp = -999., myMu2Chi2tmp = -999., myMu1Pxtmp = -999., myMu1Pytmp = -999., myMu1Pztmp =-999., myMu1Chi2tmp = -999.;

	double myTrkIsoOverFourMuSumpttmp = 0.0,  myTrkIsoOverFourMuVectpttmp= 0.0;

	if (myFourMuonVertexFitTree->isValid()) {
	  // std::cout << "caught an exception in the myFourMuon vertex fit" << std::endl;
	  //continue;
	  //}
	  myFourMuonVertexFitTree->movePointerToTheTop();
	  
	  ///Get hit pattern of tracks after and before vrtx Reco.///
	  vector<reco::Track> tmptracks;
	  tmptracks.push_back(recoMu1);
	  tmptracks.push_back(recoMu2);
	  tmptracks.push_back(recoMu3);
	  tmptracks.push_back(recoMu4);
	  myHitBeforetmp = GetHitsBefore(iSetup,tmptracks,myFourMuonVertexFitTree);
	  myMissAftertmp = GetMissesAfter(iSetup,tmptracks,myFourMuonVertexFitTree);
	  
	  RefCountedKinematicParticle myFourMuon_vFit_noMC = myFourMuonVertexFitTree->currentParticle();
	  RefCountedKinematicVertex myFourMuon_vFit_vertex_noMC = myFourMuonVertexFitTree->currentDecayVertex();
	  myFourMuontmp=myFourMuon_vFit_noMC->currentState().mass();
	  if(myFourMuon_vFit_noMC->currentState().kinematicParametersError().matrix()(6,6)>=0.0 ) {
	    myFourMuontmpErr=sqrt(myFourMuon_vFit_noMC->currentState().kinematicParametersError().matrix()(6,6));
	  }
	  
	  //cout<<"mass error="<<sqrt(myFourMuon_vFit_noMC->currentState().kinematicParametersError().matrix()[6][6])<<","<<sqrt(myFourMuon_vFit_noMC->currentState().kinematicParametersError().matrix()(6,6))<<endl;
	  //myFourMuon_vFit_noMC->currentState().mError()<<endl;
	  
	  //tmpCTauFourMuon = GetcTau(myFourMuon_vFit_vertex_noMC,myFourMuon_vFit_noMC,thePrimaryV,beamSpot);
	  //tmpCTauErrFourMuon = GetcTauErr(myFourMuon_vFit_vertex_noMC,myFourMuon_vFit_noMC,thePrimaryV,beamSpot);
	  tmpCTauFourMuon = GetcTau(myFourMuon_vFit_vertex_noMC,myFourMuon_vFit_noMC,theBeamSpotV);
	  tmpCTauErrFourMuon = GetcTauErr(myFourMuon_vFit_vertex_noMC,myFourMuon_vFit_noMC,theBeamSpotV);
	  
	  // added by asli
	  std::vector<double> MyFourMuonVtxEVec;
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().cxx() );
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().cyx() );
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().cyy() );
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().czx() );
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().czy() );
	  MyFourMuonVtxEVec.push_back( myFourMuon_vFit_vertex_noMC->error().czz() );
	  
	  SMatrixSym3D MyFourMuonVtxCov(MyFourMuonVtxEVec.begin(), MyFourMuonVtxEVec.end());
	  SMatrixSym3D totalCovMyFourMuon = MyFourMuonVtxCov + beamSpot.covariance3D();
	  SVector3 MyFourMuondistanceVector2DD( myFourMuon_vFit_vertex_noMC->position().x() - beamSpot.x0(), myFourMuon_vFit_vertex_noMC->position().y() - beamSpot.y0(), 0. );
	  
	  MyFourMuonrVtxMag2DD = ROOT::Math::Mag(MyFourMuondistanceVector2DD);
	  
	  MyFourMuonsigmaRvtxMag2DDtmp = sqrt(ROOT::Math::Similarity(totalCovMyFourMuon, MyFourMuondistanceVector2DD)) / MyFourMuonrVtxMag2DD;
	  
	  MyFourMuonFLSigtmp = (MyFourMuonrVtxMag2DD/MyFourMuonsigmaRvtxMag2DDtmp);
	  // finish added by asli
	  
	  myFourMuonVertexFitTree->movePointerToTheFirstChild();
	  RefCountedKinematicParticle mu1CandMC = myFourMuonVertexFitTree->currentParticle();
	  myFourMuonVertexFitTree->movePointerToTheNextChild();
	  RefCountedKinematicParticle mu2CandMC = myFourMuonVertexFitTree->currentParticle();
	  myFourMuonVertexFitTree->movePointerToTheNextChild();
	  RefCountedKinematicParticle mu3CandMC = myFourMuonVertexFitTree->currentParticle();
	  myFourMuonVertexFitTree->movePointerToTheNextChild();
	  RefCountedKinematicParticle mu4CandMC = myFourMuonVertexFitTree->currentParticle();
	  
	  KinematicParameters myFourMuonMu1KP = mu1CandMC->currentState().kinematicParameters();
	  KinematicParameters myFourMuonMu2KP = mu2CandMC->currentState().kinematicParameters();
	  KinematicParameters myFourMuonMu3KP = mu3CandMC->currentState().kinematicParameters();
	  KinematicParameters myFourMuonMu4KP = mu4CandMC->currentState().kinematicParameters();
	  
	  // added by asli
	  math::XYZVector pperp(myFourMuonMu4KP.momentum().x() + myFourMuonMu3KP.momentum().x() + myFourMuonMu2KP.momentum().x() + myFourMuonMu1KP.momentum().x(),
				myFourMuonMu4KP.momentum().y() + myFourMuonMu3KP.momentum().y() + myFourMuonMu2KP.momentum().y() + myFourMuonMu1KP.momentum().y(),
				0.);
	  
	  //vertex direction
	  GlobalPoint displacementFromBeamspot( -1*((beamSpot.x0() - myFourMuon_vFit_vertex_noMC->position().x()) +(myFourMuon_vFit_vertex_noMC->position().z() - beamSpot.z0()) * beamSpot.dxdz()),
						-1*((beamSpot.y0() - myFourMuon_vFit_vertex_noMC->position().y()) +(myFourMuon_vFit_vertex_noMC->position().z() - beamSpot.z0()) * beamSpot.dydz()),
						0);
	  
	  Vertex::Point vperp(displacementFromBeamspot.x(),displacementFromBeamspot.y(),0.);
	  // float cosAlphaFloat = vperp.Dot(pperp)/(vperp.R()*pperp.R());
	  cosAlphatmp = vperp.Dot(pperp)/(vperp.R()*pperp.R());
	  //    MyFourMuoncosAlpha->push_back( cosAlphaFloat );

	  //////////////Fill tmp variables ///////////////
	  myFourMuonVtxXtmp = myFourMuon_vFit_vertex_noMC->position().x();
	  myFourMuonVtxYtmp = myFourMuon_vFit_vertex_noMC->position().y();
	  myFourMuonVtxZtmp = myFourMuon_vFit_vertex_noMC->position().z();
	  myFourMuonVtxXEtmp = sqrt(myFourMuon_vFit_vertex_noMC->error().cxx());
	  myFourMuonVtxYEtmp = sqrt(myFourMuon_vFit_vertex_noMC->error().cyy());
	  myFourMuonVtxZEtmp = sqrt(myFourMuon_vFit_vertex_noMC->error().czz());
	  myFourMuonVtxCLtmp = ChiSquaredProbability((double) (myFourMuon_vFit_vertex_noMC->chiSquared()), (double) (myFourMuon_vFit_vertex_noMC->degreesOfFreedom()));
	  myFourMuonVtxCL2tmp = myFourMuon_vFit_vertex_noMC->chiSquared();
	  myFourMuonPxtmp = myFourMuonMu4KP.momentum().x() + myFourMuonMu3KP.momentum().x() + myFourMuonMu2KP.momentum().x() + myFourMuonMu1KP.momentum().x();
	  myFourMuonPytmp = myFourMuonMu4KP.momentum().y() + myFourMuonMu3KP.momentum().y() + myFourMuonMu2KP.momentum().y() + myFourMuonMu1KP.momentum().y();
	  myFourMuonPztmp = myFourMuonMu4KP.momentum().z() + myFourMuonMu3KP.momentum().z() + myFourMuonMu2KP.momentum().z() + myFourMuonMu1KP.momentum().z();
	  
	  myMu4Pxtmp = myFourMuonMu4KP.momentum().x();
          myMu4Pytmp = myFourMuonMu4KP.momentum().y();
          myMu4Pztmp = myFourMuonMu4KP.momentum().z();
          myMu4Chi2tmp = mu4CandMC->chiSquared();
          myMu4NDFtmp = mu4CandMC->degreesOfFreedom();
	  
	  myMu3Pxtmp = myFourMuonMu3KP.momentum().x();
          myMu3Pytmp = myFourMuonMu3KP.momentum().y();
          myMu3Pztmp = myFourMuonMu3KP.momentum().z();
          myMu3Chi2tmp = mu3CandMC->chiSquared();
          myMu3NDFtmp = mu3CandMC->degreesOfFreedom();
	  
	  myMu2Pxtmp = myFourMuonMu2KP.momentum().x();
	  myMu2Pytmp = myFourMuonMu2KP.momentum().y();
	  myMu2Pztmp = myFourMuonMu2KP.momentum().z();
	  myMu2Chi2tmp = mu2CandMC->chiSquared();
          myMu2NDFtmp = mu2CandMC->degreesOfFreedom();
	  
	  myMu1Pxtmp = myFourMuonMu1KP.momentum().x();
          myMu1Pytmp = myFourMuonMu1KP.momentum().y();
          myMu1Pztmp = myFourMuonMu1KP.momentum().z();
          myMu1Chi2tmp = mu1CandMC->chiSquared();
          myMu1NDFtmp = mu1CandMC->degreesOfFreedom();

	  //get trk isolation around the four muon object
	  reco::Vertex closestPV2;
	  KinematicState theCurrentXKinematicState = myFourMuon_vFit_noMC->currentState();
	  FreeTrajectoryState theXFTS = theCurrentXKinematicState.freeTrajectoryState();
	  TransientTrack XTT = (*theB).build(theXFTS);
	  double  minDist2 = 99999.;
	  for (reco::VertexCollection::const_iterator iVtx = recVtxs->begin(); iVtx != recVtxs->end(); ++iVtx) {
	    pair<bool,Measurement1D> the3DIpPairX = IPTools::absoluteImpactParameter3D(XTT,Vertex(*iVtx));	    
	    if( iVtx->ndof()>=5 && fabs(iVtx->z())<=24
		&& fabs(iVtx->position().rho())<=2.0
		&& the3DIpPairX.first && minDist2 > the3DIpPairX.second.value()){
	      minDist2 = the3DIpPairX.second.value();
	      closestPV2 = Vertex(*iVtx);
	    }//if( iVtx->ndof()>=5 && fabs(iVtx->z())<=24	    
	  }//loop Vrtx end 

	  math::XYZTLorentzVectorD FourMup4, Trackp4;
	  FourMup4.SetPxPyPzE(myFourMuonPxtmp,myFourMuonPytmp,myFourMuonPztmp,myFourMuontmp);
	  for(reco::Vertex::trackRef_iterator tv = closestPV2.tracks_begin(); tv!=closestPV2.tracks_end(); tv++){
	    const reco::Track & iextraTrk=*(tv->get());
	    reco::TrackBase::TrackQuality myquality=reco::TrackBase::qualityByName("highPurity");  //"loose","tight","highPurity"
	    Trackp4.SetXYZT(iextraTrk.px(),iextraTrk.py(),iextraTrk.pz(),0.13957);
	    if(iextraTrk.pt() > 0.9
	       && iextraTrk.quality(myquality)
	       && deltaR(FourMup4.eta(),FourMup4.phi(),Trackp4.eta(),Trackp4.phi())<0.7  //dr <0.7
	       && tv->key() != rmu1->track().key() && tv->key() != rmu2->track().key()
	       && tv->key() != rmu3->track().key() && tv->key() != rmu4->track().key()){
	      myTrkIsoOverFourMuSumpttmp+=iextraTrk.pt(); 
	      myTrkIsoOverFourMuVectpttmp+=iextraTrk.pt(); 
	    }//if(iextraTrk.pt() > 0.9
	  }//for(reco::Vertex::trackRef_iterator tv 
	  //cout<<"myTrkIsoOverFourMuSumpttmp="<<myTrkIsoOverFourMuSumpttmp<<",myTrkIsoOverFourMuVectpttmp="<<myTrkIsoOverFourMuVectpttmp<<endl;
	  myTrkIsoOverFourMuSumpttmp=myTrkIsoOverFourMuSumpttmp/(rmu1->track()->pt()+rmu2->track()->pt()+ rmu3->track()->pt()+rmu4->track()->pt());
	  myTrkIsoOverFourMuVectpttmp=myTrkIsoOverFourMuVectpttmp/FourMup4.pt();
	  //cout<<"after myTrkIsoOverFourMuSumpttmp="<<myTrkIsoOverFourMuSumpttmp<<",myTrkIsoOverFourMuVectpttmp="<<myTrkIsoOverFourMuVectpttmp<<endl;
	  //end get trk isolation around the four muon object

	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////Mass constraint fit starts here; by Maksat////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float myJpsi1massmasstmp=-9, myJpsi1massmasserrtmp=-1,myJpsi2massmasstmp=-9, myJpsi2massmasserrtmp=-1;
	float myJpsi3massmasstmp=-9, myJpsi3massmasserrtmp=-1,myJpsi4massmasstmp=-9, myJpsi4massmasserrtmp=-1;
	float myJpsi5massmasstmp=-9, myJpsi5massmasserrtmp=-1,myJpsi6massmasstmp=-9, myJpsi6massmasserrtmp=-1;
 
	float mymass1234=-9,mymass1324=-9, mymass1423=-9;
	double myJpsi1Jpsi2DisXYtmp=-9, myJpsi3Jpsi4DisXYtmp=-9, myJpsi5Jpsi6DisXYtmp=-9;
	double myJpsi1Jpsi2DisZtmp=-9, myJpsi3Jpsi4DisZtmp=-9, myJpsi5Jpsi6DisZtmp=-9;
	double tmpCTauJpsi1=-999,tmpCTauErrJpsi1=-9,tmpCTauJpsi2=-999,tmpCTauErrJpsi2=-9,tmpCTauJpsi3=-999,tmpCTauErrJpsi3=-9,tmpCTauJpsi4=-999,tmpCTauErrJpsi4=-9,tmpCTauJpsi5=-999,tmpCTauErrJpsi5=-9,tmpCTauJpsi6=-999,tmpCTauErrJpsi6=-9 ; 
	double tmpJpsi1ChiProb=-1, tmpJpsi2ChiProb=-1, tmpJpsi3ChiProb=-1,tmpJpsi4ChiProb=-1,
	  tmpJpsi5ChiProb=-1,tmpJpsi6ChiProb=-1;
	int tmpJpsi1hitVrtx=-9,tmpJpsi2hitVrtx=-9,tmpJpsi3hitVrtx=-9,tmpJpsi4hitVrtx=-9,tmpJpsi5hitVrtx=-9,
	  tmpJpsi6hitVrtx=-9,tmpJpsi1missVrtx=-9,tmpJpsi2missVrtx=-9,tmpJpsi3missVrtx=-9,tmpJpsi4missVrtx=-9,tmpJpsi5missVrtx=-9,tmpJpsi6missVrtx=-9;
	
	vector<RefCountedKinematicParticle> muonP12, muonP34, muonP13, muonP24, muonP14, muonP23;
	vector<RefCountedKinematicParticle> Chi_1, Chi_2,  Chi_3;
	vector<reco::Track> trackJpsi1,trackJpsi2,trackJpsi3,trackJpsi4,trackJpsi5,trackJpsi6;
	
	/////creating the vertex fitter
	KinematicParticleVertexFitter kpvFitter;
	/////creating the particle fitter
	KinematicParticleFitter csFitter;
	
	// creating the constraint with a little sigma to put in the resulting covariance 
	// matrix in order to avoid singularities
  	ParticleMass jp = 3.09687; 
  	float jp_m_sigma = 0.00004;
  	KinematicConstraint * jpsi_c = new MassKinematicConstraint(jp,jp_m_sigma);

	if(muon1TT.charge()+muon2TT.charge()==0)  {

		muonP12.push_back(pFactory.particle(muon1TT, muon_mass, chi, ndf, muon_sigma));
		muonP12.push_back(pFactory.particle(muon2TT, muon_mass, chi, ndf, muon_sigma));
		muonP34.push_back(pFactory.particle(muon3TT, muon_mass, chi, ndf, muon_sigma));
                muonP34.push_back(pFactory.particle(muon4TT, muon_mass, chi, ndf, muon_sigma));

		///Reconstruct J/Psi decay///
		RefCountedKinematicTree Jpsi1 = kpvFitter.fit(muonP12);
                RefCountedKinematicTree Jpsi2 = kpvFitter.fit(muonP34);

	    ////Start robust check if hypothetical vrtx is compatible with particles  
                if (Jpsi1->isValid() && Jpsi2->isValid()) //continue;
		{
		trackJpsi1.push_back(recoMu1); trackJpsi1.push_back(recoMu2);
		tmpJpsi1hitVrtx = GetHitsBefore(iSetup,trackJpsi1,Jpsi1);
      		tmpJpsi1missVrtx = GetMissesAfter(iSetup,trackJpsi1,Jpsi1);
		
		trackJpsi2.push_back(recoMu3); trackJpsi2.push_back(recoMu4);
                tmpJpsi2hitVrtx = GetHitsBefore(iSetup,trackJpsi2,Jpsi2);
                tmpJpsi2missVrtx = GetMissesAfter(iSetup,trackJpsi2,Jpsi2);

		////the constrained J/psi Mass fit:  
		if(doJPsiMassCost) {
		 Jpsi1 = csFitter.fit(jpsi_c,Jpsi1);
		 Jpsi2 = csFitter.fit(jpsi_c,Jpsi2);
		  
		  Jpsi1->movePointerToTheTop();
		  Jpsi2->movePointerToTheTop();

		  RefCountedKinematicParticle Jpsi1_part = Jpsi1->currentParticle();
		  RefCountedKinematicParticle Jpsi2_part = Jpsi2->currentParticle();
		  Chi_1.push_back(Jpsi1_part);
		  Chi_1.push_back(Jpsi2_part);

		////making a vertex fit and thus reconstructing the X particle parameters  
		
		 RefCountedKinematicTree Chi1_bTree = kpvFitter.fit(Chi_1); 

		if(!Chi1_bTree->isValid()) continue;

		RefCountedKinematicVertex myJpsi1DecayVtx = Jpsi1->currentDecayVertex();
                RefCountedKinematicVertex myJpsi2DecayVtx = Jpsi2->currentDecayVertex();

        SVector3 myJpsi1Jpsi2distanceVector2DDXY( myJpsi1DecayVtx->position().x() -myJpsi2DecayVtx->position().x(), myJpsi1DecayVtx->position().y() -myJpsi2DecayVtx->position().y(), 0 );
            myJpsi1Jpsi2DisXYtmp = ROOT::Math::Mag(myJpsi1Jpsi2distanceVector2DDXY);
        SVector3 myJpsi1Jpsi2distanceVector2DDZ( 0, 0, myJpsi1DecayVtx->position().z() -myJpsi2DecayVtx->position().z() );  
            myJpsi1Jpsi2DisZtmp = ROOT::Math::Mag(myJpsi1Jpsi2distanceVector2DDZ);

		/// Move to the head of doubleJpsi and get mass///
		Chi1_bTree->movePointerToTheTop();
                RefCountedKinematicParticle MyChi1_part = Chi1_bTree->currentParticle();

		mymass1234=MyChi1_part->currentState().mass();
		}///if mass constraint
		else {
	 	  Jpsi1->movePointerToTheTop();
                  Jpsi2->movePointerToTheTop();
                  RefCountedKinematicParticle Jpsi1_part = Jpsi1->currentParticle();
                  RefCountedKinematicParticle Jpsi2_part = Jpsi2->currentParticle();	
		  RefCountedKinematicVertex myJpsi1DecayVtx = Jpsi1->currentDecayVertex();
                  RefCountedKinematicVertex myJpsi2DecayVtx = Jpsi2->currentDecayVertex();
		  SVector3 myJpsi1Jpsi2distanceVector2DDXY( myJpsi1DecayVtx->position().x() -myJpsi2DecayVtx->position().x(), myJpsi1DecayVtx->position().y() -myJpsi2DecayVtx->position().y(), 0 );
		  myJpsi1Jpsi2DisXYtmp = ROOT::Math::Mag(myJpsi1Jpsi2distanceVector2DDXY);
		  SVector3 myJpsi1Jpsi2distanceVector2DDZ( 0, 0, myJpsi1DecayVtx->position().z() -myJpsi2DecayVtx->position().z() );
		  myJpsi1Jpsi2DisZtmp = ROOT::Math::Mag(myJpsi1Jpsi2distanceVector2DDZ);
		  
		  //tmpCTauJpsi1 = GetcTau(myJpsi1DecayVtx,Jpsi1_part,thePrimaryV,beamSpot);
		  //tmpCTauErrJpsi1 = GetcTauErr(myJpsi2DecayVtx,Jpsi1_part,thePrimaryV,beamSpot);
		  //tmpCTauJpsi2 = GetcTau(myJpsi2DecayVtx,Jpsi2_part,thePrimaryV,beamSpot);
		  //tmpCTauErrJpsi2 = GetcTauErr(myJpsi2DecayVtx,Jpsi2_part,thePrimaryV,beamSpot);
		  tmpCTauJpsi1 = GetcTau(myJpsi1DecayVtx,Jpsi1_part,theBeamSpotV);
		  tmpCTauErrJpsi1 = GetcTauErr(myJpsi2DecayVtx,Jpsi1_part,theBeamSpotV);
		  tmpCTauJpsi2 = GetcTau(myJpsi2DecayVtx,Jpsi2_part,theBeamSpotV);
		  tmpCTauErrJpsi2 = GetcTauErr(myJpsi2DecayVtx,Jpsi2_part,theBeamSpotV);
		  
		  //cout<<"mass1="<<Jpsi1_part->currentState().mass()<<",err1="<<sqrt(Jpsi1_part->currentState().kinematicParametersError().matrix()(6,6))<<",mass2="<<Jpsi2_part->currentState().mass()<<",err1="<<sqrt(Jpsi2_part->currentState().kinematicParametersError().matrix()(6,6))<<endl;		  
		  myJpsi1massmasstmp=Jpsi1_part->currentState().mass();
		  myJpsi2massmasstmp=Jpsi2_part->currentState().mass();
		  if(Jpsi1_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi1massmasserrtmp=sqrt(Jpsi1_part->currentState().kinematicParametersError().matrix()(6,6));}
		  if(Jpsi2_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi2massmasserrtmp=sqrt(Jpsi2_part->currentState().kinematicParametersError().matrix()(6,6));}

		  //cout<<myJpsi1massmasstmp<<","<<myJpsi1massmasserrtmp<<","<<myJpsi2massmasstmp<<","<<myJpsi2massmasserrtmp<<endl;

		  tmpJpsi1ChiProb = ChiSquaredProbability((double) (myJpsi1DecayVtx->chiSquared()), (double) (myJpsi1DecayVtx->degreesOfFreedom()));
		  tmpJpsi2ChiProb = ChiSquaredProbability((double) (myJpsi2DecayVtx->chiSquared()), (double) (myJpsi2DecayVtx->degreesOfFreedom()));
		  
		}///no Jpsi mass constraint 

	  }//if Jpsi1 and Jpsi2 valid	

	} /// end if(track1+track2=0)

	if(muon1TT.charge()+muon3TT.charge()==0)  {
                muonP13.push_back(pFactory.particle(muon1TT, muon_mass, chi, ndf, muon_sigma));
                muonP13.push_back(pFactory.particle(muon3TT, muon_mass, chi, ndf, muon_sigma));
                muonP24.push_back(pFactory.particle(muon2TT, muon_mass, chi, ndf, muon_sigma));
                muonP24.push_back(pFactory.particle(muon4TT, muon_mass, chi, ndf, muon_sigma));

		///Reconstruct J/Psi decay///
                RefCountedKinematicTree Jpsi3 = kpvFitter.fit(muonP13);
                RefCountedKinematicTree Jpsi4 = kpvFitter.fit(muonP24);
		
		if (Jpsi3->isValid() && Jpsi4->isValid()) //continue;
		{
		///Start robust check if hypothetical vrtx is compatible with particles  
                trackJpsi3.push_back(recoMu1); trackJpsi3.push_back(recoMu3);
                tmpJpsi3hitVrtx = GetHitsBefore(iSetup,trackJpsi3,Jpsi3);
                tmpJpsi3missVrtx = GetMissesAfter(iSetup,trackJpsi3,Jpsi3);

                trackJpsi4.push_back(recoMu2); trackJpsi4.push_back(recoMu4);
                tmpJpsi4hitVrtx = GetHitsBefore(iSetup,trackJpsi4,Jpsi4);
                tmpJpsi4missVrtx = GetMissesAfter(iSetup,trackJpsi4,Jpsi4);

		////the constrained J/psi Mass fit:  
		if(doJPsiMassCost){
                  Jpsi3 = csFitter.fit(jpsi_c,Jpsi3);
                  Jpsi4 = csFitter.fit(jpsi_c,Jpsi4);
		  Jpsi3->movePointerToTheTop();
                  Jpsi4->movePointerToTheTop();
                  RefCountedKinematicParticle Jpsi3_part = Jpsi3->currentParticle();
                  RefCountedKinematicParticle Jpsi4_part = Jpsi4->currentParticle();
                  Chi_2.push_back(Jpsi3_part);
                  Chi_2.push_back(Jpsi4_part);

                ////making a vertex fit and thus reconstructing the X particle parameters  
                RefCountedKinematicTree Chi2_bTree = kpvFitter.fit(Chi_2);
                if(!Chi2_bTree->isValid()) continue;

		RefCountedKinematicVertex myJpsi3DecayVtx = Jpsi3->currentDecayVertex();
                RefCountedKinematicVertex myJpsi4DecayVtx = Jpsi4->currentDecayVertex();

SVector3 myJpsi3Jpsi4distanceVector2DDXY( myJpsi3DecayVtx->position().x() -myJpsi4DecayVtx->position().x() , myJpsi3DecayVtx->position().y() -myJpsi4DecayVtx->position().y() , 0 );
            myJpsi3Jpsi4DisXYtmp = ROOT::Math::Mag(myJpsi3Jpsi4distanceVector2DDXY);
SVector3 myJpsi3Jpsi4distanceVector2DDZ( 0, 0, myJpsi3DecayVtx->position().z() -myJpsi4DecayVtx->position().z() );  
            myJpsi3Jpsi4DisZtmp = ROOT::Math::Mag(myJpsi3Jpsi4distanceVector2DDZ);
                Chi2_bTree->movePointerToTheTop();
                RefCountedKinematicParticle MyChi2_part = Chi2_bTree->currentParticle();
		mymass1324=MyChi2_part->currentState().mass(); 
		}///if Jpsi mass constraint
		else
		{
		Jpsi3->movePointerToTheTop();
                Jpsi4->movePointerToTheTop();
                RefCountedKinematicParticle Jpsi3_part = Jpsi3->currentParticle();
                RefCountedKinematicParticle Jpsi4_part = Jpsi4->currentParticle();
		RefCountedKinematicVertex myJpsi3DecayVtx = Jpsi3->currentDecayVertex();
                RefCountedKinematicVertex myJpsi4DecayVtx = Jpsi4->currentDecayVertex();
SVector3 myJpsi3Jpsi4distanceVector2DDXY( myJpsi3DecayVtx->position().x() -myJpsi4DecayVtx->position().x() , myJpsi3DecayVtx->position().y() -myJpsi4DecayVtx->position().y() , 0 );
            myJpsi3Jpsi4DisXYtmp = ROOT::Math::Mag(myJpsi3Jpsi4distanceVector2DDXY);
SVector3 myJpsi3Jpsi4distanceVector2DDZ( 0, 0, myJpsi3DecayVtx->position().z() -myJpsi4DecayVtx->position().z() );
            myJpsi3Jpsi4DisZtmp = ROOT::Math::Mag(myJpsi3Jpsi4distanceVector2DDZ);
	    //tmpCTauJpsi3 = GetcTau(myJpsi3DecayVtx,Jpsi3_part,thePrimaryV,beamSpot);
               //tmpCTauErrJpsi3 = GetcTauErr(myJpsi3DecayVtx,Jpsi3_part,thePrimaryV,beamSpot);
	       //tmpCTauJpsi4 = GetcTau(myJpsi4DecayVtx,Jpsi4_part,thePrimaryV,beamSpot);
               //tmpCTauErrJpsi4 = GetcTauErr(myJpsi4DecayVtx,Jpsi4_part,thePrimaryV,beamSpot);
	       tmpCTauJpsi3 = GetcTau(myJpsi3DecayVtx,Jpsi3_part,theBeamSpotV);
               tmpCTauErrJpsi3 = GetcTauErr(myJpsi3DecayVtx,Jpsi3_part,theBeamSpotV);
	       tmpCTauJpsi4 = GetcTau(myJpsi4DecayVtx,Jpsi4_part,theBeamSpotV);
               tmpCTauErrJpsi4 = GetcTauErr(myJpsi4DecayVtx,Jpsi4_part,theBeamSpotV);

		  myJpsi3massmasstmp=Jpsi3_part->currentState().mass();
		  myJpsi4massmasstmp=Jpsi4_part->currentState().mass();
		  if(Jpsi3_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi3massmasserrtmp=sqrt(Jpsi3_part->currentState().kinematicParametersError().matrix()(6,6));}
		  if(Jpsi4_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi4massmasserrtmp=sqrt(Jpsi4_part->currentState().kinematicParametersError().matrix()(6,6));}


	tmpJpsi3ChiProb = ChiSquaredProbability((double) (myJpsi3DecayVtx->chiSquared()), (double) (myJpsi3DecayVtx->degreesOfFreedom()));
        tmpJpsi4ChiProb = ChiSquaredProbability((double) (myJpsi4DecayVtx->chiSquared()), (double) (myJpsi4DecayVtx->degreesOfFreedom()));

			}///No Mass constraint
		} //if Jpsi3 Jpsi4 valid

            }/// if track1+track3 =0 


	if(muon1TT.charge()+muon4TT.charge()==0)  {
                muonP14.push_back(pFactory.particle(muon1TT, muon_mass, chi, ndf, muon_sigma));
                muonP14.push_back(pFactory.particle(muon4TT, muon_mass, chi, ndf, muon_sigma));
                muonP23.push_back(pFactory.particle(muon2TT, muon_mass, chi, ndf, muon_sigma));
                muonP23.push_back(pFactory.particle(muon3TT, muon_mass, chi, ndf, muon_sigma));

		///Reconstruct J/Psi decay///
                RefCountedKinematicTree Jpsi5 = kpvFitter.fit(muonP14);
                RefCountedKinematicTree Jpsi6 = kpvFitter.fit(muonP23);

		if (Jpsi5->isValid() && Jpsi6->isValid()) //continue;
		{
		///Start robust check if hypothetical vrtx is compatible with particles  
                trackJpsi5.push_back(recoMu1); trackJpsi5.push_back(recoMu4);
                tmpJpsi5hitVrtx = GetHitsBefore(iSetup,trackJpsi5,Jpsi5);
                tmpJpsi5missVrtx = GetMissesAfter(iSetup,trackJpsi5,Jpsi5);

                trackJpsi6.push_back(recoMu2); trackJpsi6.push_back(recoMu3);
                tmpJpsi6hitVrtx = GetHitsBefore(iSetup,trackJpsi6,Jpsi6);
                tmpJpsi6missVrtx = GetMissesAfter(iSetup,trackJpsi6,Jpsi6);

		if(doJPsiMassCost){
		///the constrained J/psi Mass fit:  
                  Jpsi5 = csFitter.fit(jpsi_c,Jpsi5);
                  Jpsi6 = csFitter.fit(jpsi_c,Jpsi6);
		  Jpsi5->movePointerToTheTop();
                  Jpsi6->movePointerToTheTop();
                  RefCountedKinematicParticle Jpsi5_part = Jpsi5->currentParticle();
                  RefCountedKinematicParticle Jpsi6_part = Jpsi6->currentParticle();
                  Chi_3.push_back(Jpsi5_part);
                  Chi_3.push_back(Jpsi6_part);
                ////making a vertex fit and thus reconstructing the X particle parameters  
                  RefCountedKinematicTree Chi3_bTree = kpvFitter.fit(Chi_3);
                if(!Chi3_bTree->isValid()) continue;

		RefCountedKinematicVertex myJpsi5DecayVtx = Jpsi5->currentDecayVertex();
                RefCountedKinematicVertex myJpsi6DecayVtx = Jpsi6->currentDecayVertex();
        SVector3 myJpsi5Jpsi6distanceVector2DDXY( myJpsi5DecayVtx->position().x() -myJpsi6DecayVtx->position().x() , myJpsi5DecayVtx->position().y() -myJpsi6DecayVtx->position().y() , 0 );
	        myJpsi5Jpsi6DisXYtmp = ROOT::Math::Mag(myJpsi5Jpsi6distanceVector2DDXY);
        SVector3 myJpsi5Jpsi6distanceVector2DDZ( 0, 0, myJpsi5DecayVtx->position().z() -myJpsi6DecayVtx->position().z()  ); 
                myJpsi5Jpsi6DisZtmp = ROOT::Math::Mag(myJpsi5Jpsi6distanceVector2DDZ);
                Chi3_bTree->movePointerToTheTop();
                RefCountedKinematicParticle MyChi3_part = Chi3_bTree->currentParticle();

		mymass1423=MyChi3_part->currentState().mass();
		}//mass constaint for Jpsi5 and Jpsi6
		else{
		  Jpsi5->movePointerToTheTop();
                  Jpsi6->movePointerToTheTop();
                  RefCountedKinematicParticle Jpsi5_part = Jpsi5->currentParticle();
                  RefCountedKinematicParticle Jpsi6_part = Jpsi6->currentParticle();
		  RefCountedKinematicVertex myJpsi5DecayVtx = Jpsi5->currentDecayVertex();
		  RefCountedKinematicVertex myJpsi6DecayVtx = Jpsi6->currentDecayVertex();
		  SVector3 myJpsi5Jpsi6distanceVector2DDXY( myJpsi5DecayVtx->position().x() -myJpsi6DecayVtx->position().x() , myJpsi5DecayVtx->position().y() -myJpsi6DecayVtx->position().y() , 0 );
		  myJpsi5Jpsi6DisXYtmp = ROOT::Math::Mag(myJpsi5Jpsi6distanceVector2DDXY);
		  SVector3 myJpsi5Jpsi6distanceVector2DDZ( 0, 0, myJpsi5DecayVtx->position().z() -myJpsi6DecayVtx->position().z()  );
		  myJpsi5Jpsi6DisZtmp = ROOT::Math::Mag(myJpsi5Jpsi6distanceVector2DDZ);
		//tmpCTauJpsi5 = GetcTau(myJpsi5DecayVtx,Jpsi5_part,thePrimaryV,beamSpot);
		//tmpCTauErrJpsi5 = GetcTauErr(myJpsi5DecayVtx,Jpsi5_part,thePrimaryV,beamSpot);
		  //tmpCTauJpsi6 = GetcTau(myJpsi6DecayVtx,Jpsi6_part,thePrimaryV,beamSpot);
		  //tmpCTauErrJpsi6 = GetcTauErr(myJpsi6DecayVtx,Jpsi6_part,thePrimaryV,beamSpot);
		  tmpCTauJpsi5 = GetcTau(myJpsi5DecayVtx,Jpsi5_part,theBeamSpotV);
		  tmpCTauErrJpsi5 = GetcTauErr(myJpsi5DecayVtx,Jpsi5_part,theBeamSpotV);
		  tmpCTauJpsi6 = GetcTau(myJpsi6DecayVtx,Jpsi6_part,theBeamSpotV);
		  tmpCTauErrJpsi6 = GetcTauErr(myJpsi6DecayVtx,Jpsi6_part,theBeamSpotV);
		  
		  myJpsi5massmasstmp=Jpsi5_part->currentState().mass();
		  myJpsi6massmasstmp=Jpsi6_part->currentState().mass();
		  if(Jpsi5_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi5massmasserrtmp=sqrt(Jpsi5_part->currentState().kinematicParametersError().matrix()(6,6));}
		  if(Jpsi6_part->currentState().kinematicParametersError().matrix()(6,6)>=0) 
		    {myJpsi6massmasserrtmp=sqrt(Jpsi6_part->currentState().kinematicParametersError().matrix()(6,6));}
		  
		  tmpJpsi5ChiProb = ChiSquaredProbability((double) (myJpsi5DecayVtx->chiSquared()), (double) (myJpsi5DecayVtx->degreesOfFreedom()));
		  tmpJpsi6ChiProb = ChiSquaredProbability((double) (myJpsi6DecayVtx->chiSquared()), (double) (myJpsi6DecayVtx->degreesOfFreedom()));
		  
		} //no mass constraint for Jpsi5, Jpsi6
		
		}//if Jpsi5 and Jpsi6 valid
	} /// track1+track4
	
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////Mass constraint fit ends here/////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/////////////////////////////////////////
	// Fill the jtree vectors
	MyFourMuonChg->push_back(  (iMuon1->charge()+iMuon2->charge()+iMuon3->charge()+iMuon4->charge() ) );
	MyFourMuonOverlap12->push_back( muon::overlap(*rmu1, *rmu2) );
	MyFourMuonOverlap13->push_back( muon::overlap(*rmu1, *rmu3) );
	MyFourMuonOverlap14->push_back( muon::overlap(*rmu1, *rmu4) );
	MyFourMuonOverlap23->push_back( muon::overlap(*rmu2, *rmu3) );
	MyFourMuonOverlap24->push_back( muon::overlap(*rmu2, *rmu4) );
	MyFourMuonOverlap34->push_back( muon::overlap(*rmu3, *rmu4) );
	MyFourMuonSharedSeg12->push_back(  muon::sharedSegments(*rmu1, *rmu2) );
	MyFourMuonSharedSeg13->push_back(  muon::sharedSegments(*rmu1, *rmu3) );
	MyFourMuonSharedSeg14->push_back(  muon::sharedSegments(*rmu1, *rmu4) );
	MyFourMuonSharedSeg23->push_back(  muon::sharedSegments(*rmu2, *rmu3) );
	MyFourMuonSharedSeg24->push_back(  muon::sharedSegments(*rmu2, *rmu4) );
	MyFourMuonSharedSeg34->push_back(  muon::sharedSegments(*rmu3, *rmu4) );
	//cout<<"4mu chg="<< (iMuon1->charge()+iMuon2->charge()+iMuon3->charge()+iMuon4->charge() )<<endl;

	My1234Mass->push_back( mymass1234);
	My1324Mass->push_back( mymass1324 );
	My1423Mass->push_back( mymass1423 );
		
	My1234JpsiDisXY->push_back(myJpsi1Jpsi2DisXYtmp);
	My1324JpsiDisXY->push_back(myJpsi3Jpsi4DisXYtmp);
	My1423JpsiDisXY->push_back(myJpsi5Jpsi6DisXYtmp);
        My1234JpsiDisZ->push_back(myJpsi1Jpsi2DisZtmp);
        My1324JpsiDisZ->push_back(myJpsi3Jpsi4DisZtmp);   
        My1423JpsiDisZ->push_back(myJpsi5Jpsi6DisZtmp);   

	MyJpsi1CTau_Mu12->push_back(tmpCTauJpsi1);
	MyJpsi1CTauErr_Mu12->push_back(tmpCTauErrJpsi1);
	MyJpsi1ChiProb_Mu12->push_back(tmpJpsi1ChiProb);
	MyJpsi1HitsBeforeMu12->push_back(tmpJpsi1hitVrtx);
	MyJpsi1MissAfterMu12->push_back(tmpJpsi1missVrtx);

	MyJpsi2CTau_Mu34->push_back(tmpCTauJpsi2);
	MyJpsi2CTauErr_Mu34->push_back(tmpCTauErrJpsi2);
	MyJpsi2ChiProb_Mu34->push_back(tmpJpsi2ChiProb);
	MyJpsi2HitsBeforeMu34->push_back(tmpJpsi2hitVrtx);
        MyJpsi2MissAfterMu34->push_back(tmpJpsi2missVrtx);

  	MyJpsi3CTau_Mu13->push_back(tmpCTauJpsi3);
        MyJpsi3CTauErr_Mu13->push_back(tmpCTauErrJpsi3);
        MyJpsi3ChiProb_Mu13->push_back(tmpJpsi3ChiProb);
	MyJpsi3HitsBeforeMu13->push_back(tmpJpsi3hitVrtx);
        MyJpsi3MissAfterMu13->push_back(tmpJpsi3missVrtx);

	MyJpsi4CTau_Mu24->push_back(tmpCTauJpsi4);
        MyJpsi4CTauErr_Mu24->push_back(tmpCTauErrJpsi4);
        MyJpsi4ChiProb_Mu24->push_back(tmpJpsi4ChiProb);
	MyJpsi4HitsBeforeMu24->push_back(tmpJpsi4hitVrtx);
        MyJpsi4MissAfterMu24->push_back(tmpJpsi4missVrtx);

	MyJpsi5CTau_Mu14->push_back(tmpCTauJpsi5);
        MyJpsi5CTauErr_Mu14->push_back(tmpCTauErrJpsi5);
        MyJpsi5ChiProb_Mu14->push_back(tmpJpsi5ChiProb);
	MyJpsi5HitsBeforeMu14->push_back(tmpJpsi5hitVrtx);
        MyJpsi5MissAfterMu14->push_back(tmpJpsi5missVrtx);

	MyJpsi6CTau_Mu23->push_back(tmpCTauJpsi6);
        MyJpsi6CTauErr_Mu23->push_back(tmpCTauErrJpsi6);
        MyJpsi6ChiProb_Mu23->push_back(tmpJpsi6ChiProb);
	MyJpsi6HitsBeforeMu23->push_back(tmpJpsi6hitVrtx);
        MyJpsi6MissAfterMu23->push_back(tmpJpsi6missVrtx);

	MyHitBeforeVrtx->push_back(myHitBeforetmp);
	MyMissAfterVrtx->push_back(myMissAftertmp);
	MyFourMuonCTau->push_back(tmpCTauFourMuon);
        MyFourMuonCTauErr->push_back(tmpCTauErrFourMuon);

	MyFourMuonMass->push_back(myFourMuontmp);
	MyFourMuonMassErr->push_back(myFourMuontmpErr);

	MyJpsi1Mass_Mu12->push_back( myJpsi1massmasstmp );
	MyJpsi1MassErr_Mu12->push_back( myJpsi1massmasserrtmp );
	MyJpsi2Mass_Mu34->push_back( myJpsi2massmasstmp );
	MyJpsi2MassErr_Mu34->push_back( myJpsi2massmasserrtmp );
	MyJpsi3Mass_Mu13->push_back( myJpsi3massmasstmp );
	MyJpsi3MassErr_Mu13->push_back( myJpsi3massmasserrtmp );
	MyJpsi4Mass_Mu24->push_back( myJpsi4massmasstmp );
	MyJpsi4MassErr_Mu24->push_back( myJpsi4massmasserrtmp );
	MyJpsi5Mass_Mu14->push_back( myJpsi5massmasstmp );
	MyJpsi5MassErr_Mu14->push_back( myJpsi5massmasserrtmp );
	MyJpsi6Mass_Mu23->push_back( myJpsi6massmasstmp );
	MyJpsi6MassErr_Mu23->push_back( myJpsi6massmasserrtmp );

	    MyFourMuonsigmaRvtxMag2D->push_back(MyFourMuonsigmaRvtxMag2DDtmp);
            MyFourMuonFLSig->push_back( MyFourMuonFLSigtmp);
            MyFourMuoncosAlpha->push_back( cosAlphatmp );
            MyFourMuonrVtxMag2D->push_back(MyFourMuonrVtxMag2DD);
	    MyFourMuonTrkIsoOverFourMuSumpt->push_back(myTrkIsoOverFourMuSumpttmp);
	    MyFourMuonTrkIsoOverFourMuVectpt->push_back(myTrkIsoOverFourMuVectpttmp);

	    MyFourMuonDecayVtxX->push_back(myFourMuonVtxXtmp);
	    MyFourMuonDecayVtxY->push_back(myFourMuonVtxYtmp);
	    MyFourMuonDecayVtxZ->push_back(myFourMuonVtxZtmp);
	    
	    MyFourMuonDecayVtxXE->push_back(myFourMuonVtxXEtmp);
	    MyFourMuonDecayVtxYE->push_back(myFourMuonVtxYEtmp);
	    MyFourMuonDecayVtxZE->push_back(myFourMuonVtxZEtmp);
	    MyFourMuonVtxCL->push_back(myFourMuonVtxCLtmp);

	    MyFourMuonVtxC2->push_back(myFourMuonVtxCL2tmp);
	    
	    MyFourMuonPx->push_back(myFourMuonPxtmp);
	    MyFourMuonPy->push_back(myFourMuonPytmp);
	    MyFourMuonPz->push_back(myFourMuonPztmp);
	    MyFourMuonMu1Idx->push_back(std::distance(thePATMuonHandle->begin(), iMuon1));
	    MyFourMuonMu2Idx->push_back(std::distance(thePATMuonHandle->begin(), iMuon2));
	    MyFourMuonMu3Idx->push_back(std::distance(thePATMuonHandle->begin(), iMuon3));
	    MyFourMuonMu4Idx->push_back(std::distance(thePATMuonHandle->begin(), iMuon4));

	    MyFourMuonMu4Px->push_back(myMu4Pxtmp);
	    MyFourMuonMu4Py->push_back(myMu4Pytmp);
	    MyFourMuonMu4Pz->push_back(myMu4Pztmp);
	    MyFourMuonMu4fChi2->push_back(myMu4Chi2tmp);
	    MyFourMuonMu4fNDF->push_back(myMu4NDFtmp);

	    MyFourMuonMu3Px->push_back(myMu3Pxtmp);
	    MyFourMuonMu3Py->push_back(myMu3Pytmp);
	    MyFourMuonMu3Pz->push_back(myMu3Pztmp);
	    MyFourMuonMu3fChi2->push_back(myMu3Chi2tmp);
	    MyFourMuonMu3fNDF->push_back(myMu3NDFtmp);

	    MyFourMuonMu2Px->push_back(myMu2Pxtmp);
	    MyFourMuonMu2Py->push_back(myMu2Pytmp);
	    MyFourMuonMu2Pz->push_back(myMu2Pztmp);
	    MyFourMuonMu2fChi2->push_back(myMu2Chi2tmp);
	    MyFourMuonMu2fNDF->push_back(myMu2NDFtmp);

	    MyFourMuonMu1Px->push_back(myMu1Pxtmp);
	    MyFourMuonMu1Py->push_back(myMu1Pytmp);
	    MyFourMuonMu1Pz->push_back(myMu1Pztmp);	    
	    MyFourMuonMu1fChi2->push_back(myMu1Chi2tmp);
	    MyFourMuonMu1fNDF->push_back(myMu1NDFtmp);
	    	    	    
	   
 
	    ++nMyFourMuon;
	    muonParticles.clear();
	  		}// 4th loop over muons (look for mu-)
	  	}// 3rd loop over muons (look for mu-)
	  }// 2nd loop over muons (look for mu-)
       }                       // first loop over muons (look for mu+)
    }                           // if four muons
    } //if two muons    
    
    // fill the tree and clear the vectors
    
    //    if (nMyFourMuon > 0 || nmumuonly>0) 
      {
	X_One_Tree_->Fill();
      }

    if (Debug_) {
      //cout << "Resetting branches, had " << nMyFourMuon << " MyFourMuon cands." << endl;
    }
    
    trigRes->clear(); trigNames->clear(); L1TT->clear(); MatchTriggerNames->clear(); 
    muIsJpsiTrigMatch->clear(); muIsUpsTrigMatch->clear(); 
    runNum = 0; evtNum = 0; lumiNum = 0;
    priVtxX = 0; priVtxY = 0; priVtxZ = 0; priVtxXE = 0;  priVtxYE = 0; priVtxZE = 0; priVtxChiNorm = 0; priVtxChi = 0; priVtxCL = 0; mybxlumicorr =0; myrawbxlumi=0;
    PriVtxXCorrX->clear();PriVtxXCorrY->clear();PriVtxXCorrZ->clear();PriVtxXCorrEX->clear();PriVtxXCorrEY->clear();
    PriVtxXCorrEZ->clear();PriVtxXCorrC2->clear();PriVtxXCorrCL->clear();

    nMu = 0;muPx->clear();muPy->clear();muPz->clear();muD0->clear();muD0E->clear();muDz->clear();muChi2->clear();muGlChi2->clear();
    mufHits->clear();muFirstBarrel->clear();muFirstEndCap->clear();muDzVtx->clear();muDxyVtx->clear();muNDF->clear();muGlNDF->clear();
    muPhits->clear();muShits->clear();muGlMuHits->clear();muType->clear();muQual->clear();muTrack->clear();muCharge->clear();muIsoratio->clear();
    muIsGoodLooseMuon->clear(); muIsGoodLooseMuonNew->clear();  muIsGoodSoftMuonNewIlse->clear(); muIsGoodSoftMuonNewIlseMod->clear(); muIsGoodTightMuon->clear(); munMatchedSeg->clear();
	muMVAMuonID->clear();   musegmentCompatibility->clear();
 
	nMyFourMuon = 0;MyFourMuonTrigMatch->clear(); MyFourMuonChg->clear(); MyFourMuonMass->clear(); MyFourMuonMassErr->clear(); MyFourMuonVtxCL->clear();MyFourMuonVtxC2->clear();
   ////New Variables////
	My1234Mass->clear();My1324Mass->clear(); My1423Mass->clear();
	My1234JpsiDisXY->clear(); My1324JpsiDisXY->clear(); My1423JpsiDisXY->clear();
        My1234JpsiDisZ->clear(); My1324JpsiDisZ->clear(); My1423JpsiDisZ->clear();
	MyHitBeforeVrtx->clear(); MyMissAfterVrtx->clear();
	MyJpsi1CTau_Mu12->clear();MyJpsi1CTauErr_Mu12->clear();MyJpsi1ChiProb_Mu12->clear();
        MyJpsi2CTau_Mu34->clear();MyJpsi2CTauErr_Mu34->clear();MyJpsi2ChiProb_Mu34->clear();
	MyJpsi3CTau_Mu13->clear();MyJpsi3CTauErr_Mu13->clear();MyJpsi3ChiProb_Mu13->clear();
        MyJpsi4CTau_Mu24->clear();MyJpsi4CTauErr_Mu24->clear();MyJpsi4ChiProb_Mu24->clear();
	MyJpsi5CTau_Mu14->clear();MyJpsi5CTauErr_Mu14->clear();MyJpsi5ChiProb_Mu14->clear();
        MyJpsi6CTau_Mu23->clear();MyJpsi6CTauErr_Mu23->clear();MyJpsi6ChiProb_Mu23->clear();
	MyJpsi1HitsBeforeMu12->clear();MyJpsi1MissAfterMu12->clear();
	MyJpsi2HitsBeforeMu34->clear();MyJpsi2MissAfterMu34->clear();
	MyJpsi3HitsBeforeMu13->clear();MyJpsi3MissAfterMu13->clear();
	MyJpsi4HitsBeforeMu24->clear();MyJpsi4MissAfterMu24->clear();
	MyJpsi5HitsBeforeMu14->clear();MyJpsi5MissAfterMu14->clear();
	MyJpsi6HitsBeforeMu23->clear();MyJpsi6MissAfterMu23->clear();
	MyFourMuonCTau->clear(); MyFourMuonCTauErr->clear();

	MyMu1TrigMatchVtxSD->clear(); MyMu2TrigMatchVtxSD->clear(); MyMu3TrigMatchVtxSD->clear(); MyMu4TrigMatchVtxSD->clear();
	MyMu1TrigMatchL3FilterSD->clear(); MyMu2TrigMatchL3FilterSD->clear(); MyMu3TrigMatchL3FilterSD->clear(); MyMu4TrigMatchL3FilterSD->clear();
	
	MyJpsi1Mass_Mu12->clear(); MyJpsi2Mass_Mu34->clear(); MyJpsi3Mass_Mu13->clear(); MyJpsi4Mass_Mu24->clear(); MyJpsi5Mass_Mu14->clear(); 
	MyJpsi6Mass_Mu23->clear();MyJpsi1MassErr_Mu12->clear(); MyJpsi2MassErr_Mu34->clear(); MyJpsi3MassErr_Mu13->clear();
	MyJpsi4MassErr_Mu24->clear(); MyJpsi5MassErr_Mu14->clear(); MyJpsi6MassErr_Mu23->clear();
	//////////////
	MyFourMuonPx->clear();MyFourMuonPy->clear();MyFourMuonPz->clear();MyFourMuonDecayVtxX->clear();MyFourMuonDecayVtxY->clear();
	MyFourMuonDecayVtxZ->clear();MyFourMuonDecayVtxXE->clear();MyFourMuonDecayVtxYE->clear();MyFourMuonDecayVtxZE->clear();
	MyFourMuonMu1Idx->clear();MyFourMuonMu2Idx->clear();MyFourMuonMu2Px->clear();MyFourMuonMu2Py->clear();MyFourMuonMu2Pz->clear();
	MyFourMuonMu1Px->clear();MyFourMuonMu1Py->clear();MyFourMuonMu1Pz->clear();MyFourMuonMu2fChi2->clear();MyFourMuonMu2fNDF->clear();
	MyFourMuonMu1fChi2->clear();MyFourMuonMu1fNDF->clear();MyFourMuoncosAlpha->clear(); MyFourMuonFLSig->clear();
	MyFourMuonrVtxMag2D->clear();MyFourMuonsigmaRvtxMag2D->clear(); MyFourMuonTrkIsoOverFourMuSumpt->clear(); MyFourMuonTrkIsoOverFourMuVectpt->clear();
	MyFourMuonMu3Idx->clear();MyFourMuonMu3Px->clear();MyFourMuonMu3Py->clear();MyFourMuonMu3Pz->clear();MyFourMuonMu3fChi2->clear();MyFourMuonMu3fNDF->clear();
	MyFourMuonMu4Idx->clear();MyFourMuonMu4Px->clear();MyFourMuonMu4Py->clear();MyFourMuonMu4Pz->clear();MyFourMuonMu4fChi2->clear();MyFourMuonMu4fNDF->clear();
	MyFourMuonOverlap12->clear();MyFourMuonOverlap13->clear(); MyFourMuonOverlap14->clear();
	MyFourMuonOverlap23->clear();MyFourMuonOverlap24->clear();MyFourMuonOverlap34->clear();
	MyFourMuonSharedSeg12->clear();MyFourMuonSharedSeg13->clear(); MyFourMuonSharedSeg14->clear();
	MyFourMuonSharedSeg23->clear();MyFourMuonSharedSeg24->clear();MyFourMuonSharedSeg34->clear();
	
	//added by yik
	nmumuonly=0;mumuonlyMass->clear();mumuonlyMassErr->clear();mumuonlyVtxCL->clear();mumuonlyPx->clear();mumuonlyPy->clear();mumuonlyPz->clear();
	mumuonlymu1Idx->clear();mumuonlymu2Idx->clear();mumuonlyctau->clear();mumuonlyctauerr->clear();mumuonlymuoverlapped->clear();mumuonlyChg->clear();
	mumuonlynsharedSeg->clear();

	if(doMC){
	  nxMC=0; 
	  MC_YmuPx->clear(); MC_YmuPy->clear(); MC_YmuPz->clear(); MC_YmuM->clear();  MC_YmuChg->clear();
	  MC_YmuPdgId->clear();
	  MC_HmuPx->clear(); MC_HmuPy->clear(); MC_HmuPz->clear(); MC_HmuM->clear(); MC_HmuChg->clear();
	  MC_HmuPdgId->clear();	  
	  MC_xPx->clear(); MC_xPy->clear(); MC_xPz->clear(); MC_xM->clear(); MC_xChg->clear(); MC_xPdgId->clear(); 
	}



	
}                               // analyze


// ------------ method called once each job just before starting event loop  ------------
void FourMuonPAT::beginRun(edm::Run const &iRun, edm::EventSetup const &iSetup) {
    // bool changed = true;
    // proccessName_="HLT";
    // hltConfig_.init(iRun,iSetup,proccessName_,changed);
}


void FourMuonPAT::beginJob() {
    edm::Service < TFileService > fs;

    // estree_ = fs->make<TTree>("eventSummary", "General Event Summary");
    X_One_Tree_ = fs->make < TTree > ("X_data", "X(3872) Data");

    X_One_Tree_->Branch("TrigRes", &trigRes);
    X_One_Tree_->Branch("TrigNames", &trigNames);
    X_One_Tree_->Branch("MatchTriggerNames", &MatchTriggerNames);
    X_One_Tree_->Branch("L1TrigRes", &L1TT);

    X_One_Tree_->Branch("evtNum", &evtNum, "evtNum/i");
    X_One_Tree_->Branch("runNum", &runNum, "runNum/i");
    X_One_Tree_->Branch("lumiNum", &lumiNum, "lumiNum/i");

    // inst. lumi is here 
    X_One_Tree_->Branch("mybxlumicorr",&mybxlumicorr,"mybxlumicorr/f");
    X_One_Tree_->Branch("myrawbxlumi",&myrawbxlumi,"myrawbxlumi/f");
    
    X_One_Tree_->Branch("priVtxX", &priVtxX, "priVtxX/f");
    X_One_Tree_->Branch("priVtxY", &priVtxY, "priVtxY/f");
    X_One_Tree_->Branch("priVtxZ", &priVtxZ, "priVtxZ/f");
    X_One_Tree_->Branch("priVtxXE", &priVtxXE, "priVtxXE/f");
    X_One_Tree_->Branch("priVtxYE", &priVtxYE, "priVtxYE/f");
    X_One_Tree_->Branch("priVtxZE", &priVtxZE, "priVtxZE/f");
    X_One_Tree_->Branch("priVtxChiNorm", &priVtxChiNorm, "priVtxChiNorm/f");
    X_One_Tree_->Branch("priVtxChi", &priVtxChi, "priVtxChi/f");
    X_One_Tree_->Branch("priVtxCL", &priVtxCL, "priVtxCL/f");

    X_One_Tree_->Branch("PriVtxXCorrX", &PriVtxXCorrX);
    X_One_Tree_->Branch("PriVtxXCorrY", &PriVtxXCorrY);
    X_One_Tree_->Branch("PriVtxXCorrZ", &PriVtxXCorrZ);
    X_One_Tree_->Branch("PriVtxXCorrEX", &PriVtxXCorrEX);
    X_One_Tree_->Branch("PriVtxXCorrEY", &PriVtxXCorrEY);
    X_One_Tree_->Branch("PriVtxXCorrEZ", &PriVtxXCorrEZ);
    X_One_Tree_->Branch("PriVtxXCorrC2", &PriVtxXCorrC2);
    X_One_Tree_->Branch("PriVtxXCorrCL", &PriVtxXCorrCL);
        
    X_One_Tree_->Branch("nMu", &nMu, "nMu/i");
    X_One_Tree_->Branch("muPx", &muPx);
    X_One_Tree_->Branch("muPy", &muPy);
    X_One_Tree_->Branch("muPz", &muPz);
    X_One_Tree_->Branch("muD0", &muD0);
    X_One_Tree_->Branch("muD0E", &muD0E);
    X_One_Tree_->Branch("muDz", &muDz);
    X_One_Tree_->Branch("muChi2", &muChi2);
    X_One_Tree_->Branch("muGlChi2", &muGlChi2);
    X_One_Tree_->Branch("mufHits", &mufHits);
    X_One_Tree_->Branch("muFirstBarrel", &muFirstBarrel);
    X_One_Tree_->Branch("muFirstEndCap", &muFirstEndCap);
    X_One_Tree_->Branch("muDzVtx", &muDzVtx);
    X_One_Tree_->Branch("muDxyVtx", &muDxyVtx);
    X_One_Tree_->Branch("muNDF", &muNDF);
    X_One_Tree_->Branch("muGlNDF", &muGlNDF);
    X_One_Tree_->Branch("muPhits", &muPhits);
    X_One_Tree_->Branch("muShits", &muShits);
    X_One_Tree_->Branch("muGlMuHits", &muGlMuHits);
    X_One_Tree_->Branch("muType", &muType);
    X_One_Tree_->Branch("muQual", &muQual);
    X_One_Tree_->Branch("muTrack", &muTrack);
    X_One_Tree_->Branch("muCharge", &muCharge);
    X_One_Tree_->Branch("muIsoratio", &muIsoratio);
    X_One_Tree_->Branch("munMatchedSeg",&munMatchedSeg);
    X_One_Tree_->Branch("muIsGoodSoftMuonNewIlseMod",&muIsGoodSoftMuonNewIlseMod);
    X_One_Tree_->Branch("muIsGoodSoftMuonNewIlse",&muIsGoodSoftMuonNewIlse);
    X_One_Tree_->Branch("muIsGoodLooseMuonNew", &muIsGoodLooseMuonNew);
    X_One_Tree_->Branch("muIsGoodLooseMuon", &muIsGoodLooseMuon);
    X_One_Tree_->Branch("muIsGoodTightMuon", &muIsGoodTightMuon);   
    X_One_Tree_->Branch("muIsJpsiTrigMatch", &muIsJpsiTrigMatch);
    X_One_Tree_->Branch("muIsUpsTrigMatch", &muIsUpsTrigMatch);
    X_One_Tree_->Branch("muMVAMuonID", &muMVAMuonID);
    X_One_Tree_->Branch("musegmentCompatibility",&musegmentCompatibility);


    X_One_Tree_->Branch("nMyFourMuon", &nMyFourMuon, "nMyFourMuon/i");
    X_One_Tree_->Branch("MyFourMuonTrigMatch", &MyFourMuonTrigMatch);
    X_One_Tree_->Branch("MyFourMuonChg", &MyFourMuonChg);

    X_One_Tree_->Branch("MyFourMuonMass", &MyFourMuonMass);
    X_One_Tree_->Branch("MyFourMuonMassErr", &MyFourMuonMassErr);

    X_One_Tree_->Branch("MyFourMuonCTau", &MyFourMuonCTau); 
    X_One_Tree_->Branch("MyFourMuonCTauErr", &MyFourMuonCTauErr);
    //trig match
    X_One_Tree_->Branch("MyMu1TrigMatchVtxSD", &MyMu1TrigMatchVtxSD); 
    X_One_Tree_->Branch("MyMu2TrigMatchVtxSD", &MyMu2TrigMatchVtxSD); 
    X_One_Tree_->Branch("MyMu3TrigMatchVtxSD", &MyMu3TrigMatchVtxSD); 
    X_One_Tree_->Branch("MyMu4TrigMatchVtxSD", &MyMu4TrigMatchVtxSD); 
    X_One_Tree_->Branch("MyMu1TrigMatchL3FilterSD", &MyMu1TrigMatchL3FilterSD); 
    X_One_Tree_->Branch("MyMu2TrigMatchL3FilterSD", &MyMu2TrigMatchL3FilterSD); 
    X_One_Tree_->Branch("MyMu3TrigMatchL3FilterSD", &MyMu3TrigMatchL3FilterSD); 
    X_One_Tree_->Branch("MyMu4TrigMatchL3FilterSD", &MyMu4TrigMatchL3FilterSD); 
    //
   //////////////// Vrtx Hit Pattern/////////////
    X_One_Tree_->Branch("MyHitBeforeVrtx", &MyHitBeforeVrtx);
    X_One_Tree_->Branch("MyMissAfterVrtx", &MyMissAfterVrtx);
   //////////////New Branches////////////////////
    X_One_Tree_->Branch("My1234Mass",&My1234Mass);
    X_One_Tree_->Branch("My1324Mass",&My1324Mass);
    X_One_Tree_->Branch("My1423Mass",&My1423Mass);

    X_One_Tree_->Branch("My1234JpsiDisXY",&My1234JpsiDisXY);
    X_One_Tree_->Branch("My1324JpsiDisXY",&My1324JpsiDisXY);
    X_One_Tree_->Branch("My1423JpsiDisXY",&My1423JpsiDisXY);
    X_One_Tree_->Branch("My1234JpsiDisZ",&My1234JpsiDisZ);
    X_One_Tree_->Branch("My1324JpsiDisZ",&My1324JpsiDisZ);
    X_One_Tree_->Branch("My1423JpsiDisZ",&My1423JpsiDisZ);

    X_One_Tree_->Branch("MyJpsi1CTau_Mu12",&MyJpsi1CTau_Mu12);
    X_One_Tree_->Branch("MyJpsi1CTauErr_Mu12",&MyJpsi1CTauErr_Mu12);
    X_One_Tree_->Branch("MyJpsi1ChiProb_Mu12",&MyJpsi1ChiProb_Mu12);
    X_One_Tree_->Branch("MyJpsi1HitsBeforeMu12",&MyJpsi1HitsBeforeMu12);
    X_One_Tree_->Branch("MyJpsi1MissAfterMu12",&MyJpsi1MissAfterMu12);

    X_One_Tree_->Branch("MyJpsi2CTau_Mu34",&MyJpsi2CTau_Mu34);
    X_One_Tree_->Branch("MyJpsi2CTauErr_Mu34",&MyJpsi2CTauErr_Mu34);
    X_One_Tree_->Branch("MyJpsi2ChiProb_Mu34",&MyJpsi2ChiProb_Mu34);
    X_One_Tree_->Branch("MyJpsi2HitsBeforeMu34",&MyJpsi2HitsBeforeMu34);
    X_One_Tree_->Branch("MyJpsi2MissAfterMu34",&MyJpsi2MissAfterMu34);
    
    X_One_Tree_->Branch("MyJpsi3CTau_Mu13",&MyJpsi3CTau_Mu13);
    X_One_Tree_->Branch("MyJpsi3CTauErr_Mu13",&MyJpsi3CTauErr_Mu13);
    X_One_Tree_->Branch("MyJpsi3ChiProb_Mu13",&MyJpsi3ChiProb_Mu13);
    X_One_Tree_->Branch("MyJpsi3HitsBeforeMu13",&MyJpsi3HitsBeforeMu13);
    X_One_Tree_->Branch("MyJpsi3MissAfterMu13",&MyJpsi3MissAfterMu13);
    
    X_One_Tree_->Branch("MyJpsi4CTau_Mu24",&MyJpsi4CTau_Mu24);
    X_One_Tree_->Branch("MyJpsi4CTauErr_Mu24",&MyJpsi4CTauErr_Mu24);
    X_One_Tree_->Branch("MyJpsi4ChiProb_Mu24",&MyJpsi4ChiProb_Mu24);
    X_One_Tree_->Branch("MyJpsi4HitsBeforeMu24",&MyJpsi4HitsBeforeMu24);
    X_One_Tree_->Branch("MyJpsi4MissAfterMu24",&MyJpsi4MissAfterMu24);


    X_One_Tree_->Branch("MyJpsi5CTau_Mu14",&MyJpsi5CTau_Mu14);
    X_One_Tree_->Branch("MyJpsi5CTauErr_Mu14",&MyJpsi5CTauErr_Mu14);
    X_One_Tree_->Branch("MyJpsi5ChiProb_Mu14",&MyJpsi5ChiProb_Mu14);
    X_One_Tree_->Branch("MyJpsi5HitsBeforeMu14",&MyJpsi5HitsBeforeMu14);
    X_One_Tree_->Branch("MyJpsi5MissAfterMu14",&MyJpsi5MissAfterMu14);


    X_One_Tree_->Branch("MyJpsi6CTau_Mu23",&MyJpsi6CTau_Mu23);
    X_One_Tree_->Branch("MyJpsi6CTauErr_Mu23",&MyJpsi6CTauErr_Mu23);
    X_One_Tree_->Branch("MyJpsi6ChiProb_Mu23",&MyJpsi6ChiProb_Mu23);
    X_One_Tree_->Branch("MyJpsi6HitsBeforeMu23",&MyJpsi6HitsBeforeMu23);
    X_One_Tree_->Branch("MyJpsi6MissAfterMu23",&MyJpsi6MissAfterMu23);

    X_One_Tree_->Branch("MyJpsi1Mass_Mu12",&MyJpsi1Mass_Mu12);
    X_One_Tree_->Branch("MyJpsi1MassErr_Mu12",&MyJpsi1MassErr_Mu12);
    X_One_Tree_->Branch("MyJpsi2Mass_Mu34",&MyJpsi2Mass_Mu34);
    X_One_Tree_->Branch("MyJpsi2MassErr_Mu34",&MyJpsi2MassErr_Mu34);
    X_One_Tree_->Branch("MyJpsi3Mass_Mu13",&MyJpsi3Mass_Mu13);
    X_One_Tree_->Branch("MyJpsi3MassErr_Mu13",&MyJpsi3MassErr_Mu13);
    X_One_Tree_->Branch("MyJpsi4Mass_Mu24",&MyJpsi4Mass_Mu24);
    X_One_Tree_->Branch("MyJpsi4MassErr_Mu24",&MyJpsi4MassErr_Mu24);
    X_One_Tree_->Branch("MyJpsi5Mass_Mu14",&MyJpsi5Mass_Mu14);
    X_One_Tree_->Branch("MyJpsi5MassErr_Mu14",&MyJpsi5MassErr_Mu14);
    X_One_Tree_->Branch("MyJpsi6Mass_Mu23",&MyJpsi6Mass_Mu23);
    X_One_Tree_->Branch("MyJpsi6MassErr_Mu23",&MyJpsi6MassErr_Mu23);

    /*
	MyJpsi1Mass_Mu12->push_back( myJpsi1massmasstmp );
	MyJpsi1MassErr_Mu12->push_back( myJpsi1massmasserrtmp );

	MyJpsi2Mass_Mu34->push_back( myJpsi2massmasstmp );
	MyJpsi2MassErr_Mu34->push_back( myJpsi2massmasserrtmp );

	MyJpsi3Mass_Mu13->push_back( myJpsi3massmasstmp );
	MyJpsi3MassErr_Mu13->push_back( myJpsi3massmasserrtmp );


	MyJpsi4Mass_Mu24->push_back( myJpsi4massmasstmp );
	MyJpsi4MassErr_Mu24->push_back( myJpsi4massmasserrtmp );

	MyJpsi5Mass_Mu14->push_back( myJpsi5massmasstmp );
	MyJpsi5MassErr_Mu14->push_back( myJpsi5massmasserrtmp );

	MyJpsi6Mass_Mu23->push_back( myJpsi6massmasstmp );
	MyJpsi6MassErr_Mu23->push_back( myJpsi6massmasserrtmp );
    */


    X_One_Tree_->Branch("MyFourMuonOverlap12", &MyFourMuonOverlap12);
    X_One_Tree_->Branch("MyFourMuonOverlap13", &MyFourMuonOverlap13);
    X_One_Tree_->Branch("MyFourMuonOverlap14", &MyFourMuonOverlap14);
    X_One_Tree_->Branch("MyFourMuonOverlap23", &MyFourMuonOverlap23);
    X_One_Tree_->Branch("MyFourMuonOverlap24", &MyFourMuonOverlap24);
    X_One_Tree_->Branch("MyFourMuonOverlap34", &MyFourMuonOverlap34);
    X_One_Tree_->Branch("MyFourMuonSharedSeg12", &MyFourMuonSharedSeg12);
    X_One_Tree_->Branch("MyFourMuonSharedSeg13", &MyFourMuonSharedSeg13);
    X_One_Tree_->Branch("MyFourMuonSharedSeg14", &MyFourMuonSharedSeg14);
    X_One_Tree_->Branch("MyFourMuonSharedSeg23", &MyFourMuonSharedSeg23);
    X_One_Tree_->Branch("MyFourMuonSharedSeg24", &MyFourMuonSharedSeg24);
    X_One_Tree_->Branch("MyFourMuonSharedSeg34", &MyFourMuonSharedSeg34);


   //////////////////////////////////////////////
    X_One_Tree_->Branch("MyFourMuonVtxCL", &MyFourMuonVtxCL);
    X_One_Tree_->Branch("MyFourMuonVtxC2", &MyFourMuonVtxC2);
    X_One_Tree_->Branch("MyFourMuonPx", &MyFourMuonPx);
    X_One_Tree_->Branch("MyFourMuonPy", &MyFourMuonPy);
    X_One_Tree_->Branch("MyFourMuonPz", &MyFourMuonPz);    
    X_One_Tree_->Branch("MyFourMuonDecayVtxX", &MyFourMuonDecayVtxX);
    X_One_Tree_->Branch("MyFourMuonDecayVtxY", &MyFourMuonDecayVtxY);
    X_One_Tree_->Branch("MyFourMuonDecayVtxZ", &MyFourMuonDecayVtxZ);    
    X_One_Tree_->Branch("MyFourMuonDecayVtxXE", &MyFourMuonDecayVtxXE);
    X_One_Tree_->Branch("MyFourMuonDecayVtxYE", &MyFourMuonDecayVtxYE);
    X_One_Tree_->Branch("MyFourMuonDecayVtxZE", &MyFourMuonDecayVtxZE);    
    X_One_Tree_->Branch("MyFourMuonMu1Idx", &MyFourMuonMu1Idx);
    X_One_Tree_->Branch("MyFourMuonMu2Idx", &MyFourMuonMu2Idx);
    X_One_Tree_->Branch("MyFourMuonMu2Px", &MyFourMuonMu2Px);
    X_One_Tree_->Branch("MyFourMuonMu2Py", &MyFourMuonMu2Py);
    X_One_Tree_->Branch("MyFourMuonMu2Pz", &MyFourMuonMu2Pz);
    X_One_Tree_->Branch("MyFourMuonMu1Px", &MyFourMuonMu1Px);
    X_One_Tree_->Branch("MyFourMuonMu1Py", &MyFourMuonMu1Py);
    X_One_Tree_->Branch("MyFourMuonMu1Pz", &MyFourMuonMu1Pz);
    X_One_Tree_->Branch("MyFourMuonMu2fChi2", &MyFourMuonMu2fChi2);
    X_One_Tree_->Branch("MyFourMuonMu2fNDF", &MyFourMuonMu2fNDF);
    X_One_Tree_->Branch("MyFourMuonMu1fChi2", &MyFourMuonMu1fChi2);
    X_One_Tree_->Branch("MyFourMuonMu1fNDF", &MyFourMuonMu1fNDF);

    X_One_Tree_->Branch("MyFourMuoncosAlpha",&MyFourMuoncosAlpha);
    X_One_Tree_->Branch("MyFourMuonFLSig",&MyFourMuonFLSig);
    X_One_Tree_->Branch("MyFourMuonrVtxMag2D",&MyFourMuonrVtxMag2D);
    X_One_Tree_->Branch("MyFourMuonsigmaRvtxMag2D",&MyFourMuonsigmaRvtxMag2D);
    X_One_Tree_->Branch("MyFourMuonTrkIsoOverFourMuSumpt",&MyFourMuonTrkIsoOverFourMuSumpt);
    X_One_Tree_->Branch("MyFourMuonTrkIsoOverFourMuVectpt",&MyFourMuonTrkIsoOverFourMuVectpt);

    X_One_Tree_->Branch("MyFourMuonMu3fNDF", &MyFourMuonMu3fNDF);
    X_One_Tree_->Branch("MyFourMuonMu3Idx",&MyFourMuonMu3Idx);
    X_One_Tree_->Branch("MyFourMuonMu3Px",&MyFourMuonMu3Px);
    X_One_Tree_->Branch("MyFourMuonMu3Py",&MyFourMuonMu3Py);
    X_One_Tree_->Branch("MyFourMuonMu3Pz",&MyFourMuonMu3Pz);
    X_One_Tree_->Branch("MyFourMuonMu3fChi2",&MyFourMuonMu3fChi2);

    X_One_Tree_->Branch("MyFourMuonMu4fNDF", &MyFourMuonMu4fNDF);
    X_One_Tree_->Branch("MyFourMuonMu4Idx",&MyFourMuonMu4Idx);
    X_One_Tree_->Branch("MyFourMuonMu4Px",&MyFourMuonMu4Px);
    X_One_Tree_->Branch("MyFourMuonMu4Py",&MyFourMuonMu4Py);
    X_One_Tree_->Branch("MyFourMuonMu4Pz",&MyFourMuonMu4Pz);
    X_One_Tree_->Branch("MyFourMuonMu4fChi2",&MyFourMuonMu4fChi2);


    //added by yik
    X_One_Tree_->Branch("nmumuonly",&nmumuonly);
    X_One_Tree_->Branch("mumuonlyMass",&mumuonlyMass);
    X_One_Tree_->Branch("mumuonlyMassErr",&mumuonlyMassErr);
    X_One_Tree_->Branch("mumuonlyVtxCL",&mumuonlyVtxCL);
    X_One_Tree_->Branch("mumuonlyPx",&mumuonlyPx);
    X_One_Tree_->Branch("mumuonlyPy",&mumuonlyPy);
    X_One_Tree_->Branch("mumuonlyPz",&mumuonlyPz);
    X_One_Tree_->Branch("mumuonlymu1Idx",&mumuonlymu1Idx);
    X_One_Tree_->Branch("mumuonlymu2Idx",&mumuonlymu2Idx);
    X_One_Tree_->Branch("mumuonlyctau",&mumuonlyctau);
    X_One_Tree_->Branch("mumuonlyctauerr",&mumuonlyctauerr);
    X_One_Tree_->Branch("mumuonlymuoverlapped",&mumuonlymuoverlapped);
    X_One_Tree_->Branch("mumuonlyChg",&mumuonlyChg);
    X_One_Tree_->Branch("mumuonlynsharedSeg",&mumuonlynsharedSeg);

    
    if(doMC){      
      X_One_Tree_->Branch("nxMC", &nxMC, "nxMC/i");      
      X_One_Tree_->Branch("MC_YmuPx",&MC_YmuPx);
      X_One_Tree_->Branch("MC_YmuPy",&MC_YmuPy);
      X_One_Tree_->Branch("MC_YmuPz",&MC_YmuPz);
      X_One_Tree_->Branch("MC_YmuM",&MC_YmuM);
      X_One_Tree_->Branch("MC_YmuChg",&MC_YmuChg);
      X_One_Tree_->Branch("MC_YmuPdgId", &MC_YmuPdgId);      
      X_One_Tree_->Branch("MC_HmuPx",&MC_HmuPx);
      X_One_Tree_->Branch("MC_HmuPy",&MC_HmuPy);
      X_One_Tree_->Branch("MC_HmuPz",&MC_HmuPz);
      X_One_Tree_->Branch("MC_HmuM",&MC_HmuM);
      X_One_Tree_->Branch("MC_HmuChg",&MC_HmuChg);
      X_One_Tree_->Branch("MC_HmuPdgId", &MC_HmuPdgId);      
      X_One_Tree_->Branch("MC_xPx",&MC_xPx);
      X_One_Tree_->Branch("MC_xPy",&MC_xPy);
      X_One_Tree_->Branch("MC_xPz",&MC_xPz);	      
      X_One_Tree_->Branch("MC_xM",&MC_xM);
      X_One_Tree_->Branch("MC_xChg",&MC_xChg);
      X_One_Tree_->Branch("MC_xPdgId",&MC_xPdgId);	
    }


   /*
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   X_One_Tree_->Branch("",&);
   */
    
}                               // begin Job

// ------------ method called once each job just after ending the event loop  ------------
void FourMuonPAT::endJob() {
    X_One_Tree_->GetDirectory()->cd();
    X_One_Tree_->Write();
//	myfile.close();
}                               // endjob


//define this as a plug-in
DEFINE_FWK_MODULE(FourMuonPAT);
