
#ifndef _DAFNEDATAPUBLISHER_H_
#define _DAFNEDATAPUBLISHER_H_
#include <string>
#include <vector>

#include <iostream>
#include <sstream>
namespace DafneData
{
	/*
	template <typename L> L getDefault(); 
	

	template <typename L> std::string getSimpleJVal(std::string val) ;
	
*/
	template <typename L>  L getDefault() {return 0;}
	template <> std::string  getDefault<std::string >();

	template <typename L>  std::string getSimpleJVal(std::string val) {return val;};
	template <> std::string getSimpleJVal<std::string >(std::string val);
	

	template <class T> class DafneSingleData
	{
	public:  
	
		T  innerValue;
		std::string description;
		std::string measureUnit;
		bool initialized;
	public:
		DafneSingleData(const char* desc, const char* measureUnit)
		{
			this->description = desc;
			this->measureUnit = measureUnit;
			this->setDafneDefaultData();
			initialized = false;
		};
		void setDafneDefaultData()
		{
			this->innerValue = getDefault<T>();
		}


		void setDafneData(T value)
		{
			this->innerValue = value;
			this->initialized = true;
		}
		DafneSingleData<T> operator=(T val)
		{
			this->innerValue = val;
			this->initialized = true;
			return *this;
		}
     
	    operator T() {return (T) this->innerValue;}

		friend std::ostream& operator<< (std::ostream& stream, const DafneSingleData& data)
		{
			stream << data.innerValue;
			return stream;
		}

		std::string to_string(int mode=0)
		{
			if ((!initialized) && mode != -1)
				return this->description + " uninitialized";
			else
			{
				std::stringstream ss;
				if (mode <= 0)
				{
					ss << this->innerValue;
				}
				else if (mode == 1)
				{
					ss << this->description << "  "<< this->innerValue;
				}
				return ss.str();
			}

		}

		std::string toJSonValueString(bool complete=false)
		{
			if (!complete)
				return this->toJSonValueStringDef();
			else
				return this->toJSonValueStringDefComplete();
		}

	private:
		std::string toJSonValueStringDef()
		{
			std::string  val= this->to_string(-1);
			return  getSimpleJVal<T>(val);
		}


	private:	std::string toJSonValueStringDefComplete()
			{
				std::string ret = "{\n";
				ret += "\t\"value:\":" + this->toJSonValueString(false) + ",\n";
				ret += "\t\"units\":\"" + this->measureUnit + "\",\n";
				ret += "\t\"description\":\"" + this->description + "\"\n";
				ret += "}";
				return ret;
			}

		

	};
	class DafneDataToShow
	{
	public:
		int32_t modeToPrint=0;
		
		DafneData::DafneSingleData<uint64_t> timestamp =  DafneSingleData<uint64_t>("timestamp of data", "UNIX TIME");
		DafneSingleData<int32_t>  dafne_status = DafneSingleData<int32_t>("Dafne Status", "enumeration");
		DafneSingleData<double>   i_ele = DafneSingleData<double>("Electron Current", "mA");
		DafneSingleData<double>   i_pos = DafneSingleData<double>("Positron Current", "mA");
		DafneSingleData<int32_t>  nbunch_ele = DafneSingleData<int32_t>("electron bunch number", "pure numeric");
		DafneSingleData<int32_t>  nbunch_pos = DafneSingleData<int32_t>("positron bunch number", "pure numeric");
		DafneSingleData<uint32_t> fill_pattern_ele = DafneSingleData<uint32_t>("fill pattern electron", "UInt32");
		DafneSingleData<uint32_t> fill_pattern_pos = DafneSingleData<uint32_t>("fill pattern positron", "UInt32");
		DafneSingleData<int32_t>  lifetime_ele = DafneSingleData<int32_t>("electrons lifetime", "seconds");
		DafneSingleData<int32_t>  lifetime_pos = DafneSingleData<int32_t>("positrons lifetime", "seconds");
		DafneSingleData<double>   sx_ele = DafneSingleData<double>("sigma X electrons", "um");
		DafneSingleData<double>   sy_ele = DafneSingleData<double>("sigma Y electrons", "um");
		DafneSingleData<double>   th_ele = DafneSingleData<double>("tilt angle electrons", "rad");
		DafneSingleData<double>   sx_pos = DafneSingleData<double>("sigma X positrons", "um");
		DafneSingleData<double>   sy_pos = DafneSingleData<double>("sigma Y positrons", "um");
		DafneSingleData<double>   th_pos = DafneSingleData<double>("tilt angle positrons", "rad");
		DafneSingleData<double>   rf = DafneSingleData<double>("DAFNE Radiofrequency", "Hz");

