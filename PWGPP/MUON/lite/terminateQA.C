#if !defined(__CINT__) || defined(__MAKECINT__)
// ROOT includes
#include "TFile.h"
#include "TGrid.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TSystem.h"
#include "TROOT.h"

// Aliroot includes
#include "AliAnalysisManager.h"
#include "AliAnalysisAlien.h"
#include "AliESDInputHandler.h"

#define COMPILEMACRO

#endif


//_____________________________________________________________________________
Int_t GetRunNumber(TString filePath)
{
  TObjArray* array = filePath.Tokenize("/");
  array->SetOwner();
  TString auxString = "";
  Int_t runNum = -1;
  for ( Int_t ientry=0; ientry<array->GetEntries(); ientry++ ) {
    auxString = array->At(ientry)->GetName();
    if ( auxString.Length() == 9 && auxString.IsDigit() ) {
      runNum = auxString.Atoi();
      break;
    }
  }
  delete array;

  if ( runNum < 0 ) {
    array = auxString.Tokenize("_");
    array->SetOwner();
    auxString = array->Last()->GetName();
    auxString.ReplaceAll(".root","");
    if ( auxString.IsDigit() )
      runNum = auxString.Atoi();
    delete array;
  }

  return runNum;
}

//_____________________________________________________________________________
void CopyFile(TString inFilename, TString baseOutDir=".", Bool_t makeRunDir=kTRUE, TString changeFilename="")
{
  if ( inFilename.Contains("alien://") && ! gGrid )
    TGrid::Connect("alien://");

  TObjArray* array = inFilename.Tokenize("/");
  array->SetOwner();
  TString outFilename = changeFilename.IsNull() ? array->Last()->GetName() : changeFilename.Data();
  delete array;

  if ( makeRunDir ) {
    Int_t runNumber = GetRunNumber(inFilename);
    if ( runNumber >= 0 ) {
      baseOutDir = Form("%s/%i", baseOutDir.Data(), runNumber);
      if ( gSystem->AccessPathName(baseOutDir.Data()) )
        gSystem->mkdir(baseOutDir.Data());
    }
    else printf("Warning: run number not found!\n");
  }
  //outFilename.ReplaceAll(".root", Form("_%i.root", runNumber));
  outFilename.Prepend(Form("%s/", baseOutDir.Data()));
  Bool_t showProgressBar = ! gROOT->IsBatch();
  TFile::Cp(inFilename.Data(), outFilename.Data(), showProgressBar);
  printf("outDir: %s\n", baseOutDir.Data());
  printf("outFile: %s\n", outFilename.Data());
}



//_____________________________________________________________________________
AliAnalysisAlien* CreateAlienHandler()
{
  AliAnalysisAlien *plugin = new AliAnalysisAlien();
  
  // Set the run mode
  plugin->SetRunMode("terminate");

  // Declare all libraries
  plugin->SetAdditionalLibs("libCORRFW.so libPWGHFbase.so libPWGmuon.so libPWGPPMUONlite.so");

  plugin->SetAdditionalRootLibs("libXMLParser.so libGui.so libProofPlayer.so");

  plugin->AddIncludePath("-I.");
  plugin->AddIncludePath("-I$ALICE_PHYSICS/PWGPP/MUON/lite");
  
  return plugin;
}


//_____________________________________________________________________________
void terminateQA(TString outfilename = "QAresults.root", Bool_t force = kFALSE, Bool_t runTrig = kFALSE, Bool_t usePhysicsSelection = kTRUE )
{
  //
  // Load common libraries
  //
  gSystem->Load("libTree");
  gSystem->Load("libGeom");
  gSystem->Load("libVMC");
  gSystem->Load("libPhysics");
  gSystem->Load("libProof");

  TString libsList = "libANALYSIS libOADB libANALYSISalice libCORRFW libPWGHFbase libPWGmuon libPWGPPMUONlite";

  TObjArray* libsArray = libsList.Tokenize(" ");
  libsArray->SetOwner();
  TString currLib = "";
  for ( Int_t ilib=0; ilib<libsArray->GetEntries(); ilib++ ) {
    currLib = libsArray->At(ilib)->GetName();
    gSystem->Load(currLib.Data());
  }

  AliAnalysisAlien* alienHandler = CreateAlienHandler();

  AliAnalysisManager* mgr = new AliAnalysisManager("testAnalysis");
  mgr->SetCommonFileName(outfilename.Data());
  mgr->SetGridHandler(alienHandler);

  // Needed to the manager (but not used in terminate mode)
  AliESDInputHandler* esdH = new AliESDInputHandler();
  esdH->SetReadFriends(kFALSE);
  mgr->SetInputEventHandler(esdH); 

#ifndef COMPILEMACRO

  TString physSelPattern = "PhysSelPass";
  if ( ! usePhysicsSelection ) physSelPattern.Append(",PhysSelReject");

  if ( runTrig ) {
    gROOT->LoadMacro("$ALICE_PHYSICS/PWGPP/macros/AddTaskMTRchamberEfficiency.C");
    AliAnalysisTaskTrigChEff* trigChEffTask = AddTaskMTRchamberEfficiency(kFALSE);
    trigChEffTask->SetTerminateOptions(physSelPattern.Data(),"ANY","-5_105",Form("FORCEBATCH trigChEff_ANY_Apt_allTrig.root?%s?ANY?-5_105?NoSelMatchAptFromTrg",physSelPattern.Data()));
  }
  else {
    gROOT->LoadMacro("$ALICE_PHYSICS/PWGPP/PilotTrain/AddTaskMuonQA.C");
    AliAnalysisTaskMuonQA* muonQATask = AddTaskMuonQA(usePhysicsSelection);
  }

#endif

  // Check if terminate was already performed
  if  ( ! force ) {
    TObject* paramContainer = mgr->GetParamOutputs()->At(0);
    if ( paramContainer ) {
      TFile* file = TFile::Open(outfilename);
      if ( file->FindObjectAny(paramContainer->GetName() ) ) {
        printf("\nTerminate was already executed!\n");
        printf("Nothing to be done\n");
        file->Close();
        return;
      }
      file->Close();
    }
  }


  if ( ! mgr->InitAnalysis()) {
    printf("Fatal: Cannot initialize analysis\n");
    return;
  }
  mgr->PrintStatus();
  mgr->StartAnalysis("grid terminate");
}
