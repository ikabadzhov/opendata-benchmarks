double rdataframe_jitted(const char * f, int ncores) {
    if ( ncores > 1 ) { 
        ROOT::EnableImplicitMT(ncores); 
    }
    ROOT::RDataFrame df("Events", f);
    auto h = df.Histo1D({"", ";MET (GeV);N_{Events}", 100, 0, 200}, "MET_pt");
    
    return h->Integral();
}

