#include "createOutputFiles.h"
//#include "globalVar.h"
#include "util.h"
#include "fanouts.h"
//#include "StratixV.h"
#include <sstream>

#ifdef CycloneIV

void LUT_WYSIWYG_CycloneIV(std::vector <Path_logic_component>& cascadedControlSignals, int i, int j, int k, std::ofstream& verilogFile, int port1, int port2, std::vector <Path_logic_component>& CoutSignals, std::vector <Path_logic_component>& controlSignals, int path, int node, int pathFeederPort1, int nodeFeederPort1, int  pathFeederPort2, int nodeFeederPort2, bool & inverting);//, std::vector <Path_logic_component>& cascadedControlSignals);
void create_location_contraint_file(int bitStreamNumber) // modified for STratix V
{
	int i, j, k;
	int x;
	int total = 0;
	std::ofstream LoFile;
	std::string locFileName = "LocationFile_" + std::to_string(bitStreamNumber) + ".txt";
	LoFile.open(locFileName);
	int path = -1;
	int node = -1;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization>0) // this LE is used, in this case assign that location to the most critical path using it.
				{
					total++;
					if (fpgaLogic[i][j][k].usedOutputPorts < 1 && k % 2 != 1)
						if (!fpgaLogic[i][j][k].isBRAM)
							std::cout << "error" << std::endl;

					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // find the undeleted most critical path using this node
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							//		LoFile << fpgaLogic[i][j][k].nodes[x].path << "NODE" << fpgaLogic[i][j][k].nodes[x].node << "_t" << std::endl;
							break;
						}
					}
					assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node);
					assert(path > -1);
					if (path == -1) // if no deleted path was found then continue 
						continue;

					if (k % LUTFreq == 0)
					{
#ifdef CycloneIV
						if (!fpgaLogic[i][j][k].isBRAM)
						{
							LoFile << "set_location_assignment LCCOMB_X" << i << "_Y" << j << "_N" << k
								<< " -to PATH" << path << "NODE" << node << "_t" << std::endl;
						}
						else // memory
						{
							assert(k == 0);
							if (fpgaLogic[i][j][k].countNumofMem == 1)
							{
								LoFile << "set_location_assignment M9K_X" << i << "_Y" << j << "_N0"
									<< " -to PATH" << path << "NODE" << node << "_t_0" << std::endl;
							}
							else
							{
								assert(fpgaLogic[i][j][k].countNumofMem == 2);
								LoFile << "set_location_assignment M9K_X" << i << "_Y" << j << "_N0"
									<< " -to PATH" << path << "NODE" << node << "_t_0" << std::endl;
								LoFile << "set_location_assignment M9K_X" << i << "_Y" << j << "_N0"
									<< " -to PATH" << path << "NODE" << node << "_t_1" << std::endl;

							}
						}
#endif
#ifdef StratixV
						LoFile << "set_location_assignment ";
						if (fpgaLogic[i][j][k].mLAB)
							LoFile << "M";
						LoFile << "LABCELL_X" << i << "_Y" << j << "_N" << k << " -to PATH" << fpgaLogic[i][j][k].owner.path /*path*/ << "NODE" << fpgaLogic[i][j][k].owner.node /*node*/ << "_t" << std::endl;

#endif
					}
					else
					{
						LoFile << "set_location_assignment FF_X" << i << "_Y" << j << "_N" << k << " -to PATH" << path << "NODE" << node << "_t" << std::endl;
					}
#ifdef DUMMYREG
					if (fpgaLogic[i][j][k].inputPorts[portC])
					{
						assert(k % 2 == 0);
						if (fpgaLogic[i][j][k + 1].utilization > 0)
							continue;
						LoFile << "set_location_assignment FF_X" << i << "_Y" << j << "_N" << k + 1 << " -to DUMMYREG_PATH" << path << "NODE" << node << "_t" << std::endl;

					}
#endif

				}
				path = -1;
				node = -1;
			}
		}
	}

	LoFile.close();
}




void create_auxil_file(std::vector <Path_logic_component> sinks,
	std::vector <Path_logic_component> controlSignals,
	std::vector <Path_logic_component> CoutSignals,
	std::vector <Path_logic_component> sources,
	std::vector <Path_logic_component> cascadedControlSignals,
	std::ifstream& verilogFileSecondPart,
	std::string BRAM_intermediateSignals,
	std::vector<BRAM> memories,
	int bitStreamNumber ) // to be merged with the WYSIWYGs file to complete the source file.
{
	std::ofstream verilogFile;
	std::ofstream LoFile;
	std::string verilogFileName = "top_" + std::to_string(bitStreamNumber) + ".v";
	verilogFile.open(verilogFileName);
	verilogFile << "module top (input CLK, input reset, input start_test, output error, output fuck";
#ifdef DUMMYREG
	verilogFile << ", dummyOut";
#endif
	verilogFile << " );" << std::endl;
	int i = 0;
	verilogFile << "//Control Signals" << std::endl;
	verilogFile << "wire ";
	for (i = 0; i <(int)controlSignals.size(); i++)
	{
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F, ";
	}
	verilogFile << std::endl << "//Sinks//" << std::endl;


	for (i = 0; i < (int)sinks.size() - 1; i++)
	{
		verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << ", ";
	}

	verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "; " << std::endl;
	//verilogFile << ");" << std::endl;
	verilogFile << "//Sources" << std::endl;
	// source
	for (i = 0; i < (int)sources.size() - 1; i++)
	{
		if (i == 0)
			verilogFile << "wire PATH" << sources[i].path << "NODE" << sources[i].node << ", ";
		else
			verilogFile << "PATH" << sources[i].path << "NODE" << sources[i].node << ", ";
	}

	if (i != 0) // source is bigger than 1
		verilogFile << "PATH" << sources[i].path << "NODE" << sources[i].node << " ;" << std::endl;
	else 
	{
		if (sources.size()>0)
			verilogFile << "wire PATH" << sources[i].path << "NODE" << sources[i].node << " ;" << std::endl;
	}

	// todo: check if controlSignals has only one element
	// intermediate signals
	verilogFile << "//intermediate signals" << std::endl; // todo: generate signal if it is only needed not double outputs of wysiwygs
	for (i = 0; i < (int)controlSignals.size() - 1; i++)
	{
		if (i == 0)
			verilogFile << "wire PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << ", ";
		else
			verilogFile << "PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << ", ";
	}


	verilogFile << "PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node /*<< "  synthesis keep ;"*/ << ";" << std::endl; // removed synnthesis keep


	 // cout signals
	if (CoutSignals.size() > 0)
	{
		for (i = 0; i < (int)CoutSignals.size() - 1; i++)
		{
			if (i == 0)
				verilogFile << "wire PATH" << CoutSignals[i].path << "NODE" << CoutSignals[i].node << "_Cout, ";
			else
				verilogFile << "PATH" << CoutSignals[i].path << "NODE" << CoutSignals[i].node << "_Cout, ";
		}


		verilogFile << "PATH" << CoutSignals[i].path << "NODE" << CoutSignals[i].node << "_Cout;" << std::endl;
	}

	//// wires used for cacaded stuff
	verilogFile << "// cascaded LUTs extra wires required for controlling the testing of cascaded paths" << std::endl;
	int cascPath, cascNode;
	for (i = 0; i < (int)cascadedControlSignals.size(); i++)
	{
		cascPath = cascadedControlSignals[i].path;
		cascNode = cascadedControlSignals[i].node;
		if (i == 0)
		{
			verilogFile << "wire source_path" << cascPath << "_node" << cascNode << " , cascaded_selector_path" << cascPath << "_node" << cascNode << " , PATH" << cascPath << "NODE" << cascNode << "F_cascaded ";
		}
		else
		{
			verilogFile << ", source_path" << cascPath << "_node" << cascNode << " , cascaded_selector_path" << cascPath << "_node" << cascNode << " , PATH" << cascPath << "NODE" << cascNode << "F_cascaded ";

		}
	}
	if (cascadedControlSignals.size() > 0)
		verilogFile << " ;" << std::endl;

	// intermediate BRAM signals
	verilogFile << "// intermediate BRAM signals" << std::endl;
	verilogFile << BRAM_intermediateSignals;

	// control signals from separate controllers to BRAM
	// loop over al memory controllers

	verilogFile << "\n \n// signals between separate controllers and BRAMs \n \n";
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			if (memoryControllers[i][j].size() > 0) // memory controller is needed
			{
				int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
				int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;


				// signals between BRAM and BRAM test controller
				for (int k = 0; k < memoryControllers[i][j].size(); k++)
				{

					for (int counter = memoryControllers[i][j][k].second -1 ; counter >= 0; counter--)
					{
						verilogFile << "wire PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
							+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + "; \n";
					}
				}

				// singals between BRAM test controller and main controller
				int testedPortSize = -1;
				int controllerType = -1;
				if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
				{

					verilogFile << "wire error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM;\n";
					verilogFile << "wire done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM;\n";
					verilogFile << "wire reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM;\n";
					verilogFile << "wire start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM;\n";
					if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
						|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
						|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
						|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
					{
						verilogFile << "wire [" << testedPortSize - 1 << ":0] info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM;\n";
					}

				}

			}
		}
	}


	/////////////////////////////////////////////////
	///////// create BRAM controllers ///////////////
	/////////////////////////////////////////////////

	verilogFile << "// Create BRAM controllers\n\n";


	// Create BRAM controllers
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
			int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
			// todo: handle multiple BRAMs in the same BRAM
			
			int testedPortSize = -1;
			int controllerType = -1;
			if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
			{
				int memIndex = fpgaLogic[i][j][0].indexMemories[0];
				//controllerFile << "reg [" << memoryControllers[i][j][addressIndex].second << " : 0] address_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling;\n";

				// we are using a BRAM test controller so let's do this

				// todo: refactor the instatiation of controllers
				// address in read incapable controller
				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER)
				{
					verilogFile << "controller_address_writeOnly #(.data_width(" << memories[memIndex].portAUsedDataWidth << "),\n"
						<< "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportBAddress].size() << "),\n"
						<< "\t.Latency(" << TESTING_LATENCY << ")) controller_BRAM_" << BRAMPathOwner << "_" << BRAMNodeOwner
						<< " (.clk(CLK),\n"
						<< "\t.startTest(start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.reset(reset),\n"
						<< "\t.resetTest(reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.infoToTest(info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.BRAM_dataOut(PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_b_dataout),\n"
						<< "\t.error(error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.done(done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

					for (int k = 0; k < memoryControllers[i][j].size(); k++)
					{
						if (memoryControllers[i][j][k].first == BRAMportAData)
							verilogFile << "\t.porta_dataIn({";
						else if (memoryControllers[i][j][k].first == BRAMportAWE)
							verilogFile << "\t.write_enable({";
						else if (memoryControllers[i][j][k].first == BRAMportBAddress)
							verilogFile << "\t.portb_address({";
						else
						{
							assert(true);
						}

						int counter = 0;
						for (counter = memoryControllers[i][j][k].second - 1; counter >= 1; counter--)
						{
							verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
								+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + ",";
						}
						verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
							+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + "})";

						// last iteration
						if (k == memoryControllers[i][j].size() - 1)
						{
							verilogFile << ");\n\n";
						}
						else
						{
							verilogFile << ",\n";
						}
					}
				}

				// data in controller
				if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER)
				{
					verilogFile << "controller_dataIn #(.data_width(" << testedPortSize << "),\n";

					// if BRAM is not registered then say taht to the controller
					if (!memories[memIndex].portARegistered)
						verilogFile << "\t.isBRAMReg(0),\n";


					if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportBAddress].size() << "),\n";
					else
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportAAddress].size() << "),\n";

					verilogFile << "\t.Latency(" << TESTING_LATENCY << ")) controller_BRAM_" << BRAMPathOwner << "_" << BRAMNodeOwner
						<< " (.clk(CLK),\n"
						<< "\t.startTest(start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.reset(reset),\n"
						<< "\t.resetTest(reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.infoToTest(info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";
					if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA)
						verilogFile << "\t.BRAM_dataOut(PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_a_dataout),\n";
					else
						verilogFile << "\t.BRAM_dataOut(PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_b_dataout),\n";

					verilogFile << "\t.error(error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.done(done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";


					for (int k = 0; k < memoryControllers[i][j].size(); k++)
					{
						if (memoryControllers[i][j][k].first == BRAMportAData)
							assert(false);
						else if (memoryControllers[i][j][k].first == BRAMportAWE)
							verilogFile << "\t.write_enable({";
						else if (memoryControllers[i][j][k].first == BRAMportBAddress)
							if (memories[memIndex].operationMode == simpleDualPort)
								verilogFile << "\t.port_r_address_optional({";
							else
								verilogFile << "\t.port_w_address({";
						else if (memoryControllers[i][j][k].first == BRAMportBWE)
							verilogFile << "\t.write_enable({";
						else if (memoryControllers[i][j][k].first == BRAMportAAddress)
							verilogFile << "\t.port_w_address({";
						else
						{
							assert(false);
						}

						int counter = 0;
						for (counter = memoryControllers[i][j][k].second - 1; counter >= 1; counter--)
						{
							verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
								+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + ",";
						}
						verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
							+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + "})";

						// last iteration
						if (k == memoryControllers[i][j].size() - 1)
						{
							verilogFile << ");\n\n";
						}
						else
						{
							verilogFile << ",\n";
						}
					}
				}

				// WE controller
				if (controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == WE_READ_INCAPABLE_CONTROLLER)
				{
					if (controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB)
						verilogFile << "controller_WE #(.data_width(" << getBRAMPortSize(i,j, BRAMportBData, memories) << "),\n";
					else
						verilogFile << "controller_WE #(.data_width(" << getBRAMPortSize(i, j, BRAMportAData, memories) << "),\n";

					// if BRAM is not registered then say taht to the controller
					if (!memories[memIndex].portARegistered)
						verilogFile << "\t.isBRAMReg(0),\n";


					if (controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB)
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportBAddress].size() << "),\n";
					else
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportAAddress].size() << "),\n";

					verilogFile << "\t.Latency(" << TESTING_LATENCY << ")) controller_BRAM_" << BRAMPathOwner << "_" << BRAMNodeOwner
						<< " (.clk(CLK),\n"
						<< "\t.startTest(start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.reset(reset),\n"
						<< "\t.resetTest(reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

					if (controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA)
						verilogFile << "\t.BRAM_dataOut(PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_a_dataout),\n";
					else
						verilogFile << "\t.BRAM_dataOut(PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_b_dataout),\n";

					verilogFile << "\t.error(error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.done(done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";


					for (int k = 0; k < memoryControllers[i][j].size(); k++)
					{
						if (memoryControllers[i][j][k].first == BRAMportAWE)
							assert(false);
						else if (memoryControllers[i][j][k].first == BRAMportAData)
							verilogFile << "\t.data_in({";
						else if (memoryControllers[i][j][k].first == BRAMportBAddress)
							if (memories[memIndex].operationMode == simpleDualPort)
								verilogFile << "\t.port_r_address_optional({";
							else
								verilogFile << "\t.port_w_address({";
						else if (memoryControllers[i][j][k].first == BRAMportBData)
							verilogFile << "\t.data_in({";
						else if (memoryControllers[i][j][k].first == BRAMportAAddress)
							verilogFile << "\t.port_w_address({";
						else
						{
							assert(false);
						}

						int counter = 0;
						for (counter = memoryControllers[i][j][k].second - 1; counter >= 1; counter--)
						{
							verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
								+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + ",";
						}
						verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
							+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + "})";

						// last iteration
						if (k == memoryControllers[i][j].size() - 1)
						{
							verilogFile << ");\n\n";
						}
						else
						{
							verilogFile << ",\n";
						}
					}
				}

				// dataout
				if (controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
				{
					if (controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA || controllerType == DATAOUT_ROM_CONTROLLER_PORTA)
						verilogFile << "controller_dataOut #(.data_width(" << getBRAMPortSize(i, j, BRAMportAData, memories) << "),\n";
					else
						verilogFile << "controller_dataOut #(.data_width(" << getBRAMPortSize(i, j, BRAMportBData, memories) << "),\n";

					// if BRAM is not registered then say taht to the controller
					if (!memories[memIndex].portARegistered)
						verilogFile << "\t.isBRAMReg(0),\n";


					if (controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA || controllerType == DATAOUT_ROM_CONTROLLER_PORTA)
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportAAddress].size() << "),\n";
					else
						verilogFile << "\t.address_width(" << fpgaLogic[i][j][0].BRAMinputPorts[BRAMportBAddress].size() << "),\n";

					verilogFile << "\t.Latency(" << TESTING_LATENCY << ")) controller_BRAM_" << BRAMPathOwner << "_" << BRAMNodeOwner
						<< " (.clk(CLK),\n"
						<< "\t.startTest(start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.reset(reset),\n"
						<< "\t.resetTest(reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

					verilogFile << "\t.checkForErrorsOut(error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n"
						<< "\t.done(done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";


					for (int k = 0; k < memoryControllers[i][j].size(); k++)
					{
						
						if (memoryControllers[i][j][k].first == BRAMportAData)
							verilogFile << "\t.data_in({";
						else if (memoryControllers[i][j][k].first == BRAMportBAddress)
							if (memories[memIndex].operationMode == simpleDualPort)
								verilogFile << "\t.port_r_address_optional({";
							else
								verilogFile << "\t.port_w_address({";
						else if (memoryControllers[i][j][k].first == BRAMportBData)
							verilogFile << "\t.data_in({";
						else if (memoryControllers[i][j][k].first == BRAMportAAddress)
							verilogFile << "\t.port_w_address({";
						else if (memoryControllers[i][j][k].first == BRAMportAWE || memoryControllers[i][j][k].first == BRAMportBWE)
						{
							verilogFile << "\t.write_enable({";
						}
						else
						{
							assert(false);
						}

						int counter = 0;
						for (counter = memoryControllers[i][j][k].second - 1; counter >= 1; counter--)
						{
							verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
								+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + ",";
						}
						verilogFile << "PATH" + std::to_string(BRAMPathOwner) + "NODE" + std::to_string(BRAMNodeOwner)
							+ "_" + portNumbertoName(memoryControllers[i][j][k].first) + "_control_" + std::to_string(counter) + "})";

						// last iteration
						if (k == memoryControllers[i][j].size() - 1)
						{
							verilogFile << ");\n\n";
						}
						else
						{
							verilogFile << ",\n";
						}
					}

				}
			}
		}
	}



	//// input signals to sources
	verilogFile << std::endl << "//input signal to sources " << std::endl;
	if (sources.size() > 0)
	{
		verilogFile << "wire [" << sources.size() - 1 << ":0] Xin;" << std::endl;
		verilogFile << "wire [" << sources.size() - 1 << ":0] Xin_temp;" << std::endl << std::endl << std::endl;
		verilogFile << "// latency between toggling inputs is " << TESTING_LATENCY << std::endl;
		verilogFile << "genvar i;" << std::endl;
		verilogFile << "generate" << std::endl;
		verilogFile << "\tfor (i=0; i<" << sources.size() << "; i=i+1) begin: sourceModules" << std::endl;
		verilogFile << "\t\t sourceExcitation #(.latency(" << TESTING_LATENCY << ")) srcModule (.reset(reset), .clk(CLK), .out(Xin_temp[i]), .outBefore());" << std::endl;
		verilogFile << "\tend" << std::endl;
		verilogFile << "endgenerate" << std::endl;
	}
	verilogFile << "wire vcc, gnd;" << std::endl;
	verilogFile << "assign vcc = 1'b1;" << std::endl;
	verilogFile << "assign gnd = 1'b0;" << std::endl;

	verilogFile << std::endl << "//connections between counter and controller " << std::endl;
	verilogFile << "wire reset_counter, timerReached;";

	verilogFile << std::endl << "//connections between source registers and controller " << std::endl;
	if (sources.size()>0)
		verilogFile << "wire [" << sources.size() - 1 << ":0] set_source_registers;" << std::endl;

	// always block to genreate t-flipflop which feeds all sources
	verilogFile << std::endl << "//input signal to sources " << std::endl;
	if (sources.size() > 0)
	{
		//verilogFile << "always @ (posedge CLK or posedge reset) begin" << std::endl;
		//verilogFile << "\tif (reset) " << std::endl;
		//verilogFile << "\t\tXin <= " << sources.size() << "'b0;" << std::endl;
		//verilogFile << "\telse " << std::endl;
		//verilogFile << "\t\tXin <= Xin_temp | set_source_registers;" << std::endl;
		//verilogFile << "end" << std::endl;

		verilogFile << "assign Xin = Xin_temp | set_source_registers;" << std::endl;
	}

	/// counter to be connected to controller
	verilogFile << std::endl << "//counter to count how long to stay at each test phase " << std::endl;
	verilogFile << "counter_testing count0 (.CLK(CLK),.clr(reset_counter), .timerReached(timerReached));";


	// cascaded stuff
	verilogFile << std::endl << "//cascaded MUXES and tFFs " << std::endl;
	for (i = 0; i < (int)cascadedControlSignals.size(); i++) // for each cascaded control signal we instantiate a Tff and a 2to1 mux
	{
		verilogFile << "sourceExcitation  #(.latency(" << TESTING_LATENCY
			<< ")) t_" << i << " (.reset(reset), .out( source_path" << cascadedControlSignals[i].path 
			<< "_node" << cascadedControlSignals[i].node 
			<< "), .clk(CLK), .outBefore());" 
			<< std::endl;
		verilogFile << "mux2to1 mux_" << i 
			<< "(.in0( source_path" << cascadedControlSignals[i].path << "_node" 
			<< cascadedControlSignals[i].node << "), .in1( PATH" 
			<< cascadedControlSignals[i].path << "NODE" 
			<< cascadedControlSignals[i].node << "F ), .s( cascaded_selector_path"
			<< cascadedControlSignals[i].path << "_node" << cascadedControlSignals[i].node << " ), .out (PATH"
			<< cascadedControlSignals[i].path << "NODE" << cascadedControlSignals[i].node << "F_cascaded)); "
			<< std::endl;

	}
	verilogFile << std::endl << std::endl;



	//////////////////////////////////////////////
	////////////// Instantiate controller ////////
	//////////////////////////////////////////////



	verilogFile << std::endl << "//create the controller " << std::endl;
	verilogFile << "controller control0(.CLK(CLK),\n"
		<< "\t.start_test(start_test),\n"
		<< "\t.reset(reset),\n"
		<< "\t.error(error),\n"
		<< "\t.timer_reached(timerReached),\n"
		<< "\t.reset_counter(reset_counter),\n";

	// signals between the main controller and the BRAM controllers


	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
			int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
			int addressIndex = -1;
			int testedPortSize = -1;
			int controllerType = -1;
			if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
			{
				verilogFile << "\t.error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM("
					<<"error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

				verilogFile << "\t.done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM("
					<< "done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

				verilogFile << "\t.reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM("
					<< "reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";

				verilogFile << "\t.start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM("
					<< "start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";
				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
				{
					verilogFile << "\t.info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM("
						<< "info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM),\n";
				}

			}
		}
	}


	verilogFile << "\t.controlSignals({";

	for (i = 0; i < (int)controlSignals.size() - 1; i++)
	{
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F, ";
	}
	verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
	verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F}),\n\t";
	verilogFile << ".sinks({";

	int memX, memY, memZ;
	// paths should be like this as sinks[0] should be sink(0)
	for (i = ((int)sinks.size()) - 1; i > 0; i--)
	{
		// check if this sink corresponds to a memory
		memX = paths[sinks[i].path][sinks[i].node].x;
		memY = paths[sinks[i].path][sinks[i].node].y;
		memZ = paths[sinks[i].path][sinks[i].node].z;

		// this is a memory block that we need to test it's address bit
		// address from a read capable port
		if (fpgaLogic[memX][memY][memZ].isBRAM)
		{
			// port A used
			if (fpgaLogic[memX][memY][memZ].portAInputCount > 0)
			{
				assert(fpgaLogic[memX][memY][memZ].portBInputCount == 0);
				verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "_a_dataout[0], ";
			}
			else
			{
				// port B used
				assert(fpgaLogic[memX][memY][memZ].portAInputCount == 0);
				verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "_b_dataout[0], ";

			}
		}
		else
		{
			verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << ", ";
		}
		
	}

	// the last sink
	// check if this sink corresponds to a memory
	memX = paths[sinks[i].path][sinks[i].node].x;
	memY = paths[sinks[i].path][sinks[i].node].y;
	memZ = paths[sinks[i].path][sinks[i].node].z;

	// this is a memory block that we need to test it's address bit
	// address from a read capable port
	if (fpgaLogic[memX][memY][memZ].isBRAM)
	{
		// port A used
		if (fpgaLogic[memX][memY][memZ].portAInputCount > 0)
		{
			assert(fpgaLogic[memX][memY][memZ].portBInputCount == 0);
			verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "_a_dataout[0], }),";
		}
		else
		{
			// port B used
			assert(fpgaLogic[memX][memY][memZ].portAInputCount == 0);
			verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "_b_dataout[0] }),";

		}
	}
	else
	{
		verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "}), ";
	}



	



	// cascaded LUTs mux selector
	if (cascadedControlSignals.size() > 0)
	{
		verilogFile << "\n\t.cascaded_select({ cascaded_selector_path" << cascadedControlSignals[0].path << "_node" << cascadedControlSignals[0].node;
	}

	for (i = 1; i < (int)cascadedControlSignals.size(); i++)
	{
		cascPath = cascadedControlSignals[i].path;
		cascNode = cascadedControlSignals[i].node;
		verilogFile << ", cascaded_selector_path" << cascPath << "_node" << cascNode;
	}
	if (cascadedControlSignals.size() > 0)
		verilogFile << "}), ";

