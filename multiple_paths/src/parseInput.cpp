
#include "globalVar.h"
#include "parseInput.h"
#include "util.h"

#ifdef CycloneIV

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
	std::string & MCsimFileName){
	////////////////////////////////////////////////
	// Possible inputs file that iFRoC could take///
	////////////////////////////////////////////////

	// Musts
	std::string metaFileName = "";
	std::string metaRoutingFileName = "";

	// not required
	std::string metaEdgeFileName = "";
	MCsimFileName = "";
	std::string checkRoutingFileName = ""; // file name of the rcf generated from the calibration bitstream, we use it to double check that our constraint were honored
	outputName = "unkown";

	// default option values
	MCsimulation = false; // default no MCsim
	readMCsamplesFile = false; // default no MCsim
	calibBitstreams = 1; // 1 calibration bitstream
	ILPform = false; // no ILP
	optPerXBitstreams = false; // default mazimize per 1 bitstream
	var = 0.05;
	yld = 2;
	MCsamplesCount = 10000;


	// vector of strings to store all command line arguments
	std::vector<std::string> allCommandArguments;
	allCommandArguments.resize(argc - 1);

	// copy from argv to our vector of strings
	for (int i = 0; i < (int)allCommandArguments.size(); i++)
		allCommandArguments[i] = argv[i + 1];


	std::string option = "";
	for (int i = 0; i < (int)allCommandArguments.size(); i++)
	{
		if (allCommandArguments[i][0] == '-') // if its an option, lets check if we are expecting an option or not
		{
			if (option != "") // we are not expecting an option, oops wrong input
			{
				if (option == "-h" || option == "--help") // meta file name
				{
					helpMessage();
					return 0;
				}
				std::cout << "option " << option << " has no value." << std::endl;
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
			else // we are actually expecting an option, so all good.
			{
				option = allCommandArguments[i];
			}
			continue;

		}

		if (option == "")
		{
			std::cout << "Wrong input format. " << allCommandArguments[i] << " was not preceeded by an option." << std::endl;
			std::cout << "Terminating program...." << std::endl;
			return 0;
		}

		// we have an option set and allcommandarg[i] is a value

		if (option == "-fp" || option == "--meta") // meta file name
		{
			std::cout << "Meta file name is: " << allCommandArguments[i] << std::endl;
			metaFileName = allCommandArguments[i];
		}
		else if (option == "-fr" || option == "--metaRouting") // meta routing file name
		{
			std::cout << "MetaRouting routing file name is: " << allCommandArguments[i] << std::endl;
			metaRoutingFileName = allCommandArguments[i];
		}
		else if (option == "-fe" || option == "--metaEdge") // meta edge file name
		{
			std::cout << "MetaEdge used for MC file name is " << allCommandArguments[i] << std::endl;
			metaEdgeFileName = allCommandArguments[i];
			// meta edge presence means that we are doing MC sim
			MCsimulation = true;
		}
		else if (option == "-fmc" || option == "--MCfile") // meta edge file name
		{
			std::cout << "MC samples file name is " << allCommandArguments[i] << std::endl;
			MCsimFileName = allCommandArguments[i];
			MCsimulation = true; 
			ILPform = true; // MC sim omplies ILP form
			readMCsamplesFile = true;
		}
		else if (option == "-o" || option == "--outputName") // output
		{
			std::cout << "Output file preceeded by  " << allCommandArguments[i] << std::endl;
			outputName = allCommandArguments[i];
		}
		else if (option == "-n" || option == "--bitstreams") // number of bit streams
		{
			std::cout << "test number of bit streams is " << allCommandArguments[i] << std::endl;
			if(isInt(allCommandArguments[i]))
				calibBitstreams = stoi(allCommandArguments[i]);
			else
			{
				std::cout << "ahaa but " << allCommandArguments[i] << " is not really an integer !!" << std::endl;
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
		}
		else if (option == "-v" || option == "--var") // var for MC sim, should be entered as a pecentage of the mean
		{
			std::cout << "var is: " << allCommandArguments[i] << std::endl;
			if (isInt(allCommandArguments[i]))
			{
				var = (stoi(allCommandArguments[i])*1.0) / 100.0;
				MCsimulation = true;
				ILPform = true; // MC sim omplies ILP form
			}
			else
			{
				std::cout << "ahaa but " << allCommandArguments[i] << " is not really an integer !!" << std::endl;
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
			
		}
		else if (option == "-y" || option == "--yld") // yld, how Quartus interprets the "worst-case delay"
		{
			std::cout << "yld is " << allCommandArguments[i] << std::endl;
			if (isInt(allCommandArguments[i]))
			{
				yld = (stoi(allCommandArguments[i])*1.0) ;
				MCsimulation = true;
				ILPform = true; // MC sim omplies ILP form
			}
			else
			{
				std::cout << "ahaa but " << allCommandArguments[i] << " is not really an integer !!" << std::endl;
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
		}
		else if (option == "-mc" || option == "--MCsamples") // number of mc samples
		{
			std::cout << "Number of MCsamples is " << allCommandArguments[i] << std::endl;
			if (isInt(allCommandArguments[i]))
			{
				MCsamplesCount = (stoi(allCommandArguments[i]));
				MCsimulation = true;
				ILPform = true; // MC sim omplies ILP form
			}
			else
			{
				std::cout << "ahaa but " << allCommandArguments[i] << " is not really an integer !!" << std::endl;
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
		}
		else if (option == "-i" || option == "--ILP") // 0 means per bit stream anyhing else means per X bit stream
		{
			std::cout << "test ILP is " << allCommandArguments[i] << std::endl;
			ILPform = true;
			if (allCommandArguments[i][0] == '0')
			{
				std::cout << "ILP optimization per 1 bitstream" << std::endl;
			}
			else
			{
				optPerXBitstreams = true;
				std::cout << "ILP optimization per X bitstreams" << std::endl;
			}
		}
		else
		{
			std::cout << "option " << option << " is not supported." << std::endl;
			std::cout << "Terminating program...." << std::endl;
			return 0;
		}

		option = "";
	}

	// if we finished the loop and option is not empty then sthis option was not set
	if (option != "")
	{
		if (option == "-h" || option == "--help") // meta file name
		{
			helpMessage();
			return 0;
		}
		std::cout << "option " << option << " has no value." << std::endl;
		std::cout << "Terminating program...." << std::endl;
		return 0;
	}

	if (metaFileName == "" || metaRoutingFileName == "") // no inputs files were given so we can't run iFRoC
	{
		std::cout << "Not enough input files were given. iFRoC needs a meta file and a meta routing file." << std::endl;
		std::cout << "Terminating program...." << std::endl;
		return 0;

	}

	if (metaEdgeFileName == "" && MCsimulation) // if the user asked for MC simulation but did not give a meta edge file then we will not do MC
	{
		MCsimulation = false;
		std::cout << std::endl << "Mc simulation was requested but no timing edge file was given. iFRoC will ignoe MC request." << std::endl;

	}

	// parse the meta file

	if (parseIn(metaFileName) == 0)
	{
		std::cout << "Terminating program...." << std::endl;
		return 0;
	}

	// parse the routing file
	if (read_routing(metaRoutingFileName) == 1)
	{
		if (MCsimulation)
		{
			// parse timing edge file for MC
			if (read_timing_edges(argv[5]) == 1)
				return 1;
			else
			{
				std::cout << "Terminating program...." << std::endl;
				return 0;
			}
				
		}
	}

	else
	{
		std::cout << "Terminating program...." << std::endl;
		return 0;
	}


	return 1;
}

bool isInt(std::string input) // return true if input string is an int
{

	for (int i = 0; i <(int)input.size(); i++)
	{
		if (!isdigit(input[i]))// not a digit
			return false;
	}

	return true;
}

void helpMessage() // prints help message
{
	std::cout << "Welcome to iFRoC :)" << std::endl;
	std::cout << "Let's calibirate your application to your specific chip!!" << std::endl;
	std::cout << "iFRoC can be run in multiple modes, so pay attention to the following messages:" << std::endl << std::endl;

	std::cout << "USAGE: " << std::endl;
	std::cout << "\t iFRoC (-fp | -meta <metaFileName>) (-fr | -metaRouting <metaRoutingFileName>) [options <value>]" << std::endl << std::endl;

	std::cout << "Options: " << std::endl;
	std::cout << "\t -h   --help       \tShow help(this) screen" << std::endl;
	std::cout << "\t -fp  --meta       \tMeta file name need to run out parser on the delay report." << std::endl;
	std::cout << "\t -fr  --metaRouting\tMeta routing file name need to run out parser on the delay report." << std::endl;
	std::cout << "\t -fe  --metaEdge   \tMeta edge file used for MC simulation, need to run our edgeParser." << std::endl;
	std::cout << "\t -fmc --MCfile     \tFile with a previous run of MC simulation, used to save time." << std::endl;
	std::cout << "\t -o   --outputName \tApplication name, string will be appended to all output file name" << std::endl;
	std::cout << "\t -n   --bitstreams \tNumber of desired calibration bitstreams" << std::endl;
	std::cout << "\t -v   --var        \tExpected delay variation from mean, integer value as a percentage of mean.[defualt 5%]" << std::endl;
	std::cout << "\t -y   --yld        \tHow does Quartus build their timing models.[default mu + 2 sigma]." << std::endl;
	std::cout << "\t -mc  --MCsamples \tNUmber of MC samples" << std::endl;
	std::cout << "\t -i   --ILP        \tILP mode, 0 is optimize per 1 but stream other wise optimize over all calibration bitstream." << std::endl << std::endl << std::endl;


	std::cout << "Good luck with running iFRoC." << std::endl;




}

int parseIn(std::string metaFileName)
{

	std::ifstream metaData(metaFileName);
	if (!metaData)
	{
		std::cout << "Can not find file" << metaFileName << "  Terminating.... " << std::endl;
		return 0;
	}
	std::string line;
	std::vector<Path_node> tempPath;
	int path, node;
	path = 0;
	node = 0;
	
	pathSlack.resize(0);
	pathSlack.push_back(0);

	pathClockSkew.resize(0);
	pathClockSkew.push_back(0);

	pathClockRelation.resize(0);
	pathClockRelation.push_back(0);

	int i, counter, x, y, z, pIn, pOut, edgeType;
	int maxOverlap = 0;
	int  index1 = -1;
	double tempSlack;
	bool slackReached = false;
	bool invertingSignal;



	while (std::getline(metaData, line))
	{
		if (line[0] == 'P')
		{
			//if (path > 0)
			//{
			paths.push_back(tempPath);
			tempPath.clear();
			//}
			path++;
		//	if (path == 44)
		//		std::cout << "debugging" << std::endl;
			node = 0;
			// add slack value to pathSlack
			slackReached = false;
			for (i = 0; i < (int)line.size(); i++)
			{
				if (line[i] == ':')
					slackReached = true;
				if (!slackReached)
					continue;
				if (line[i] == '-' || isdigit(line[i]))
					break;
			}

			tempSlack = stod(line.substr(i, line.size() - i));
			pathSlack.push_back(tempSlack);

			/// I know that there must be a next line with clock relation ship 
			// so
			assert(std::getline(metaData, line));
			pathClockRelation.push_back(std::stod(line));
				

			/// I know that htere must be anext line with clock skew info 
			// so
			assert(std::getline(metaData, line));

			pathClockSkew.push_back(std::stod(line));
				


			continue;
		}
		counter = 0;
		for (i = 0; i < (int)line.size(); i++)
		{
			if (line[i] == '_')
				counter++;
			if (counter == 3)
			{
				index1 = i + 2;
				break;
			}
		}

		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		x = stoi(line.substr(index1, i - index1));
		index1 = i + 2;
		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		y = stoi(line.substr(index1, i - index1));

		index1 = i + 2;
		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		z = stoi(line.substr(index1, i - index1));



		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		if (line[0] == 'F')
		{
			pIn = FFd;
			pOut = FFq;
		}
		else
		{
			pIn = line[4] - '0';
			pOut = line[6] - '0';
		}
		// inverting behanviour
		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		if (line[0] == 'I')
		{
			invertingSignal = true;
		}
		else
		{
			invertingSignal = false;
		}

		// read edge type
		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		edgeType = 0;

		if (line[0] == 'R')
			edgeType += 2;

		if (line[1] == 'R')
			edgeType++;




		// have x and y and z stored
		fpgaLogic[x][y][z].add_node(path, node, pIn, pOut);
		// handle FF with sdata or data
		if (node!=0 && z%LUTFreq==1) // this is a FF and not a source, it is a sink FF
		{
			int tempFFMode;
			assert(node - 1 == (int)tempPath.size() - 1);
			if (tempPath[node - 1].x == x && tempPath[node - 1].y == y && (tempPath[node - 1].z == z - 1)) // means its fed from the LUT connected to the D input
			{
				tempFFMode = dInput;
			}
			else
			{
				tempFFMode = sData;
			}
			if (fpgaLogic[x][y][z].FFMode == -1) // modes is still undefined, as this FF was not used b4
			{
			//	if (tempFFMode == sData)
			//		std::cout << "new sdata FF ";
				fpgaLogic[x][y][z].FFMode = tempFFMode;
			}
			else
			{
				if (tempFFMode != fpgaLogic[x][y][z].FFMode)
				{
					std::cout << x << " " << y << " " << z;
				}
				assert(tempFFMode == fpgaLogic[x][y][z].FFMode); // we only support one mode of FF, either input through d or through sdata, but not both.
			}
		}


		assert(x >= 0 && y >= 0 && z >= 0);

		// if its the source reg then add 4 to the dedge typ because source reg has the same Tco delay  
		if (tempPath.size() == 0)
			edgeType += 4;

		tempPath.push_back(Path_node(x, y, z, pIn, pOut, invertingSignal,edgeType));
		for (i = 0; i<(int)fpgaLogic[x][y][z].nodes.size(); i++) // pass through all path nodes that uses this logic element
		{
			if (fpgaLogic[x][y][z].nodes[i].path != path) // check if it is used by another path through the same portIn and portOut
			{
				if (pIn == paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn&&pOut == paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portOut) // check if the other path node uses the same input pin and output pin, which means its redundant, we only check for the first match as this is the most critical path using this node and is considered the master path usign this node
				{
					tempPath[tempPath.size() - 1].redun = true;
					tempPath[tempPath.size() - 1].eqPath = fpgaLogic[x][y][z].nodes[i].path;
					tempPath[tempPath.size() - 1].eqNode = fpgaLogic[x][y][z].nodes[i].node;
					break;
				}

			}
		}

		if (node != 0) // skip source registers
			if (fpgaLogic[x][y][z].get_utilization()>maxOverlap)
				maxOverlap = fpgaLogic[x][y][z].get_utilization();
		node++;
	}
	// push last path to list of paths
	paths.push_back(tempPath);
	tempPath.clear();

	update_cascaded_list();

	

	return 1;
}



#endif




void update_cascaded_list() // go through all LEs and adds cscaded paths to every LUT feeding a cascaded FF
{
	int i, j, k;
	int feederPath, feederNode;

	int feederX, feederY, feederZ;
	
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (k%LUTFreq == 0) // get FFs only
					continue;
				if (fpgaLogic[i][j][k].utilization == 0) // if not used then skip it
					continue;

				if (!is_cascaded_reg(i, j, k)) // if not cacsaded reg then skip it
					continue;
				// if it is a reg and used and cascaded, then get feeder node [it is only one feeder]
				if (!get_feeder(i, j, k, feederPath, feederNode)) // get the feedrer to this LUT, when this function returns false, this means that this register is not a sink. It is only a source so it doesnt have any feeders
					continue;

				feederX = paths[feederPath][feederNode].x;
				feederY = paths[feederPath][feederNode].y;
				feederZ = paths[feederPath][feederNode].z;

				for (int x = 0; x <(int)fpgaLogic[i][j][k].nodes.size(); x++) // loop across all nodes using FF i,j,k
				{
					if (fpgaLogic[i][j][k].nodes[x].node == 0) // this node is a source at FF i,j,k so it can't be tested with any paths in the feeder to FF i,j,k
					{
						if (paths[fpgaLogic[i][j][k].nodes[x].path].back().x == i && paths[fpgaLogic[i][j][k].nodes[x].path].back().y == j && paths[fpgaLogic[i][j][k].nodes[x].path].back().z == k) // if the sink of path (fpgaLogic[i][j][k].nodes[x].path) is the same as the source location, then this is a wraparound path. so we dont add it as acascaded path
						{
							continue;
				//			assert(false);
						}
						//fpgaLogic[66][22][30]
					//	if (feederX == 66 && feederY == 22 && feederZ == 30)
					//		std::cout << "shit" << i << " " << j << " " <<k << std::endl;

						fpgaLogic[feederX][feederY][feederZ].cascadedPaths.push_back(fpgaLogic[i][j][k].nodes[x].path); // add this path to the list of cascaded paths at LUT feederX,Y,Z.
					}

				}

			}
		}
	}

}

bool check_source(std::string target);
bool check_destination(std::string target);
bool check_signature_begin(std::string target, std::string signature);
bool check_char_exists(std::string target, char x);
std::string remove_leading_spaces(std::string target); // also removes the ';' at the end

#ifdef StratixV 
int parseIn(int argc, char* argv[])
{
	if (argc<3)
	{
		std::cout << "No Input file was given. Terminating.... /n";
		return 0;
	}
	std::ifstream metaData(argv[1]);
	if (!metaData)
	{
		std::cout << "Can not find file" << argv[1] << "  Terminating.... " << std::endl;
		return 0;
	}
	std::string line;
	std::vector<Path_node> tempPath;
	int path, node;
	path = 0;
	node = 0;
	pathSlack.resize(0);
	pathSlack.push_back(0);
	int i, counter, index1, x, y, z, pIn, pOut;
	int maxOverlap = 0;
	double tempSlack;
	bool slackReached = false;
	bool invertingSignal;



	while (std::getline(metaData, line))
	{
		if (line[0] == 'P')
		{
			//if (path > 0)
			//{
			paths.push_back(tempPath);
			tempPath.clear();
			//}
			path++;
			node = 0;
			// add slack value to pathSlack
			slackReached = false;
			for (i = 0; i < (int)line.size(); i++)
			{
				if (line[i] == ':')
					slackReached = true;
				if (!slackReached)
					continue;
				if (line[i] == '-' || isdigit(line[i]))
					break;
			}

			tempSlack = stod(line.substr(i, line.size() - i));
			pathSlack.push_back(tempSlack);
			continue;
		}
		counter = 0;
		bool MLAB = false;
		for (i = 0; i < (int)line.size(); i++)
		{
			if (line[i] == '_')
				counter++;
			if (counter == 3)
			{
				index1 = i + 2;
				break;
			}
			// check if it MLAB or LAB, other option is see in data sheet to identify which labs are mlab and which are just lab
			if (line[i]==' ')
			{
				if (line[i + 1] == 'M')
					MLAB = true;
			}
		}

		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		x = stoi(line.substr(index1, i - index1));
		index1 = i + 2;
		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		y = stoi(line.substr(index1, i - index1));

		index1 = i + 2;
		for (i = index1; i <(int)line.size(); i++)
		{
			if (!isdigit(line[i]))
				break;
		}
		z = stoi(line.substr(index1, i - index1));



		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		if (line[0] == 'F')
		{
			pIn = FFd;
			pOut = FFq;
		}
		else
		{
			pIn = line[4] - '0';
			pOut = line[6] - '0';
		}
		// inverting behanviour
		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		if (line[0] == 'I')
		{
			invertingSignal = true;
		}
		else
		{
			invertingSignal = false;
		}
		// high speed or low power, stratix V specific
		bool highS = false;
		if (!std::getline(metaData, line))
		{
			std::cout << "Incorrect file format. Terminating...." << std::endl;
			return 0;
		}
		if (line[0] == 'H')
		{
			highS = true;
		}
	
		// have x and y and z stored
		if (z%LUTFreq == 0)
		{
			assert(pIn < InputPortSize && pOut < OutputPortSize);
		}
		else
		{
			assert(pIn > InputPortSize && pOut > OutputPortSize);
		}
		fpgaLogic[x][y][z].add_node(path, node, pIn, pOut, highS, MLAB);
		tempPath.push_back(Path_node(x, y, z, pIn, pOut, invertingSignal));

		for (i = 0; i<(int)fpgaLogic[x][y][z].nodes.size(); i++) // pass through all path nodes that uses this logic element
		{
			if (fpgaLogic[x][y][z].nodes[i].path != path) // check if it is used by another path through the same portIn and portOut
			{
				if (pIn == paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portIn&&pOut == paths[fpgaLogic[x][y][z].nodes[i].path][fpgaLogic[x][y][z].nodes[i].node].portOut) // check if the other path node uses the same input pin and output pin, which means its redundant, we only check for the first match as this is the most critical path using this node and is considered the master path usign this node
				{
					tempPath[tempPath.size() - 1].redun = true;
					tempPath[tempPath.size() - 1].eqPath = fpgaLogic[x][y][z].nodes[i].path;
					tempPath[tempPath.size() - 1].eqNode = fpgaLogic[x][y][z].nodes[i].node;
					break;
				}

			}
		}

		if (node != 0) // skip source registers
			if (fpgaLogic[x][y][z].get_utilization()>maxOverlap)
				maxOverlap = fpgaLogic[x][y][z].get_utilization();
		node++;
	}
	// push last path to list of paths

	paths.push_back(tempPath);
	tempPath.clear();


	read_routing(argv[2]);

	///// check shared Inputs, adjust shared infputs for fpgalogic and note number of inputs for each ALM, stratix:specific
	check_shared_inputs();




	return 1;
}

void check_shared_inputs() {

	std::vector< Path_logic_component > feedersTop;
	std::vector< Path_logic_component > feedersBottom;
	int pathFeeder, nodeFeeder ;
	feedersTop.resize(0);
	feedersBottom.resize(0);
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k+=6) // loop across all ALMs 
			{
				feedersTop.resize(0);
				feedersBottom.resize(0);
				int totalShared = 0;
				if (fpgaLogic[i][j][k].utilization == 0 || fpgaLogic[i][j][k + LUTFreq].utilization == 0)
				{
					fpgaLogic[i][j][k].sharedInputPorts = 0; // top ALUT
					fpgaLogic[i][j][k + LUTFreq].sharedInputPorts = 0; // Bottom ALUT
					alms[i][j][k / ALUTtoALM].numOfInputs = fpgaLogic[i][j][k].usedInputPorts + fpgaLogic[i][j][k + LUTFreq].usedInputPorts;
					continue;
				}
				//////////////////// top
				for (int l = 0; l < InputPortSize; l++) // loop across input ports of the top ALUT
				{
					pathFeeder = -1;
					nodeFeeder = -1;
					if (fpgaLogic[i][j][k].inputPorts[l]) // if port l is used then check if the signal connected to this port is share with the bottom ALUT
					{

						assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // gets feeder of port l,
						// we must check if the bottom LUT has an input from the same source 
					}
					feedersTop.push_back(Path_logic_component(pathFeeder, nodeFeeder));
				}
				////////////////////// bottom
				for (int l = 0; l < InputPortSize; l++) // loop across input ports of the bottom ALUT
				{
						pathFeeder = -1;
						nodeFeeder = -1;
					if (fpgaLogic[i][j][k+LUTFreq].inputPorts[l]) // if port l is used then check if the signal connected to this port is share with the bottom ALUT
					{
						assert(get_feeder(i, j, k+ LUTFreq, l, pathFeeder, nodeFeeder)); // gets feeder of port l,		
					}
					feedersBottom.push_back(Path_logic_component(pathFeeder, nodeFeeder));
				}
				assert(feedersBottom.size() == LUTinputSize && feedersTop.size() == LUTinputSize);

				//////////////////////// now compare the feedersBottom against the feedersTop
				for (int l = 0; l < InputPortSize; l++)
				{
					for (int n = 0; n < InputPortSize; n++)
					{
						if (feedersTop[l].path < 0) // empty // this ensures that we dont assume unconneceted tports to be shared
							continue;
						if (feedersTop[l].path == feedersBottom[n].path && feedersTop[l].node == feedersBottom[n].node) // then input l from top LUT and input n from bottom LUT are fed from the same source, they could share the same ALM pin or not, must check the block INput mux used
						{

							int xFeeder = paths[feedersTop[l].path][feedersTop[l].node].x;
							int yFeeder = paths[feedersTop[l].path][feedersTop[l].node].y;
							int zFeeder = paths[feedersTop[l].path][feedersTop[l].node].z;
							std::string blockInputMuxTemp;
							bool found = false;
							bool shared = true;
							for (int routeConnectionCounter = 0; routeConnectionCounter < fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); routeConnectionCounter++) // looping through the connection
							{
								if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].destinationX == i && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].destinationY == j && (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].destinationZ == k || fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].destinationZ == k + LUTFreq)) // this is the connection used to connect to the current ALM
								{
									if (!found) // this is the first time to find a connection to the current ALM
									{
										assert(fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources.size()>0);
										blockInputMuxTemp = fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources[fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources.size() - 1];
										found = true;
									}
									else
									{
										assert(fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources.size()>0);
										if (blockInputMuxTemp != fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources[fpgaLogic[xFeeder][yFeeder][zFeeder].connections[routeConnectionCounter].usedRoutingResources.size() - 1])
										{
											shared = false;
										}
									}
								}
							}

							if (!shared)
								continue;

							totalShared++;
							fpgaLogic[i][j][k].isInputPortShared[l] = true;
							fpgaLogic[i][j][k].sharedWith[l] = n;


							fpgaLogic[i][j][k+3].isInputPortShared[n] = true;
							fpgaLogic[i][j][k+3].sharedWith[n] = l;
						}
					}
				}

				fpgaLogic[i][j][k].sharedInputPorts = totalShared; // top ALUT
				fpgaLogic[i][j][k+ LUTFreq].sharedInputPorts = totalShared; // Bottom ALUT
				alms[i][j][k / ALUTtoALM].numOfInputs = fpgaLogic[i][j][k].usedInputPorts + fpgaLogic[i][j][k + LUTFreq].usedInputPorts - totalShared;
			}
		}
	}
}
#endif 




