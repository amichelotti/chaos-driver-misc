#include <iostream>
#include <fstream>
#include "DafneDataPublisher.h"
#include <stdlib.h>
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
using namespace std;
using namespace DafneData;
vector<string> split( 	const  string  & theString, 	const  string  & theDelimiter)
{
	vector<string> retVec;
	size_t  start = 0, end = 0;
	if (theDelimiter.size() == 0)
		return retVec;
	while (end != string::npos)
	{
		end = theString.find(theDelimiter, start);

		// If at end, use length=maxLength.  Else use length=end-start.
		retVec.push_back(theString.substr(start,
			(end == string::npos) ? string::npos : end - start));

		// If at end, use start=maxSize.  Else use start=end+delimiter.
		start = ((end > (string::npos - theDelimiter.size()))
			? string::npos : end + theDelimiter.size());
	}
	return retVec;
}

	template <>  std::string  DafneData::getDefault<std::string>() {return "";}
	template <>  std::string DafneData::getSimpleJVal<std::string>(std::string val) {return "\""+val+"\"";}




	//DafneSingleData<std::string> testo = DafneSingleData<std::string>("prova", "10^30 cm-2 s-1");

   void DafneDataToShow::PrintOnConsole()
	{
	  int md = this->modeToPrint;
	  std::cout << this->dafne_status.to_string(md) << std::endl;
	  std::cout << this->i_ele.to_string(md) << std::endl;
	  std::cout <<  this->i_pos.to_string(md) << std::endl;
	 
	  std::cout << this->fill_pattern_ele.to_string(md) << std::endl;
	  std::cout << this->fill_pattern_pos.to_string(md) << std::endl;
	  std::cout << this->nbunch_ele.to_string(md) << std::endl;
	  std::cout << this->nbunch_pos.to_string(md) << std::endl;
	  std::cout << this->rf.to_string(md) << std::endl;
	  std::cout << this->timestamp.to_string(md) << std::endl;
	  std::cout << this->Dead_TC.to_string(md) << std::endl;
	  

	}

std::string DafneDataToShow::asJSonKey(std::string name)
{
	return "\"" + name + "\":";
}

bool DafneDataToShow::PrintAsJson(std::string outFilePath,bool complete)
{

#define PRINTJVAL(namestr)  outFile << asJSonKey(#namestr )   << namestr.toJSonValueString(complete)

	
	std::ofstream outFile(outFilePath);
	if (outFile.is_open())
	{
		outFile << "{" << std::endl;
		//outFile << asJSonKey("timestamp") << this->timestamp.toJSonValueString() <<","<<std::endl;
		PRINTJVAL(timestamp) << "," << std::endl;
		PRINTJVAL(i_ele) << "," << std::endl;
		PRINTJVAL(i_pos) << "," << std::endl;
		PRINTJVAL(dafne_status) << "," << std::endl;
		PRINTJVAL(nbunch_ele) << "," << std::endl;
		PRINTJVAL(nbunch_pos) << "," << std::endl;
		PRINTJVAL(fill_pattern_ele) << "," << std::endl;
		PRINTJVAL(fill_pattern_pos) << "," << std::endl;
		PRINTJVAL(lifetime_ele) << "," << std::endl;
		PRINTJVAL(lifetime_pos) << "," << std::endl;
		PRINTJVAL(sx_ele) << "," << std::endl;
		PRINTJVAL(sy_ele) << "," << std::endl;
		PRINTJVAL(th_ele) << "," << std::endl;
		PRINTJVAL(sx_pos) << "," << std::endl;
		PRINTJVAL(sy_pos) << "," << std::endl;
		PRINTJVAL(th_pos) << "," << std::endl;
		PRINTJVAL(rf) << "," << std::endl;  //

		PRINTJVAL(VUGEL101) << "," << std::endl;
		PRINTJVAL(VUGEL102) << "," << std::endl;
		PRINTJVAL(VUGEL103) << "," << std::endl;
		PRINTJVAL(VUGEL201) << "," << std::endl;
		PRINTJVAL(VUGEL202) << "," << std::endl;
		PRINTJVAL(VUGEL203) << "," << std::endl;
		PRINTJVAL(VUGES101) << "," << std::endl;
		PRINTJVAL(VUGES102) << "," << std::endl;
		PRINTJVAL(VUGES103) << "," << std::endl;
		PRINTJVAL(VUGES201) << "," << std::endl;
		PRINTJVAL(VUGES202) << "," << std::endl;
		PRINTJVAL(VUGES203) << "," << std::endl;

		PRINTJVAL(VUGPL101) << "," << std::endl;
		PRINTJVAL(VUGPL102) << "," << std::endl;
		PRINTJVAL(VUGPL103) << "," << std::endl;
		PRINTJVAL(VUGPL201) << "," << std::endl;
		PRINTJVAL(VUGPL202) << "," << std::endl;
		PRINTJVAL(VUGPL203) << "," << std::endl;
		PRINTJVAL(VUGPS101) << "," << std::endl;
		PRINTJVAL(VUGPS102) << "," << std::endl;
		PRINTJVAL(VUGPS103) << "," << std::endl;
		PRINTJVAL(VUGPS201) << "," << std::endl;
		PRINTJVAL(VUGPS202) << "," << std::endl;
		PRINTJVAL(VUGPS203) << "," << std::endl;

		PRINTJVAL(Ty_ele) << "," << std::endl;
		PRINTJVAL(Ty_pos) << "," << std::endl;
		PRINTJVAL(R2_CCAL) << "," << std::endl;
		PRINTJVAL(R2_BKG) << "," << std::endl;
		PRINTJVAL(Dead_TC) << "," << std::endl;
		PRINTJVAL(R1C_ele) << "," << std::endl;
		PRINTJVAL(R1C_pos) << "," << std::endl;
		
		PRINTJVAL(lum_CCAL) << std::endl;
		

		outFile << "}";
		outFile.close();
	}
	else
	{
	   return false;
	}
	
#undef PRINTJVAL	
return true;
}