/*
	// memors sinks begin
	int memPath, memNode;
	if (memorySinks.size() > 0)
	{
		memPath = memorySinks[i].path;
		memNode = memorySinks[i].node;
		int memX, memY, memZ;
		memX = paths[memPath][memNode].x;
		memY = paths[memPath][memNode].y;
		memZ = paths[memPath][memNode].z;
		assert(memZ == 0 && fpgaLogic[memX][memY][memZ].isBRAM);
		verilogFile << ".memorySink({ cascaded_selector_path" << memorySinks[0].path << "_node" << memorySinks[0].node;
		if (fpgaLogic[memX][memY][memZ].portAInputCount > 0)
		{
			assert(fpgaLogic[memX][memY][memZ].portBInputCount == 0);
			verilogFile << ".memorySink({ PATH" << memPath << "NODE" << memNode << "_a_dataout[0]";
		}
		else
		{
			assert(fpgaLogic[memX][memY][memZ].portAInputCount == 0);
			verilogFile << ".memorySink({ PATH" << memPath << "NODE" << memNode << "_b_dataout[0]";

		}

	}
	
	
	for (i = 1; i < (int)memorySinks.size(); i++)
	{
		memPath = memorySinks[i].path;
		memNode = memorySinks[i].node;
		int memX, memY, memZ;
		memX = paths[memPath][memNode].x;
		memY = paths[memPath][memNode].y;
		memZ = paths[memPath][memNode].z;
		assert(memZ == 0 && fpgaLogic[memX][memY][memZ].isBRAM);

		// check which BRAM port is used
		// currently only testing address in for read able port
		// todo: extend this to cover all ports
		if (fpgaLogic[memX][memY][memZ].portAInputCount > 0)
		{
			assert(fpgaLogic[memX][memY][memZ].portBInputCount == 0);
			verilogFile << ", PATH" << memPath << "NODE" << memNode << "_a_dataout[0]";
		}
		else
		{
			assert(fpgaLogic[memX][memY][memZ].portAInputCount == 0);
			verilogFile << ", PATH" << memPath << "NODE" << memNode << "_b_dataout[0]";

		}

		//verilogFile << ", cascaded_selector_path" << cascPath << "_node" << cascNode;
	}
	if (memorySinks.size() > 0)
		verilogFile << "}), ";




	// memory sinks end
	*/

	verilogFile << ".finished_one_iteration(fuck)";
	if (sources.size() > 0)
		verilogFile << ",\n\t.set_source_registers(set_source_registers));" << std::endl;
	else
		verilogFile << ");" << std::endl;

	// merge verilogSecondPart and verilog into one file

	verilogFile << std::endl;
	verilogFile << std::endl;

	std::string line;
	while (std::getline(verilogFileSecondPart, line)) // only loop when read succeeds
	{
		// use line here
		verilogFile << line << '\n'; // copy line to output
	}


	verilogFile.close();
	verilogFileSecondPart.close();
}


bool isBRAMControllerUsedInThisTestPhase(int iBRAM, int jBRAM, int testPhase, int & testedAddressPortIndex, int & controllerType)
{




	if (memoryControllers[iBRAM][jBRAM].size() > 0) // memory controller is needed
	{
		int BRAMPathOwner = fpgaLogic[iBRAM][jBRAM][0].owner.path;
		int BRAMNodeOwner = fpgaLogic[iBRAM][jBRAM][0].owner.node;
		bool BRAMCurrentlyTested = false;


		testedAddressPortIndex = -1;
		// we will define the controller type abased on what ports need to be controlled
		// summing the 2^port gives a unique number for each controller type
		controllerType = 0;
		//int addressIndex = -1;
		for (int kBRAM = 0; kBRAM < memoryControllers[iBRAM][jBRAM].size(); kBRAM++)
		{
			controllerType += pow(2, memoryControllers[iBRAM][jBRAM][kBRAM].first);

		//	if (memoryControllers[iBRAM][jBRAM][kBRAM].first == BRAMportBAddress)
			//	addressIndex = kBRAM;
		}
		// not checking data out controller as it cannot be a sink
		// dataout controller are hooked up to BRAMs that are sources to paths
		if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
			|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
			|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
			|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
			|| controllerType == WE_READ_INCAPABLE_CONTROLLER
			|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
			|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB)
		{
			// this BRAM is tested, so let's check if any path ends there and being tested in the current test phase
			for (int k = 0; k < (int)fpgaLogic[iBRAM][jBRAM][0].nodes.size(); k++)
			{
				// ensure that the path size is larger than 1 
				//--> it's less than or equal one if it's buried inside a BRAM
				if (paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path].size() <= 1)
					continue;

				if (paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][0].testPhase == testPhase)
				{
					if (fpgaLogic[iBRAM][jBRAM][0].nodes[k].node == 0) // if this node is a source then we shouldnt be checking the output of this FF for erro. THis only happens in cascaded cases.
						continue;

					assert(!paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][0].deleted);

					// tested must be an address port of port A
				//	assert(paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][fpgaLogic[iBRAM][jBRAM][0].nodes[k].node].BRAMPortIn == BRAMportAAddress);
					// return which pin is being tested
					testedAddressPortIndex = paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][fpgaLogic[iBRAM][jBRAM][0].nodes[k].node].BRAMPortInIndex;

					BRAMCurrentlyTested = true;
					break;
				}
			}
		}
		else if(controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
			|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
			|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
			|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
			|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
		{
			// this BRAM is tested, so let's check if any path starts here and being tested in the current test phase
			for (int k = 0; k < (int)fpgaLogic[iBRAM][jBRAM][0].nodes.size(); k++)
			{
				// ensure that the path size is larger than 1 
				//--> it's less than or equal one if it's buried inside a BRAM
				if (paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path].size() <= 1)
					continue;

				if (paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][0].testPhase == testPhase)
				{
					// if this node is not a source then contniue as we are only looking for sources
					if (fpgaLogic[iBRAM][jBRAM][0].nodes[k].node != 0) 
						continue;

					assert(!paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][0].deleted);

					testedAddressPortIndex = -1;// paths[fpgaLogic[iBRAM][jBRAM][0].nodes[k].path][fpgaLogic[iBRAM][jBRAM][0].nodes[k].node].BRAMPortInIndex;

					BRAMCurrentlyTested = true;
					break;
				}
			}
		}
		return BRAMCurrentlyTested;
	}
	return false;
}



// return true if BRAM at i and j has a controller
// returnts the controller type and the size of the tested port
bool isBRAMwithController(int i, int j, int & testedPortSize, int & controllerType, std::vector<BRAM>  memories)
{

	
	if (memoryControllers[i][j].size() > 0) // memory controller is needed
	{
		int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
		int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
		// we will define the controller type abased on what ports need to be controlled
		// summing the 2^port gives a unique number for each controller type
		controllerType = 0;
		testedPortSize = -1;
		for (int k = 0; k < memoryControllers[i][j].size(); k++)
		{
			controllerType += pow(2, memoryControllers[i][j][k].first);
		//	if (memoryControllers[i][j][k].first == BRAMportBAddress)
		//		addressIndex = k;
		}

		if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
			|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
			|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
			|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
			|| controllerType == WE_READ_INCAPABLE_CONTROLLER
			|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
			|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB
			|| controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
			|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
			|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
			|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
			|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
		{

			testedPortSize = -1;
			if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER)
			{
				testedPortSize = getBRAMPortSize(i, j, BRAMportBAddress, memories);
				
			}

			if (controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
				|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA)
			{
				assert(testedPortSize == -1);
				testedPortSize = getBRAMPortSize(i, j, BRAMportAData, memories);
				
			}

			if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
			{
				assert(testedPortSize == -1);
				testedPortSize = getBRAMPortSize(i, j, BRAMportBData, memories);
			}

			if (controllerType == WE_READ_INCAPABLE_CONTROLLER
				|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
				|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB)
			{
				assert(testedPortSize == -1);
				testedPortSize = 1;
			}




			return true;
		}
	}

	return false;


}
void create_controller_module(std::vector <Path_logic_component> sinks,
	std::vector <Path_logic_component> controlSignals, 
	std::vector <Path_logic_component> sources,
	std::vector <Path_logic_component> cascadedControlSignals,
	std::vector<BRAM>  memories,
	int bitStreamNumber)
{

	std::ofstream controllerFile;
	std::string controllerFileName = "controller_" + std::to_string(bitStreamNumber) + ".v";
	controllerFile.open(controllerFileName);
	// number of phases stored in numberOfStates.
	// controller module will output all control signals and ((reset signals for all sources to paths)), takes input start tests and all sinks from the tested paths, outputs when it has completed one loop over all paths and error signal, input also when counter is full and outputs reset to the counter 
	controllerFile << "module controller ( input CLK,\n\tinput start_test,\n\tinput reset,\n\toutput reg error,\n\tinput timer_reached,\n\toutput reg reset_counter,\n\t";

	int i, j, k, l, path_t, node_t, portInToCombout, portIn_t;
	int testPhaseCount = 0;
	bool inverting_t;
	bool currentlyTested;
	std::vector <std::vector<std::pair <int, int>>> errorSignalDivision; // store error signals of each test phase to be used as input to the or tree network
	errorSignalDivision.resize(numberOfTestPhases);
	/// group paths in the same test phase together
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);

	for (i = 1; i < (int)paths.size(); i++)
	{
		// path buried in a BRAM
		if (paths[i].size() <= 1)
			continue;

		if (!paths[i][0].deleted)
			test_structure[paths[i][0].testPhase].push_back(i);
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}


	// control signals
	// option 1
	controllerFile << "output [" << 2 * controlSignals.size() - 1 << ":0] controlSignals,\n\t";
	// option 2
	/*	for (i = 0; i < controlSignals.size(); i++)
	{
	controllerFile << "output PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
	controllerFile << "output PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F, ";
	}
	// finish option 2*/
	// option 1
	controllerFile << "input [" << sinks.size() - 1 << ":0] sinks,\n\t";

	// memory Sinks
	//if (memorySinks.size())
	//	controllerFile << "input [" << memorySinks.size() - 1 << ":0] memorySinks,";


	//option 2
	/*	// sink signals from tested paths
	for (i = 0; i < sinks.size(); i++)
	{
	controllerFile << "input PATH" << sinks[i].path << "NODE" << sinks[i].node << ", ";

	}
	// option 2 finish */
	// set signal for source registers 
	if (sources.size()>0) // when testing only cascaded path we dont have any sources.
		controllerFile << "output reg [" << sources.size() - 1 << ":0] set_source_registers,\n\t";

	// cascaded LUTs mux selector cascaded_select
	if (cascadedControlSignals.size() > 0)
		controllerFile << "output [" << cascadedControlSignals.size() - 1 << ":0] cascaded_select,\n\t";


	////////////////////////////////////////
	// control signals to and from BRAMs////
	////////////////////////////////////////
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			if (memoryControllers[i][j].size() > 0) // memory controller is needed
			{
				int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
				int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;

				// we will define the controller type abased on what ports need to be controlled
				// summing the 2^port gives a unique number for each controller type
				int controllerType = 0;
				int addressIndex = -1;
				for (int k = 0; k < memoryControllers[i][j].size(); k++)
				{
					controllerType += pow(2, memoryControllers[i][j][k].first);

				//	if (memoryControllers[i][j][k].first == BRAMportBAddress)
				//		addressIndex = k;
				}

				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == WE_READ_INCAPABLE_CONTROLLER
					|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
				{
					//assert(addressIndex >= 0);
					controllerFile << "// Control signals to BRAM test controller \n\t";

					// this means this BRAM is accompanied by memory controller
					controllerFile << "input error_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					controllerFile << "input done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					controllerFile << "output reg reset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					controllerFile << "output reg start_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";

					int to_test = -1;
					if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER)
					{
						to_test = getBRAMPortSize(i, j, BRAMportBAddress, memories);
						controllerFile << "output reg [" << /*memoryControllers[i][j][addressIndex].second*/ to_test - 1 << ":0] info_to_test_PATH" 
							<< BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					}

					if (controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
						|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA)
					{
						assert(to_test == -1);
						to_test = getBRAMPortSize(i, j, BRAMportAData, memories);
						controllerFile << "output reg [" << /*memoryControllers[i][j][addressIndex].second*/ to_test - 1 << ":0] info_to_test_PATH"
							<< BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					}

					if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
					{
						assert(to_test == -1);
						to_test = getBRAMPortSize(i, j, BRAMportBData, memories);
						controllerFile << "output reg [" << /*memoryControllers[i][j][addressIndex].second*/ to_test - 1 << ":0] info_to_test_PATH"
							<< BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM,\n\t";
					}
				}
			}
		}
	}

	////////////////////////////////////////////////
	// end control signals to and from BRAM/////////
	////////////////////////////////////////////////


	controllerFile << "output reg finished_one_iteration );" << std::endl;

	// create reg to hold the buffered values of the sinks, these values will be xored with the values from the sinks to ensure that the value changes every cycle
	controllerFile << "reg [" << sinks.size() - 1 << ":0] sinks_buff , sinks_buff_buff ;" << std::endl;
	controllerFile << "reg [" << 2 * controlSignals.size() - 1 << ":0] controlSignals_temp;" << std::endl;

	// create regs to hold the buffered values of memory sinks
//	if (memorySinks.size() > 0)
//	{
//		controllerFile << "reg [" << memorySinks.size() - 1 << ":0] memorySinks_buff , memorySinks_buff_buff ;" << std::endl;
//		controllerFile << "reg [" << memorySinks.size() - 1 << ":0] memoryErrorVec;" << std::endl;
//	}

	if (cascadedControlSignals.size() > 0)
		controllerFile << "reg [" << cascadedControlSignals.size() - 1 << ":0] cascaded_select_temp; " << std::endl;


	controllerFile << "reg [" << sinks.size() - 1 << ":0] errorVec;" << std::endl;
	controllerFile << "reg error_temp;" << std::endl;
	// create states
	controllerFile << "reg [" << ceil(log2(2 * numberOfTestPhases + 2)) << ":0] state;" << std::endl;
	controllerFile << "wire [" << numberOfTestPhases - 1 << ":0]" << "errorTestPhase;" << std::endl;
	controllerFile << "parameter Idle = " << ceil(log2(2 * numberOfTestPhases + 2)) << "'d0, ";



	for (i = 0; i < numberOfTestPhases; i++)
	{

		controllerFile << "Test_phase_" << i << " = " << ceil(log2(2 * numberOfTestPhases + 2)) << "'d" << i + 1 << ", ";
	}

	for (i = 0; i < numberOfTestPhases; i++)
	{
		controllerFile << "Reset_phase_" << i << " = " << ceil(log2(2 * numberOfTestPhases + 2)) << "'d" << i + 1 + numberOfTestPhases << ", ";
		//controllerFile << "reset_from_phase_" << i << " = " << i + 1 + numberOfTestPhases << ", ";
	}
	controllerFile << "Done_One_Iteration" << " = " << ceil(log2(2 * numberOfTestPhases + 2)) << "'d" << i + 1 + numberOfTestPhases << "; " << std::endl;

	// reset counter
	controllerFile << "reg reset_or_network; // reset or network pipelines to zeroes" << std::endl;
#ifdef shiftRegOrTree
	controllerFile << "wire reset_or_network_shifted; // shifted reset to allow some time after reaching a test phase where error is ignored" << std::endl;
#endif
	controllerFile << "reg reset_counter_reset; // reset counter for reset phase" << std::endl;
	controllerFile << "wire timer_reached_reset; // timer reached value for reset signal to switch to testing state" << std::endl;
	controllerFile << "wire timer_reached_sticky; // timer reached sticky to change from the testing state" << std::endl;
	controllerFile << "reg timer_reached_sticky_reset; // reset timer reached sticky to become zero, should be high in all reset states" << std::endl;

	// enable Error checking
	controllerFile << "// enable error checking" << std::endl;
	controllerFile << "wire enableErrorChecking; " << std::endl << std::endl;
	controllerFile << "reg [" << sinks.size() - 1 << ":0] enableErrorCheckingTemp; " << std::endl;

	// sticky module for timer reahed
	controllerFile << "// sticky modules for timer and done signals from BRAM is they exist\n\n\n";
	controllerFile << "sticky_register timerReachedStickyReg (.clk(CLK), .d(timer_reached), .reset(timer_reached_sticky_reset), .q(timer_reached_sticky));\n\n" << std::endl;




	////////////////////////////////////////
	// sticky modules for BRAM done signal//
	////////////////////////////////////////
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			if (memoryControllers[i][j].size() > 0) // memory controller is needed
			{
				int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
				int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;

				// we will define the controller type abased on what ports need to be controlled
				// summing the 2^port gives a unique number for each controller type
				int controllerType = 0;
				for (int k = 0; k < memoryControllers[i][j].size(); k++)
				{
					controllerType += pow(2, memoryControllers[i][j][k].first);
				}

				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == WE_READ_INCAPABLE_CONTROLLER
					|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == WE_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
					|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
					|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
				{
					controllerFile << "wire done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_sticky;\n";
					controllerFile << "sticky_register done_sticky_register_PATH" << BRAMPathOwner  << "NODE" 
						<< BRAMNodeOwner << " (.clk(CLK), .d(done_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner
						<< "_BRAM), .reset(timer_reached_sticky_reset), .q(done_PATH" << BRAMPathOwner
						<< "NODE" << BRAMNodeOwner << "_BRAM_sticky));\n" << std::endl;
				}
			}
		}
	}

	////////////////////////////////////////////
	// end sticky modules for BRAM done signal//
	////////////////////////////////////////////






	// always block to get the buffered sinks
	controllerFile << "always @ (posedge CLK) begin" << std::endl;

	// option 1
	/*for (i = 0; i < sinks.size(); i++)
	{
	controllerFile << "\tsinks_buff[" << i << "] <= sinks[" << i << "];" << std::endl;
	}*/
#ifdef sinksLoad
	controllerFile << "\tif (reset) begin" << std::endl;
	controllerFile << "\t\tsinks_buff <= " << sinks.size() << "'b0;" << std::endl;
	controllerFile << "\t\tsinks_buff_buff <= " << sinks.size() << "'b0;" << std::endl;
	controllerFile << "\tend" << std::endl;
	controllerFile << "\telse begin" << std::endl;
	controllerFile << "\t\tsinks_buff <= sinks;" << std::endl;
	controllerFile << "\t\tsinks_buff_buff <= sinks_buff;" << std::endl;
	//controllerFile << "\t\terrorVec <= ~(sinks_buff^sinks_buff_buff) ; " << std::endl; //  
	//controllerFile << "\t\terrorVec <= ({"<< sinks.size() <<"{enableErrorChecking}}) & ~(sinks_buff^sinks_buff_buff) ; " << std::endl;
	controllerFile << "\t\terrorVec <= (enableErrorCheckingTemp) & ~(sinks_buff^sinks_buff_buff) ; " << std::endl;
	controllerFile << "\t\terror <= error_temp;" << std::endl;
	//	controllerFile << "\t\tcontrolSignals <= controlSignals_temp;" << std::endl;
	controllerFile << "\tend" << std::endl;
#endif

#ifndef sinksLoad
	controllerFile << "\tsinks_buff <= sinks;" << std::endl;
	controllerFile << "\tsinks_buff_buff <= sinks_buff;" << std::endl;
	controllerFile << "\terrorVec <= ~(sinks_buff^sinks_buff_buff) ; " << std::endl; //  
	controllerFile << "\terror <= error_temp;" << std::endl;