		DafneSingleData<double>	  VUGEL101 = DafneSingleData<double>("Vacuometro elettroni Long1 num1", "torr");
		DafneSingleData<double>   VUGEL102 = DafneSingleData<double>("Vacuometro elettroni Long1 num2", "torr");
		DafneSingleData<double>   VUGEL103 = DafneSingleData<double>("Vacuometro elettroni Long1 num3", "torr");

		DafneSingleData<double>	  VUGEL201 = DafneSingleData<double>("Vacuometro elettroni Long2 num1", "torr");
		DafneSingleData<double>   VUGEL202 = DafneSingleData<double>("Vacuometro elettroni Long2 num2", "torr");
		DafneSingleData<double>   VUGEL203 = DafneSingleData<double>("Vacuometro elettroni Long2 num3", "torr");

		DafneSingleData<double>   VUGES101 = DafneSingleData<double>("Vacuometro elettroni Short1 num1", "torr");
		DafneSingleData<double>   VUGES102 = DafneSingleData<double>("Vacuometro elettroni Short1 num2", "torr");
		DafneSingleData<double>   VUGES103 = DafneSingleData<double>("Vacuometro elettroni Short1 num3", "torr");

		DafneSingleData<double>   VUGES201 = DafneSingleData<double>("Vacuometro elettroni Short2 num1", "torr");
		DafneSingleData<double>   VUGES202 = DafneSingleData<double>("Vacuometro elettroni Short2 num2", "torr");
		DafneSingleData<double>   VUGES203 = DafneSingleData<double>("Vacuometro elettroni Short2 num3", "torr");

		DafneSingleData<double>   VUGPL101 = DafneSingleData<double>("Vacuometro positroni Long1 num1", "torr");
		DafneSingleData<double>   VUGPL102 = DafneSingleData<double>("Vacuometro positroni Long1 num2", "torr");
		DafneSingleData<double>   VUGPL103 = DafneSingleData<double>("Vacuometro positroni Long1 num3", "torr");

		DafneSingleData<double> VUGPL201 = DafneSingleData<double>("Vacuometro positroni Long2 num1", "torr");
		DafneSingleData<double> VUGPL202 = DafneSingleData<double>("Vacuometro positroni Long2 num2", "torr");
		DafneSingleData<double> VUGPL203 = DafneSingleData<double>("Vacuometro positroni Long2 num3", "torr");

		DafneSingleData<double> VUGPS101 = DafneSingleData<double>("Vacuometro positroni Short1 num1", "torr");
		DafneSingleData<double> VUGPS102 = DafneSingleData<double>("Vacuometro positroni Short1 num2", "torr");
		DafneSingleData<double> VUGPS103 = DafneSingleData<double>("Vacuometro positroni Short1 num3", "torr");

		DafneSingleData<double> VUGPS201 = DafneSingleData<double>("Vacuometro positroni Short2 num1", "torr");
		DafneSingleData<double> VUGPS202 = DafneSingleData<double>("Vacuometro positroni Short2 num2", "torr");
		DafneSingleData<double> VUGPS203 = DafneSingleData<double>("Vacuometro positroni Short2 num3", "torr");

		
		DafneSingleData<double> Ty_ele = DafneSingleData<double>("Temperatura camera Y elettroni", "Celsius");
		DafneSingleData<double> Ty_pos = DafneSingleData<double>("Temperatura camera Y positroni", "Celsius");

		DafneSingleData<double> R2_CCAL = DafneSingleData<double>("Rate totale di coincidenze misurata dal CCALT", "Hz");
		DafneSingleData<double> R2_BKG = DafneSingleData<double>("Rate di coincidenze di fondo misurata dal CCALT", "Hz");
		DafneSingleData<double> Dead_TC = DafneSingleData<double>("Frazione di Tempo morto del DAQ CCALT", "pure numeric");
		DafneSingleData<double> R1C_ele = DafneSingleData<double>("Rate di conteggi sul lato illuminato dagli elettroni", "Hz");
		DafneSingleData<double> R1C_pos = DafneSingleData<double>(" Rate di conteggi sul lato illuminato dai positroni", "Hz");
		DafneSingleData<double> lum_CCAL = DafneSingleData<double>("Luminosita'", "10^30 cm-2 s-1");


		public: void PrintOnConsole();
		private: std::string asJSonKey(std::string name);

		public: void PrintAsJson(std::string outFilePath,bool complete);
		public: bool PrintAsRowtxt(std::string outFilePath);
		public:  bool ReadFromNewDafne(std::string newdafnepath);

	};
}
#endif