double rdataframe_jitted(const char * f, int ncores) {
    if ( ncores > 1 ) {
        ROOT::EnableImplicitMT(ncores);
    }
    ROOT::RDataFrame df("Events", f);
    auto h = df.Histo1D({"", ";Jet p_{T} (GeV);N_{Events}", 100, 15, 60}, "Jet_pt");

    return h->Integral();
}