#endif
	// option 2
	/*	for (i = 0; i < sinks.size(); i++)
	{
	controllerFile << "\tsinks_buff[" << i << "] <= PATH" << sinks[i].path << "NODE" << sinks[i].node << ";" << std::endl;
	}*/
	// option 2 finish
	controllerFile << "end" << std::endl << std::endl << std::endl;


	controllerFile << "wire srcIn, srcIn_before;" << std::endl << std::endl;

	controllerFile << "wire srcTgl;" << std::endl << std::endl;

	controllerFile << "// get the time when the toggling of the input source occurs" << std::endl;
	controllerFile << "sourceExcitation #(.latency(" << TESTING_LATENCY << ")) srcModule (.reset(reset), .clk(CLK), .out(srcIn), .outBefore(srcIn_before));" << std::endl << std::endl;

	controllerFile << "assign srcTgl = srcIn ^ srcIn_before;" << std::endl << std::endl;


	controllerFile << "// 4 cycles after the input source toggle, the sink register should toggle" << std::endl;
	controllerFile << "shift_reg #(.L(4)) shiftRegSrcTgl(.CLK(CLK),.in(srcTgl),.out(enableErrorChecking));" << std::endl << std::endl;


	controllerFile << std::endl << "assign controlSignals = controlSignals_temp;" << std::endl;

	if (cascadedControlSignals.size() > 0)
		controllerFile << std::endl << "assign cascaded_select = cascaded_select_temp; " << std::endl << std::endl;

	controllerFile << "//////////////////////////////////////////////////////////////////" << std::endl;
	controllerFile << "// starting the Sequential part of the FSM changing between states" << std::endl;
	controllerFile << "//////////////////////////////////////////////////////////////////" << std::endl;

	///////////////////////////// 
	//sequential part of the FSM, state transitions
	////////////////////////////

	controllerFile << "reg done_state;" << std::endl;

	controllerFile << "always @ (posedge CLK or posedge reset) begin" << std::endl;
	controllerFile << "\tif (reset)" << std::endl;
	controllerFile << "\t\tstate <= Idle;" << std::endl;
	controllerFile << "\telse " << std::endl;
	///// in idle state
	controllerFile << "\t\tcase (state)" << std::endl;
	controllerFile << "\t\t\tIdle : " << std::endl;
	controllerFile << "\t\t\t\tif (start_test)" << std::endl;
	controllerFile << "\t\t\t\t\tstate <= Reset_phase_0;" << std::endl;
	controllerFile << "\t\t\t\telse " << std::endl;
	controllerFile << "\t\t\t\t\tstate <= Idle;" << std::endl;
	///////////// all reset and test phases states
	for (i = 0; i < numberOfTestPhases; i++)
	{
		controllerFile << "\t\t\tReset_phase_" << i << " : " << std::endl;
		controllerFile << "\t\t\t\tif (timer_reached_reset)" << std::endl;
		controllerFile << "\t\t\t\t\tstate <= Test_phase_" << i << "; " << std::endl;
		controllerFile << "\t\t\t\telse " << std::endl;
		controllerFile << "\t\t\t\t\tstate <= Reset_phase_" << i << "; " << std::endl;

		controllerFile << "\t\t\tTest_phase_" << i << " : " << std::endl;
		controllerFile << "\t\t\t\tif (timer_reached_sticky & done_state)" << std::endl;
		if (i < numberOfTestPhases - 1)
			controllerFile << "\t\t\t\t\tstate <= Reset_phase_" << i + 1 << "; " << std::endl;
		else
			controllerFile << "\t\t\t\t\tstate <= Done_One_Iteration;" << std::endl;
		controllerFile << "\t\t\t\telse " << std::endl;
		controllerFile << "\t\t\t\t\tstate <= Test_phase_" << i << "; " << std::endl;
	}
	/////// done one iteration state
	controllerFile << "\t\t\tDone_One_Iteration : " << std::endl;
	controllerFile << "\t\t\t\tstate <= Reset_phase_0;" << std::endl;
	controllerFile << "\t\t\tdefault : " << std::endl;
	controllerFile << "\t\t\t\tstate <= Idle;" << std::endl;
	controllerFile << "\tendcase" << std::endl << std::endl << std::endl;
	controllerFile << "end" << std::endl << std::endl << std::endl;


	controllerFile << "////////////////////////////////////////////////////////////////////////////////////////////" << std::endl;
	controllerFile << "// starting the Combinational part of the FSM, assigning inputs and outputs for every stage" << std::endl;
	controllerFile << "///////////////////////////////////////////////////////////////////////////////////////////" << std::endl;


	///////////////////////////// 
	//combinational part of the FSM 
	///////////////////////////

	// check if there are any address_to_test
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
			int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
			int testedPortSize = -1;
			int controllerType = -1;

			
			if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
			{
				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
				{

					controllerFile << "reg [" << testedPortSize - 1 << " : 0] info_to_test_PATH"
						<< BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling;\n";
				}

			}
		}
	}


	controllerFile << "always @ (*) begin" << std::endl;
	controllerFile << "//default values " << std::endl;

	// BRAM start_test and reset_test default values

	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{			
			int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
			int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
			int testedPortSize = -1;
			int controllerType = -1;
			if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
			{
				controllerFile << "\treset_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM <= 1'b0;\n";
				controllerFile << "\tstart_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM <= 1'b0;\n";

				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
				{
					controllerFile << "\tinfo_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling <= {"
						<< testedPortSize << "{1'b0}};\n";
				}
				
			}		
		}
	}
	controllerFile << "\tenableErrorCheckingTemp <= ({ " << sinks.size() << "{enableErrorChecking}});" << std::endl;
	controllerFile << "\ttimer_reached_sticky_reset <= 1'b0;\n";
	controllerFile << "\tdone_state <= 1'b1;\n";
	controllerFile << "\tcase (state)" << std::endl;
	controllerFile << "\t\tIdle : begin" << std::endl;
	//RESET state: in reset state fix all LUTs to a fixed output and todo: reset all source signals
	// option 1
	//	controllerFile << "\t\t\tcontrolSignals <= " << 2 * controlSignals.size()  << "'b";
	//	for (i = 0; i < controlSignals.size(); i++)
	//		controllerFile << "01";
	//	controllerFile << "; " << std::endl;

	//controllerFile << "\t\t\tcontrolSignals <= {" << 2 * controlSignals.size() << "{1'b1}};" << std::endl;

	int x, y, z, feederPath, feederNode;
	feederNode = -1;
	feederPath = -1;
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////// TRIAL BETTER EFACTOR LATER////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef controlSignalIdleStateReduction
	controllerFile << "\t\t\tcontrolSignals_temp <= " << 2 * controlSignals.size() << "'b";
	std::string controlSignalsDefaultValue;
	i = 0;
	for (j = 0; j < controlSignals.size(); j++)
	{
		// find x,y,z location of that wysiwyg
		x = paths[controlSignals[j].path][controlSignals[j].node].x;
		y = paths[controlSignals[j].path][controlSignals[j].node].y;
		z = paths[controlSignals[j].path][controlSignals[j].node].z;
		path_t = -1;
		node_t = -1;
		portInToCombout = -1;
		portIn_t = -1;
		inverting_t = false; // default value of inverting, chosen randomly with no specific reason. If combout is used then later code will set this bool accordingly.
							 // loop across nodes using this WYSIWYG to see of any path using this LUT is tested at the current phase
		for (k = 0; k < fpgaLogic[x][y][z].nodes.size(); k++)
		{
			if (paths[fpgaLogic[x][y][z].nodes[k].path][0].testPhase == i) // this path should be tested in the current test phase
			{
				// the path cant be deleted AND assigned a test phase higher than -1
				assert(!paths[fpgaLogic[x][y][z].nodes[k].path][0].deleted);
				if (path_t == -1) // should add break if this condition is true, but as a double check I continue checking alll paths that are tested at this phase and make sure that all of them use the same portin by the assert statement.
				{
					path_t = fpgaLogic[x][y][z].nodes[k].path;
					node_t = fpgaLogic[x][y][z].nodes[k].node;
					portIn_t = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn;
				}
				assert(portIn_t == paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn);
				if (paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portOut == Combout)
				{
					if (portInToCombout == -1)
					{
						portInToCombout = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn;
						inverting_t = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].inverting;
					}
				}

			}
		}
		// at this point if any path in element x,y,z is assigned to be tested at the current phase, path_t, node_t should be > -1 and portIn indicates the input port used

		if (path_t == -1) // this element has no paths that should be tested in the current phase, so set control signals to stop it
		{
			//option 1
			controllerFile << "01";
			controlSignalsDefaultValue.push_back('0');
			controlSignalsDefaultValue.push_back('1');
			// option 2
			/*	controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "Con <= 1'b0; " << std::endl;
			controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;*/
		}
		else/* if (!paths[controlSignals[j].path][0].deleted)*/// this should be tested in the current state, check if it is deleted. Actually not needed
		{
			// set con to 1
			// option 1 
			controllerFile << "1";
			controlSignalsDefaultValue.push_back('1');
			// option 2
			//controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "Con <= 1'b1; " << std::endl;
			// assign F as if the signal tranisition is inverting
			// important to remember F has no control on Cout. Cout is adjusted by adjusting the other inputs when creating the WYSIWYGs
			// F only controls the transition to Combout, this depends on the LUT mask created. So lets figure out how to choose F to get the edge we want man		
			if (fpgaLogic[x][y][z].LUTMask == "D77D" || fpgaLogic[x][y][z].LUTMask == "D728")
			{
				// there are two differnet LUT configuration that use this mask one with cout and one withou
				if (fpgaLogic[x][y][z].outputPorts[Cout - 5]) // cout is one of the outputs and cin is used for sure
				{
					assert(fpgaLogic[x][y][z].inputPorts[Cin]);
					if (fpgaLogic[x][y][z].usedInputPorts == 1)
					{
						//	assert(portInToCombout == -1); //since cout is used and only one input (which is cin) is used. not sure why I added this assert signal 
						// for inverting with only one input used F should be 0. Since we set the other non used input to vcc.
						if (inverting_t)
						{
							controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
							controlSignalsDefaultValue.push_back('0');
						}
						else
						{
							controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
							controlSignalsDefaultValue.push_back('1');
						}
					}
					else // 2 input ports are used
					{
						if (/*fpgaLogic[x][y][z].inputPorts[Cin] &&*/ portIn_t != Cin) // cin is used, and the current path is using an other input port
						{
							// find what coud does the feeder of this in gives when it is turned off
							//	if (!get_feeder(x, y, z, Cin, feederPath, feederNode))
							//		assert(false);
							assert(get_feeder(x, y, z, Cin, feederPath, feederNode));
							// if the feeder defaults cout to zero
							//	std::cout << feederNode << std::endl;
							//	std::cout << feederPath << std::endl;
							if (fpgaLogic[paths[feederPath][feederNode].x][paths[feederPath][feederNode].y][paths[feederPath][feederNode].z].CoutFixedDefaultValue)
							{
								if (inverting_t)
								{
									controllerFile << "1"; // option 2controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									controlSignalsDefaultValue.push_back('1');
								}
								else
								{
									controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									controlSignalsDefaultValue.push_back('0');
								}
							}
							else // feeeder defaults Cout to 1
							{
								if (inverting_t)
								{
									controllerFile << "0"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									controlSignalsDefaultValue.push_back('0');
								}
								else
								{
									controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									controlSignalsDefaultValue.push_back('1');
								}
							}
						}
						else //  the current input is cin. inputs coming from combout are set to 1 when off 
						{
							if (inverting_t)
							{
								controllerFile << "0"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
								controlSignalsDefaultValue.push_back('0');
							}
							else
							{
								controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								controlSignalsDefaultValue.push_back('1');
							}
						}

					}

				}
				else // output is only combout
				{
					assert(fpgaLogic[x][y][z].LUTMask == "D77D");
					if (fpgaLogic[x][y][z].usedInputPorts == 1)
					{
						assert(portInToCombout > -1);
						// for inverting with only one input used F should be 1. Since we set the other non used input to gnd.
						if (inverting_t)
						{
							controllerFile << "1";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
							controlSignalsDefaultValue.push_back('1');
						}
						else
						{
							controllerFile << "0";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
							controlSignalsDefaultValue.push_back('0');
						}
					}
					else // 2 input ports are used
					{
						if (fpgaLogic[x][y][z].inputPorts[Cin] && portIn_t != Cin) // cin is used, and the current path is using an other input port
						{
							// find what coud does the feeder of this in gives when it is turned off
							assert(get_feeder(x, y, z, Cin, feederPath, feederNode));
							// if the feeder defaults cout to zero
							if (fpgaLogic[paths[feederPath][feederNode].x][paths[feederPath][feederNode].y][paths[feederPath][feederNode].z].CoutFixedDefaultValue)
							{
								if (inverting_t)
								{
									controllerFile << "1";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									controlSignalsDefaultValue.push_back('1');
								}
								else
								{
									controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									controlSignalsDefaultValue.push_back('0');
								}
							}
							else // feeeder defaults Cout to 1
							{
								if (inverting_t)
								{
									controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									controlSignalsDefaultValue.push_back('0');
								}
								else
								{
									controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									controlSignalsDefaultValue.push_back('1');
								}
							}
						}
						else // cin is not used, or it is used and the current input is cin. inputs coming from combout are set to 1 when off 
						{
							if (inverting_t)
							{
								controllerFile << "0";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
								controlSignalsDefaultValue.push_back('0');
							}
							else
							{
								controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								controlSignalsDefaultValue.push_back('1');
							}
						}
					}
				}
			}
			else if (fpgaLogic[x][y][z].LUTMask == "9F60") // cout is one of the outputs, cin is not used and two input ports are used
			{
				assert(fpgaLogic[x][y][z].usedInputPorts > 1);
				assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]);
				assert(!fpgaLogic[x][y][z].inputPorts[Cin]);
				if (inverting_t)
				{
					controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
					controlSignalsDefaultValue.push_back('0');
				}
				else
				{
					controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
					controlSignalsDefaultValue.push_back('1');
				}



			}
			else if (fpgaLogic[x][y][z].LUTMask == "6F9F") // cout is one of the outputs, cin is not used and two input ports are used
			{
				assert(fpgaLogic[x][y][z].usedInputPorts > 1);
				assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]);
				assert(!fpgaLogic[x][y][z].inputPorts[Cin]);
				if (inverting_t)
				{
					controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
					controlSignalsDefaultValue.push_back('1');
				}
				else
				{
					controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
					controlSignalsDefaultValue.push_back('0');
				}

			}
			else if (fpgaLogic[x][y][z].LUTMask == "DD22") // cout is one of the outputs, cin not used, only one port is used 
			{
				assert(fpgaLogic[x][y][z].usedInputPorts == 1);
				assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]);
				assert(!fpgaLogic[x][y][z].inputPorts[Cin]);
				if (inverting_t)
				{
					controllerFile << "0";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
					controlSignalsDefaultValue.push_back('0');
				}
				else
				{
					controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
					controlSignalsDefaultValue.push_back('1');
				}
			}
			else if (fpgaLogic[x][y][z].LUTMask == "77DD")
			{
				assert(fpgaLogic[x][y][z].usedInputPorts == 1);
				assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]);
				assert(!fpgaLogic[x][y][z].inputPorts[Cin]);
				if (inverting_t)
				{
					controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
					controlSignalsDefaultValue.push_back('1');
				}
				else
				{
					controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
					controlSignalsDefaultValue.push_back('0');
				}
			}
			else
			{
				std::cout << "Could not match LUT mask to something known." << std::endl;
			}
		}

	}
	controllerFile << ";" << std::endl;
#endif // controlSignalIdleStateReduction

#ifndef controlSignalIdleStateReduction
	controllerFile << "\t\t\tcontrolSignals_temp <= " << 2 * controlSignals.size() << "'b";
	for (i = 0; i < (int)controlSignals.size(); i++)
		controllerFile << "01";
	controllerFile << "; " << std::endl;
#endif // !controlSignalIdleStateReduction


	/////////////////////////////////////////////////////////////////////////////
	//////////// Finish trial
	////////////////////////////////////////////////////////////////////////////
	controllerFile << "\t\t\treset_or_network <= 1'b1;" << std::endl;
	if (sources.size()>0)
		controllerFile << "\t\t\tset_source_registers <= {" << sources.size() << "{1'b1}};" << std::endl;
	//// option 2
	/*	for (i = 0; i < controlSignals.size(); i++)
	{
	controllerFile << "\t\t\tPATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con <= 1'b0; " << std::endl;;
	controllerFile << "\t\t\tPATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F <= 1'b1; " << std::endl;
	}*/
	// option 2 finish
	// reset counter output
	controllerFile << "\t\t\treset_counter <= 1'b1; " << std::endl;
	controllerFile << "\t\t\treset_counter_reset <= 1'b1;" << std::endl;
	if (cascadedControlSignals.size()>0)
		controllerFile << "\t\t\tcascaded_select_temp <= {" << cascadedControlSignals.size() << "{1'b1}}; " << std::endl;
	// reset counter output
	controllerFile << "\t\t\terror_temp <= 1'b0; " << std::endl;
	// finished_one_iteration
	controllerFile << "\t\t\tfinished_one_iteration <= 1'b0; " << std::endl;
	controllerFile << "\t\t\tend" << std::endl;


	// write output for all test and reset phases states
	for (i = 0; i < numberOfTestPhases; i++)
	{
		// repeat the same control signals twice, once for test phase and once for reset phase
		for (l = 0; l < 2; l++)
		{
			if (l == 1)
				controllerFile << "\t\tTest_phase_" << i << " : begin" << std::endl;
			else
				controllerFile << "\t\tReset_phase_" << i << " : begin" << std::endl;
			// generate control singnals corresponding to testphase i
			//option 1

			/////////////////////////////////
			///// Changing states signals ///
			/////////////////////////////////

			if (l == 0) // reset phase
			{
				// reset the sticky register
				controllerFile << "\t\t\ttimer_reached_sticky_reset <= 1'b1;" << std::endl;
			}
			////////////////////////////////
			///////// control signals //////
			////////////////////////////////

			controllerFile << "\t\t\tcontrolSignals_temp <= " << 2 * controlSignals.size() << "'b";
			for (j = 0; j <(int)controlSignals.size(); j++)
			{
				// find x,y,z location of that wysiwyg
				x = paths[controlSignals[j].path][controlSignals[j].node].x;
				y = paths[controlSignals[j].path][controlSignals[j].node].y;
				z = paths[controlSignals[j].path][controlSignals[j].node].z;
				path_t = -1;
				node_t = -1;
				portInToCombout = -1;
				portIn_t = -1;
				inverting_t = false; // default value of inverting, chosen randomly with no specific reason. If combout is used then later code will set this bool accordingly.
				 // loop across nodes using this WYSIWYG to see of any path using this LUT is tested at the current phase
				for (k = 0; k < (int)fpgaLogic[x][y][z].nodes.size(); k++)
				{
					if (paths[fpgaLogic[x][y][z].nodes[k].path][0].testPhase == i) // this path should be tested in the current test phase
					{
						// the path cant be deleted AND assigned a test phase higher than -1
						assert(!paths[fpgaLogic[x][y][z].nodes[k].path][0].deleted);
						if (path_t == -1) // should add break if this condition is true, but as a double check I continue checking alll paths that are tested at this phase and make sure that all of them use the same portin by the assert statement.
						{
							path_t = fpgaLogic[x][y][z].nodes[k].path;
							node_t = fpgaLogic[x][y][z].nodes[k].node;
							portIn_t = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn;
						}
						assert(portIn_t == paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn);
						if (paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portOut == Combout)
						{
							if (portInToCombout == -1)
							{
								portInToCombout = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].portIn;
								inverting_t = paths[fpgaLogic[x][y][z].nodes[k].path][fpgaLogic[x][y][z].nodes[k].node].inverting;
								if (paths[path_t][node_t].portOut == Combout)
								{
									// check that we have the same inverting behaviour. 
									// must check that the first encountered path (path_t) uses combout before checking the assertion
									assert(inverting_t == paths[path_t][node_t].inverting);
								}
							}
						}

					}
				}
				// at this point if any path in element x,y,z is assigned to be tested at the current phase, path_t, node_t should be > -1 and portIn indicates the input port used
				//	if (path_t == 1004 && (node_t == 30 || node_t == 31))
				//		std::cout << "yalla" << std::endl;
				if (path_t == -1) // this element has no paths that should be tested in the current phase, so set control signals to stop it
				{
					//option 1
					//controllerFile << "01";

					// to have combout as 1, when LUT is fixed we should check what LUT mask do we have, first.
					if (fpgaLogic[x][y][z].LUTMask == "827D" || fpgaLogic[x][y][z].LUTMask == "22DD" || fpgaLogic[x][y][z].LUTMask == "609F")
					{
						controllerFile << "00";
					}
					else
					{
						controllerFile << "01";
					}
					// option 2
					/*	controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "Con <= 1'b0; " << std::endl;
					controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;*/
				}
				else/* if (!paths[controlSignals[j].path][0].deleted)*/// this should be tested in the current state, check if it is deleted. Actually not needed
				{
					// set con to 1
					// option 1 
					controllerFile << "1";
					// option 2
					//controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "Con <= 1'b1; " << std::endl;
					// assign F as if the signal tranisition is inverting
					// important to remember F has no control on Cout. Cout is adjusted by adjusting the other inputs when creating the WYSIWYGs
					// F only controls the transition to Combout, this depends on the LUT mask created. So lets figure out how to choose F to get the edge we want man		
					if (fpgaLogic[x][y][z].LUTMask == /*"D77D"*/ "827D" || fpgaLogic[x][y][z].LUTMask == "D728")
					{
						// there are two differnet LUT configuration that use this mask one with cout and one withou
						if (fpgaLogic[x][y][z].outputPorts[Cout - 5]) // cout is one of the outputs and cin is used for sure
						{
							assert(fpgaLogic[x][y][z].inputPorts[Cin]);
							if (fpgaLogic[x][y][z].usedInputPorts == 1)
							{
								//	assert(portInToCombout == -1); //since cout is used and only one input (which is cin) is used. not sure why I added this assert signal 
								// for inverting with only one input used F should be 0. Since we set the other non used input to vcc.
								if (inverting_t)
									controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
								else
									controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;

							}
							else // 2 input ports are used
							{
								if (/*fpgaLogic[x][y][z].inputPorts[Cin] &&*/ portIn_t != Cin) // cin is used, and the current path is using an other input port
								{
									// find what coud does the feeder of this in gives when it is turned off
									//	if (!get_feeder(x, y, z, Cin, feederPath, feederNode))
									//		assert(false);
									// BRAM stuff todo
									assert(get_feeder(x, y, z, Cin, feederPath, feederNode));
									// if the feeder defaults cout to zero
									//	std::cout << feederNode << std::endl;
									//	std::cout << feederPath << std::endl;

									// if there is no combout (inverting_t is false by default) set f to 1 so that we share as many control signals as possible, to prevent routing congestion
									if (fpgaLogic[x][y][z].usedOutputPorts == 1) // ib 11022016
									{
										if (inverting_t)
											controllerFile << "0"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
										else
											controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									}
									else if (fpgaLogic[paths[feederPath][feederNode].x][paths[feederPath][feederNode].y][paths[feederPath][feederNode].z].CoutFixedDefaultValue) // determine the tranissiton edge from normal port (non cin) to combout by checking what is the feeder default value for cout
									{
										if (inverting_t)
											controllerFile << "1"; // option 2controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
										else
											controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									}
									else // feeeder defaults Cout to 1
									{
										if (inverting_t)
											controllerFile << "0"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
										else
											controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									}
								}
								else //  the current input is cin. inputs coming from combout are set to 1 when off 
								{
									if (inverting_t)
										controllerFile << "0"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									else
										controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								}

							}

						}
						else // output is only combout
						{
							assert(fpgaLogic[x][y][z].LUTMask == "D728");
							if (fpgaLogic[x][y][z].usedInputPorts == 1)
							{
								assert(portInToCombout > -1);
								// for inverting with only one input used F should be 1. Since we set the other non used input to gnd.
								if (inverting_t)
									controllerFile << "1";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								else
									controllerFile << "0";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;

							}
							else // 2 input ports are used
							{
								if (fpgaLogic[x][y][z].inputPorts[Cin] && portIn_t != Cin) // cin is used, and the current path is using an other input port
								{
									// find what coud does the feeder of this in gives when it is turned off
									assert(get_feeder(x, y, z, Cin, feederPath, feederNode));
									// if the feeder defaults cout to zero
									if (fpgaLogic[paths[feederPath][feederNode].x][paths[feederPath][feederNode].y][paths[feederPath][feederNode].z].CoutFixedDefaultValue)
									{
										if (inverting_t)
											controllerFile << "1";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
										else
											controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									}
									else // feeeder defaults Cout to 1
									{
										if (inverting_t)
											controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
										else
											controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
									}
								}
								else // cin is not used, or it is used and the current input is cin. inputs coming from combout are set to 1 when off 
								{
									if (inverting_t)
										controllerFile << "0";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
									else
										controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								}
							}
						}
					}
					else if (fpgaLogic[x][y][z].LUTMask == "9F60") // cout is one of the outputs, cin is not used and two input ports are used
					{
						assert(fpgaLogic[x][y][z].usedInputPorts > 1); // more than 1 port used
						assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]); // cout is used
						assert(!fpgaLogic[x][y][z].inputPorts[Cin]); // cin is not used
						if (inverting_t)
							controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
						else
							controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;




					}
					else if (fpgaLogic[x][y][z].LUTMask == /*"6F9F"*/ "609F") // cout is one of the outputs, cin is not used and two input ports are used
					{
						assert(fpgaLogic[x][y][z].usedInputPorts > 1); // more than 1 port used
						assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]); // cout is used
						assert(!fpgaLogic[x][y][z].inputPorts[Cin]); // cin is not used
						if (inverting_t)
							controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
						else
							controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;


					}
					else if (fpgaLogic[x][y][z].LUTMask == "DD22") // cout is one of the outputs, cin not used, only one port is used 
					{
						assert(fpgaLogic[x][y][z].usedInputPorts == 1); // only 1 input port used
						assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]); // cout is used
						assert(!fpgaLogic[x][y][z].inputPorts[Cin]); // cin is not used
						if (inverting_t)
							controllerFile << "0";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
						else
							controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;

					}
					else if (fpgaLogic[x][y][z].LUTMask == "22DD")
					{
						assert(fpgaLogic[x][y][z].usedInputPorts == 1); // only 1 port used
						assert(fpgaLogic[x][y][z].outputPorts[Cout - 5]); // cout is used
						assert(!fpgaLogic[x][y][z].inputPorts[Cin]); // cin is not used
						if (inverting_t)
							controllerFile << "1"; //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
						else
							controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;

					}
					else if (fpgaLogic[x][y][z].LUTMask == "6996") // three inputs are used and only combout used
					{
						assert(!fpgaLogic[x][y][z].outputPorts[Cout - 5]);
						if (fpgaLogic[x][y][z].inputPorts[Cin] && portIn_t != Cin) // Cin is being used and the curent path is using another input
						{
							// find what coud does the feeder of this in gives when it is turned off
							//int impPath, impNode;
							assert(get_feeder(x, y, z, Cin, feederPath, feederNode));
							// if the feeder defaults cout to zero
							if (fpgaLogic[paths[feederPath][feederNode].x][paths[feederPath][feederNode].y][paths[feederPath][feederNode].z].CoutFixedDefaultValue)
							{
								if (inverting_t)
									controllerFile << "0";  // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
								else
									controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
							}
							else // feeeder defaults Cout to 1
							{
								if (inverting_t)
									controllerFile << "1"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;
								else
									controllerFile << "0"; // option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
							}
						}
						else // all three inputs are normal inputs
						{
							assert(portInToCombout > -1);
							// for inverting with only one input used F should be 1. Since we the two other inputs are fixed at vcc
							if (inverting_t)
								controllerFile << "1";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b1; " << std::endl;
							else
								controllerFile << "0";  //option 2 controllerFile << "\t\t\tPATH" << controlSignals[j].path << "NODE" << controlSignals[j].node << "F <= 1'b0; " << std::endl;


						}
					}
					else
					{
						std::cout << "Could not match LUT mask to something known." << std::endl;
					}
				}

			}
			controllerFile << " ; " << std::endl;

			/////////////////////////////////
			//////// cascaded LUTs selector//
			/////////////////////////////////

			if (cascadedControlSignals.size()>0)
				controllerFile << "\t\t\tcascaded_select_temp <= " << cascadedControlSignals.size() << "'b";
			for (j = 0; j < (int)cascadedControlSignals.size(); j++)
			{
				x = paths[cascadedControlSignals[j].path][cascadedControlSignals[j].node].x;
				y = paths[cascadedControlSignals[j].path][cascadedControlSignals[j].node].y;
				z = paths[cascadedControlSignals[j].path][cascadedControlSignals[j].node].z;
				assert(fpgaLogic[x][y][z].cascadedPaths.size()>0); // this must be true, other wise why does this LUT have a cascaded control signal
				bool cascTested = false;
				for (int p = 0; p < (int)fpgaLogic[x][y][z].cascadedPaths.size(); p++) // loop across all cascaded paths
				{
					int currentCascPath = fpgaLogic[x][y][z].cascadedPaths[p];
					if (paths[currentCascPath][0].deleted) // if this is deleted
						continue;

					if (paths[currentCascPath][0].testPhase == i) // this cascaded path is tested at the current test phase i
					{
						cascTested = true;
						break;
					}

				}

				if (cascTested)
				{
					//////////// double check that should be deleted //////// if the ith test phase is testing one of the cascaded paths at LUT x,y,z then it can't be testing any path going through LUT x,y,z;
					for (int dd = 0; dd < (int)fpgaLogic[x][y][z].nodes.size(); dd++)
					{
						assert(paths[fpgaLogic[x][y][z].nodes[dd].path][0].testPhase != i);
					}

					///////// end double check ///////
					controllerFile << "0"; // this select signal goes to the mux and ensure that the f signal of the lut is connected to the tff
				}
				else
				{
					controllerFile << "1";
				}
			}
			if (cascadedControlSignals.size()>0)
				controllerFile << " ; " << std::endl;

			/////////////////////////////////////
			///// output set source registers ///
			/////////////////////////////////////

			if (sources.size()>0)
				controllerFile << "\t\t\tset_source_registers <= " << sources.size() << "'b";
			//	std::cout << "phase " << i << " :";
			for (j = ((int)sources.size()) - 1; j > -1; j--)
			{
				x = paths[sources[j].path][sources[j].node].x;
				y = paths[sources[j].path][sources[j].node].y;
				z = paths[sources[j].path][sources[j].node].z;
				currentlyTested = false;
				// loop across all nodes using this LUT and check if any of them is tested in the current test phase
				for (k = 0; k < (int)fpgaLogic[x][y][z].nodes.size(); k++)
				{
					if (paths[fpgaLogic[x][y][z].nodes[k].path][0].testPhase == i)
					{
						assert(!paths[fpgaLogic[x][y][z].nodes[k].path][0].deleted);
						currentlyTested = true;
						break;
					}
				}

				if (currentlyTested) // if this source signals is tested in the current phase then do not set the source signal for this register (lkeave it toggling)
				{
					controllerFile << "0";

				}
				else // if this source is not used at the current test phase, then set it to 1.
				{

					controllerFile << "1";
				}

			}
			//		std::cout << std::endl;
			if (sources.size()>0)
				controllerFile << " ; " << std::endl;

			if (l == 1) // test phase state
			{
				// reset counter output
				controllerFile << "\t\t\treset_counter <= 1'b0; " << std::endl;
				controllerFile << "\t\t\treset_counter_reset <= 1'b1;" << std::endl;
				controllerFile << "\t\t\terror_temp <= errorTestPhase[" << testPhaseCount << "];" << std::endl;
				errorSignalDivision[testPhaseCount].resize(0);
				// loop across sinks and see which sinks are used at the current test phase

				//////////////////////////////////////////
				/////// Sinks ////////////////////////////
				//////////////////////////////////////////

				std::vector<std::pair<int, int>> BRAMSources;

				for (j = 0; j <(int)sinks.size(); j++)
				{
					// get x, y, z of this sink
					x = paths[sinks[j].path][sinks[j].node].x;
					y = paths[sinks[j].path][sinks[j].node].y;
					z = paths[sinks[j].path][sinks[j].node].z;
					currentlyTested = false;
					// loop across all nodes using this LUT and check if any of them is tested in the current test phase

					/*
					// check if the sink is a BRAM
					bool isMem = false;
					if (fpgaLogic[x][y][z].isBRAM)
					{
						assert(z == 0);
						isMem = true;

					}
					*/

					// assumption: only testing one port of a memory at each calib bitstreams
					// so if a one of the sinks is addressport of a BRAM
					// and a path uses the BRAM then it must be using it from that port 

					for (k = 0; k < (int)fpgaLogic[x][y][z].nodes.size(); k++)
					{
						// ensure that the path size is larger than 1 
						//--> it's less than or equal one if it's buried inside a BRAM
						if (paths[fpgaLogic[x][y][z].nodes[k].path].size() <= 1)
							continue;

						if (paths[fpgaLogic[x][y][z].nodes[k].path][0].testPhase == i)
						{
							if (fpgaLogic[x][y][z].nodes[k].node == 0) // if this node is a source then we shouldnt be checking the output of this FF for erro. THis only happens in cascaded cases.
								continue;

							assert(!paths[fpgaLogic[x][y][z].nodes[k].path][0].deleted);

							currentlyTested = true;

							// check if the source is a BRAM
							int testedPath = fpgaLogic[x][y][z].nodes[k].path;
							int testedNode = fpgaLogic[x][y][z].nodes[k].node;
							int srcX = paths[testedPath][0].x;
							int srcY = paths[testedPath][0].y;
							int srcZ = paths[testedPath][0].z;

							// if the source is a BRAM then adjust the checking for error signal
							if (fpgaLogic[srcX][srcY][srcZ].isBRAM)
							{
								controllerFile << "\t\t\tenableErrorCheckingTemp[" << j << "] <= error_PATH"
									<< fpgaLogic[srcX][srcY][srcZ].owner.path << "NODE" 
									<< fpgaLogic[srcX][srcY][srcZ].owner.node << "_BRAM;" << std::endl;
							}

							break;
						}
					}

					if (currentlyTested)
					{
						errorSignalDivision[testPhaseCount].push_back(std::make_pair(-1,j));
						//controllerFile << "| errorVec[" << j << "] ";
					}

				}

				/////////////////////////////////////////////////////////////
				///// check for errors from BRAM test controllers ///////////
				/////////////////////////////////////////////////////////////

				std::vector<std::pair<int, int>> BRAMControllersTestedNow;

				BRAMControllersTestedNow.resize(0);

				for (int iBRAM = 0; iBRAM < FPGAsizeX; iBRAM++)
				{
					for (int jBRAM = 0; jBRAM < FPGAsizeY; jBRAM++)
					{
						int testedAddressPortIndex = -1;
						int controllerType = -1;
						bool BRAMCurrentlyTested = isBRAMControllerUsedInThisTestPhase( iBRAM, jBRAM, i, testedAddressPortIndex, controllerType);
						int BRAMPathOwner = fpgaLogic[iBRAM][jBRAM][0].owner.path;
						int BRAMNodeOwner = fpgaLogic[iBRAM][jBRAM][0].owner.node;
						// if BRAM is currently tested then let's add it to the errorSignalDivision
						if (BRAMCurrentlyTested)
						{
							// add the error signal only if the controller is not a dataout controller
							if (controllerType == DATAOUT_SIMPLE_DUAL_CONTROLLER
								|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTA
								|| controllerType == DATAOUT_NORMAL_CONTROLLER_PORTB
								|| controllerType == DATAOUT_ROM_CONTROLLER_PORTA
								|| controllerType == DATAOUT_ROM_CONTROLLER_PORTB)
							{
								BRAMControllersTestedNow.push_back(std::make_pair(BRAMPathOwner, BRAMNodeOwner));
							}
							else
							{
								errorSignalDivision[testPhaseCount].push_back(std::make_pair(BRAMPathOwner, BRAMNodeOwner));
								BRAMControllersTestedNow.push_back(std::make_pair(BRAMPathOwner, BRAMNodeOwner));
							}
							
							

							if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
								|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
								|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
								|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB)
							{
								// set the corresponding info to test address or data
								controllerFile << "\t\t\tinfo_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling[" << testedAddressPortIndex
									<< "] <= 1'b1;\n";
							}
							

							//controllerFile << "| errorVec[" << j << "] ";
						}
					}
					
				}

				// done_state signal for the current test phase

				if (BRAMControllersTestedNow.size() > 0)
				{
					controllerFile << "\t\t\tdone_state <= 1'b1 & ";
				}

				for (int count = 0; count < BRAMControllersTestedNow.size(); count++)
				{
					controllerFile << " done_PATH" << BRAMControllersTestedNow[count].first << "NODE" << BRAMControllersTestedNow[count].second << "_BRAM_sticky &";

				}

				if (BRAMControllersTestedNow.size() > 0)
				{
					controllerFile << " 1'b1;\n";
				}

				// start_test signals for BRAM tested now

				for (int count = 0; count < BRAMControllersTestedNow.size(); count++)
				{
					controllerFile << "\t\t\tstart_test_PATH" << BRAMControllersTestedNow[count].first << "NODE" << BRAMControllersTestedNow[count].second << "_BRAM <= 1'b1;\n";

				}

				

				/////////////////////////////////////////////////////////////
				///////// End error from BRAM test controllers //////////////
				/////////////////////////////////////////////////////////////




				testPhaseCount++;
				//controllerFile << ";" << std::endl;
				//controllerFile << "\t\t\terror <= 1'b0; " << std::endl;
				// finished_one_iteration
				controllerFile << "\t\t\tfinished_one_iteration <= 1'b0; " << std::endl;
				controllerFile << "\t\t\treset_or_network <= 1'b0;" << std::endl;
				controllerFile << "\t\t\tend" << std::endl;
			}
			else // reset for  reset phase state
			{
				// reset_test for BRAMs


				/////////////////////////////////////////////////////////////
				///// decide on the reset_test from BRAM test controllers ///////////
				/////////////////////////////////////////////////////////////

				std::vector<std::pair<int, int>> BRAMControllersTestedNow;

				BRAMControllersTestedNow.resize(0);

				for (int iBRAM = 0; iBRAM < FPGAsizeX; iBRAM++)
				{
					for (int jBRAM = 0; jBRAM < FPGAsizeY; jBRAM++)
					{
						int testedAddressPortIndex = -1;
						int controllerType = -1;
						bool BRAMCurrentlyTested = isBRAMControllerUsedInThisTestPhase(iBRAM, jBRAM, i, testedAddressPortIndex, controllerType);
						int BRAMPathOwner = fpgaLogic[iBRAM][jBRAM][0].owner.path;
						int BRAMNodeOwner = fpgaLogic[iBRAM][jBRAM][0].owner.node;

						if (BRAMCurrentlyTested)
						{

							BRAMControllersTestedNow.push_back(std::make_pair(BRAMPathOwner, BRAMNodeOwner));
							if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER
								|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER
								|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
								|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
								)
							{
								// set the corresponding info to test (address or data in)
								controllerFile << "\t\t\tinfo_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling[" << testedAddressPortIndex
									<< "] <= 1'b1;\n";
							}
							//controllerFile << "| errorVec[" << j << "] ";
						}
					}

				}

				// start_test signals for BRAM tested now

				for (int count = 0; count < BRAMControllersTestedNow.size(); count++)
				{
					controllerFile << "\t\t\treset_test_PATH" << BRAMControllersTestedNow[count].first << "NODE" << BRAMControllersTestedNow[count].second << "_BRAM <= 1'b1;\n";

				}

				// reset counter output
				controllerFile << "\t\t\treset_counter <= 1'b1; " << std::endl;
				controllerFile << "\t\t\treset_counter_reset <= 1'b0;" << std::endl;
				// error output
				controllerFile << "\t\t\terror_temp <= 1'b0; " << std::endl;
				// finished_one_iteration
				controllerFile << "\t\t\tfinished_one_iteration <= 1'b0; " << std::endl;
				controllerFile << "\t\t\treset_or_network <= 1'b1;" << std::endl;
				controllerFile << "\t\t\tend" << std::endl;

			}

		}
	}

	/// last state done
	controllerFile << "\t\tDone_One_Iteration : begin" << std::endl;
	//Done one iteration  state:  fix all LUTs to a fixed output 
	// option 1 
	controllerFile << "\t\t\tcontrolSignals_temp <= " << 2 * controlSignals.size() << "'b";
