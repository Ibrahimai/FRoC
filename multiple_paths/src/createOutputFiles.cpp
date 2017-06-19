#include "createOutputFiles.h"
#include "globalVar.h"
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


void create_auxil_file(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> CoutSignals, std::vector <Path_logic_component> sources, std::vector <Path_logic_component> cascadedControlSignals, std::ifstream& verilogFileSecondPart, int bitStreamNumber) // to be merged with the WYSIWYGs file to complete the source file.
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
	verilogFile << "// cascaded LUTs estra wires required for controlling the testing of cascaded paths" << std::endl;
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



	//// input signals to sources
	verilogFile << std::endl << "//input signal to sources " << std::endl;
	if (sources.size()>0)
		verilogFile << "reg [" << sources.size() - 1 << ":0] Xin;" << std::endl;;

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
		verilogFile << "always @ (posedge CLK or posedge reset) begin" << std::endl;
		verilogFile << "\tif (reset) " << std::endl;
		verilogFile << "\t\tXin <= " << sources.size() << "'b0;" << std::endl;
		verilogFile << "\telse " << std::endl;
		verilogFile << "\t\tXin <= ~Xin | set_source_registers;" << std::endl;
		verilogFile << "end" << std::endl;
	}

	/// counter to be connected to controller
	verilogFile << std::endl << "//counter to count how long to stay at each test phase " << std::endl;
	verilogFile << "counter_testing count0 (.CLK(CLK),.clr(reset_counter), .timerReached(timerReached));";


	// cascaded stuff
	verilogFile << std::endl << "//cascaded MUXES and tFFs " << std::endl;
	for (i = 0; i < (int)cascadedControlSignals.size(); i++) // for each cascaded control signal we instantiate a Tff and a 2to1 mux
	{
		verilogFile << "t_ff " << "t_" << i << " (.reset(reset), .out( source_path" << cascadedControlSignals[i].path << "_node" << cascadedControlSignals[i].node << "), .clk(CLK));" << std::endl;
		verilogFile << "mux2to1 mux_" << i << "(.in0( source_path" << cascadedControlSignals[i].path << "_node" << cascadedControlSignals[i].node << "), .in1( PATH" << cascadedControlSignals[i].path << "NODE" << cascadedControlSignals[i].node << "F ), .s( cascaded_selector_path" << cascadedControlSignals[i].path << "_node" << cascadedControlSignals[i].node << " ), .out (PATH" << cascadedControlSignals[i].path << "NODE" << cascadedControlSignals[i].node << "F_cascaded)); " << std::endl;

	}
	verilogFile << std::endl << std::endl;





	verilogFile << std::endl << "//create the controller " << std::endl;
	verilogFile << "controller control0(.CLK(CLK),.start_test(start_test),.reset(reset),.error(error),.timer_reached(timerReached),.reset_counter(reset_counter),.controlSignals({";
	for (i = 0; i < (int)controlSignals.size() - 1; i++)
	{
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
		verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F, ";
	}
	verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
	verilogFile << " PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F}), ";
	verilogFile << ".sinks({";
	// paths should be like this as sinks[0] should be sink(0)
	for (i = ((int)sinks.size()) - 1; i > 0; i--)
	{
		verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << ", ";
	}
	verilogFile << " PATH" << sinks[i].path << "NODE" << sinks[i].node << "}), ";



	// cascaded LUTs mux selector
	if (cascadedControlSignals.size() > 0)
	{
		verilogFile << ".cascaded_select({ cascaded_selector_path" << cascadedControlSignals[0].path << "_node" << cascadedControlSignals[0].node;
	}

	for (i = 1; i < (int)cascadedControlSignals.size(); i++)
	{
		cascPath = cascadedControlSignals[i].path;
		cascNode = cascadedControlSignals[i].node;
		verilogFile << ", cascaded_selector_path" << cascPath << "_node" << cascNode;
	}
	if (cascadedControlSignals.size() > 0)
		verilogFile << "}), ";

	verilogFile << ".finished_one_iteration(fuck)";
	if (sources.size() > 0)
		verilogFile << ",.set_source_registers(set_source_registers));" << std::endl;
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