// insert RE delay to REsDelay map
void insert_to_REsDelay(std::string tempKey, double delay, int edgeType)
{
	auto iter = REsDelay.find(tempKey);

	// 

	if (tempKey == "RELE_BUFFER:X84Y45S0I12")
		std::cout << "debuggng" << std::endl;
	if (iter == REsDelay.end()) // RE not found
	{

		std::vector<Edge_Delay > tempVector;

		Edge_Delay temp = Edge_Delay(edgeType, delay);
		tempVector.push_back(temp);

		// insert this to the map
		REsDelay.insert(std::pair<std::string, std::vector<Edge_Delay > >(tempKey, tempVector));
	}
	else // RE found
	{
		bool found = false;
		for (int i = 0; i < (int)iter->second.size(); i++)
		{
			if ((iter->second)[i].delay == delay && (iter->second)[i].type == edgeType)
			{
				found = true;
				break;
			}
		}

		if (!found) // if the delay was not found then it's a new delay
		{
			// if we found the RE but not the corresponding delay, then at there must be exactly one delay in the vecotr. The reason for that is that RE can only has 2 delays ff or rr
		//should be there	assert((iter->second).size() == 1);
			Edge_Delay temp = Edge_Delay(edgeType, delay);
			(iter->second).push_back(temp);

		}
	}

}