#ifdef controlSignalIdleStateReduction
	for (i = 0; i < controlSignalsDefaultValue.size(); i++)
		controllerFile << controlSignalsDefaultValue[i];
	controllerFile << "; " << std::endl;
#endif
#ifndef controlSignalIdleStateReduction
	for (i = 0; i < (int)controlSignals.size(); i++)
		controllerFile << "01";
	controllerFile << "; " << std::endl;
#endif
	//	controllerFile << "\t\t\tcontrolSignals <= {" << 2 * controlSignals.size() << "{1'b1}};" << std::endl;;
	// option 2 
	/*	for (i = 0; i < controlSignals.size(); i++)
	{
	controllerFile << "\t\t\tPATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con <= 1'b0; " << std::endl;;
	controllerFile << "\t\t\tPATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F <= 1'b1; " << std::endl;
	}*/
	// reset counter output
	controllerFile << "\t\t\treset_counter <= 1'b1; " << std::endl;
	controllerFile << "\t\t\treset_counter_reset <= 1'b1;" << std::endl;
	if (cascadedControlSignals.size()>0)
		controllerFile << "\t\t\tcascaded_select_temp <= {" << cascadedControlSignals.size() << "{1'b1}}; " << std::endl;
	// reset counter output
	controllerFile << "\t\t\terror_temp <= 1'b0; " << std::endl;
	if (sources.size()>0)
		controllerFile << "\t\t\tset_source_registers <= {" << sources.size() << "{1'b1}};" << std::endl;
	// finished_one_iteration
	controllerFile << "\t\t\tfinished_one_iteration <= 1'b1; " << std::endl;
	controllerFile << "\t\t\treset_or_network <= 1'b1;" << std::endl;
	controllerFile << "\t\t\tend" << std::endl;
	///////////////////////////////// default
	/////////////////////////////////
	controllerFile << "\t\tdefault : begin" << std::endl;
	controllerFile << "\t\t\tcontrolSignals_temp <= " << 2 * controlSignals.size() << "'b";
	for (i = 0; i < (int)controlSignals.size(); i++)
		controllerFile << "01";
	controllerFile << "; " << std::endl;

	controllerFile << "\t\t\treset_or_network <= 1'b1;" << std::endl;
	if (sources.size()>0)
		controllerFile << "\t\t\tset_source_registers <= {" << sources.size() << "{1'b1}};" << std::endl;
	// reset counter output
	controllerFile << "\t\t\treset_counter <= 1'b1; " << std::endl;
	controllerFile << "\t\t\treset_counter_reset <= 1'b1;" << std::endl;
	if (cascadedControlSignals.size()>0)
		controllerFile << "\t\t\tcascaded_select_temp <= {" << cascadedControlSignals.size() << "{1'b1}}; " << std::endl;
	// reset counter output
	controllerFile << "\t\t\terror_temp <= 1'b0; " << std::endl;
	// finished_one_iteration
	controllerFile << "\t\t\tfinished_one_iteration <= 1'b0; " << std::endl;
	controllerFile << "\t\t\tend" << std::endl;

	///////////////////////////////
	/////////////////////////////

	///// end case
	controllerFile << "\tendcase" << std::endl;
	controllerFile << "end" << std::endl;

	// assign address to test mate

	bool alwaysBlockflag = false;
	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			int BRAMPathOwner = fpgaLogic[i][j][0].owner.path;
			int BRAMNodeOwner = fpgaLogic[i][j][0].owner.node;
			int testedPortSize = -1;
			int controllerType = -1;
			if (isBRAMwithController(i, j, testedPortSize, controllerType, memories))
			{
				if (controllerType == ADDRESS_READ_INCAPABLE_CONTROLLER)
				{

					controllerFile << "always @ (posedge CLK)\n"
						<< "begin\n";

					controllerFile << "\tinfo_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM <= ("
						<< "info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM ^ "
						<< "info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling) | "
						<< "~info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling;\n";

					controllerFile << "end\n\n";
				}

				if (controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTA
					|| controllerType == DATAIN_READ_CAPABLE_CONTROLLER_PORTB
					|| controllerType == DATAIN_READ_INCAPABLE_CONTROLLER)
				{
					controllerFile << "always @ (*)\n"
						<< "begin\n";

					controllerFile << "info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM = "
						<< "~info_to_test_PATH" << BRAMPathOwner << "NODE" << BRAMNodeOwner << "_BRAM_toggling;\n";

					controllerFile << "end\n\n";
				}
			}
		}
	}





	controllerFile << std::endl << "//// instantiate counter for reset phases basically just counter to flush the orTree and relax timing requirments for control signals" << std::endl;
	// find maximum latenct of or network, i.e max depth of the network. So first find maximum input
	int max = (int)errorSignalDivision[0].size();
	for (i = 0; i < (int)errorSignalDivision.size(); i++)
	{
		if ((int)errorSignalDivision[i].size()>max)
			max = ((int)errorSignalDivision[i].size());
	}
	// get counter length to flush the or network
	// network depth
	int orDepth = (int)ceil(log(max) / log(LUTinputSize));
	int reset_counter_length = (int)ceil(log(orDepth + addedLatencyToBufferSinks + addedLatencyTorelaxTimingConstraints) / log(2));
	std::cout << "Reset Counter Latency is " << reset_counter_length << std::endl;
	controllerFile << "counter_testing #(.L(" << reset_counter_length << ")) count0_reset(.CLK(CLK),.clr(reset_counter_reset),.timerReached(timer_reached_reset));" << std::endl;
#ifdef shiftRegOrTree
	controllerFile << "shift_reg #(.L(" << orResetShift << ")) shift_reg_0(.CLK(CLK),.in(reset_or_network),.out(reset_or_network_shifted));" << std::endl;
#endif
	controllerFile << std::endl << "//// instantiate OR tree for oring error signals within each phase and generating one error signal for each test phase" << std::endl;

	for (i = 0; i < numberOfTestPhases; i++) // instanitate or modules
	{
#ifdef shiftRegOrTree
		controllerFile << "or_tree_" << i << " or_test_phase_" << i << " (.reset(reset_or_network_shifted),.CLK(CLK),.out (errorTestPhase[" << i << "]), .in({";
#else
		controllerFile << "or_tree_" << i << " or_test_phase_" << i << " (.reset(reset_or_network),.CLK(CLK),.out (errorTestPhase[" << i << "]), .in({";
#endif
		for (j = 0; j < (int)errorSignalDivision[i].size() - 1; j++)
		{
			if (errorSignalDivision[i][j].first == -1)
			{
				controllerFile << "errorVec[" << errorSignalDivision[i][j].second << "],";
			}
			else
			{
				// this error is coming from a BRAM
				controllerFile << "error_PATH" << errorSignalDivision[i][j].first << "NODE" << errorSignalDivision[i][j].second << "_BRAM," ;
			}
		}
		if (errorSignalDivision[i][j].first == -1)
		{
			controllerFile << "errorVec[" << errorSignalDivision[i][j].second << "]}));" << std::endl;
		}
		else
		{
			// this error is coming from a BRAM
			controllerFile << "error_PATH" << errorSignalDivision[i][j].first << "NODE" << errorSignalDivision[i][j].second << "_BRAM}));" << std::endl;
		}
	}
	controllerFile << "endmodule" << std::endl;
	//create
	for (i = 0; i < numberOfTestPhases; i++)
	{
		create_or_tree((int)errorSignalDivision[i].size(), LUTinputSize, i, controllerFile);
	}

	//	create_or_tree(9, 4, 0, controllerFile);
	//	create_or_tree(9, 4, 0, controllerFile);
	//	create_or_tree(1, 4, 0, controllerFile);
	//	create_or_tree(2, 4, 0, controllerFile);
	//	create_or_tree(3, 4, 0, controllerFile);
	//	create_or_tree(4, 4, 0, controllerFile);
	//	create_or_tree(5, 4, 0, controllerFile);
	//	create_or_tree(33, 4, 0, controllerFile);
	controllerFile.close();
}

void create_WYSIWYGs_file(int bitStreamNumber, std::vector<BRAM> memories) // also calls create_auxill and create_controller
{
	int i, j, k;
	int x;
	//int total = 0;
	//bool deleted = true;
	std::ofstream verilogFile;
	verilogFile.open("VerilogFile.txt");
	int path = -1;
	int node = -1;
	std::vector <Path_logic_component> sources; // stores source flipflops
	std::vector <Path_logic_component> sinks; // stores the output signals of the tested paths;

	// stores the sinks of memories
	// whenever a path ends at a BRAM we will need to check it's output
	// so we need to add this signal as an input to the controller
	std::vector <Path_logic_component> memorySinks; 

	std::vector <Path_logic_component> controlSignals; // stores the control signals of the tested paths;
	std::vector <Path_logic_component> cascadedControlSignals; // stores the control signals of LUTs feeding a cascaded register;
	std::vector <Path_logic_component> CoutSignals;
	std::vector <Path_logic_component> DummyRegSignals;
	DummyRegSignals.resize(0);
	int pathFeeder, nodeFeeder;
	pathFeeder = -1;
	nodeFeeder = -1;
	int pathFeederPort1, pathFeederPort2, nodeFeederPort1, nodeFeederPort2;
	int port1, port2;
	bool inverting = false;

	std::string BRAM_intermediateSignals = "";

	// loop through all FPGA elements
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				inverting = false;
				port1 = -1;
				port2 = -1;
				pathFeederPort1 = -1;
				pathFeederPort2 = -1;
				nodeFeederPort1 = -1;
				nodeFeederPort2 = -2;
				path = -1; // new ibrahim 17/02/2016
				node = -1;

				// make sure that this node is not deleted and gets the name of the wysiwyg, or we can check utilization
				for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) 
				{
					if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
					{
						path = fpgaLogic[i][j][k].nodes[x].path;
						node = fpgaLogic[i][j][k].nodes[x].node;
						break;
					}
				}

				if (path == -1 || node == -1) // all paths using this node are deleted so do not instantiate a wysiwygs for this node
					continue;
				assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node);

				if (k % LUTFreq == 0) // LUT or BRAM
				{
#ifdef CycloneIV

					// is a not a memory, so it's a LUt
					if (!fpgaLogic[i][j][k].isBRAM)
					{
						// location X1, put the code into a functoin and calling it below. 
						LUT_WYSIWYG_CycloneIV(cascadedControlSignals, i, j, k, verilogFile, port1, port2, CoutSignals, 
							controlSignals, path, node, pathFeederPort1, nodeFeederPort1, pathFeederPort2, nodeFeederPort2,
							inverting);// , cascadedControlSignals);
					}
					else // is a memory
					{
						assert(k == 0);
						int memIndex = fpgaLogic[i][j][k].indexMemories[0];

						//todo: handle source, sink and control signals to BRAMs
						BRAM_intermediateSignals += BRAM_WYSIWYG_cycloneIV(memories[memIndex], verilogFile, false, memories, sinks);
					}
				
#endif
#ifdef StratixV
					ALUT_WYSIWYGS_StratixV(i, j, k,verilogFile);
#endif
				

				}
				else // FLIPFLOP
				{
					// check if a sink uses this FF (can not simply use node as we might have cascaded or wrap-around paths)
					bool isSink = false;

					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
					{
						if (fpgaLogic[i][j][k].nodes[x].node != 0 && !paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted) // node x uses FF i,j,k as asink register and it is not deleted
						{
							isSink = true;
							break;
						}
					}

					verilogFile << "dffeas PATH" << path << "NODE" << node << "_t (" << std::endl;
					verilogFile << "	.clk(CLK)," << std::endl;
					if (!isSink/*node == 0*/) // [old]--> this is a source register, assuming no cascaded paths, no register is a source and a sink
					{
						verilogFile << "	.d(Xin[" << sources.size() << "])," << std::endl; // assuming that all sources will share the same input
						verilogFile << "	.q(PATH" << path << "NODE" << node << "));" << std::endl;
						sources.push_back(Path_logic_component(path, node));
					}
					else // this is a sink register						
					{
						assert(get_feeder(i, j, k, pathFeeder, nodeFeeder));
						if (fpgaLogic[i][j][k].FFMode == sData) // input is connected using asdata
						{
							verilogFile << "	.asdata(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl; // connected to the sdata port
							verilogFile << "	.sload(1'b1)," << std::endl; // sload is high
						}
						else
						{
							// assuming that all sources will share the same input
							verilogFile << "	.d(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl; 

						}
						verilogFile << "	.q(PATH" << path << "NODE" << node << "));" << std::endl;
						sinks.push_back(Path_logic_component(path, node));

					}
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .power_up = \"low\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .is_wysiwyg = \"true\";" << std::endl;
				}
				path = -1;
				node = -1;
			}
		}
	}
#ifdef DUMMYREG
	verilogFile << "//adding dummy regs man hope it works fuck this shit." << std::endl;

	///// loop across fpga to add dummy register after any LUT using port c as input

	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization>0 && fpgaLogic[i][j][k].inputPorts[portC]) //  if a cell is utilized and uses port c as input then check if cascaded register is used or not
				{
					if (fpgaLogic[i][j][k + 1].utilization>0) // if the register is used then skip this and go on
						continue;

					path = -1;
					node = -1;
					for (x = 0; x < fpgaLogic[i][j][k].nodes.size(); x++) // make sure that this node is not deleted and gets the name of the wysiwyg, or we can check utilization
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							break;
						}
					}
					assert(path > -1 && node > -1);
					verilogFile << "dffeas DUMMYREG_PATH" << path << "NODE" << node << "_t (" << std::endl;
					verilogFile << "	.clk(CLK)," << std::endl;
					verilogFile << "	.d(PATH" << path << "NODE" << node << ")," << std::endl; // assuming that all sources will share the same input
					verilogFile << "	.q(DUMMYREG_PATH" << path << "NODE" << node << "));" << std::endl;
					DummyRegSignals.push_back(Path_logic_component(path, node));
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .power_up = \"low\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .is_wysiwyg = \"true\";" << std::endl;
				}

			}
		}
	}
	if (DummyRegSignals.size() > 0)
		verilogFile << "assign dummyOut = DUMMYREG_PATH" << DummyRegSignals[0].path << "NODE" << DummyRegSignals[0].node;
	for (x = 1; x < DummyRegSignals.size(); x++)
	{
		verilogFile << "| DUMMYREG_PATH" << DummyRegSignals[x].path << "NODE" << DummyRegSignals[x].node;
	}
	verilogFile << ";" << std::endl;
#endif
	verilogFile << "endmodule" << std::endl;
	verilogFile.close();


	// open the same file again as input
	std::ifstream verilogFileSecondPart;
	verilogFileSecondPart.open("VerilogFile.txt");
	create_auxil_file(sinks,
		controlSignals,
		CoutSignals, 
		sources, 
		cascadedControlSignals, 
		verilogFileSecondPart,
		BRAM_intermediateSignals,
		memories,
		bitStreamNumber);

	// comment out for BRAM testing checking
	create_controller_module(sinks, 
		controlSignals,
		sources, 
		cascadedControlSignals,
		memories,
		bitStreamNumber);


}





