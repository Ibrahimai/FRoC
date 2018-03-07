
#define checksetr 

#ifdef checksetr
#include "globalVar.h"
#include "ILPSolver.h"
#include "util.h"
#include "gurobi_c++.h"
#include <unordered_map>

/*

void ILP_solve()
{
	try {
		GRBEnv env = GRBEnv();

		GRBModel model = GRBModel(env);

		model.getEnv().set("TimeLimit", "500.0");

		///////////////////////////////////////////////////////////////////////////
		// create variables for the ILP////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		int num_of_paths = paths.size() - 1;
		double* lb_vars = new double[num_of_paths];
		double* up_vars = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars = new char[num_of_paths];
		string* names_vars = new string[num_of_paths];


		for (int i = 0; i < num_of_paths; i++)
		{
			lb_vars[i] = 0.0;
			up_vars[i] = 1.0;
			type_vars[i] = GRB_BINARY;
			string temp = "P" + std::to_string(i + 1);
			names_vars[i] = temp;
			std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars = model.addVars(lb_vars, up_vars, NULL, type_vars, names_vars, num_of_paths);

		//////////////////////////////////////////////////////////////////////
		///////////// set deleted path to 0//////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		for (int i = 1; i < paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars[i - 1], GRB_EQUAL, 0.0);

			}

		}

		
		//////////////////////////////////////////////////////////////////////
		// constraints due to number of inputs constraint/////////////////////
		//////////////////////////////////////////////////////////////////////
		// loop across all LEs
		for (int i = 0; i < FPGAsizeX; i++)
		{
			std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++;
						}
					}


					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;

					assert(k%LUTFreq == 0); // check that it's a LUT

					// loop across LE inputs

					// sets of all paths using this node, first dimension is 
					std::vector<std::vector<int>> paths_sets;
					paths_sets.resize(0);
					paths_sets.resize(InputPortSize);
					for (int l = 0; l < InputPortSize; l++) // loop across all nodes using this LE
					{
						paths_sets[l].resize(0);
					}

					for (int l = 0; l < fpgaLogic[i][j][k].nodes.size(); l++) // loop across all nodes using this LE
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
					//	if (paths[temp_node.path][temp_node.node].portIn != 2)
					//		std::cout << "debug" << std::endl;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path);// push back this path in the correct paths_set.

					}
					// remove path_set of unused ports
					std::vector<std::vector<int>> paths_sets_final;
					paths_sets_final.resize(0);
					int counter = 0;
					for (int l = 0; l < InputPortSize; l++) // loop across all nodes using this LE
					{
						if (paths_sets[l].size()>0)
						{ 
						//	paths_sets_final[counter].resize(0);
							paths_sets_final.push_back(paths_sets[l]);
						}
					}

					// now generate constraints for this paths_sets_final, permutations of paths using this LE

					// vector of counters
					std::vector<int> counters(paths_sets_final.size());
					std::fill(counters.begin(), counters.end(), 0);
					GRBLinExpr temp_constraint;
					while (1)
					{
						temp_constraint = 0.0;
						// for each paths_sets_final[counters_counter] we will pick the index stored in the counters vector.
						for (int counters_counter = 0; counters_counter < counters.size(); counters_counter++)
						{
							temp_constraint += vars[paths_sets_final[counters_counter][counters[counters_counter]]-1];
						}

						// add the constraint to the model
						model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - control_signals);

						// now update the values
						for (int counters_counter = counters.size() - 1; ; )
						{
							counters[counters_counter]++;
							if (counters[counters_counter] == paths_sets_final[counters_counter].size()) // reached the end of this set
							{
								if (counters_counter == 0)
									goto finished_all_constraints;
								else
								{
									counters[counters_counter] = 0; // reset this counter
									counters_counter--; // go to the counter of the next set
								}
								

							}
							else
							{
								break;
							}

						}



					}
				finished_all_constraints: temp_constraint = 0.0;
				}
			}
		}
		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;

		for (int i = 0; i < num_of_paths; i++)
		{
			obj += vars[i];
		}

		model.setObjective(obj, GRB_MAXIMIZE);

		model.optimize();
		int total = 0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (vars[i].get(GRB_DoubleAttr_X) == 0.0)
			{
				if (delete_path(i + 1))
					total++;
			}
			else
			{
				assert(vars[i].get(GRB_DoubleAttr_X) == 1.0);
			}
			std:: cout << vars[i].get(GRB_StringAttr_VarName) << " "
				<< vars[i].get(GRB_DoubleAttr_X) << std::endl;
		}
		std::cout << "deleted number of paths by ILP solver is " << total << std::endl;
	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}

}

*/
// maximize number of paths/bitstream,
// using auxilliary variables to represent LUT inputs and use these variables to create constraints for LUT inputs and re-convergent fanout
void ILP_solve(std::vector<double> pathsImport, bool use_MC, int bitstreams) // bitstreams just for debugging purposes
{
	try {
		GRBEnv env = GRBEnv();

		GRBModel model = GRBModel(env);
		model.getEnv().set("TimeLimit", "1000.0");
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ////////////////////////////////////////////////////////          create variables for the ILP       ////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////
		////// Path variables////////////////////
		/////////////////////////////////////////

		int num_of_paths = (int)paths.size() - 1;
		double* lb_vars = new double[num_of_paths];
		double* up_vars = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars = new char[num_of_paths];
		string* names_vars = new string[num_of_paths];


		for (int i = 0; i < num_of_paths; i++)
		{
			lb_vars[i] = 0.0;
			up_vars[i] = 1.0;
			type_vars[i] = GRB_BINARY;
			string temp = "P" + std::to_string(i + 1);
			names_vars[i] = temp;
		//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars = model.addVars(lb_vars, up_vars, NULL, type_vars, names_vars, num_of_paths);
	
		/////////////////////////////////////////////////////////////////////
		//////// auxilliary variables input ports of LUTs //////////////////
		////////////////////////////////////////////////////////////////////

		std::unordered_map <std::string, int> hashTable;
		std::unordered_map <int, std::string> reverseHashTable;

		// build hash table for the new auxiliary variables
		int num_of_auxiliary_variables = 0;
		int hash_table_size = 0;
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // not lut skip
						continue;

					// 1st sol if BRAM then ignore
					if (fpgaLogic[i][j][k].isBRAM)
						continue;

					if (fpgaLogic[i][j][k].utilization < 1) // not used skip
						continue;

					num_of_auxiliary_variables += fpgaLogic[i][j][k].usedInputPorts; // add used input port of this lut as they are variables;

					int double_check = 0;
					for (int l = 0; l < InputPortSize; l++)
					{
						if (fpgaLogic[i][j][k].inputPorts[l]) //port l is used
						{
							double_check++;
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 == hashTable.end()); // every entry is unique, we only enter it once, so double check that 

							hashTable.insert(make_pair(temp_key, hash_table_size));
							reverseHashTable.insert(make_pair(hash_table_size, temp_key));

							hash_table_size++;
						}
					}
					assert(double_check == fpgaLogic[i][j][k].usedInputPorts);
				}
			}
		}

		assert(hash_table_size == num_of_auxiliary_variables);

		/// now create the variables and add it to the model

		double* lb_vars_aux = new double[num_of_auxiliary_variables];
		double* up_vars_aux = new double[num_of_auxiliary_variables];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_aux = new char[num_of_auxiliary_variables];
		string* names_vars_aux = new string[num_of_auxiliary_variables];


		for (int i = 0; i < num_of_auxiliary_variables; i++)
		{
			lb_vars_aux[i] = 0.0;
			up_vars_aux[i] = 1.0;
			type_vars_aux[i] = GRB_BINARY;
			std::unordered_map < int, std::string>::iterator iter2 = reverseHashTable.find(i); // representative variable names
			assert(iter2 != reverseHashTable.end());// this element must be there
			string temp = "X_" +iter2->second;
			names_vars_aux[i] = temp;
		//	std::cout << names_vars_aux[i] << " ";
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_aux = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////          create constraints for the ILP      //////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////
		///////////// set deleted path to 0//////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		for (int i = 1; i < (int)paths.size(); i++)
		{
			// if this is a path in the BRAM
			// then its size would be 0
			// ignore this path
			if (paths[i].size() == 0)
				continue;
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars[i - 1], GRB_EQUAL, 0.0);
				
				
			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// //////////
		//////////////////////////////////////////////////////////////////////

		//int totala = 0;
		
		// /*
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a reg skip it
						continue;

					// 1st sol if BRAM then ignore
					if (fpgaLogic[i][j][k].isBRAM)
						continue;

					std::vector<std::vector<int> > paths_sets;// 2d-vector to store all paths using pin x
					paths_sets.resize(InputPortSize);

					for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path); // add this node to the list of paths using paths[temp_node.path][temp_node.node].portIn
					}

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE, I am checkin if I can fix input L and test LUT using other inputs
					{
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the l port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;
							
							std::vector<int> feeder_paths; // paths using the feeder of LUT i,j,k port l

							feeder_paths.resize(0);


							for (int m = 0; m < (int)fpgaLogic[feeederX][feeederY][feeederZ].nodes.size(); m++) // loop across paths using this LUT
							{
								Path_logic_component temp_node = fpgaLogic[feeederX][feeederY][feeederZ].nodes[m];
								if (paths[temp_node.path][0].deleted) // if deleted ignore it
									continue;
								if (feeederZ%LUTFreq != 0) // reg
								{
									if (temp_node.node != 0) // if this is not a source then skip this path. It means that the connection between this reg and LUT i,j,k does not include path tempNode.path, we take care of this in fix __ off path input function. TRIAL
										continue;
								}
								feeder_paths.push_back(temp_node.path);

							}

							std::vector<int> conflict_ports;
							// loopo across paths_sets and make sure there is no common path between feeder_paths and other paths using 
							//bool extra_check = false;

							std::vector<int> intersectionPaths; // set of paths that is common betwwen feeder_paths & paths_sets[m]

							for (int m = 0; m < (int)paths_sets.size(); m++)
							{
								if (m == l) // current port, that we are checking if it can be fixed
									continue;
								intersectionPaths.resize(0);
								intersectionPaths.clear();

								conflict_ports.resize(0);
								for (int mm = 0; mm < (int)paths_sets[m].size();mm++)
								{
									for (int counter = 0; counter < (int)feeder_paths.size(); counter++)
									{
										if (feeder_paths[counter] == paths_sets[m][mm]) // then port l and port m can not be tested together, because the feeder of port l also connected to another port
										{ 
											conflict_ports.push_back(m);
											// add path to the intersection vector
											intersectionPaths.push_back(feeder_paths[counter]);

										}
									}

								}

								if (intersectionPaths.size()>0) //conflict_ports.size() > 0)
								{

									/* ib 16/12/2016

			//						assert(!extra_check); // check that this happens only once for each port
									extra_check = true;
									/// port l
									add_constraint_recon: std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									assert(iter1 != hashTable.end());

									/// port conflict_ports[0]
									temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(conflict_ports[0]);
									std::unordered_map <std::string, int>::iterator iter2 = hashTable.find(temp_key);
									assert(iter2 != hashTable.end());

									// add contraint
									model.addConstr(vars_aux[iter1->second] + vars_aux[iter2->second] <= 1);

									*/ // ib 16 / 12 / 2016




									//// atthis point we have two sets of paths 1st intersectionPaths, 2nd paths_sets[l] ; these 2 paths can't be tested together so lets create constraints accordigly

									// ib 16/12/2016

									// create two variables to represent each set

									GRBVar intersectionSetILP;
									GRBVar pinSetILP;

									intersectionSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY); 
									pinSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

									for (int setCounter = 0; setCounter < (int)intersectionPaths.size(); setCounter++)
									{
										model.addConstr(intersectionSetILP, GRB_GREATER_EQUAL, vars[intersectionPaths[setCounter] -1]); // model as ILP => Pi
									}

									for (int setCounter = 0; setCounter < (int)paths_sets[l].size(); setCounter++)
									{
										model.addConstr(pinSetILP, GRB_GREATER_EQUAL, vars[paths_sets[l][setCounter ] - 1]); // model as ILP => Pi
									}

									// now add constraint that the sume of these two variables must be less than or equal one
									model.addConstr(pinSetILP + intersectionSetILP <= 1);
								}

							}

						}
					}
				}
			}
		}
		
		/*
///////////////////////////////////////////////////////
//////////////////trial 02/11/2016 //////////////////
		int tempComponentX, tempComponentY, tempComponentZ;
		int deletedPaths = 0;
		Path_logic_component tempNode;
		std::vector < Path_logic_component > rootNodes;

		std::vector<std::vector<int>> paths_matrix;
		paths_matrix.resize(paths.size());

		for (int i = 1; i < paths.size(); i++)
		{
			paths_matrix[i].resize(paths.size());
			std::fill(paths_matrix[i].begin(), paths_matrix[i].end(), 0);

		}

		for (int i = 1; i < paths.size(); i++)
		{
			paths[i][1].deleted = false;
		}

		for (int i = 0; i < paths.size(); i++) // loop across all paths, another approach would be to loop across used logic elements
		{
			std::cout << std::endl << "path:" << i;
			for (int j = 0; j < paths[i].size(); j++) // loop across nodes in that path
			{
				if (i == 61 && j == 0)
					std::cout << "debug reconvergent fanout" << std::endl;
				if (paths[i][0].deleted) // this path is deleted then continue to the next path
					break;
				tempComponentX = paths[i][j].x;
				tempComponentY = paths[i][j].y;
				tempComponentZ = paths[i][j].z;

				for (int k = 0; k < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); k++) // loop across all nodes sharing the LE used by node j in path i
				{
					tempNode = fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[k];
					if (i == tempNode.path) // same path
						continue;
					////// trial stuff
					if (paths[tempNode.path][0].deleted) // if this path is deleted then dont do anything
						continue;

				//	if (paths[tempNode.path][1].deleted && paths[i][1].deleted) // these 2 pairs have been marked already
					if(paths_matrix[i][tempNode.path]==1) // continue added this constraint already
						continue;

					if (paths[i][j].portIn != paths[tempNode.path][tempNode.node].portIn) // diffenent paths and use different inputs
					{
		
						rootNodes.push_back(tempNode);


					}
	
				}
				////// if we can control the outpuf of an LE then just ensure that all paths using any node before rootNodes are no tested at the same time as path i

				for (int k = 0; k < rootNodes.size(); k++)
				{
					tempNode = rootNodes[k];
					tempComponentX = paths[tempNode.path][tempNode.node - 1].x; // get the location of the LE feeding the node in rootNodes
					tempComponentY = paths[tempNode.path][tempNode.node - 1].y;
					tempComponentZ = paths[tempNode.path][tempNode.node - 1].z;
					if (j == 0)
						std::cout << i << " " << j << std::endl;
					//// trial studd
					for (int kk = 0; kk < fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes.size(); kk++) // check the presence of the special reconvergent fanout, if it exists then delete the (less critical) path causing this
					{
						if (i == fpgaLogic[tempComponentX][tempComponentY][tempComponentZ].nodes[kk].path) // this means the fanout is present so we must delete all paths between tempNode.node and tempNode.node - 1 
						{
							//paths[tempNode.path][0].deleted = true;
#ifdef CycloneIV
						//	if (delete_path(tempNode.path))
						//		deletedPaths++;

							if (paths[tempNode.path][0].deleted)
								continue;

						//	if (paths[tempNode.path][1].deleted && paths[i][1].deleted) // these 2 pairs have been marked already
							
							if (paths_matrix[i][tempNode.path] == 1)
								continue;

							model.addConstr(vars[i-1] + vars[tempNode.path-1] <= 1);
							paths_matrix[i][tempNode.path] = 1;
							paths_matrix[tempNode.path][i] = 1;
							paths[i][1].deleted = true;
							paths[tempNode.path][1].deleted = true;
							deletedPaths++;

#endif

#ifdef StratixV
					if (delete_path_stratix(tempNode.path))
						deletedPaths++;
#endif
					//	std::cout << "deleted : " << tempNode.path << std::endl;

						}

					}


				}
				rootNodes.clear();
				//	std::fill(inputs.begin(), inputs.end(), false);
			}
		}
		*/

		//////////////////////////////////////////////////////////////////////
		////// constraints over auxiliary var/////////////////////////////////
		/////// number of inputs constraint///////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////
	//	std::cout << std::endl << "number of constraints" << deletedPaths << std::endl;

		for (int i = 0; i < FPGAsizeX; i++)
		{
			//std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					// 1st sol if BRAM then ignore
					if (fpgaLogic[i][j][k].isBRAM)
						continue;

					bool adder = false;
					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++; // todo:deal with this
							//adder = true;
						}
					}

					//ib 17/12/2016 to deal with adders my man
					if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
					{
						
						adder = true;
					}

					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;

					if (i == 101 && j == 31 && k == 4)
						std::cout << "debugging " << std::endl;

					assert(k%LUTFreq == 0); // check that it's a LUT

					GRBLinExpr temp_constraint = 0.0;

					/// ibrahim 14/12/16 begin
					GRBVar tempControl;

					tempControl = model.addVar(0.0, 1.0, 0.0, GRB_BINARY); // create temp variable to represent one if control signal is needed and zero other wise

					for (int fanout = 0; fanout < (int)fpgaLogic[i][j][k].connections.size(); fanout++) // loop across all fanouts
					{
						// get destination port
						int fanoutDestX = fpgaLogic[i][j][k].connections[fanout].destinationX;
						int fanoutDestY = fpgaLogic[i][j][k].connections[fanout].destinationY;
						int fanoutDestZ = fpgaLogic[i][j][k].connections[fanout].destinationZ;

						int fanoutDestInPort = fpgaLogic[i][j][k].connections[fanout].destinationPort;

						if (fanoutDestX == -1) // this connection is deleted so dont check it
							continue;


						if (fanoutDestZ%LUTFreq != 0) // reg
						{
							if (fpgaLogic[i][j][k].cascadedPaths.size() == 0) // no cascaded paths
								continue;

						//	if (!is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ))
						//		std::cout << "debug" << std::endl;
							bool isCascaded = is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ);
							//assert(is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ) || fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size()==0); // reg is either a cascaded reg or has no fan our connections

							/// else we this LUT is connected to a cascaded reg so control signal might be required depending on the cascaded paths
							// so we will check the connections of this LUT and see 

							
							for (int cascadedFanout = 0; cascadedFanout < (int)fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size(); cascadedFanout++)
							{
								int cascadedFanoutDestX = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationX;
								int cascadedFanoutDestY = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationY;
								int cascadedFanoutDestZ = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationZ;

								if (cascadedFanoutDestX == -1) // means its deleted
									continue;

								assert(isCascaded);

								int cascadedFanoutDestInPort = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationPort;
								
								assert(fpgaLogic[cascadedFanoutDestX][cascadedFanoutDestY][cascadedFanoutDestZ].inputPorts[cascadedFanoutDestInPort]);

								std::string temp_key = std::to_string(cascadedFanoutDestX) + "_" + std::to_string(cascadedFanoutDestY) + "_" + std::to_string(cascadedFanoutDestZ) + "__" + std::to_string(cascadedFanoutDestInPort);
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								model.addConstr(tempControl, GRB_GREATER_EQUAL, vars_aux[iter1->second]); // C_0 >= X11
								if (control_signals < 2)
								{
									std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ << " cascadedX " << cascadedFanoutDestX << " cascadedY " << cascadedFanoutDestY << " cascadedZ " << cascadedFanoutDestZ << std::endl;
								}
								assert(control_signals > 1);

							}

						}
						else // LUT
						{
							for (int inports = 0; inports < InputPortSize; inports++) // loop across all input ports of this LUT (one of the fnaouts of LUT i,j,k)
							{
								if (inports == fanoutDestInPort) // if this is th input port that is part of the fanout connection then continue
									continue;
								if (fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].inputPorts[inports]) // if this othe inport is used then we have to add constraint, to make sure thatwe will add a control signal if required
								{
									std::string temp_key = std::to_string(fanoutDestX) + "_" + std::to_string(fanoutDestY) + "_" + std::to_string(fanoutDestZ) + "__" + std::to_string(inports);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									if (iter1 == hashTable.end())
									{
										std::cout << "destX " << fanoutDestX << " destY " << fanoutDestY << " destZ " << fanoutDestZ << " inports " << inports << std::endl;
									}
									assert(iter1 != hashTable.end());
									model.addConstr(tempControl, GRB_GREATER_EQUAL, vars_aux[iter1->second]); // C_0 >= X11
									if (control_signals < 2)
									{
										std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ <<  std::endl;
									}
									assert(control_signals > 1);
								}

							}
						}

						// loop across 
					}

					/// ibrahim 14/12/16 end

					for (int l = 0; l < InputPortSize ; l++) // loop across all inputs using this LE
					{
						
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							temp_constraint += vars_aux[iter1->second];
						}
					}
					
					//assert(!adder);

					if(adder) // adder
						model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 2);
					else
						model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 1 - tempControl);

					//model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 1 - 1);// tempControl);//control_signals); // ib 14/12/2016// todo make sure u are not messing this up for adders

				}
			}
		}


		////////////////////////////////////////////////////////////////////////////////////
		//////////// Constaints to fix offpath inputs when they are connected to Regs///////
		////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // if it's a reg then skip
						continue;

					// 1st sol if BRAM then ignore
					if (fpgaLogic[i][j][k].isBRAM)
						continue;

					// LUTs only
					if (fpgaLogic[i][j][k].utilization < 2) // if one or less (zero) paths are using this LUT then skip we will never need to fix an off-path input coz there aint
						continue;
					if (fpgaLogic[i][j][k].usedInputPorts < 2) // no off path inputs since only one port is used
						continue;

					// LUT used with more than one input port used
					for (int z = 0; z < InputPortSize; z++)
					{
						if (fpgaLogic[i][j][k].inputPorts[z]) // check if this port is used
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, z, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the z port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							if (feeederZ%LUTFreq == 0) // if the feeder is a lut then skip this port as we can fix this input port using the fix signal of the feeder LUT
								continue;

							// feeder is a reg
							///	if (!is_cascaded_reg(i, j, k)) // if its not a cascaded reg, then we can control it 
							//		continue;


							if (reg_free_input(feeederX, feeederY, feeederZ)) // if its input is free we can control this reg, so thats fine
								continue;

							// could be cascaded or a feed-back path

							int regFeederPath, regFeederNode;
							assert(get_feeder(feeederX, feeederY, feeederZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

							int regFeederX = paths[regFeederPath][regFeederNode].x;
							int regFeederY = paths[regFeederPath][regFeederNode].y;
							int regFeederZ = paths[regFeederPath][regFeederNode].z;

							bool shouldDelete = false;

							std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
							pathsToReg.resize(0);
							std::vector<int> pathsIJKandRegFeeder; // paths using lut i,j,k and using an input other than z and going through the lut feeding the reg
							pathsIJKandRegFeeder.resize(0);

							std::vector<int> pathsIJKZ; // paths using lut i,j,k and using an input Z
							pathsIJKZ.resize(0);

							for (int ii = 0; ii < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); ii++) // loop across all paths using the lut feeding the reg in question
							{
								Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[ii];
								if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
									continue;

								// check if this path feeds the reg
								if (paths[tempNode.path][tempNode.node + 1].x == feeederX && paths[tempNode.path][tempNode.node + 1].y == feeederY && paths[tempNode.path][tempNode.node + 1].z == feeederZ)
								{
									pathsToReg.push_back(tempNode.path);
								}


								// loop across nodes using LUT i,j,k to see all paths using lut i,j,k from an input other than port z
								for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
								{
									if (paths[fpgaLogic[i][j][k].nodes[l].path][0].deleted)
										continue;


									if (ii == 0 && paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn == z)
									{
										pathsIJKZ.push_back(fpgaLogic[i][j][k].nodes[l].path);
									}

									if (paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn != z) // found a path using lut i,j,k from a node other than Z
									{
										if (fpgaLogic[i][j][k].nodes[l].path == tempNode.path) // one of the found paths also use the LUt feeding the reg
										{
											pathsIJKandRegFeeder.push_back(fpgaLogic[i][j][k].nodes[l].path);
											shouldDelete = true;
										}


									}

								}

							}

							if (shouldDelete) // we should delte some stuff coz we cant fix all off-path inputs
							{
							/*	std::vector<int> deletedPaths;
								if (pathsIJKandRegFeeder.size() > pathsToReg.size())
									deletedPaths = pathsToReg;
								else
									deletedPaths = pathsIJKandRegFeeder;

								for (int pop = 0; pop < deletedPaths.size(); pop++)
								{
									if (delete_path(deletedPaths[pop]))
									{
										total++;
										std::cout << "fix off path " << deletedPaths[pop] << std::endl;
									}
								}*/


								// create constraints betweentwo sets. this is different than the re-convergent fanout as these two paths may have commpn paths in them.

								// variable representing set IJKZ
								GRBVar pathsIJKZSetILP;
								pathsIJKZSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

								for (int setCounter = 0; setCounter < (int)pathsIJKZ.size(); setCounter++)
								{
									model.addConstr(pathsIJKZSetILP, GRB_GREATER_EQUAL, vars[pathsIJKZ[setCounter] - 1]); // model as ILP => Pi
								}

								/*	for (int set1Counter = 0; set1Counter < pathsToReg.size(); set1Counter++)
								{
									for (int set2Counter = 0; set2Counter < pathsIJKandRegFeeder.size(); set2Counter++)
									{

										model.addConstr(vars[pathsToReg[set1Counter] - 1] + vars[pathsIJKandRegFeeder[set2Counter]-1] <= 2 - pathsIJKZSetILP);
									}
								}*/

								// create two variables to represent each set

								GRBVar pathsToRegSetILP;
								GRBVar pathsIJKandRegFeederSetILP;

								pathsToRegSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
								pathsIJKandRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

								for (int setCounter = 0; setCounter <(int)pathsToReg.size(); setCounter++)
								{
									model.addConstr(pathsToRegSetILP, GRB_GREATER_EQUAL, vars[pathsToReg[setCounter] - 1]); // model as ILP => Pi
								}

								for (int setCounter = 0; setCounter <(int)pathsIJKandRegFeeder.size(); setCounter++)
								{
									model.addConstr(pathsIJKandRegFeederSetILP, GRB_GREATER_EQUAL, vars[pathsIJKandRegFeeder[setCounter] - 1]); // model as ILP => Pi
								}


								std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(z);
								//	std::cout << temp_key << std::endl;
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

								

								// now add constraint that the sume of these two variables must be less than or equal one
								model.addConstr(pathsIJKandRegFeederSetILP + pathsToRegSetILP <= 2 - vars_aux[iter1->second]);

							}


						}
					}

				}
			}
		}


		/////////////////////////////////////////////////////////////////////////////
		////////////////// Constraints to check that the source reg. can toggle /////
		/////////////////////////////////////////////////////////////////////////////
	//	int pol = 0;

		for (int i = 1; i < (int)paths.size(); i++)
		{
			if (paths[i].size() < 1)
				continue;

			if (paths[i][0].deleted) // if its deleted
				continue;

			int sourceRegX = paths[i][0].x;
			int sourceRegY = paths[i][0].y;
			int sourceRegZ = paths[i][0].z;

			assert(sourceRegZ%LUTFreq != 0 
				|| fpgaLogic[sourceRegX][sourceRegY][sourceRegZ].isBRAM);// muts be reg or a BRAM

			// 1st sol if BRAM then ignore
			if (fpgaLogic[sourceRegX][sourceRegY][sourceRegZ].isBRAM)
				continue;

			if (paths[i][paths[i].size() - 1].x == sourceRegX && paths[i][paths[i].size() - 1].y == sourceRegY && paths[i][paths[i].size() - 1].z == sourceRegZ) // feedback path so ignore it for now
				continue;

			if (reg_free_input(sourceRegX, sourceRegY, sourceRegZ)) // if its input is free we can control this reg, so thats fine
				continue;

			// could be cascaded 

			int regFeederPath, regFeederNode;
			assert(get_feeder(sourceRegX, sourceRegY, sourceRegZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

			int regFeederX = paths[regFeederPath][regFeederNode].x;
			int regFeederY = paths[regFeederPath][regFeederNode].y;
			int regFeederZ = paths[regFeederPath][regFeederNode].z;

			bool shouldDelete = false;
			std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
			pathsToReg.resize(0);

			for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); j++) // loop across this reg feeder and see if it has the same path as path i
			{


				Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j];
				if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
					continue;

				// check if this path feeds the reg
				if (paths[tempNode.path][tempNode.node + 1].x == sourceRegX && paths[tempNode.path][tempNode.node + 1].y == sourceRegY && paths[tempNode.path][tempNode.node + 1].z == sourceRegZ)
				{
					pathsToReg.push_back(tempNode.path);
				}



				if (fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j].path == i)
				{
					//if (delete_path(i))
					//{
						//total++;
						shouldDelete = true;
						//break;
					//}
				}
			}

			if (shouldDelete) // path i uses the LUT feeding its source reg
			{
				GRBVar pathsToRegSetILP;
			//	GRBVar pathsIJKandRegFeederSetILP;

				pathsToRegSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
			//	pathsIJKandRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

				for (int setCounter = 0; setCounter < (int)pathsToReg.size(); setCounter++)
				{
					model.addConstr(pathsToRegSetILP, GRB_GREATER_EQUAL, vars[pathsToReg[setCounter] - 1]); // model as ILP => Pi
				}

				model.addConstr(pathsToRegSetILP + vars[i - 1] <= 1);
			}
			else
			{
				 // path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?
				


				GRBVar pathsRegFeederSetILP;
				
				// ILp var representing all paths using reg feeder and connected to reg
				pathsRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
				

				for (int setCounter = 0; setCounter < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); setCounter++)
				{
					if ((paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][0].deleted)) // ibrahim 01/01/2017
						continue;

					if (paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node+1].x == sourceRegX && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].y == sourceRegY && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].z == sourceRegZ)
						model.addConstr(pathsRegFeederSetILP, GRB_GREATER_EQUAL, vars[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path - 1]); // model as ILP => Pi
				}

				int destinationX, destinationY, destinationZ, destinationPortIn;

				for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
				{
						// get destination j
					destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
					destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
					destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;
					destinationPortIn = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationPort;

					if (destinationX == -1) // deleted connection
						continue;

					if (destinationZ%LUTFreq != 0) // it's a reg then continue. I think.
						continue;

					for (int k = 0; k <(int)fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
					{
						if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
						{

							assert(destinationPortIn != paths[i][fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].node].portIn);

							std::string temp_key = std::to_string(destinationX) + "_" + std::to_string(destinationY) + "_" + std::to_string(destinationZ) + "__" + std::to_string(destinationPortIn);
							//	std::cout << temp_key << std::endl;
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

		//					pol++;

							// now add constraint that the says either P1 can be tested or we can use Xij
						//	model.addConstr(vars[i - 1] + vars_aux[iter1->second] <= 1.7);
							model.addConstr(vars[i - 1] + pathsRegFeederSetILP <= 2 - vars_aux[iter1->second] );

						//	if (delete_path(i))
						//	{
						//		total++;
						////		std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
						//		std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
						//		std::cout << "Hit the extreme corner case" << std::endl;
						//		break;
						//	}
						}
					}
			//		if (pol > 0)
			//			std::cout << "5ara 3ala " << pol << " *********************************************" << std::endl;

				//	pol = 0;
				}

				
			}

		/*	if (!shouldDelete) // path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?
			{

				int destinationX, destinationY, destinationZ;

				for (int j = 0; j < fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
				{
					// get destination j
					destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
					destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
					destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;

					if (destinationX == -1) // deleted connection
						continue;

					if (destinationZ%LUTFreq != 0) // it's a reg then continue. I think.
						continue;

					for (int k = 0; k < fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
					{
						if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
						{
							if (delete_path(i))
							{
								total++;
								std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
								std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
								std::cout << "Hit the extreme corner case" << std::endl;
								break;
							}
						}
					}

				}*/

			}




		/////////////////////////////////////////////////////////////////////////
		////// constraints to relate auxiliary variable with paths variables ////
		////// Pi = X_i_j_k_P1 & X_i_j_kk_P2 & ......... ////////////////////////
		/////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < (int)paths.size(); i++)
		{
			if (paths[i].size() < 3) // no LUTs
				continue;
			if (paths[i][0].deleted)
				continue;
			// ib14/12/16 GRBVar* temp_constr = new GRBVar[paths[i].size()-2];
			for (int j = 1; j <(int)paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
			//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

				model.addConstr(vars[i - 1], GRB_LESS_EQUAL, vars_aux[iter1->second]); // model as Pi<=Xij

			}
			// ib14/12/16 model.addGenConstrAnd(vars[i-1], temp_constr, paths[i].size() - 2);
			// ib14/12/16 delete [] temp_constr;
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;
		double* coeff = new double[num_of_paths];
		if (use_MC) // use the information we got from MC to test paths
		{
			assert((int)pathsImport.size() == num_of_paths + 1);

			if (bitstreams == 1)
			{
				std::cout << "\\\\\\\\\\\\\\\\\\\\\\///////////////\\\\\\\\\\\\\\\\\\/////////////////deleteing" << std::endl;
			}

			for (int i = 0; i < num_of_paths; i++)
			{
				coeff[i] = pathsImport[i + 1];
			//	if (coeff[i] >= 1)
			//		std::cout << i + 1 << " ";
				//(1.0 / (num_of_paths*num_of_paths))* (num_of_paths - i); //num_of_paths - 
				if (bitstreams == 1) // lsat bit stream with the problem
				{
				//	if (coeff[i] < 1.0)
				//		model.addConstr(vars[i], GRB_EQUAL, 0.0);
				}
			}

			obj.addTerms(coeff, vars, num_of_paths);
			
		}
		else // just try maximizing tested paths with equal probability
		{


			for (int i = 0; i < num_of_paths; i++)
			{
				// if this is a path in the BRAM
				// then its size would be 0
				// ignore this path
				if (paths[i + 1].size() == 0)
					continue;
				if (paths[i + 1][0].deleted)
					continue;
				obj += vars[i];
			}
		}
		model.setObjective(obj, GRB_MAXIMIZE);

		model.optimize();
		int total = 0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (vars[i].get(GRB_DoubleAttr_X) < 0.99999)
			{
				if (delete_path(i + 1))
					total++;

				if (vars[i].get(GRB_DoubleAttr_X) != 0.0)
				{
					if (abs(vars[i].get(GRB_DoubleAttr_X)) >= 0.00000000001)
						std::cout << i << " " << vars[i].get(GRB_DoubleAttr_X) << std::endl;
					assert(abs(vars[i].get(GRB_DoubleAttr_X)) < 0.00000000001);

				}
			}
			else
			{
					//	std::cout << vars[i].get(GRB_StringAttr_VarName) << " "
					//		<< vars[i].get(GRB_DoubleAttr_X) << " " << 
					//		coeff[i] << std::endl;
				if (vars[i].get(GRB_DoubleAttr_X)  <= 0.9999)
					std::cout << i << " " << vars[i].get(GRB_DoubleAttr_X) << std::endl;
				assert(vars[i].get(GRB_DoubleAttr_X) > 0.9999);
			}
	//		std::cout << vars[i].get(GRB_StringAttr_VarName) << " "
	//			<< vars[i].get(GRB_DoubleAttr_X) << std::endl;
		}


	//	for (int i = 0; i < num_of_paths; i++)
	//		std::cout << vars[i].get(GRB_StringAttr_VarName) << std::endl;

		std::cout << "**********Number of deleted paths from ILP solver : " << total << " **********" << std::endl;
		IgnoredPathStats << total << "\t";

		////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////// House Cleaning ////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////

		delete[] lb_vars;
		delete[] up_vars;
		delete[] type_vars;
		delete[] names_vars;
		delete[] lb_vars_aux;
		delete[] up_vars_aux;
		delete[] type_vars_aux;
		delete[] names_vars_aux;
		delete[] coeff;

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}





}



