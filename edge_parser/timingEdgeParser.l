%{
#define portA (0)
#define portB (1)
#define portC  (2)
#define portD (3)
#define Cin (4)
#define Cout (5)
#define Combout (6)
#define normal (0)
#define arith (1)
int num_of_elem =0;
int FLAG = 0;
int cellFlag = 0;
int startRoute =0;
int path =0;
int node =0;


int firstFF = 0;
int Tco = 0;





FILE *metaEdge;
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
";"[0-9. \t]+"; "("R"|"F")("R "|"F ")"; CELL ;"[0-9 \t]+";"[A-Z0-9_ \t]+"; "[^; \t]+ {FLAG =1;startRoute=1;
					int index = 0;
					cellFlag = 1;
					int j = 0;
					if(firstFF == 1)
					{
						firstFF = 0;
						
						index = 1;
						for (j=0;j<4; )
						{
							for (i=index;i<strlen(yytext);i++)
							{
								if (yytext[i]==';')
								{	
									index = i+1;
									j++;
									break;	
								}
							}
						}
						
						index++;
						
						fprintf(metaEdge,"*");
					
						for (i=index;i<strlen(yytext);i++)
						{
							if (yytext[i]==' ')
								break;

							fprintf (metaEdge,"%c",yytext[i]);
						

						}
						
						for (i=1;i<strlen(yytext);i++)
						{
							if (yytext[i]==';')
								break;
						}
						
						fprintf(metaEdge,"\n");
						fprintf(metaEdge,"%c",yytext[i+2]);
						fprintf(metaEdge,"%c",yytext[i+3]);
						fprintf(metaEdge,"\n");
	
						goto done1;
					}
					
					
																														
					
						index = 1;
						for (j=0;j<4; )
						{
							for (i=index;i<strlen(yytext);i++)
							{
								if (yytext[i]==';')
								{	
									index = i+1;
									j++;
									break;	
								}
							}
						}
						
						index++;
					fprintf(metaEdge,"*");
					
					for (i=index;i<strlen(yytext);i++)
					{
							if (yytext[i]==' ')
								break;

						fprintf (metaEdge,"%c",yytext[i]);
						

					}

					//printf (" -to \"PATH%dNODE%d\"\n", path,node); //debug
					if (yytext[index]=='F')
					{
						fprintf(metaEdge,"\n");
						fprintf(metaEdge,"FF d q\n");
						for (i=1;i<strlen(yytext);i++)
						{
							if (yytext[i]==';')
								break;
						}
						
						fprintf(metaEdge,"%c",yytext[i+2]);
						fprintf(metaEdge,"%c",yytext[i+3]);
						fprintf(metaEdge,"\n");

						for (i=1;i<strlen(yytext);i++)
						{
							if (yytext[i]==';')
								break;
							if (yytext[i]==' ')
								continue;	
							fprintf (metaEdge,"%c",yytext[i]);					
						}
						fprintf(metaEdge,"\n");
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
		
					fprintf(metaEdge,"\n");	
					fprintf(metaEdge,"LUT %d %d\n", port, portOut);
					// write the non/inverting behaviour of the signal to the meta file
						for (i=1;i<strlen(yytext);i++)
						{
							if (yytext[i]==';')
								break;
						}
						
						fprintf(metaEdge,"%c",yytext[i+2]);
						fprintf(metaEdge,"%c",yytext[i+3]);
						fprintf(metaEdge,"\n");

					for (i=1;i<strlen(yytext);i++)
					{
						if (yytext[i]==';')
							break;
						if (yytext[i]==' ')
							continue;	
						fprintf (metaEdge,"%c",yytext[i]);					
					}
					fprintf(metaEdge,"\n");


				
					done1 : i = 0; 

}



";"[0-9. \t]+";    ; uTco ;"[0-9 \t]+"; "[A-Z0-9_]+ {
		fprintf(metaEdge,"______________________________________________________________**_________\n");
		
		for (i=1;i<strlen(yytext);i++)
		{
			if (yytext[i]==';')
				break;
			if (yytext[i]==' ')
				continue;
			fprintf (metaEdge,"%c",yytext[i]);					
		}
fprintf(metaEdge,"\n");
}












"Path #"[0-9]+": Setup slack is "("-"?[0-9]+"."?[0-9]+) {	
		firstFF = 1;
	//	printf("helo\n");


}

";"[0-9. \t]+"; "("R"|"F")("R "|"F ")"; IC   ;"[0-9 \t]+";"[A-Z0-9_ \t]+"; "[^; \t]+	{


							switch (yytext[strlen(yytext)-1])
							{
								case 'a' :
									port = portA;
									break;
								case 'b' :
									port = portB;
									break;
								case 'c' :
									port = portC;
									break;
								case 'd' :
									port = portD;
									break;
								case 'n' :
									port = Cin;
									break;
								default : 
									port = 43545;
							}
						



						for (i=1;i<strlen(yytext);i++)
						{
							if (yytext[i]==';')
								break;
							if (yytext[i]==' ')
								continue;	
							fprintf (metaEdge,"%c",yytext[i]);					
						}

						fprintf(metaEdge,"\n");



						
							done:	i = 0;			
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
//../fils/
//metaEdge = fopen("../fils/metaEdge", "w");
metaEdge = fopen(argv[2], "w");
  yylex();

			//	fprintf(auxil,"reg PATH%dNODE%d, PATH%dNODE%d /* synthesis noprune*/;\nwire ",path,0,path,node-1);
				
			//	for (i=1;i<node-2;i++)
			//		fprintf(auxil,"PATH%dNODE%d, ",path,i);
			//	fprintf(auxil,"PATH%dNODE%d ;\n", path,i);
				
			//	if (testVectorIndex>0)
				//{
				//	fprintf(auxil,"wire [%d:0] testVector%d /*synthesis keep*/ ; \n",testVectorIndex-1,path);
				//	fprintf(auxil,"assign testVector%d = %d\'b",path,testVectorIndex);
				//	for (i=0;i<testVectorIndex;i++)
				//	{
				//		fprintf(auxil,"%c",testVec[i]);
				//	}	
					
				//	fprintf(auxil," ;\n");
				//}
  printf("DONE\n");
}