void LUT_WYSIWYG_CycloneIV(std::vector <Path_logic_component>& cascadedControlSignals,int i, int j, int k, 
	std::ofstream& verilogFile, int port1, int port2, std::vector <Path_logic_component>& CoutSignals, 
	std::vector <Path_logic_component>& controlSignals, int path, int node, int pathFeederPort1,
	int nodeFeederPort1, int  pathFeederPort2, int nodeFeederPort2, bool & inverting)//, std::vector <Path_logic_component>& cascadedControlSignals)
{
	int pathFeederPort3, nodeFeederPort3;
	int port3 = -1;

	int port1Path = -1;
	int port1PrevNode = -1;
	int port2Path = -1;
	int port2PrevNode = -1;
	int port3Path = -1;
	int port3PrevNode = -1;

	//std::vector <Path_logic_component>& cascadedControlSignals;
	bool feedsCascaded = false;

//	if (i == 19 && j == 7 && k == 18)
//		std::cout << "wasaaal" << std::endl;

	for (int ii = 0; ii < (int)fpgaLogic[i][j][k].cascadedPaths.size(); ii++) //loop through list of cascaded path to check if one of them is not deleted
	{
		if (!paths[fpgaLogic[i][j][k].cascadedPaths[ii]][0].deleted) // if it is not deleted
		{
			feedsCascaded = true;
			break;
		}
	}

	if (feedsCascaded)
	{
		cascadedControlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring cascaded control signals
	}

	if (fpgaLogic[i][j][k].outputPorts[Combout - 5] && !fpgaLogic[i][j][k].outputPorts[Cout - 5]) // output is only from Combout
	{

		if (fpgaLogic[i][j][k].usedInputPorts < 3) // check that maximum 2 inputs are being used
		{
			//assert(!check_control_signal_required(i, j, k)); // double check that this cell does not require a control (fix) signal [not sure y i had this, 25/09/2016 I think I dont need it]
			// get the two ports used
			for (int x = 0; x < InputPortSize; x++)
			{
				if (fpgaLogic[i][j][k].inputPorts[x] && port1 < 0) // 1st port not yet set
				{
					port1 = x;
				}
				else if (fpgaLogic[i][j][k].inputPorts[x]) // 1st port ws set, so set the second port
				{
					port2 = x;
				}

			}
			// at least one port must be used
			assert(port1 > -1);
			if (fpgaLogic[i][j][k].usedInputPorts == 2)
				assert(port2>-1);
			if (fpgaLogic[i][j][k].inputPorts[Cin]) // Cin is used to connect to combout
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con )," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;
				if (port1 == Cin) // port 1 is Cin
				{
					if (port2 < 0) // no port 2, set port b to gnd
					{
						verilogFile << "	.datab(gnd)," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout )," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
						// ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
					else // port 2 is also used 
					{
						// get the x,y, z for port 2 feeder
						int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
						int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
						int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
						{
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 
								<< BRAMoutputPort << "[" <<  BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}
						
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout )," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
						//ib0502verilogFile << "	.datad(vcc )," << std::endl;
					}
				}
				else // port 2 is  cin and port 1 must be used
				{
					assert(port2>-1);



					int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
					int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
					int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
					{
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}





					//verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					verilogFile << "	.cin(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << "_Cout )," << std::endl;
					if (feedsCascaded)
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
					}
					else
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;

					}
					
					//ib0502verilogFile << "	.datad(vcc )," << std::endl;
				}
				verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD728;" << std::endl; //D77D 
				fpgaLogic[i][j][k].LUTMask = "D728";
				fpgaLogic[i][j][k].CoutFixedDefaultValue = false; // probably not needed as this case has no cout as output

			}
			else // normal inputs to combout // common case
			{

				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));

				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
																			//// instantiate the wysiwyg
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;

				int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
				int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
				int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

				// if it'snot a BRAM
				if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
				{
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
				}
				else
				{
					int BRAMoutputPortIndex;
					std::string BRAMoutputPort;
					assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
						<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
						<< " )," << std::endl;

				}


				//verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;





				if (port2 > -1)
				{



					// get the x,y, z for port 2 feeder
					int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
					int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
					int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
					{
						verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}



				//	verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
				}
				else // only one port is used
					verilogFile << "	.datac(gnd)," << std::endl;

				if (feedsCascaded)
				{
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
				}
				else
				{
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
				}
				
				//ib0502verilogFile << "	.datad(vcc )," << std::endl;
				verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"datac\";" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD728;" << std::endl;// D77D
				fpgaLogic[i][j][k].LUTMask = "D728";
				fpgaLogic[i][j][k].CoutFixedDefaultValue = false;
			}
		}
		else
		{
			assert(fpgaLogic[i][j][k].usedInputPorts < 4);
			if (fpgaLogic[i][j][k].usedInputPorts < 4)
			{
				 port3 = -1;
				// get the three ports used
				for (int x = 0; x < InputPortSize; x++)
				{
					if (fpgaLogic[i][j][k].inputPorts[x] && port1 < 0) // 1st port not yet set
					{
						port1 = x;
					}
					else if (fpgaLogic[i][j][k].inputPorts[x] && port2 < 0) // 1st port ws set, so set the second port
					{
						port2 = x;
					}
					else if (fpgaLogic[i][j][k].inputPorts[x])
					{
						port3 = x;
					}

				}
				// all three ports must be used
				assert(port1 > -1);
				assert(port2 > -1);
				assert(port3 > -1);

				if (fpgaLogic[i][j][k].inputPorts[Cin]) // Cin is used to connect to combout
				{
					//int pathFeederPort3, nodeFeederPort3;
					assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));
					assert(get_feeder(i, j, k, port3, pathFeederPort3, nodeFeederPort3));

					controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals

					verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;

				/*	if (check_control_signal_required(i, j, k)) //ib New check control
						verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << "Con )," << std::endl;
					else
						verilogFile << "	.dataa(vcc)," << std::endl;

						*/

					if (port1 == Cin) // port 1 is Cin
					{

						// get the x,y, z for port 3 feeder
						int feederXPort3 = paths[pathFeederPort3][nodeFeederPort3].x;
						int feederYPort3 = paths[pathFeederPort3][nodeFeederPort3].y;
						int feederZPort3 = paths[pathFeederPort3][nodeFeederPort3].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort3][feederYPort3][feederZPort3].isBRAM)
						{
							verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port3, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}

						int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
						int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
						int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
						{
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}


						//verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
						//verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout )," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
							
							//ib0502verilogFile << "	.datad(vcc )," << std::endl;
						
					}
					else if (port2 == Cin)// port 2 is  cin and port 1 must be used
					{
						assert(port2>-1);


						int feederXPort3 = paths[pathFeederPort3][nodeFeederPort3].x;
						int feederYPort3 = paths[pathFeederPort3][nodeFeederPort3].y;
						int feederZPort3 = paths[pathFeederPort3][nodeFeederPort3].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort3][feederYPort3][feederZPort3].isBRAM)
						{
							verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port3, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}

						int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
						int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
						int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
						{
							verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}














						//verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
						//verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << "_Cout )," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
						//ib0502verilogFile << "	.datad(vcc )," << std::endl;
					}
					else
					{
						assert(port3 == Cin);





						int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
						int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
						int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
						{
							verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}



						int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
						int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
						int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
						{
							verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}

						




						//verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						//verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << "_Cout )," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
					}
					verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h6996;" << std::endl;
					fpgaLogic[i][j][k].LUTMask = "6996";
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false; // probably not needed as this case has no cout as output

				}
				else // normal inputs to combout // common case
				{

					assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));
					assert(get_feeder(i, j, k, port3, pathFeederPort3, nodeFeederPort3));

					controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
																				//// instantiate the wysiwyg
					verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;



					int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
					int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
					int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
					{
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}



					int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
					int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
					int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
					{
						verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}

					int feederXPort3 = paths[pathFeederPort3][nodeFeederPort3].x;
					int feederYPort3 = paths[pathFeederPort3][nodeFeederPort3].y;
					int feederZPort3 = paths[pathFeederPort3][nodeFeederPort3].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort3][feederYPort3][feederZPort3].isBRAM)
					{
						verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port3, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}




					//verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
					//verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					//verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
					
					if (feedsCascaded)
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
					}
					else
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
					}
					
					//ib0502verilogFile << "	.datad(vcc )," << std::endl;
					verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"datac\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h6996;" << std::endl;
					fpgaLogic[i][j][k].LUTMask = "6996";
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false;
				}
			}
			else
			{
				std::cout << "ERROR: A LUT which uses " << fpgaLogic[i][j][k].usedInputPorts << " is not supported. SomethingWrong.";
			}
			
		}
		
	}
	else // output can be cout or (cout,combout)
	{
		if (fpgaLogic[i][j][k].usedInputPorts < 3) // max two ports are used
		{
			// get the two ports used
			for (int x = 0; x < InputPortSize; x++)
			{
				if (fpgaLogic[i][j][k].inputPorts[x] && port1 < 0) // 1st port not yet set
				{
					port1 = x;
				}
				else if (fpgaLogic[i][j][k].inputPorts[x]) // 1st port ws set, so set the second port
				{
					port2 = x;
				}

			}
			// at least one port must be used
			assert(port1 > -1);
			if (fpgaLogic[i][j][k].usedInputPorts == 2)
				assert(port2>-1);

			if (fpgaLogic[i][j][k].inputPorts[Cin]) // port Cin is used (possible inputs (Cin), (Cin,A), (Cin,B), (Cin,D))
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1)); // get the feeder for port1
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2)); // get the feeder for port2
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;
				if (port1 == Cin) // port 1 is Cin
				{
					if (port2 < 0) // no port 2, set port b to Vcc
					{
						verilogFile << "	.datab(vcc)," << std::endl; // set b to vcc so that cin is inverted to cout
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout)," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
						//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
					else // port 2 is also used 
					{


						int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
						int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
						int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

						// if it'snot a BRAM
						if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
						{
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						}
						else
						{
							int BRAMoutputPortIndex;
							std::string BRAMoutputPort;
							assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
								<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
								<< " )," << std::endl;

						}



					//	verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << ")," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout)," << std::endl;
						if (feedsCascaded)
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
						}
						else
						{
							verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						}
						
						//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
				}
				else // port 2 is  cin and port 1 must be used
				{


					int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
					int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
					int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
					{
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}





				//	verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << ")," << std::endl;
					verilogFile << "	.cin(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << "_Cout)," << std::endl;
					if (feedsCascaded)
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
					}
					else
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
					}
					
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				// output
				if (fpgaLogic[i][j][k].outputPorts[Combout - 5]) // combout is also used
					verilogFile << "	.combout(PATH" << path << "NODE" << node << ")," << std::endl;
				verilogFile << "	.cout(PATH" << path << "NODE" << node << "_Cout));" << std::endl;
				CoutSignals.push_back(Path_logic_component(path, node));
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				// function to decide which LUT mask to use
				if (check_down_link_edge_transition(i, j, k)) // if the down link has an inverting/non-inverting edge between its inputports (other than cin) and cout, then this bock should give 1/0 from its cout when it is shut off
				{
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h827D;" << std::endl; // cout is 1 when off, D77D
					fpgaLogic[i][j][k].LUTMask = "827D"; //D77D
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false;
				}
				else
				{
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD728;" << std::endl; // cout is zero when off
					fpgaLogic[i][j][k].LUTMask = "D728";
				}
				//	verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = \"D77D\";" << std::endl;


			}
			else // port Cin not used possible inputs affecting Cout (A), (B), (A,B), here we assume that the LUT feeding Cin is free and we will use it
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1)); // get the feeder for port1
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2)); // get the feeder for port2
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;

				int feederXPort1 = paths[pathFeederPort1][nodeFeederPort1].x;
				int feederYPort1 = paths[pathFeederPort1][nodeFeederPort1].y;
				int feederZPort1 = paths[pathFeederPort1][nodeFeederPort1].z;

				// if it'snot a BRAM
				if (!fpgaLogic[feederXPort1][feederYPort1][feederZPort1].isBRAM)
				{
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
				}
				else
				{
					int BRAMoutputPortIndex;
					std::string BRAMoutputPort;
					assert(get_feederPort_from_BRAM(i, j, k, port1, BRAMoutputPort, BRAMoutputPortIndex));
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1
						<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
						<< " )," << std::endl;

				}



			//	verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << ")," << std::endl;



				if (port2 > -1) // port 2 exists
				{

					int feederXPort2 = paths[pathFeederPort2][nodeFeederPort2].x;
					int feederYPort2 = paths[pathFeederPort2][nodeFeederPort2].y;
					int feederZPort2 = paths[pathFeederPort2][nodeFeederPort2].z;

					// if it'snot a BRAM
					if (!fpgaLogic[feederXPort2][feederYPort2][feederZPort2].isBRAM)
					{
						verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
					}
					else
					{
						int BRAMoutputPortIndex;
						std::string BRAMoutputPort;
						assert(get_feederPort_from_BRAM(i, j, k, port2, BRAMoutputPort, BRAMoutputPortIndex));
						verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2
							<< BRAMoutputPort << "[" << BRAMoutputPortIndex << "]"
							<< " )," << std::endl;

					}


					//verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << ")," << std::endl;
				}

				//		else // port 2 does not exist tie the other inpu to vcc
				//			verilogFile << "	.cin(vcc)," << std::endl;  //verilogFile << "	.datab(vcc)," << std::endl; new
				// cin will control the output
				//	verilogFile << "	.cin(PATH" << path << "NODE" << node << "Con)," << std::endl; removed for new stuff
				//	verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
				//////////////////////// new stuff
				if (port2 > -1) // port 2 is used
				{
					if (check_control_signal_required(i, j, k)) //ib New check control
						verilogFile << "	.cin(PATH" << path << "NODE" << node << "Con)," << std::endl;
					else
						verilogFile << "	.cin(vcc)," << std::endl;
					if (feedsCascaded)
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
					}
					else
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
					}
					
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				else // only one port non cin is used then do not use cin for control as it is not needed
				{
					if (check_control_signal_required(i, j, k)) //ib New check control
						verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
					else
						verilogFile << "	.dataa(vcc)," << std::endl;
					
					if (feedsCascaded)
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F_cascaded )," << std::endl;
					}
					else
					{
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
					}
					
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				//////////////////////////////// end new
				if (fpgaLogic[i][j][k].outputPorts[Combout - 5]) // combout is also used
					verilogFile << "	.combout(PATH" << path << "NODE" << node << ")," << std::endl;
				verilogFile << "	.cout(PATH" << path << "NODE" << node << "_Cout));" << std::endl;
				CoutSignals.push_back(Path_logic_component(path, node));
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				// check if the edge from operand to cout is inverting or non inverting
				inverting = true;
				for (int x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
				{
					if (paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portOut == Cout && !paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
					{
						inverting = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].inverting;
						break;
					}
				}
				if (inverting)
				{
					// new syuff
					if (port2 > -1) // new
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h9F60;" << std::endl; // cout is zero when off
						fpgaLogic[i][j][k].LUTMask = "9F60";
					}
					else
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hDD22;" << std::endl; // cout is zero when off (todo: check when writing controller for tester)
						fpgaLogic[i][j][k].LUTMask = "DD22";
					}
				}
				else
				{
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false; // since with this mask cout is set to 1 when off to allow the down link logic to be served correctly (inverting or non inverting).
					if (port2 > -1) // new
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h609F;" << std::endl; // cout is one when off, //6F9F
						fpgaLogic[i][j][k].LUTMask = "609F";
					}
					else
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h22DD;" << std::endl; // cout is one when off, //h77DD
						fpgaLogic[i][j][k].LUTMask = "22DD";
					}
				}

			}
		}
		else
		{
			std::cout << "ERROR: A LUT (Cout) which uses " << fpgaLogic[i][j][k].usedInputPorts << " is not supported. SomethingWrong.";
		}

	}
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//////////// New BRAM stuff ////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////



// returns the size of the port port of the BRAM located in x & y
// ignores the weird case of 2 bram wysiwygs located in the same pgysical BRAM
int getBRAMPortSize(int x, int y, int BRAMportInfo, std::vector<BRAM>  memories)
{
	assert(fpgaLogic[x][y][0].isBRAM);
	int memIndex = (int)fpgaLogic[x][y][0].indexMemories[0];
	int begin = (int)fpgaLogic[x][y][0].BRAMinputPorts[BRAMportInfo].size() - 1;
	BRAM memory = memories[memIndex];
	// must be a single memory in this x & y
	assert(fpgaLogic[x][y][0].indexMemories.size() == 1);
	if (BRAMportInfo == BRAMportAData)
	{
		begin = memory.portAUsedDataWidth - 1;
	}
	else if (BRAMportInfo == BRAMportBData)
	{

		begin = memory.portBUsedDataWidth - 1;

	}
	return begin + 1;
}

// return true if BRAM located in x,y,z uses any pin of input port port is tested
bool isPortTested(int x, int y, int z, int port)
{
	assert(z == 0);

	assert(fpgaLogic[x][y][z].isBRAM);

	for (int i = 0; i < fpgaLogic[x][y][z].BRAMinputPorts[port].size(); i++)
		if (fpgaLogic[x][y][z].BRAMinputPorts[port][i])
			return true;

	return false;
}

// return true if BRAM located in x,y,z uses any pin of output port portOut is tested
bool isOutPortTested(int x, int y, int z, int portOut)
{
	assert(z == 0);

	assert(fpgaLogic[x][y][z].isBRAM);

	for (int i = 0; i < fpgaLogic[x][y][z].BRAMoutputPorts[portOut].size(); i++)
		if (fpgaLogic[x][y][z].BRAMoutputPorts[portOut][i])
			return true;

	return false;
}
// return true if BRAMportInfor oin lox x, y is alread marked as need to control
bool isPortMarked(int x, int y, int BRAMportInfo, int & index)
{
	for (int i = 0; i < memoryControllers[x][y].size(); i++)
		if (memoryControllers[x][y][i].first == BRAMportInfo)
		{
			index = i;
			return true;
		}
	return false;
}


//returns a string that will be assigned to the intermediate signal of the BRAM
// this is used for BRAM as their WYISYWGS are harder
//example of returned string "{1'b0, 1'b0, PATHxNODEx, etc...}"
// the returned string connects thre BRAM to the previous nodes
// if nothing is connected it defaults it to 1 unless we it defaults to gnd
// memIndex is which memory index in the associated BRAM is being used 
//--> handles the case where more BRAMs are mapped into the same mem
std::string assign_BRAM_intemediateSignals(BRAM memory, int BRAMportInfo, int memIndex, std::vector<BRAM> memoriesList, bool & portUsed )
{
	// this function should only be called for datain
	//assert(BRAMportInfo == BRAMportAData || BRAMportInfo == BRAMportBData);
	// get the x and y 
	int x = memory.x;
	int y = memory.y;

	int pathOwner = fpgaLogic[x][y][0].owner.path;
	int nodeOwner = fpgaLogic[x][y][0].owner.node;

	// check that the memory is set
	assert(fpgaLogic[x][y][0].isBRAM);

	std::string interSignal = "";
	// loop across the pins of the requested port
	
	int last = -1;
	int begin = -1;


	portUsed = false;

	// if it's a single memory --> normal case
	if (fpgaLogic[x][y][0].countNumofMem == 1)
	{
		begin = (int)fpgaLogic[x][y][0].BRAMinputPorts[BRAMportInfo].size() - 1;
		// must be a single memory in this x & y
		assert(memIndex == 0);
		if (BRAMportInfo == BRAMportAData)
		{
			begin = memory.portAUsedDataWidth - 1;
		}
		else if (BRAMportInfo == BRAMportBData)
		{

			begin = memory.portBUsedDataWidth - 1;

		}
		last = 0;
	}
	else
	{
		assert(fpgaLogic[x][y][0].countNumofMem > 1);
		if (memIndex == 0)
		{
			if (BRAMportInfo == BRAMportAData)
			{
				begin = memoriesList[memIndex].portAUsedDataWidth - 1;
			}
			else
			{
				assert(BRAMportInfo == BRAMportAData);
				begin = memoriesList[memIndex].portBUsedDataWidth - 1;

			}
			last = 0;
		}
		else
		{
			assert(memIndex == 1);
			if (BRAMportInfo == BRAMportAData)
			{
				//begin = memoriesList[memIndex].portADataWidth - 1;
				begin = memoriesList[memIndex].portAUsedDataWidth + memoriesList[0].portAUsedDataWidth - 1;
				last = memoriesList[0].portAUsedDataWidth;
			}
			else
			{
				assert(BRAMportInfo == BRAMportBData);
				begin = memoriesList[memIndex].portBUsedDataWidth + memoriesList[0].portBUsedDataWidth - 1;
				last = memoriesList[0].portBUsedDataWidth;

			}
			

		}

	}

	for (int i = begin; i >= last; i--)
	{
		// is this pin being tested (part of a timing critical path)?
		if (fpgaLogic[x][y][0].BRAMinputPorts[BRAMportInfo][i])
		{
			int pathFeeder, nodeFeeder;
			assert(get_BRAM_feeder(x, y, 0, std::make_pair(BRAMportInfo, i), pathFeeder, nodeFeeder));
			interSignal += "PATH" + std::to_string(pathFeeder);
			interSignal += "NODE" + std::to_string(nodeFeeder);
			portUsed = true;
			if (i != last)
				interSignal += ", ";
		}
		// is not tested
		else
		{
			bool needsControlling = false;
			
			int numOfTestedPorts = 0;
			// testing the address port 
			if (isPortTested(x, y, 0, BRAMportAAddress))
			{
				numOfTestedPorts++;
				// memory is simple dual port
				// testing the address port of the write only port (port A)
				if (memory.operationMode == simpleDualPort)
				{
					// if it's the WE of port A, address of port b or data of port A we need to control it
					if (BRAMportInfo == BRAMportAWE || BRAMportInfo == BRAMportBAddress || BRAMportInfo == BRAMportAData)
					{
						needsControlling = true;
						interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
							+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
						if (i != last)
							interSignal += ", ";

						int controllerIndex = -1;
						if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
							memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
						else
							(memoryControllers[x][y])[controllerIndex].second++;

					}
				}
			}

			// testing the data port 
			if (isPortTested(x, y, 0, BRAMportBData) || isPortTested(x, y, 0, BRAMportAData))
			{
				numOfTestedPorts++;
				int data_inPort;
				// assign the correct port
				if (isPortTested(x, y, 0, BRAMportBData))
					data_inPort = BRAMportBData;
				else
					data_inPort = BRAMportAData;
				assert(!needsControlling);
				// if it's the WE of the corresponding port, address of corresponding port then we need to control it
				if (BRAMportInfo == data_inPort + 1 || BRAMportInfo == data_inPort + 2 )
				{
					needsControlling = true;
					interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
						+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
					if (i != last)
						interSignal += ", ";

					int controllerIndex = -1;
					if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
						memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
					else
						(memoryControllers[x][y])[controllerIndex].second++;

				}
				// if we are testing data in of port A and this memory is simple dual
				// then port a is write only, so we need to control port B address to read
				if (isPortTested(x, y, 0, BRAMportAData) && memory.operationMode == simpleDualPort)
				{

					if (BRAMportInfo == BRAMportBAddress)
					{
						needsControlling = true;
						interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
							+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
						if (i != last)
							interSignal += ", ";

						int controllerIndex = -1;
						if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
							memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
						else
							(memoryControllers[x][y])[controllerIndex].second++;

					}

				}

			}

			// write enable testing
			if (isPortTested(x, y, 0, BRAMportAWE)|| isPortTested(x, y, 0, BRAMportBWE))
			{
				numOfTestedPorts++;
				int data_inPort;
				// assign the correct port
				if (isPortTested(x, y, 0, BRAMportBWE))
					data_inPort = BRAMportBWE;
				else
					data_inPort = BRAMportAWE;
				assert(!needsControlling);
				// if it's the data in of the corresponding port, address of corresponding port then we need to control it
				if (BRAMportInfo == data_inPort - 1 || BRAMportInfo == data_inPort - 2)
				{
					needsControlling = true;
					interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
						+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
					if (i != last)
						interSignal += ", ";

					int controllerIndex = -1;
					if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
						memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
					else
						(memoryControllers[x][y])[controllerIndex].second++;

				}
				// if we are testing WE of port A and this memory is simple dual
				// then port a is write only, so we need to control port B address to read
				if (isPortTested(x, y, 0, BRAMportAWE) && memory.operationMode == simpleDualPort)
				{

					if (BRAMportInfo == BRAMportBAddress)
					{
						needsControlling = true;
						interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
							+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
						if (i != last)
							interSignal += ", ";

						int controllerIndex = -1;
						if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
							memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
						else
							(memoryControllers[x][y])[controllerIndex].second++;

					}

				}


			}

			// output data is being tested
			if (isOutPortTested(x, y, 0, BRAMportAout) || isOutPortTested(x, y, 0, BRAMportBout))
			{
				assert(!needsControlling);
				numOfTestedPorts++;
				int data_outPort;
				// assign the correct port
				if (isOutPortTested(x, y, 0, BRAMportBout))
					data_outPort = BRAMportBout;
				else
					data_outPort = BRAMportAout;

				
				if (memory.operationMode == simpleDualPort)
				{
					// check that the tested port is port B as A must be a write only
					assert(data_outPort == BRAMportBout);

					// need to control WE_A, data_in_A, address_A & address_B
					if (BRAMportInfo == BRAMportAWE || BRAMportInfo == BRAMportAAddress 
						|| BRAMportInfo == BRAMportAData || BRAMportInfo == BRAMportBAddress)
					{
						needsControlling = true;
						interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
							+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
						if (i != last)
							interSignal += ", ";
													int controllerIndex = -1;
						if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
							memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
						else
							(memoryControllers[x][y])[controllerIndex].second++;

					}

				}
				else// the tested port can be used for reading and writing
				{	
					if (data_outPort == BRAMportAout)
					{
						// need to control WE_A, data_in_A, address_A 
						if (BRAMportInfo == BRAMportAWE || BRAMportInfo == BRAMportAAddress
							|| BRAMportInfo == BRAMportAData)
						{
							needsControlling = true;
							interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
								+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
							if (i != last)
								interSignal += ", ";
							int controllerIndex = -1;
							if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
								memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
							else
								(memoryControllers[x][y])[controllerIndex].second++;

						}
					}
					else
					{
						// need to control WE_B, data_in_B, address_B 
						if (BRAMportInfo == BRAMportBWE || BRAMportInfo == BRAMportBAddress
							|| BRAMportInfo == BRAMportBData)
						{
							needsControlling = true;
							interSignal += "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
								+ "_" + portNumbertoName(BRAMportInfo) + "_control_" + std::to_string(i);
							if (i != last)
								interSignal += ", ";
							int controllerIndex = -1;
							if (!isPortMarked(x, y, BRAMportInfo, controllerIndex))
								memoryControllers[x][y].push_back(std::make_pair(BRAMportInfo, 1));
							else
								(memoryControllers[x][y])[controllerIndex].second++;

						}
					}
				
				}




				
			}
			

			assert(numOfTestedPorts <= 1);

			// if it doesnot need controlling
			if (!needsControlling)
			{
				if (BRAMportInfo == BRAMportAWE || BRAMportInfo == BRAMportBWE)
					interSignal += "1'b0";
				else
					interSignal += "1'b1";
				if (i != last)
					interSignal += ", ";
			}
		}
	}

	return interSignal;
}

