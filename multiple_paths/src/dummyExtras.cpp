#include "dummyExtras.h"
#include "globalVar.h"
#include "util.h"

#ifdef StratixV 
bool need_to_add_dummy(std::string target);


void add_dummy_cell(int i, int j, int k, std::ofstream& dummyFile, std::ofstream& dummyLocationFile, bool MLAB);

void add_redundant_LUT() // this function check ALMs that are half full (one LUT is used) and that connects to the LUT through port D, in this case to ensure that port D is actually used we reserve the free LUT to prevent the tool from ignoring the routing constraint and using port E instead of D
{
	std::ofstream dummyFile;
	dummyFile.open("dummyFile.txt");
	std::ofstream dummyLocationFile;
	dummyLocationFile.open("dummyLocationFile.txt");
	int topZ, bottomZ;
	int dummyCells = 0;

	std::string blockInput;

	int xFeeder, yFeeder, zFeeder;
	bool flagD = false;

	int exception = 0;

	std::vector <int> tempArray;
	tempArray.resize(3);

	std::vector <std::vector<int> > tempDummyCells;

	int feederPath, feederNode;

	for (int i = 0; i < FPGAsizeX; i++)
	{
		for (int j = 0; j < FPGAsizeY; j++)
		{
			for (int k = 0; k < FPGAsizeZ / ALUTtoALM; k++)
			{
				topZ = k *ALUTtoALM;
				bottomZ = topZ + LUTFreq;
				if (fpgaLogic[i][j][topZ].utilization>0 && fpgaLogic[i][j][bottomZ].utilization>0) // both LUTs are utilized so skip this man
					continue;

				if (fpgaLogic[i][j][topZ].utilization>0) // only the top is utilized
				{
					assert(fpgaLogic[i][j][bottomZ].utilization == 0); // double check

					if (fpgaLogic[i][j][topZ].usedInputPorts < 4 || (fpgaLogic[i][j][topZ].usedInputPorts==4 && !check_control_signal_required(i,j,topZ)))// ensure that it uses less than 6 inputs ( I am assuming worst case control signals coz lets face it)
					{
						if (fpgaLogic[i][j][topZ].inputPorts[portD]) // port D is used
						{
							// check if the block inpout mux has an odd index value, in this case dont add a dummy as this pin can not be shared with port E
							flagD = false;
							if (i == 117 && j == 48 && topZ == 30)
								std::cout << "deubazitaion" << std::endl; 

							// must find which LUT feeds port D of cell i,j, topZ
							assert(get_feeder( i,  j, topZ,  portD, feederPath,  feederNode));
							xFeeder = paths[feederPath][feederNode].x;
							yFeeder = paths[feederPath][feederNode].y;
							zFeeder = paths[feederPath][feederNode].z;

							// find routing connection from feeder to the current cell
							for (int x = 0; x < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); x++)
							{
								if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationX == i && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationY == j && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationZ == topZ && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationPort == portD)
								{
									assert(fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size()>1);
									blockInput = fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources[fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size() - 1];
									assert(blockInput[0] == 'B');
									if ((blockInput[blockInput.size() - 1] - '0') % 2 == 0) // if it is even
									{
										if (need_to_add_dummy(blockInput))
											flagD = true;
										else
											flagD = false;
									}
									else
										flagD = false;

									break;
								}
							}

							if (flagD)
							{ 
								add_dummy_cell(i, j, bottomZ, dummyFile, dummyLocationFile, fpgaLogic[i][j][topZ].mLAB);
								dummyCells++;
	
								if (fpgaLogic[i][j][topZ].inputPorts[4])
									exception++;
	
								tempArray[0] = i;
								tempArray[1] = j;
								tempArray[2] = bottomZ;

								tempDummyCells.push_back(tempArray);
							}
						}

					}
					else
					{
						std::cout << "usign 6 input so can not add dummy cell" << std::endl;//(true);
					}
				}
				else // top LUT not utilized
				{
					if (fpgaLogic[i][j][bottomZ].utilization>0) // only the botom is utilized
					{
						assert(fpgaLogic[i][j][topZ].utilization == 0); // double check
				//		if (i == 131 && j == 56 && bottomZ ==27)
				//			std::cout << "deubazitaion" << std::endl;

						if (fpgaLogic[i][j][bottomZ].usedInputPorts < 4 || (fpgaLogic[i][j][bottomZ].usedInputPorts == 4 && !check_control_signal_required(i, j, bottomZ)))// ensure that it uses less than 6 inputs
						{
							if (fpgaLogic[i][j][bottomZ].inputPorts[3]) // port D is used
							{

								// check if the block inpout mux has an odd index value, in this case dont add a dummy as this pin can not be shared with port E
								flagD = false;
								// must find which LUT feeds port D of cell i,j, bottomZ
								assert(get_feeder(i, j, bottomZ, portD, feederPath, feederNode));
								xFeeder = paths[feederPath][feederNode].x;
								yFeeder = paths[feederPath][feederNode].y;
								zFeeder = paths[feederPath][feederNode].z;

								// find routing connection from feeder to the current cell
								for (int x = 0; x < (int)fpgaLogic[xFeeder][yFeeder][zFeeder].connections.size(); x++)
								{
									if (fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationX == i && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationY == j && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationZ == bottomZ && fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].destinationPort == portD)
									{
										assert(fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size()>1);
										blockInput = fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources[fpgaLogic[xFeeder][yFeeder][zFeeder].connections[x].usedRoutingResources.size() - 1];
										assert(blockInput[0] == 'B');
										if ((blockInput[blockInput.size() - 1] - '0') % 2 == 0)
										{
											if (need_to_add_dummy(blockInput))
												flagD = true;
											else
												flagD = false;
										}
										else
											flagD = false;

										break;
									}
								}

								if (flagD)
								{ 
									add_dummy_cell(i, j, topZ, dummyFile, dummyLocationFile, fpgaLogic[i][j][bottomZ].mLAB);
									dummyCells++;
	
									if (fpgaLogic[i][j][bottomZ].inputPorts[4])
										exception++;
	
									tempArray[0] = i;
									tempArray[1] = j;
									tempArray[2] = topZ;
	
									tempDummyCells.push_back(tempArray);
								}
							}

						}
						else
						{
							std::cout << "usign 6 input so can not add dummy cell" << std::endl;//(true);
						}
					}

				}
			}
		}
	}

	if (tempDummyCells.size()>0)
	{ 
		dummyFile << "wire dummyAll /*synthesis keep*/;" << std::endl;
		dummyFile << "assign dummyAll = 1'b1 ";
	}

	for (int i = 0; i < (int)tempDummyCells.size(); i++)
	{
		dummyFile << "&  dummy_" << tempDummyCells[i][0] << "_" << tempDummyCells[i][1] << "_" << tempDummyCells[i][2] << " ";
	}

	if (tempDummyCells.size()>0)
	{
		dummyFile << " ; " << std::endl;
	}


	return;
}