void create_controller_module(std::vector <Path_logic_component> sinks, std::vector <Path_logic_component> controlSignals, std::vector <Path_logic_component> sources, std::vector <Path_logic_component> cascadedControlSignals, int bitStreamNumber)
{

	std::ofstream controllerFile;
	std::string controllerFileName = "controller_" + std::to_string(bitStreamNumber) + ".v";
	controllerFile.open(controllerFileName);
	// number of phases stored in numberOfStates.
	// controller module will output all control signals and ((reset signals for all sources to paths)), takes input start tests and all sinks from the tested paths, outputs when it has completed one loop over all paths and error signal, input also when counter is full and outputs reset to the counter 
	controllerFile << "module controller ( input CLK, input start_test, input reset, output reg error, input timer_reached, output reg reset_counter, ";

	int i, j, k, l, path_t, node_t, portInToCombout, portIn_t;
	int testPhaseCount = 0;
	bool inverting_t;
	bool currentlyTested;
	std::vector <std::vector<int>> errorSignalDivision; // store error signals of each test phase to be used as input to the or tree network
	errorSignalDivision.resize(numberOfTestPhases);
	/// group paths in the same test phase together
	std::vector <std::vector<int> > test_structure;
	test_structure.resize(numberOfTestPhases);

	for (i = 1; i < (int)paths.size(); i++)
	{
		if (!paths[i][0].deleted)
			test_structure[paths[i][0].testPhase].push_back(i);
		else
		{
			assert(paths[i][0].testPhase == -1);
		}
	}


	// control signals
	// option 1
	controllerFile << "output [" << 2 * controlSignals.size() - 1 << ":0] controlSignals,";
	// option 2
	/*	for (i = 0; i < controlSignals.size(); i++)
	{
	controllerFile << "output PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "Con, ";
	controllerFile << "output PATH" << controlSignals[i].path << "NODE" << controlSignals[i].node << "F, ";
	}
	// finish option 2*/
	// option 1
	controllerFile << "input [" << sinks.size() - 1 << ":0] sinks,";
	//option 2
	/*	// sink signals from tested paths
	for (i = 0; i < sinks.size(); i++)
	{
	controllerFile << "input PATH" << sinks[i].path << "NODE" << sinks[i].node << ", ";

	}
	// option 2 finish */
	// set signal for source registers 
	if (sources.size()>0) // when testing only cascaded path we dont have any sources.
		controllerFile << "output reg [" << sources.size() - 1 << ":0] set_source_registers, ";

	// cascaded LUTs mux selector cascaded_select
	if (cascadedControlSignals.size() > 0)
		controllerFile << "output [" << cascadedControlSignals.size() - 1 << ":0] cascaded_select, ";


	controllerFile << "output reg finished_one_iteration );" << std::endl;

	// create reg to hold the buffered values of the sinks, these values will be xored with the values from the sinks to ensure that the value changes every cycle
	controllerFile << "reg [" << sinks.size() - 1 << ":0] sinks_buff , sinks_buff_buff ;" << std::endl;
	controllerFile << "reg [" << 2 * controlSignals.size() - 1 << ":0] controlSignals_temp;" << std::endl;
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
	controllerFile << "\t\terrorVec <= ~(sinks_buff^sinks_buff_buff) ; " << std::endl; //  
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

	controllerFile << std::endl << "assign controlSignals = controlSignals_temp;" << std::endl;

	if (cascadedControlSignals.size() > 0)
		controllerFile << std::endl << "assign cascaded_select = cascaded_select_temp; " << std::endl << std::endl;

	controllerFile << "//////////////////////////////////////////////////////////////////" << std::endl;
	controllerFile << "// starting the Sequential part of the FSM changing between states" << std::endl;
	controllerFile << "//////////////////////////////////////////////////////////////////" << std::endl;

	///////////////////////////// 
	//sequential part of the FSM, state transitions
	////////////////////////////
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
		controllerFile << "\t\t\t\tif (timer_reached)" << std::endl;
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
	controllerFile << "always @ (*) begin" << std::endl;
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

			//////// cascaded LUTs selector

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


			///// output set source registers
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
				// error signal todo: zabat el error signal option1) connect them to an or network
				//	controllerFile << "\t\t\terror_temp <= 1'b0";
				// new
				controllerFile << "\t\t\terror_temp <= errorTestPhase[" << testPhaseCount << "];" << std::endl;
				errorSignalDivision[testPhaseCount].resize(0);
				// loop across sinks and see which sinks are used at the current test phase
				for (j = 0; j <(int)sinks.size(); j++)
				{
					x = paths[sinks[j].path][sinks[j].node].x;
					y = paths[sinks[j].path][sinks[j].node].y;
					z = paths[sinks[j].path][sinks[j].node].z;
					currentlyTested = false;
					// loop across all nodes using this LUT and check if any of them is tested in the current test phase
					for (k = 0; k < (int)fpgaLogic[x][y][z].nodes.size(); k++)
					{
						if (paths[fpgaLogic[x][y][z].nodes[k].path][0].testPhase == i)
						{
							if (fpgaLogic[x][y][z].nodes[k].node == 0) // if this node is a source then we shouldnt be checking the output of this FF for erro. THis only happens in cascaded cases.
								continue;
							assert(!paths[fpgaLogic[x][y][z].nodes[k].path][0].deleted);
							currentlyTested = true;
							break;
						}
					}

					if (currentlyTested)
					{
						errorSignalDivision[testPhaseCount].push_back(j);
						//controllerFile << "| errorVec[" << j << "] ";
					}

				}
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
		for (j = 0; j <(int)errorSignalDivision[i].size() - 1; j++)
			controllerFile << "errorVec[" << errorSignalDivision[i][j] << "],";
		controllerFile << "errorVec[" << errorSignalDivision[i][j] << "]}));" << std::endl;;
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

void create_WYSIWYGs_file(int bitStreamNumber) // also calls create_auxill and create_controller
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
					// location X1, put the code into a functoin and calling it below. 
					LUT_WYSIWYG_CycloneIV(cascadedControlSignals,i, j, k, verilogFile, port1, port2, CoutSignals, controlSignals, path, node, pathFeederPort1, nodeFeederPort1, pathFeederPort2, nodeFeederPort2, inverting);// , cascadedControlSignals);
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
					else // this is a sink regist+
						
					{
						assert(get_feeder(i, j, k, pathFeeder, nodeFeeder));
						if (fpgaLogic[i][j][k].FFMode == sData) // input is connected using asdata
						{
							verilogFile << "	.asdata(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl; // connected to the sdata port
							verilogFile << "	.sload(1'b1)," << std::endl; // sload is high
						}
						else
						{
							verilogFile << "	.d(PATH" << pathFeeder << "NODE" << nodeFeeder << ")," << std::endl; // assuming that all sources will share the same input

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
	create_auxil_file(sinks, controlSignals, CoutSignals, sources, cascadedControlSignals, verilogFileSecondPart, bitStreamNumber);
	create_controller_module(sinks, controlSignals, sources, cascadedControlSignals, bitStreamNumber);


}




// this may have problems I removed it from location X1;
void LUT_WYSIWYG_CycloneIV(std::vector <Path_logic_component>& cascadedControlSignals,int i, int j, int k, std::ofstream& verilogFile, int port1, int port2, std::vector <Path_logic_component>& CoutSignals, std::vector <Path_logic_component>& controlSignals, int path, int node, int pathFeederPort1, int nodeFeederPort1, int  pathFeederPort2, int nodeFeederPort2, bool & inverting)//, std::vector <Path_logic_component>& cascadedControlSignals)
{
	int pathFeederPort3, nodeFeederPort3;
	int port3 = -1;
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
						verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
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
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
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
				verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
				if (port2>-1)
					verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
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
							verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
							verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
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
						verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
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
						verilogFile << "	.dataa(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
						verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
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

					verilogFile << "	.dataa(PATH" << pathFeederPort3 << "NODE" << nodeFeederPort3 << " )," << std::endl;
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << " )," << std::endl;
					verilogFile << "	.datac(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << " )," << std::endl;
					
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
						verilogFile << "	.datab(PATH" << pathFeederPort2 << "NODE" << nodeFeederPort2 << ")," << std::endl;
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
					verilogFile << "	.datab(PATH" << pathFeederPort1 << "NODE" << nodeFeederPort1 << ")," << std::endl;
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




void create_RCF_file(int bitStreamNumber)
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
        
	for (i = 0; i < FPGAsizeX; i++) // loop across all LUTs
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
					assert(path == fpgaLogic[i][j][k].owner.path && node == fpgaLogic[i][j][k].owner.node); // check that the owner is correct
					//there must be path, otherwise utilization should have been 0
					assert(path > -1);
					// clear branch map
					branchLabel.clear();
					label = 0;
					bool first_route_found = false;
					// write source signal
					for (l = 0; l < (int)fpgaLogic[i][j][k].connections.size(); l++) // loop across all routes in the cell i,j,k
					{
						if (fpgaLogic[i][j][k].connections[l].usedRoutingResources.size() == 0) // so there is a connection but without any used resources, this must be a connection betwee a LUT and a FF that are in the same LE. So I am gonna check that
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
							if (l == (int)fpgaLogic[i][j][k].connections.size() - 1 && l!=0)
								RoFile << "}" << std::endl;

							continue;
						}
						if (fpgaLogic[i][j][k].connections[l].destinationX == -1)
						{ 
							if (l == (int)fpgaLogic[i][j][k].connections.size() - 1 && first_route_found) 
								RoFile << "}" << std::endl;
							continue;
						}
						
						if (!first_route_found)//if (l == 0)
						{
							RoFile << "signal_name = PATH" << path << "NODE" << node << " {" << std::endl;
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
								RoFile << "\tlabel = label_" << label << "_" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
								// insert the label
                                                                
                                                                
 //*****                                                        //either mark as used, or look for label
                                                                
                                                                
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
									RoFile << "\tlabel = label_" << label << "_" << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ", " << fpgaLogic[i][j][k].connections[l].usedRoutingResources[x] << ";" << std::endl;
									
                                                                        //either mark as used or look for label
                                                                        
                                                                        
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
						assert(pathDest == fpgaLogic[destX][destY][destZ].owner.path && nodeDest == fpgaLogic[destX][destY][destZ].owner.node); // check that the owner is correct
						assert(pathDest > -1);
						RoFile << "\t" << "dest = (";
						if (destZ%LUTFreq != 0) // FF
						{
							if (fpgaLogic[destX][destY][destZ].FFMode != sData)
							{
								std::cout << "something is wrog with one of the registers when creating destiantion port in rcf file" << std::endl;
								assert(1 == 2);

							}
							else
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", D ), route_port = ";
							}
						}
						else if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 2) // 2 output ports are used then name the destination signal as if it was combout
						{
							RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
						}
						else if (fpgaLogic[destX][destY][destZ].usedOutputPorts == 1)
						{
							if (fpgaLogic[destX][destY][destZ].outputPorts[Combout - 5]) // combout is th eonly output
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << ", DATAB ), route_port = ";
							//	if (pathDest == 2 && nodeDest == 9)
						///			std::cout << "Debugging" << std::endl;
							}
							else // cout is the output
							{
								RoFile << "PATH" << pathDest << "NODE" << nodeDest << "_Cout, DATAB ), route_port = ";
							}
						}
						else
						{
							assert(true);
							std::cout << "something wrong with the output of a destination when creating rcf." << std::endl;
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
						default:
							if (destZ%LUTFreq==0 || fpgaLogic[destX][destY][destZ].FFMode != sData)
							{
								std::cout << "Something wrong with destination port when creating RCF files at the final case statement of destination port" << std::endl;
							}
							else
							{
								RoFile << "ASDATA ;" << std::endl << std::endl;
							}
							break;
						}

						if (l == (int)fpgaLogic[i][j][k].connections.size() - 1){
							RoFile << std::endl;
                                                        addBrace = true;
                                                }
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
                                            RoFile << "}" << std::endl;
                                            addBrace = false;
                                        }
				}

			}
		}
	}
        
        

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