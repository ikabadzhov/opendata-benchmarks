#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "Math/Vector4D.h"
#include "TStopwatch.h"
#include <fstream>

template <typename T> using Vec = const ROOT::RVec<T>&;
using FourVector = ROOT::Math::PtEtaPhiMVector;
using ROOT::Math::XYZTVector;

double query1(const char * f);
double query2(const char * f);
double query3(const char * f);
double query4(const char * f);
double query5(const char * f);
double query6(const char * f);
double query7(const char * f);
double query8(const char * f);

void jitted(int ncores, const char * f, int query){
    if ( ncores > 1 ) { 
        ROOT::EnableImplicitMT(ncores); 
    }

    TStopwatch StopWatch;
    StopWatch.Reset();
    StopWatch.Start();

    switch(query){
    case 1:
        query1(f);
        break;
    case 2:
        query2(f);
        break;
    case 3:
        query3(f);
        break;
    case 4:
        query4(f);
        break;
    case 5:
        query5(f);
        break;
    case 6:
        query6(f);
        break;
    case 7:
        query7(f);
        break;
    case 8:
        query8(f);
        break;
    default:
        std::cout << "Invalid query" << std::endl;
    }

    StopWatch.Stop();
    std::ofstream outf;
    outf.open("reports/LOG7.txt", std::ios::app);
    int nf = 1;
    if (((std::string)f).find('*') != std::string::npos) nf = 10;
    outf << "JIT:: Q:" << query << ", C:" << ncores
         << ", T:" <<  StopWatch.RealTime()
         <<", F:" << nf << std::endl;

    return;
}

double query1(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Histo1D({"", ";MET (GeV);N_{Events}", 100, 0, 200}, "MET_pt");
    
    return h->Integral();
}

double query2(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Histo1D({"", ";Jet p_{T} (GeV);N_{Events}", 100, 15, 60}, "Jet_pt");

    return h->Integral();
}

double query3(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Define("goodJet_pt", "Jet_pt[abs(Jet_eta) < 1.0]")
               .Histo1D({"", ";Jet p_{T} (GeV);N_{Events}", 100, 15, 60}, "goodJet_pt");

    return h->Integral();
}

double query4(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Filter("Sum(Jet_pt > 40) > 1", "More than one jet with pt > 40")
               .Histo1D({"", ";MET (GeV);N_{Events}", 100, 0, 200}, "MET_pt");
    
    return h->Integral();
}



auto compute_dimuon_masses(Vec<float> pt, Vec<float> eta, Vec<float> phi, Vec<float> mass, Vec<int> charge)
{
    ROOT::RVec<float> masses;
    const auto c = ROOT::VecOps::Combinations(pt, 2);
    for (auto i = 0; i < c[0].size(); i++) {
        const auto i1 = c[0][i];
        const auto i2 = c[1][i];
        if (charge[i1] == charge[i2]) continue;
        const FourVector p1(pt[i1], eta[i1], phi[i1], mass[i1]);
        const FourVector p2(pt[i2], eta[i2], phi[i2], mass[i2]);
        masses.push_back((p1 + p2).mass());
    }
    return masses;
};

double query5(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Filter("nMuon >= 2", "At least two muons")
               .Define("Dimuon_mass", compute_dimuon_masses, {"Muon_pt", "Muon_eta", "Muon_phi", "Muon_mass", "Muon_charge"})
               .Filter("Sum(Dimuon_mass > 60 && Dimuon_mass < 120) > 0",
                       "At least one dimuon system with mass in range [60, 120]")
               .Histo1D({"", ";MET (GeV);N_{Events}", 100, 0, 200}, "MET_pt");

    return h->Integral();
}

ROOT::RVec<std::size_t> find_trijet(Vec<XYZTVector> jets) {
  const auto c = ROOT::VecOps::Combinations(jets, 3);

  float distance = 1e9;
  const auto top_mass = 172.5;
  std::size_t idx = 0;
  for (auto i = 0; i < c[0].size(); i++) {
    auto p1 = jets[c[0][i]];
    auto p2 = jets[c[1][i]];
    auto p3 = jets[c[2][i]];
    const auto tmp_mass = (p1 + p2 + p3).mass();
    const auto tmp_distance = std::abs(tmp_mass - top_mass);
    if (tmp_distance < distance) {
      distance = tmp_distance;
      idx = i;
    }
  }

  return {c[0][idx], c[1][idx], c[2][idx]};
}


float trijet_pt(Vec<float> pt, Vec<float> eta, Vec<float> phi, Vec<float> mass, Vec<std::size_t> idx)
{
    auto p1 = ROOT::Math::PtEtaPhiMVector(pt[idx[0]], eta[idx[0]], phi[idx[0]], mass[idx[0]]);
    auto p2 = ROOT::Math::PtEtaPhiMVector(pt[idx[1]], eta[idx[1]], phi[idx[1]], mass[idx[1]]);
    auto p3 = ROOT::Math::PtEtaPhiMVector(pt[idx[2]], eta[idx[2]], phi[idx[2]], mass[idx[2]]);
    return (p1 + p2 + p3).pt();
}


