#include "AliAnalysisTaskNanoAODFemtoDreamPhi.h"
#include "AliFemtoDreamBasePart.h"
#include "AliLog.h"
#include "AliNanoAODTrack.h"
#include "AliVEvent.h"
#include "TH1F.h"
#include "TList.h"

ClassImp(AliAnalysisTaskNanoAODFemtoDreamPhi)
    AliAnalysisTaskNanoAODFemtoDreamPhi::AliAnalysisTaskNanoAODFemtoDreamPhi()
    : AliAnalysisTaskSE(),
      fIsMC(false),
      fTrigger(AliVEvent::kINT7),
      fOutput(),
      fInputEvent(nullptr),
      fEvent(),
      fTrack(),
      fPhiParticle(),
      fEventCuts(),
      fPosKaonCuts(),
      fNegKaonCuts(),
      fPhiCuts(),
      fTrackCutsPartProton(),
      fTrackCutsPartAntiProton(),
      fConfig(),
      fPairCleaner(),
      fPartColl(),
      fGTI(),
      fTrackBufferSize() {}

AliAnalysisTaskNanoAODFemtoDreamPhi::AliAnalysisTaskNanoAODFemtoDreamPhi(
    const char *name, bool isMC)
    : AliAnalysisTaskSE(name),
      fIsMC(isMC),
      fTrigger(AliVEvent::kINT7),
      fOutput(),
      fInputEvent(nullptr),
      fEvent(),
      fTrack(),
      fPhiParticle(),
      fEventCuts(),
      fPosKaonCuts(),
      fNegKaonCuts(),
      fPhiCuts(),
      fTrackCutsPartProton(),
      fTrackCutsPartAntiProton(),
      fConfig(),
      fPairCleaner(),
      fPartColl(),
      fGTI(),
      fTrackBufferSize(2000) {
  DefineOutput(1, TList::Class());
}

AliAnalysisTaskNanoAODFemtoDreamPhi::~AliAnalysisTaskNanoAODFemtoDreamPhi() {}

void AliAnalysisTaskNanoAODFemtoDreamPhi::UserCreateOutputObjects() {
  fOutput = new TList();
  fOutput->SetName("Output");
  fOutput->SetOwner();

  fEvent = new AliFemtoDreamEvent(false, true, fTrigger);
  fEvent->SetCalcSpherocity(true);
  fOutput->Add(fEvent->GetEvtCutList());

  fTrack = new AliFemtoDreamTrack();
  fTrack->SetUseMCInfo(fIsMC);

  fPhiParticle = new AliFemtoDreamv0();
  fPhiParticle->SetPDGCode(fPhiCuts->GetPDGv0());
  fPhiParticle->SetUseMCInfo(fIsMC);
  fPhiParticle->SetPDGDaughterPos(
      fPhiCuts->GetPDGPosDaug());  // order +sign doesnt play a role
  fPhiParticle->GetPosDaughter()->SetUseMCInfo(fIsMC);
  fPhiParticle->SetPDGDaughterNeg(
      fPhiCuts->GetPDGNegDaug());  // only used for MC Matching
  fPhiParticle->GetNegDaughter()->SetUseMCInfo(fIsMC);

  fGTI = new AliVTrack *[fTrackBufferSize];

  if (!fEventCuts) {
    AliFatal("Event Cuts not set!");
  }
  fEventCuts->InitQA();
  fOutput->Add(fEventCuts->GetHistList());

  if (!fPosKaonCuts) {
    AliFatal("Track Cuts for Particle One not set!");
  }
  fPosKaonCuts->Init();
  fPosKaonCuts->SetName("Particle1");
  fOutput->Add(fPosKaonCuts->GetQAHists());

  if (fPosKaonCuts->GetIsMonteCarlo()) {
    fPosKaonCuts->SetMCName("MCParticle1");
    fOutput->Add(fPosKaonCuts->GetMCQAHists());
  }

  if (!fNegKaonCuts) {
    AliFatal("Track Cuts for Particle One not set!");
  }
  fNegKaonCuts->Init();
  fNegKaonCuts->SetName("Particle2");
  fOutput->Add(fNegKaonCuts->GetQAHists());
  if (fNegKaonCuts->GetIsMonteCarlo()) {
    fNegKaonCuts->SetMCName("MCParticle2");
    fOutput->Add(fNegKaonCuts->GetMCQAHists());
  }

  if (!fPhiCuts) {
    AliFatal("Cuts for the phi not set!");
  }
  fPhiCuts->Init();
  fPhiCuts->SetName("Phi");
  fOutput->Add(fPhiCuts->GetQAHists());
  if (fPhiCuts->GetIsMonteCarlo()) {
    fPhiCuts->SetMCName("MCPhi");
    fOutput->Add(fPhiCuts->GetMCQAHists());
  }

  if (!fTrackCutsPartProton) {
    AliFatal("Track Cuts for Proton not set!");
  }
  fTrackCutsPartProton->Init();
  fTrackCutsPartProton->SetName("Proton");
  fOutput->Add(fTrackCutsPartProton->GetQAHists());
  if (fTrackCutsPartProton->GetIsMonteCarlo()) {
    fTrackCutsPartProton->SetMCName("MCProton");
    fOutput->Add(fTrackCutsPartProton->GetMCQAHists());
  }

  if (!fTrackCutsPartAntiProton) {
    AliFatal("Track Cuts for AntiProton not set!");
  }
  fTrackCutsPartAntiProton->Init();
  fTrackCutsPartAntiProton->SetName("AntiProton");
  fOutput->Add(fTrackCutsPartAntiProton->GetQAHists());
  if (fTrackCutsPartAntiProton->GetIsMonteCarlo()) {
    fTrackCutsPartAntiProton->SetMCName("MCAntiProton");
    fOutput->Add(fTrackCutsPartAntiProton->GetMCQAHists());
  }

  fPairCleaner =
      new AliFemtoDreamPairCleaner(3, 1, fConfig->GetMinimalBookingME());
  fOutput->Add(fPairCleaner->GetHistList());

  fPartColl =
      new AliFemtoDreamPartCollection(fConfig, fConfig->GetMinimalBookingME());

  fOutput->Add(fPartColl->GetHistList());
  fOutput->Add(fPartColl->GetQAList());

  PostData(1, fOutput);
}