// prints a WYSIWYG for a single BRAM (memoryCell) into the verilogFile
// adds the curent BRAM to the sinks if the address port from 
// a read capable port is to be tested
std::string BRAM_WYSIWYG_cycloneIV(BRAM memoryCell, std::ofstream & verilogFile, bool testingBRAMsOnly, std::vector<BRAM>  memories, std::vector <Path_logic_component> & sinks)
{

	// double check that this is stored as memory
	int x = memoryCell.x;
	int y = memoryCell.y;

	assert(fpgaLogic[x][y][0].isBRAM);

	int pathOwner = fpgaLogic[memoryCell.x][memoryCell.y][0].owner.path;
	int nodeOwner = fpgaLogic[memoryCell.x][memoryCell.y][0].owner.node;


	// list of memories sharing the same physical ram
	std::vector<BRAM> memoriesList;

	for (int i = 0; i < fpgaLogic[x][y][0].indexMemories.size(); i++)
	{
		memoriesList.push_back(memories[fpgaLogic[x][y][0].indexMemories[i]]);
	}

	bool isDoubleMem = fpgaLogic[x][y][0].countNumofMem > 1;

	std::string intermediateSignals = "";

	verilogFile << "//BRAM WYSIWYG " << std::endl << std::endl;
	

	// true if 
	bool portUsedBefore = false;

	for (int i = 0; i < fpgaLogic[x][y][0].countNumofMem; i++)
	{
		// at most 2 brams in the same physical
		assert(i < 2);

	//	if (!isDoubleMem)
	//	{
	//		verilogFile << "cycloneive_ram_block " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t (" << std::endl;
	//	}
	//	else
	//	{
			verilogFile << "cycloneive_ram_block " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << "(" << std::endl;
	//	}

		if (memoryCell.clr0)
		{
			// for now just set clr to zero
			// todo: handle testing it later
			//verilogFile << ".clr0(" << "PATH" << pathOwner << "NODE" << nodeOwner << "_clear)," << std::endl;
			verilogFile << ".clr0(1'b0)," << std::endl;
		}
		if (memoryCell.portAAddressWidth > 0)
		{
			//assign_BRAM_intemediateSignals
		/*	verilogFile << ".portaaddr( PATH" << pathOwner << "NODE" << nodeOwner << "_a_address)," << std::endl;

			if (i == 0)
			{
				// get the internal connections in string to print later
				intermediateSignals += "wire [" + std::to_string(memoryCell.portAAddressWidth - 1) + ":0] "
					+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
					+ "_a_address; \n";
				intermediateSignals += "assign PATH" + std::to_string(pathOwner);
				intermediateSignals += "NODE" + std::to_string(nodeOwner);
				intermediateSignals += "_a_address = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportAAddress) + "};\n";
			}
			*/
			bool portUsed;
			verilogFile << ".portaaddr({" << assign_BRAM_intemediateSignals(memoryCell, BRAMportAAddress, i, memoriesList, portUsed) << "})," << std::endl;

			// if a port is used before then the current port must not be used
			// this assures that we test one port of the BRAM at a time
			if (i == 0)
			{
				assert(!(portUsedBefore & portUsed));
				portUsedBefore = portUsed;

				// port A address is used
				if (portUsed)
				{
					// not somple dula mode
					// this means that we can read from address A
					if (memoryCell.operationMode != simpleDualPort)
					{
						// let's push the sinks of this memory to sinks
						// so we can test them using the main controller
						sinks.push_back(Path_logic_component(pathOwner, nodeOwner));
					}

				}
			}
		}

		if (memoryCell.portAUsedDataWidth > 0)
		{

			// new

			bool portUsed = false;
			// if it's not a ROM then add data in
			//if(memoryCell.operationMode != ROMPort)
			verilogFile << ".portadatain({" << assign_BRAM_intemediateSignals(memoriesList[i], BRAMportAData, i, memoriesList, portUsed) << "})," << std::endl;

			// if a port is used before then the current port must not be used
			// this assures that we test one port of the BRAM at a time
			if (i == 0)
			{
				assert(!(portUsedBefore & portUsed));
				portUsedBefore = portUsed;
			}
			/*
			if (!isDoubleMem)
			{
				verilogFile << ".portadatain(PATH" << pathOwner << "NODE" << nodeOwner << "_a_datain)," << std::endl;
			}
			else
			{
				if (i == 0)
				{
					verilogFile << ".portadatain(PATH" << pathOwner << "NODE" << nodeOwner << "_a_datain[" 
						<< memoriesList[0].portAUsedDataWidth -1 << ":0])," << std::endl;
				}
				else
				{
					verilogFile << ".portadatain(PATH" << pathOwner << "NODE" << nodeOwner << "_a_datain["
						<< memoriesList[1].portAUsedDataWidth + memoriesList[0].portAUsedDataWidth - 1 << ":" <<  memoriesList[0].portAUsedDataWidth << "])," << std::endl;
				}
			}
			// get the internal connections in string to print later
			if (i == 0)
			{
				if (!isDoubleMem)
				{
					intermediateSignals += "wire [" + std::to_string(memoryCell.portAUsedDataWidth - 1) + ":0] "
						+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
						+ "_a_datain; \n";
				}
				else
				{
					intermediateSignals += "wire [" + std::to_string(memoriesList[1].portAUsedDataWidth + memoriesList[0].portAUsedDataWidth - 1) + ":0] "
						+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
						+ "_a_datain; \n";
				}
				intermediateSignals += "assign PATH" + std::to_string(pathOwner);
				intermediateSignals += "NODE" + std::to_string(nodeOwner);
				intermediateSignals += "_a_datain = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportAData) + "};\n";
			}
			*/
			// in simple dual mode, A is used in writing so it has no data out
			if (memoryCell.operationMode != simpleDualPort)
			{

				if (!isDoubleMem)
				{
					verilogFile << ".portadataout( PATH" << pathOwner << "NODE" << nodeOwner << "_a_dataout)," << std::endl;

					intermediateSignals += "wire [" + std::to_string(memoryCell.portAUsedDataWidth - 1) + ":0] "
						+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
						+ "_a_dataout; \n";
				}
				else
				{
					if (i == 0)
					{
						verilogFile << ".portadataout(PATH" << pathOwner << "NODE" << nodeOwner << "_a_dataout["
							<< memoriesList[0].portAUsedDataWidth - 1 << ":0])," << std::endl;

						intermediateSignals += "wire [" + std::to_string(memoriesList[1].portAUsedDataWidth + memoriesList[0].portAUsedDataWidth - 1) + ":0] "
							+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
							+ "_a_dataout; \n";
					}
					else
					{
						verilogFile << ".portadataout(PATH" << pathOwner << "NODE" << nodeOwner << "_a_dataout["
							<< memoriesList[1].portAUsedDataWidth + memoriesList[0].portAUsedDataWidth - 1 << ":"
							<< memoriesList[0].portAUsedDataWidth << "])," << std::endl;
					}

				}
			}
		}

		if (memoryCell.portAWE)
		{
			bool portUsed;
			verilogFile << ".portawe({" << assign_BRAM_intemediateSignals(memoryCell, BRAMportAWE, i, memoriesList, portUsed) << "})," << std::endl;
			// if a port is used before then the current port must not be used
			// this assures that we test one port of the BRAM at a time
			if (i == 0)
			{
				assert(!(portUsedBefore & portUsed));
				portUsedBefore = portUsed;
			}
			/*
			verilogFile << ".portawe( PATH" << pathOwner << "NODE" << nodeOwner << "_a_we)," << std::endl;

			if (i == 0)
			{
				// get the internal connections in string to print later
				intermediateSignals += "wire PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
					+ "_a_we; \n";
				intermediateSignals += "assign PATH" + std::to_string(pathOwner);
				intermediateSignals += "NODE" + std::to_string(nodeOwner);
				intermediateSignals += "_a_we = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportAWE) + "};\n";
			}
			*/
		}

		// singals to the BRAM for port B

		if (memoryCell.portBAddressWidth > 0)
		{
			bool portUsed;
			verilogFile << ".portbaddr({" << assign_BRAM_intemediateSignals(memoryCell, BRAMportBAddress, i, memoriesList, portUsed) << "})," << std::endl;
			// if a port is used before then the current port must not be used
			// this assures that we test one port of the BRAM at a time
			if (i == 0)
			{
				assert(!(portUsedBefore & portUsed));
				portUsedBefore = portUsed;
				// port B address is used
				// port B always have an output
				// if port B is used then we must be able to rad from it
				if (portUsed)
				{

					// let's push the sinks of this memory to sinks
					// so we can test them using the main controller
					sinks.push_back(Path_logic_component(pathOwner, nodeOwner));

				}
			}
			/*
			verilogFile << ".portbaddr( PATH" << pathOwner << "NODE" << nodeOwner << "_b_address)," << std::endl;

			// get the internal connections in string to print later

			intermediateSignals += "wire [" + std::to_string(memoryCell.portBAddressWidth - 1) + ":0] "
				+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
				+ "_b_address; \n";

			intermediateSignals += "assign PATH" + std::to_string(pathOwner);
			intermediateSignals += "NODE" + std::to_string(nodeOwner);
			intermediateSignals += "_b_address = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportBAddress) + "};\n";
			*/
		}

		if (memoryCell.portBUsedDataWidth > 0)
		{

				
			// in simple dual mode, B is used in reading so it has no data in
			if (memoryCell.operationMode != simpleDualPort)
			{

				bool portUsed;
				verilogFile << ".portbdatain({" << assign_BRAM_intemediateSignals(memoryCell, BRAMportBData, i, memoriesList, portUsed) << "})," << std::endl;
				// if a port is used before then the current port must not be used
				// this assures that we test one port of the BRAM at a time
				if (i == 0)
				{
					assert(!(portUsedBefore & portUsed));
					portUsedBefore = portUsed;
				}
				/*
				verilogFile << ".portbdatain( PATH" << pathOwner << "NODE" << nodeOwner << "_b_datain)," << std::endl;

				// get the internal connections in string to print later

				intermediateSignals += "wire [" + std::to_string(memoryCell.portBUsedDataWidth - 1) + ":0] "
					+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
					+ "_b_datain; \n";

				intermediateSignals += "assign PATH" + std::to_string(pathOwner);
				intermediateSignals += "NODE" + std::to_string(nodeOwner);
				intermediateSignals += "_b_datain = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportBData) + "};\n";
				*/
			}

			

			verilogFile << ".portbdataout(PATH" << pathOwner << "NODE" << nodeOwner << "_b_dataout)," << std::endl;

			intermediateSignals += "wire [" + std::to_string(memoryCell.portBUsedDataWidth - 1) + ":0] "
				+ "PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
				+ "_b_dataout; \n";
		}

		if (memoryCell.portBWE)
		{
			bool portUsed;
			verilogFile << ".portbwe({" << assign_BRAM_intemediateSignals(memoryCell, BRAMportBWE, i, memoriesList, portUsed) << "})," << std::endl;
			// if a port is used before then the current port must not be used
			// this assures that we test one port of the BRAM at a time
			if (i == 0)
			{
				assert(!(portUsedBefore & portUsed));
				portUsedBefore = portUsed;
			}
			/*
			verilogFile << ".portbwe(PATH" << pathOwner << "NODE" << nodeOwner << "_b_we)," << std::endl;

			intermediateSignals += "wire PATH" + std::to_string(pathOwner) + "NODE" + std::to_string(nodeOwner)
				+ "_b_we; \n";
			// get the internal connections in string to print later
			intermediateSignals += "assign PATH" + std::to_string(pathOwner);
			intermediateSignals += "NODE" + std::to_string(nodeOwner);
			intermediateSignals += "_b_we = {" + assign_BRAM_intemediateSignals(memoryCell, BRAMportBWE) + "};\n";
			*/
		}

		// quartus was compaining when a true dual port or a simple dual port memory
		// has no read enable connection
		if (memoryCell.operationMode == trueDualPort)
		{
			verilogFile << ".portbre( 1'b1)," << std::endl;
			verilogFile << ".portare( 1'b1)," << std::endl;
		}
		else
		{
			if (memoryCell.operationMode == simpleDualPort)
				verilogFile << ".portbre( 1'b1)," << std::endl;
		}

		verilogFile << ".clk0(CLK)" << std::endl;

		// BRAM mode and stuff
		verilogFile << ");" << std::endl;

		verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".operation_mode = ";
		if (memoryCell.operationMode == trueDualPort)
			verilogFile << "\"bidir_dual_port\";" << std::endl;
		else if (memoryCell.operationMode == simpleDualPort)
			verilogFile << "\"dual_port\";" << std::endl;
		else
			verilogFile << "\"single_port\";" << std::endl;

		verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".ram_block_type = \"M9K\" ;" << std::endl;
		verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".logical_ram_name = \"PATH" << pathOwner << "NODE" << nodeOwner << "_log_" << i << " \" ;" << std::endl;
		verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".data_interleave_width_in_bits = 1;" << std::endl;
		verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".data_interleave_offset_in_bits = 1;" << std::endl;

		if (memoryCell.portAAddressWidth > 0)
		{
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_logical_ram_depth = " << pow(2, memoryCell.portAAddressWidth) << " ;" << std::endl;
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_first_address = 0;" << std::endl;
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_last_address = " << pow(2, memoryCell.portAAddressWidth) - 1 << " ;" << std::endl;
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_address_width = " << memoryCell.portAAddressWidth << " ;" << std::endl;
		}
		if (memoryCell.portAUsedDataWidth > 0)
		{
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_logical_ram_width = " << memoryCell.portAUsedDataWidth << " ;" << std::endl;
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_first_bit_number = 0;" << std::endl;
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_data_width = " << memoryCell.portAUsedDataWidth << " ;" << std::endl;
		}
		if (memoryCell.clr0)
		{
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_address_clear = \"clear0\";" << std::endl;
			// in simple dual mode, A is used in writing so it has no data out
			if (memoryCell.operationMode != simpleDualPort)
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_data_out_clear = \"clear0\";" << std::endl;
		}
		else
		{
			verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_address_clear = \"none\";" << std::endl;
			// in simple dual mode, A is used in writing so it has no data out
			if (memoryCell.operationMode != simpleDualPort)
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_data_out_clear = \"none\";" << std::endl;
		}


		if (memoryCell.portARegistered)
		{
			// in simple dual mode, A is used in writing so it has no data out
			if (memoryCell.operationMode != simpleDualPort)
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_data_out_clock = \"clock0\";" << std::endl;
		}
		else
		{
			// in simple dual mode, A is used in writing so it has no data out
			if (memoryCell.operationMode != simpleDualPort)
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_a_data_out_clock = \"none\";" << std::endl;
		}

		// parameters for port b
		if (memoryCell.operationMode == trueDualPort || memoryCell.operationMode == simpleDualPort)
		{
			if (memoryCell.portBAddressWidth > 0)
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_logical_ram_depth = " << pow(2, memoryCell.portBAddressWidth) << " ;" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_first_address = 0;" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_last_address = " << pow(2, memoryCell.portBAddressWidth) - 1 << " ;" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_address_width = " << memoryCell.portBAddressWidth << " ;" << std::endl;
			}
			if (memoryCell.portBUsedDataWidth > 0)
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_logical_ram_width = " << memoryCell.portBUsedDataWidth << " ;" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_first_bit_number = 0;" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_width = " << memoryCell.portBUsedDataWidth << " ;" << std::endl;
			}
			if (memoryCell.clr0)
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_address_clear = \"clear0\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_out_clear = \"clear0\";" << std::endl;
			}
			else
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_address_clear = \"none\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_out_clear = \"none\";" << std::endl;
			}

			// port b must bec clocked with the same clock as port a

			if (memoryCell.portBWE)
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_in_clock = \"clock0\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_write_enable_clock = \"clock0\";" << std::endl;
			}

			if (memoryCell.portBRegistered)
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_out_clock = \"clock0\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_read_enable_clock = \"clock0\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_address_clock = \"clock0\";" << std::endl;
			}
			else
			{
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_data_out_clock = \"none\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_read_enable_clock = \"clock0\";" << std::endl;
				verilogFile << "defparam " << "PATH" << pathOwner << "NODE" << nodeOwner << "_t_" << i << ".port_b_address_clock = \"clock0\";" << std::endl;
			}

		}

	}
	if (!testingBRAMsOnly)
	{
	//	verilogFile << "//Intermediate signals  " << std::endl << std::endl;
	//	verilogFile << intermediateSignals << std::endl << std::endl;
	}

	/*
	if (portUsedBefore)
	{
		memorySinks.push_back(Path_logic_component(pathOwner, nodeOwner));
	}
	*/

	return intermediateSignals;
}


// pritns WysyWigs for all BRAMs, this might be used for testing BRAMs only
void generate_BRAMsWYSYWIGs(std::vector<BRAM>  memories, int bitStreamNumber)
{
	std::ofstream verilogFile;
	std::string verilogFileName = "top_BRAM_" + std::to_string(bitStreamNumber) + ".v";
	verilogFile.open(verilogFileName);

	verilogFile << "module top (" << std::endl;


	for (unsigned int i = 0; i < memories.size(); i++)
	{


		/// memoy inputs
		if (memories[i].portAAddressWidth > 0)
			verilogFile << "input " << "[" << memories[i].portAAddressWidth - 1 << ":0] BRAM_" << i << "_a_address," << std::endl;

		if (memories[i].portAUsedDataWidth > 0)
			verilogFile << "input " << "[" << memories[i].portAUsedDataWidth - 1 << ":0] BRAM_" << i << "_a_datain," << std::endl;

		if (memories[i].portAWE)
			verilogFile << "input " << "BRAM_" << i << "_a_we," << std::endl;

		if (memories[i].portBAddressWidth > 0)
			verilogFile << "input " << "[" << memories[i].portBAddressWidth - 1 << ":0] BRAM_" << i << "_b_address," << std::endl;

		if (memories[i].portBUsedDataWidth > 0)
			// in simple dual mode, B is used in reading so it has no data in
			if (memories[i].operationMode != simpleDualPort)
				verilogFile << "input " << "[" << memories[i].portBUsedDataWidth - 1 << ":0] BRAM_" << i << "_b_datain," << std::endl;

		if (memories[i].portBWE)
			verilogFile << "input " << "BRAM_" << i << "_b_we," << std::endl;

		if (memories[i].clr0)
			verilogFile << "input " << "BRAM_" << i << "_clear," << std::endl;


		/// memoy outputs
		if (memories[i].portAUsedDataWidth > 0)
		{
			// in simple dual mode, A is used in writing so it has no data out
			if (memories[i].operationMode != simpleDualPort)
				verilogFile << "output " << "[" << memories[i].portAUsedDataWidth - 1 << ":0] BRAM_" << i << "_a_dataout," << std::endl;
		}
		if (memories[i].portBUsedDataWidth > 0)
			verilogFile << "output " << "[" << memories[i].portBUsedDataWidth - 1 << ":0] BRAM_" << i << "_b_dataout," << std::endl;


	}

	verilogFile << ");" << std::endl << std::endl << std::endl;

	verilogFile << "//BRAMs instantiations" << std::endl;

	std::vector <Path_logic_component>  dummySinks;

	for (unsigned int i = 0; i < memories.size(); i++)
	{

		BRAM_WYSIWYG_cycloneIV(memories[i], verilogFile, false, memories, dummySinks);
	}


	verilogFile << "endmodule" << std::endl;


	for (unsigned int i = 0; i < memories.size(); i++)
	{
		verilogFile << "set_location_assignment M9K_X" << memories[i].x << "_Y" << memories[i].y << "_N0 -to " << "BRAM_" << i << "_t" << std::endl;
	}


}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//////////// End BRAM stuff ////////////////////////////////////
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


void create_or_tree(int inputs, int LUTinputs, int number, std::ofstream& controllerFile)
{

	controllerFile << std::endl << std::endl << "/////// Creating deeply pipelined order tree number " << number << " with " << inputs << " inputs" << std::endl;

	controllerFile << "module or_tree_" << number << " ( input [" << inputs - 1 << ":0] in , input reset, input CLK, output out);" << std::endl;
	if (inputs <= LUTinputs)
	{
		controllerFile << "reg out_temp;" << std::endl;;
		controllerFile << "always @ (posedge CLK or posedge reset) begin" << std::endl;
		controllerFile << "\tif (reset)" << std::endl;
		controllerFile << "\t\tout_temp <= 1'b0;" << std::endl;
		controllerFile << "\telse" << std::endl;
		controllerFile << "\t\tout_temp <= |in;" << std::endl;
		controllerFile << "end" << std::endl;;
		controllerFile << "assign out = out_temp;" << std::endl;
		controllerFile << "endmodule";
		return;

	}
	int numOfLevels = (int)ceil((log(inputs) / log(LUTinputs)));
	int currentLevelLength = (int)ceil(inputs / 4.0);
	int i, j;
	std::vector<int> levelSizes;
	levelSizes.resize(0);
	for (i = 0; i < numOfLevels; i++)
	{
		levelSizes.push_back(currentLevelLength);
		controllerFile << "reg [" << currentLevelLength - 1 << ":0] level_" << i + 1 << " /*synthesis noprune */ ; " << std::endl;
		currentLevelLength = (int)ceil(currentLevelLength / 4.0);
	}

	controllerFile << "always @ (posedge CLK or posedge reset) begin" << std::endl;
	controllerFile << "\tif (reset) begin" << std::endl;

	for (i = 0; i < numOfLevels; i++)
	{
		controllerFile << "\t\tlevel_" << i + 1 << " <= " << levelSizes[i] << "'b0 ; " << std::endl;
	}
	controllerFile << "\tend" << std::endl;
	controllerFile << "\telse begin" << std::endl;
	for (i = 0; i < numOfLevels; i++)
	{
		for (j = 0; j < levelSizes[i]; j++)
		{
			if (j < levelSizes[i] - 1) // not the last or in this level
			{
				if (i == 0) // first level so takes directly from th input
					controllerFile << "\t\tlevel_" << i + 1 << " [" << j << "] <= |in[" << (j + 1)*LUTinputs - 1 << ":" << j*LUTinputs << "];" << std::endl;
				else
					controllerFile << "\t\tlevel_" << i + 1 << " [" << j << "] <= |level_" << i << "[" << (j + 1)*LUTinputs - 1 << ":" << j*LUTinputs << "];" << std::endl;

			}
			else
			{
				if (i == 0) // first level so takes directly from th input
					controllerFile << "\t\tlevel_" << i + 1 << " [" << j << "] <= |in[" << inputs - 1 << ":" << j*LUTinputs << "];" << std::endl;
				else
					controllerFile << "\t\tlevel_" << i + 1 << " [" << j << "] <= |level_" << i << "[" << levelSizes[i - 1] - 1 << ":" << j*LUTinputs << "];" << std::endl;

			}
		}
	}
	controllerFile << "\tend" << std::endl;
	controllerFile << "end" << std::endl;
	assert(levelSizes[levelSizes.size() - 1] == 1);
	controllerFile << "assign out = level_" << levelSizes.size() << "[0];" << std::endl;
	controllerFile << "endmodule" << std::endl;
}