void adjust_REsDelay()
{
	pathREsDelta.resize(paths.size());
	// insert all zeros to the delta associated with each path
	std::fill(pathREsDelta.begin(), pathREsDelta.end(), 0.0);

	// loop over the global variable REsDelay to get average of each RE delay
	for (auto iter = REsDelay.begin(); iter != REsDelay.end(); iter++)
	{
		double avgFallDelay = 0.0;
		int fallCount = 0;
		double avgRiseDelay = 0.0;
		int riseCount = 0;

		// loop over all edge delays in this RE to get an average for fall and rise delay for each RE
		for (int i = 0; i < (int)(iter->second).size(); i++)
		{
			if ((iter->second)[i].type==0) // fall
			{
				fallCount++;
				avgFallDelay += (iter->second)[i].delay;
			}
			else
			{
				assert((iter->second)[i].type == 3); // rise
				riseCount++;
				avgRiseDelay += (iter->second)[i].delay;
			}
		}

		avgFallDelay = avgFallDelay / fallCount;
		avgRiseDelay = avgRiseDelay / riseCount;

		//  now clear this vector and only add the average delay
		// clear vec
		(iter->second).clear();
		// add fall average if exists
		if (avgFallDelay > 0)
		{
			Edge_Delay temp = Edge_Delay(0, avgFallDelay);
			(iter->second).push_back(temp);
		}
		// add rise average if exists
		if (avgRiseDelay > 0)
		{
			Edge_Delay temp = Edge_Delay(3, avgRiseDelay);
			(iter->second).push_back(temp);
		}

		// now loop over all paths using this RE to add the delta delay associated with this RE

		// get the vector of paths using this RE
		auto iter_path = REToPaths.find((iter->first));
		assert(iter_path != REToPaths.end());

		for (int i = 0; i < (int)(iter_path->second).size(); i++)
		{
			if (paths[(iter_path->second)[i].path][(iter_path->second)[i].node].edgeType < 2)// then fall edge along this RE
			{
				pathREsDelta[(iter_path->second)[i].path] += (iter_path->second)[i].delay - avgFallDelay;
			}
			else
			{
				pathREsDelta[(iter_path->second)[i].path] += (iter_path->second)[i].delay - avgRiseDelay;
			}
		}
	}

}



