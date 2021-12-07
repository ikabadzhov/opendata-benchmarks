double rdataframe_jitted(const char * f, int ncores) {
    if ( ncores > 1 ) {
        ROOT::EnableImplicitMT(ncores);
    }
    ROOT::RDataFrame df("Events", f);
    auto h = df.Filter("Sum(Jet_pt > 40) > 1", "More than one jet with pt > 40")
               .Histo1D({"", ";MET (GeV);N_{Events}", 100, 0, 200}, "MET_pt");
    
    return h->Integral();
}