bool DafneDataToShow::AppendSiddhartaFile(std::string siddhartaMainPath)
{
	time_t t = time(0);
	struct tm * now = localtime( & t );
    char buffer [80];
    strftime (buffer,80,"%Y%m%d.stat",now);
	std::string currFile=siddhartaMainPath+ "/"+ std::string(buffer);
	std::fstream outFile;
	//if (outFile.open(currFile,std::ios_base::)
	outFile.open (currFile,std::fstream::out | std::fstream::app);
	if (outFile.is_open())
	{
		this->PrintAsRawtxt(outFile);
		outFile.close();
	}
	else
	{
		return false;
	}
	return true;
	




}

bool DafneDataToShow::PrintAsRawtxt(std::fstream& outFile)
{
	
	if (outFile.is_open())
	{
		outFile << (timestamp) << " ";
		outFile << (i_ele) << " ";
		outFile << (i_pos) << " ";
		outFile << (dafne_status) << " ";
		outFile << (nbunch_ele) << " ";
		outFile << (nbunch_pos) << " ";
		outFile << (fill_pattern_ele) << " ";
		outFile << (fill_pattern_pos) << " ";
		outFile << (lifetime_ele) << " ";
		outFile << (lifetime_pos) << " ";
		outFile << (sx_ele) << " ";
		outFile << (sy_ele) << " ";
		outFile << (th_ele) << " ";
		outFile << (sx_pos) << " ";
		outFile << (sy_pos) << " ";
		outFile << (th_pos) << " ";
		outFile << (rf) << " ";  //

		outFile << (VUGEL101) << " ";
		outFile << (VUGEL102) << " ";
		outFile << (VUGEL103) << " ";
		outFile << (VUGEL201) << " ";
		outFile << (VUGEL202) << " ";
		outFile << (VUGEL203) << " ";
		outFile << (VUGES101) << " ";
		outFile << (VUGES102) << " ";
		outFile << (VUGES103) << " ";
		outFile << (VUGES201) << " ";
		outFile << (VUGES202) << " ";
		outFile << (VUGES203) << " ";

		outFile << (VUGPL101) << " ";
		outFile << (VUGPL102) << " ";
		outFile << (VUGPL103) << " ";
		outFile << (VUGPL201) << " ";
		outFile << (VUGPL202) << " ";
		outFile << (VUGPL203) << " ";
		outFile << (VUGPS101) << " ";
		outFile << (VUGPS102) << " ";
		outFile << (VUGPS103) << " ";
		outFile << (VUGPS201) << " ";
		outFile << (VUGPS202) << " ";
		outFile << (VUGPS203) << " ";

		outFile << (Ty_ele) << " ";
		outFile << (Ty_pos) << " ";
		outFile << (R2_CCAL) << " ";
		outFile << (R2_BKG) << " ";
		outFile << (Dead_TC) << " ";
		outFile << (R1C_ele) << " ";
		outFile << (R1C_pos) << " ";

		outFile << (lum_CCAL) << std::endl;
		
	}
	return true;
}