int number_of_edges_cacsaded_paths(std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, bool strictEdgeCounting , std::map<std::string, double>  timingEdgesMapComplete) // strictEdgeCounting means we only count this edge if it is tested through the longest critical path
{
	int total = 0;
	bool flag = true;
	for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++) // loop across all edges
	{
		// check if this edge is tested
		flag = true;
		std::vector <int> tempPaths;// = iter->second; // get the equivalent edge from the complete timing net list
		for (int counter_vector = 0; counter_vector < (int)(iter->second).size(); counter_vector++)
			tempPaths.push_back((iter->second)[counter_vector].path);

		assert(tempPaths.size() > 0);
		if (!strictEdgeCounting)
		{

			for (int i = 0; i <(int)tempPaths.size(); i++)
			{
				if ((paths[tempPaths[i]][0].x != paths[tempPaths[i]].back().x) || (paths[tempPaths[i]][0].y != paths[tempPaths[i]].back().y) || ((paths[tempPaths[i]][0].z != paths[tempPaths[i]].back().z)))
				{
					flag = false;
					break;
				}
			}

		}
		else // only check if the first path (most critical path) is a feedback path
		{
		//	if ((paths[tempPaths[0]][0].x != paths[tempPaths[0]].back().x) || (paths[tempPaths[0]][0].y != paths[tempPaths[0]].back().y) || ((paths[tempPaths[0]][0].z != paths[tempPaths[0]].back().z)))
		//	{
		//		flag = false;
		//		break;
		//	}

			for (int i = 0; i <(int)tempPaths.size(); i++)
			{
				auto iter_temp = timingEdgesMapComplete.find(iter->first); // get the equivalent edge from the complete timing net list
				assert(iter_temp != timingEdgesMapComplete.end()); // must be there
				if (pathSlack[tempPaths[i]]<= iter_temp->second)
				{ 
					if ((paths[tempPaths[i]][0].x != paths[tempPaths[i]].back().x) || (paths[tempPaths[i]][0].y != paths[tempPaths[i]].back().y) || ((paths[tempPaths[i]][0].z != paths[tempPaths[i]].back().z)))
					{
						flag = false;
						break;
					}
				}
			}

		}
		if (flag)
			total++;

	}

	return total;
}


