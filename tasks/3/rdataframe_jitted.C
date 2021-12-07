double rdataframe_jitted(const char * f, int ncores) {
    if ( ncores > 1 ) {
        ROOT::EnableImplicitMT(ncores);
    }
    ROOT::RDataFrame df("Events", f);
    auto h = df.Define("goodJet_pt", "Jet_pt[abs(Jet_eta) < 1.0]")
               .Histo1D({"", ";Jet p_{T} (GeV);N_{Events}", 100, 15, 60}, "goodJet_pt");

    return h->Integral();
}