bool DafneDataToShow::ReadFromNewDafne(std::string newdafnepath)
{
	std::ifstream newdafne(newdafnepath);
	if (newdafne.is_open())
	{
		std::string line;

		if (getline(newdafne, line))
		{
			//cout << "Read: " << line << endl;
			std::vector<std::string> tokenized = split(line, " ");
			std::vector<std::string> values;
			for (unsigned int i = 0; i < tokenized.size(); i++)
			{
				if ((tokenized[i].size() > 0) && (tokenized[i] != " ") && (tokenized[i] != "  ") && (tokenized[i] != "   "))
					values.push_back(tokenized[i]);
			}
			uint32_t tmp;
			for (uint32_t i = 0; i < values.size(); i++)
			{
				switch (i)
				{
					case 0:		this->timestamp =        (uint64_t)atol(values[i].c_str()); break;
					case 1:		this->i_ele		=        atof(values[i].c_str()); break;
					case 2:		this->i_pos		=        atof(values[i].c_str()); break;
					case 6:		this->nbunch_ele =       atoi(values[i].c_str()); break;
					case 7:		sscanf(values[i].c_str(), "%x", &tmp); this->fill_pattern_ele = tmp; break;  //FORSE
					case 11:	this->nbunch_pos =       atoi(values[i].c_str()); break;
					case 12:	sscanf(values[i].c_str(), "%x", &tmp); this->fill_pattern_pos = tmp; break;    //FORSE
					case 18:	this->dafne_status =     atoi(values[i].c_str()); break;  
					case 20:	this->lifetime_ele =     atoi(values[i].c_str()); break;
					case 21:	this->lifetime_pos =	 atoi(values[i].c_str()); break;
					case 25:	this->rf			=    atof(values[i].c_str()); break;
					default: break;

				}

			}
			return true;
		}
	}
	else
	{
		std::cout << "File " << newdafnepath << " Not Found!\n";
		return false;

	}
	return false;
}

bool DafneDataToShow::ReadFromFast(std::string fastfilepath)
{
	std::ifstream newdafne(fastfilepath);
	if (newdafne.is_open())
	{
		std::string line;

		if (getline(newdafne, line))
		{
			std::vector<std::string> tokenized = split(line, " ");
			std::vector<std::string> values;
			for (unsigned int i = 0; i < tokenized.size(); i++)
			{
				if ((tokenized[i].size() > 0) && (tokenized[i] != " ") && (tokenized[i] != "  ") && (tokenized[i] != "   "))
					values.push_back(tokenized[i]);
			}
			uint32_t tmp;
			for (uint32_t i = 0; i < values.size(); i++)
			{
				switch (i)
				{
					case 0: this->timestamp = (uint64_t)atol(values[i].c_str()); break;
					case 1: this->i_ele =  atof(values[i].c_str());break; //DUPLIC
					case 2: this->i_pos = atof(values[i].c_str());break; //DUPLIC
					case 7: sscanf(values[i].c_str(), "%x", &tmp); this->fill_pattern_ele = tmp; break;  //DUPLIC
					case 8:	sscanf(values[i].c_str(), "%x", &tmp); this->fill_pattern_pos = tmp; break;    //DUPLIC
					case 20: this->rf = atof(values[i].c_str());break;
					case 34: this->VUGPL101 = atof(values[i].c_str());break;//G
					case 35: this->VUGPS101 = atof(values[i].c_str());break;//G
					case 36: this->VUGPS201 = atof(values[i].c_str());break;//G
					case 37: this->VUGEL201 = atof(values[i].c_str());break;
					case 38: this->VUGEL202 = atof(values[i].c_str());break;
					case 39: this->VUGEL203 = atof(values[i].c_str());break;
					case 40: this->VUGES101 = atof(values[i].c_str());break;
					case 41: this->VUGES102 = atof(values[i].c_str());break;
					case 42: this->VUGES103 = atof(values[i].c_str());break;
					case 43: this->VUGES201 = atof(values[i].c_str());break;
					case 44: this->VUGES202 = atof(values[i].c_str());break;
					case 45: this->VUGES203 = atof(values[i].c_str());break;
					default: break;
				}
			}
			return true;
		}
		else
		{
			/* FILE fastfilepath empty*/
			return false;
		}

	}
	else
	{
		/* FILE NOT FOUND*/
		return false;
	}

}

#ifndef CHAOS
int main()
{
    std::string newdafnepath= "/u2/data/fast/newdafne.stat";
	std::string outFilePath = "/home/aduffizi/Documenti/DafnePublisher/DafneJson.json";
	std::string outFileTxtPath = "/home/aduffizi/Documenti/DafnePublisher/";
	DafneDataToShow   DATO;
	DATO.modeToPrint = 1;
	bool completeJson = false;
	DATO.ReadFromNewDafne(newdafnepath);
	DATO.PrintAsJson(outFilePath, completeJson);
	DATO.AppendSiddhartaFile(outFileTxtPath);
	return 0;
}
#endif