// maximize number of timing edges teted/ bitstream , uses auxiliary variables tor epresent LUT inputs and timing edges.
void ILP_solve_max_timing_edges(std::map<std::string, double>  testedTimingEdgesMap , std::map<std::string, std::vector<Path_logic_component> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete , bool strictEdgeCounting, bool cascadedRegion)
{
	try {
		GRBEnv env = GRBEnv();

		GRBModel model = GRBModel(env);
		model.getEnv().set("TimeLimit", "1000.0");
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ////////////////////////////////////////////////////////          create variables for the ILP       ////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////
		////// Path variables////////////////////
		/////////////////////////////////////////

		int num_of_paths = (int)paths.size() - 1;
		double* lb_vars = new double[num_of_paths];
		double* up_vars = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars = new char[num_of_paths];
		string* names_vars = new string[num_of_paths];


		for (int i = 0; i < num_of_paths; i++)
		{
			lb_vars[i] = 0.0;
			up_vars[i] = 1.0;
			type_vars[i] = GRB_BINARY;
			string temp = "P" + std::to_string(i + 1);
			names_vars[i] = temp;
			//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars = model.addVars(lb_vars, up_vars, NULL, type_vars, names_vars, num_of_paths);
		

		/////////////////////////////////////////////////////////////////////
		///////// variables for timing edges ////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		double* lb_vars_edges = new double[timingEdgesMapComplete.size()];
		double* up_vars_edges = new double[timingEdgesMapComplete.size()];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_edges = new char[timingEdgesMapComplete.size()];
		string* names_vars_edges = new string[timingEdgesMapComplete.size()];

		for (int i = 0; i < (int)timingEdgesMapComplete.size(); i++)
		{
			lb_vars_edges[i] = 0.0;
			up_vars_edges[i] = 1.0;
			type_vars_edges[i] = GRB_BINARY;
			string temp = "E" + std::to_string(i);
			names_vars_edges[i] = temp;
			//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_edges = model.addVars(lb_vars_edges, up_vars_edges, NULL, type_vars_edges, names_vars_edges, (int)timingEdgesMapComplete.size());

		

		/////////////////////////////////////////////////////////////////////
		//////// auxilliary variables input ports of LUTs //////////////////
		////////////////////////////////////////////////////////////////////

		std::unordered_map <std::string, int> hashTable;
		std::unordered_map <int, std::string> reverseHashTable;

		// build hash table for the new auxiliary variables
		int num_of_auxiliary_variables = 0;
		int hash_table_size = 0;
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // not lut skip
						continue;

					if (fpgaLogic[i][j][k].utilization < 1) // not used skip
						continue;

					num_of_auxiliary_variables += fpgaLogic[i][j][k].usedInputPorts; // add used input port os this lut as they are variables;

					int double_check = 0;
					for (int l = 0; l < InputPortSize; l++)
					{
						if (fpgaLogic[i][j][k].inputPorts[l]) //port l is used
						{
							double_check++;
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 == hashTable.end()); // every entry is unique, we only enter it once, so double check that 

							hashTable.insert(make_pair(temp_key, hash_table_size));
							reverseHashTable.insert(make_pair(hash_table_size, temp_key));

							hash_table_size++;
						}
					}
					assert(double_check == fpgaLogic[i][j][k].usedInputPorts);
				}
			}
		}

		assert(hash_table_size == num_of_auxiliary_variables);

		/// now create the variables and add it to the model

		double* lb_vars_aux = new double[num_of_auxiliary_variables];
		double* up_vars_aux = new double[num_of_auxiliary_variables];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_aux = new char[num_of_auxiliary_variables];
		string* names_vars_aux = new string[num_of_auxiliary_variables];


		for (int i = 0; i < num_of_auxiliary_variables; i++)
		{
			lb_vars_aux[i] = 0.0;
			up_vars_aux[i] = 1.0;
			type_vars_aux[i] = GRB_BINARY;
			std::unordered_map < int, std::string>::iterator iter2 = reverseHashTable.find(i); // representative variable names
			assert(iter2 != reverseHashTable.end());// this element must be there
			string temp = "X_" + iter2->second;
			names_vars_aux[i] = temp;
			//	std::cout << names_vars_aux[i] << " ";
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_aux = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////          create constraints for the ILP      //////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////
		////////////////// set tested timing edges to 0 //////////////////////
		//////////////////////////////////////////////////////////////////////
		int total_tested_timing_edges = 0;
		int counter = 0;
		for (auto iter = timingEdgesMapComplete.begin(); iter != timingEdgesMapComplete.end(); iter++) // loop across all edges
		{
			// check if this edge is tested
			auto iter_temp = testedTimingEdgesMap.find(iter->first); // get the equivalent edge from the complete timing net list

			if (iter_temp!= testedTimingEdgesMap.end()) // then this edge exists in the testedTimingEdgesMap, thus it is tested alread so thet it to zero
			{ 
				if (!strictEdgeCounting) // if we are not counting timing edges strictly, (strict means that we count timing edge as tested only if it's tested through the most critical edge)
				{ 
					model.addConstr(vars_edges[counter], GRB_EQUAL, 0.0);
					total_tested_timing_edges++;
				}
				else
				{
					if (iter_temp->second <= iter->second) // if we are strictly testing timing paths then, we only count the edge as testing if it's tested with the worst slack
					{
						model.addConstr(vars_edges[counter], GRB_EQUAL, 0.0);
						total_tested_timing_edges++;
					}
				}
			}
			counter++;

		}

		///////////////////////////////////////////////////////////////////////
		//////////////////////// Ei = | (All paths using this edge) ///////////
		///////////////////////////////////////////////////////////////////////
		counter = 0;

		for (auto iter = timingEdgesMapComplete.begin(); iter != timingEdgesMapComplete.end(); iter++) // loop across all edges
		{
			// check if this edge is tested
			auto iter_temp = testedTimingEdgesMap.find(iter->first); // get the equivalent edge from the complete timing net list


			if (iter_temp != testedTimingEdgesMap.end()) // then this edge exists in the testedTimingEdgesMap, thus it is tested alread so thet it to zero
			{
				if (!strictEdgeCounting) // if we are not counting timing edges strictly, (strict means that we count timing edge as tested only if it's tested through the most critical edge)
				{
					counter++;
					continue;
				}
				else
				{
					if (iter_temp->second <= iter->second) // if we are strictly testing timing paths then, we only count the edge as testing if it's tested with the worst slack
					{
						counter++;
						continue;
					}
				}
			}


	/*		if (iter_temp != testedTimingEdgesMap.end() && !strictEdgeCounting) // then this edge exists in the testedTimingEdgesMap, thus it is tested alread so go to the next edge as this one is already set to 0
			{ 
				counter++;
				continue;
			}
			*/

			// get the vector of paths corresponding to this edge
			auto iter_edge_paths = timingEdgeToPaths.find(iter->first);

			assert(iter_edge_paths != timingEdgeToPaths.end());
			std::vector<int> paths_using_edge;// = iter_edge_paths->second;

			for (int counter_vector = 0; counter_vector < (int)(iter_edge_paths->second).size(); counter_vector++)
				paths_using_edge.push_back((iter_edge_paths->second)[counter_vector].path);


			// add all paths variable to be ored later
			GRBVar* temp_constr = new GRBVar[paths_using_edge.size()];
			int number_of_paths = 0;
			for (int j = 0; j < (int)paths_using_edge.size(); j++)
			{
				if (!strictEdgeCounting) // if we are not counting edges strictly, then add all paths using this edge
				{ 
					temp_constr[j] = vars[paths_using_edge[j] - 1];
					number_of_paths++;
				}
				else
				{
					if (pathSlack[paths_using_edge[j]]<=iter->second) // only add paths if they are using the edge through worst slack
					{
						temp_constr[j] = vars[paths_using_edge[j] - 1];
						number_of_paths++;
			//			if (counter == 24 || counter == 241)
			//				std::cout << "edge " << counter << " path " << paths_using_edge[j] << std::endl;
					}
				}
		//		std::cout << "path " << paths_using_edge[j] << std::endl;
			}
		//	std::cout << counter << std::endl;
		//	std::cout << paths_using_edge.size() << std::endl;
			
			assert(number_of_paths > 0);
		//	if (counter == 24 || counter == 241)
		//		std::cout << "edge " << counter << "number of paths " << number_of_paths << " path " << paths_using_edge[0] << std::endl;
			model.addGenConstrOr(vars_edges[counter], temp_constr, number_of_paths); // paths_using_edge.size());
			delete[] temp_constr;
			
			counter++;

		}



		//////////////////////////////////////////////////////////////////////
		///////////// set deleted path to 0//////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		for (int i = 1; i < (int)paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars[i - 1], GRB_EQUAL, 0.0);

			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		//////														//////////
		//////////////////////////////////////////////////////////////////////

		// copied from ilp_solve_2
		//int totala = 0;

		// /*
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a reg skip it
						continue;


					std::vector<std::vector<int> > paths_sets;// 2d-vector to store all paths using pin x
					paths_sets.resize(InputPortSize);

					for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path); // add this node to the list of paths using paths[temp_node.path][temp_node.node].portIn
					}

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE, I am checkin if I can fix input L and test LUT using other inputs
					{
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the l port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							std::vector<int> feeder_paths; // paths using the feeder of LUT i,j,k port l

							feeder_paths.resize(0);


							for (int m = 0; m <(int)fpgaLogic[feeederX][feeederY][feeederZ].nodes.size(); m++) // loop across paths using this LUT
							{
								Path_logic_component temp_node = fpgaLogic[feeederX][feeederY][feeederZ].nodes[m];
								if (paths[temp_node.path][0].deleted) // if deleted ignore it
									continue;
								if (feeederZ%LUTFreq != 0) // reg
								{
									if (temp_node.node != 0) // if this is not a source then skip this path. It means that the connection between this reg and LUT i,j,k does not include path tempNode.path, we take care of this in fix __ off path input function. TRIAL
										continue;
								}
								feeder_paths.push_back(temp_node.path);

							}

							std::vector<int> conflict_ports;
							// loopo across paths_sets and make sure there is no common path between feeder_paths and other paths using 
							//bool extra_check = false;

							std::vector<int> intersectionPaths; // set of paths that is common betwwen feeder_paths & paths_sets[m]

							for (int m = 0; m < (int)paths_sets.size(); m++)
							{
								if (m == l) // current port, that we are checking if it can be fixed
									continue;
								intersectionPaths.resize(0);
								intersectionPaths.clear();

								conflict_ports.resize(0);
								for (int mm = 0; mm <(int)paths_sets[m].size(); mm++)
								{
									for ( counter = 0; counter < (int)feeder_paths.size(); counter++)
									{
										if (feeder_paths[counter] == paths_sets[m][mm]) // then port l and port m can not be tested together, because the feeder of port l also connected to another port
										{
											conflict_ports.push_back(m);
											// add path to the intersection vector
											intersectionPaths.push_back(feeder_paths[counter]);

										}
									}

								}

								if (intersectionPaths.size()>0) //conflict_ports.size() > 0)
								{

									/* ib 16/12/2016

									//						assert(!extra_check); // check that this happens only once for each port
									extra_check = true;
									/// port l
									add_constraint_recon: std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									assert(iter1 != hashTable.end());

									/// port conflict_ports[0]
									temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(conflict_ports[0]);
									std::unordered_map <std::string, int>::iterator iter2 = hashTable.find(temp_key);
									assert(iter2 != hashTable.end());

									// add contraint
									model.addConstr(vars_aux[iter1->second] + vars_aux[iter2->second] <= 1);

									*/ // ib 16 / 12 / 2016




									//// atthis point we have two sets of paths 1st intersectionPaths, 2nd paths_sets[l] ; these 2 paths can't be tested together so lets create constraints accordigly

									// ib 16/12/2016

									// create two variables to represent each set

									GRBVar intersectionSetILP;
									GRBVar pinSetILP;

									intersectionSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
									pinSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

									for (int setCounter = 0; setCounter < (int)intersectionPaths.size(); setCounter++)
									{
										model.addConstr(intersectionSetILP, GRB_GREATER_EQUAL, vars[intersectionPaths[setCounter] - 1]); // model as ILP => Pi
									}

									for (int setCounter = 0; setCounter < (int)paths_sets[l].size(); setCounter++)
									{
										model.addConstr(pinSetILP, GRB_GREATER_EQUAL, vars[paths_sets[l][setCounter] - 1]); // model as ILP => Pi
									}

									// now add constraint that the sume of these two variables must be less than or equal one
									model.addConstr(pinSetILP + intersectionSetILP <= 1);
								}

							}

						}
					}
				}
			}
		}

		 /*
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a reg skip it
						continue;


					std::vector<std::vector<int> > paths_sets;// 2d-vector to store all paths using pin x
					paths_sets.resize(InputPortSize);

					for (int l = 0; l < fpgaLogic[i][j][k].nodes.size(); l++)
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path); // add this node to the list of paths using paths[temp_node.path][temp_node.node].portIn
					}
					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE, I am checkin if I can fix input L and test LUT using other inputs
					{
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the l port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							std::vector<int> feeder_paths; // paths using the feeder of LUT i,j,k port l

							for (int m = 0; m < fpgaLogic[feeederX][feeederY][feeederZ].nodes.size(); m++) // loop across paths using this LUT
							{
								Path_logic_component temp_node = fpgaLogic[feeederX][feeederY][feeederZ].nodes[m];
								if (paths[temp_node.path][0].deleted) // if deleted ignore it
									continue;
								if (feeederZ%LUTFreq != 0) // reg
								{
									if (temp_node.node != 0) // if this is not a source then skip this path. It means that the connection between this reg and LUT i,j,k does not include path tempNode.path, we take care of this in fix __ off path input function. TRIAL
										continue;
								}
								feeder_paths.push_back(temp_node.path);

							}

							std::vector<int> conflict_ports;
							// loopo across paths_sets and make sure there is no common path between feeder_paths and other paths using 
							bool extra_check = false;

							for (int m = 0; m < paths_sets.size(); m++)
							{
								if (m == l) // current port, that we are checking if it can be fixed
									continue;

								conflict_ports.resize(0);
								for (int mm = 0; mm < paths_sets[m].size(); mm++)
								{
									for (int counter = 0; counter < feeder_paths.size(); counter++)
									{
										if (feeder_paths[counter] == paths_sets[m][mm]) // then port l and port m can not be tested together, because the feeder of port l also connected to another port
										{
											conflict_ports.push_back(m);
										}
									}

								}
								if (conflict_ports.size() > 0)
								{
									//						assert(!extra_check); // check that this happens only once for each port
									extra_check = true;
									/// port l
								add_constraint_recon: std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									assert(iter1 != hashTable.end());

									/// port conflict_ports[0]
									temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(conflict_ports[0]);
									std::unordered_map <std::string, int>::iterator iter2 = hashTable.find(temp_key);
									assert(iter2 != hashTable.end());

									// add contraint
									model.addConstr(vars_aux[iter1->second] + vars_aux[iter2->second] <= 1);

								}

							}

						}
					}
				}
			}
		}*/

	

		//////////////////////////////////////////////////////////////////////
		////// constraints over auxiliary var/////////////////////////////////
		/////// number of inputs constraint///////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////
		//	std::cout << std::endl << "number of constraints" << deletedPaths << std::endl;
		
		// copied from ILP_solve_2
