%{
#define portA (0)
#define portB (1)
#define portC  (2)
#define portD (3)
#define portE (4)
#define portF (5)
#define Cin (6)
#define Cout (2)
#define Combout (0)
#define normal (0)
#define arith (1)
int num_of_elem =0;
int FLAG = 0;
int cellFlag = 0;
int startRoute =0;
int path =0;
int node =0;
FILE *loCon;
FILE *roCon;
FILE *veri;
FILE *auxil;
FILE *meta;
FILE *metaRouting;
int i =0;
int routingBefore = 0;
int port = 999;
int portOut = 999;
int currentMode = normal;
int testVectorIndex = 0;
char *testVec;
int carryChainNum = 0;
int counter = 0;
%}




%%
"; "("R"|"F")("R "|"F ")"; CELL ;"[0-9 \t]+";"[A-Z0-9_ \t]+(("; High Speed"[ \t]+)|("; Low Power"[ \t]+))"; "[^; \t]+ {FLAG =1;startRoute=1;
					int index = 0; //"; High Speed ;"[^; \t]+
					cellFlag = 1;
					for (i=13;i<strlen(yytext);i++)
					{
						if (yytext[i]==';')
						{	
						index = i+2;
						break;	
						}
					}
				///// test test test
					//fprintf(loCon,"set_location_assignment %s -to \"PATH%dNODE%d\"\n",yytext,path,node);
					fprintf(loCon,"set_location_assignment ");
					fprintf(meta,"set_location_assignment ");
					//	printf(yytext);
					//	printf("\n");
					//	printf("%c",yytext[strlen(yytext)-1]);
					//	printf("\n");
					for (i=index;i<strlen(yytext);i++)
					{
							if (yytext[i]==' ')
								break;
						fprintf (loCon,"%c",yytext[i]);
						fprintf (meta,"%c",yytext[i]);
						

					}
					int indexPower = i;
					fprintf (loCon," -to \"PATH%dNODE%d\"\n", path,node);
					fprintf (meta," -to \"PATH%dNODE%d\"\n", path,node);
					//printf (" -to \"PATH%dNODE%d\"\n", path,node); //debug
					if (yytext[index]=='F')
					{
						if (node > 0)
						{
							fprintf(veri,"dffeas PATH%dNODE%d_t (\n\t.clk(CLK),\n\t.d(PATH%dNODE%d),\n\t.q(PATH%dNODE%d));\ndefparam PATH%dNODE%d_t .power_up = \"low\";\ndefparam PATH%dNODE%d_t .is_wysiwyg = \"true\";\n\n",path,node,path,node-1,path,node,path,node,path,node);
						
						}						
						else
						{
							fprintf(veri,"dffeas PATH%dNODE%d_t (\n\t.clk(CLK),\n\t.d(xin),\n\t.q(PATH%dNODE%d));\ndefparam PATH%dNODE%d_t .power_up = \"low\";\ndefparam PATH%dNODE%d_t .is_wysiwyg = \"true\";\n\n",path,node,path,node,path,node,path,node);	
											
						}
						fprintf(meta,"FF d q\n");
						if (yytext[2]==yytext[3]) // same transition
							fprintf(meta,"Non-Inv\n");
						else
							fprintf(meta,"Inv\n");
						goto done1;
					}


					int index2 = 0;
					for (i=0;i<strlen(yytext);i++)
					{
						if (yytext[i]=='|')
							index2 =i;
					}

					if (yytext[index2+3] == 'm')
						portOut = Combout;
					else if (yytext[index2+3] == 'u')
						portOut = Cout;
					else
						printf("error the output is not recognised\n");
		
							
					fprintf(meta,"LUT %d %d\n", port, portOut);
					// write the non/inverting behaviour of the signal to the meta file
					if (yytext[2]==yytext[3]) // same transition
							fprintf(meta,"Non-Inv\n");
						else
							fprintf(meta,"Inv\n");
					
					if (port < Cin)
					{

						if (portOut == Combout) ////////////// portin is not Cin portOut is Combout very common 
						{
							switch (port)
							{
								case portA :
									fprintf(veri,".dataa(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,testVectorIndex);
									break;
								case portB :
									fprintf(veri,".datab(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,testVectorIndex);
									break;
								case portC :
									fprintf(veri,".datac(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,testVectorIndex);
									break;
								case portD :
									fprintf(veri,".datac(testVector%d[%d]),\n\t.datad(PATH%dNODE%d),\n\t",path,testVectorIndex,path,node-1);
									break;
									default : break ;
							}
							testVectorIndex++;	
								//	printf (" -to FLAAG 1 \"PATH%dNODE%d\", testvecIndex%d\n", path,node,testVectorIndex); //debug
								//	if (testVec == NULL)
							//	printf("eeeerrroooor\n");
							testVec = (char *)realloc(testVec,sizeof(char)*testVectorIndex);
							if (testVec == NULL)
								printf("eeeerrroooor\n");

						//	printf (" -to FLAAG 1 \"PATH%dNODE%d\", testvecIndex%d\n", path,node,testVectorIndex); //debug
//
							
							
							if (yytext[2]==yytext[3]) // same transition
								testVec[testVectorIndex-1] = '0';
							else
								testVec[testVectorIndex-1] = '1';
							
							
							
							fprintf(veri,".combout(PATH%dNODE%d));\n",path,node);
							fprintf(veri,"defparam PATH%dNODE%d_t .sumlutc_input = \"datac\";\n",path,node);
							fprintf(veri,"defparam PATH%dNODE%d_t .lut_mask = ",path,node);
							if (port == portA)
								fprintf(veri,"\"55AA\";\n\n");
							if (port == portB)
								fprintf(veri,"\"33CC\";\n\n");
							if (port == portC)
								fprintf(veri,"\"0FF0\";\n\n");
							if (port == portD)
								fprintf(veri,"\"0FF0\";\n\n");	
						


						}
						else if (portOut = Cout) // pass incoming value to cout,////////////// portin is not Cin portOut is Cout not common, only at the beginning of a carry chain
						{

							switch (port)
							{
								case portA :
									fprintf(veri,".dataa(PATH%dNODE%d),\n\t.datad(vcc),\n\t",path,node-1);
									break;
								case portB :
									fprintf(veri,".datab(PATH%dNODE%d),\n\t.datad(vcc),\n\t",path,node-1);
									break;
								case portC :
									fprintf(veri,".datac(PATH%dNODE%d),\n\t.datad(vcc),\n\t",path,node-1);
									break;
								default : break ;
							}	
						fprintf(veri,".cout(PATH%dNODE%d));\n",path,node);
						fprintf(veri,"defparam PATH%dNODE%d_t .sumlutc_input = \"datac\";\n",path,node);
						fprintf(veri,"defparam PATH%dNODE%d_t .lut_mask = ",path,node);
						if (port == portA)  // as is cout follows the input port we can negate it by adjustin the mask
							if (yytext[2]==yytext[3])
								fprintf(veri,"\"00AA\";\n\n");
							else
								fprintf(veri,"\"0055\";\n\n");
						if (port == portB)
							if (yytext[2]==yytext[3])
								fprintf(veri,"\"00CC\";\n\n");
							else
								fprintf(veri,"\"0033\";\n\n");
						if (port == portC)
							if (yytext[2]==yytext[3])
								fprintf(veri,"\"00F0\";\n\n");
							else
								fprintf(veri,"\"000F\";\n\n");
						}
					}
					else if(port == Cin)
					{
						if (portOut == Cout)
						{
							fprintf(veri,".dataa(vcc),\n\t.datab(gnd),\n\t.datad(vcc),\n\t.cin(PATH%dNODE%d),\n\t.cout(PATH%dNODE%d));\n",path,node-1,path,node);
						}
						else if (portOut == Combout)
						{	
							
							if (carryChainNum%2==0)
							{
								if (yytext[2]==yytext[3])
									fprintf(veri,".dataa(gnd),\n\t.datab(gnd),\n\t.datad(vcc),\n\t.cin(PATH%dNODE%d),\n\t.combout(PATH%dNODE%d));\n",path,node-1,path,node);
								else
									fprintf(veri,".dataa(vcc),\n\t.datab(gnd),\n\t.datad(vcc),\n\t.cin(PATH%dNODE%d),\n\t.combout(PATH%dNODE%d));\n",path,node-1,path,node);	 
							}
							else
							{
								if (yytext[2]!=yytext[3])
									fprintf(veri,".dataa(gnd),\n\t.datab(gnd),\n\t.datad(vcc),\n\t.cin(PATH%dNODE%d),\n\t.combout(PATH%dNODE%d));\n",path,node-1,path,node);
								else
									fprintf(veri,".dataa(vcc),\n\t.datab(gnd),\n\t.datad(vcc),\n\t.cin(PATH%dNODE%d),\n\t.combout(PATH%dNODE%d));\n",path,node-1,path,node);	 
								
							}
						}
						fprintf(veri,"defparam PATH%dNODE%d_t .sumlutc_input = \"cin\";\n",path,node);
						fprintf(veri,"defparam PATH%dNODE%d_t .lut_mask = ",path,node);
						if (carryChainNum%2 == 0)
							fprintf(veri,"\"9617\";\n\n");
						else
							fprintf(veri,"\"698E\";\n\n");
						
						
						carryChainNum++;
							
							
						



					}






				
					done1 : 
							for (i=indexPower;i<strlen(yytext);i++)
								{
									if (yytext[i]==';')
										break;
								}
					
							if(yytext[i+2]=='H')
								fprintf(meta,"HS\n");
							else if (yytext[i+2]=='L')
								fprintf(meta,"LP\n");
					node++; 

}
"Path #"[0-9]+": Setup slack is "("-"?[0-9]+"."?[0-9]+) {	fprintf(loCon,"PATH%d\n",path+1);
			//ib fprintf(meta,"PATH%d\n",path+1);
			 fprintf(meta,yytext);
			 fprintf(meta,"\n",path+1);
			path++; carryChainNum = 0;
		//	free(testVec);
		//	testVec = (char *)malloc(sizeof(char));
			fprintf(veri,"//////////////////////////////////////////////////****************************************PATH%d*****************************************//////////////\n",path);
			//printf("//////////////////////////////////////////////////****************************************PATH%d*****************************************//////////////\n",path);
			
			if (path>1)
			{
				//printf("%d\n",node);
				fprintf(auxil,"reg PATH%dNODE%d, PATH%dNODE%d /* synthesis noprune*/;\nwire ",path-1,0,path-1,node-1);
				
				for (i=1;i<node-2;i++)
					fprintf(auxil,"PATH%dNODE%d, ",path-1,i);
				fprintf(auxil,"PATH%dNODE%d ;\n", path-1,i);
				
				if (testVectorIndex>0)
				{

					fprintf(auxil,"wire [%d:0] testVector%d /*synthesis keep*/ ; \n",testVectorIndex-1,path-1);
					fprintf(auxil,"assign testVector%d = %d\'b",path-1,testVectorIndex);
					for (i=0;i<testVectorIndex;i++)
					{
						fprintf(auxil,"%c",testVec[i]);
					}	
					
					fprintf(auxil," ;\n");
				}
				//printf("%d\n",node);
			}
		free(testVec);
		testVec = (char *)malloc(sizeof(char));
		node=0;
		testVectorIndex = 0;



}

"; IC   ;"[0-9 \t]+";"[A-Z0-9_ \t]+"; "[ \t]+"; "[^; \t]+	{
							//
						//	printf("route\n");
						//	printf(yytext);
						//	printf("route\n");
							if (routingBefore == 1)
							{
								int index = 0;
								for (i=0;i<strlen(yytext);i++)
								{
									if (yytext[i]=='|')
									index =i;
								}
								fprintf(roCon,"\tdest = ( PATH%dNODE%d, DATAB ), route_port = ",path,node);
								fprintf(metaRouting,"*DestinationPATH%dNODE%d\n",path,node);
								for (i=index+1;i<strlen(yytext);i++)
									fprintf(roCon,"%c",yytext[i]-32);
								fprintf(roCon," ;\n}\n");
								routingBefore = 0;
							}

							for (i=8;i<strlen(yytext);i++)
							{
								if (yytext[i]==';')
									if (yytext[i+2]=='F')
										goto done;

							}

							
							fprintf(veri,"cycloneive_lcell_comb PATH%dNODE%d_t (\n\t",path,node);
						//	printf("IC\n");
						//	printf(yytext);
						//	printf("\n");
							switch (yytext[strlen(yytext)-1])
							{
								case 'a' :
									port = portA;
							//		fprintf(veri,".dataa(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,node-1);
									break;
								case 'b' :
									port = portB;
							//		fprintf(veri,".datab(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,node-1);
									break;
								case 'c' :
									port = portC;
							//		fprintf(veri,".datac(PATH%dNODE%d),\n\t.datad(testVector%d[%d]),\n\t",path,node-1,path,node-1);
									break;
								case 'd' :
									port = portD;
							//		fprintf(veri,".datac(testVector%d[%d]),\n\t.datad(PATH%dNODE%d),\n\t",path,node-1,path,node-1);
									break;
								case 'e' :
									port = portE;
									break;
								case 'f' :
									port = portF;
									break;
								case 'n' :
									port = Cin;
									break;
								default : 
									port = 43545;
							}
			
						//	fprintf(veri,".combout(PATH%dNODE%d));\n",path,node);
						//	fprintf(veri,"defparam PATH%dNODE%d_t .sumlutc_input = \"datac\";\n",path,node);
						//	fprintf(veri,"defparam PATH%dNODE%d_t .lut_mask = ",path,node);
						//	if (port == portA)
						//		fprintf(veri,"\"55AA\";\n\n");
						//	if (port == portB)
						//		fprintf(veri,"\"33CC\";\n\n");
						//	if (port == portC)
						//		fprintf(veri,"\"0FF0\";\n\n");
						//	if (port == portD)
						//		fprintf(veri,"\"0FF0\";\n\n");	
						//		printf("cycloneive_lcell_comb PATH%dNODE%d_t (\n\t",path,node); //DEBUG
							done:	i = 0;			
						}

"; RE   ;"[0-9 \t]+"; "[A-Z0-9_]+ {
										
										if (cellFlag == 1) // CELL was seen before this RE so ignore it, it is not really a routing element
										{
											cellFlag = 0;
										}
										else
										{
											if (startRoute==1)
											{
												fprintf(roCon,"signal_name = PATH%dNODE%d \{\n",path,node-1);
												fprintf(metaRouting,"*SourcePATH%dNODE%d \n",path,node-1); // metadata to be further processed by c++ tool
												routingBefore = 1;									
											}
											
											i = 0;
											counter = 0;
											
											for (i=0;i<strlen(yytext);i++)
											{
												if (yytext[i]==';')
													counter++;
													
												if (counter==3)
													break;
											}
											i+=2;
											
											fprintf(roCon,"\t");
											
											while(!(yytext[i]=='_' && yytext[i+1]=='X'))
											{
												fprintf(roCon,"%c",yytext[i]);    
												fprintf(metaRouting,"%c",yytext[i]);
												i++;
											}			
											
											fprintf(roCon,":");		
											fprintf(metaRouting,":");														
											for (;i<strlen(yytext);i++)
											{
												if (yytext[i]!='_')
												{
													if (yytext[i]=='N')
													{
														fprintf(roCon,"S");
														fprintf(metaRouting,"S");
													}
													else
													{
														fprintf(roCon,"%c",yytext[i]);
														fprintf(metaRouting,"%c",yytext[i]);
													}
												}
											}
											
											fprintf(roCon,";\n");
											fprintf(metaRouting,"\n");
											startRoute=0;
											
										}
																




}



. ;
[ \n] ;
%%


main(argc, argv)
int argc;
char **argv;
{
  if(argc > 1) {
    if(!(yyin = fopen(argv[1], "r"))) {
      perror(argv[1]);
      return (1);
    }
  }
loCon = fopen("../fils/locationConstraint", "w");
roCon = fopen("../fils/routingConstraint", "w");
veri = fopen("../fils/verilo", "w");
auxil = fopen("../fils/auxill", "w");
meta = fopen("../fils/meta", "w");
metaRouting =  fopen("../fils/metaRouting", "w");
  yylex();

				fprintf(auxil,"reg PATH%dNODE%d, PATH%dNODE%d /* synthesis noprune*/;\nwire ",path,0,path,node-1);
				
				for (i=1;i<node-2;i++)
					fprintf(auxil,"PATH%dNODE%d, ",path,i);
				fprintf(auxil,"PATH%dNODE%d ;\n", path,i);
				
				if (testVectorIndex>0)
				{
					fprintf(auxil,"wire [%d:0] testVector%d /*synthesis keep*/ ; \n",testVectorIndex-1,path);
					fprintf(auxil,"assign testVector%d = %d\'b",path,testVectorIndex);
					for (i=0;i<testVectorIndex;i++)
					{
						fprintf(auxil,"%c",testVec[i]);
					}	
					
					fprintf(auxil," ;\n");
				}
  printf("DONE\n");
}
