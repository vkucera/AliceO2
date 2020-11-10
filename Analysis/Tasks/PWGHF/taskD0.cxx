// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file taskD0.cxx
/// \brief D0 analysis task
///
/// \author Gian Michele Innocenti <gian.michele.innocenti@cern.ch>, CERN
/// \author Vít Kučera <vit.kucera@cern.ch>, CERN

#include "Framework/AnalysisTask.h"
#include "Framework/HistogramRegistry.h"
#include "Analysis/HFSecondaryVertex.h"
#include "Analysis/HFCandidateSelectionTables.h"

using namespace o2;
using namespace o2::framework;
using namespace o2::aod::hf_cand_prong2;
using namespace o2::framework::expressions;

void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  ConfigParamSpec optionDoMC{"doMC", VariantType::Bool, false, {"Fill MC histograms."}};
  workflowOptions.push_back(optionDoMC);
}

#include "Framework/runDataProcessing.h"

/// D0 analysis task
struct TaskD0 {
  HistogramRegistry registry{
    "registry",
    {{"hmass", "2-prong candidates;inv. mass (#pi K) (GeV/#it{c}^{2});entries", {HistType::kTH1F, {{500, 0., 5.}}}},
     {"hptcand", "2-prong candidates;candidate #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hptprong0", "2-prong candidates;prong 0 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hptprong1", "2-prong candidates;prong 1 #it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hdeclength", "2-prong candidates;decay length (cm);entries", {HistType::kTH1F, {{200, 0., 2.}}}},
     {"hdeclengthxy", "2-prong candidates;decay length xy (cm);entries", {HistType::kTH1F, {{200, 0., 2.}}}},
     {"hd0Prong0", "2-prong candidates;prong 0 DCAxy to prim. vertex (cm);entries", {HistType::kTH1F, {{100, -1., 1.}}}},
     {"hd0Prong1", "2-prong candidates;prong 1 DCAxy to prim. vertex (cm);entries", {HistType::kTH1F, {{100, -1., 1.}}}},
     {"hd0d0", "2-prong candidates;product of DCAxy to prim. vertex (cm^{2});entries", {HistType::kTH1F, {{500, -1., 1.}}}},
     {"hCTS", "2-prong candidates;cos #it{#theta}* (D^{0});entries", {HistType::kTH1F, {{110, -1.1, 1.1}}}},
     {"hCt", "2-prong candidates;proper lifetime (D^{0}) * #it{c} (cm);entries", {HistType::kTH1F, {{120, -20., 100.}}}},
     {"hCPA", "2-prong candidates;cosine of pointing angle;entries", {HistType::kTH1F, {{110, -1.1, 1.1}}}},
     {"hEta", "2-prong candidates;candidate #it{#eta};entries", {HistType::kTH1F, {{100, -2., 2.}}}},
     {"hselectionstatus", "2-prong candidates;selection status;entries", {HistType::kTH1F, {{5, -0.5, 4.5}}}},
     {"hImpParErr", "2-prong candidates;impact parameter error (cm);entries", {HistType::kTH1F, {{100, -1., 1.}}}},
     {"hDecLenErr", "2-prong candidates;decay length error (cm);entries", {HistType::kTH1F, {{100, 0., 1.}}}},
     {"hDecLenXYErr", "2-prong candidates;decay length xy error (cm);entries", {HistType::kTH1F, {{100, 0., 1.}}}}}};

  Configurable<int> d_selectionFlagD0{"d_selectionFlagD0", 1, "Selection Flag for D0"};
  Configurable<int> d_selectionFlagD0bar{"d_selectionFlagD0bar", 1, "Selection Flag for D0bar"};
  Configurable<double> cutEtaCandMax{"cutEtaCandMax", -1., "max. cand. pseudorapidity"};

  Filter filterSelectCandidates = (aod::hf_selcandidate_d0::isSelD0 >= d_selectionFlagD0 || aod::hf_selcandidate_d0::isSelD0bar >= d_selectionFlagD0bar);