for (int i = 0; i < FPGAsizeX; i++)
		{
			//std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					bool adder = false;
					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++; // todo:deal with this
							//adder = true;
						}
					}

					//ib 17/12/2016 to deal with adders my man
					if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
					{
						
						adder = true;
					}

					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;

					if (i == 101 && j == 31 && k == 4)
						std::cout << "debugging " << std::endl;

					assert(k%LUTFreq == 0); // check that it's a LUT

					GRBLinExpr temp_constraint = 0.0;

					/// ibrahim 14/12/16 begin
					GRBVar tempControl;

					tempControl = model.addVar(0.0, 1.0, 0.0, GRB_BINARY); // create temp variable to represent one if control signal is needed and zero other wise

					for (int fanout = 0; fanout < (int)fpgaLogic[i][j][k].connections.size(); fanout++) // loop across all fanouts
					{
						// get destination port
						int fanoutDestX = fpgaLogic[i][j][k].connections[fanout].destinationX;
						int fanoutDestY = fpgaLogic[i][j][k].connections[fanout].destinationY;
						int fanoutDestZ = fpgaLogic[i][j][k].connections[fanout].destinationZ;

						int fanoutDestInPort = fpgaLogic[i][j][k].connections[fanout].destinationPort;

						if (fanoutDestX == -1) // this connection is deleted so dont check it
							continue;


						if (fanoutDestZ%LUTFreq != 0) // reg
						{
							if (fpgaLogic[i][j][k].cascadedPaths.size() == 0) // no cascaded paths
								continue;

						//	if (!is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ))
						//		std::cout << "debug" << std::endl;
							bool isCascaded = is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ);
							//assert(is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ) || fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size()==0); // reg is either a cascaded reg or has no fan our connections

							/// else we this LUT is connected to a cascaded reg so control signal might be required depending on the cascaded paths
							// so we will check the connections of this LUT and see 

							
							for (int cascadedFanout = 0; cascadedFanout < (int)fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size(); cascadedFanout++)
							{
								int cascadedFanoutDestX = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationX;
								int cascadedFanoutDestY = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationY;
								int cascadedFanoutDestZ = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationZ;

								if (cascadedFanoutDestX == -1) // means its deleted
									continue;

								assert(isCascaded);

								int cascadedFanoutDestInPort = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationPort;
								
								assert(fpgaLogic[cascadedFanoutDestX][cascadedFanoutDestY][cascadedFanoutDestZ].inputPorts[cascadedFanoutDestInPort]);

								std::string temp_key = std::to_string(cascadedFanoutDestX) + "_" + std::to_string(cascadedFanoutDestY) + "_" + std::to_string(cascadedFanoutDestZ) + "__" + std::to_string(cascadedFanoutDestInPort);
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								model.addConstr(tempControl, GRB_GREATER_EQUAL, vars_aux[iter1->second]); // C_0 >= X11
								if (control_signals < 2)
								{
									std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ << " cascadedX " << cascadedFanoutDestX << " cascadedY " << cascadedFanoutDestY << " cascadedZ " << cascadedFanoutDestZ << std::endl;
								}
								assert(control_signals > 1);

							}

						}
						else // LUT
						{
							for (int inports = 0; inports < InputPortSize; inports++) // loop across all input ports of this LUT (one of the fnaouts of LUT i,j,k)
							{
								if (inports == fanoutDestInPort) // if this is th input port that is part of the fanout connection then continue
									continue;
								if (fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].inputPorts[inports]) // if this othe inport is used then we have to add constraint, to make sure thatwe will add a control signal if required
								{
									std::string temp_key = std::to_string(fanoutDestX) + "_" + std::to_string(fanoutDestY) + "_" + std::to_string(fanoutDestZ) + "__" + std::to_string(inports);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									if (iter1 == hashTable.end())
									{
										std::cout << "destX " << fanoutDestX << " destY " << fanoutDestY << " destZ " << fanoutDestZ << " inports " << inports << std::endl;
									}
									assert(iter1 != hashTable.end());
									model.addConstr(tempControl, GRB_GREATER_EQUAL, vars_aux[iter1->second]); // C_0 >= X11
									if (control_signals < 2)
									{
										std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ <<  std::endl;
									}
									assert(control_signals > 1);
								}

							}
						}

						// loop across 
					}

					/// ibrahim 14/12/16 end

					for (int l = 0; l < InputPortSize ; l++) // loop across all inputs using this LE
					{
						
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							temp_constraint += vars_aux[iter1->second];
						}
					}
					
					//assert(!adder);

					if(adder) // adder
						model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 2);
					else
						model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 1 - tempControl);

					//model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 1 - 1);// tempControl);//control_signals); // ib 14/12/2016// todo make sure u are not messing this up for adders

				}
			}
		}





		/*
		for (int i = 0; i < FPGAsizeX; i++)
		{
			//std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++; // todo:deal with this
						}
					}


					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;

					assert(k%LUTFreq == 0); // check that it's a LUT

					GRBLinExpr temp_constraint = 0.0;

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE
					{

						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							temp_constraint += vars_aux[iter1->second];
						}
					}

					model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - control_signals); // todo make sure u are not messing this up for adders

				}
			}
		}
		*/


		////////////////////////////////////////////////////////////////////////////////////
		//////////// Constaints to fix offpath inputs when they are connected to Regs///////
		////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // if it's a reg then skip
						continue;

					// LUTs only
					if (fpgaLogic[i][j][k].utilization < 2) // if one or less (zero) paths are using this LUT then skip we will never need to fix an off-path input coz there aint
						continue;
					if (fpgaLogic[i][j][k].usedInputPorts < 2) // no off path inputs since only one port is used
						continue;

					// LUT used with more than one input port used
					for (int z = 0; z < InputPortSize; z++)
					{
						if (fpgaLogic[i][j][k].inputPorts[z]) // check if this port is used
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, z, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the z port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							if (feeederZ%LUTFreq == 0) // if the feeder is a lut then skip this port as we can fix this input port using the fix signal of the feeder LUT
								continue;

							// feeder is a reg
							///	if (!is_cascaded_reg(i, j, k)) // if its not a cascaded reg, then we can control it 
							//		continue;


							if (reg_free_input(feeederX, feeederY, feeederZ)) // if its input is free we can control this reg, so thats fine
								continue;

							// could be cascaded or a feed-back path

							int regFeederPath, regFeederNode;
							assert(get_feeder(feeederX, feeederY, feeederZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

							int regFeederX = paths[regFeederPath][regFeederNode].x;
							int regFeederY = paths[regFeederPath][regFeederNode].y;
							int regFeederZ = paths[regFeederPath][regFeederNode].z;

							bool shouldDelete = false;

							std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
							pathsToReg.resize(0);
							std::vector<int> pathsIJKandRegFeeder; // paths using lut i,j,k and using an input other than z and going through the lut feeding the reg
							pathsIJKandRegFeeder.resize(0);

							std::vector<int> pathsIJKZ; // paths using lut i,j,k and using an input Z
							pathsIJKZ.resize(0);

							for (int ii = 0; ii < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); ii++) // loop across all paths using the lut feeding the reg in question
							{
								Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[ii];
								if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
									continue;

								// check if this path feeds the reg
								if (paths[tempNode.path][tempNode.node + 1].x == feeederX && paths[tempNode.path][tempNode.node + 1].y == feeederY && paths[tempNode.path][tempNode.node + 1].z == feeederZ)
								{
									pathsToReg.push_back(tempNode.path);
								}


								// loop across nodes using LUT i,j,k to see all paths using lut i,j,k from an input other than port z
								for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
								{
									if (paths[fpgaLogic[i][j][k].nodes[l].path][0].deleted)
										continue;


									if (ii == 0 && paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn == z)
									{
										pathsIJKZ.push_back(fpgaLogic[i][j][k].nodes[l].path);
									}

									if (paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn != z) // found a path using lut i,j,k from a node other than Z
									{
										if (fpgaLogic[i][j][k].nodes[l].path == tempNode.path) // one of the found paths also use the LUt feeding the reg
										{
											pathsIJKandRegFeeder.push_back(fpgaLogic[i][j][k].nodes[l].path);
											shouldDelete = true;
										}


									}

								}

							}

							if (shouldDelete) // we should delte some stuff coz we cant fix all off-path inputs
							{
								/*	std::vector<int> deletedPaths;
								if (pathsIJKandRegFeeder.size() > pathsToReg.size())
								deletedPaths = pathsToReg;
								else
								deletedPaths = pathsIJKandRegFeeder;

								for (int pop = 0; pop < deletedPaths.size(); pop++)
								{
								if (delete_path(deletedPaths[pop]))
								{
								total++;
								std::cout << "fix off path " << deletedPaths[pop] << std::endl;
								}
								}*/


								// create constraints betweentwo sets. this is different than the re-convergent fanout as these two paths may have commpn paths in them.

								// variable representing set IJKZ
								GRBVar pathsIJKZSetILP;
								pathsIJKZSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

								for (int setCounter = 0; setCounter < (int)pathsIJKZ.size(); setCounter++)
								{
									model.addConstr(pathsIJKZSetILP, GRB_GREATER_EQUAL, vars[pathsIJKZ[setCounter] - 1]); // model as ILP => Pi
								}

								/*	for (int set1Counter = 0; set1Counter < pathsToReg.size(); set1Counter++)
								{
								for (int set2Counter = 0; set2Counter < pathsIJKandRegFeeder.size(); set2Counter++)
								{

								model.addConstr(vars[pathsToReg[set1Counter] - 1] + vars[pathsIJKandRegFeeder[set2Counter]-1] <= 2 - pathsIJKZSetILP);
								}
								}*/

								// create two variables to represent each set

								GRBVar pathsToRegSetILP;
								GRBVar pathsIJKandRegFeederSetILP;

								pathsToRegSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
								pathsIJKandRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

								for (int setCounter = 0; setCounter < (int)pathsToReg.size(); setCounter++)
								{
									model.addConstr(pathsToRegSetILP, GRB_GREATER_EQUAL, vars[pathsToReg[setCounter] - 1]); // model as ILP => Pi
								}

								for (int setCounter = 0; setCounter <(int)pathsIJKandRegFeeder.size(); setCounter++)
								{
									model.addConstr(pathsIJKandRegFeederSetILP, GRB_GREATER_EQUAL, vars[pathsIJKandRegFeeder[setCounter] - 1]); // model as ILP => Pi
								}


								std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(z);
								//	std::cout << temp_key << std::endl;
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];



								// now add constraint that the sume of these two variables must be less than or equal one
								model.addConstr(pathsIJKandRegFeederSetILP + pathsToRegSetILP <= 2 - vars_aux[iter1->second]);

							}


						}
					}

				}
			}
		}


		/////////////////////////////////////////////////////////////////////////////
		////////////////// Constraints to check that the source reg. can toggle /////
		/////////////////////////////////////////////////////////////////////////////
	//	int pol = 0;

		for (int i = 1; i < (int)paths.size(); i++)
		{
			if (paths[i].size() < 1)
				continue;

			if (paths[i][0].deleted) // if its deleted
				continue;

			int sourceRegX = paths[i][0].x;
			int sourceRegY = paths[i][0].y;
			int sourceRegZ = paths[i][0].z;

			assert(sourceRegZ%LUTFreq != 0);// muts be reg

			if (paths[i][paths[i].size() - 1].x == sourceRegX && paths[i][paths[i].size() - 1].y == sourceRegY && paths[i][paths[i].size() - 1].z == sourceRegZ) // feedback path so ignore it for now
				continue;

			if (reg_free_input(sourceRegX, sourceRegY, sourceRegZ)) // if its input is free we can control this reg, so thats fine
				continue;

			// could be cascaded 

			int regFeederPath, regFeederNode;
			assert(get_feeder(sourceRegX, sourceRegY, sourceRegZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

			int regFeederX = paths[regFeederPath][regFeederNode].x;
			int regFeederY = paths[regFeederPath][regFeederNode].y;
			int regFeederZ = paths[regFeederPath][regFeederNode].z;

			bool shouldDelete = false;
			std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
			pathsToReg.resize(0);

			for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); j++) // loop across this reg feeder and see if it has the same path as path i
			{


				Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j];
				if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
					continue;

				// check if this path feeds the reg
				if (paths[tempNode.path][tempNode.node + 1].x == sourceRegX && paths[tempNode.path][tempNode.node + 1].y == sourceRegY && paths[tempNode.path][tempNode.node + 1].z == sourceRegZ)
				{
					pathsToReg.push_back(tempNode.path);
				}



				if (fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j].path == i)
				{
					//if (delete_path(i))
					//{
					//total++;
					shouldDelete = true;
					//break;
					//}
				}
			}

			if (shouldDelete) // path i uses the LUT feeding its source reg
			{
				GRBVar pathsToRegSetILP;
				//	GRBVar pathsIJKandRegFeederSetILP;

				pathsToRegSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
				//	pathsIJKandRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

				for (int setCounter = 0; setCounter < (int)pathsToReg.size(); setCounter++)
				{
					model.addConstr(pathsToRegSetILP, GRB_GREATER_EQUAL, vars[pathsToReg[setCounter] - 1]); // model as ILP => Pi
				}

				model.addConstr(pathsToRegSetILP + vars[i - 1] <= 1);
			}
			else
			{
				// path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?



				GRBVar pathsRegFeederSetILP;

				// ILp var representing all paths using reg feeder and connected to reg
				pathsRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);


				for (int setCounter = 0; setCounter < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); setCounter++)
				{
					if ((paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][0].deleted)) // ibrahim 01/01/2017
						continue;

					if (paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].x == sourceRegX && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].y == sourceRegY && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].z == sourceRegZ)
						model.addConstr(pathsRegFeederSetILP, GRB_GREATER_EQUAL, vars[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path - 1]); // model as ILP => Pi
				}

				int destinationX, destinationY, destinationZ, destinationPortIn;

				for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
				{
					// get destination j
					destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
					destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
					destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;
					destinationPortIn = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationPort;

					if (destinationX == -1) // deleted connection
						continue;

					if (destinationZ%LUTFreq != 0) // it's a reg then continue. I think.
						continue;

					for (int k = 0; k < (int)fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
					{
						if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
						{

							assert(destinationPortIn != paths[i][fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].node].portIn);

							std::string temp_key = std::to_string(destinationX) + "_" + std::to_string(destinationY) + "_" + std::to_string(destinationZ) + "__" + std::to_string(destinationPortIn);
							//	std::cout << temp_key << std::endl;
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

							//					pol++;

							// now add constraint that the says either P1 can be tested or we can use Xij
							//	model.addConstr(vars[i - 1] + vars_aux[iter1->second] <= 1.7);
							model.addConstr(vars[i - 1] + pathsRegFeederSetILP <= 2 - vars_aux[iter1->second]);

							//	if (delete_path(i))
							//	{
							//		total++;
							////		std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
							//		std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
							//		std::cout << "Hit the extreme corner case" << std::endl;
							//		break;
							//	}
						}
					}
					//		if (pol > 0)
					//			std::cout << "5ara 3ala " << pol << " *********************************************" << std::endl;

					//	pol = 0;
				}


			}

			/*	if (!shouldDelete) // path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?
			{

			int destinationX, destinationY, destinationZ;

			for (int j = 0; j < fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
			{
			// get destination j
			destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
			destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
			destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;

			if (destinationX == -1) // deleted connection
			continue;

			if (destinationZ%LUTFreq != 0) // it's a reg then continue. I think.
			continue;

			for (int k = 0; k < fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
			{
			if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
			{
			if (delete_path(i))
			{
			total++;
			std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
			std::cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << std::endl;
			std::cout << "Hit the extreme corner case" << std::endl;
			break;
			}
			}
			}

			}*/

		}


		/////////////////////////////////////////////////////////////////////////
		////// constraints to relate auxiliary variable with paths variables ////
		////// Pi = X_i_j_k_P1 & X_i_j_kk_P2 & ......... ////////////////////////
		/////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < (int)paths.size(); i++)
		{
			if (paths[i].size() < 3) // no LUTs
				continue;
			if (paths[i][0].deleted)
				continue;
			// ib14/12/16 GRBVar* temp_constr = new GRBVar[paths[i].size()-2];
			for (int j = 1; j < (int)paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
				//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

				model.addConstr(vars[i - 1], GRB_LESS_EQUAL, vars_aux[iter1->second]); // model as Pi<=Xij

			}
			// ib14/12/16 model.addGenConstrAnd(vars[i-1], temp_constr, paths[i].size() - 2);
			// ib14/12/16 delete [] temp_constr;
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		double* coeff = new double[num_of_paths];
		GRBLinExpr obj = 0.0;

		int remaining_deges = (int)timingEdgesMapComplete.size() - total_tested_timing_edges;
		std::cout << " remaining_edges " << remaining_deges << " cascaeded deges " << number_of_edges_cacsaded_paths(timingEdgeToPaths,  strictEdgeCounting, timingEdgesMapComplete) << std::endl;
		
	//	bool more_Edges_to_test;
	//	if (!strictEdgeCounting)
	//		more_Edges_to_test = total_tested_timing_edges < timingEdgesMapComplete.size();
	//	else

		std::cout << "total tested timing edges = " << total_tested_timing_edges << std::endl;
		if (total_tested_timing_edges<(int)timingEdgesMapComplete.size()&&(cascadedRegion || (remaining_deges>number_of_edges_cacsaded_paths(  timingEdgeToPaths, strictEdgeCounting, timingEdgesMapComplete)))) // if we still ahve timing edges to test, then our objective function is to maximize number of edges, other wise maximze number of paths
		{ 
			std::cout << "***************************************** optimizing for edgesss **********************" << std::endl;
			counter = 0;
			for (auto iter = timingEdgesMapComplete.begin(); iter != timingEdgesMapComplete.end(); iter++) // loop across all edges
			{
				// check if this edge is tested
				auto iter_temp = testedTimingEdgesMap.find(iter->first); // get the equivalent edge from the complete timing net list
	
				if (iter_temp != testedTimingEdgesMap.end()) // then this edge exists in the testedTimingEdgesMap, thus it is tested alread so thet it to zero
				{
					if (!strictEdgeCounting)
					{ 
						counter++;
						continue;
					}
					else
					{
						if (iter_temp->second <= iter->second)
						{
							counter++;
							continue;
						}
					}
				}
				obj += vars_edges[counter];
				counter++;

			}

			

#ifdef MCsim
			for (int i = 0; i < num_of_paths; i++)
			{
				coeff[i] = (1.0 / (num_of_paths*num_of_paths))* (num_of_paths - i); //num_of_paths - 
			}

			obj.addTerms(coeff, vars, num_of_paths);
#endif // MCsim

		}
		else
		{
			std::cout << "***************************************** optimizing for paths **********************" << std::endl;
			for (int i = 0; i < num_of_paths; i++)
			{
				if (paths[i + 1][0].deleted)
					continue;
				obj += vars[i];
			}
		}

		// ib manualOverride
	//	obj += vars[64];
	//	obj += vars[66];

		model.setObjective(obj, GRB_MAXIMIZE);

		model.optimize();
		int total = 0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (vars[i].get(GRB_DoubleAttr_X) < 0.99999)
			{
				if (delete_path(i + 1))
					total++;

				if (vars[i].get(GRB_DoubleAttr_X) != 0.0)
				{
					if (abs(vars[i].get(GRB_DoubleAttr_X)) >= 0.00000000001)
						std::cout << i << " " << vars[i].get(GRB_DoubleAttr_X) << std::endl;
					assert(abs(vars[i].get(GRB_DoubleAttr_X)) < 0.00000000001);

				}
			}
			else
			{
				if (vars[i].get(GRB_DoubleAttr_X) <= 0.9999)
					std::cout << i << " " << vars[i].get(GRB_DoubleAttr_X) << std::endl;
				assert(vars[i].get(GRB_DoubleAttr_X) > 0.9999);
			}
		//			std::cout << vars[i].get(GRB_StringAttr_VarName) << " "
		///				<< vars[i].get(GRB_DoubleAttr_X) << std::endl;
		}

	///	for (int i = 0; i < timingEdgesMapComplete.size();i++)
	//		std::cout << vars_edges[i].get(GRB_StringAttr_VarName) << " "	<< vars_edges[i].get(GRB_DoubleAttr_X) << std::endl;

		std::cout << "**********Number of deleted paths from ILP solver : " << total << " **********" << std::endl;
		IgnoredPathStats << total << "\t";

		////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////// House Cleaning ////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////

		delete[] lb_vars;
		delete[] up_vars;
		delete[] type_vars;
		delete[] names_vars;
		delete[] lb_vars_aux;
		delete[] up_vars_aux;
		delete[] type_vars_aux;
		delete[] names_vars_aux;
		delete[] coeff;

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}





}