void create_RCF_file(int bitStreamNumber, std::vector<BRAM>  memories)
{
	int i, j, k, l;
	int x;
//	int total = 0;
	int destX, destY, destZ;
	std::ofstream RoFile;
	std::string RoFileName = "RCF_File_" + std::to_string(bitStreamNumber) + ".txt";
	RoFile.open(RoFileName);
	int path = -1;
	int node = -1;
	int pathDest = -1;
	int nodeDest = -1;
	int label;
	std::map<std::string, int> branchLabel;

	// map to store which signal (output) is being routed from the current atom
	// added this to help with BRAMs having multiple output
	// so we can't just print connections to the RCF file
	// we need to figure out whichsignal is it
	std::unordered_map<std::string, std::string> outputPorts;
	std::string completeFile;

	bool foundSource = false;
    bool addBrace = false;
        
        //reset all reserved fanout placements for the next bitstream
        for(unsigned i = 0; i < FPGAsizeX; i++){
            for(unsigned j = 0; j < FPGAsizeY; j++){
                for(unsigned k = 0; k < FPGAsizeZ; k++){
                    fanoutLUTPlacement[i][j][k] = false;
                }
            }
        }
        
        terminal_LUTS.clear();
        
	for (i = 0; i < FPGAsizeX; i++) // loop across all cells
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (i == 77 && j == 69 && k == 31)
					std::cout << "DEbug double BRAM Address 0" << std::endl;

				if (fpgaLogic[i][j][k].utilization>0) // this cell is used, so lets check its fanouts
				{
					path = -1;
					node = -1;
					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // find the undeleted most critical path using this node
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							//		LoFile << fpgaLogic[i][j][k].nodes[x].path << "NODE" << fpgaLogic[i][j][k].nodes[x].node << "_t" << std::endl;
							break;
						}
					}
					assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node); // check that the owner is correct
					//there must be path, otherwise utilization should have been 0
					assert(path > -1);

					// clear outputPorts map

					


					outputPorts.clear();


					// clear branch map
					branchLabel.clear();
					label = 0;
					bool first_route_found = false;
					// write source signal
					// loop across all routes in the cell i,j,k
					for (l = 0; l < (int)fpgaLogic[i][j][k].connections.size(); l++) 
					{

						// newBRAM stuff

						std::string tempKey;

						if (!fpgaLogic[i][j][k].isBRAM)
						{
							tempKey = "notBRAM";
						}
						else
						{
							assert(k == 0);
							// if it's a BRAM let's get the key for this specific port
							tempKey = "BRAM" + std::to_string(fpgaLogic[i][j][k].connections[l].memorySourcePort.first) + "_"
								+ std::to_string(fpgaLogic[i][j][k].connections[l].memorySourcePort.first);
							// siince this is a new signal
							first_route_found = false;
						}

						// check if this port was used before
						auto iterOutputPorts = outputPorts.find(tempKey);

						// this port was not seen before, so let's create a new entry in the map
						if (iterOutputPorts == outputPorts.end())
						{
							outputPorts.insert(std::pair<std::string, std::string>(tempKey, ""));
							iterOutputPorts = outputPorts.find(tempKey);
							assert(iterOutputPorts != outputPorts.end());
						}
						// so there is a connection but without any used resources, 
						//this must be a connection betwee a LUT and a FF that are in the same LE.
						//So I am gonna check that
						if (fpgaLogic[i][j][k].connections[l].usedRoutingResources.size() == 0) 
						{
							int destinX = fpgaLogic[i][j][k].connections[l].destinationX;
							int destinY = fpgaLogic[i][j][k].connections[l].destinationY;
							int destinZ = fpgaLogic[i][j][k].connections[l].destinationZ;

							// deleted
							if (destinX == -1)
								continue;
							// must be areg
							assert(destinZ%LUTFreq != 0);
							assert(fpgaLogic[destinX][destinY][destinZ].FFMode == dInput);

							// if this was the last connection, we need to close the routing signal with a brace
							// We only have to close if it is not the only connection
							if (l == (int)fpgaLogic[i][j][k].connections.size() - 1 && l != 0)
							{
								RoFile << "}" << std::endl;
								(iterOutputPorts->second) += "}";
							}

							continue;
						}
						if (fpgaLogic[i][j][k].connections[l].destinationX == -1)
						{ 
							if (l == (int)fpgaLogic[i][j][k].connections.size() - 1 && first_route_found)
							{
								RoFile << "}" << std::endl;
								(iterOutputPorts->second) += "}";
							}
							continue;
						}
						





						if (!first_route_found)//if (l == 0)
						{
							// if the source is not a BRAM
							if (!fpgaLogic[i][j][k].isBRAM)
							{
								RoFile << "} \n signal_name = PATH" << path << "NODE" << node << " {" << std::endl;
								(iterOutputPorts->second) += "} \n signal_name = PATH" + std::to_string(path) + "NODE" + std::to_string(node) + " {\n";
							}
							else // source is a BRAM
							{
								assert(k == 0);
								RoFile << "} \n signal_name = PATH" << path << "NODE" << node;
								(iterOutputPorts->second) += "} \n signal_name = PATH" + std::to_string(path) + "NODE" + std::to_string(node);
								// uses port A
								if (fpgaLogic[i][j][k].connections[l].memorySourcePort.first == BRAMportAout)
								{
									RoFile << "_a_dataout[" << fpgaLogic[i][j][k].connections[l].memorySourcePort.second << "] {"  << std::endl;
									(iterOutputPorts->second) += "_a_dataout["
										+ std::to_string(fpgaLogic[i][j][k].connections[l].memorySourcePort.second)
										+ "] {\n";
								}
								else // uses BRAM port B
								{
									if (fpgaLogic[i][j][k].connections[l].memorySourcePort.first != BRAMportBout)
									{
										std::cout << "Path " << path << " node " << node << std::endl;
										std::cout << " Shit is " << paths[path][node].BRAMPortOut << std::endl;
 									}
									assert(fpgaLogic[i][j][k].connections[l].memorySourcePort.first == BRAMportBout);
									RoFile << "_b_dataout[" << fpgaLogic[i][j][k].connections[l].memorySourcePort.second << "] {" << std::endl;
									(iterOutputPorts->second) += "_b_dataout["
										+ std::to_string(fpgaLogic[i][j][k].connections[l].memorySourcePort.second)
										+ "] {\n";

								}

							}
							first_route_found = true;
						}
						foundSource = false;
						// start writing rcf for this connection
						//	RoFile << "signal_name = PATH" << path << "NODE" << node << " {" << std::endl;
						for (x = 0; x < (int)fpgaLogic[i][j][k].connections[l].usedRoutingResources.size(); x++)
						{

							// check if the routing resource has been used before
							auto iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x]);
							if (foundSource)
							{
								assert(iter == branchLabel.end());
								// write the connection to the rcf
								RoFile << "\tlabel = label_" << label << "_" 
									<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", "
									<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" 
									<< std::endl;

								(iterOutputPorts->second) += "\tlabel = label_" + std::to_string(label) + "_"
									+ (fpgaLogic[i][j][k].connections[l].usedRoutingResources[x]) + ", "
									+ fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] + ";\n";

								// insert the label
                                                                
                                                                
 //*****                        //either mark as used, or look for label
                                                                
                                                                
								branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
								label++;
								continue;

							}
							if (iter == branchLabel.end()) // if not found then 
							{
								if (x == 0) // the first routing resource was not used before, so the source is not a label
								{
									// write the connection to the rcf
									RoFile << "\tlabel = label_" << label << "_" 
										<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", "
										<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" 
										<< std::endl;

									(iterOutputPorts->second) += "\tlabel = label_" + std::to_string(label) + "_"
										+ fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] + ", "
										+ fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] + ";\n";

									// insert the label
                                                                        
                                                                        
                                    //either mark as used or look for label
									branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
									label++;
									foundSource = true;
								}
								else // must use branch_point 
								{
									iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x - 1]);
									// element must be found
									assert(iter != branchLabel.end());
									// start from the routing resource before the current (that was definetly used before)
									RoFile << "\tbranch_point = label_" << iter->second << "_" << iter->first << "; " << std::endl;

									(iterOutputPorts->second) += "\tbranch_point = label_" + std::to_string(iter->second) + "_"
										+ iter->first + ";\n";

									RoFile << "\tlabel = label_" << label << "_" 
										<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " 
										<< fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" 
										<< std::endl;

									(iterOutputPorts->second) += "\tlabel = label_" + std::to_string(label) + "_"
										+ fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] + ", "
										+ fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] + ";\n";
									
                                    //either mark as used or look for label                                                  
                                    branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
									label++;
									foundSource = true;


								}

							}


						}
						if (!foundSource) // this means the connection is useing the exact sam resources used before
						{
							auto iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x - 1]);
							// element must be found
							assert(iter != branchLabel.end());
							// start from the routing resource before the current (that was definetly used before)
							RoFile << "\tbranch_point = label_" << iter->second << "_" << iter->first << "; " << std::endl;
							(iterOutputPorts->second) += "\tbranch_point = label_" + std::to_string(iter->second) + "_" 
								+ iter->first + ";\n";
						}

						// write last line dest = {...}, ....; but first we have to find who owns the destination LUT
						destX = fpgaLogic[i][j][k].connections[l].destinationX;
						destY = fpgaLogic[i][j][k].connections[l].destinationY;
						destZ = fpgaLogic[i][j][k].connections[l].destinationZ;
						pathDest = -1;
						nodeDest = -1;
						for (x = 0; x < (int)fpgaLogic[destX][destY][destZ].nodes.size(); x++) // find the undeleted most critical path using this node
						{
							if (!paths[fpgaLogic[destX][destY][destZ].nodes[x].path][0].deleted)
							{
								pathDest = fpgaLogic[destX][destY][destZ].nodes[x].path;
								nodeDest = fpgaLogic[destX][destY][destZ].nodes[x].node;
								break;
							}
						}
						assert(pathDest == fpgaLogic[destX][destY][destZ].owner.path && nodeDest == fpgaLogic[destX][destY][destZ].owner.node); // check that the owner is correct
						assert(pathDest > -1);
						RoFile << "\t" << "dest = (";
						(iterOutputPorts->second) += "\tdest = (";

						bool specialBRAMPortIn = false;

						if (destZ%LUTFreq != 0) // FF
						{
							if (fpgaLogic[destX][destY][destZ].FFMode != sData)
							{
								std::cout << "something is wrong with one of the registers when creating destination port in rcf file" << std::endl;
								assert(1 == 2);

							}
							else
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", D ), route_port = ";
								(iterOutputPorts->second) += "PATH" + std::to_string(pathDest) + "NODE" + std::to_string(nodeDest) + ", D ), route_port = ";
							}
						}
						// BRAM
						else if (fpgaLogic[destX][destY][destZ].isBRAM)
						{
							assert(destZ == 0);
							RoFile << "PATH" << pathDest << "NODE" << nodeDest << "_";
							(iterOutputPorts->second) += "PATH" + std::to_string(pathDest) + "NODE" + std::to_string(nodeDest) + "_";
							if (memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].operationMode == simpleDualPort) // the output is only port b
							{
								RoFile << "b_dataout[0], ";
								(iterOutputPorts->second) += "b_dataout[0], ";
							}
							else
							{
								// if it's not a double memory or if the destination port is not data in then common case
								if (fpgaLogic[destX][destY][destZ].countNumofMem == 0 || (fpgaLogic[i][j][k].connections[l].memoryDestinationPort.first != BRAMportAData))
								{
									RoFile << "a_dataout[0], ";
									(iterOutputPorts->second) += "a_dataout[0], ";
								}
								else
								{
									// double memories and destination port is data in
									// larger than first memory width
									if (fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second >= memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].portAUsedDataWidth) 
									{
										RoFile << "a_dataout[" << memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].portAUsedDataWidth << "], ";
										(iterOutputPorts->second) += "a_dataout[" + std::to_string(memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].portAUsedDataWidth) + "], ";
										specialBRAMPortIn = true;
									}
									else
									{
										RoFile << "a_dataout[0], ";
										(iterOutputPorts->second) += "a_dataout[0], ";
									}

								}
							}

							// now print the exact input port and index
							// fpgaLogic[i][j][k].connections[l].dest
							switch (fpgaLogic[i][j][k].connections[l].memoryDestinationPort.first)
							{
							case BRAMportAData:
								if (!specialBRAMPortIn)
								{
									RoFile << "PORTADATAIN[" << fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second
										<< "] );" << std::endl << std::endl;

									(iterOutputPorts->second) += "PORTADATAIN["
										+ std::to_string(fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second)
										+ "] );\n\n";
								}
								else
								{
									RoFile << "PORTADATAIN[" << fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second 
										- memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].portAUsedDataWidth
										<< "] );" << std::endl << std::endl;

									(iterOutputPorts->second) += "PORTADATAIN["
										+ std::to_string(fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second 
											- memories[fpgaLogic[destX][destY][destZ].indexMemories[0]].portAUsedDataWidth)
										+ "] );\n\n";
								}
								break;
							case BRAMportAAddress:
								RoFile << "PORTAADDR[" << fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second
									<< "] );" << std::endl << std::endl;

								(iterOutputPorts->second) +=  "PORTAADDR["
									+ std::to_string(fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second)
									+ "] );\n\n";
								break;

							case BRAMportBData:
								RoFile << "PORTBDATAIN[" << fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second
									<< "] );" << std::endl << std::endl;

								(iterOutputPorts->second) += "PORTBDATAIN["
									+ std::to_string(fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second)
									+ "] );\n\n";
								break;
							case BRAMportBAddress:
								RoFile << "PORTBADDR[" << fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second
									<< "] );" << std::endl << std::endl;

								(iterOutputPorts->second) += "PORTBADDR["
									+ std::to_string(fpgaLogic[i][j][k].connections[l].memoryDestinationPort.second)
									+ "] );\n\n";
								break;

							case BRAMportAWE:
								RoFile << "PORTAWE );" << std::endl << std::endl;
								(iterOutputPorts->second) += "PORTAWE );\n\n";
								break;
							case BRAMportBWE:
								RoFile << "PORTBWE);" << std::endl << std::endl;
								(iterOutputPorts->second) += "PORTBWE);\n\n";
								break;

							default:
								std::cout << "Wrong destaination port for memory " << std::endl;
								assert(false);
								break;
							}


						}
						// not a ff nor a BRAM then must be a LUT
						else if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 2) // 2 output ports are used then name the destination signal as if it was combout
						{
							RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
							(iterOutputPorts->second) += "PATH" + std::to_string(pathDest) + "NODE" + std::to_string(nodeDest) + ", DATAB ), route_port = ";
							
                                                        std:: stringstream LUTName;
                                                        LUTName << "LCCOMB:X" << destX << "Y" << destY << "N" << destZ;
                                                        terminal_LUTS[LUTName.str()] = true;
						}
						else if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 1)
						{
							if (fpgaLogic[destX][destY][destZ].outputPorts[Combout - 5]) // combout is th eonly output
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
								(iterOutputPorts->second) += "PATH" + std::to_string(pathDest) + "NODE" 
									+ std::to_string(nodeDest) + ", DATAB ), route_port = ";
							//	if (pathDest == 2 && nodeDest == 9)
						///			std::cout << "Debugging" << std::endl;
							}
							else // cout is the output
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << "_Cout, DATAB ), route_port = ";
								(iterOutputPorts->second) += "PATH" + std::to_string(pathDest) 
									+ "NODE" + std::to_string(nodeDest) + "_Cout, DATAB ), route_port = ";
							}
                                                        std:: stringstream LUTName;
                                                        LUTName << "LCCOMB:X" << destX << "Y" << destY << "N" << destZ;
                                                        terminal_LUTS[LUTName.str()] = true;
						}
						else
						{
							assert(false);
							std::cout << "something wrong with the output of a destination when creating rcf." << std::endl;
						}
						if (!fpgaLogic[destX][destY][destZ].isBRAM)
						{
							switch (fpgaLogic[i][j][k].connections[l].destinationPort)
							{
							case portA:
								RoFile << "DATAA ;" << std::endl << std::endl;
								(iterOutputPorts->second) += "DATAA ;\n\n";
								break;
							case portB:
								RoFile << "DATAB ;" << std::endl << std::endl;
								(iterOutputPorts->second) += "DATAB ;\n\n";
								break;
							case portC:
								RoFile << "DATAC ;" << std::endl << std::endl;
								(iterOutputPorts->second) += "DATAC ;\n\n";
								break;
							case portD:
								RoFile << "DATAD ;" << std::endl << std::endl;
								(iterOutputPorts->second) += "DATAD ;\n\n";
								break;
							default:
								if (destZ%LUTFreq == 0 || fpgaLogic[destX][destY][destZ].FFMode != sData)
								{
									std::cout << "Something wrong with destination port when creating RCF files at the final case statement of destination port" << std::endl;
								}
								else
								{
									RoFile << "ASDATA ;" << std::endl << std::endl;
								}
								break;
							}
						}
						// if this was the last connection, we need to close the routing signal with a brace
					//	if (l == (int)fpgaLogic[i][j][k].connections.size() - 1 )//&& l != 0)
					//	{
					//		RoFile << "}" << std::endl;
					//		(iterOutputPorts->second) += "}\n";
					//	}
						if (l == (int)fpgaLogic[i][j][k].connections.size() - 1){
							
                                                        addBrace = true;
                                                }
					}
                                        

					// print the connection of i, j, k to file

					for (auto it = outputPorts.begin(); it != outputPorts.end(); ++it)
					{
						completeFile += it->second;
					}
                                        if(addBrace){
                                            //At the end of a net's routing, add missing fanouts to the net
                                            std:: string resource_name;
                                            std:: ostringstream FF_name_stream;
                                            //create name of the FF (starting node) to search for it in the fanout map
                                            FF_name_stream << "FF_X" << i << "_Y" << j << "_N" << k;
                                            resource_name = FF_name_stream.str();
                                            std::ostringstream current_node_name;
                                            current_node_name << "PATH" << path << "NODE" << node;
                                            std::string current_node_name_str = current_node_name.str();
                                            //if the FF is found, add fanouts to the path
                                            if(routing_trees.find(resource_name) != routing_trees.end()){
                                                add_fanouts_to_routing(routing_trees[resource_name], branchLabel, RoFile, current_node_name_str);
                                            }
                                            //now check if the source node is a LUT
                                            std:: ostringstream LUT_name_stream;
                                            LUT_name_stream << "LCCOMB_X" << i << "_Y" << j << "_N" << k;
                                            resource_name = LUT_name_stream.str();
                                            //if the LUT is found, add fanouts to th epath (only one of this or the one above will trigger)
                                            if(routing_trees.find(resource_name) != routing_trees.end()){
                                                add_fanouts_to_routing(routing_trees[resource_name], branchLabel, RoFile, current_node_name_str);
                                            }
                                            //finish the file with a brace
                                         //   RoFile << "}" << std::endl;
											
                                            addBrace = false;
                                        }
				}

			}
		}
	}
	std::ofstream RoFileTest;
	std::string RoFileNameTest = "RCF_File_test_" + std::to_string(bitStreamNumber) + ".txt";
	std::string fileBraceRemoved = completeFile.substr(1, completeFile.size() - 1);
	fileBraceRemoved += "\n } \n";
	RoFileTest.open(RoFileNameTest);
	RoFileTest << fileBraceRemoved;
}

#endif



#ifdef StratixV
void create_auxil_file_stratix(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlConSignals, std::vector <Path_logic_component> controlFSignals, std::vector <Path_logic_component> sources);


void create_location_contraint_file() // modified for STratix V
{
	int i, j, k;
	int x;
	int total = 0;
	std::ofstream LoFile;
	LoFile.open(locationConsraintFile);
	int path = -1;
	int node = -1;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization>0) // this LE is used, in this case assign that location to the most critical path using it.
				{
					total++;
					if (fpgaLogic[i][j][k].usedOutputPorts < 1 && k % 2 != 1)
						std::cout << "error" << std::endl;

					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // find the undeleted most critical path using this node
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							//		LoFile << fpgaLogic[i][j][k].nodes[x].path << "NODE" << fpgaLogic[i][j][k].nodes[x].node << "_t" << std::endl;
							break;
						}
					}
					assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node);
					assert(path > -1);
					if (path == -1) // if no deleted path was found then continue 
						continue;

					if (k % LUTFreq == 0)
					{
#ifdef CycloneIV
						LoFile << "set_location_assignment LCCOMB_X" << i << "_Y" << j << "_N" << k << " -to PATH" << path << "NODE" << node << "_t" << std::endl;
#endif
#ifdef StratixV
						LoFile << "set_location_assignment ";
						if (fpgaLogic[i][j][k].mLAB)
							LoFile << "M";
						LoFile << "LABCELL_X" << i << "_Y" << j << "_N" << k << " -to PATH" << fpgaLogic[i][j][k].owner.path /*path*/ << "NODE" << fpgaLogic[i][j][k].owner.node /*node*/ << "_t" << std::endl;

#endif
					}
					else
					{
						LoFile << "set_location_assignment FF_X" << i << "_Y" << j << "_N" << k << " -to PATH" << path << "NODE" << node << "_t" << std::endl;
					}
#ifdef DUMMYREG
					if (fpgaLogic[i][j][k].inputPorts[portC])
					{
						assert(k % 2 == 0);
						if (fpgaLogic[i][j][k + 1].utilization > 0)
							continue;
						LoFile << "set_location_assignment FF_X" << i << "_Y" << j << "_N" << k + 1 << " -to DUMMYREG_PATH" << path << "NODE" << node << "_t" << std::endl;

					}
