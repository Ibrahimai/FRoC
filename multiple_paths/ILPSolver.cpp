
#define checksetr 

#ifdef checksetr
#include "globalVar.h"
//#include "completeNetlist.h"
#include "ILPSolver.h"
#include "util.h"
#include "gurobi_c++.h"
#include <unordered_map>


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


// maximize number of paths/ bitstream, using auxilliary variables to represent LUT inputs and use these variables to create constraints for LUT inputs and re-convergent fanout
void ILP_solve_2()
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

		for (int i = 1; i < paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars[i - 1], GRB_EQUAL, 0.0);

			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////


		
		// /*
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
								for (int mm = 0; mm < paths_sets[m].size();mm++)
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
					
					model.addConstr(temp_constraint, GRB_LESS_EQUAL, LUTinputSize - control_signals); // todo make sure u are not messing this up for adders

				}
			}
		}


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
			GRBVar* temp_constr = new GRBVar[paths[i].size()-2];
			for (int j = 1; j < paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
			//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				temp_constr[j-1] = vars_aux[iter1->second];

			}
			model.addGenConstrAnd(vars[i-1], temp_constr, paths[i].size() - 2);
			delete [] temp_constr;
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (paths[i + 1][0].deleted)
				continue;
			obj += vars[i];
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

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}





}



int number_of_edges_cacsaded_paths(std::map<std::string, std::vector<int> >  timingEdgeToPaths, bool strictEdgeCounting , std::map<std::string, double>  timingEdgesMapComplete) // strictEdgeCounting means we only count this edge if it is tested through the longest critical path
{
	int total = 0;
	bool flag = true;
	for (auto iter = timingEdgeToPaths.begin(); iter != timingEdgeToPaths.end(); iter++) // loop across all edges
	{
		// check if this edge is tested
		flag = true;
		std::vector <int> tempPaths = iter->second; // get the equivalent edge from the complete timing net list
		assert(tempPaths.size() > 0);
		if (!strictEdgeCounting)
		{

			for (int i = 0; i < tempPaths.size(); i++)
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

			for (int i = 0; i < tempPaths.size(); i++)
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
void ILP_solve_max_timing_edges(std::map<std::string, double>  testedTimingEdgesMap , std::map<std::string, std::vector<int> >  timingEdgeToPaths, std::map<std::string, double>  timingEdgesMapComplete , bool strictEdgeCounting, bool cascadedRegion)
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

		for (int i = 0; i < timingEdgesMapComplete.size(); i++)
		{
			lb_vars_edges[i] = 0.0;
			up_vars_edges[i] = 1.0;
			type_vars_edges[i] = GRB_BINARY;
			string temp = "E" + std::to_string(i);
			names_vars_edges[i] = temp;
			//	std::cout << names_vars[i] << std::endl;
			//std::cout << temp << std::endl;
		}

		GRBVar* vars_edges = model.addVars(lb_vars_edges, up_vars_edges, NULL, type_vars_edges, names_vars_edges, timingEdgesMapComplete.size());

		

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
			std::vector<int> paths_using_edge = iter_edge_paths->second;

			// add all paths variable to be ored later
			GRBVar* temp_constr = new GRBVar[paths_using_edge.size()];
			int number_of_paths = 0;
			for (int j = 0; j < paths_using_edge.size(); j++)
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

		for (int i = 1; i < paths.size(); i++)
		{
			if (paths[i][0].deleted) // path is deleted
			{
				model.addConstr(vars[i - 1], GRB_EQUAL, 0.0);

			}

		}



		//////////////////////////////////////////////////////////////////////
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////



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
			for (int j = 1; j < paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
				//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				temp_constr[j - 1] = vars_aux[iter1->second];

			}
			model.addGenConstrAnd(vars[i - 1], temp_constr, paths[i].size() - 2);
			delete[] temp_constr;
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;

		int remaining_deges = timingEdgesMapComplete.size() - total_tested_timing_edges;
		std::cout << " remaining_edges " << remaining_deges << " cascaeded deges " << number_of_edges_cacsaded_paths(timingEdgeToPaths,  strictEdgeCounting, timingEdgesMapComplete) << std::endl;
		
	//	bool more_Edges_to_test;
	//	if (!strictEdgeCounting)
	//		more_Edges_to_test = total_tested_timing_edges < timingEdgesMapComplete.size();
	//	else

		std::cout << "total tested timing edges = " << total_tested_timing_edges << std::endl;
		if (total_tested_timing_edges<timingEdgesMapComplete.size()&&(cascadedRegion || (remaining_deges>number_of_edges_cacsaded_paths(  timingEdgeToPaths, strictEdgeCounting, timingEdgesMapComplete)))) // if we still ahve timing edges to test, then our objective function is to maximize number of edges, other wise maximze number of paths
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

	}
	catch (GRBException e) {
		std::cout << "Error code = " << e.getErrorCode() << std::endl;
		std::cout << e.getMessage() << std::endl;
	}
	catch (...) {
		std::cout << "Exception during optimization" << std::endl;
	}





}



void ILP_solve_max_paths_per_x_bit_stream(int bitStreams)
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
		////// constraints over auxilliary variables//////////////////////////
		/////// special re-convergent fan-out/////////////////////////////////
		////// X_i_j_k_P1 + X_i_j_k_P2 <= LUTinputs - controlSignals//////////
		//////////////////////////////////////////////////////////////////////



		// /*
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
			for (int j = 1; j < paths[i].size() - 1; j++) // loop through all nodes in path i, execluding the source ans sink register
			{
				std::string temp_key = std::to_string(paths[i][j].x) + "_" + std::to_string(paths[i][j].y) + "_" + std::to_string(paths[i][j].z) + "__" + std::to_string(paths[i][j].portIn);
				//	std::cout << temp_key << std::endl;
				std::unordered_map <std::string, int>::iterator iter1 = hashTable.find(temp_key);
				assert(iter1 != hashTable.end());
				temp_constr[j - 1] = vars_aux[iter1->second];

			}
			model.addGenConstrAnd(vars[i - 1], temp_constr, paths[i].size() - 2);
			delete[] temp_constr;
		}

		////////////////////////////////
		/// set objective function /////
		////////////////////////////////
		GRBLinExpr obj = 0.0;

		for (int i = 0; i < num_of_paths; i++)
		{
			if (paths[i + 1][0].deleted)
				continue;
			obj += vars[i];
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
				if (vars[i].get(GRB_DoubleAttr_X) <= 0.9999)
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
void ILP_solve_3()
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


}

#endif