void ILP_solve_max_paths_per_x_bit_stream(int bitStreams, std::vector < std::vector<int> > & pathsSchedule, std::vector<double> pathsImport, bool use_MC)
{
	try {
		std::cout << "-------MAximizing paths per " << bitStreams << "-----" << std::endl;
		GRBEnv env = GRBEnv();

		GRBModel model = GRBModel(env);
		model.getEnv().set("TimeLimit", "1000.0");

		pathsSchedule.clear();
		pathsSchedule.resize(bitStreams);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ////////////////////////////////////////////////////////          create variables for the ILP       ////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		std::cout << "Number of bit streams " << bitStreams << std::endl;

		/////////////////////////////////////////
		////// Path variables////////////////////
		/////////////////////////////////////////

		int num_of_paths = (int)paths.size() - 1;
		double* lb_vars = new double[num_of_paths];
		double* up_vars = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars = new char[num_of_paths];
		string* names_vars = new string[num_of_paths];


		for (int i = 0; i < num_of_paths; i++)
		{
			lb_vars[i] = 0.0;
			up_vars[i] = 1.0;
			type_vars[i] = GRB_BINARY;
			string temp = "P" + std::to_string(i + 1);
			names_vars[i] = temp;
			//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		std::vector <GRBVar*> vars;
		vars.resize(bitStreams);

		for (int i = 0; i < bitStreams; i++)
		{
			vars[i] = model.addVars(lb_vars, up_vars, NULL, type_vars, names_vars, num_of_paths);
		}

		//GRBVar* vars = model.addVars(lb_vars, up_vars, NULL, type_vars, names_vars, num_of_paths);

		/////////////////////////////////////////////////////////////////////
		//////// auxilliary variables input ports of LUTs //////////////////
		////////////////////////////////////////////////////////////////////

		std::unordered_map <std::string, int> hashTable;
		std::unordered_map <int, std::string> reverseHashTable;

		// build hash table for the new auxiliary variables
		int num_of_auxiliary_variables = 0;
		int hash_table_size = 0;
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // not lut skip
						continue;

					if (fpgaLogic[i][j][k].utilization < 1) // not used skip
						continue;

					num_of_auxiliary_variables += fpgaLogic[i][j][k].usedInputPorts; // add used input port os this lut as they are variables;

					int double_check = 0;
					for (int l = 0; l < InputPortSize; l++)
					{
						if (fpgaLogic[i][j][k].inputPorts[l]) //port l is used
						{
							double_check++;
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 == hashTable.end()); // every entry is unique, we only enter it once, so double check that 

							hashTable.insert(make_pair(temp_key, hash_table_size));
							reverseHashTable.insert(make_pair(hash_table_size, temp_key));

							hash_table_size++;
						}
					}
					assert(double_check == fpgaLogic[i][j][k].usedInputPorts);
				}
			}
		}

		assert(hash_table_size == num_of_auxiliary_variables);

		/// now create the variables and add it to the model

		double* lb_vars_aux = new double[num_of_auxiliary_variables];
		double* up_vars_aux = new double[num_of_auxiliary_variables];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_aux = new char[num_of_auxiliary_variables];
		string* names_vars_aux = new string[num_of_auxiliary_variables];


		for (int i = 0; i < num_of_auxiliary_variables; i++)
		{
			lb_vars_aux[i] = 0.0;
			up_vars_aux[i] = 1.0;
			type_vars_aux[i] = GRB_BINARY;
			std::unordered_map < int, std::string>::iterator iter2 = reverseHashTable.find(i); // representative variable names
			assert(iter2 != reverseHashTable.end());// this element must be there
			string temp = "X_" + iter2->second;
			names_vars_aux[i] = temp;
			//	std::cout << names_vars_aux[i] << " ";
			//std::cout << temp << std::endl;
		}

		std::vector <GRBVar*> vars_aux;
		vars_aux.resize(bitStreams);

		for (int i = 0; i < bitStreams; i++)
		{
			vars_aux[i] = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);
		}

		//GRBVar* vars_aux = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////          create constraints for the ILP      //////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////
		///////////// set deleted path to 0//////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		for (int i = 1; i <(int)paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				for (int j = 0; j < bitStreams; j++)
				{
					model.addConstr((vars[j])[i - 1], GRB_EQUAL, 0.0);
				}
				

			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// //////////
		//////////////////////////////////////////////////////////////////////

		//int totala = 0;

		// /*
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a reg skip it
						continue;


					std::vector<std::vector<int> > paths_sets;// 2d-vector to store all paths using pin x
					paths_sets.resize(InputPortSize);

					for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path); // add this node to the list of paths using paths[temp_node.path][temp_node.node].portIn
					}

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE, I am checkin if I can fix input L and test LUT using other inputs
					{
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the l port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							std::vector<int> feeder_paths; // paths using the feeder of LUT i,j,k port l

							feeder_paths.resize(0);


							for (int m = 0; m < (int)fpgaLogic[feeederX][feeederY][feeederZ].nodes.size(); m++) // loop across paths using this LUT
							{
								Path_logic_component temp_node = fpgaLogic[feeederX][feeederY][feeederZ].nodes[m];
								if (paths[temp_node.path][0].deleted) // if deleted ignore it
									continue;
								if (feeederZ%LUTFreq != 0) // reg
								{
									if (temp_node.node != 0) // if this is not a source then skip this path. It means that the connection between this reg and LUT i,j,k does not include path tempNode.path, we take care of this in fix __ off path input function. TRIAL
										continue;
								}
								feeder_paths.push_back(temp_node.path);

							}

							std::vector<int> conflict_ports;
							// loopo across paths_sets and make sure there is no common path between feeder_paths and other paths using 
						//	bool extra_check = false;

							std::vector<int> intersectionPaths; // set of paths that is common betwwen feeder_paths & paths_sets[m]

							for (int m = 0; m < (int)paths_sets.size(); m++)
							{
								if (m == l) // current port, that we are checking if it can be fixed
									continue;
								intersectionPaths.resize(0);
								intersectionPaths.clear();

								conflict_ports.resize(0);
								for (int mm = 0; mm < (int)paths_sets[m].size(); mm++)
								{
									for (int counter = 0; counter < (int)feeder_paths.size(); counter++)
									{
										if (feeder_paths[counter] == paths_sets[m][mm]) // then port l and port m can not be tested together, because the feeder of port l also connected to another port
										{
											conflict_ports.push_back(m);
											// add path to the intersection vector
											intersectionPaths.push_back(feeder_paths[counter]);

										}
									}

								}

								if (intersectionPaths.size()>0) //conflict_ports.size() > 0)
								{

									//// atthis point we have two sets of paths 1st intersectionPaths, 2nd paths_sets[l] ; these 2 paths can't be tested together so lets create constraints accordigly

									// ib 16/12/2016

									// create two variables to represent each set

									//GRBVar intersectionSetILP;
									std::vector <GRBVar> intersectionSetILP;
									intersectionSetILP.resize(bitStreams);
									
									std::vector <GRBVar> pinSetILP;
									pinSetILP.resize(bitStreams);


									//GRBVar pinSetILP;

									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										intersectionSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
										pinSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
									}
									

									for (int setCounter = 0; setCounter <(int)intersectionPaths.size(); setCounter++)
									{
										for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
										{
											model.addConstr(intersectionSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][intersectionPaths[setCounter] - 1]); // model as ILP => Pi
										}
									//	model.addConstr(intersectionSetILP, GRB_GREATER_EQUAL, vars[intersectionPaths[setCounter] - 1]); // model as ILP => Pi
									}

									for (int setCounter = 0; setCounter < (int)paths_sets[l].size(); setCounter++)
									{
										for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
										{
											model.addConstr(pinSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][paths_sets[l][setCounter] - 1]); // model as ILP => Pi
										}
									}

									// now add constraint that the sume of these two variables must be less than or equal one
									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										model.addConstr(pinSetILP[bitStreamCounter] + intersectionSetILP[bitStreamCounter] <= 1);
									}
								}

							}

						}
					}
				}
			}
		}

	

		//////////////////////////////////////////////////////////////////////
		////// constraints over auxiliary var/////////////////////////////////
		/////// number of inputs constraint///////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////
		//	std::cout << std::endl << "number of constraints" << deletedPaths << std::endl;

		for (int i = 0; i < FPGAsizeX; i++)
		{
			//std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					bool adder = false;
					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++; // todo:deal with this
											   //adder = true;
						}
					}

					//ib 17/12/2016 to deal with adders my man
					if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
					{

						adder = true;
					}

					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;


					assert(k%LUTFreq == 0); // check that it's a LUT

					//GRBLinExpr temp_constraint = 0.0;

					std::vector <GRBLinExpr> temp_constraint;
					temp_constraint.resize(bitStreams);

					std::vector <GRBVar> tempControl;
					tempControl.resize(bitStreams);
					/// ibrahim 14/12/16 begin
				//	GRBVar tempControl;

					for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
					{
						tempControl[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY); // create temp variable to represent one if control signal is needed and zero other wise
						temp_constraint[bitStreamCounter] = 0.0;
					}
					//tempControl = model.addVar(0.0, 1.0, 0.0, GRB_BINARY); // create temp variable to represent one if control signal is needed and zero other wise

					for (int fanout = 0; fanout < (int)fpgaLogic[i][j][k].connections.size(); fanout++) // loop across all fanouts
					{
						// get destination port
						int fanoutDestX = fpgaLogic[i][j][k].connections[fanout].destinationX;
						int fanoutDestY = fpgaLogic[i][j][k].connections[fanout].destinationY;
						int fanoutDestZ = fpgaLogic[i][j][k].connections[fanout].destinationZ;

						int fanoutDestInPort = fpgaLogic[i][j][k].connections[fanout].destinationPort;

						if (fanoutDestX == -1) // this connection is deleted so dont check it
							continue;


						if (fanoutDestZ%LUTFreq != 0) // reg
						{
							if (fpgaLogic[i][j][k].cascadedPaths.size() == 0) // no cascaded paths
								continue;

							if (!(is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ) || fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size() == 0))
							{
								std::cout << "debug" << std::endl;
								std::cout << "fanout size " << fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size() << std::endl;
								std::cout << "cascadede paths " << fpgaLogic[i][j][k].cascadedPaths.size() << std::endl;
								std::cout << "cascaded path is " << fpgaLogic[i][j][k].cascadedPaths[0] << std::endl;
								std::cout << "fanoutdestx " << fanoutDestX << "fanoutdesty " << fanoutDestY << "fanoutdestz " << fanoutDestZ << std::endl;
								std::cout << "sourcetx " << i << "sourcety " << j << "sourcestz " << k << std::endl;
							}
						//	assert(is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ) || fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size() == 0); // reg is either a cascaded reg or has no fan our connections

							// the register could be a cascaded reg or it has no connections (in that case the cascadedPaths include paths connected to another register)

							bool isCascaded = is_cascaded_reg(fanoutDestX, fanoutDestY, fanoutDestZ);

																																										/// else we this LUT is connected to a cascaded reg so control signal might be required depending on the cascaded paths
																																										// so we will check the connections of this LUT and see 
							for (int cascadedFanout = 0; cascadedFanout < (int)fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections.size(); cascadedFanout++)
							{
								int cascadedFanoutDestX = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationX;
								int cascadedFanoutDestY = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationY;
								int cascadedFanoutDestZ = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationZ;

								if (cascadedFanoutDestX == -1) // means its deleted
									continue;

								assert(isCascaded);


								int cascadedFanoutDestInPort = fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].connections[cascadedFanout].destinationPort;

								assert(fpgaLogic[cascadedFanoutDestX][cascadedFanoutDestY][cascadedFanoutDestZ].inputPorts[cascadedFanoutDestInPort]);

								std::string temp_key = std::to_string(cascadedFanoutDestX) + "_" + std::to_string(cascadedFanoutDestY) + "_" + std::to_string(cascadedFanoutDestZ) + "__" + std::to_string(cascadedFanoutDestInPort);
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
								{
									model.addConstr(tempControl[bitStreamCounter], GRB_GREATER_EQUAL, vars_aux[bitStreamCounter][iter1->second]); // C_0 >= X11
								}
								if (control_signals < 2)
								{
									std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ << " cascadedX " << cascadedFanoutDestX << " cascadedY " << cascadedFanoutDestY << " cascadedZ " << cascadedFanoutDestZ << std::endl;
								}
								assert(control_signals > 1);

							}

						}
						else // LUT
						{
							for (int inports = 0; inports < InputPortSize; inports++) // loop across all input ports of this LUT (one of the fnaouts of LUT i,j,k)
							{
								if (inports == fanoutDestInPort) // if this is th input port that is part of the fanout connection then continue
									continue;
								if (fpgaLogic[fanoutDestX][fanoutDestY][fanoutDestZ].inputPorts[inports]) // if this othe inport is used then we have to add constraint, to make sure thatwe will add a control signal if required
								{
									std::string temp_key = std::to_string(fanoutDestX) + "_" + std::to_string(fanoutDestY) + "_" + std::to_string(fanoutDestZ) + "__" + std::to_string(inports);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									if (iter1 == hashTable.end())
									{
										std::cout << "destX " << fanoutDestX << " destY " << fanoutDestY << " destZ " << fanoutDestZ << " inports " << inports << std::endl;
									}
									assert(iter1 != hashTable.end());
									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										model.addConstr(tempControl[bitStreamCounter], GRB_GREATER_EQUAL, vars_aux[bitStreamCounter][iter1->second]); // C_0 >= X11
									}
									if (control_signals < 2)
									{
										std::cout << "x " << i << " y " << j << " z " << k << " fanoutx " << fanoutDestX << " fanouty " << fanoutDestY << " fanout Z " << fanoutDestZ << std::endl;
									}
									assert(control_signals > 1);
								}

							}
						}

						// loop across 
					}

					/// ibrahim 14/12/16 end

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE
					{

						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
							{
								temp_constraint[bitStreamCounter] += vars_aux[bitStreamCounter][iter1->second];
							}
						}
					}

					//assert(!adder);

					if (adder)
					{
						for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
						{
							model.addConstr(temp_constraint[bitStreamCounter], GRB_LESS_EQUAL, LUTinputSize - 2);
						}
					}
						
					else
					{ 
						for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
						{
							model.addConstr(temp_constraint[bitStreamCounter], GRB_LESS_EQUAL, LUTinputSize - 1 - tempControl[bitStreamCounter]);
						}
					}

					//model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - 1 - 1);// tempControl);//control_signals); // ib 14/12/2016// todo make sure u are not messing this up for adders

				}
			}
		}


		////////////////////////////////////////////////////////////////////////////////////
		//////////// Constaints to fix offpath inputs when they are connected to LUTs///////
		////////////////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // if it's a reg then skip
						continue;

					// LUTs only
					if (fpgaLogic[i][j][k].utilization < 2) // if one or less (zero) paths are using this LUT then skip we will never need to fix an off-path input coz there aint
						continue;
					if (fpgaLogic[i][j][k].usedInputPorts < 2) // no off path inputs since only one port is used
						continue;

					// LUT used with more than one input port used
					for (int z = 0; z < InputPortSize; z++)
					{
						if (fpgaLogic[i][j][k].inputPorts[z]) // check if this port is used
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, z, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the z port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							if (feeederZ%LUTFreq == 0) // if the feeder is a lut then skip this port as we can fix this input port using the fix signal of the feeder LUT
								continue;

							// feeder is a reg
							///	if (!is_cascaded_reg(i, j, k)) // if its not a cascaded reg, then we can control it 
							//		continue;


							if (reg_free_input(feeederX, feeederY, feeederZ)) // if its input is free we can control this reg, so thats fine
								continue;

							// could be cascaded or a feed-back path

							int regFeederPath, regFeederNode;
							assert(get_feeder(feeederX, feeederY, feeederZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

							int regFeederX = paths[regFeederPath][regFeederNode].x;
							int regFeederY = paths[regFeederPath][regFeederNode].y;
							int regFeederZ = paths[regFeederPath][regFeederNode].z;

							bool shouldDelete = false;

							std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
							pathsToReg.resize(0);
							std::vector<int> pathsIJKandRegFeeder; // paths using lut i,j,k and using an input other than z and going through the lut feeding the reg
							pathsIJKandRegFeeder.resize(0);

							std::vector<int> pathsIJKZ; // paths using lut i,j,k and using an input Z
							pathsIJKZ.resize(0);

							for (int ii = 0; ii < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); ii++) // loop across all paths using the lut feeding the reg in question
							{
								Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[ii];
								if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
									continue;

								// check if this path feeds the reg
								if (paths[tempNode.path][tempNode.node + 1].x == feeederX && paths[tempNode.path][tempNode.node + 1].y == feeederY && paths[tempNode.path][tempNode.node + 1].z == feeederZ)
								{
									pathsToReg.push_back(tempNode.path);
								}


								// loop across nodes using LUT i,j,k to see all paths using lut i,j,k from an input other than port z
								for (int l = 0; l < (int)fpgaLogic[i][j][k].nodes.size(); l++)
								{
									if (paths[fpgaLogic[i][j][k].nodes[l].path][0].deleted)
										continue;


									if (ii == 0 && paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn == z)
									{
										pathsIJKZ.push_back(fpgaLogic[i][j][k].nodes[l].path);
									}

									if (paths[fpgaLogic[i][j][k].nodes[l].path][fpgaLogic[i][j][k].nodes[l].node].portIn != z) // found a path using lut i,j,k from a node other than Z
									{
										if (fpgaLogic[i][j][k].nodes[l].path == tempNode.path) // one of the found paths also use the LUt feeding the reg
										{
											pathsIJKandRegFeeder.push_back(fpgaLogic[i][j][k].nodes[l].path);
											shouldDelete = true;
										}


									}

								}

							}

							if (shouldDelete) // we should delte some stuff coz we cant fix all off-path inputs
							{



								// create constraints betweentwo sets. this is different than the re-convergent fanout as these two paths may have commpn paths in them.

								// variable representing set IJKZ
								std::vector< GRBVar> pathsIJKZSetILP;
								pathsIJKZSetILP.resize(bitStreams);
								for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
								{
									pathsIJKZSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
								}

								for (int setCounter = 0; setCounter <(int)pathsIJKZ.size(); setCounter++)
								{
									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										model.addConstr(pathsIJKZSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][pathsIJKZ[setCounter] - 1]); // model as ILP => Pi
									}
								}

							

								// create two variables to represent each set

								std::vector <GRBVar> pathsToRegSetILP;
								std::vector <GRBVar>  pathsIJKandRegFeederSetILP;
								pathsToRegSetILP.resize(bitStreams);
								pathsIJKandRegFeederSetILP.resize(bitStreams);

								for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
								{
									pathsToRegSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
									pathsIJKandRegFeederSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
								}

								for (int setCounter = 0; setCounter < (int)pathsToReg.size(); setCounter++)
								{
									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										model.addConstr(pathsToRegSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][pathsToReg[setCounter] - 1]); // model as ILP => Pi
									}
								}

								for (int setCounter = 0; setCounter <(int)pathsIJKandRegFeeder.size(); setCounter++)
								{
									for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
									{
										model.addConstr(pathsIJKandRegFeederSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][pathsIJKandRegFeeder[setCounter] - 1]); // model as ILP => Pi
									}
								}


								std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(z);
								//	std::cout << temp_key << std::endl;
								std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
								assert(iter1 != hashTable.end());
								// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];



								// now add constraint that the sume of these two variables must be less than or equal one
								for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
								{
									model.addConstr(pathsIJKandRegFeederSetILP[bitStreamCounter] + pathsToRegSetILP[bitStreamCounter] <= 2 - vars_aux[bitStreamCounter][iter1->second]);
								}

							}


						}
					}

				}
			}
		}


		/////////////////////////////////////////////////////////////////////////////
		////////////////// Constraints to check that the source reg. can toggle /////
		/////////////////////////////////////////////////////////////////////////////
	//	int pol = 0;

		for (int i = 1; i <(int)paths.size(); i++)
		{
			if (paths[i].size() < 1)
				continue;

			if (paths[i][0].deleted) // if its deleted
				continue;

			int sourceRegX = paths[i][0].x;
			int sourceRegY = paths[i][0].y;
			int sourceRegZ = paths[i][0].z;

			assert(sourceRegZ%LUTFreq != 0);// muts be reg

			if (paths[i][paths[i].size() - 1].x == sourceRegX && paths[i][paths[i].size() - 1].y == sourceRegY && paths[i][paths[i].size() - 1].z == sourceRegZ) // feedback path so ignore it for now
				continue;

			if (reg_free_input(sourceRegX, sourceRegY, sourceRegZ)) // if its input is free we can control this reg, so thats fine
				continue;

			// could be cascaded 

			int regFeederPath, regFeederNode;
			assert(get_feeder(sourceRegX, sourceRegY, sourceRegZ, regFeederPath, regFeederNode)); // get the feeder for this reg 

			int regFeederX = paths[regFeederPath][regFeederNode].x;
			int regFeederY = paths[regFeederPath][regFeederNode].y;
			int regFeederZ = paths[regFeederPath][regFeederNode].z;

			bool shouldDelete = false;
			std::vector<int> pathsToReg; // vector of paths using the lut feeding the reg and the reg
			pathsToReg.resize(0);

			for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); j++) // loop across this reg feeder and see if it has the same path as path i
			{


				Path_logic_component tempNode = fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j];
				if (paths[tempNode.path][0].deleted) // if this path is deleted, then continue
					continue;

				// check if this path feeds the reg
				if (paths[tempNode.path][tempNode.node + 1].x == sourceRegX && paths[tempNode.path][tempNode.node + 1].y == sourceRegY && paths[tempNode.path][tempNode.node + 1].z == sourceRegZ)
				{
					pathsToReg.push_back(tempNode.path);
				}



				if (fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[j].path == i)
				{
					//if (delete_path(i))
					//{
					//total++;
					shouldDelete = true;
					//break;
					//}
				}
			}

			if (shouldDelete) // path i uses the LUT feeding its source reg
			{
				std::vector< GRBVar > pathsToRegSetILP;
				pathsToRegSetILP.resize(bitStreams);
				//	GRBVar pathsIJKandRegFeederSetILP;

				for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
				{
					pathsToRegSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
				}
				
				//	pathsIJKandRegFeederSetILP = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);

				for (int setCounter = 0; setCounter <(int)pathsToReg.size(); setCounter++)
				{
					for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
					{
						model.addConstr(pathsToRegSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][pathsToReg[setCounter] - 1]); // model as ILP => Pi
					}
				}
				for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
				{
					model.addConstr(pathsToRegSetILP[bitStreamCounter] + vars[bitStreamCounter][i - 1] <= 1);
				}
			}
			else
			{
				// path i was not deleted as we can toggle the source reg. But now we will check can we toggle the source reg and at the same time keep all off path-inputs of path i fixed ?



				std::vector<GRBVar> pathsRegFeederSetILP;
				pathsRegFeederSetILP.resize(bitStreams);

				// ILp var representing all paths using reg feeder and connected to reg
				for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
				{
					pathsRegFeederSetILP[bitStreamCounter] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
				}


				for (int setCounter = 0; setCounter < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes.size(); setCounter++)
				{
					if (paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].x == sourceRegX && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].y == sourceRegY && paths[fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].node + 1].z == sourceRegZ)
					{
						for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
						{
							model.addConstr(pathsRegFeederSetILP[bitStreamCounter], GRB_GREATER_EQUAL, vars[bitStreamCounter][fpgaLogic[regFeederX][regFeederY][regFeederZ].nodes[setCounter].path - 1]); // model as ILP => Pi
						}
					}
				}

				int destinationX, destinationY, destinationZ, destinationPortIn;

				for (int j = 0; j < (int)fpgaLogic[regFeederX][regFeederY][regFeederZ].connections.size(); j++) // loop over all connections from regFeeder to its fanout
				{
					// get destination j
					destinationX = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationX;
					destinationY = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationY;
					destinationZ = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationZ;
					destinationPortIn = fpgaLogic[regFeederX][regFeederY][regFeederZ].connections[j].destinationPort;

					if (destinationX == -1) // deleted connection
						continue;

					if (destinationZ%LUTFreq != 0) // it's a reg then continue. I think.
						continue;

					for (int k = 0; k < (int)fpgaLogic[destinationX][destinationY][destinationZ].nodes.size(); k++)
					{
						if (fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].path == i) // then this destination is a lut that is also used by path i, we can not toggle the source of path i while keeping the off-path inputs at this LUT fixed.
						{

							assert(destinationPortIn != paths[i][fpgaLogic[destinationX][destinationY][destinationZ].nodes[k].node].portIn);

							std::string temp_key = std::to_string(destinationX) + "_" + std::to_string(destinationY) + "_" + std::to_string(destinationZ) + "__" + std::to_string(destinationPortIn);
							//	std::cout << temp_key << std::endl;
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];

							//					pol++;

							// now add constraint that the says either P1 can be tested or we can use Xij
							//	model.addConstr(vars[i - 1] + vars_aux[iter1->second] <= 1.7);
							for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
							{
								model.addConstr(vars[bitStreamCounter][i - 1] + pathsRegFeederSetILP[bitStreamCounter] <= 2 - vars_aux[bitStreamCounter][iter1->second]);
							}

						
						}
					}
				
				}


			}

		

		}
		



		/////////////////////////////////////////////////////////////////////////
		////// constraints to relate auxiliary variable with paths variables ////
		////// Pi = X_i_j_k_P1 & X_i_j_kk_P2 & ......... ////////////////////////
		/////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < (int)paths.size(); i++)
		{
			if (paths[i].size() < 3) // no LUTs
				continue;
			if (paths[i][0].deleted)
				continue;
			// ib14/12/16 GRBVar* temp_constr = new GRBVar[paths[i].size()-2];
			for (int j = 1; j < (int)paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
				//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				// ib14/12/16		temp_constr[j-1] = vars_aux[iter1->second];
				for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
				{
					model.addConstr(vars[bitStreamCounter][i - 1], GRB_LESS_EQUAL, vars_aux[bitStreamCounter][iter1->second]); // model as Pi<=Xij
				}

			}
			// ib14/12/16 model.addGenConstrAnd(vars[i-1], temp_constr, paths[i].size() - 2);
			// ib14/12/16 delete [] temp_constr;
		}

		

		///////////////////////////////////////////////////////////////////
		////////// B0Pi + B1Pi <=1
		///////////////////////////////////////////////////////
		GRBLinExpr temp_constraint = 0.0;
		for (int i = 0; i < num_of_paths; i++)
		{
			if (paths[i + 1][0].deleted)
				continue;
			temp_constraint = 0.0;
			for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
			{
				temp_constraint += vars[bitStreamCounter][i];
			//	model.addConstr(vars_1[i] + vars_2[i] + vars_3[i] <= 1);
			}
			model.addConstr(temp_constraint, GRB_LESS_EQUAL, 1.5);
		}

		


		////////////////////////////////
		/// set objective function /////
		////////////////////////////////

		/////////
		/////////
		
		/////////
		/////////
		GRBLinExpr obj = 0.0;
		double* coeff = new double[num_of_paths];
		std::cout << "number of bit streams " << bitStreams << std::endl;
		if (use_MC) // use the information we got from MC to test paths
		{
			assert((int)pathsImport.size() == num_of_paths + 1);

			for (int i = 0; i < num_of_paths; i++)
			{
				
				coeff[i] = pathsImport[i + 1];
				//(1.0 / (num_of_paths*num_of_paths))* (num_of_paths - i); //num_of_paths - 
			}



			for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
			{
			//	obj += vars[bitStreamCounter][i];
				obj.addTerms(coeff, vars[bitStreamCounter], num_of_paths);
			}
			

			
		}
		else
		{
	
			for (int i = 0; i < num_of_paths; i++)
			{
				if (paths[i + 1][0].deleted)
					continue;
				for (int bitStreamCounter = 0; bitStreamCounter < bitStreams; bitStreamCounter++)
				{
					obj += vars[bitStreamCounter][i];
				}
			}
		}
		model.setObjective(obj, GRB_MAXIMIZE);

		model.optimize();
		int total = 0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (vars[0][i].get(GRB_DoubleAttr_X) < 0.99999)
			{
				if (delete_path(i + 1))
					total++;

				if (vars[0][i].get(GRB_DoubleAttr_X) != 0.0)
				{
					if (abs(vars[0][i].get(GRB_DoubleAttr_X)) >= 0.00000000001)
						std::cout << i << " " << vars[0][i].get(GRB_DoubleAttr_X) << std::endl;
					assert(abs(vars[0][i].get(GRB_DoubleAttr_X)) < 0.00000000001);

				}
			}
			else
			{


				if (vars[0][i].get(GRB_DoubleAttr_X) <= 0.9999)
					std::cout << i << " " << vars[0][i].get(GRB_DoubleAttr_X) << std::endl;
				assert(vars[0][i].get(GRB_DoubleAttr_X) > 0.9999);
			}
			
					
		}


		for (int i = 0; i < bitStreams; i++)
		{
			for (int j = 0; j < num_of_paths; j++)
			{
				if (vars[i][j].get(GRB_DoubleAttr_X)>0.9) // tested
				{
					pathsSchedule[i].push_back(j + 1);
				}
			}
		}


		//	for (int i = 0; i < num_of_paths; i++)
		//		std::cout << vars[i].get(GRB_StringAttr_VarName) << std::endl;

		std::cout << "**********Number of deleted paths from ILP solver : " << total << " **********" << std::endl;
		IgnoredPathStats << total << "\t";

		////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////// House Cleaning ////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////

		delete[] lb_vars;
		delete[] up_vars;
		delete[] type_vars;
		delete[] names_vars;
		delete[] lb_vars_aux;
		delete[] up_vars_aux;
		delete[] type_vars_aux;
		delete[] names_vars_aux;
		delete[] coeff;

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}





}