double query6(const char * f) {
    using ROOT::Math::PtEtaPhiMVector;
    using ROOT::VecOps::Construct;

    ROOT::RDataFrame df("Events", f);
    auto df2 = df.Filter("nJet >= 3", "At least three jets")
                 .Define("JetXYZT", [](Vec<float> pt, Vec<float> eta, Vec<float> phi, Vec<float> m) {
                              return Construct<XYZTVector>(Construct<PtEtaPhiMVector>(pt, eta, phi, m));},
                         {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass"})
                 .Define("Trijet_idx", find_trijet, {"JetXYZT"});
    auto h1 = df2.Define("Trijet_pt", trijet_pt, {"Jet_pt", "Jet_eta", "Jet_phi", "Jet_mass", "Trijet_idx"})
                 .Histo1D({"", ";Trijet pt (GeV);N_{Events}", 100, 15, 40}, "Trijet_pt");
    auto h2 = df2.Define("Trijet_leadingBtag", "Max(Take(Jet_btag, Trijet_idx))")
                 .Histo1D({"", ";Trijet leading b-tag;N_{Events}", 100, 0, 1}, "Trijet_leadingBtag");

    return h1->Integral() + h2->Integral();
}

ROOT::RVec<int> find_isolated_jets(Vec<float> eta1, Vec<float> phi1, Vec<float> pt2, Vec<float> eta2, Vec<float> phi2)
{
    ROOT::RVec<int> mask(eta1.size(), 1);
    if (eta2.size() == 0) {
        return mask;
    }

    const auto ptcut = pt2 > 10;
    const auto eta2_ptcut = eta2[ptcut];
    const auto phi2_ptcut = phi2[ptcut];
    if (eta2_ptcut.size() == 0) {
        return mask;
    }

    const auto c = ROOT::VecOps::Combinations(eta1, eta2_ptcut);
    for (auto i = 0; i < c[0].size(); i++) {
        const auto i1 = c[0][i];
        const auto i2 = c[1][i];
        const auto dr = ROOT::VecOps::DeltaR(eta1[i1], eta2_ptcut[i2], phi1[i1], phi2_ptcut[i2]);
        if (dr < 0.4) mask[i1] = 0;
    }
    return mask;
}

double query7(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Filter("nJet > 0", "At least one jet")
               .Define("goodJet_ptcut", "Jet_pt > 30")
               .Define("goodJet_antiMuon", find_isolated_jets, {"Jet_eta", "Jet_phi", "Muon_pt", "Muon_eta", "Muon_phi"})
               .Define("goodJet_antiElectron", find_isolated_jets, {"Jet_eta", "Jet_phi", "Electron_pt", "Electron_eta", "Electron_phi"})
               .Define("goodJet", "goodJet_ptcut && goodJet_antiMuon && goodJet_antiElectron")
               .Filter("Sum(goodJet) > 0")
               .Define("goodJet_sumPt", "Sum(Jet_pt[goodJet])")
               .Histo1D({"", ";Jet p_{T} sum (GeV);N_{Events}", 100, 15, 200}, "goodJet_sumPt");

    return h->Integral();
}

unsigned int additional_lepton_idx(Vec<float> pt, Vec<float> eta, Vec<float> phi, Vec<float> mass, Vec<int> charge, Vec<int> flavour)
{
    const auto c = Combinations(pt, 2);
    unsigned int lep_idx = -999;
    float best_mass = 99999;
    int best_i1 = -1;
    int best_i2 = -1;
    const auto z_mass = 91.2;
    const auto make_p4 = [&](std::size_t idx) {
        return ROOT::Math::PtEtaPhiMVector(pt[idx], eta[idx], phi[idx], mass[idx]);
    };

    for (auto i = 0; i < c[0].size(); i++) {
        const auto i1 = c[0][i];
        const auto i2 = c[1][i];
        if (charge[i1] == charge[i2]) continue;
        if (flavour[i1] != flavour[i2]) continue;
        const auto p1 = make_p4(i1);
        const auto p2 = make_p4(i2);
        const auto tmp_mass = (p1 + p2).mass();
        if (std::abs(tmp_mass - z_mass) < std::abs(best_mass - z_mass)) {
            best_mass = tmp_mass;
            best_i1 = i1;
            best_i2 = i2;
        }
    }

    if (best_i1 == -1) return lep_idx;

    float max_pt = -999;
    for (auto i = 0; i < pt.size(); i++) {
        if (i != best_i1 && i != best_i2 && pt[i] > max_pt) {
            max_pt = pt[i];
            lep_idx = i;
        }
    }

    return lep_idx;
}

double query8(const char * f) {
    ROOT::RDataFrame df("Events", f);
    auto h = df.Filter("nElectron + nMuon > 2", "At least three leptons")
               .Define("Lepton_pt", "Concatenate(Muon_pt, Electron_pt)")
               .Define("Lepton_eta", "Concatenate(Muon_eta, Electron_eta)")
               .Define("Lepton_phi", "Concatenate(Muon_phi, Electron_phi)")
               .Define("Lepton_mass", "Concatenate(Muon_mass, Electron_mass)")
               .Define("Lepton_charge", "Concatenate(Muon_charge, Electron_charge)")
               .Define("Lepton_flavour", "Concatenate(ROOT::RVec<int>(nMuon, 0), ROOT::RVec<int>(nElectron, 1))")
               .Define("AdditionalLepton_idx", additional_lepton_idx,
                       {"Lepton_pt", "Lepton_eta", "Lepton_phi", "Lepton_mass", "Lepton_charge", "Lepton_flavour"})
               .Filter("AdditionalLepton_idx != -999", "No valid lepton pair found.")
               .Define("TransverseMass",
                       "sqrt(2.0 * Lepton_pt[AdditionalLepton_idx] * MET_pt * (1.0 - cos(ROOT::VecOps::DeltaPhi(MET_phi, Lepton_phi[AdditionalLepton_idx]))))")
               .Histo1D({"", ";Transverse mass (GeV);N_{Events}", 100, 0, 200}, "TransverseMass");

    return h->Integral();
}