void add_dummy_cell(int i, int j, int k, std::ofstream& dummyFile, std::ofstream& dummyLocationFile, bool MLAB)
{
	assert(fpgaLogic[i][j][k].utilization == 0);


	dummyFile << "wire dummy_" << i << "_" << j << "_" << k << " ;" << std::endl;// "/*synthesis keep*/;" << std::endl; // try it out without synthesis keep


	// add the lcell, it is just a buffer
	dummyFile << "stratixv_lcell_comb dummy_" << i << "_" << j << "_" << k << "_t (" << std::endl;
	dummyFile << "\t.dataa(dummyIn1_gnd)," << std::endl;
	dummyFile << "\t.combout(" << " dummy_" << i << "_" << j << "_" << k << " ));" << std::endl;
	dummyFile << "defparam dummy_" << i << "_" << j << "_" << k << "_t .shared_arith = \"off\";" << std::endl;
	dummyFile << "defparam dummy_" << i << "_" << j << "_" << k << "_t .extended_lut = \"off\";" << std::endl;
	dummyFile << "defparam dummy_" << i << "_" << j << "_" << k << "_t .lut_mask = 64'hAAAAAAAAAAAAAAAA;" << std::endl << std::endl;
	//dummyFile << "64'hAAAAAAAAAAAAAAAA"

	// add constraint to location constraint file
	std::ofstream LoFile;
	LoFile.open(locationConsraintFile,std::ofstream::app);

	LoFile << "set_location_assignment ";
	if (MLAB)
		LoFile << "M";
	LoFile << "LABCELL_X" << i << "_Y" << j << "_N" << k << " -to dummy_" << i << "_" << j << "_" << k << "_t" << std::endl;


		//PATH" << fpgaLogic[i][j][k].owner.path /*path*/ << "NODE" << fpgaLogic[i][j][k].owner.node /*node*/ << "_t" << std::endl;






}

bool need_to_add_dummy(std::string target)
{
	int i;
	for (i = target.size() - 1; i > 0; i--)
	{
		if (target[i] == 'I')
			break;
	}

	int subIndex = stoi(target.substr(i+1));

	if (subIndex % 4 != 0) // not divisable by 4
		return true;

	if ((subIndex / 4) % 2 == 1)
		return false;
	else
		return true;
}
#endif