// trying global optimumum
/*void ILP_solve_3()
{
	
	try {
		GRBEnv env = GRBEnv();

		GRBModel model = GRBModel(env);
		model.getEnv().set("TimeLimit", "1000.0");
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// ////////////////////////////////////////////////////////          create variables for the ILP       ////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////////////
		////// Path variables////////////////////
		/////////////////////////////////////////

		int num_of_paths = paths.size() - 1;
		double* lb_vars_1 = new double[num_of_paths];
		double* up_vars_1 = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_1 = new char[num_of_paths];
		string* names_vars_1 = new string[num_of_paths];


		double* lb_vars_2 = new double[num_of_paths];
		double* up_vars_2 = new double[num_of_paths];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_2 = new char[num_of_paths];
		string* names_vars_2 = new string[num_of_paths];


		for (int i = 0; i < num_of_paths; i++)
		{
			lb_vars_1[i] = 0.0;
			up_vars_1[i] = 1.0;
			type_vars_1[i] = GRB_BINARY;
			string temp = "B0P" + std::to_string(i + 1);
			names_vars_1[i] = temp;
			lb_vars_2[i] = 0.0;
			up_vars_2[i] = 1.0;
			type_vars_2[i] = GRB_BINARY;
			string temp_2 = "B1P" + std::to_string(i + 1);
			names_vars_2[i] = temp_2;
			//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_1 = model.addVars(lb_vars_1, up_vars_1, NULL, type_vars_1, names_vars_1, num_of_paths);
		GRBVar* vars_2 = model.addVars(lb_vars_1, up_vars_1, NULL, type_vars_1, names_vars_1, num_of_paths);
		GRBVar* vars_3 = model.addVars(lb_vars_2, up_vars_2, NULL, type_vars_2, names_vars_2, num_of_paths);

		/////////////////////////////////////////////////////////////////////
		//////// auxilliary variables input ports of LUTs //////////////////
		////////////////////////////////////////////////////////////////////

		std::unordered_map <std::string, int> hashTable;
		std::unordered_map <int, std::string> reverseHashTable;

		// build hash table for the new auxiliary variables
		int num_of_auxiliary_variables = 0;
		int hash_table_size = 0;
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (k%LUTFreq != 0) // not lut skip
						continue;

					if (fpgaLogic[i][j][k].utilization < 1) // not used skip
						continue;

					num_of_auxiliary_variables += fpgaLogic[i][j][k].usedInputPorts; // add used input port os this lut as they are variables;

					int double_check = 0;
					for (int l = 0; l < InputPortSize; l++)
					{
						if (fpgaLogic[i][j][k].inputPorts[l]) //port l is used
						{
							double_check++;
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 == hashTable.end()); // every entry is unique, we only enter it once, so double check that 

							hashTable.insert(make_pair(temp_key, hash_table_size));
							reverseHashTable.insert(make_pair(hash_table_size, temp_key));

							hash_table_size++;
						}
					}
					assert(double_check == fpgaLogic[i][j][k].usedInputPorts);
				}
			}
		}

		assert(hash_table_size == num_of_auxiliary_variables);

		/// now create the variables and add it to the model

		double* lb_vars_aux = new double[num_of_auxiliary_variables];
		double* up_vars_aux = new double[num_of_auxiliary_variables];
		// obj set to null, so the coe-effecients will be set to default values of zeros
		char* type_vars_aux = new char[num_of_auxiliary_variables];
		string* names_vars_aux = new string[num_of_auxiliary_variables];



		for (int i = 0; i < num_of_auxiliary_variables; i++)
		{
			lb_vars_aux[i] = 0.0;
			up_vars_aux[i] = 1.0;
			type_vars_aux[i] = GRB_BINARY;
			std::unordered_map < int, std::string>::iterator iter2 = reverseHashTable.find(i); // representative variable names
			assert(iter2 != reverseHashTable.end());// this element must be there
			string temp = "X_" + iter2->second;
			names_vars_aux[i] = temp;
			//	std::cout << names_vars_aux[i] << " ";
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_aux = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);
		GRBVar* vars_aux_2 = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);
		GRBVar* vars_aux_3 = model.addVars(lb_vars_aux, up_vars_aux, NULL, type_vars_aux, names_vars_aux, num_of_auxiliary_variables);



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////          create constraints for the ILP      //////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////
		///////////// set deleted path to 0//////////////////////////////////
		/////////////////////////////////////////////////////////////////////

		for (int i = 1; i < paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars_1[i - 1], GRB_EQUAL, 0.0);
				model.addConstr(vars_2[i - 1], GRB_EQUAL, 0.0);
				model.addConstr(vars_3[i - 1], GRB_EQUAL, 0.0);


			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= 1 //////////
		//////////////////////////////////////////////////////////////////////


		int tot_cons = 0;
		 
		for (int i = 0; i < FPGAsizeX; i++)
		{
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;


					std::vector<std::vector<int> > paths_sets;// 2d-vector to store all paths using pin x
					paths_sets.resize(InputPortSize);

					for (int l = 0; l < fpgaLogic[i][j][k].nodes.size(); l++)
					{
						Path_logic_component temp_node = fpgaLogic[i][j][k].nodes[l];
						if (paths[temp_node.path][0].deleted)
							continue;
						paths_sets[paths[temp_node.path][temp_node.node].portIn].push_back(temp_node.path); // add this node to the list of paths using paths[temp_node.path][temp_node.node].portIn
					}
					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE, I am checkin if I can fix input L and test LUT using other inputs
					{
						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							int pathFeeder, nodeFeeder;
							assert(get_feeder(i, j, k, l, pathFeeder, nodeFeeder)); // get the feeder for this LUT using the l port

							int feeederX = paths[pathFeeder][nodeFeeder].x;
							int feeederY = paths[pathFeeder][nodeFeeder].y;
							int feeederZ = paths[pathFeeder][nodeFeeder].z;

							std::vector<int> feeder_paths; // paths using the feeder of LUT i,j,k port l

							for (int m = 0; m < fpgaLogic[feeederX][feeederY][feeederZ].nodes.size(); m++) // loop across paths using this LUT
							{
								Path_logic_component temp_node = fpgaLogic[feeederX][feeederY][feeederZ].nodes[m];
								if (paths[temp_node.path][0].deleted) // if deleted ignore it
									continue;
								if (feeederZ%LUTFreq != 0) // reg
								{
									if (temp_node.node != 0) // if this is not a source then skip this path. It means that the connection between this reg and LUT i,j,k does not include path tempNode.path, we take care of this in fix __ off path input function. TRIAL
										continue;
								}
								feeder_paths.push_back(temp_node.path);

							}

							std::vector<int> conflict_ports;
							// loopo across paths_sets and make sure there is no common path between feeder_paths and other paths using 
							bool extra_check = false;

							for (int m = 0; m < paths_sets.size(); m++)
							{
								if (m == l) // current port, that we are checking if it can be fixed
									continue;

								conflict_ports.resize(0);
								for (int mm = 0; mm < paths_sets[m].size(); mm++)
								{
									for (int counter = 0; counter < feeder_paths.size(); counter++)
									{
										if (feeder_paths[counter] == paths_sets[m][mm]) // then port l and port m can not be tested together, because the feeder of port l also connected to another port
										{
											conflict_ports.push_back(m);
										}
									}

								}
								if (conflict_ports.size() > 0)
								{
									//						assert(!extra_check); // check that this happens only once for each port
									extra_check = true;
									/// port l
								add_constraint_recon: std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
									std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
									assert(iter1 != hashTable.end());

									/// port conflict_ports[0]
									temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(conflict_ports[0]);
									std::unordered_map <std::string, int>::iterator iter2 = hashTable.find(temp_key);
									assert(iter2 != hashTable.end());

									// add contraint
									model.addConstr(vars_aux[iter1->second] + vars_aux[iter2->second] <= 1);
									model.addConstr(vars_aux_2[iter1->second] + vars_aux_2[iter2->second] <= 1);
									model.addConstr(vars_aux_3[iter1->second] + vars_aux_3[iter2->second] <= 1);
									tot_cons++;

								}

							}

						}
					}
				}
			}
		}
		std::cout << " reconv fan out " << tot_cons << std::endl;
		tot_cons = 0;
		
		//////////////////////////////////////////////////////////////////////
		////// constraints over auxiliary var/////////////////////////////////
		/////// number of inputs constraint///////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////
		//	std::cout << std::endl << "number of constraints" << deletedPaths << std::endl;

		for (int i = 0; i < FPGAsizeX; i++)
		{
			//std::cout << "iteration" << i << std::endl;
			for (int j = 0; j < FPGAsizeY; j++)
			{
				for (int k = 0; k < FPGAsizeZ; k++)
				{
					if (fpgaLogic[i][j][k].utilization < 1) // if le not used, go to the next one
						continue;

					if (k%LUTFreq != 0) // if its a lut skip it
						continue;

					int control_signals = 1;
					if (check_control_signal_required(i, j, k))
						control_signals++;
					else // todo : should be considered further, possible better solution to handle cout
					{
						if (fpgaLogic[i][j][k].outputPorts[Cout - 5]) // this ensures that the LUT is i arith mode, we can only test paths using maximum 2 input ports, I think this could be relaxed a bit
						{
							control_signals++; // todo:deal with this
						}
					}


					if (fpgaLogic[i][j][k].usedInputPorts + control_signals <= LUTinputSize) // if inputs are enough and no need to delete then continue;
						continue;

					assert(k%LUTFreq == 0); // check that it's a LUT

					GRBLinExpr temp_constraint = 0.0;
					GRBLinExpr temp_constraint_2 = 0.0;
					GRBLinExpr temp_constraint_3 = 0.0;

					for (int l = 0; l < InputPortSize; l++) // loop across all inputs using this LE
					{

						if (fpgaLogic[i][j][k].inputPorts[l])
						{
							std::string temp_key = std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k) + "__" + std::to_string(l);
							std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
							assert(iter1 != hashTable.end());
							temp_constraint += vars_aux[iter1->second];
							temp_constraint_2 += vars_aux_2[iter1->second];
							temp_constraint_3 += vars_aux_3[iter1->second];
						}
					}

					model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - control_signals); // todo make sure u are not messing this up for adders
					model.addConstr(temp_constraint_2, GRB_LESS_EQUAL, LUTinputSize - control_signals); // todo make sure u are not messing this up for adders
					model.addConstr(temp_constraint_3, GRB_LESS_EQUAL, LUTinputSize - control_signals); // todo make sure u are not messing this up for adders
					tot_cons++;

				}
			}
		}
		std::cout << " input constr " << tot_cons << std::endl;
		tot_cons = 0;

		/////////////////////////////////////////////////////////////////////////
		////// constraints to relate auxiliary variable with paths variables ////
		////// Pi = X_i_j_k_P1 & X_i_j_kk_P2 & ......... ////////////////////////
		/////////////////////////////////////////////////////////////////////////

		for (int i = 0; i < paths.size(); i++)
		{
			if (paths[i].size() < 3) // no LUTs
				continue;
			if (paths[i][0].deleted)
				continue;
			GRBVar* temp_constr = new GRBVar[paths[i].size() - 2];
			GRBVar* temp_constr_2 = new GRBVar[paths[i].size() - 2];
			GRBVar* temp_constr_3 = new GRBVar[paths[i].size() - 2];
			for (int j = 1; j < paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
				//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				temp_constr[j - 1] = vars_aux[iter1->second];
				temp_constr_2[j - 1] = vars_aux_2[iter1->second];
				temp_constr_3[j - 1] = vars_aux_3[iter1->second];

			}
			model.addGenConstrAnd(vars_1[i - 1], temp_constr, paths[i].size() - 2);
			model.addGenConstrAnd(vars_2[i - 1], temp_constr_2, paths[i].size() - 2);
			model.addGenConstrAnd(vars_3[i - 1], temp_constr_3, paths[i].size() - 2);
			delete[] temp_constr;
		}



		///////////////////////////////////////////////////////////////////
		////////// B0Pi + B1Pi <=1
		///////////////////////////////////////////////////////
		for (int i = 0; i < num_of_paths; i++)
		{
			if (paths[i + 1][0].deleted)
				continue;
			model.addConstr(vars_1[i] + vars_2[i] + vars_3[i] <= 1);
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (paths[i + 1][0].deleted)
				continue;
			obj += vars_1[i] +vars_2[i] + vars_3[i];
		}

		model.setObjective(obj, GRB_MAXIMIZE);

		model.optimize();


		int total = 0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (vars_1[i].get(GRB_DoubleAttr_X) < 0.99999 && vars_2[i].get(GRB_DoubleAttr_X) < 0.99999) // not tested in the first nor the second bit-stream
			{
				std::cout << "path " << i << "not tested in both ebit streams" << std::endl;
				if (delete_path(i + 1))
					total++;
				if (vars_1[i].get(GRB_DoubleAttr_X) != 0.0)
				{
				//	if (abs(vars_1[i].get(GRB_DoubleAttr_X)) >= 0.00000000001)
				//		std::cout << i << " fuck it  " << vars_1[i].get(GRB_DoubleAttr_X) << std::endl;
					assert(abs(vars_1[i].get(GRB_DoubleAttr_X)) < 0.00000000001);

				}

				if (vars_2[i].get(GRB_DoubleAttr_X) != 0.0)
				{
				//	if (abs(vars_2[i].get(GRB_DoubleAttr_X)) >= 0.00000000001)
				//		std::cout << i << " fuck it " << vars_2[i].get(GRB_DoubleAttr_X) << std::endl;
					assert(abs(vars_2[i].get(GRB_DoubleAttr_X)) < 0.00000000001);

				}
			}
			else
			{
			//	if (vars_1[i].get(GRB_DoubleAttr_X) <= 0.9999)
			//		std::cout << i << " " << vars_1[i].get(GRB_DoubleAttr_X) << std::endl;

				if (vars_1[i].get(GRB_DoubleAttr_X) < 0.99999)
					delete_path(i + 1);

				assert(vars_1[i].get(GRB_DoubleAttr_X) > 0.9999 || vars_2[i].get(GRB_DoubleAttr_X) > 0.9999);
			}
			std::cout << vars_1[i].get(GRB_StringAttr_VarName) << " "
				<< vars_1[i].get(GRB_DoubleAttr_X) << std::endl;
			std::cout << vars_2[i].get(GRB_StringAttr_VarName) << " "
				<< vars_2[i].get(GRB_DoubleAttr_X) << std::endl;
		}
		std::cout << "**********Number of deleted paths from ILP solver : " << total << " **********" << std::endl;
		IgnoredPathStats << total << "\t";

		////////////////////////////////////////////////////////////////////////////////////////
		//////////////////////////////// House Cleaning ////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////

		delete[] lb_vars_1;
		delete[] up_vars_1;
		delete[] type_vars_1;
		delete[] names_vars_1;
		delete[] lb_vars_aux;
		delete[] up_vars_aux;
		delete[] type_vars_aux;
		delete[] names_vars_aux;

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}


}*/

#endif