void AliAnalysisTaskNanoAODFemtoDreamPhi::UserExec(Option_t *) {
  AliVEvent *fInputEvent = InputEvent();

  // PREAMBLE - CHECK EVERYTHING IS THERE
  if (!fInputEvent) {
    AliError("No Input event");
    return;
  }

  // EVENT SELECTION
  fEvent->SetEvent(fInputEvent);
  if (!fEventCuts->isSelected(fEvent)) return;

  ResetGlobalTrackReference();
  for (int iTrack = 0; iTrack < fInputEvent->GetNumberOfTracks(); ++iTrack) {
    AliVTrack *track = static_cast<AliVTrack *>(fInputEvent->GetTrack(iTrack));
    if (!track) {
      AliFatal("No Standard AOD");
      return;
    }
    StoreGlobalTrackReference(track);
  }
  fTrack->SetGlobalTrackInfo(fGTI, fTrackBufferSize);

  static std::vector<AliFemtoDreamBasePart> Particles;
  Particles.clear();
  static std::vector<AliFemtoDreamBasePart> AntiParticles;
  AntiParticles.clear();
  static std::vector<AliFemtoDreamBasePart> V0Particles;
  V0Particles.clear();
  static std::vector<AliFemtoDreamBasePart> Protons;
  Protons.clear();
  static std::vector<AliFemtoDreamBasePart> AntiProtons;
  AntiProtons.clear();
  static std::vector<AliFemtoDreamBasePart> PhiTRUE;
  PhiTRUE.clear();
  static std::vector<AliFemtoDreamBasePart> ProtonTRUE;
  ProtonTRUE.clear();
  static std::vector<AliFemtoDreamBasePart> AProtonTRUE;
  AProtonTRUE.clear();
  static std::vector<AliFemtoDreamBasePart> PhiPRIM;
  PhiPRIM.clear();
  static std::vector<AliFemtoDreamBasePart> ProtonNOprim;
  ProtonNOprim.clear();
  static std::vector<AliFemtoDreamBasePart> AProtonNOprim;
  AProtonNOprim.clear();
  static std::vector<AliFemtoDreamBasePart> ProtonCOMMON;
  ProtonNOprim.clear();
  static std::vector<AliFemtoDreamBasePart> AProtonCOMMON;
  AProtonNOprim.clear();
  static std::vector<AliFemtoDreamBasePart> PhiCOMMON;
  PhiCOMMON.clear();

  static float massKaon =
      TDatabasePDG::Instance()->GetParticle(fPosKaonCuts->GetPDGCode())->Mass();

  for (int iTrack = 0; iTrack < fInputEvent->GetNumberOfTracks(); ++iTrack) {
    AliVTrack *track = static_cast<AliVTrack *>(fInputEvent->GetTrack(iTrack));

    if (!track) continue;
    fTrack->SetTrack(track, fInputEvent);
    // find mothers of MC Kaons (if phi->stop loop, else loop until arriving to
    // g,q,p) and set MotherPDG
    if (fIsMC) {
      TClonesArray *mcarray = dynamic_cast<TClonesArray *>(
          fInputEvent->FindListObject(AliAODMCParticle::StdBranchName()));
      if (!mcarray) {
        AliError("SPTrack: MC Array not found");
      }
      if (fTrack->GetID() >= 0) {
        AliAODMCParticle *mcPart =
            (AliAODMCParticle *)mcarray->At(fTrack->GetID());
        if (!(mcPart)) {
          break;
        }
        int motherID = mcPart->GetMother();
        int lastMother = motherID;
        AliAODMCParticle *mcMother = nullptr;
        while (motherID != -1) {
          lastMother = motherID;
          mcMother = (AliAODMCParticle *)mcarray->At(motherID);
          motherID = mcMother->GetMother();
          if (mcMother->GetPdgCode() == 333) {
            break;
          }
        }
        if (lastMother != -1) {
          mcMother = (AliAODMCParticle *)mcarray->At(lastMother);
        }
        if (mcMother) {
          fTrack->SetMotherPDG(mcMother->GetPdgCode());
          //          std::cout<<"Track Mother: "<<fTrack->GetMotherPDG()<<endl;
          fTrack->SetMotherID(lastMother);
        }
      } else {
        break;  // if we don't have MC Information, don't use that track
      }
    }
    fTrack->SetInvMass(massKaon);
    if (fPosKaonCuts->isSelected(fTrack)) {
      Particles.push_back(*fTrack);
    }
    if (fNegKaonCuts->isSelected(fTrack)) {
      AntiParticles.push_back(*fTrack);
    }
    if (fTrackCutsPartProton->isSelected(fTrack)) {
      Protons.push_back(*fTrack);
    }
    if (fTrackCutsPartAntiProton->isSelected(fTrack)) {
      AntiProtons.push_back(*fTrack);
    }
  }

  if (fIsMC) {
    TClonesArray *fArrayMCAOD = dynamic_cast<TClonesArray *>(
        fInputEvent->FindListObject(AliAODMCParticle::StdBranchName()));
    int noPart = fArrayMCAOD->GetEntriesFast();
    int mcpdg;
    AliFemtoDreamBasePart part;
    AliFemtoDreamBasePart part2;
    for (int iPart = 1; iPart < noPart; iPart++) {
      AliAODMCParticle *mcPart = (AliAODMCParticle *)fArrayMCAOD->At(iPart);
      if (!(mcPart)) {
        std::cout << "NO MC particle" << std::endl;
        continue;
      }
      mcpdg = mcPart->GetPdgCode();
      if (mcpdg == 333) {
        int firstdaughter = mcPart->GetDaughterFirst();
        AliAODMCParticle *mcDaughter =
            (AliAODMCParticle *)fArrayMCAOD->At(firstdaughter);

        if (mcDaughter) {
          int dpdg = mcDaughter->GetPdgCode();
          double dpt = mcDaughter->Pt();
          double deta = mcDaughter->Eta();
          if (std::abs(dpdg) == 321) {
            if ((dpt < 999 && dpt > 0.15) && (deta > -0.8 && deta < 0.8)) {
              part.SetMCParticleRePart(mcPart);
              PhiTRUE.push_back(part);
              continue;
            }
          }
        }
      }

      if (mcpdg == 333) {
        int firstdaughter = mcPart->GetDaughterFirst();
        AliAODMCParticle *mcDaughter =
            (AliAODMCParticle *)fArrayMCAOD->At(firstdaughter);

        if (mcDaughter) {
          int dpdg = mcDaughter->GetPdgCode();
          if (std::abs(dpdg) == 321) {
            if (mcDaughter->IsPhysicalPrimary()) {
              double dpt = mcDaughter->Pt();
              double deta = mcDaughter->Eta();
              if ((dpt < 999 && dpt > 0.15) && (deta > -0.8 && deta < 0.8)) {
                part.SetMCParticleRePart(mcPart);
                PhiPRIM.push_back(part);
                continue;
              }
            }
          }
        }
      }

      if (mcpdg == 2212) {
        if (!(mcPart->IsPhysicalPrimary())) {
          double pt = mcPart->Pt();
          double eta = mcPart->Eta();

          if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
            part.SetMCParticleRePart(mcPart);
            ProtonNOprim.push_back(part);
            continue;
          }
        }
      }

      if (mcpdg == -2212) {
        if (!(mcPart->IsPhysicalPrimary())) {
          double pt = mcPart->Pt();
          double eta = mcPart->Eta();
          if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
            part.SetMCParticleRePart(mcPart);
            AProtonNOprim.push_back(part);
            continue;
          }
        }
      }

      if (mcpdg == 2212) {
        double pt = mcPart->Pt();
        double eta = mcPart->Eta();

        if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
          part.SetMCParticleRePart(mcPart);
          ProtonTRUE.push_back(part);
          continue;
        }
      }

      if (mcpdg == -2212) {
        double pt = mcPart->Pt();
        double eta = mcPart->Eta();
        if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
          part.SetMCParticleRePart(mcPart);
          AProtonTRUE.push_back(part);
          continue;
        }
      }

      if (mcpdg == 333) {
        int motherID = mcPart->GetMother();
        AliAODMCParticle *mcMother =
            (AliAODMCParticle *)fArrayMCAOD->At(motherID);
        if (mcMother) {
          int DaughterID = mcMother->GetDaughterFirst();
          if (DaughterID == motherID) {
            DaughterID = mcMother->GetDaughterLast();
            AliAODMCParticle *mcDaughter2 =
                (AliAODMCParticle *)fArrayMCAOD->At(DaughterID);
            if (mcDaughter2) {
              int d2pdg = mcDaughter2->GetPdgCode();
              if (d2pdg == 2212) {
                double pt = mcDaughter2->Pt();
                double eta = mcDaughter2->Eta();
                if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
                  part.SetMCParticleRePart(mcDaughter2);
                  part2.SetMCParticleRePart(mcPart);
                  ProtonCOMMON.push_back(part);
                  PhiCOMMON.push_back(part2);
                  continue;
                }
              }
              if (d2pdg == -2212) {
                double pt = mcDaughter2->Pt();
                double eta = mcDaughter2->Eta();
                if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
                  part.SetMCParticleRePart(mcDaughter2);
                  part2.SetMCParticleRePart(mcPart);
                  AProtonCOMMON.push_back(part);
                  PhiCOMMON.push_back(part2);
                  continue;
                }
              }
            }
          } else {
            AliAODMCParticle *mcDaughter1 =
                (AliAODMCParticle *)fArrayMCAOD->At(DaughterID);
            if (mcDaughter1) {
              int d2pdg = mcDaughter1->GetPdgCode();
              if (d2pdg == 2212) {
                double pt = mcDaughter1->Pt();
                double eta = mcDaughter1->Eta();
                if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
                  part.SetMCParticleRePart(mcDaughter1);
                  part2.SetMCParticleRePart(mcPart);
                  ProtonCOMMON.push_back(part);
                  PhiCOMMON.push_back(part2);
                  continue;
                }
              }
              if (d2pdg == -2212) {
                double pt = mcDaughter1->Pt();
                double eta = mcDaughter1->Eta();
                if ((pt < 4.05 && pt > 0.5) && (eta > -0.8 && eta < 0.8)) {
                  part.SetMCParticleRePart(mcDaughter1);
                  part2.SetMCParticleRePart(mcPart);
                  AProtonCOMMON.push_back(part);
                  PhiCOMMON.push_back(part2);
                  continue;
                }
              }
            }
          }
        }
      }
    }
  }

  fPhiParticle->SetGlobalTrackInfo(fGTI, fTrackBufferSize);
  for (const auto &posK : Particles) {
    for (const auto &negK : AntiParticles) {
      fPhiParticle->Setv0(posK, negK, fInputEvent, false, false, true);
      fPhiParticle->SetParticleOrigin(AliFemtoDreamBasePart::kPhysPrimary);
      //      std::cout<<"ID mother kp: "<<posK.GetMotherID()<<endl;
      //      std::cout<<"ID mother km: "<<negK.GetMotherID()<<endl;
      //      std::cout<<"PDG kp: "<<posK.GetMCPDGCode()<<endl;
      //      std::cout<<"PDG km: "<<negK.GetMCPDGCode()<<endl;
      //      std::cout<<"PDG phi: "<<fPhiParticle->GetMCPDGCode()<<endl;
      if (fPhiCuts->isSelected(fPhiParticle)) {
        fPhiParticle->SetCPA(
            gRandom->Uniform());  // cpacode needed for CleanDecay v0;
        V0Particles.push_back(*fPhiParticle);
        //      std::cout<<"PDG phi cut:
        //      "<<fPhiParticle->GetMCPDGCode()<<endl;
      }
    }
  }

  fPairCleaner->CleanTrackAndDecay(&Protons, &AntiProtons, 0);
  fPairCleaner->CleanTrackAndDecay(&Protons, &V0Particles, 1);
  fPairCleaner->CleanTrackAndDecay(&AntiProtons, &V0Particles, 2);
  fPairCleaner->CleanDecay(&V0Particles, 0);
  fPairCleaner->ResetArray();

  fPairCleaner->StoreParticle(Protons);
  fPairCleaner->StoreParticle(AntiProtons);
  fPairCleaner->StoreParticle(V0Particles);
  fPairCleaner->StoreParticle(ProtonTRUE);
  fPairCleaner->StoreParticle(AProtonTRUE);
  fPairCleaner->StoreParticle(PhiTRUE);
  fPairCleaner->StoreParticle(PhiPRIM);
  fPairCleaner->StoreParticle(ProtonCOMMON);
  fPairCleaner->StoreParticle(AProtonCOMMON);
  fPairCleaner->StoreParticle(PhiCOMMON);
  fPairCleaner->StoreParticle(ProtonNOprim);
  fPairCleaner->StoreParticle(AProtonNOprim);

  fPartColl->SetEvent(fPairCleaner->GetCleanParticles(), fEvent->GetZVertex(),
                      fEvent->GetRefMult08(), fEvent->GetV0MCentrality());

  PostData(1, fOutput);
}

