

// returns 0 if it fails
int parseOptions(int argc, char* argv[],
	std::string & outputName,
	bool &  MCsimulation,
	bool &  readMCsamplesFile,
	int &  calibBitstreams,
	bool &  ILPform,
	bool &  optPerXBitstreams,
	double &  var,
	double &  yld,
	int &  MCsamplesCount,
	std::string & MCsimFileName,
	std::vector<BRAM> & memories);

void helpMessage(); // prints help message;
bool isInt(std::string input); // return true if input string is an int
int parseIn(std::string metaFileName);
int read_routing(std::string routingFile);// read routing file, should be called after deleting paths
void check_routing(char* routingFilePost);
void check_shared_inputs();
bool compare_routing();
void update_cascaded_list(); // adds cascaded paths to the list of cascadedPaths at every LUT.
void print_path(int path);
int read_timing_edges(char* edgesFile);

