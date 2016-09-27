

int parseIn(int argc, char* argv[]);
void read_routing(char* routingFile);// read routing file, should be called after deleting paths
void check_routing(char* routingFilePost);
void check_shared_inputs();
bool compare_routing();
void update_cascaded_list(); // adds cascaded paths to the list of cascadedPaths at every LUT.