#endif

				}
				path = -1;
				node = -1;
			}
		}
	}

	LoFile.close();
}
/*
void LUT_WYSIWYG_CycloneIV(int i, int j, int k, std::ofstream& verilogFile, int port1, int port2, std::vector <Path_logic_component>& CoutSignals, std::vector <Path_logic_component>& controlSignals, int path, int node, int pathFeederPort1, int nodeFeederPort1, int  pathFeederPort2, int nodeFeederPort2, bool & inverting)
{
	if (fpgaLogic[i][j][k].outputPorts[Combout - 5] && !fpgaLogic[i][j][k].outputPorts[Cout - 5]) // output is only from Combout
	{

		if (fpgaLogic[i][j][k].usedInputPorts < 3) // check that maximum 2 inputs are being used
		{
			// get the two ports used
			for (int x = 0; x < InputPortSize; x++)
			{
				if (fpgaLogic[i][j][k].inputPorts[x] && port1 < 0) // 1st port not yet set
				{
					port1 = x;
				}
				else if (fpgaLogic[i][j][k].inputPorts[x]) // 1st port ws set, so set the second port
				{
					port2 = x;
				}

			}
			// at least one port must be used
			assert(port1 > -1);
			if (fpgaLogic[i][j][k].usedInputPorts == 2)
				assert(port2>-1);
			if (fpgaLogic[i][j][k].inputPorts[Cin]) // Cin is used to connect to combout
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con )," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;
				if (port1 == Cin) // port 1 is Cin
				{
					if (port2 < 0) // no port 2, set port b to gnd
					{
						verilogFile << "	.datab(gnd)," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout )," << std::endl;
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						// ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
					else // port 2 is also used 
					{
						verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout )," << std::endl;
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
						//ib0502verilogFile << "	.datad(vcc )," << std::endl;
					}
				}
				else // port 2 is  cin and port 1 must be used
				{
					assert(port2>-1);
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					verilogFile << "	.cin(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << "_Cout )," << std::endl;
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
					//ib0502verilogFile << "	.datad(vcc )," << std::endl;
				}
				verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD77D;" << std::endl;
				fpgaLogic[i][j][k].LUTMask = "D77D";
				fpgaLogic[i][j][k].CoutFixedDefaultValue = false; // probably not needed as this case has no cout as output

			}
			else // normal inputs to combout // common case
			{

				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1));
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2));

				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
																			//// instantiate the wysiwyg
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;
				verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
				if (port2>-1)
					verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
				else // only one port is used
					verilogFile << "	.datac(gnd)," << std::endl;

				verilogFile << "	.datad(PATH" << path << "NODE" << node << "F )," << std::endl;
				//ib0502verilogFile << "	.datad(vcc )," << std::endl;
				verilogFile << "	.combout(PATH" << path << "NODE" << node << "));" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"datac\";" << std::endl;
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD77D;" << std::endl;
				fpgaLogic[i][j][k].LUTMask = "D77D";
				fpgaLogic[i][j][k].CoutFixedDefaultValue = false;
			}
		}
		else
		{
			std::cout << "ERROR: A LUT which uses " << fpgaLogic[i][j][k].usedInputPorts << " is not supported. SomethingWrong.";
		}
	}
	else // output can be cout or (cout,combout)
	{
		if (fpgaLogic[i][j][k].usedInputPorts < 3) // max two ports are used
		{
			// get the two ports used
			for (x = 0; x < InputPortSize; x++)
			{
				if (fpgaLogic[i][j][k].inputPorts[x] && port1 < 0) // 1st port not yet set
				{
					port1 = x;
				}
				else if (fpgaLogic[i][j][k].inputPorts[x]) // 1st port ws set, so set the second port
				{
					port2 = x;
				}

			}
			// at least one port must be used
			assert(port1 > -1);
			if (fpgaLogic[i][j][k].usedInputPorts == 2)
				assert(port2>-1);

			if (fpgaLogic[i][j][k].inputPorts[Cin]) // port Cin is used (possible inputs (Cin), (Cin,A), (Cin,B), (Cin,D))
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1)); // get the feeder for port1
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2)); // get the feeder for port2
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				if (check_control_signal_required(i, j, k)) //ib New check control
					verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
				else
					verilogFile << "	.dataa(vcc)," << std::endl;
				if (port1 == Cin) // port 1 is Cin
				{
					if (port2 < 0) // no port 2, set port b to gnd
					{
						verilogFile << "	.datab(vcc)," << std::endl; // set b to vcc so that cin is inverted to cout
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout)," << std::endl;
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
						//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
					else // port 2 is also used 
					{
						verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << ")," << std::endl;
						verilogFile << "	.cin(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << "_Cout)," << std::endl;
						verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
						//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
					}
				}
				else // port 2 is  cin and port 1 must be used
				{
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << ")," << std::endl;
					verilogFile << "	.cin(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << "_Cout)," << std::endl;
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				// output
				if (fpgaLogic[i][j][k].outputPorts[Combout - 5]) // combout is also used
					verilogFile << "	.combout(PATH" << path << "NODE" << node << ")," << std::endl;
				verilogFile << "	.cout(PATH" << path << "NODE" << node << "_Cout));" << std::endl;
				CoutSignals.push_back(Path_logic_component(path, node));
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				// function to decide which LUT mask to use
				if (check_down_link_edge_transition(i, j, k)) // if the down link has an inverting/non-inverting edge between its inputports (other than cin) and cout, then this bock should give 1/0 from its cout when it is shut off
				{
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD77D;" << std::endl; // cout is 1 when off
					fpgaLogic[i][j][k].LUTMask = "D77D";
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false;
				}
				else
				{
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hD728;" << std::endl; // cout is zero when off
					fpgaLogic[i][j][k].LUTMask = "D728";
				}
				//	verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = \"D77D\";" << std::endl;


			}
			else // port Cin not used possible inputs affecting Cout (A), (B), (A,B), here we assume that the LUT feeding Cin is free and we will use it
			{
				assert(get_feeder(i, j, k, port1, pathFeederPort1, nodeFeederPort1)); // get the feeder for port1
				if (port2>-1) // second input port is used
					assert(get_feeder(i, j, k, port2, pathFeederPort2, nodeFeederPort2)); // get the feeder for port2
				controlSignals.push_back(Path_logic_component(path, node)); //stores all the nodes requiring control signals
				verilogFile << "cycloneive_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
				//		verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
				verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << ")," << std::endl;
				if (port2>-1) // port 2 exists
					verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << ")," << std::endl;
				//		else // port 2 does not exist tie the other inpu to vcc
				//			verilogFile << "	.cin(vcc)," << std::endl;  //verilogFile << "	.datab(vcc)," << std::endl; new
				// cin will control the output
				//	verilogFile << "	.cin(PATH" << path << "NODE" << node << "Con)," << std::endl; removed for new stuff
				//	verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
				//////////////////////// new stuff
				if (port2 > -1) // port 2 is used
				{
					if (check_control_signal_required(i, j, k)) //ib New check control
						verilogFile << "	.cin(PATH" << path << "NODE" << node << "Con)," << std::endl;
					else
						verilogFile << "	.cin(vcc)," << std::endl;
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				else // only one port non cin is used then do not use cin for control as it is not needed
				{
					if (check_control_signal_required(i, j, k)) //ib New check control
						verilogFile << "	.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
					else
						verilogFile << "	.dataa(vcc)," << std::endl;
					verilogFile << "	.datad(PATH" << path << "NODE" << node << "F)," << std::endl;
					//ib0502 verilogFile << "	.datad(vcc )," << std::endl;
				}
				//////////////////////////////// end new
				if (fpgaLogic[i][j][k].outputPorts[Combout - 5]) // combout is also used
					verilogFile << "	.combout(PATH" << path << "NODE" << node << ")," << std::endl;
				verilogFile << "	.cout(PATH" << path << "NODE" << node << "_Cout));" << std::endl;
				CoutSignals.push_back(Path_logic_component(path, node));
				verilogFile << "defparam PATH" << path << "NODE" << node << "_t .sum_lutc_input = \"cin\";" << std::endl;
				// check if the edge from operand to cout is inverting or non inverting
				inverting = true;
				for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++)
				{
					if (paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].portOut == Cout && !paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
					{
						inverting = paths[fpgaLogic[i][j][k].nodes[x].path][fpgaLogic[i][j][k].nodes[x].node].inverting;
						break;
					}
				}
				if (inverting)
				{
					// new syuff
					if (port2 > -1) // new
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h9F60;" << std::endl; // cout is zero when off
						fpgaLogic[i][j][k].LUTMask = "9F60";
					}
					else
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'hDD22;" << std::endl; // cout is zero when off (todo: check when writing controller for tester)
						fpgaLogic[i][j][k].LUTMask = "DD22";
					}
				}
				else
				{
					fpgaLogic[i][j][k].CoutFixedDefaultValue = false; // since with this mask cout is set to 1 when off to allow the down link logic to be served correctly (inverting or non inverting).
					if (port2 > -1) // new
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h6F9F;" << std::endl; // cout is one when off
						fpgaLogic[i][j][k].LUTMask = "6F9F";
					}
					else
					{
						verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 16'h77DD;" << std::endl; // cout is one when off
						fpgaLogic[i][j][k].LUTMask = "77DD";
					}
				}

			}
		}
		else
		{
			std::cout << "ERROR: A LUT (Cout) which uses " << fpgaLogic[i][j][k].usedInputPorts << " is not supported. SomethingWrong.";
		}

	}
}
*/
void ALUT_WYSIWYGS_StratixV(int i, int j, int k, std::vector <Path_logic_component> & controlConSignals, std::vector <Path_logic_component> & controlFSignals, std::ofstream& verilogFile) //Con controls the edge transition, F fixes the output 
{
	/// assuming that the only output is from combout for now 10/05/2016
	int path = -1;
	int node = -1;
	for (int x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // find the undeleted most critical path using this node
	{
		if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
		{
			path = fpgaLogic[i][j][k].nodes[x].path;
			node = fpgaLogic[i][j][k].nodes[x].node;
			//		LoFile << fpgaLogic[i][j][k].nodes[x].path << "NODE" << fpgaLogic[i][j][k].nodes[x].node << "_t" << std::endl;
			break;
		}
	}
	assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node);
	assert(path > -1);
	if (path == -1) // if no deleted path was found then continue 
		return;

	bool fixRequired = false;

	verilogFile << "stratixv_lcell_comb PATH" << path << "NODE" << node << "_t (" << std::endl;
	verilogFile << "\t.dataa(PATH" << path << "NODE" << node << "Con)," << std::endl;
	// add this signal to its vector
	controlConSignals.push_back(Path_logic_component(path, node));
/*	for (int x = 0; x < LUTinputSize; x++)
	{
		verilogFile << ".data";
		if (x == portA) // use it as edge, this is just a placeholder the actual port can be determined from .rcf files
		{
			verilogFile << "a(PATH" << path << "NODE" << node << "Con)," << std::endl;
		}
		else if (x == portB)
			verilogFile << "b";
		else if (x == portC)
			verilogFile << "c";
		else if (x == portD)
			verilogFile << "d";
		else if (x == portE)
			verilogFile << "e";
		else if (x == portF)
		{
			if (check_control_signal_required(i, j, k)) // if a a fix signal is needed add it
			{
				verilogFile << "f(PATH" << path << "NODE" << node << "F)," << std::endl;
				fixRequired = true;
			}
			else
			{
				verilogFile << "f";
				fixRequired = false;
			}
		}
		else
			std::cout << "Error: at creating wysyigys, input port does not match." << std::endl;
		if (!fpgaLogic[i][j][k].inputPorts[x]) // current port not used
		{

		}

	}*/
	int assignedInputs = 1;
	int pathFeeder, nodeFeeder;
	for (int x = 0; x < LUTinputSize; x++)
	{
		if (fpgaLogic[i][j][k].inputPorts[x]) // input port used
		{
			// find feeder man
			assert(get_feeder(i, j, k, x, pathFeeder, nodeFeeder));
			if (assignedInputs == portB)
				verilogFile << "\t.datab";
			else if (assignedInputs == portC)
				verilogFile << "\t.datac";
			else if (assignedInputs == portD)
				verilogFile << "\t.datad";
			else if (assignedInputs == portE)
				verilogFile << "\t.datae";
			else if (assignedInputs == portF) // this is true if we are instantiating a 6-input LUT
			{
				if (check_control_signal_required(i, j, k)) // this should never be true
				{
					assert(true);
					verilogFile << "\t.dataf(PATH" << path << "NODE" << node << "F)," << std::endl;
					controlFSignals.push_back(Path_logic_component(path, node));
				//	fixRequired = true;
				}
				else
				{
					verilogFile << "\t.dataf";
				//	fixRequired = false;
				}
			}
			verilogFile << "(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl;
			assignedInputs++;
		}
	}
	assert(assignedInputs > 1);

	bool flagPortCPortDShared = check_c_d_shared(i, j, k);

	if (flagPortCPortDShared)
	{
		std::cout << "Shared port C and D are shared " << std::endl;
	}

	bool similarLUTsinALM = false;
	if (flagPortCPortDShared)
	{
		similarLUTsinALM = check_similar_LUT_in_ALM(i, j, k);
	}

	int dummyCount = 0;
	// now fill the remainig ports with zero 
	for (int x = assignedInputs; x < LUTinputSize; x++)
	{
		if (x == portB)
			verilogFile << "\t.datab";
		else if (x == portC)
			verilogFile << "\t.datac";
		else if (x == portD)
			verilogFile << "\t.datad";
		else if (x == portE)
			verilogFile << "\t.datae";
		else if (x == portF)
		{
			if (check_control_signal_required(i, j, k)) // if a a fix signal is needed, add it
			{
				//assert(true);
				verilogFile << "\t.dataf(PATH" << path << "NODE" << node << "F)," << std::endl;
				controlFSignals.push_back(Path_logic_component(path, node));
				continue;
			//	fixRequired = true;
			}
			else
			{
				verilogFile << "\t.dataf";
			//	fixRequired = false;
				if (flagPortCPortDShared)
				{
					if (similarLUTsinALM) // both LUTS do not require a control signal to fix the output, so just add a gnd dummy signal to port F
					{
						verilogFile << "(dummyIn" << dummyCount + 1 << "_gnd)," << std::endl;
						dummyCount++;
						continue;
					}
					else // this LUT does not require a control signal to fix the output, but the other LUT in the alm does, so we will have to change the LUT mask of this LUT to match the other one. This emans that we have to set port F to 1 so that it does not screw-up the equation.
					{
						verilogFile << "(dummyIn1_vdd)," << std::endl;
						continue;
					}
				}
			}
		}
		if (flagPortCPortDShared)
		{
			verilogFile << "(dummyIn" << dummyCount + 1 << "_gnd)," << std::endl;
			dummyCount++;
		}
		else
		{
			verilogFile << "(1'b0)," << std::endl;
		}
		
	}

	verilogFile << "\t.combout(PATH" << path << "NODE" << node << " ));" << std::endl;
	verilogFile << "defparam PATH" << path << "NODE" << node << "_t .shared_arith = \"off\";" << std::endl;
	verilogFile << "defparam PATH" << path << "NODE" << node << "_t .extended_lut = \"off\";" << std::endl;
	verilogFile << "defparam PATH" << path << "NODE" << node << "_t .lut_mask = 64'h";

	if (check_control_signal_required(i, j, k))
	{
		verilogFile << "9669699600000000;" << std::endl << std::endl; // F = (A xor B xor C .. xor E)& F
	}
	else
	{
		if (flagPortCPortDShared)
		{
			if (!similarLUTsinALM) // LUT masks in the same ALM not similar so we will change the one with no fix signal to be the same as the on with the fix signal and we already set the fix to one, making sure the output is never fixed
			{
				verilogFile << "9669699600000000;" << std::endl << std::endl; // F = (A xor B xor C .. xor E)& F
			}
			else
			{
				verilogFile << "6996966996696996;" << std::endl << std::endl; // F = (A xor B xor C ..xor E xor F

			}
		}
		else
		{
			verilogFile << "6996966996696996;" << std::endl << std::endl; // F = (A xor B xor C ..xor E xor F
		}
		
	}

	//verilogFile << "defparam PATH" << path << "NODE" << node << "_t .shared_arith = \"off\";" << std::endl;
}




void create_WYSIWYGs_file() // also calls create_auxill and create_controller
{
	int i, j, k;
	int x;
	int total = 0;
	bool deleted = true;
	std::ofstream verilogFile;
	verilogFile.open("VerilogFile.txt");
	int path = -1;
	int node = -1;
	std::vector <Path_logic_component> sources; // stores source flipflops
	std::vector <Path_logic_component> sinks; // stores the output signals of the tested paths;
	std::vector <Path_logic_component> controlSignals; // stores the control signals of the tested paths;
	std::vector <Path_logic_component> controlConSignals; // stores the control signal that selects the esge transistion the output of the ALUT for STratix;
	std::vector <Path_logic_component> controlFSignals; // stores the control signal that fixes the output of the ALUT for STratix;
	std::vector <Path_logic_component> CoutSignals;
	std::vector <Path_logic_component> DummyRegSignals;
	DummyRegSignals.resize(0);
	int pathFeeder, nodeFeeder;
	pathFeeder = -1;
	nodeFeeder = -1;
	int pathFeederPort1, pathFeederPort2, nodeFeederPort1, nodeFeederPort2;
	int port1, port2;
	bool inverting = false;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				inverting = false;
				port1 = -1;
				port2 = -1;
				pathFeederPort1 = -1;
				pathFeederPort2 = -1;
				nodeFeederPort1 = -1;
				nodeFeederPort2 = -2;
				path = -1; // new ibrahim 17/02/2016
				node = -1;
				for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // make sure that this node is not deleted and gets the name of the wysiwyg, or we can check utilization
				{
					if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
					{
						path = fpgaLogic[i][j][k].nodes[x].path;
						node = fpgaLogic[i][j][k].nodes[x].node;
						break;
					}
				}

				if (path == -1 || node == -1) // all paths using this node are deleted so do not instantiate a wysiwygs for this node
					continue;
				assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node);

				if (k % LUTFreq == 0) // LUT
				{
#ifdef CycloneIV
					// location X1, put the code into a functoin and calling it below. not tested may have problems
					LUT_WYSIWYG_CycloneIV(i, j, k, verilogFile, port1, port2, CoutSignals, controlSignals, path, node, pathFeederPort1, nodeFeederPort1, pathFeederPort2, nodeFeederPort2, inverting);
#endif
#ifdef StratixV
					ALUT_WYSIWYGS_StratixV(i, j, k, controlConSignals, controlFSignals, verilogFile);
#endif


				}
				else // FLIPFLOP todo : have to deal with cascaded paths maaan
				{
					verilogFile << "dffeas PATH" << path << "NODE" << node << "_t (" << std::endl;
					verilogFile << "	.clk(CLK)," << std::endl;
					if (node == 0) // this is a source register, assuming no cascaded paths, no register is a source and a sink
					{
						verilogFile << "	.d(Xin[" << sources.size() << "])," << std::endl; // assuming that all sources will share the same input
						verilogFile << "	.q(PATH" << path << "NODE" << node << "));" << std::endl;
						sources.push_back(Path_logic_component(path, node));
					}
					else // this is a sink register
					{
						assert(get_feeder(i, j, k, pathFeeder, nodeFeeder));
						verilogFile << "	.d(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl; // assuming that all sources will share the same input
						verilogFile << "	.q(PATH" << path << "NODE" << node << "));" << std::endl;
						sinks.push_back(Path_logic_component(path, node));

					}
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .power_up = \"low\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .is_wysiwyg = \"true\";" << std::endl <<std::endl;
				}
				path = -1;
				node = -1;
			}
		}
	}
#ifdef DUMMYREG
	verilogFile << "//adding dummy regs man hope it works fuck this shit." << std::endl;

	///// loop across fpga to add dummy register after any LUT using port c as input

	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization>0 && fpgaLogic[i][j][k].inputPorts[portC]) //  if a cell is utilized and uses port c as input then check if cascaded register is used or not
				{
					if (fpgaLogic[i][j][k + 1].utilization>0) // if the register is used then skip this and go on
						continue;

					path = -1;
					node = -1;
					for (x = 0; x < fpgaLogic[i][j][k].nodes.size(); x++) // make sure that this node is not deleted and gets the name of the wysiwyg, or we can check utilization
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							break;
						}
					}
					assert(path > -1 && node > -1);
					verilogFile << "dffeas DUMMYREG_PATH" << path << "NODE" << node << "_t (" << std::endl;
					verilogFile << "	.clk(CLK)," << std::endl;
					verilogFile << "	.d(PATH" << path << "NODE" << node << ")," << std::endl; // assuming that all sources will share the same input
					verilogFile << "	.q(DUMMYREG_PATH" << path << "NODE" << node << "));" << std::endl;
					DummyRegSignals.push_back(Path_logic_component(path, node));
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .power_up = \"low\";" << std::endl;
					verilogFile << "defparam PATH" << path << "NODE" << node << "_t .is_wysiwyg = \"true\";" << std::endl;
				}

			}
		}
	}
	if (DummyRegSignals.size() > 0)
		verilogFile << "assign dummyOut = DUMMYREG_PATH" << DummyRegSignals[0].path << "NODE" << DummyRegSignals[0].node;
	for (x = 1; x < DummyRegSignals.size(); x++)
	{
		verilogFile << "| DUMMYREG_PATH" << DummyRegSignals[x].path << "NODE" << DummyRegSignals[x].node;
	}
	verilogFile << ";" << std::endl;
#endif
	verilogFile << "endmodule" << std::endl;
	verilogFile.close();
#ifdef CyclonIV
//	create_auxil_file(sinks, controlSignals, CoutSignals, sources);
#endif
#ifdef StratixV
	create_auxil_file_stratix(sinks, controlConSignals, controlFSignals, sources);
#endif
//	create_controller_module(sinks, controlSignals, sources);


}

void create_auxil_file_stratix(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlConSignals, std::vector <Path_logic_component> controlFSignals, std::vector <Path_logic_component> sources)
{
	std::ofstream verilogFile;
	verilogFile.open("AuxilFile.txt");
	verilogFile << "module top (input CLK, output fuck, ";
	verilogFile << "input [" << sources.size() - 1 << ":" << "0] Xin";
	int i;
//	if (controlConSignals.size() > 0 || controlFSignals.size() > 0)
//		verilogFile << ", ";
	// control signals
	for ( i = 0; i < (int)controlConSignals.size(); i++)
	{
		verilogFile << ", input PATH" << controlConSignals[i].path << "NODE" << controlConSignals[i].node << "Con ";
	}

	for ( i = 0; i < (int)controlFSignals.size(); i++)
	{
		verilogFile << ", input PATH" << controlFSignals[i].path << "NODE" << controlFSignals[i].node << "F ";
	}
	verilogFile << " );" << std::endl << std::endl;
#ifdef DUMMYREG
	verilogFile << ", dummyOut";
#endif

	// intermediate signals
	verilogFile << "wire ";
	for ( i = 0; i < (int) controlConSignals.size()-1; i++)
	{
		verilogFile << " PATH" << controlConSignals[i].path << "NODE" << controlConSignals[i].node << " , ";
	}

	for (int j = 0; j < (int)sources.size(); j++)
	{
		verilogFile << " PATH" << sources[j].path << "NODE" << sources[j].node << " , ";
		assert(sources[j].node == 0);
	}

	for (int j = 0; j < (int)sinks.size(); j++)
	{
		verilogFile << " PATH" << sinks[j].path << "NODE" << sinks[j].node << " , ";
		
	}

	verilogFile << " PATH" << controlConSignals[i].path << "NODE" << controlConSignals[i].node << " ; " << std::endl;

	// group sink singnals
	assert(sinks.size() > 0);
	verilogFile << "assign fuck = PATH" << sinks[0].path << "NODE" << sinks[0].node;
	for ( i = 1; i < (int)sinks.size(); i++)
	{
		verilogFile << " & PATH" << sinks[i].path << "NODE" << sinks[i].node;
	}

	verilogFile << ";" << std::endl << std::endl;

	for (i = 0; i < 5; i++)
	{
		verilogFile << " wire dummyIn" << i + 1 << "_gnd /* synthesis keep*/;" << std::endl;
	}

	for (i = 0; i < 5; i++)
	{
		verilogFile << " wire dummyIn" << i + 1 << "_vdd /* synthesis keep*/;" << std::endl;
	}

	for (i = 0; i < 5; i++)
	{
		verilogFile << " assign dummyIn" << i + 1 << "_vdd = 1'b1;" << std::endl;
	}

	for (i = 0; i < 5; i++)
	{
		verilogFile << " assign dummyIn" << i + 1 << "_gnd = 1'b0;" << std::endl;
	}


}



void create_RCF_file()
{
	int i, j, k, l;
	int x;
	int total = 0;
	int destX, destY, destZ;
	std::ofstream RoFile;
	RoFile.open("RCF_File.txt");
	int path = -1;
	int node = -1;
	int pathDest = -1;
	int nodeDest = -1;
	int label;
	std::map<std::string, int> branchLabel;
	bool foundSource = false;
	for (i = 0; i < FPGAsizeX; i++)
	{
		for (j = 0; j < FPGAsizeY; j++)
		{
			for (k = 0; k < FPGAsizeZ; k++)
			{
				if (fpgaLogic[i][j][k].utilization>0) // this cell is used, so lets check its fanouts
				{
					path = -1;
					node = -1;
					for (x = 0; x < (int)fpgaLogic[i][j][k].nodes.size(); x++) // find the undeleted most critical path using this node
					{
						if (!paths[fpgaLogic[i][j][k].nodes[x].path][0].deleted)
						{
							path = fpgaLogic[i][j][k].nodes[x].path;
							node = fpgaLogic[i][j][k].nodes[x].node;
							//		LoFile << fpgaLogic[i][j][k].nodes[x].path << "NODE" << fpgaLogic[i][j][k].nodes[x].node << "_t" << std::endl;
							break;
						}
					}
#ifdef StratixV
					assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node); // check that the owner is correct
#endif
					//there must be path, otherwise utilization should have been 0
					assert(path > -1);
					// clear branch map
					branchLabel.clear();
					label = 0;
					// write source signal
					if (path == 1152 && node == 0)
						std::cout << "debug";
					for (l = 0; l < (int)fpgaLogic[i][j][k].connections.size(); l++) // loop across all routes
					{
//.deletedConn						if (fpgaLogic[i][j][k].connections[l].deleted) // if the connection is deleted then go on, dont add constraints to it.
//							continue;
// trial ibrahim 23/05/2016
						if (fpgaLogic[i][j][k].connections[l].destinationX == -1)
							continue;

						if (l == 0)
							RoFile << "signal_name = PATH" << path << "NODE" << node << " {" << std::endl;
						foundSource = false;
						// start writing rcf for this connection
						//	RoFile << "signal_name = PATH" << path << "NODE" << node << " {" << std::endl;
						for (x = 0; x < (int)fpgaLogic[i][j][k].connections[l].usedRoutingResources.size(); x++)
						{
							/* // does not handle branch out will use the same resource twice for the same signal (functionally correct but quartud generates an error)
							RoFile << "\t" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
							*/
							// check if the routing resource has been used before
							auto iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x]);
							if (foundSource)
							{
								assert(iter == branchLabel.end());
								// write the connection to the rcf
								RoFile << "\tlabel = label_" << label << "_" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
								// insert the label
								branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
								label++;
								continue;

							}
							if (iter == branchLabel.end()) // if not found then 
							{
								if (x == 0) // the first routing resource was not used before, so the source is not a label
								{
									// write the connection to the rcf
									RoFile << "\tlabel = label_" << label << "_" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
									// insert the label
									branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
									label++;
									foundSource = true;
								}
								else // must use branch_point 
								{
									iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x - 1]);
									// element must be found
									assert(iter != branchLabel.end());
									// start from the routing resource before the current (that was definetly used before)
									RoFile << "\tbranch_point = label_" << iter->second << "_" << iter->first << "; " << std::endl;
									RoFile << "\tlabel = label_" << label << "_" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
									branchLabel.insert(std::pair<std::string, int>(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x], label));
									label++;
									foundSource = true;


								}

							}
							//		else
							//	{
							//	std::cout << "Something wrong woith the source connection of the route" << std::endl;
							//	}


						}
						if (!foundSource) // this means the connection is useing the exact sam resources used before
						{
							auto iter = branchLabel.find(fpgaLogic[i][j][k].connections[l].usedRoutingResources[x - 1]);
							// element must be found
							assert(iter != branchLabel.end());
							// start from the routing resource before the current (that was definetly used before)
							RoFile << "\tbranch_point = label_" << iter->second << "_" << iter->first << "; " << std::endl;
						}

						// write last line dest = {...}, ....; but first we have to find who owns the destiantion LUT
						destX = fpgaLogic[i][j][k].connections[l].destinationX;
						destY = fpgaLogic[i][j][k].connections[l].destinationY;
						destZ = fpgaLogic[i][j][k].connections[l].destinationZ;
						pathDest = -1;
						nodeDest = -1;
						for (x = 0; x < (int)fpgaLogic[destX][destY][destZ].nodes.size(); x++) // find the undeleted most critical path using this node
						{
							if (!paths[fpgaLogic[destX][destY][destZ].nodes[x].path][0].deleted)
							{
								pathDest = fpgaLogic[destX][destY][destZ].nodes[x].path;
								nodeDest = fpgaLogic[destX][destY][destZ].nodes[x].node;
								break;
							}
						}
#ifdef StratixV
						assert(pathDest == fpgaLogic[destX][destY][destZ].owner.path && nodeDest == fpgaLogic[destX][destY][destZ].owner.node); // check that the owner is correct
#endif
						assert(pathDest > -1);
						RoFile << "\t" << "dest = (";
						if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 2) // 2 output ports are used then name the destination signal as if it was combout
						{
							RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
						}
						else if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 1)
						{
							if (destZ%LUTFreq != 0) // a reg
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", D);" << std::endl << "}";
								continue;
							}
#ifdef CycloneIV
							if (fpgaLogic[destX][destY][destZ].outputPorts[Combout - 5]) // combout is th eonly output
							{
#endif
#ifdef StratixV
								if (fpgaLogic[destX][destY][destZ].outputPorts[Combout]) // combout is th eonly output
								{
#endif
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
							}
							else // cout is the output
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << "_Cout, DATAB ), route_port = ";
							}
						}
						else
						{
							assert(true);
							std::cout << "something wron with output od a destination when creating rcf." << std::endl;
						}
						switch (fpgaLogic[i][j][k].connections[l].destinationPort)
						{
						case portA:
							RoFile << "DATAA ;" << std::endl << std::endl;
							break;
						case portB:
							RoFile << "DATAB ;" << std::endl << std::endl;
							break;
						case portC:
							RoFile << "DATAC ;" << std::endl << std::endl;
							break;
						case portD:
							RoFile << "DATAD ;" << std::endl << std::endl;
							break;
#ifdef StratixV
						case portE:
							RoFile << "DATAE ;" << std::endl << std::endl;
							break;
						case portF:
							RoFile << "DATAF ;" << std::endl << std::endl;
							break;
#endif
						default:
							std::cout << "Something wron with destination port when creatubg RCF files" << std::endl;
							break;
						}

						if (l == fpgaLogic[i][j][k].connections.size() - 1)
							RoFile << "}" << std::endl;
					}

				}

			}
		}
	}

}



bool check_c_d_shared(int i, int j, int k) //checks if LUT i,j,k shares port C or D with the other LUT in port D or C
{
	assert(k%LUTFreq == 0);
	//assert()
	if (fpgaLogic[i][j][k].utilization < 1) // not utilized
		return false;
	
	int sharedPorts = 0;
	int shitMan = 0;

	for (int index = 0; index < LUTinputSize; index++)
	{
		if (fpgaLogic[i][j][k].isInputPortShared[index]) // this input is shared
		{
			sharedPorts++;
			if (fpgaLogic[i][j][k].sharedWith[index] != index)
			{
				assert(((index == portC) && (fpgaLogic[i][j][k].sharedWith[index] == portD)) || ((index == portD) && (fpgaLogic[i][j][k].sharedWith[index] == portC))); // checks that the shared stuff are port C and D or D and C
				shitMan++;
			}

		}
	}

	return shitMan > 0;
}


bool check_similar_LUT_in_ALM(int i, int j, int k) // returns true if the alm that contains the i,j,k LUT
{
	int kALM = k / ALUTtoALM; // get the ALM index

	int topK = kALM * ALUTtoALM; // top ALUT
	int bottomK = topK + LUTFreq; // bottom ALUT

	assert((fpgaLogic[i][j][topK].utilization > 0 && fpgaLogic[i][j][bottomK].utilization > 0));

	if (!(fpgaLogic[i][j][topK].utilization > 0 && fpgaLogic[i][j][bottomK].utilization > 0))
		return false;

	return !(check_control_signal_required(i, j, topK) ^ check_control_signal_required(i, j, bottomK)); 
}
#endif