  void process(soa::Filtered<soa::Join<aod::HfCandProng2, aod::HFSelD0Candidate>> const& candidates)
  {
    for (auto& candidate : candidates) {
      if (cutEtaCandMax > 0. && std::abs(candidate.eta()) > cutEtaCandMax) {
        continue;
      }
      if (candidate.isSelD0() >= d_selectionFlagD0) {
        registry.get<TH1>("hmass")->Fill(InvMassD0(candidate));
      }
      if (candidate.isSelD0bar() >= d_selectionFlagD0bar) {
        registry.get<TH1>("hmass")->Fill(InvMassD0bar(candidate));
      }
      registry.get<TH1>("hptcand")->Fill(candidate.pt());
      registry.get<TH1>("hptprong0")->Fill(candidate.ptProng0());
      registry.get<TH1>("hptprong1")->Fill(candidate.ptProng1());
      registry.get<TH1>("hdeclength")->Fill(candidate.decayLength());
      registry.get<TH1>("hdeclengthxy")->Fill(candidate.decayLengthXY());
      registry.get<TH1>("hd0Prong0")->Fill(candidate.impactParameter0());
      registry.get<TH1>("hd0Prong1")->Fill(candidate.impactParameter1());
      registry.get<TH1>("hd0d0")->Fill(candidate.impactParameterProduct());
      registry.get<TH1>("hCTS")->Fill(CosThetaStarD0(candidate));
      registry.get<TH1>("hCt")->Fill(CtD0(candidate));
      registry.get<TH1>("hCPA")->Fill(candidate.cpa());
      registry.get<TH1>("hEta")->Fill(candidate.eta());
      registry.get<TH1>("hselectionstatus")->Fill(candidate.isSelD0() + (candidate.isSelD0bar() * 2));
      registry.get<TH1>("hImpParErr")->Fill(candidate.errorImpactParameter0());
      registry.get<TH1>("hImpParErr")->Fill(candidate.errorImpactParameter1());
      registry.get<TH1>("hDecLenErr")->Fill(candidate.errorDecayLength());
      registry.get<TH1>("hDecLenXYErr")->Fill(candidate.errorDecayLengthXY());
    }
  }
};

/// Fills MC histograms.
struct TaskD0MC {
  HistogramRegistry registry{
    "registry",
    {{"hPtRecSig", "2-prong candidates (rec. matched);#it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hPtRecBg", "2-prong candidates (rec. unmatched);#it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hPtGen", "2-prong candidates (gen. matched);#it{p}_{T} (GeV/#it{c});entries", {HistType::kTH1F, {{100, 0., 10.}}}},
     {"hCPARecSig", "2-prong candidates (rec. matched);cosine of pointing angle;entries", {HistType::kTH1F, {{110, -1.1, 1.1}}}},
     {"hCPARecBg", "2-prong candidates (rec. unmatched);cosine of pointing angle;entries", {HistType::kTH1F, {{110, -1.1, 1.1}}}}}};

  Configurable<int> d_selectionFlagD0{"d_selectionFlagD0", 1, "Selection Flag for D0"};
  Configurable<int> d_selectionFlagD0bar{"d_selectionFlagD0bar", 1, "Selection Flag for D0bar"};
  Configurable<double> cutEtaCandMax{"cutEtaCandMax", -1., "max. cand. pseudorapidity"};

  Filter filterSelectCandidates = (aod::hf_selcandidate_d0::isSelD0 >= d_selectionFlagD0 || aod::hf_selcandidate_d0::isSelD0bar >= d_selectionFlagD0bar);

  void process(soa::Filtered<soa::Join<aod::HfCandProng2, aod::HFSelD0Candidate, aod::HfCandProng2MCRec>> const& candidates,
               soa::Join<aod::McParticles, aod::HfCandProng2MCGen> const& particlesMC)
  {
    // MC rec.
    for (auto& candidate : candidates) {
      if (cutEtaCandMax > 0. && std::abs(candidate.eta()) > cutEtaCandMax) {
        continue;
      }
      if (candidate.flagMCMatchRec() == 1) {
        registry.get<TH1>("hPtRecSig")->Fill(candidate.pt());
        registry.get<TH1>("hCPARecSig")->Fill(candidate.cpa());
      } else {
        registry.get<TH1>("hPtRecBg")->Fill(candidate.pt());
        registry.get<TH1>("hCPARecBg")->Fill(candidate.cpa());
      }
    }
    // MC gen.
    for (auto& particle : particlesMC) {
      if (cutEtaCandMax > 0. && std::abs(particle.eta()) > cutEtaCandMax) {
        continue;
      }
      if (particle.flagMCMatchGen() == 1) {
        registry.get<TH1>("hPtGen")->Fill(particle.pt());
      }
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  WorkflowSpec workflow{
    adaptAnalysisTask<TaskD0>("hf-task-d0")};
  const bool doMC = cfgc.options().get<bool>("doMC");
  if (doMC) {
    workflow.push_back(adaptAnalysisTask<TaskD0MC>("hf-task-d0-mc"));
  }
  return workflow;
}