void AliAnalysisTaskNanoAODFemtoDreamPhi::ResetGlobalTrackReference() {
  for (UShort_t i = 0; i < fTrackBufferSize; i++) {
    fGTI[i] = 0;
  }
}

void AliAnalysisTaskNanoAODFemtoDreamPhi::StoreGlobalTrackReference(
    AliVTrack *track) {
  // see AliFemtoDreamAnalysis for details
  AliNanoAODTrack *nanoTrack = dynamic_cast<AliNanoAODTrack *>(track);
  const int trackID = track->GetID();
  if (trackID < 0) {
    return;
  }
  if (trackID >= fTrackBufferSize) {
    printf("Warning: track ID too big for buffer: ID: %d, buffer %d\n", trackID,
           fTrackBufferSize);
    return;
  }

  if (fGTI[trackID]) {
    if ((!nanoTrack->GetFilterMap()) && (!track->GetTPCNcls())) {
      return;
    }
    if (dynamic_cast<AliNanoAODTrack *>(fGTI[trackID])->GetFilterMap() ||
        fGTI[trackID]->GetTPCNcls()) {
      printf("Warning! global track info already there!");
      printf("         TPCNcls track1 %u track2 %u",
             (fGTI[trackID])->GetTPCNcls(), track->GetTPCNcls());
      printf("         FilterMap track1 %u track2 %u\n",
             dynamic_cast<AliNanoAODTrack *>(fGTI[trackID])->GetFilterMap(),
             nanoTrack->GetFilterMap());
    }
  }
  (fGTI[trackID]) = track;
}