// read routing files, model routing structures and create REstoDelay and REstoPaths.
int read_routing(std::string routingFile)
{
	int i;
	int path = -1; 
	int node = -1;
	std::ifstream metaData(routingFile);
	if (!metaData)
	{
		std::cout << "Can not find file" << routingFile << "  Terminating.... " << std::endl;
		return 0;
	}
	Routing_connection tempConnection;
	std::string line;
	int index1;
	bool deletedPath = false;
	bool connExists;
	int count = 0;
	int sourceX = -1;
	int sourceY = -1;
	int sourceZ = -1;
	double cellREDelay = 0.0;
	while (std::getline(metaData, line))
	{
		count++;
	//	std::cout << count << " ";
	/*	if (count == 13504 )
		{
			std::cout << "debug" << std::endl;
			std::cout << (((fpgaLogic[132][56][39]).connections)[0]).usedRoutingResources[0] << std::endl;
		}

		if (count == 82)
		{
			std::cout << "debug" << std::endl;
		}*/



		if (line[0] == '*') // either a source or destination
		{
			if (line[1] == 'S') // source
			{
				tempConnection.clear(); // clear connection
			//	tempConnection.deleted = false;
				index1 = 11; // skip the "*PATH"
				for (i = index1; i < (int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				path = stoi(line.substr(index1, i - index1));
				index1 = i + 4; // skip the "NODE"
				for (i = index1; i < (int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				node = stoi(line.substr(index1, i - index1));
				if (paths[path][0].deleted)
					deletedPath = true;
				else
					deletedPath = false;

				sourceX = paths[path][node].x;
				sourceY = paths[path][node].y;
				sourceZ = paths[path][node].z;

		//		if (sourceX == 132 && sourceY == 56 && sourceZ == 39)
		//			std::cout << "debug" << std::endl;

				tempConnection.sourcePort = paths[path][node].portOut; // assign the source of this connection to the output port of the node in question

				// read the RE delay of this cell, we will store it int the first RE used by this connection
				// read delay
				assert(getline(metaData, line));
				cellREDelay = std::stod(line);

				// read type of edge but we ignore this as we add this RE delay  to the first real RE element
				assert(getline(metaData, line));
			}
			else // destination
			{
				if (deletedPath)
					continue;

				index1 = 16;
				for (i = index1; i < (int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				path = stoi(line.substr(index1, i - index1));
				index1 = i + 4;
				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				node = stoi(line.substr(index1, i - index1));


				tempConnection.destinationX = paths[path][node].x;
				tempConnection.destinationY = paths[path][node].y;
				tempConnection.destinationZ = paths[path][node].z;
				tempConnection.destinationPort = paths[path][node].portIn;

				//// now we have all needed information, we will check if this connection already exists
				connExists = false;
				for (i = 0; i < (int)fpgaLogic[sourceX][sourceY][sourceZ].connections.size(); i++) // loop across existing connections
				{
					// checking if this connection exists already
					if (fpgaLogic[sourceX][sourceY][sourceZ].connections[i].destinationX == tempConnection.destinationX && fpgaLogic[sourceX][sourceY][sourceZ].connections[i].destinationY == tempConnection.destinationY && fpgaLogic[sourceX][sourceY][sourceZ].connections[i].destinationZ == tempConnection.destinationZ && fpgaLogic[sourceX][sourceY][sourceZ].connections[i].destinationPort == tempConnection.destinationPort && fpgaLogic[sourceX][sourceY][sourceZ].connections[i].sourcePort == tempConnection.sourcePort)
					{
						connExists = true;
						break;
					}
				}

				if (!connExists) // in this case add this connection
				{
					fpgaLogic[sourceX][sourceY][sourceZ].connections.push_back(tempConnection);
				}

			//	tempConnection.clear();
			}
		}
		// if it is not a source nor destiantion
		else
		{
			// no paths should be deleted  while parsing routing information.
			assert(!deletedPath);
			if (!deletedPath)
			{
				// push to the used connection to the array of strings
				tempConnection.usedRoutingResources.push_back(line);
				std::string tempKeyRE = "RE" + line;

				// read the RE delay 
				// read delay
				assert(getline(metaData, line));
				double REDelay = std::stod(line);

				// read delay edge type
				assert(getline(metaData, line));
				int edgeType = 0;
				if (line[0] == 'R')
					edgeType = 3;
				else
					assert(line[0] == 'F');

				if (tempKeyRE == "REC4:X49Y15S0I14")
					std::cout << "lala land" << std::endl;

				// enter the delay into the REsDelay map in addition to the cell re delay this is only inserted in the first routing element (RE)
				insert_to_REsDelay(tempKeyRE, cellREDelay + REDelay, edgeType);
				// set cell re delay to 0. It is only set to a value at the beginning of a routing connections based on the cell re delay.
				

				// enter the path using this RE to the REstopaths
				auto iter_REtoPaths = REToPaths.find(tempKeyRE);
				if (iter_REtoPaths == REToPaths.end()) // RE was not inserted before
				{
					std::vector <RE_logic_component> tempPaths;
					// store the path using RE, for node we choose the sink node as the node owning this RE
					tempPaths.push_back(RE_logic_component(path,node+1, cellREDelay + REDelay));
					assert((int)paths[path].size() > node + 1);
					REToPaths.insert(std::pair<std::string, std::vector<RE_logic_component> >(tempKeyRE, tempPaths));
				}
				else // this RE has been inserted then add the current path to the list of paths using it
				{
					// the current path can not be added to this RE, so check that (should be removed later as it takes time to do this check)
					
		//			assert((std::find((iter_REtoPaths->second).begin(), (iter_REtoPaths->second).end(), path)) == (iter_REtoPaths->second).end());
					// store the path using RE, for node we choose the sink node as the node owning this RE
					(iter_REtoPaths->second).push_back(RE_logic_component(path,node+1, cellREDelay + REDelay));
				}
				cellREDelay = 0.0;
			}
			else
				continue;
		}

	}



	////// add connections between the last lut in a path and the reg its feeding because up till this point we didnt add this if the reg is fed using port D. We have added it if the reg is feeded using asdata

	for ( i = 1; i < (int)paths.size(); i++) // loop across all paths
	{

		// empty path, just checking
		if (paths[i].size() < 2)
			continue;
		int lastNode = (int)paths[i].size() - 1;


		int FFx = paths[i][lastNode].x;
		int FFy = paths[i][lastNode].y;
		int FFz = paths[i][lastNode].z;

		if (fpgaLogic[FFx][FFy][FFz].FFMode == sData) // coz we have added the connection to this guy
			continue;

		int lastLUTx = paths[i][lastNode - 1].x;
		int lastLUTy = paths[i][lastNode - 1].y;
		int lastLUTz = paths[i][lastNode - 1].z;

		bool existAlready = false;

		// check if this connection has already been added
		for ( count = 0; count <(int)fpgaLogic[lastLUTx][lastLUTy][lastLUTz].connections.size(); count++)
		{
			if (fpgaLogic[lastLUTx][lastLUTy][lastLUTz].connections[count].destinationX == FFx && fpgaLogic[lastLUTx][lastLUTy][lastLUTz].connections[count].destinationY == FFy && fpgaLogic[lastLUTx][lastLUTy][lastLUTz].connections[count].destinationZ == FFz)
			{
				existAlready = true;
				break;
			}
		}

		if (!existAlready) // this connection was not added yet
		{
			Routing_connection tempConnectionExtra;

			tempConnectionExtra.destinationX = FFx;
			tempConnectionExtra.destinationY = FFy;
			tempConnectionExtra.destinationZ = FFz;

			tempConnectionExtra.destinationPort = paths[i][lastNode].portIn;

			fpgaLogic[lastLUTx][lastLUTy][lastLUTz].connections.push_back(tempConnectionExtra);


		}

	}

	adjust_REsDelay();
	return 1;

}





void insert_to_timingEdgesDelay(std::string tempKey, double delay, int edgeType)
{
	auto iter = timingEdgesDelay.find(tempKey);

	if (iter == timingEdgesDelay.end()) // edge not found
	{
		// since its not found we will insert it 
		// first create a vector with one elemtn, this element
		std::vector<Edge_Delay > tempVector;

		Edge_Delay temp = Edge_Delay(edgeType, delay);

		tempVector.push_back(temp);

		// insert this to the map
		timingEdgesDelay.insert(std::pair<std::string, std::vector<Edge_Delay > >(tempKey, tempVector));

	}
	else // edge found
	{
		//edge was found we will loop throuh the vector of delays to check if its already there or not

		bool found = false;

		for (int i = 0; i < (int)iter->second.size(); i++)
		{
			if ((iter->second)[i].type == edgeType)
			{
				found = true;
				if ((iter->second)[i].delay != delay)
				{
					std::cout << delay << std::endl;
					std::cout << (iter->second)[i].delay << std::endl;
				}
				assert((iter->second)[i].delay == delay);
				break;
			}

		}

		// if we didnt find it then we should add it to the vector
		if (!found)
		{
			Edge_Delay temp = Edge_Delay(edgeType, delay);
			(iter->second).push_back(temp);
		}

	}

}

// reads the meta edge file that stores IC and Cell delays, used for MCsim when dealing with timing edges as random variables
int read_timing_edges(char* edgesFile)
{
	//std::map<std::string, Edge_Delay >  timingEdgeToPaths;

	timingEdgesDelay.clear();
	std::string tempKey;


	std::ifstream edgeData(edgesFile);
	if (!edgeData)
	{
		std::cout << "Can not find file" << edgesFile << "  Terminating.... " << std::endl;
		return 0;
	}

	std::string line;



	int i, counter;



	int  index1 = -1;
	int currX = -1;
	int currY = -1;
	int currZ = -1;
	int currPin = -1;
	int currPout = -1;
	int prevX = -1; 
	int prevY = -1; 
	int prevZ = -1; 
//	int prevPin = -1; 
	int prevPout = -1;

	char currI = 'l'; 
	char currO  = 'l'; 
//	char prevI  = 'l'; 
	char prevO = 'l'; // the edge typ of the input and output from the prev and current node

	int edgeType = -1;

	double cellDel = -1.0;
	double ICDel = -1.0;
	while (std::getline(edgeData, line))
	{
		if (line[0] == '_') // new path
		{
			// structure is as follows
			// Tco // I will use this as CELL delay from the input of hte FF to its output
			// Flip flop location
			// edge type (FF, FR, RF, RR)
			// IC to next atom

			// read Tco
			assert(getline(edgeData, line));

			cellDel = std::stod(line);

			// read FF location man
			assert(getline(edgeData, line));
			assert(line[0] == '*' && line[1] == 'F');

			// set x, y, z
			counter = 0;
			for (i = 0; i < (int)line.size(); i++)
			{
				if (line[i] == '_')
					counter++;
				if (counter == 1)
				{
					index1 = i + 2;
					break;
				}
			}

			for (i = index1; i <(int)line.size(); i++)
			{
				if (!isdigit(line[i]))
					break;
			}
			currX = stoi(line.substr(index1, i - index1));
			index1 = i + 2;
			for (i = index1; i <(int)line.size(); i++)
			{
				if (!isdigit(line[i]))
					break;
			}
			currY = stoi(line.substr(index1, i - index1));

			index1 = i + 2;
			for (i = index1; i <(int)line.size(); i++)
			{
				if (!isdigit(line[i]))
					break;
			}
			currZ = stoi(line.substr(index1, i - index1));

			//// read edge type
			assert(getline(edgeData, line));

			currI = line[0];
			currO = line[1];

			edgeType = 4; // its the startinf FF so this is actuall Tco so to differentiate from reg delay we do this

			if (currI == 'R')
				edgeType+=2;

			if (currO == 'R')
				edgeType++;

			// read IC delay
			assert(getline(edgeData, line));
			ICDel = std::stod(line);

			currPin = FFd;
			currPout = FFq;

			// insert the CELL delay of this FF to edgesDelay
			tempKey = tempKey = "CELLsX" + std::to_string(currX) + "sY" + std::to_string(currY) + "sZ" + std::to_string(currZ) + "sP" + std::to_string(FFd) + "dP" + std::to_string(FFq);

			insert_to_timingEdgesDelay(tempKey, cellDel, edgeType);

		}
		else
		{
			if (line[0] == '*' && line[1] == 'L')
			{
				///*LCCOMB_X47_Y5_N2
				///LUT 1 6
				///RR // dege type
				///0.361 cell delay
				///0.788 IC delay to next atom

				prevX = currX;
				prevY = currY;
				prevZ = currZ;
			//	prevPin = currPin;
				prevPout = currPout;
			//	prevI = currI;
				prevO = currO;

				///////////////////////////////////// read x, y, z /////////////////////////////////////////////////////////
				// set x, y, z
				counter = 0;
				for (i = 0; i < (int)line.size(); i++)
				{
					if (line[i] == '_')
						counter++;
					if (counter == 1)
					{
						index1 = i + 2;
						break;
					}
				}

				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currX = stoi(line.substr(index1, i - index1));
				index1 = i + 2;
				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currY = stoi(line.substr(index1, i - index1));

				index1 = i + 2;
				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currZ = stoi(line.substr(index1, i - index1));

				// ///////////////////////////////////read port in and pirt out/////////////////////////////////////////////////////////////////

				assert(getline(edgeData, line));

				currPin = line[4] - '0';
				currPout = line[6] - '0';

				// add IC delay from previous node to current node

				tempKey = "ICsX" + std::to_string(prevX) + "sY" + std::to_string(prevY) + "sZ" + std::to_string(prevZ) + "sP" + std::to_string(prevPout) + "dX" + std::to_string(currX) + "dY" + std::to_string(currY) + "dZ" + std::to_string(currZ) + "dP" + std::to_string(currPin);
				
				// IC delay can either be FF or RR, nothing else so we just check what was the previous transition and set the edge type accordingle
				edgeType = 0;
				if (prevO == 'R')
					edgeType = 3;


				insert_to_timingEdgesDelay(tempKey, ICDel, edgeType);

				////////////////////////////////// read edge type of current node /////////////////////////////////////////
				assert(getline(edgeData, line));

				currI = line[0];
				currO = line[1];

				edgeType = 0;

				if (currI == 'R')
					edgeType += 2;

				if (currO == 'R')
					edgeType++;

				//////////////////// read cell delay of current node ////////////////////////////////////////////////////////
				assert(getline(edgeData, line));

				cellDel = std::stod(line);

				// insert the CELL delay of this FF to edgesDelay
				tempKey = tempKey = "CELLsX" + std::to_string(currX) + "sY" + std::to_string(currY) + "sZ" + std::to_string(currZ) + "sP" + std::to_string(currPin) + "dP" + std::to_string(currPout);

				insert_to_timingEdgesDelay(tempKey, cellDel, edgeType);

				///////////////////////// read IC delay ////////////////////////////////////////////////////////////
				assert(getline(edgeData, line));
				ICDel = std::stod(line);


			}
			else
			{
				// its the last register in the path

				assert(line[0] == '*'&&line[1] == 'F');

				prevX = currX;
				prevY = currY;
				prevZ = currZ;
			//	prevPin = currPin;
				prevPout = currPout;
			//	prevI = currI;
				prevO = currO;

				///////////////////////////////////// read x, y, z /////////////////////////////////////////////////////////
				// set x, y, z
				counter = 0;
				for (i = 0; i < (int)line.size(); i++)
				{
					if (line[i] == '_')
						counter++;
					if (counter == 1)
					{
						index1 = i + 2;
						break;
					}
				}

				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currX = stoi(line.substr(index1, i - index1));
				index1 = i + 2;
				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currY = stoi(line.substr(index1, i - index1));

				index1 = i + 2;
				for (i = index1; i <(int)line.size(); i++)
				{
					if (!isdigit(line[i]))
						break;
				}
				currZ = stoi(line.substr(index1, i - index1));

				////////////// read port In and out nor important
				//////////////////////////////////////////////////////////////////////////////////////
				assert(getline(edgeData, line));
				currPin = FFd;
				currPout = FFq;


				// add IC delay from previous node to current node

				tempKey = "ICsX" + std::to_string(prevX) + "sY" + std::to_string(prevY) + "sZ" + std::to_string(prevZ) + "sP" + std::to_string(prevPout) + "dX" + std::to_string(currX) + "dY" + std::to_string(currY) + "dZ" + std::to_string(currZ) + "dP" + std::to_string(currPin);

				edgeType = 0;
				if (prevO == 'R')
					edgeType = 3;


				insert_to_timingEdgesDelay(tempKey, ICDel, edgeType);

				////////////////////////////////// read edge type of current node /////////////////////////////////////////
				assert(getline(edgeData, line));

				currI = line[0];
				currO = line[1];

				edgeType = 0;

				if (currI == 'R')
					edgeType += 2;

				if (currO == 'R')
					edgeType++;

				assert(edgeType == 0 || edgeType == 3);// its a FF s it cant be inverting

				//////////////////// read cell delay of current node ////////////////////////////////////////////////////////
				assert(getline(edgeData, line));

				cellDel = std::stod(line);

				// insert the CELL delay of this FF to edgesDelay
				tempKey = tempKey = "CELLsX" + std::to_string(currX) + "sY" + std::to_string(currY) + "sZ" + std::to_string(currZ) + "sP" + std::to_string(currPin) + "dP" + std::to_string(currPout);

				insert_to_timingEdgesDelay(tempKey, cellDel, edgeType);


			}
		}
	}


	return 1;

}

#ifdef StratixV // check routing is correct, currently only compatable with Stratix V arch
void check_routing(char* routingFilePost) // this function read the rcf file created by quartus for the calibration bit-stream. We used it to check that quartus followed all our routing constraints and that every thing is routed as we want.
{

	std::ifstream metaData(routingFilePost);
	if (!metaData)
	{
		std::cout << "Can not find file" << routingFilePost << "  Terminating.... " << std::endl;
		return;
	}
	Routing_connection tempConnection;
	std::string line;
	std::map<std::string, std::pair<int,int> > labelDefinition;

	int i, index1, sourcePath, sourceNode, sourceX, sourceY, sourceZ, currentConn, currentRE;

	bool foundSource = false;
	bool foundDestination = false;
	std::string signature = "signal_name = PATH";

	while (std::getline(metaData, line))
	{
		if (!foundSource) // did not find soiurce yet so keep looking, trying to match the read line with the signature we are looking for
		{
			bool flag = true;
			if (line.length() < signature.length())
			{
				continue;
			}

			for ( i = 0; i < (int)signature.length(); i++)
			{
				if (line[i] != signature[i])
				{
					flag = false;
					break;
				}
			}
			if (!flag) // did not match
			{
				continue;
			}

			if (!check_source(line)) // this is a function that checks strongly for the signature not just the beginning
				continue;

			foundSource = true;

			index1 = i;

			for (i = index1; i < (int)line.size(); i++)
			{
				if (!isdigit(line[i]))
					break;
			}
			sourcePath = stoi(line.substr(index1, i - index1));

			index1 = i + 4; // skip the word "NODE"

			for (i = index1; i <(int)line.size(); i++)
			{
				if (!isdigit(line[i]))
					break;
			}
			sourceNode = stoi(line.substr(index1, i - index1));
			if (sourcePath == 33 && sourceNode == 1)
				std::cout << "DEbug" << std::endl;
			sourceX = paths[sourcePath][sourceNode].x;
			sourceY = paths[sourcePath][sourceNode].y;
			sourceZ = paths[sourcePath][sourceNode].z;

			tempConnection.clear();
			currentConn = 0;
			currentRE = 0;

			// ensure that this fanout was not seen before
			assert(fpgaLogic[sourceX][sourceY][sourceZ].actualConnections.size() == 0);

		}
		else
		{
			if (line[0] != '}') // this fanout is not terminated yet
			{
				// 5 options, 1) a routing elemnt without label, 2) a routing element with label, 3) destination line, 4) empty line, 5) branch_point 
				if (line.size() < 2) // option 4, empty line
					continue;

				if (check_char_exists(line, '=')) // if the line has an '=' then this is either option 2 (re witha a label) or option 3 (destination) or optioon 5 (branch point)
				{
					if (line[1] == 'l') // option 2 RE with label
					{
						std::string tempString;
						std::pair<int, int> value = std::make_pair(currentConn, currentRE); // value f the label , position of the routing element in the actualConnection array
						/// handle the label shit, store the corresponding connection and re with the right label
						int j;
						// skip all part until the equal sign
						for (j = 0; j < (int)line.length(); j++)
						{
							if (line[j] == '=')
								break;
						}

						assert(j < (int)line.length()); // checks that an = exist in th line string
						// after the equal, skip all parts until the first letter ('L')

						for (; j < (int)line.length(); j++)
						{
							if (isalpha(line[j])) // this char is an alphabetic letter
								break;
						}

						// create the string by adding all letters from now until the comma
						for (; j < (int)line.length(); j++)
						{
							if (line[j] == ',') // label name ends
								break;
							
							tempString.push_back(line[j]);
						}
						// tempString contains the label name

						// check that this label was not seen before must be true
						auto iter = labelDefinition.find(tempString);
						assert(iter == labelDefinition.end());

						// insert the label with the corresponding connection and routing element
						labelDefinition.insert(make_pair(tempString, value));

						// push the Re without leading spaces into tempConnections
						tempConnection.usedRoutingResources.push_back(remove_leading_spaces(line.substr(j+1))); // the string given to the function starts after the comma in the label line and goes until the end of the string
						currentRE++;


					}
					else if (line[1] == 'b') // option 5 branch point
					{
						assert(currentConn > 0); // first connection can not have a branchpoint
						assert(currentRE == 0); // branch point can only exists as the first thing in a routing element

						std::string tempString;
						tempString.clear();
						int j;
						// skip all part until the equal sign
						for (j = 0; j <(int)line.length(); j++)
						{
							if (line[j] == '=')
								break;
						}

						assert(j < (int)line.length()); // checks that an = exist in th line string
					
						// after the equal, skip all parts until the first letter ('L')

						for (; j < (int)line.length(); j++)
						{
							if (isalpha(line[j])) // this char is an alphabetic letter
								break;
						}

						// create the string by adding all letters from now until the ';'
						for (; j < (int)line.length(); j++)
						{
							if (line[j] == ';') // label name ends
								break;

							tempString.push_back(line[j]);
						}
						// now tempString has the label we are branching from, find connection and re corresponding to that branch point

						auto iter = labelDefinition.find(tempString);

						// label must be there already
						assert(iter != labelDefinition.end());

						int correspConn = (iter->second).first;
						int correspRE = (iter->second).second;

						assert((int)fpgaLogic[sourceX][sourceY][sourceZ].actualConnections.size()>correspConn);
						assert((int)fpgaLogic[sourceX][sourceY][sourceZ].actualConnections[correspConn].usedRoutingResources.size() > correspRE);


						for (int k = 0; k <= correspRE; k++)
						{
							tempConnection.usedRoutingResources.push_back(fpgaLogic[sourceX][sourceY][sourceZ].actualConnections[correspConn].usedRoutingResources[k]);
						}
						currentRE = correspRE + 1;

					}
					else // option 3 destination
					{
						assert(line[1] == 'd');
						// check if the destination is a node in our structure (constrianed wy keda)
						if (check_destination(line))
						{


							// find the destination 
							std::string tempString;
							tempString.clear();
							int j;
							// skip all part until the equal sign
							for (j = 0; j < (int)line.length(); j++)
							{
								if (line[j] == '=')
									break;
							}

							assert(j < (int)line.length()); // checks that an = exist in th line string

							 // after the equal, skip all parts until the first number ('P')

							for (; j < (int)line.length(); j++)
							{
								if (isdigit(line[j])) // this char is a number
									break;
							}

							for (; j < (int)line.length(); j++)
							{
								if (!isdigit(line[j]))
									break;
								tempString.push_back(line[j]);
							}

							int destinationPath = stoi(tempString);

							// now do the same to get the node
							tempString.clear();

							for (; j < (int)line.length(); j++)
							{
								if (isdigit(line[j])) // this char is a number
									break;
							}

							for (; j < (int)line.length(); j++)
							{
								if (!isdigit(line[j]))
									break;
								tempString.push_back(line[j]);
							}

							int destinationNode = stoi(tempString);

							int destinationX = paths[destinationPath][destinationNode].x;
							int destinationY = paths[destinationPath][destinationNode].y;
							int destinationZ = paths[destinationPath][destinationNode].z;

							tempConnection.destinationX = destinationX;
							tempConnection.destinationY = destinationY;
							tempConnection.destinationZ = destinationZ;

							if (destinationZ%LUTFreq == 0) // destination is a LUT, so check the destination port
							{
								// skip all part until the second equal sign
								for (; j < (int)line.length(); j++)
								{
									if (line[j] == '=')
										break;
								}
								// skip all parts until the first letter after the second equal sign which would be 'D' from DATA
								for (; j < (int)line.length(); j++)
								{
									if (line[j] == ';') // the end of this line
										break;
								}
								char destPort;
								for (; j > 0; j--)
								{
									if (isalpha(line[j]))
									{ 
										destPort = line[j];
										break;
									}
								}

								//char destPort = line[j - 1]; // to get the port which is the letter just beforer the ';'
								
								int destinationPort;
								switch (destPort)
								{
								case 'A':
									destinationPort = portA;
									break;
								case 'B':
									destinationPort = portB;
									break;
								case 'C':
									destinationPort = portC;
									break;
								case 'D':
									destinationPort = portD;
									break;
#ifdef StratixV
								case 'E':
									destinationPort = portE;
									break;
								case 'F':
									destinationPort = portF;
									break;
#endif
								default:
									std::cout << "Something wrong with destination port when reading post routing file" << std::endl;
									break;
								}
								tempConnection.destinationPort = destinationPort;
							}
						}

						// add temp cponnection to the LUT

						fpgaLogic[sourceX][sourceY][sourceZ].actualConnections.push_back(tempConnection);
						// clear temp connection
						tempConnection.clear();

						// add 1 to get the next connection, reset the routing elements to zero
						currentConn++;
						currentRE = 0;

					}

				}
				else // this must be option 1 , re with no label
				{
					// push the Re without leading spaces into tempConnections
					tempConnection.usedRoutingResources.push_back(remove_leading_spaces(line));
					currentRE++;
				}

			}
			else
			{
				foundSource = false;
			}
		}


	}

	

}

bool check_signature_begin(std::string target, std::string signature)
{
	int i;
	if (target.length() < signature.length())
		return false;

	for (i = 0; i < (int)signature.length(); i++)
	{
		if (target[i] != signature[i])
		{
			return false;
		}
	}


	return true;
}

bool check_char_exists(std::string target, char x)
{
	for (int i = 0; i <(int)target.length(); i++)
	{
		if (target[i] == x)
			return true;
	}
	return false;
}

std::string remove_leading_spaces(std::string target) // also removes the ';' at the end
{
	std::string noSpaces;
	bool flag = false;
	for (int i = 0; i < (int)target.length(); i++)
	{
		if (target[i] != ' ' && target[i] != '\t')
			flag = true;
		if (flag && target[i] != ';')
			noSpaces.push_back(target[i]);


	}
	return noSpaces;

}

bool check_destination(std::string target)
{
	int j;
	for (j = 0; j < (int)target.length(); j++)
	{
		if (target[j] == '(')
			break;
	}
	j++;
	for (; j < (int)target.length(); j++)
	{
		if (target[j] == ' ' || target[j] == '\t')
			continue;
		else
			break;
	}


	bool cond1 = false;
	bool cond2 = false;
	bool cond3 = false;
	bool cond4 = false;
	int stage = 0;
	std::string part1 = "PATH";
	int countPart1 = 0;
	std::string part2 = "ODE";
	int countPart2 = 0;
	for (; j < (int)target.length(); j++)
	{
		if (stage == 0) // matching PATH
		{
			if (target[j] != part1[countPart1])
				break;
			countPart1++;
			if (countPart1 == part1.length())
				stage++;
		}
		else if (stage == 1) // matching numbers until N
		{
			if (countPart1 == part1.length()) // 1st time to go into the stage 1 section, so we must have a digit
			{
				if (!isdigit(target[j]))
					break;

				countPart1++;
			}
			else // make sure the first letter we see is N
			{
				if (!isdigit(target[j]))
				{
					if (target[j] != 'N')
						break;
					else
						stage++;
				}
			}
		}
		else if (stage == 2) // checking for part2
		{
			if (target[j] != part2[countPart2])
				break;
			countPart2++;
			if (countPart2 == part2.length())
				stage++;
		}
		else
		{
			if (countPart2 == part2.length()) // 1st time to go into the stage 1 section, so we must have a digit
			{
				if (!isdigit(target[j]))
					break;

				countPart2++;
			}
			else // make sure the first letter we see is N
			{
				if (!isdigit(target[j]))
				{
					if (target[j] != ',')
						break;
					else
						stage++;
				}
			}

		}

	}
	return (stage == 4);
}



bool check_source(std::string target)
{
	int j;
	for (j = 0; j < (int)target.length(); j++)
	{
		if (target[j] == '=')
			break;
	}

	j++;

	for (; j < (int)target.length(); j++)
	{
		if (target[j] == ' ' || target[j] == '\t')
			continue;
		else
			break;
	}



	int stage = 0;
	std::string part1 = "PATH";
	int countPart1 = 0;
	std::string part2 = "ODE";
	int countPart2 = 0;
	for (; j < (int)target.length(); j++)
	{
		if (stage == 0) // matching PATH
		{
			if (target[j] != part1[countPart1])
				break;
			countPart1++;
			if (countPart1 == part1.length())
				stage++;
		}
		else if (stage == 1) // matching numbers until N
		{
			if (countPart1 == part1.length()) // 1st time to go into the stage 1 section, so we must have a digit
			{
				if (!isdigit(target[j]))
					break;

				countPart1++;
			}
			else // make sure the first letter we see is N
			{
				if (!isdigit(target[j]))
				{
					if (target[j] != 'N')
						break;
					else
						stage++;
				}
			}
		}
		else if (stage == 2) // checking for part2
		{
			if (target[j] != part2[countPart2])
				break;
			countPart2++;
			if (countPart2 == part2.length())
				stage++;
		}
		else
		{
			if (countPart2 == part2.length()) // 1st time to go into the stage 1 section, so we must have a digit
			{
				if (!isdigit(target[j]))
					break;

				countPart2++;
			}
			else // make sure that after the nde number nothing exists
			{
				if (!isdigit(target[j]))
				{
					if (target[j] != ' ' && target[j] != '{')
						break;
					else
					{
						stage++;
						break;
					}
				}
			}

		}

	}
	return (stage == 4);
}

bool compare_routing()
{
	int totalMatched = 0;
	int sizeMis = 0;
	int REmis = 0;
	int destMis = 0;

	std::ofstream debug;
	debug.open("debug.txt");


	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ; k++)
			{
				// loop across all constraint fanouts at this LUT
				for (int l = 0; l < (int)fpgaLogic[i][j][k].connections.size(); l++)
				{
					if (fpgaLogic[i][j][k].connections[l].destinationX < 0) // this connection is deleted
						continue;

					assert(fpgaLogic[i][j][k].utilization>0);
					if (k%LUTFreq == 0) // this is a lut and not a FF
					{
						assert(fpgaLogic[i][j][k].connections[l].sourcePort == Combout);
					}
					bool equal = false;
					// loop across actual fanouts at this LUT
					for (int m = 0; m < (int)fpgaLogic[i][j][k].actualConnections.size(); m++)
					{
						if (fpgaLogic[i][j][k].connections[l].destinationX == fpgaLogic[i][j][k].actualConnections[m].destinationX  && fpgaLogic[i][j][k].connections[l].destinationY == fpgaLogic[i][j][k].actualConnections[m].destinationY && fpgaLogic[i][j][k].connections[l].destinationZ == fpgaLogic[i][j][k].actualConnections[m].destinationZ)
						{
							// this is the same connection check if it is routed correctly
							if (fpgaLogic[i][j][k].connections[l].usedRoutingResources.size() != fpgaLogic[i][j][k].actualConnections[m].usedRoutingResources.size())
							{
								// something is wrong 
								std::cout << "Routing mismatch not the same number of routing elements" << std::endl;
								sizeMis++;
								debug << "Routing mismatch not the same number of routing elements" << std::endl;
								debug << "source node is PATH " << fpgaLogic[i][j][k].owner.path << " NODE " << fpgaLogic[i][j][k].owner.node << " to PATH " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.path << " NODE " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.node << " which is " << fpgaLogic[i][j][k].connections[l].destinationX << "_" << fpgaLogic[i][j][k].connections[l].destinationY << "_" << fpgaLogic[i][j][k].connections[l].destinationZ << " " << " through port " << fpgaLogic[i][j][k].connections[l].destinationPort << " not port " << fpgaLogic[i][j][k].actualConnections[m].destinationPort << std::endl;
								debug << std::endl;
								return false;
							}

							// routing elemnts used are the number
							for (int x = 0; x < (int)fpgaLogic[i][j][k].connections[l].usedRoutingResources.size(); x++)
							{
								if (fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] != fpgaLogic[i][j][k].actualConnections[m].usedRoutingResources[x])
								{
									std::cout << "mismatch in used routing element" << std::endl;
									debug << "mismatch in used routing element" << std::endl;
									debug << "source node is PATH " << fpgaLogic[i][j][k].owner.path << " NODE " << fpgaLogic[i][j][k].owner.node << " to PATH " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.path << " NODE " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.node << " which is " << fpgaLogic[i][j][k].connections[l].destinationX << "_" << fpgaLogic[i][j][k].connections[l].destinationY << "_" << fpgaLogic[i][j][k].connections[l].destinationZ << " " << " through port " << fpgaLogic[i][j][k].connections[l].destinationPort << " not port " << fpgaLogic[i][j][k].actualConnections[m].destinationPort << std::endl;
									debug << std::endl;
									REmis++;
									return false;
								}
							}
							if (fpgaLogic[i][j][k].connections[l].destinationZ%LUTFreq == 0) // the destination is a LUT so check destination port
							{
								if (fpgaLogic[i][j][k].connections[l].destinationPort != fpgaLogic[i][j][k].actualConnections[m].destinationPort)
								{


									debug << "mismatch in destination POrt" << std::endl;
									debug << "source node is PATH " << fpgaLogic[i][j][k].owner.path << " NODE " << fpgaLogic[i][j][k].owner.node << " to PATH " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.path << " NODE " << fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ].owner.node << " which is " << fpgaLogic[i][j][k].connections[l].destinationX << "_" << fpgaLogic[i][j][k].connections[l].destinationY << "_" << fpgaLogic[i][j][k].connections[l].destinationZ << " " << " through port " << fpgaLogic[i][j][k].connections[l].destinationPort << " not port " << fpgaLogic[i][j][k].actualConnections[m].destinationPort << std::endl;
									
									assert(fpgaLogic[i][j][k].connections[l].usedRoutingResources.size()>1);

									debug << fpgaLogic[i][j][k].connections[l].usedRoutingResources[fpgaLogic[i][j][k].connections[l].usedRoutingResources.size() - 2] << std::endl;
									debug << fpgaLogic[i][j][k].connections[l].usedRoutingResources[fpgaLogic[i][j][k].connections[l].usedRoutingResources.size() - 1] << std::endl ;


									if (fpgaLogic[i][j][k].owner.path == 113)
										std::cout << "debug" << std::endl;

									if ((fpgaLogic[i][j][k].connections[l].destinationZ / 3) % 2 == 0) // destination is a top LUT in the ALM
									{
										debug << "destination is a top LUT and";

										if (fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ + 3].utilization == 0)
											debug << " bottom LUT is empty" << std::endl << std::endl;
										else
											debug << " bottom LUT is utilized" << std::endl << std::endl;

									}
									else // destination is a bottom LUT
									{
										std::cout << "destination is a bottom LUT and";

										if (fpgaLogic[fpgaLogic[i][j][k].connections[l].destinationX][fpgaLogic[i][j][k].connections[l].destinationY][fpgaLogic[i][j][k].connections[l].destinationZ - 3].utilization == 0)
											debug << " top LUT is empty" << std::endl << std::endl;
										else
											debug << " top LUT is utilized" << std::endl << std::endl;

									}

									std::cout << std::endl;
									
									destMis++;
									return false;
								}
							}
						}
					}
					totalMatched++;
				}

				

			}
		}
	}


	return true;
}

#endif